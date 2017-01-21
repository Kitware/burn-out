/*ckwg +5
 * Copyright 2008-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/* Standard includes */
#include <assert.h>
#include <math.h>
#include <stdlib.h>   /* malloc(), realloc() */
#include <stdio.h>

/* Our includes */
#include "base.h"
#include "error.h"
#include "convolve.h"
#include "klt_util.h"   /* printing */

#define MAX_KERNEL_WIDTH        71

#if VIDTK_SSE2
#include <xmmintrin.h>
#include <string.h>
// Also instruction for aligning stack memory is compiler dependent
#if defined(VCL_GCC)
#define SSE_STACK_ALIGNED(x)  __attribute__((aligned(x)))
#elif defined(VCL_VC) || defined(VCL_ICC)
#define SSE_STACK_ALIGNED(x)  __declspec(align(x))
#else
# define SSE_STACK_ALIGNED(x)
#endif
#endif


typedef struct  {
  int width;
  float data[MAX_KERNEL_WIDTH];
}  ConvolutionKernel;

/* Kernels */
static ConvolutionKernel gauss_kernel;
static ConvolutionKernel gaussderiv_kernel;
static float sigma_last = -10.0;


/*********************************************************************
 * _KLTToFloatImage
 *
 * Given a pointer to image data (probably unsigned chars), copy
 * data to a float image.
 */

void _KLTToFloatImage(
  KLT_PixelType *img,
  int ncols, int nrows,
  _KLT_FloatImage floatimg)
{
  KLT_PixelType *ptrend = img + ncols*nrows;
  float *ptrout = floatimg->data;

  /* Output image must be large enough to hold result */
  assert(floatimg->ncols >= ncols);
  assert(floatimg->nrows >= nrows);

  floatimg->ncols = ncols;
  floatimg->nrows = nrows;

  while (img < ptrend)  *ptrout++ = (float) *img++;
}


/*********************************************************************
 * _computeKernels
 */

static void _computeKernels(
  float sigma,
  ConvolutionKernel *gauss,
  ConvolutionKernel *gaussderiv)
{
  const float factor = 0.01f;   /* for truncating tail */
  int i;

  assert(MAX_KERNEL_WIDTH % 2 == 1);
  assert(sigma >= 0.0);

  /* Compute kernels, and automatically determine widths */
  {
    const int hw = MAX_KERNEL_WIDTH / 2;
    float max_gauss = 1.0f, max_gaussderiv = (float) (sigma*exp(-0.5f));

    /* Compute gauss and deriv */
    for (i = -hw ; i <= hw ; i++)  {
      gauss->data[i+hw]      = (float) exp(-i*i / (2*sigma*sigma));
      gaussderiv->data[i+hw] = -i * gauss->data[i+hw];
    }

    /* Compute widths */
    gauss->width = MAX_KERNEL_WIDTH;
    for (i = -hw ; fabs(gauss->data[i+hw] / max_gauss) < factor ;
         i++, gauss->width -= 2);
    gaussderiv->width = MAX_KERNEL_WIDTH;
    for (i = -hw ; fabs(gaussderiv->data[i+hw] / max_gaussderiv) < factor ;
         i++, gaussderiv->width -= 2);
    if (gauss->width == MAX_KERNEL_WIDTH ||
        gaussderiv->width == MAX_KERNEL_WIDTH)
      KLTError("(_computeKernels) MAX_KERNEL_WIDTH %d is too small for "
               "a sigma of %f", MAX_KERNEL_WIDTH, sigma);
  }

  /* Shift if width less than MAX_KERNEL_WIDTH */
  for (i = 0 ; i < gauss->width ; i++)
    gauss->data[i] = gauss->data[i+(MAX_KERNEL_WIDTH-gauss->width)/2];
  for (i = 0 ; i < gaussderiv->width ; i++)
    gaussderiv->data[i] = gaussderiv->data[i+(MAX_KERNEL_WIDTH-gaussderiv->width)/2];
  /* Normalize gauss and deriv */
  {
    const int hw = gaussderiv->width / 2;
    float den;

    den = 0.0;
    for (i = 0 ; i < gauss->width ; i++)  den += gauss->data[i];
    for (i = 0 ; i < gauss->width ; i++)  gauss->data[i] /= den;
    den = 0.0;
    for (i = -hw ; i <= hw ; i++)  den -= i*gaussderiv->data[i+hw];
    for (i = -hw ; i <= hw ; i++)  gaussderiv->data[i+hw] /= den;
  }

  sigma_last = sigma;
}


/*********************************************************************
 * _KLTGetKernelWidths
 *
 */

void _KLTGetKernelWidths(
  float sigma,
  int *gauss_width,
  int *gaussderiv_width)
{
  _computeKernels(sigma, &gauss_kernel, &gaussderiv_kernel);
  *gauss_width = gauss_kernel.width;
  *gaussderiv_width = gaussderiv_kernel.width;
}


/*********************************************************************
 * _convolveImageHoriz
 */
#ifdef VIDTK_SSE2
static void _convolveImageHoriz_SSE(
  _KLT_FloatImage imgin,
  ConvolutionKernel kernel,
  _KLT_FloatImage imgout)
{
  float *ptrrow = imgin->data;           /* Points to row's first pixel */
  float *ptrout = imgout->data, /* Points to next output pixel */
  *ppp;
  int radius = kernel.width / 2;
  int ncols = imgin->ncols, nrows = imgin->nrows;
  int i, j, k, s;
  const int middle_width = ncols - radius;

  SSE_STACK_ALIGNED(16) float temp[4];
  __m128 kernel_data[MAX_KERNEL_WIDTH];
  __m128 sse_sum;
  __m128 sse_pixels;
  /* Kernel width must be odd */
  assert(kernel.width % 2 == 1);
  i = 0;
  for(k = kernel.width-1 ; k >= 0 ; k--, ++i)
  {
    kernel_data[i] = _mm_set1_ps(kernel.data[k]);
  }

  /* Must read from and write to different images */
  assert(imgin != imgout);

  /* Output image must be large enough to hold result */
  assert(imgout->ncols >= imgin->ncols);
  assert(imgout->nrows >= imgin->nrows);

  /* For each row, do ... */
  for (j = 0 ; j < nrows ; j++)  {

    /* Zero leftmost columns */
    for (i = 0 ; i < radius ; i++)
    {
      *ptrout++ = 0.0f;
    }

    /* Convolve middle columns with kernel */
    /*first aligned such that ith is on 16 */
    s = 4 - (((unsigned long)ptrout)%16)/4;
    assert(s > 0 && s <= 4);
    if( s != 4 )
    {
      ppp = ptrrow + i - radius;
      i += s;
      sse_sum = _mm_setzero_ps();
      for (k = 0; k < kernel.width; ++k)
      {
        sse_pixels = _mm_loadu_ps(ppp++);
        sse_sum = _mm_add_ps(sse_sum, _mm_mul_ps(kernel_data[k],sse_pixels));
      }
      _mm_store_ps(temp, sse_sum);
      memcpy( ptrout, temp, s*sizeof(float) );
      ptrout+=s;
    }
    assert(((unsigned long)ptrout & (15)) == 0);
    /*Now aligned, we are going to do 4 pixels at once*/
    for ( ; i < middle_width - 4; i+=4)
    {
      ppp = ptrrow + i - radius;
      sse_sum = _mm_setzero_ps();
      for (k = 0; k < kernel.width; ++k)
      {
        sse_pixels = _mm_loadu_ps(ppp++);
        sse_sum = _mm_add_ps(sse_sum, _mm_mul_ps(kernel_data[k],sse_pixels));
      }
      _mm_store_ps(ptrout, sse_sum);
      ptrout+=4;
    }
    /*clean up remaining*/
    s = middle_width - i;//Number left
    assert(s<=4 && s >= 0);
    if(s > 0)
    {
      ppp = ptrrow + i - radius;
      i += s;
      sse_sum = _mm_setzero_ps();
      for (k = 0; k < kernel.width; ++k)
      {
        /*We make sure the image data has a buffer at the end to keep to not read "outside"*/
        sse_pixels = _mm_loadu_ps(ppp++);
        sse_sum = _mm_add_ps(sse_sum, _mm_mul_ps(kernel_data[k],sse_pixels));
      }
      _mm_store_ps(temp, sse_sum);
      /*only copy off the valid values*/
      memcpy( ptrout, temp, s*sizeof(float) );
      ptrout+=s;
    }

    /* Zero rightmost columns */
    for ( ; i < ncols ; i++)
    {
      *ptrout++ = 0.0f;
    }

    ptrrow += ncols;
  }
}

#else /* VIDTK_SSE2 */

static void _convolveImageHoriz(
  _KLT_FloatImage imgin,
  ConvolutionKernel kernel,
  _KLT_FloatImage imgout)
{
  float *ptrrow = imgin->data;           /* Points to row's first pixel */
  float *ptrout = imgout->data, /* Points to next output pixel */
    *ppp;
  float sum;
  int radius = kernel.width / 2;
  int ncols = imgin->ncols, nrows = imgin->nrows;
  int i, j, k;

  /* Kernel width must be odd */
  assert(kernel.width % 2 == 1);

  /* Must read from and write to different images */
  assert(imgin != imgout);

  /* Output image must be large enough to hold result */
  assert(imgout->ncols >= imgin->ncols);
  assert(imgout->nrows >= imgin->nrows);

  /* For each row, do ... */
  for (j = 0 ; j < nrows ; j++)  {

    /* Zero leftmost columns */
    for (i = 0 ; i < radius ; i++)
      *ptrout++ = 0.0f;

    /* Convolve middle columns with kernel */
    for ( ; i < ncols - radius ; i++)  {
      ppp = ptrrow + i - radius;
      sum = 0.0f;
      for (k = kernel.width-1 ; k >= 0 ; k--)
        sum += *ppp++ * kernel.data[k];
      *ptrout++ = sum;
    }

    /* Zero rightmost columns */
    for ( ; i < ncols ; i++)
      *ptrout++ = 0.0f;

    ptrrow += ncols;
  }
}
#endif /* VIDTK_SSE2 */

/*********************************************************************
 * _convolveImageVert
 */

#ifdef VIDTK_SSE2

static void _convolveImageVert_SSE(
  _KLT_FloatImage imgin,
  ConvolutionKernel kernel,
  _KLT_FloatImage imgout)
{
  float *ptrout = imgout->data;  /* Points to next output pixel */
  int radius = kernel.width / 2;
  int ncols = imgin->ncols, nrows = imgin->nrows;
  int i, j, k, s;

  SSE_STACK_ALIGNED(16) float temp[4];
  __m128 kernel_data[MAX_KERNEL_WIDTH];
  float * cols_rows[MAX_KERNEL_WIDTH];
  __m128 sse_sum;
  __m128 sse_pixels;
  float *ptrout_end = imgout->data + ncols*(nrows-radius);

  /* Kernel width must be odd */
  assert(kernel.width % 2 == 1);
  i = 0;
  for(k = kernel.width-1 ; k >= 0 ; k--, ++i)
  {
    kernel_data[i] = _mm_set1_ps(kernel.data[k]);
  }

  /* Must read from and write to different images */
  assert(imgin != imgout);

  /* Output image must be large enough to hold result */
  assert(imgout->ncols == imgin->ncols);
  assert(imgout->nrows == imgin->nrows);

  /* zero out the first radius rows and last radius rows*/
  for (j = 0 ; j < radius ; j++)
  {
    for ( i = 0; i < ncols; i++)
    {
      assert(ptrout_end < imgout->data+ncols*nrows);
      assert(ptrout < imgout->data+ncols*nrows);
      *ptrout_end = *ptrout = 0.0f;
      ptrout_end++;
      ptrout++;
    }
  }

  for(k = 0 ; k < kernel.width ; ++k)
  {
    cols_rows[k] = imgin->data + k*imgin->ncols;
  }

  for( ; j < nrows - radius; ++j)
  {
    i = 0;
    /*Align the output row*/
    s = 4 - (((unsigned long)ptrout)%16)/4;
    assert(s > 0 && s <= 4);
    if( s != 4 )
    {
      i += s;
      sse_sum = _mm_setzero_ps();
      for ( k = 0 ; k < kernel.width; ++k )
      {
        sse_pixels = _mm_loadu_ps(cols_rows[k]);
        sse_sum = _mm_add_ps(sse_sum, _mm_mul_ps(kernel_data[k],sse_pixels));
        cols_rows[k]+=s;
      }
      _mm_store_ps(temp, sse_sum);
      memcpy( ptrout, temp, s*sizeof(float) );
      ptrout+=s;
    }
    assert(((unsigned long)ptrout & (15)) == 0);

    for ( ; i < ncols-4; i+=4)
    {
      sse_sum = _mm_setzero_ps();
      for ( k = 0 ; k < kernel.width; ++k )
      {
        sse_pixels = _mm_loadu_ps(cols_rows[k]);
        sse_sum = _mm_add_ps(sse_sum, _mm_mul_ps(kernel_data[k],sse_pixels));
        cols_rows[k]+=4;
      }
      _mm_store_ps(ptrout, sse_sum);
      ptrout+=4;
    }
    /*clean up remaining*/
    s = ncols-i;
    assert(s >= 0 && s <= 4);
    if( s != 0 )
    {
      i += s;
      sse_sum = _mm_setzero_ps();
      for ( k = 0 ; k < kernel.width; ++k )
      {
        /*We make sure the image data has a buffer at the end to keep to not read "outside"*/
        sse_pixels = _mm_loadu_ps(cols_rows[k]);
        sse_sum = _mm_add_ps(sse_sum, _mm_mul_ps(kernel_data[k],sse_pixels));
        cols_rows[k]+=s;
      }
      _mm_store_ps(temp, sse_sum);
      /*only copy off the valid values*/
      memcpy( ptrout, temp, s*sizeof(float) );
      ptrout+=s;
    }
  }
}

#else /* VIDTK_SSE2 */

static void _convolveImageVert(
  _KLT_FloatImage imgin,
  ConvolutionKernel kernel,
  _KLT_FloatImage imgout)
{
  float *ptrcol = imgin->data;            /* Points to row's first pixel */
  float *ptrout = imgout->data,  /* Points to next output pixel */
    *ppp;
  float sum;
  int radius = kernel.width / 2;
  int ncols = imgin->ncols, nrows = imgin->nrows;
  int i, j, k;

  /* Kernel width must be odd */
  assert(kernel.width % 2 == 1);

  /* Must read from and write to different images */
  assert(imgin != imgout);

  /* Output image must be large enough to hold result */
  assert(imgout->ncols >= imgin->ncols);
  assert(imgout->nrows >= imgin->nrows);

  /* For each column, do ... */
  for (i = 0 ; i < ncols ; i++)  {

    /* Zero topmost rows */
    for (j = 0 ; j < radius ; j++)  {
      *ptrout = 0.0f;
      ptrout += ncols;
    }

    /* Convolve middle rows with kernel */
    for ( ; j < nrows - radius ; j++)  {
      ppp = ptrcol + ncols * (j - radius);
      sum = 0.0f;
      for (k = kernel.width-1 ; k >= 0 ; k--)  {
        sum += *ppp * kernel.data[k];
        ppp += ncols;
      }
      *ptrout = sum;
      ptrout += ncols;
    }

    /* Zero bottommost rows */
    for ( ; j < nrows ; j++)  {
      *ptrout = 0.0f;
      ptrout += ncols;
    }

    ptrcol++;
    ptrout -= nrows * ncols - 1;
  }
}

#endif /* VIDTK_SSE2 */

/*********************************************************************
 * _convolveSeparate
 */

static void _convolveSeparate(
  _KLT_FloatImage imgin,
  ConvolutionKernel horiz_kernel,
  ConvolutionKernel vert_kernel,
  _KLT_FloatImage imgout)
{
  /* Create temporary image */
  _KLT_FloatImage tmpimg;
  tmpimg = _KLTCreateFloatImage(imgin->ncols, imgin->nrows);

  /* Do convolution */
#ifdef VIDTK_SSE2
  _convolveImageHoriz_SSE(imgin, horiz_kernel, tmpimg);
  _convolveImageVert_SSE(tmpimg, vert_kernel, imgout);
#else
  _convolveImageHoriz(imgin, horiz_kernel, tmpimg);
  _convolveImageVert(tmpimg, vert_kernel, imgout);
#endif

  /* Free memory */
  _KLTFreeFloatImage(tmpimg);
}


/*********************************************************************
 * _KLTComputeGradients
 */

void _KLTComputeGradients(
  _KLT_FloatImage img,
  float sigma,
  _KLT_FloatImage gradx,
  _KLT_FloatImage grady)
{

  /* Output images must be large enough to hold result */
  assert(gradx->ncols >= img->ncols);
  assert(gradx->nrows >= img->nrows);
  assert(grady->ncols >= img->ncols);
  assert(grady->nrows >= img->nrows);

  /* Compute kernels, if necessary */
  if (fabs(sigma - sigma_last) > 0.05)
    _computeKernels(sigma, &gauss_kernel, &gaussderiv_kernel);

  _convolveSeparate(img, gaussderiv_kernel, gauss_kernel, gradx);
  _convolveSeparate(img, gauss_kernel, gaussderiv_kernel, grady);

}


/*********************************************************************
 * _KLTComputeSmoothedImage
 */

void _KLTComputeSmoothedImage(
  _KLT_FloatImage img,
  float sigma,
  _KLT_FloatImage smooth)
{
  /* Output image must be large enough to hold result */
  assert(smooth->ncols >= img->ncols);
  assert(smooth->nrows >= img->nrows);

  /* Compute kernel, if necessary; gauss_deriv is not used */
  if (fabs(sigma - sigma_last) > 0.05)
    _computeKernels(sigma, &gauss_kernel, &gaussderiv_kernel);

  _convolveSeparate(img, gauss_kernel, gauss_kernel, smooth);
}



/* Copyright: public domain */

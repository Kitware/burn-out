/*********************************************************************
 * klt_util.c
 *********************************************************************/

/* Standard includes */
#include <assert.h>
#include <stdlib.h>  /* malloc() */
#include <math.h>		/* fabs() */

/* Our includes */
#include "base.h"
#include "error.h"
#include "pnmio.h"
#include "klt.h"
#include "klt_util.h"


/*********************************************************************/

float _KLTComputeSmoothSigma(
  KLT_TrackingContext tc)
{
  return (tc->smooth_sigma_fact * max(tc->window_width, tc->window_height));
}


/*********************************************************************
 * _KLTCreateFloatImage
 */

_KLT_FloatImage _KLTCreateFloatImage(
  int ncols,
  int nrows)
{
  _KLT_FloatImage floatimg;
  int nbytes = sizeof(_KLT_FloatImageRec) +
    ncols * nrows * sizeof(float);
#if VIDTK_SSE2
   /* Used to simplify the code a bit on
    * boundry cases.  This buffer makes sure
    * we never read off the end of the image*/
  nbytes += 4*sizeof(float);
#endif

  floatimg = (_KLT_FloatImage)  malloc((unsigned int)nbytes);
  if (floatimg == NULL)
    KLTError("(_KLTCreateFloatImage)  Out of memory");
  floatimg->ncols = ncols;
  floatimg->nrows = nrows;
  floatimg->data = (float *)  (floatimg + 1);

  return(floatimg);
}


/*********************************************************************
 * _KLTFreeFloatImage
 */

void _KLTFreeFloatImage(
  _KLT_FloatImage floatimg)
{
  free(floatimg);
}

//commented out this code because it is not used anywhere and is untested
//might be useful in the future for debugging
#if 0
/*********************************************************************
 * _KLTPrintSubFloatImage
 */

void _KLTPrintSubFloatImage(
  _KLT_FloatImage floatimg,
  int x0, int y0,
  int width, int height)
{
  int ncols = floatimg->ncols;
  int offset;
  int i, j;

  assert(x0 >= 0);
  assert(y0 >= 0);
  assert(x0 + width <= ncols);
  assert(y0 + height <= floatimg->nrows);

  fprintf(stderr, "\n");
  for (j = 0 ; j < height ; j++)  {
    for (i = 0 ; i < width ; i++)  {
      offset = (j+y0)*ncols + (i+x0);
      fprintf(stderr, "%6.2f ", *(floatimg->data + offset));
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
}
#endif


#if 0
/*********************************************************************
 * _KLTWriteFloatImageToPGM
 */

void _KLTWriteFloatImageToPGM(
  _KLT_FloatImage img,
  char *filename)
{
  int npixs = img->ncols * img->nrows;
  float mmax = -999999.9f, mmin = 999999.9f;
  float fact;
  float *ptr;
  uchar *byteimg, *ptrout;
  int i;

  /* Calculate minimum and maximum values of float image */
  ptr = img->data;
  for (i = 0 ; i < npixs ; i++)  {
    mmax = max(mmax, *ptr);
    mmin = min(mmin, *ptr);
    ptr++;
  }

  /* Allocate memory to hold converted image */
  byteimg = (uchar *) malloc(npixs * sizeof(uchar));

  /* Convert image from float to uchar */
  fact = 255.0f / (mmax-mmin);
  ptr = img->data;
  ptrout = byteimg;
  for (i = 0 ; i < npixs ; i++)  {
    *ptrout++ = (uchar) ((*ptr++ - mmin) * fact);
  }

  /* Write uchar image to PGM */
  pgmWriteFile(filename, byteimg, img->ncols, img->nrows);

  /* Free memory */
  free(byteimg);
}

/*********************************************************************
 * _KLTWriteFloatImageToPGM
 */

void _KLTWriteAbsFloatImageToPGM(
  _KLT_FloatImage img,
  char *filename,float scale)
{
  int npixs = img->ncols * img->nrows;
  float fact;
  float *ptr;
  uchar *byteimg, *ptrout;
  int i;
  float tmp;

  /* Allocate memory to hold converted image */
  byteimg = (uchar *) malloc(npixs * sizeof(uchar));

  /* Convert image from float to uchar */
  fact = 255.0f / scale;
  ptr = img->data;
  ptrout = byteimg;
  for (i = 0 ; i < npixs ; i++)  {
    tmp = (float) (fabs(*ptr++) * fact);
    if(tmp > 255.0) tmp = 255.0;
    *ptrout++ =  (uchar) tmp;
  }

  /* Write uchar image to PGM */
  pgmWriteFile(filename, byteimg, img->ncols, img->nrows);

  /* Free memory */
  free(byteimg);
}
/* Copyright: public domain */
#endif

/*********************************************************************
 * _KLTApply3x3HomogToXY
 */
void _KLTApply3x3HomogToXY(
  float x0, float y0, const float *h,
  float *x1, float *y1)
{
  float x1_, y1_, w1_;

  /* A NULL homography implies identity */
  if(!h)
  {
    *x1 = x0;
    *y1 = y0;
    return;
  }

  x1_ = h[0]*x0 + h[1]*y0 + h[2];
  y1_ = h[3]*x0 + h[4]*y0 + h[5];
  w1_ = h[6]*x0 + h[7]*y0 + h[8];

  *x1 = x1_/w1_;
  *y1 = y1_/w1_;
}

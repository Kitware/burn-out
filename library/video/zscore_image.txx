/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "zscore_image.h"
#include <vil/vil_math.h>
#include <vil/vil_decimate.h>
#include <vil/vil_plane.h>
#include <vil/vil_crop.h>
#include <vil/vil_transpose.h>

namespace vidtk
{

//: Apply threshold such that dest(i,j,p)=true if src(i,j,p)>=t
template<class srcT>
void zscore_threshold_above( const vil_image_view<srcT>& src,
                             vil_image_view<bool>& dest,  double thresh,
                             unsigned step )
{
  unsigned ni = src.ni(),nj = src.nj(),np = src.nplanes();
  dest.set_size(ni,nj,np);

  vcl_ptrdiff_t istepA=step*src.istep(),jstepA=step*src.jstep(),pstepA = src.planestep();
  vcl_ptrdiff_t istepB=step*dest.istep(),jstepB=step*dest.jstep(),pstepB = dest.planestep();
  const srcT* planeA = src.top_left_ptr();
  bool* planeB = dest.top_left_ptr();

  double sum = 0.0;
  double sumSq = 0.0;
  unsigned count = 0;
  for (unsigned p=0;p<np;++p,planeA += pstepA,planeB += pstepB)
  {
    const srcT* rowA   = planeA;
    bool* rowB   = planeB;
    for (unsigned j=0;j < nj; j += step, rowA += jstepA, rowB += jstepB)
    {
      const srcT* pixelA = rowA;
      bool* pixelB = rowB;
      for (unsigned i=0;i<ni;i += step,pixelA += istepA, pixelB += istepB)
      {
        sum = sum + *pixelA ;
        sumSq = sumSq + ( *pixelA ) * ( *pixelA );
        ++count;
      } // unsigned i
    } // unsigned j
  } // unsigned p

  double mean = sum / count;
  double std =  vcl_sqrt( sumSq / count - mean * mean );

  istepA=src.istep(); jstepA=src.jstep(); pstepA = src.planestep();
  istepB=dest.istep(); jstepB=dest.jstep(); pstepB = dest.planestep();
  planeA = src.top_left_ptr();
  planeB = dest.top_left_ptr();

  for (unsigned p=0;p<np;++p,planeA += pstepA,planeB += pstepB)
  {
    const srcT* rowA   = planeA;
    bool* rowB   = planeB;
    for (unsigned j=0;j<nj;++j,rowA += jstepA,rowB += jstepB)
    {
      const srcT* pixelA = rowA;
      bool* pixelB = rowB;
      for (unsigned i=0;i<ni;++i,pixelA+=istepA,pixelB+=istepB)
      {
        double z = ( *pixelA - mean ) / std;
        if ( z > thresh )
        {
          *pixelB = true;
        }
        else
        {
          *pixelB = false;
        }
      } // unsigned i
    } // unsigned j
  } // unsigned p
}


//: Standardize the image in place using global statistics.
//  The mean (mu) and stanndard deviation (sigma) are computed over the whole image.
//  The result is a z-score image z = (x-mu)/sigma
template<class srcT>
void zscore_global( vil_image_view<srcT>& img, unsigned step )
{
  assert(img.nplanes() == 1);
  double mean, var;
  vil_math_mean_and_variance(mean, var, vil_decimate( img, step ), 0);
  double stdev = vcl_sqrt(var);
  vil_math_scale_and_offset_values(img, 1.0/stdev, -mean/stdev );
}


// Evaluate the integral over a box using an integral image
// pixel is the base pixel and offset if an array of pointer offsets to
// the pixels at the corners of the box in this order [UL, UR, LL, LR]
template <class T>
inline T integral_image_box( const T* pixel, const vcl_ptrdiff_t offset[4] )
{
  return *(pixel+offset[0]) + *(pixel+offset[3])
       - *(pixel+offset[1]) - *(pixel+offset[2]);
}


// Standardize a radius x radius box starting at row by mean and standard deviation
template<class srcT, class T>
void standardize_corner( srcT* row, vcl_ptrdiff_t istep, vcl_ptrdiff_t jstep,
                         unsigned radius, T mean, T stdev )
{
  for (unsigned j=0; j<=radius; j++, row+=jstep)
  {
    srcT* pixel = row;
    for (unsigned i=0; i<=radius; i++, pixel+=istep)
    {
      *pixel -= mean;
      *pixel /= stdev;
    }
  }
}


// Standardize each image column using intergral images.
// this function is used for top and bottom image boundaries and corners.
template<class srcT>
void standardize_cols( vil_image_view<srcT>& img,
                       const vcl_vector<float>& means,
                       const vcl_vector<float>& stdevs )
{
  const vcl_ptrdiff_t istep=img.istep(), jstep=img.jstep();
  const unsigned ni= img.ni(), nj=img.nj();
  const unsigned radius = nj-1;

  srcT* row = img.top_left_ptr();
  standardize_corner(row, istep, jstep, radius, means.front(), stdevs.front());
  row = img.top_left_ptr() + nj*istep;
  for (unsigned j=0; j<nj; j++, row+=jstep)
  {
    srcT* pixel = row;
    for (unsigned i=1; i<ni-2*radius-1; i++, pixel+=istep)
    {
      *pixel -= means[i];
      *pixel /= stdevs[i];
    }
  }
  row = img.top_left_ptr() + (ni-radius-1)*istep;
  standardize_corner(row, istep, jstep, radius, means.back(), stdevs.back());
}


//: Standardize the image in place using local statistics over a group of rows.
//  The mean (mu) and stanndard deviation (sigma) are computed for each pixel
//  over a groups 2*radius+1 rows centered at each pixel.
//  The result is a z-score image z = (x-mu)/sigma
template<class srcT>
void zscore_local_per_row( vil_image_view<srcT>& img, unsigned radius )
{
  assert(img.nplanes() == 1);
  assert(radius > 0);
  const unsigned diameter = 2*radius + 1;
  const unsigned ni = img.ni(), nj = img.nj();

  if (diameter >= nj)
  {
    // if the box is bigger than the image height, just use the global calculation
    zscore_global(img, 1);
    return;
  }

  float area = diameter*ni;

  vcl_vector<float> sum_vec(nj+1, 0.0f);
  vcl_vector<float> sum2_vec(nj+1, 0.0f);
  const vcl_ptrdiff_t istepA=img.istep(), jstepA=img.jstep();
  srcT* rowA = img.top_left_ptr();
  for (unsigned j=0; j<nj; ++j, rowA+=jstepA)
  {
    srcT* pixelA = rowA;
    float& sum = sum_vec[j+1];
    float& sum2 = sum2_vec[j+1];
    sum += sum_vec[j];
    sum2 += sum2_vec[j];
    for (unsigned i=0; i<ni; ++i, pixelA+=istepA)
    {
      sum += *pixelA;
      sum2 += (*pixelA) * (*pixelA);
    }
  }

  float mean = (sum_vec[diameter]) / area;
  float stdev = (sum2_vec[diameter]) / area;
  stdev = vcl_sqrt(stdev - mean*mean);
  rowA = img.top_left_ptr();
  unsigned j=0;
  for (; j<=radius; ++j, rowA+=jstepA)
  {
    srcT* pixelA = rowA;
    for (unsigned i=0; i<ni; ++i, pixelA+=istepA)
    {
      *pixelA -= mean;
      *pixelA /= stdev;
    }
  }
  for (; j<nj-radius; ++j, rowA+=jstepA)
  {
    srcT* pixelA = rowA;
    mean = (sum_vec[j+radius+1] - sum_vec[j-radius]) / area;
    stdev = (sum2_vec[j+radius+1] - sum2_vec[j-radius]) / area;
    stdev = vcl_sqrt(stdev - mean*mean);
    for (unsigned i=0; i<ni; ++i, pixelA+=istepA)
    {
      *pixelA -= mean;
      *pixelA /= stdev;
    }
  }
  for (; j<nj; ++j, rowA+=jstepA)
  {
    srcT* pixelA = rowA;
    for (unsigned i=0; i<ni; ++i, pixelA+=istepA)
    {
      *pixelA -= mean;
      *pixelA /= stdev;
    }
  }
}


//: Standardize the image in place using local statistics over a box.
//  The mean (mu) and stanndard deviation (sigma) are computed for each pixel
//  over a box of radius \a radius centered at each pixel.
//  The result is a z-score image z = (x-mu)/sigma
template<class srcT>
void zscore_local_box( vil_image_view<srcT>& img, unsigned radius )
{
  assert(img.nplanes() == 1);
  assert(radius > 0);
  const unsigned diameter = 2*radius + 1;
  const unsigned ni = img.ni(), nj = img.nj();

  if (diameter >= ni)
  {
    // if the box is bigger than the image width then do z-score per row
    zscore_local_per_row(img, radius);
    return;
  }
  else if (diameter >= nj)
  {
    // if the box is bigger than the image height, transpose, then do z-score per row
    vil_image_view<srcT> img_t = vil_transpose(img);
    zscore_local_per_row(img_t, radius);
    return;
  }


  // compute integral image and squared image.
  vil_image_view<float> int_img(ni+1, nj+1, 2);
  vil_image_view<float> int_img1 = vil_plane(int_img,0);
  vil_image_view<float> int_img2 = vil_plane(int_img,1);
  vil_math_integral_sqr_image(img, int_img1, int_img2);

  const vcl_ptrdiff_t istepA=img.istep(), jstepA=img.jstep();
  const vcl_ptrdiff_t istepI=int_img.istep(), jstepI=int_img.jstep(),
                      pstepI=int_img.planestep();

  // pointer offsets for quick access to box corners
  unsigned radius1 = radius + 1;
  const vcl_ptrdiff_t offsetsI[] = { -istepI*radius  - jstepI*radius,
                                      istepI*radius1 - jstepI*radius,
                                     -istepI*radius  + jstepI*radius1,
                                      istepI*radius1 + jstepI*radius1 };

  const float* rowI = int_img.top_left_ptr() - offsetsI[0];
  const float area = diameter*diameter;

  const unsigned rowlength = ni - 2 * radius;
  vcl_vector<float> boundary_means(rowlength, 0.0f);
  vcl_vector<float> boundary_stdevs(rowlength, 0.0f);
  const float* pixelI = rowI;
  for (unsigned i=0; i<rowlength; ++i, pixelI+=istepI)
  {
    float& mean = boundary_means[i];
    float& stdev = boundary_stdevs[i];
    mean = integral_image_box(pixelI, offsetsI) / area;
    stdev = integral_image_box(pixelI+pstepI, offsetsI) / area;
    stdev = vcl_sqrt(stdev - mean*mean);
  }

  vil_image_view<srcT> top_strip = vil_crop(img,0,ni,0,radius1);
  standardize_cols(top_strip, boundary_means, boundary_stdevs);

  srcT* rowA = img.top_left_ptr() + radius1*jstepA;
  rowI = int_img.top_left_ptr() + radius*istepI + radius1*jstepI;
  const int sni = ni - radius, snj = nj - radius;
  for (unsigned j=radius1; j<snj-1; j++, rowA+=jstepA, rowI+=jstepI)
  {
    srcT* pixelA = rowA;
    const float* pixelI = rowI;
    float mean = integral_image_box(pixelI, offsetsI) / area;
    float stdev = integral_image_box(pixelI+pstepI, offsetsI) / area;
    stdev = vcl_sqrt(stdev - mean*mean);
    unsigned i=0;
    for (; i<=radius; i++, pixelA+=istepA)
    {
      *pixelA -= mean;
      *pixelA /= stdev;
    }
    pixelI+=istepI;
    // main loop over non-boundary area
    for (; i<sni; i++, pixelA+=istepA, pixelI+=istepI)
    {
      mean = integral_image_box(pixelI, offsetsI) / area;
      stdev = integral_image_box(pixelI+pstepI, offsetsI) / area;
      stdev = vcl_sqrt(stdev - mean*mean);
      *pixelA -= mean;
      *pixelA /= stdev;
    }
    for (; i<ni; i++, pixelA+=istepA)
    {
      *pixelA -= mean;
      *pixelA /= stdev;
    }
  } // unsigned j

  pixelI = rowI;
  for (unsigned i=0; i<rowlength; ++i, pixelI+=istepI)
  {
    float& mean = boundary_means[i];
    float& stdev = boundary_stdevs[i];
    mean = integral_image_box(pixelI, offsetsI) / area;
    stdev = integral_image_box(pixelI+pstepI, offsetsI) / area;
    stdev = vcl_sqrt(stdev - mean*mean);
  }

  vil_image_view<srcT> bottom_strip = vil_crop(img,0,ni,nj-radius1,radius1);
  standardize_cols(bottom_strip, boundary_means, boundary_stdevs);

}

} // end namespace vidtk


#define INSTANTIATE_ZSCORE_IMAGE( srcT )                                \
  template                                                              \
  void vidtk::zscore_threshold_above( const vil_image_view<srcT>& src,  \
                                      vil_image_view<bool>& dest,       \
                                      double thresh, unsigned step );   \
  template                                                              \
  void vidtk::zscore_global( vil_image_view<srcT>& img,                 \
                             unsigned step );                           \
  template                                                              \
  void vidtk::zscore_local_box( vil_image_view<srcT>& img,              \
                                unsigned radius )


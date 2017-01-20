/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/illumination_normalization.txx>
#include <climits>

//For now I am putting this here.  This should go to VXL
void vil_math_median(vxl_uint_16& median, const vil_image_view<vxl_uint_16>& im, unsigned p)
{
  unsigned ni = im.ni();
  unsigned nj = im.nj();

  // special case the empty image.
  if ( ni == 0 && nj == 0 )
  {
    median = 0;
    return;
  }

  unsigned hist[USHRT_MAX+1] = { 0 };
  for (unsigned j=0;j<nj;++j)
  {
    for (unsigned i=0;i<ni;++i)
    {
      ++hist[ im(i,j,p) ];
    }
  }

  unsigned tot = ni*nj;
  // Target is ceil(tot/2)
  unsigned tgt = (tot+1) / 2;
  unsigned cnt = 0;
  unsigned idx = 0;
  while ( cnt < tgt )
  {
    cnt += hist[idx];
    ++idx;
  }

  // Test for halfway case
  if ( cnt == tgt && tot % 2 == 0 )
  {
    // idx is
    unsigned lo = idx-1;
    while ( hist[idx] == 0 )
    {
      ++idx;
    }
    median = vxl_uint_16((lo+idx)/2);
  }
  else
  {
    median = vxl_uint_16(idx-1);
  }
}

template class vidtk::median_illumination_normalization<vxl_byte>;
template class vidtk::median_illumination_normalization<vxl_uint_16>;
template class vidtk::mean_illumination_normalization<vxl_byte>;
template class vidtk::mean_illumination_normalization<vxl_uint_16>;

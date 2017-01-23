/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <video_transforms/variable_image_erode.h>

#include <cassert>
#include <vil/vil_border.h>

namespace vidtk
{


// Perform a variable erosion of the image.
template< typename PixType, typename IDType >
void variable_image_erode( const vil_image_view<PixType>& src,
                           const vil_image_view<IDType>& elem_ids,
                           const structuring_element_vector& se_vector,
                           vil_image_view<PixType>& dst )
{
  assert(src.nplanes()==1);
  assert(src.ni()==elem_ids.ni());
  assert(src.nj()==elem_ids.nj());

  // Compute corresponding offset list and find maximum sized element
  int largest_width = 0;
  IDType largest = 0;
  offset_vector se_offsets;

  for( unsigned int i = 0; i < se_vector.size(); i++ )
  {
    se_offsets.push_back( std::vector< std::ptrdiff_t >() );
    vil_compute_offsets( se_offsets[i], se_vector[i], src.istep(), src.jstep() );

    if( se_vector[i].max_i() - se_vector[i].min_i() > largest_width )
    {
      largest_width = se_vector[i].max_i() - se_vector[i].min_i();
      largest = i;
    }
  }

  // Perform erosion
  variable_image_erode( src, elem_ids, se_vector, se_offsets, largest, dst );
}


// Core [unsafe] implementation for performing a variable erosion
template< typename PixType, typename IDType >
void variable_image_erode( const vil_image_view<PixType>& src,
                           const vil_image_view<IDType>& elem_ids,
                           const structuring_element_vector& se_vector,
                           const offset_vector& offset_vec,
                           const IDType& max_id,
                           vil_image_view<PixType>& dst )
{
  assert(src.nplanes()==1);
  assert(src.ni()==elem_ids.ni());
  assert(src.nj()==elem_ids.nj());
  assert(se_vector.size()>=max_id);
  assert(se_vector.size()==offset_vec.size());

  // Get input properties
  const unsigned ni = src.ni();
  const unsigned nj = src.nj();
  dst.set_size(ni,nj,1);

  std::ptrdiff_t s_istep = src.istep(), s_jstep = src.jstep();
  std::ptrdiff_t d_istep = dst.istep(), d_jstep = dst.jstep();
  std::ptrdiff_t m_istep = elem_ids.istep(), m_jstep = elem_ids.jstep();

  // Link to the largest element in the current image
  const vil_structuring_element& largest_element = se_vector[ max_id ];

  // Perform actual dilation
  const PixType* src_row0 = src.top_left_ptr();
  const IDType* esm_row0 = elem_ids.top_left_ptr();
  PixType* dst_row0 = dst.top_left_ptr();

  // Define box in which all elements will be valid
  int ilo = -largest_element.min_i();
  int ihi = ni-1-largest_element.max_i();
  int jlo = -largest_element.min_j();
  int jhi = nj-1-largest_element.max_j();

  // Deal with left edge
  for (int i=0;i<ilo;++i)
  {
    for (unsigned int j=0;j<nj;++j)
    {
      dst(i,j,0)=vil_greyscale_erode(src,0,se_vector[elem_ids(i,j)],i,j);
    }
  }
  // Deal with right edge
  for (unsigned int i=ihi+1;i<ni;++i)
  {
    for (unsigned int j=0;j<nj;++j)
    {
      dst(i,j,0)=vil_greyscale_erode(src,0,se_vector[elem_ids(i,j)],i,j);
    }
  }
  // Deal with bottom edge
  for (int i=ilo;i<=ihi;++i)
  {
    for (int j=0;j<jlo;++j)
    {
      dst(i,j,0)=vil_greyscale_erode(src,0,se_vector[elem_ids(i,j)],i,j);
    }
  }
  // Deal with top edge
  for (int i=ilo;i<=ihi;++i)
  {
    for (unsigned int j=jhi+1;j<nj;++j)
    {
      dst(i,j,0)=vil_greyscale_erode(src,0,se_vector[elem_ids(i,j)],i,j);
    }
  }

  for (int j=jlo;j<=jhi;++j)
  {
    const PixType* src_p = src_row0 + j*s_jstep + ilo * s_istep;
    const IDType* esm_p = esm_row0 + j*m_jstep + ilo * m_istep;
    PixType* dst_p = dst_row0 + j*d_jstep + ilo * d_istep;

    for (int i=ilo;i<=ihi;++i,src_p+=s_istep,dst_p+=d_istep,esm_p+=m_istep)
    {
      const std::vector< std::ptrdiff_t >& offsets = offset_vec[*esm_p];
      *dst_p=vil_greyscale_erode(src_p,&offsets[0],offsets.size());
    }
  }
}


// Specialized (overload) for boolean type
template< typename IDType >
void variable_image_erode( const vil_image_view<bool>& src,
                           const vil_image_view<IDType>& elem_ids,
                           const structuring_element_vector& se_vector,
                           const offset_vector& offset_vec,
                           const IDType& max_id,
                           vil_image_view<bool>& dst )
{
  assert(src.nplanes()==1);
  assert(src.ni()==elem_ids.ni());
  assert(src.nj()==elem_ids.nj());
  assert(se_vector.size()>=max_id);
  assert(se_vector.size()==offset_vec.size());

  // Get input properties
  const unsigned ni = src.ni();
  const unsigned nj = src.nj();
  dst.set_size(ni,nj,1);

  std::ptrdiff_t s_istep = src.istep(), s_jstep = src.jstep();
  std::ptrdiff_t d_istep = dst.istep(), d_jstep = dst.jstep();
  std::ptrdiff_t m_istep = elem_ids.istep(), m_jstep = elem_ids.jstep();

  // Link to the largest element in the current image
  const vil_structuring_element& largest_element = se_vector[ max_id ];

  // Formulate a vil_border class for handling borders
  const vil_border<vil_image_view<bool> > border = vil_border_create_constant(src, true);

  // Perform actual dilation
  const bool* src_row0 = src.top_left_ptr();
  const IDType* esm_row0 = elem_ids.top_left_ptr();
  bool* dst_row0 = dst.top_left_ptr();

  // Define box in which all elements will be valid
  int ilo = -largest_element.min_i();
  int ihi = ni-1-largest_element.max_i();
  int jlo = -largest_element.min_j();
  int jhi = nj-1-largest_element.max_j();

  vil_border_accessor<vil_image_view<bool> > border_accessor =
     vil_border_create_accessor(src, border);

  // Deal with left edge
  for (int i=0;i<ilo;++i)
  {
    for (unsigned int j=0;j<nj;++j)
    {
      dst(i,j,0)=vil_binary_erode(border_accessor,0,se_vector[elem_ids(i,j)],i,j);
    }
  }
  // Deal with right edge
  for (unsigned int i=ihi+1;i<ni;++i)
  {
    for (unsigned int j=0;j<nj;++j)
    {
      dst(i,j,0)=vil_binary_erode(border_accessor,0,se_vector[elem_ids(i,j)],i,j);
    }
  }
  // Deal with bottom edge
  for (int i=ilo;i<=ihi;++i)
  {
    for (int j=0;j<jlo;++j)
    {
      dst(i,j,0)=vil_binary_erode(border_accessor,0,se_vector[elem_ids(i,j)],i,j);
    }
  }
  // Deal with top edge
  for (int i=ilo;i<=ihi;++i)
  {
    for (unsigned int j=jhi+1;j<nj;++j)
    {
      dst(i,j,0)=vil_binary_erode(border_accessor,0,se_vector[elem_ids(i,j)],i,j);
    }
  }

  for (int j=jlo;j<=jhi;++j)
  {
    const bool* src_p = src_row0 + j*s_jstep + ilo * s_istep;
    const IDType* esm_p = esm_row0 + j*m_jstep + ilo * m_istep;
    bool* dst_p = dst_row0 + j*d_jstep + ilo * d_istep;

    for (int i=ilo;i<=ihi;++i,src_p+=s_istep,dst_p+=d_istep,esm_p+=m_istep)
    {
      const std::vector< std::ptrdiff_t >& offsets = offset_vec[*esm_p];
      *dst_p=vil_binary_erode(src_p,&offsets[0],offsets.size());
    }
  }
}

}

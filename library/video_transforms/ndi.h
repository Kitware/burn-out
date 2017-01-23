
/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ndi_h_
#define vidtk_ndi_h_

#include <limits>

#include <vector>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_ndi_h__
VIDTK_LOGGER("ndi_h");

namespace vidtk
{

// Compute the disjoint information and normalized disjoint information
// between two image patches.

template < class PixType, class DestType >
void
get_ndi(const PixType *src_im, std::ptrdiff_t s_istep,
        std::ptrdiff_t s_jstep, std::ptrdiff_t s_pstep,
        const vil_image_view<PixType>& kernel,
        DestType *pdi,
        DestType *pndi )
{
  unsigned const ni = kernel.ni();
  unsigned const nj = kernel.nj();
  unsigned const np = kernel.nplanes();

  std::ptrdiff_t k_istep = kernel.istep(), k_jstep = kernel.jstep();

  int n = ( 1 << (sizeof( PixType)*8 ) );
  int mm = n*n;

  std::vector<float> vs(n,0);
  std::vector<float> vk(n,0);
  std::vector<float> vsk(mm,0);

  float counter=0;
  for (unsigned p = 0; p<np; ++p)
  {
    // Select first row of p-th plane
    PixType const* src_row  = src_im + p*s_pstep;
    PixType const* k_row =  kernel.top_left_ptr() + p*kernel.planestep();

    for (unsigned int j=0;j<nj;++j,src_row+=s_jstep,k_row+=k_jstep)
    {
      PixType const* sp = src_row;
      PixType const* kp = k_row;
      // select j-th row
      for (unsigned int i=0;i<ni;++i, sp += s_istep, kp += k_istep)
      {
        vs[ *sp ] ++;
        vk[ *kp ] ++;
        vsk[ (*sp)*n + (*kp) ] ++;
        ++counter;
      }
    }
  }

  const float log2f = static_cast<const float>(std::log(2.0));
  const float epsilon = std::numeric_limits<float>::min();

  float hs=0.0;
  for( int i=0; i<n; i++ )
  {
    float pr = vs[i] / counter;
    if( pr > epsilon )
      hs -= pr * std::log( pr ) / log2f;
  }

  float hk=0.0;
  for( int i=0; i<n; i++ )
  {
    float pr = vk[i] / counter;
    if( pr > epsilon )
      hk -= pr * std::log( pr ) / log2f;
  }

  float hsk=0.0;
  for( int i=0; i<mm; i++ )
  {
    float pr = vsk[i] / counter;
    if( pr > epsilon )
      hsk -= pr * std::log( pr ) / log2f;
  }

  hsk = std::max( hsk, epsilon );

  float di = 2*hsk - hs - hk;
  float ndi = di / hsk;

  *pdi = DestType(di);
  *pndi= DestType(ndi);

}


/*
Inputs:
   src_im: source image of size Ns x Ms
   kernel: kernel image of size Nk x Mk
           where Nk <= Ns and Mk <= Ms

Output:
   di_im:  DI surface of size NsxMs.
   ndi_im: DI surface of size NsxMs.
   return value: true if dest_img was computed successfully, false otherwise.
*/

template < class PixType, class DestType >
bool
get_ndi_surf(const vil_image_view<PixType> &src_im,
             const vil_image_view<PixType> &kernel,
             vil_image_view<DestType> &di_im,
             vil_image_view<DestType> &ndi_im)
{
  unsigned ni = src_im.ni();
  unsigned nj = src_im.nj();
  if(ni < kernel.ni() || nj < kernel.nj() || src_im.nplanes() != kernel.nplanes() )
  {
    LOG_INFO("get_ndi_surf: returning, too small image size");
    return false;
  }
  std::ptrdiff_t s_istep = src_im.istep();
  std::ptrdiff_t s_jstep = src_im.jstep();
  std::ptrdiff_t s_pstep = src_im.planestep();

  di_im.set_size(static_cast<unsigned int>(ni),static_cast<unsigned int>(nj),1);
  std::ptrdiff_t di_istep = di_im.istep(),di_jstep = di_im.jstep();

  ndi_im.set_size(static_cast<unsigned int>(ni),static_cast<unsigned int>(nj),1);
  std::ptrdiff_t ndi_istep = ndi_im.istep(),ndi_jstep = ndi_im.jstep();

  const PixType*  src_row  = src_im.top_left_ptr();
  DestType * di_row  = di_im.top_left_ptr();
  DestType * ndi_row = ndi_im.top_left_ptr();

  unsigned mj = nj - kernel.nj();
  unsigned mi = ni - kernel.ni();

  for (unsigned j=0;j<nj;++j, src_row+=s_jstep, di_row+=di_jstep, ndi_row+=ndi_jstep)
  {
    const PixType* sp = src_row;
    DestType* pdi = di_row;
    DestType* pndi= ndi_row;
    for (unsigned i=0;i<ni;++i, sp+=s_istep, pdi+=di_istep, pndi+=ndi_istep)
    {
      if( j <= mj && i <= mi )
      {
        get_ndi<PixType, DestType>(sp,s_istep,s_jstep,s_pstep,kernel,pdi,pndi);
      }
      else
      {
        *pdi = 1000.0;
        *pndi= 1.0;
      }
    }
  }

  return true;
}

/********************************************************************/
/*
  Returns the set of local valley (min) points in the provided surface.
  The size of the local neighorhood used to compute local minimum is
  defined by the specified *width* and *height* parameters.
*/

template < class DestType >
void
find_local_minima_ndi( vil_image_view<DestType> const &dist_surf,
                       double const min_value,
                       unsigned int const width,
                       unsigned int const height,
                       std::vector< unsigned > & min_is,
                       std::vector< unsigned > & min_js )
{
  if( width >= dist_surf.ni() || height >= dist_surf.nj() )
  {
    LOG_WARN( "ndi.h : failed to find local minima due to inconsistent"
      <<" sizes of dist_surf ("<< dist_surf.ni() << "x" << dist_surf.nj()
      <<") and width("<< width <<")xheight("<<height<<")" );
    return;
  }

  min_is.clear();
  min_js.clear();

  unsigned int mid_i = width/2;
  unsigned int mid_j = height/2;
  unsigned ib = dist_surf.ni() - static_cast<unsigned>(width*2);
  unsigned jb = dist_surf.nj() - static_cast<unsigned>(height*2);

  for ( unsigned int i = 0; i < ib; i++ )
  {
    unsigned int ci = i+mid_i;
    for ( unsigned int j = 0; j < jb; j++ )
    {
      unsigned int cj = j+mid_j;
      bool is_min = dist_surf(ci,cj) <= min_value;
      for(unsigned int u = i; is_min && u < i+width; ++u)
      {
        for(unsigned int v = j; is_min && v < j+height; ++v)
        {
          if((u != ci || v != cj) && dist_surf(u,v) < dist_surf(ci,cj) )
          {
            is_min = false;
            break;
          }
        }
        if( !is_min )
          break;
      }
      if(is_min)
      {
        min_is.push_back(ci);
        min_js.push_back(cj);
      }
    }
  }
}

} //end namespace vidtk

#endif // vidtk_ndi_h_

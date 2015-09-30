
/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ssd_h_
#define vidtk_ssd_h_

#include <vcl_limits.h>

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <utilities/log.h>

#include <vnl/vnl_matrix.h>

//#ifdef PRINT_DEBUG_INFO
//#include <tracking/debug_tracker.h>
//extern unsigned DT_id;
//#endif

namespace vidtk
{

template < typename T >
inline
float
fast_square_of_diff(T const& l, T const& r)
{
  float const d = static_cast<float>(l) - static_cast<float>(r);
  return (d * d);
}


template <>
inline
float
fast_square_of_diff<vxl_byte>(vxl_byte const& l, vxl_byte const& r)
{
  vxl_int_32 const d = l - r;
  return (d * d);
}


template < class PixType, class DestType >
void
get_ssd(const PixType *src_im, vcl_ptrdiff_t s_istep,
          vcl_ptrdiff_t s_jstep, vcl_ptrdiff_t s_pstep,
          const vil_image_view<PixType>& kernel,
          DestType * destp)
{
  unsigned const ni = kernel.ni();
  unsigned const nj = kernel.nj();
  unsigned const np = kernel.nplanes();
  unsigned const n = ni * nj * np;

  vcl_ptrdiff_t k_istep = kernel.istep(), k_jstep = kernel.jstep();

  float ssd = 0;
  for (unsigned p = 0; p<np; ++p)
  {
    // Select first row of p-th plane
    PixType const* src_row  = src_im + p*s_pstep;
    PixType const* k_row =  kernel.top_left_ptr() + p*kernel.planestep();

    for (unsigned int j=0;j<nj;++j,src_row+=s_jstep,k_row+=k_jstep)
    {
      PixType const* sp = src_row;
      PixType const* kp = k_row;
      // Sum over j-th row
      for (unsigned int i=0;i<ni;++i, sp += s_istep, kp += k_istep)
      {
        ssd += fast_square_of_diff<PixType>(*sp, *kp);
      }
    }
  }
  ssd /= n;

  *destp = DestType(ssd);
}

/********************************************************************/
/*
  Returns the valley (min) point in the provided surfance.
*/
template < class DestType >
void
find_local_minimum( vil_image_view<DestType> const &dist_surf,
                    unsigned &min_i,
                    unsigned &min_j )
{
  // The image will look like it is non-contiguous if either
  // dimension is 1 and, therefore, getting iterators will fail.
  // If one of the dimensions is 1, fall back to another implementation.
  if ( dist_surf.ni() > 1 && dist_surf.nj() > 1 )
  {
    typename vil_image_view<const DestType>::iterator iter = vcl_min_element(
      dist_surf.begin(), dist_surf.end() );

    unsigned min_ind_1d = std::distance(dist_surf.begin(), iter);
    min_i = min_ind_1d % dist_surf.ni();
    min_j = (unsigned) (min_ind_1d / dist_surf.ni());
  }
  else
  {
    double minVal = vcl_numeric_limits<double>::max();
    for ( unsigned int i = 0; i < dist_surf.ni(); i++ )
    {
      for ( unsigned int j = 0; j < dist_surf.nj(); j++ )
      {
        DestType val = dist_surf( i, j );
        if ( val < minVal )
        {
          minVal = val;
          min_i = i;
          min_j = j;
        }
      }
    }
  }
}

/********************************************************************/
/*
  Returns the local valley (min) points in the provided surface.
  This function uses a fixed size 5x5 neighborhood. This function is
  now obsolete and has been super-seeded by:

  void find_local_minima(vil_image_view<DestType> const &dist_surf,
                         double const max_value,
                         double const min_ave_corner_depth,
                         unsigned int const width,
                         unsigned int const height,
                         vcl_vector< unsigned > & min_is,
                         vcl_vector< unsigned > & min_js)
*/
template < class DestType >
void
find_local_minima( vil_image_view<DestType> const & dist_surf,
                   double const max_value,
                   vcl_vector< unsigned > & min_is,
                   vcl_vector< unsigned > & min_js )
{
  min_is.clear(); min_js.clear();
  for ( unsigned int i = 0; i < dist_surf.ni(); i++ )
  {
    for ( unsigned int j = 0; j < dist_surf.nj(); j++ )
    {
      bool is_min = dist_surf( i, j ) <= max_value;
      for(unsigned int u = (i>2)?i-2:0; is_min && u <= i+2 && u < dist_surf.ni(); ++u)
      {
        for(unsigned int v = (j>2)?j-2:0; is_min && v <= j+2 && v < dist_surf.nj(); ++v)
        {
          if(u != i || v != j )
          {
            is_min = is_min && dist_surf( i, j ) <= dist_surf( u, v );
          }
        }
      }
      if(is_min)
      {
        min_is.push_back(i);
        min_js.push_back(j);
      }
    }
  }
}

/********************************************************************/
/*
  Returns the a set of local valley (min) points in the provided surface.
  The size of the local neighorhood used to compute local minimum is
  defined by the specified *width* and *height* parameters. This could
  be the size of the template/kernel used in SSD.
*/

template < class DestType >
void
find_local_minima( vil_image_view<DestType> const &dist_surf,
                   double const max_value,
                   double const min_ave_corner_depth,
                   unsigned int const width,
                   unsigned int const height,
                   vcl_vector< unsigned > & min_is,
                   vcl_vector< unsigned > & min_js )
{
  if( width >= dist_surf.ni() || height >= dist_surf.nj() )
  {
    log_warning( "ssd.h : failed to find local minima due to inconsistent"
      <<" sizes of dist_surf ("<< dist_surf.ni() << "x" << dist_surf.nj()
      <<") and width("<< width <<")xheight("<<height<<")\n" );
    return;
  }

  min_is.clear();
  min_js.clear();

  unsigned int mid_i = width/2;
  unsigned int mid_j = height/2;
  for ( unsigned int i = 0; i < dist_surf.ni()-width; i++ )
  {
    unsigned int ci = i+mid_i;
    for ( unsigned int j = 0; j < dist_surf.nj()-height; j++ )
    {
      unsigned int cj = j+mid_j;
      bool is_min = dist_surf(ci,cj) <= max_value;
      for(unsigned int u = i; is_min && u < i+width; ++u)
      {
        for(unsigned int v = j; is_min && v < j+height; ++v)
        {
          if((u != ci || v != cj) && dist_surf(u,v) < dist_surf(ci,cj) )
          {
            is_min = false;
          }
        }
      }
      if(is_min)
      {
        unsigned int corners[][2] = {{i,j},
                                     {i,j+height-1},
                                     {i+width-1,j},
                                     {i+width-1,j+height-1}};
        double ave_corner = 0;
#ifdef PRINT_DEBUG_INFO
        //if(DT_is_track_of_interest(DT_id))
        //{
        // vcl_cout << "\t\t" << DT_id << " " << ci << " " << cj << " C: ";
        //}
#endif
        for( unsigned int u = 0; u < 4; ++u )
        {
#ifdef PRINT_DEBUG_INFO
          //if(DT_is_track_of_interest(DT_id))
          {
            vcl_cout << dist_surf(corners[u][0],corners[u][1]) << " ";
          }
#endif
          ave_corner += dist_surf(corners[u][0],corners[u][1]);
        }
        ave_corner = ave_corner*0.25;
        double depth = ave_corner-dist_surf(ci,cj);

#ifdef PRINT_DEBUG_INFO
        //if(DT_is_track_of_interest(DT_id))
        {
          vcl_cout << "\tA: " << ave_corner << " S: " << dist_surf(ci,cj) << "\t D: " << depth << vcl_endl;
        }
#endif

        if(depth >= min_ave_corner_depth)
        {
          min_is.push_back(ci);
          min_js.push_back(cj);
        }
      }
    }
  }
}



/********************************************************************/
/*
  This function is called by get_ssd_surf()
*/

template < class PixType, class DestType >
void
get_ssd(  const PixType *src_im, vcl_ptrdiff_t s_istep,
          vcl_ptrdiff_t s_jstep, vcl_ptrdiff_t s_pstep,
          const vil_image_view<PixType>& kernel,
          DestType * destp,
          const vil_image_view<float> & weights )
{
  unsigned ni = kernel.ni();
  unsigned nj = kernel.nj();
  unsigned np = kernel.nplanes();
  unsigned n = ni * nj * np;

  vcl_ptrdiff_t k_istep = kernel.istep(), k_jstep = kernel.jstep();
  vcl_ptrdiff_t w_istep = weights.istep(), w_jstep = weights.jstep();

  float ssd = 0;
  for (unsigned p = 0; p<np; ++p)
  {
    // Select first row of p-th plane
    const PixType* src_row  = src_im + p*s_pstep;
    const PixType* k_row =  kernel.top_left_ptr() + p*kernel.planestep();
    const float* w_row =  weights.top_left_ptr();

    for (unsigned j=0; j<nj; ++j, src_row += s_jstep,
                                  k_row += k_jstep,
                                  w_row += w_jstep )
    {
      const PixType* sp = src_row;
      const PixType* kp = k_row;
      const float* wp = w_row;
      // Sum over j-th row
      for (unsigned int i=0;i<ni;++i, sp += s_istep,
                                      kp += k_istep,
                                      wp += w_istep )
      {
        ssd += (*wp) * fast_square_of_diff<PixType>(*sp, *kp);
      }
    }
  }

  ssd /= n;

  *destp = (DestType) ssd;
}

/********************************************************************/

/*

A weighted version of get_ssd_surf(). The extra input includes:

Inputs:
   weights: weight matrix of size Nk x Mk, used for weighted ssd.
*/
template < class PixType, class DestType >
bool
get_ssd_surf( const vil_image_view<PixType> &src_im,
              vil_image_view<DestType> &dest_im,
              const vil_image_view<PixType> &kernel,
              const vil_image_view<float> &weights )
{
  int ni = static_cast<int>(1+src_im.ni()-kernel.ni()); //assert(ni >= 0);
  int nj = static_cast<int>(1+src_im.nj()-kernel.nj()); //assert(nj >= 0);
  if( ni < 1 || nj < 1 )
  {
    vcl_cout<<"get_ssd_surf: returning, too small image size\n";
    return false;
  }
  if( kernel.ni() != weights.ni() || kernel.nj() != weights.nj() )
  {
    vcl_cout<<"get_ssd_surf: returning, kernel & weights are unequal sizes.\n";
    return false;
  }
  vcl_ptrdiff_t s_istep = src_im.istep(), s_jstep = src_im.jstep();
  vcl_ptrdiff_t s_pstep = src_im.planestep();

  dest_im.set_size(static_cast<unsigned int>(ni),static_cast<unsigned int>(nj),1);
  vcl_ptrdiff_t d_istep = dest_im.istep(),d_jstep = dest_im.jstep();

  const PixType*  src_row  = src_im.top_left_ptr();
  DestType * dest_row = dest_im.top_left_ptr();

  for (int j=0;j<nj;++j, src_row+=s_jstep, dest_row+=d_jstep)
  {
    const PixType* sp = src_row;
    DestType* dp = dest_row;
    for (int i=0;i<ni;++i, sp += s_istep, dp += d_istep)
      get_ssd(sp,s_istep,s_jstep,s_pstep,kernel,dp, weights);
  }

  return true;
}

/********************************************************************/

/*
  This function uses the 'valleyness' of ssd surface as opposed to
  peakness of correlation surface. If the valley is not very
  discriminative/obvious, then there is a good chance that we have
  the aperture problem. This is common on building edges and ghost
  effect on the movers on flat roads.

  max_frac_valley_width_: e.g. 0.3 value will say that "there
  will be at the most 30% of the pixels of the ssd surface which will
  form a valley". 0.3 percentile in the spatial range.

  min_frac_valley_depth_: e.g. 0.2 percentile in the dynamic range.
*/

template<class SurfType>
bool
detect_ghost( vil_image_view<SurfType> const & corr_surf,
              unsigned max_bbox_side_in_pixels,
              double max_frac_valley_width,
              double min_frac_valley_depth )
{
  //For optimization.
  if(  corr_surf.ni() > max_bbox_side_in_pixels
    || corr_surf.nj() > max_bbox_side_in_pixels )
    return false;

  vcl_vector< double > fractions(3);
  fractions[0] = 0;
  fractions[1] = max_frac_valley_width;
  fractions[2] = 1.0;

  vcl_vector<SurfType> values(3);

  vil_math_value_range_percentiles( corr_surf, fractions, values);

  double fract_ssd = double( values[1] - values[0] )
                   / double( values[2] - values[0] );

  return fract_ssd < min_frac_valley_depth;
}

/********************************************************************/

/*
Inputs:
   src_im: source image of size Ns x Ms
   kernel: kernel image of size Nk x Mk
           where Nk <= Ns and Mk <= Ms

Output:
   dest_im: SSD surface of size (1+Ns-Nk)x(1+Ms-Mk).
   return value: true if dest_img was computed successfully, false otherwise.
*/

template < class PixType, class DestType >
bool
get_ssd_surf(const vil_image_view<PixType> &src_im,
               vil_image_view<DestType> &dest_im,
               const vil_image_view<PixType> &kernel)
{
  int ni = static_cast<int>(1+src_im.ni()-kernel.ni()); //assert(ni >= 0);
  int nj = static_cast<int>(1+src_im.nj()-kernel.nj()); //assert(nj >= 0);
  if(ni < 1 || nj < 1)
  {
    vcl_cout<<"get_ssd_surf: returning, too small image size\n";
    return false;
  }
  vcl_ptrdiff_t s_istep = src_im.istep(), s_jstep = src_im.jstep();
  vcl_ptrdiff_t s_pstep = src_im.planestep();

  dest_im.set_size(static_cast<unsigned int>(ni),static_cast<unsigned int>(nj),1);
  vcl_ptrdiff_t d_istep = dest_im.istep(),d_jstep = dest_im.jstep();

  const PixType*  src_row  = src_im.top_left_ptr();
  DestType * dest_row = dest_im.top_left_ptr();

  for (int j=0;j<nj;++j, src_row+=s_jstep, dest_row+=d_jstep)
  {
    const PixType* sp = src_row;
    DestType* dp = dest_row;
    for (int i=0;i<ni;++i, sp += s_istep, dp += d_istep)
      get_ssd(sp,s_istep,s_jstep,s_pstep,kernel,dp);
  }

  return true;
}

/********************************************************************/


} //end namespace vidtk

#endif // vidtk_rgb2yuv_h_

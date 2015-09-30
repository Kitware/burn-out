/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_fg_matcher_h_
#define vidtk_fg_matcher_h_

#include <tracking/track.h>
#include <utilities/config_block.h>
#include <utilities/paired_buffer.h>

#include <boost/shared_ptr.hpp>

#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

/// \file Abstract class capturing API for foreground match detection algorithms.

namespace vidtk
{

template < class PixType >
class fg_matcher
{
public:
  typedef boost::shared_ptr< fg_matcher< PixType > > sptr_t;
  typedef paired_buffer< timestamp, vil_image_view< PixType > > image_buffer_t;

  fg_matcher(){};

  virtual ~fg_matcher(){};

  static config_block params();

  static boost::shared_ptr< fg_matcher< PixType > >
    create_with_params( config_block const& blk );

  virtual bool initialize(){ return true; }

  virtual bool create_model( track_sptr track,
                             image_buffer_t const& img_buff,
                             timestamp const & curr_ts ) = 0;

  virtual bool find_matches( track_sptr track,
                             vgl_box_2d<unsigned> const & predicted_bbox,
                             vil_image_view<PixType> const & curr_im,
                             vcl_vector< vgl_box_2d<unsigned> > &out_bboxes ) const = 0;

  virtual bool update_model( track_sptr track,
                             image_buffer_t const & img_buff,
                             timestamp const & curr_ts ) = 0;

  virtual sptr_t deep_copy() = 0;

  virtual bool cleanup_model( track_sptr track ){ return true; }

protected:

  virtual bool set_params( config_block const & blk ) = 0;
};

}// namespace vidtk

#endif //vidtk_fg_matcher_h_

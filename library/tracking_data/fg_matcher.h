/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_fg_matcher_h_
#define vidtk_fg_matcher_h_

#include <tracking_data/track.h>
#include <tracking_data/track_state_attributes.h>

#include <utilities/paired_buffer.h>

#include <boost/shared_ptr.hpp>

#include <vector>
#include <vnl/vnl_double_2.h>
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>


/// A simple struct definining a region as used by fg_matcher classes
class fg_search_region_box
{
public:
  fg_search_region_box()
  {
   bbox_.empty();
   current_loc_img_.fill(-1);
   predict_loc_img_.fill(-1);
   centroid_loc_img_.fill(-1);
  }

  vnl_double_2 const& get_current_loc_img() const
  {
    return current_loc_img_;
  }

  vnl_double_2 const& get_predict_loc_img() const
  {
    return predict_loc_img_;
  }

  vnl_double_2 const& get_centroid_loc_img() const
  {
    return centroid_loc_img_;
  }

  vgl_box_2d<unsigned> const& get_bbox() const
  {
    return bbox_;
  }

  bool update( vnl_double_2 const& cur, vnl_double_2 const& pred, vgl_box_2d<unsigned> const& wh_bbox,
               bool is_bottom, bool both, unsigned int img_ni, unsigned int img_nj );

protected:
  /// the box defines the region for fg search
  vgl_box_2d<unsigned> bbox_;

  /// points of interest,
  /// These points can be used to  produce
  /// spatial prior surface for fg matching
  vnl_double_2 current_loc_img_;
  vnl_double_2 predict_loc_img_;
  vnl_double_2 centroid_loc_img_;
};


/// \file
/// Pure virtual class for fg_matching objects, storable in the track property map
///
/// This file will likely be over-written in the very near future. It exists
/// solely to define a virtual deep_copy function, to assist with deep_copying
/// the property maps stored in tracks, when a deep_copy on the track is invoked.
namespace vidtk
{

template < class PixType >
class fg_matcher
{

public:

  typedef boost::shared_ptr< fg_matcher< PixType > > sptr_t;
  typedef paired_buffer< timestamp, vil_image_view< PixType > > image_buffer_t;

  virtual ~fg_matcher() {}

  virtual bool initialize() = 0;

  virtual bool create_model( track_sptr track,
                             image_buffer_t const& img_buff,
                             timestamp const & curr_ts ) = 0;

  virtual bool find_matches( track_sptr track,
                             fg_search_region_box const & fg_search_bbox,
                             vil_image_view<PixType> const & curr_im,
                             std::vector< vgl_box_2d<unsigned> > &out_bboxes ) const = 0;

  virtual bool update_model( track_sptr track,
                             image_buffer_t const & img_buff,
                             timestamp const & curr_ts ) = 0;

  virtual typename track_state_attributes::state_attr_t get_track_state_attribute() const = 0;

  virtual sptr_t deep_copy() = 0;

  virtual bool cleanup_model( track_sptr track ) = 0;

};

}// namespace vidtk

#endif //vidtk_fg_matcher_algorithm_h_

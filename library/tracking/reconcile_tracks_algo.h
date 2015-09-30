/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _RECONCILE_TRACKS_ALGO_H_
#define _RECONCILE_TRACKS_ALGO_H_

#include "reconcile_tracks_process.h"

namespace vidtk {
namespace reconciler {

//
// This could use some more work on class scoping and placement.
// Currently it is rather sloppy.
// Reconciler delegates and derived classes could be moved to separate files.
// Reconciler and finalizer could be refactored into a class hierarchy,
// as a buffered delay process (?)
//

// ----------------------------------------------------------------
/** Track overlap factors.
 *
 *
 */
struct track2track_frame_overlap_record
{
public:
  typedef vcl_vector <track2track_frame_overlap_record > vector_t;
  typedef vector_t::iterator iterator_t;
  typedef vector_t::const_iterator const_iterator_t;

  unsigned int fL_frame_num; // frame number of first overlapping frame
  unsigned int fR_frame_num; // frame number of second overlapping frame
  double first_area;    // area of first bounding box
  double second_area; // area of second bounding box
  double overlap_area;  // area of overlap between the two
  double centroid_distance;  // distance between centroid of boxes
  double center_bottom_distance; // distance between "feet" of boxes

  // CTOR
  track2track_frame_overlap_record()
    : fL_frame_num(0),
      fR_frame_num(0),
      first_area(0.0),
      second_area(0.0),
      overlap_area(0.0),
      centroid_distance(0.0),
      center_bottom_distance(0.0)
  {
  }


};


// ----------------------------------------------------------------
/** output reconciler delegate class base class
 *
 * Methods of this class are called at specific times through the
 * lifecycle of the list of tracks.
 *
 * This is the base class for all delegete processing. It also serves
 * as the default delegate if no other is specified.
 */
class simple_track_reconciler
  : public reconciler_delegate
{
public:
  // -- TYPES --
  typedef vgl_box_2d < unsigned > bbox_t;
  typedef vcl_vector < bbox_t > bbox_vector_t;

  /** Operational parameters.
   *
   */
  class param_set {
  public:
    /// Minimum track size to compare.  If either track does not have
    /// enough states, then it will not be comapred.
    unsigned min_track_size;

    /// Box overlap fraction. If the boxes overlap more than the
    /// specified percentage, then they are considered identical.
    double overlap_fraction;

    /// Fraction of all state that overlap (by overlap fraction,
    /// above) for tracks to be considered equivalent.
    double state_count_fraction;

    // Possibly add a max_compare_size to limit mow many tracks states
    // are used in the comaprison. This value must be greater than the
    // delay window.
  };


  // -- CONSTRUCTORS --
  simple_track_reconciler(param_set const& p)
    : params(p)
  { }
  virtual ~simple_track_reconciler() { }


  /** Process finalized tracks.
   *
   * This method is called after the list of finalized tracks is
   * assembled for a frame time.
   *
   * @param[in] ts Frame time for vector of tracks
   * @param[in,out] tracks Vector of finalized tracks.
   */
//   virtual void process_finalized_tracks(vidtk::timestamp const& ts, vidtk::track_vector_t& tracks)


  /** Process updated track map.
   *
   * This method is called after all new tracks have been added to the
   * track map.
   *
   * @param[in] ts Frame time for last update of map.
   * @param[in,out] tracks Map of currently active tracks.
   * @param[out] terminated_tracks list of terminated tracks
   */
  virtual void process_track_map (vidtk::timestamp const& ts, track_map_t& tracks,
                                  vidtk::track_vector_t* terminated_tracks);

protected:
  enum { DISJOINT = 1, // return value from comparison
         A_OVER_B,
         B_OVER_A,
         A_SAME_AS_B
  };

  unsigned compare_tracks (vidtk::timestamp const& ts, track_sptr a, track_sptr b);
  unsigned compare_box_vector ( bbox_vector_t a_box_vect, bbox_vector_t b_box_vect);
  track2track_frame_overlap_record compare_box ( bbox_t b1, bbox_t b2);

  param_set params;

private:


};

} // end namespace
} // end namespace

#endif /* _RECONCILE_TRACKS_ALGO_H_ */

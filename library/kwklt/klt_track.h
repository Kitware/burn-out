/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kwklt_track_h_
#define vidtk_kwklt_track_h_

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <tracking_data/track.h>
#include <tracking_data/track_sptr.h>

namespace vidtk
{

class klt_track;

typedef boost::shared_ptr<klt_track> klt_track_ptr;


/// A track object specialized for capturing klt tracks over time
class klt_track
{
  public:
    typedef double ordinate_t;
    struct point_
    {
      ordinate_t x;
      ordinate_t y;
      int frame;
    };
    typedef const point_& point_t;

    /**
     * Adds a new point to a klt_track.
     */
    static klt_track_ptr extend_track(point_t pt, klt_track_ptr tail = klt_track_ptr());

    /**
     * Returns true if this is the start of the track.
     */
    bool is_start() const;

    /**
     * The size of the track.
     */
    int size() const;

    /**
     * Will assert if is_end is true.
     */
    point_t point() const;

    /**
     * Returns a NULL pointer if is_end is true.
     */
    klt_track_ptr tail() const;

    void trim_track( int count );

  private:
    klt_track();
    klt_track(point_t pt, klt_track_ptr tail);

    static klt_track_ptr base_track;

    point_ pt_;
    klt_track_ptr tail_;
    const int size_;
};


// Converts a klt track into a standard vidtk track, this function will
// become deprecated if kw_klt is converted to use a light-weight common
// track object in tracking_data.
vidtk::track_sptr convert_from_klt_track(klt_track_ptr trk);

}

bool operator==(vidtk::klt_track::point_t p1, vidtk::klt_track::point_t p2);
bool operator==(vidtk::klt_track const& t1, vidtk::klt_track const& t2);

#endif

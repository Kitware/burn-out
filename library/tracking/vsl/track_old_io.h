/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_old_io_h_
#define vidtk_track_old_io_h_

/// \file
///
/// \brief Binary IO functions for vidtk::track.

#include <vsl/vsl_fwd.h>
#include <tracking/track.h>
#include <vbl/vbl_smart_ptr.txx>

namespace vidtk
{
class track_old : public track { };
typedef vbl_smart_ptr<track_old> track_old_sptr;

class track_state_old : public track_state
{
public:
  track_state_old(void) { new_ptr_ = this; }
  track_state_old(track_state *ptr) : new_ptr_(ptr) { }
  track_state * new_ptr(void) { return this->new_ptr_; }
private:
  track_state *new_ptr_;
};
typedef vbl_smart_ptr<track_state_old> track_state_old_sptr;

class amhi_data_old : public amhi_data
{
public:
  amhi_data_old(void) { new_ptr_ = this; }
  amhi_data_old(amhi_data *ptr) : new_ptr_(ptr) { }
  amhi_data * new_ptr(void) { return this->new_ptr_; }
private:
  amhi_data *new_ptr_;
};
typedef vbl_smart_ptr<amhi_data_old> amhi_data_old_sptr;

}

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state_old &ts );
void vsl_b_read( vsl_b_istream& is, vidtk::track_state_old &ts );

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state_old *ts );
void vsl_b_read( vsl_b_istream& is, vidtk::track_state_old *&ts );

void vsl_b_write( vsl_b_ostream& os, const vidtk::amhi_data_old &a );
void vsl_b_read( vsl_b_istream& is, vidtk::amhi_data_old &a );

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_old &t );
void vsl_b_read( vsl_b_istream& is, vidtk::track_old &t );

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_old *t );
void vsl_b_read( vsl_b_istream& is, vidtk::track_old *&t );


#endif // vidtk_track_io_h_

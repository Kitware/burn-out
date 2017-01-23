/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "track_attributes.h"

namespace vidtk
{

track_attributes
::track_attributes()
  : attributes_(0)
{ }


// ----------------------------------------------------------------
// Expand these with groups as needed. See track_state.cxx for sample
// code.
// ----------------------------------------------------------------
void
track_attributes
::set_attr (track_attr_t attr)
{
  this->attributes_ |= attr;
}


// ----------------------------------------------------------------
void
track_attributes
::clear_attr (track_attr_t attr)
{
  this->attributes_ &= ~attr;
}


// ----------------------------------------------------------------
bool
track_attributes
::has_attr (track_attr_t attr) const
{
  return (this->attributes_ & attr) == attr;
}


// ----------------------------------------------------------------
void
track_attributes
::set_attrs (track_attributes::raw_attrs_t attrs)
{
  this->attributes_ = attrs;
}


// ----------------------------------------------------------------
track_attributes::raw_attrs_t
track_attributes
::get_attrs () const
{
  return this->attributes_;
}


// ----------------------------------------------------------------
std::string
track_attributes
::get_attr_text () const
{
#define APPEND_OUTPUT(X)                                \
  do { if (! output.empty()) output.append(", ");       \
    output.append(X); } while (0)

  std::string output;
  raw_attrs_t local_attr;

  local_attr = this->attributes_;
  if (local_attr & ATTR_AMHI) APPEND_OUTPUT ("AMHI");

#undef APPEND_OUTPUT

  return output;
}


} // end namespace

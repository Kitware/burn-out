/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "track_state_attributes.h"

namespace vidtk
{

// ----------------------------------------------------------------
/** CTOR
 *
 *
 */
track_state_attributes
::track_state_attributes(track_state_attributes::raw_attrs_t attrs)
  : attributes_(attrs)
{ }


// ----------------------------------------------------------------
void
track_state_attributes
::set_attr (state_attr_t attr)
{
  if (attr & _ATTR_ASSOC_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_ASSOC_MASK) | attr;
  }
  else if (attr & _ATTR_KALMAN_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_KALMAN_MASK) | attr;
  }
  else if (attr & _ATTR_INTERVAL_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_INTERVAL_MASK) | attr;
  }
  else if (attr & _ATTR_LINKING_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_LINKING_MASK) | attr;
  }
  else
  {
    this->attributes_ |= attr;
  }
}

// ----------------------------------------------------------------
void
track_state_attributes
::set_attrs (track_state_attributes::raw_attrs_t attrs)
{
  this->attributes_ = attrs;
}

// ----------------------------------------------------------------
void
track_state_attributes
::set_attrs(track_state_attributes::strings_t const& attrs)
{
  this->attributes_ = 0;

  strings_t::const_iterator iter, end = attrs.end();
  for (iter = attrs.begin(); iter != end; ++iter)
  {
    state_attr_t const attr = from_string(*iter);
    if (attr != _ATTR_NONE) this->set_attr(attr);
  }
}

// ----------------------------------------------------------------
track_state_attributes::raw_attrs_t
track_state_attributes
::get_attrs () const
{
  return this->attributes_;
}

// ----------------------------------------------------------------
track_state_attributes::strings_t
track_state_attributes
::get_attrs_strings() const
{
#define SWITCH_ADD_ATTR(a) case a: result.insert(to_string(a)); break
#define ADD_ATTR(a) if (this->attributes_ & a) result.insert(to_string(a))

  strings_t result;
  switch (this->attributes_ & _ATTR_ASSOC_MASK)
  {
    SWITCH_ADD_ATTR(ATTR_ASSOC_FG_SSD);
    SWITCH_ADD_ATTR(ATTR_ASSOC_FG_CSURF);
    SWITCH_ADD_ATTR(ATTR_ASSOC_FG_LOFT);
    SWITCH_ADD_ATTR(ATTR_ASSOC_FG_NXCORR);
    SWITCH_ADD_ATTR(ATTR_ASSOC_FG_NDI);
    SWITCH_ADD_ATTR(ATTR_ASSOC_DA_KINEMATIC);
    SWITCH_ADD_ATTR(ATTR_ASSOC_DA_MULTI_FEATURES);
  }
  switch (this->attributes_ & _ATTR_KALMAN_MASK)
  {
    SWITCH_ADD_ATTR(ATTR_KALMAN_ESH);
    SWITCH_ADD_ATTR(ATTR_KALMAN_LVEL);
  }
  switch (this->attributes_ & _ATTR_INTERVAL_MASK)
  {
    SWITCH_ADD_ATTR(ATTR_INTERVAL_FORWARD);
    SWITCH_ADD_ATTR(ATTR_INTERVAL_BACK);
    SWITCH_ADD_ATTR(ATTR_INTERVAL_INIT);
  }
  switch (this->attributes_ & _ATTR_LINKING_MASK)
  {
    SWITCH_ADD_ATTR(ATTR_LINKING_START);
    SWITCH_ADD_ATTR(ATTR_LINKING_END);
  }
  ADD_ATTR(ATTR_SCORING_DOMINANT);
  ADD_ATTR(ATTR_SCORING_NONDOMINANT);

  return result;

#undef SWITCH_ADD_ATTR
}

// ----------------------------------------------------------------
void
track_state_attributes
::clear_attr (state_attr_t attr)
{
  if (attr & _ATTR_ASSOC_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_ASSOC_MASK);
  }
  else if (attr & _ATTR_KALMAN_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_KALMAN_MASK);
  }
  else if (attr & _ATTR_INTERVAL_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_INTERVAL_MASK);
  }
  else if (attr & _ATTR_LINKING_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_LINKING_MASK);
  }
  else
  {
    this->attributes_ &= ~attr;
  }
}


// ----------------------------------------------------------------
bool
track_state_attributes
::has_attr (state_attr_t attr) const
{
  if (attr & _ATTR_ASSOC_MASK)
  {
    // handle group queries
    if (attr == ATTR_ASSOC_FG_GROUP)
    {
      return (this->attributes_ & ATTR_ASSOC_FG_GROUP) != 0;
    }
    else if (attr == ATTR_ASSOC_DA_GROUP)
    {
      return (this->attributes_ & ATTR_ASSOC_DA_GROUP) != 0;
    }
    else
    {
      // handle set of bits
      return (this->attributes_ & _ATTR_ASSOC_MASK) == attr;
    }
  }
  else if (attr & _ATTR_KALMAN_MASK)
  {
    // handle set of bits
    return (this->attributes_ & _ATTR_KALMAN_MASK) == attr;
  }
  else if (attr & _ATTR_INTERVAL_MASK)
  {
    // handle set of bits
    return (this->attributes_ & _ATTR_INTERVAL_MASK) == attr;
  }
  else if (attr & _ATTR_LINKING_MASK)
  {
    // handle set of bits
    return (this->attributes_ & _ATTR_LINKING_MASK) == attr;
  }
  else
  {
    return (this->attributes_ & attr) == attr;
  }
}


// ----------------------------------------------------------------
std::string
track_state_attributes
::get_attr_text () const
{
#define APPEND_OUTPUT(X)                                \
  do { if (! output.empty()) output.append(", ");       \
    output.append(X); } while(0)

  std::string output;
  int local_attr;

  local_attr =  this->attributes_ & _ATTR_ASSOC_MASK;
  switch (local_attr)
  {
  case ATTR_ASSOC_FG_SSD:
    APPEND_OUTPUT("SSD associative tracker");
    break;

  case ATTR_ASSOC_FG_NDI:
    APPEND_OUTPUT("NDI associative tracker");
    break;

  case ATTR_ASSOC_FG_CSURF:
    APPEND_OUTPUT("CSURF associative tracker");
    break;

  case ATTR_ASSOC_DA_KINEMATIC:
    APPEND_OUTPUT("Data Association Kinematic");
    break;

  case ATTR_ASSOC_DA_MULTI_FEATURES:
    APPEND_OUTPUT("Data Association Multi-feature");
    break;
  }

  local_attr =  this->attributes_ & _ATTR_KALMAN_MASK;
  switch (local_attr)
  {
  case ATTR_KALMAN_ESH:
    APPEND_OUTPUT("Extended Kalman");
    break;

  case ATTR_KALMAN_LVEL:
    APPEND_OUTPUT("Linear Kalman");
    break;
  }

  local_attr = this->attributes_ & _ATTR_INTERVAL_MASK;
  switch (local_attr)
  {
  case ATTR_INTERVAL_FORWARD:
    APPEND_OUTPUT("Forward");
    break;

  case ATTR_INTERVAL_BACK:
    APPEND_OUTPUT("Backward");
    break;

  case ATTR_INTERVAL_INIT:
    APPEND_OUTPUT("Init");
    break;
  }

  local_attr = this->attributes_ & _ATTR_LINKING_MASK;
  switch (local_attr)
  {
  case ATTR_LINKING_START:
    APPEND_OUTPUT("Linking start");
    break;

  case ATTR_LINKING_END:
    APPEND_OUTPUT("Linking end");
    break;

  }

  #undef APPEND_OUTPUT

  return output;
}

#define FOREACH_ATTR_IMPL(func, a) func(ATTR_##a, #a)
#define FOREACH_ATTR(func) \
  FOREACH_ATTR_IMPL(func, ASSOC_FG_SSD) \
  FOREACH_ATTR_IMPL(func, ASSOC_FG_CSURF) \
  FOREACH_ATTR_IMPL(func, ASSOC_FG_LOFT) \
  FOREACH_ATTR_IMPL(func, ASSOC_FG_NXCORR) \
  FOREACH_ATTR_IMPL(func, ASSOC_FG_NDI) \
  FOREACH_ATTR_IMPL(func, ASSOC_DA_KINEMATIC) \
  FOREACH_ATTR_IMPL(func, ASSOC_DA_MULTI_FEATURES) \
  FOREACH_ATTR_IMPL(func, KALMAN_ESH) \
  FOREACH_ATTR_IMPL(func, KALMAN_LVEL) \
  FOREACH_ATTR_IMPL(func, INTERVAL_FORWARD) \
  FOREACH_ATTR_IMPL(func, INTERVAL_BACK) \
  FOREACH_ATTR_IMPL(func, INTERVAL_INIT) \
  FOREACH_ATTR_IMPL(func, LINKING_START) \
  FOREACH_ATTR_IMPL(func, LINKING_END) \
  FOREACH_ATTR_IMPL(func, SCORING_DOMINANT) \
  FOREACH_ATTR_IMPL(func, SCORING_NONDOMINANT) \
  FOREACH_ATTR_IMPL(func, SCORING_SRC_IS_TRUTH) \
  FOREACH_ATTR_IMPL(func, SCORING_STATE_MATCHED)

// ----------------------------------------------------------------
std::string
track_state_attributes
::to_string(track_state_attributes::state_attr_t const attr)
{
#define TO_STRING(attr, name) case attr: return name;
  switch (attr)
  {
    FOREACH_ATTR(TO_STRING)
    default: return std::string();
  }
#undef TO_STRING
}

// ----------------------------------------------------------------
track_state_attributes::state_attr_t
track_state_attributes
::from_string(std::string const& text)
{
#define FROM_STRING(attr, name) if (text == name) return attr;
  FOREACH_ATTR(FROM_STRING)
  return _ATTR_NONE;
#undef FROM_STRING
}

} // end namespace

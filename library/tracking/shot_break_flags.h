/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_SHOT_BREAK_FLAGS_H_
#define _VIDTK_SHOT_BREAK_FLAGS_H_

namespace vidtk {

// ----------------------------------------------------------------
/** Status of pipeline at a specific time.
 *
 * @todo rename this class to frame_flags and maybe move to utils.
 */
class shot_break_flags
{
public:
  shot_break_flags()
    : m_shot_end(false),
      m_frame_usable(false),
      m_frame_not_processed(false)
  {  }

  virtual ~shot_break_flags() { }

  // -- ACCESORS --
  bool is_shot_end() const         { return m_shot_end; }
  bool is_frame_usable() const     { return m_frame_usable; }
  bool is_frame_not_processed() const  { return m_frame_not_processed; }

  // -- MANIPULATORS --
  void set_shot_end (bool v = true)     { m_shot_end = v; }
  void set_frame_usable (bool v = true) { m_frame_usable = v; }
  void set_frame_not_processed(bool v = true) { m_frame_not_processed = v; }


private:
  bool m_shot_end;
  bool m_frame_usable;
  bool m_frame_not_processed;

}; // end class shot_break_flags

inline bool operator== ( const ::vidtk::shot_break_flags& rhs, const ::vidtk::shot_break_flags& lhs )
{
  return
    (lhs.is_shot_end() == rhs.is_shot_end()) &&
    (lhs.is_frame_usable() == rhs.is_frame_usable()) &&
    (lhs.is_frame_not_processed() == rhs.is_frame_not_processed());
}

inline vcl_ostream& operator<< ( vcl_ostream& str, ::vidtk::shot_break_flags const& obj)
{
  str << "[se:" << obj.is_shot_end()
      << ", fu:" << obj.is_frame_usable() << ", fnp:" << obj.is_frame_not_processed()
      << "]";
  return (str);
}

} // end namespace

#endif /* _VIDTK_SHOT_BREAK_FLAGS_H_ */

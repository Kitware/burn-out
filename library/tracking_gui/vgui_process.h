/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vgui_process_h_
#define vidtk_vgui_process_h_

#include <tracking/gui_process.h>

namespace vidtk
{

class vgui_process_impl;

class vgui_process : public gui_process
{
public:
  typedef vgui_process self_type;
  vgui_process( vcl_string const& name );
  ~vgui_process();

  virtual bool initialize();

  virtual bool step();

  virtual bool persist();
private:
  vgui_process_impl * vgui_impl_;
};

} // end namespace vidtk

#endif// vidtk_vgui_process_h_


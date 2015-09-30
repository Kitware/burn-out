/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_SYNCED_VIEWER2D_MANAGER_H
#define INCL_SYNCED_VIEWER2D_MANAGER_H

#include <vcl_map.h>

#include <vgui/vgui_viewer2D_tableau.h>  // for the token
#include "synced_viewer2D_tableau_sptr.h"


class synced_viewer2D_manager
{
public:

  static synced_viewer2D_manager& Instance();
  void sync( synced_viewer2D_tableau_sptr tab, unsigned id );
  void handle( synced_viewer2D_tableau_sptr tab,
               const vgui_viewer2D_tableau::token_t& tokenBefore,
               const vgui_viewer2D_tableau::token_t& tokenAfter );

private:
  synced_viewer2D_manager() {}
  ~synced_viewer2D_manager() {}
  synced_viewer2D_manager( const synced_viewer2D_manager& other );
  synced_viewer2D_manager& operator=( const synced_viewer2D_manager& rhs );

  vcl_map< synced_viewer2D_tableau_sptr, unsigned > group_map;

  static synced_viewer2D_manager* p;
};

#endif

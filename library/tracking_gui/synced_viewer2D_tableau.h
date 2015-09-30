/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_SYNCED_VIEWER2D_TABLEAU_H
#define INCL_SYNCED_VIEWER2D_TABLEAU_H

#include <vgui/vgui_viewer2D_tableau.h>
#include "synced_viewer2D_tableau.h"
#include "synced_viewer2D_tableau_sptr.h"

class synced_viewer2D_tableau : public vgui_viewer2D_tableau
{
public:

  synced_viewer2D_tableau( );

  //: ctor; don't use-- use synced_viewer2D_tableau_new
  synced_viewer2D_tableau( const vgui_tableau_sptr& );

  //: handle events; synchronize zoom/drag/center events with peers
  virtual bool handle( const vgui_event& event );

  //: return the type of this tableau ('synced_viewer2D_tableau')
  virtual vcl_string type_name() const;

  //: "synchronize" zoom/pan events with other SVTs
  // whose sync() method was called with this ID
  // call with ID==0 to unsync
  void sync( unsigned group_id = 1 );

  void desync() { this->sync( 0 ); }

  unsigned get_sync_group_id() const;

protected:
  //: dtor; called by synced_viewer2D_tableau_sptr
  ~synced_viewer2D_tableau();

private:
  //: synchronization group ID, or 0 for none
  unsigned sync_group_id;
};

struct synced_viewer2D_tableau_new : public synced_viewer2D_tableau_sptr
{
  synced_viewer2D_tableau_new( )
    : synced_viewer2D_tableau_sptr( new synced_viewer2D_tableau( ) ) {}

  synced_viewer2D_tableau_new( const vgui_tableau_sptr& that )
    : synced_viewer2D_tableau_sptr( new synced_viewer2D_tableau( that )) {}
};

#endif

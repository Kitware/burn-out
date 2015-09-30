/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "synced_viewer2D_tableau.h"
#include "synced_viewer2D_manager.h"

synced_viewer2D_tableau
::synced_viewer2D_tableau( )
  : vgui_viewer2D_tableau( ), sync_group_id( 0 )
{
}

synced_viewer2D_tableau
::synced_viewer2D_tableau( const vgui_tableau_sptr& s )
  : vgui_viewer2D_tableau( s ), sync_group_id( 0 )
{
}

synced_viewer2D_tableau
::~synced_viewer2D_tableau()
{
}

vcl_string
synced_viewer2D_tableau
::type_name() const
{
  return "synced_viewer2D_tableau";
}

bool
synced_viewer2D_tableau
::handle( const vgui_event& event )
{
  vgui_viewer2D_tableau::token_t tokenBefore = this->token;
  bool rc = vgui_viewer2D_tableau::handle( event );
  vgui_viewer2D_tableau::token_t tokenAfter = this->token;
  synced_viewer2D_manager::Instance().handle( this, tokenBefore, tokenAfter );
  return rc;
}

void
synced_viewer2D_tableau
::sync( unsigned id )
{
  this->sync_group_id = id;
  synced_viewer2D_manager::Instance().sync( this, this->sync_group_id );
  // maybe something to trigger an initial sync with the existing group?
}

unsigned
synced_viewer2D_tableau
::get_sync_group_id() const
{
  return this->sync_group_id;
}

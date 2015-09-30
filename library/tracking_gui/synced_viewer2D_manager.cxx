/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "synced_viewer2D_tableau.h"
#include "synced_viewer2D_manager.h"

synced_viewer2D_manager* synced_viewer2D_manager::p = 0;

namespace
{
  bool operator==( const vgui_viewer2D_tableau::token_t& lhs,
                   const vgui_viewer2D_tableau::token_t& rhs )
  {
    return ( (lhs.scaleX == rhs.scaleX) &&
             (lhs.scaleY == rhs.scaleY) &&
             (lhs.offsetX == rhs.offsetX) &&
             (lhs.offsetY == rhs.offsetY) );
  }
};


synced_viewer2D_manager&
synced_viewer2D_manager
::Instance()
{
  if ( ! synced_viewer2D_manager::p )
  {
    synced_viewer2D_manager::p = new synced_viewer2D_manager();
  }
  return *synced_viewer2D_manager::p;
}

void
synced_viewer2D_manager
::sync( synced_viewer2D_tableau_sptr tab, unsigned id )
{
  this->group_map[ tab ] = id;
}

void
synced_viewer2D_manager
::handle( synced_viewer2D_tableau_sptr tab,
          const vgui_viewer2D_tableau::token_t& tokenBefore,
          const vgui_viewer2D_tableau::token_t& tokenAfter )
{
  vcl_map< synced_viewer2D_tableau_sptr, unsigned >::const_iterator i =
    this->group_map.find( tab );

  // quick exit on unsynchronized
  if ( ( i == this->group_map.end() ) || (i->second == 0)) return;

  // if tokens are different, set tokens on everybody in the map who
  // share tab's group ID

  if ( ! (tokenBefore == tokenAfter) )
  {
    for (vcl_map< synced_viewer2D_tableau_sptr, unsigned >::const_iterator j = this->group_map.begin();
         j != this->group_map.end();
         ++j )
    {
      if ( i == j ) continue;
      if ( i->second == j->second )
      {
        j->first->token = tokenAfter;
        j->first->post_redraw();
      }
    }
  }
}


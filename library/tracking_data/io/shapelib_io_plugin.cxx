/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include "track_writer_shape.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  fact = pm->ADD_FACTORY( vidtk::track_writer_interface, vidtk::track_writer_shape );
  fact->add_attribute( "file-type", "shape" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"shape\" - Write tracks in shapelib format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Shape file track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes shape file formatted tracks" );
}

/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include "track_reader_mit_layer_annotation.h"
#include "track_reader_viper.h"
#include "track_writer_mit.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  //                 interface type                concrete type
  fact = pm->ADD_FACTORY( vidtk::ns_track_reader::track_reader_interface, vidtk::ns_track_reader::track_reader_viper );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Viper track reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads VIPER formatted tracks" );

  fact = pm->ADD_FACTORY( vidtk::ns_track_reader::track_reader_interface, vidtk::ns_track_reader::track_reader_mit_layer_annotation );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "MIT track reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads MIT formatted tracks" );


  fact = pm->ADD_FACTORY( vidtk::track_writer_interface, vidtk::track_writer_mit );
  fact->add_attribute( "file-type", "xml mit" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"xml\" \"mit\" - Write tracks in MIT XML format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "MIT track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes MIT/XML formatted tracks" );

}

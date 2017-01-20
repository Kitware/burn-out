/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include "image_object_reader_protobuf.h"
#include "image_object_writer_protobuf.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  fact = pm->ADD_FACTORY( vidtk::ns_image_object_reader::image_object_reader_interface,
                          vidtk::ns_image_object_reader::image_object_reader_protobuf );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object Protobuf reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads Image Objects in Kitware protobuf container format" );

  fact = pm->ADD_FACTORY( vidtk::image_object_writer,
                          vidtk::image_object_writer_protobuf );
  fact->add_attribute( "file-type", "protobuf" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"protobuf\" - Write image objects in protobuf format" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object Protobuf writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes Image Objects in Kitware protobuf container format" );
}

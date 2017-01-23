/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include "image_object_reader_default.h"
#include "image_object_reader_vsl.h"
#include "image_object_reader_kw18.h"

#include "image_object_writer_default.h"
#include "image_object_writer_vsl.h"
#include "image_object_writer_kw18.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  // -- add reader factories --
  fact = pm->ADD_FACTORY( vidtk::ns_image_object_reader::image_object_reader_interface, vidtk::ns_image_object_reader::image_object_reader_default );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object default reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads Image Objects in default format" );

  fact = pm->ADD_FACTORY( vidtk::ns_image_object_reader::image_object_reader_interface, vidtk::ns_image_object_reader::image_object_reader_kw18 );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object kw18 reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads Image Objects in kw18 format" );

  fact = pm->ADD_FACTORY( vidtk::ns_image_object_reader::image_object_reader_interface, vidtk::ns_image_object_reader::image_object_reader_vsl );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object VSL reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads Image Objects in vsl format" );

  // -- add writer factories --
  fact = pm->ADD_FACTORY( vidtk::image_object_writer, vidtk::image_object_writer_default );
  fact->add_attribute( "file-type", "det 0" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"det\" or \"0\" - Write image objects in default format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object default writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION,
                    "Writes Image Objects in default csv format with columns  "
                    "1:Frame-number  2:Time  3-5:World-loc(x,y,z)  6-7:Image-loc(i,j)  8:Area   9-12:Img-bbox(i0,j0,i1,j1)" );

  fact = pm->ADD_FACTORY( vidtk::image_object_writer, vidtk::image_object_writer_vsl );
  fact->add_attribute( "file-type", "vsl" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"vsl\" - Write image objects in binary VSL format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object VSL writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes Image Objects in vsl format" );

  fact = pm->ADD_FACTORY( vidtk::image_object_writer, vidtk::image_object_writer_kw18 );
  fact->add_attribute( "file-type", "kw18" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"kw18\" - Write image objects in kw18 csv format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Image Object KW18 writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION,
                    "Writes Image Objects in kw18 acceptable format with one object per \"track\" " );
}

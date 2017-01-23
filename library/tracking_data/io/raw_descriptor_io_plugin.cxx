/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include "raw_descriptor_writer_csv.h"
#include "raw_descriptor_writer_json.h"
#include "raw_descriptor_writer_xml.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  // -- add writer factories --
  fact = pm->ADD_FACTORY( vidtk::ns_raw_descriptor_writer::raw_descriptor_writer_interface, vidtk::ns_raw_descriptor_writer::raw_descriptor_writer_csv );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "CSV Raw descriptor writer" )
    .add_attribute( "file-type", "csv" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"csv\" - Write raw descriptor in csv format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION,
                    "Writes raw descriptor in csv format with the following columns - 1:descriptor_id; 2:start_frame; 3:end_frame; "
                    "4:start_timestamp; 5:end_timestamp; 6:track_references (may be a group of values); 7:descriptor_data_vector" );

    fact = pm->ADD_FACTORY( vidtk::ns_raw_descriptor_writer::raw_descriptor_writer_interface, vidtk::ns_raw_descriptor_writer::raw_descriptor_writer_json );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "JSON Raw descriptor writer" )
    .add_attribute( "file-type", "json" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"json\" - Write raw descriptors in JSON format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION,
                    "Writes raw descriptor in JSON format" );

  fact = pm->ADD_FACTORY( vidtk::ns_raw_descriptor_writer::raw_descriptor_writer_interface, vidtk::ns_raw_descriptor_writer::raw_descriptor_writer_xml );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "XML Raw descriptor writer" )
    .add_attribute( "file-type", "xml" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"xml\" - Write raw descriptors in XML format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION,
                    "Writes raw descriptor in XML format" );
}

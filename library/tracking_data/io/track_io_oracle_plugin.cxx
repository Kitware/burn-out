/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>
#include <plugin_loader/plugin_config_util.h>

#include "track_writer_oracle.h"

#include <track_oracle/file_format_base.h>
#include <track_oracle/file_format_manager.h>

#include <sstream>

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

namespace  vidtk {
namespace {

file_format_enum& operator++(file_format_enum& in)
{
  in = static_cast<file_format_enum>(static_cast<int>(in) + 1);
  return in;
}


// Make a custom factory because the sub-help text is complicated and
// has to be determined at run time.

class oracle_writer_factory :
    public vidtk::plugin_factory_0 < vidtk::track_writer_oracle >
{
public:
  oracle_writer_factory(std::string const& itype )
    : vidtk::plugin_factory_0 < vidtk::track_writer_oracle >( itype )
  {
    bool first_format = true;
    std::stringstream oracle_help;

    oracle_help << "Output format for oracle writer (one of ";
    for ( file_format_enum off = TF_BEGIN; off != TF_INVALID_TYPE; ++off )
    {
      file_format_base const* const off_instance = file_format_manager::get_format( off );
      if ( off_instance && ( off_instance->supported_operations() & FF_WRITE_FILE ) )
      {
        if ( ! first_format ) { oracle_help << ", "; }
        oracle_help << file_format_type::to_string( off );
        first_format = false;
      }
    } // end for
    oracle_help << "); required when using oracle writer";

    add_config_attribute( *this, "oracle_format", "csv", oracle_help.str() );
  }


  virtual ~oracle_writer_factory() { }
};

} // end namespace
} // end namespace


// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  fact = pm->add_factory( new vidtk::oracle_writer_factory( typeid( vidtk::track_writer_interface ).name() ) );
  fact->add_attribute( "file-type", "oracle" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"oracle\" - Write tracks in track Oracle XML format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "Track Oracle track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes Track Oracle formatted tracks" );
}

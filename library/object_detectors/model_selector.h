/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_model_selector_h_
#define vidtk_model_selector_h_

#include <utilities/external_settings.h>

#include <vector>
#include <iostream>

namespace vidtk
{


#define settings_macro( add_param, add_array, add_enumr ) \
  add_param( \
    selector_filename, \
    std::string, \
    "", \
    "Filename containing the maximum parameter value that an individual, " \
    "model applies to followed by the filename for each model. The " \
    "parameter value can be \"Default\" to make the model be used when " \
    "there is no parameter." ) \

init_external_settings3( model_selector_settings, settings_macro );

#undef settings_macro


/// \brief A simple class for performing model selection based on a
/// single parameter, for example, gsd information about an image.
template< typename ModelType, typename ModelSettings >
class model_selector
{
public:

  model_selector();
  ~model_selector() {}

  /// Set any settings for this selector.
  bool configure( const model_selector_settings& settings )
  {
    std::ifstream input( settings.selector_filename );

    if( !input )
    {
      return false;
    }

    while( !input.eof() )
    {
      std::string param, fn;

      input << param << fn;

      ModelSettings settings;
      settings.read_from_file( fn );

      ModelType model;
      model.configure( settings );

      if( param == "Default" )
      {
        default_model_ =
      }
      else
      {

      }
    }

    input.close();
  }

  /// Return the correct model for the given parameter.
  boost::shared_ptr< ModelType > get_model( double param )
  {
    for( unsigned i = 0; i < params_.size(); ++i )
    {
      if( param < params_[i] && ( i == 0 || param >= params_[i-1] ) )
      {
        return models_[i];
      }
    }

    return default_model_;
  }

  /// Return the default model.
  boost::shared_ptr< ModelType > get_model()
  {
    return default_model_;
  }

private:

  // Stored models
  std::vector< boost::shared_ptr< ModelType > > models_;
  std::vector< double > params_;

  // The default model
  boost::shared_ptr< ModelType > default_model_;

};


} // end namespace vidtk

#endif // vidtk_model_selector_h_

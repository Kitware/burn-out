/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/cnn_descriptor.h>

#include <caffe/blob.hpp>
#include <caffe/common.hpp>

#include <vil/vil_convert.h>

#include <logger/logger.h>

using caffe::Blob;
using caffe::Net;
using caffe::Caffe;

namespace vidtk
{

VIDTK_LOGGER( "cnn_descriptor" );

bool
cnn_descriptor
::configure( const cnn_descriptor_settings& settings )
{
  if( settings.use_gpu )
  {
    CHECK_GE( settings.device_id, 0 );

    Caffe::SetDevice( settings.device_id );
    Caffe::set_mode( Caffe::GPU );
  }
  else
  {
    Caffe::set_mode( Caffe::CPU );
  }

  cnn_.reset( new Net< feature_t >( settings.model_definition, caffe::TEST ) );
  cnn_->CopyTrainedLayersFrom( settings.model_weights );

  layers_to_use_ = settings.layers_to_use;
  final_layer_str_ = settings.final_layer_string;
  final_layer_inds_ = settings.final_layer_indices;

  for( unsigned i = 0; i < layers_to_use_.size(); ++i )
  {
    if( layers_to_use_[i] == final_layer_str_ )
    {
      continue;
    }

    if( !cnn_->has_blob( layers_to_use_[i] ) )
    {
      LOG_ERROR( "Invalid Layer ID: " << layers_to_use_[i] );
      return false;
    }
  }

  return true;
}

void
cnn_descriptor
::compute( const input_image_t image, output_t& features )
{
  std::vector< output_t > batch_output;
  batch_compute( std::vector< input_image_t >( 1, image ), batch_output );

  if( batch_output.size() == 1 )
  {
    features = batch_output[0];
  }
  else
  {
    features.clear();
  }
}

void
cnn_descriptor
::batch_compute( const std::vector< input_image_t > images,
                 std::vector< output_t >& output )
{
  if( images.empty() )
  {
    output.clear();
    return;
  }

  output.clear();
  output.resize( images.size(), output_t( layers_to_use_.size(), feature_vec_t() ) );

  // Format input
  const unsigned image_count = images.size();
  const unsigned ni = images[0].ni();
  const unsigned nj = images[0].nj();
  const unsigned nplanes = images[0].nplanes();

  Blob< feature_t >* input_blob = new Blob< feature_t >( image_count, nplanes, nj, ni );

  for( unsigned i = 0; i < image_count; ++i )
  {
    feature_t* data_start = input_blob->mutable_cpu_data() + input_blob->offset( i );

    vil_image_view< feature_t > tmp( data_start, ni, nj, nplanes, 1, ni, ni*nj );
    vil_convert_cast( images[i], tmp );
  }

  std::vector< Blob< feature_t >* > input_vector;
  input_vector.push_back( input_blob );
  feature_t loss = 0.0;

  // Run CNN on all inputs at once
  const std::vector< Blob< feature_t >* >& cnn_output = cnn_->Forward( input_vector, &loss );

  if( cnn_output.empty() )
  {
    LOG_ERROR( "Invalid CNN result!" );
    return;
  }

  // Retrieve descriptors from internal CNN layers
  for( unsigned l = 0; l < layers_to_use_.size(); ++l )
  {
    if( layers_to_use_[l] == final_layer_str_ )
    {
      const unsigned batch_size = cnn_output[0]->num();
      const unsigned dim_features = cnn_output[0]->count() / batch_size;

      if( final_layer_inds_.empty() )
      {
        for( unsigned i = 0; i < batch_size; ++i )
        {
          const feature_t* start = cnn_output[0]->cpu_data() + cnn_output[0]->offset( i );
          output[i][l].insert( output[i][l].begin(), start, start + dim_features );
        }
      }
      else
      {
        for( unsigned i = 0; i < batch_size; ++i )
        {
          for( unsigned r = 0; r < final_layer_inds_.size(); ++r )
          {
            const feature_t* start = cnn_output[0]->cpu_data() + cnn_output[0]->offset( i );
            output[i][l].push_back( start[ final_layer_inds_[r] ] );
          }
        }
      }
    }
    else
    {
      const boost::shared_ptr< Blob< feature_t > > feature_blob =
        cnn_->blob_by_name( layers_to_use_[l] );

      const unsigned batch_size = feature_blob->num();
      const unsigned dim_features = feature_blob->count() / batch_size;

      for( unsigned i = 0; i < batch_size; ++i )
      {
        const feature_t* start = feature_blob->cpu_data() + feature_blob->offset( i );
        output[i][l].insert( output[i][l].begin(), start, start + dim_features );
      }
    }
  }
}

} // end namespace vidtk

/*ckwg +5
 * Copyright 2015-2017 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/cnn_descriptor.h>

#include <caffe/net.hpp>
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

class cnn_descriptor::priv
{
public:
  priv() {}
  ~priv() {}

  // The internally loaded CNN model and weights
  boost::shared_ptr< caffe::Net< feature_t > > cnn_;

  // Internal copy of externally set options
  std::vector< std::string > layers_to_use_;

  // Final layer string
  std::string final_layer_str_;

  // Indices to include in final layer
  std::vector< unsigned > final_layer_inds_;
};

cnn_descriptor::cnn_descriptor()
 : d( new priv() )
{
}

cnn_descriptor::~cnn_descriptor()
{
}

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

  d->cnn_.reset( new Net< feature_t >( settings.model_definition, caffe::TEST ) );
  d->cnn_->CopyTrainedLayersFrom( settings.model_weights );

  d->layers_to_use_ = settings.layers_to_use;
  d->final_layer_str_ = settings.final_layer_string;
  d->final_layer_inds_ = settings.final_layer_indices;

  for( unsigned i = 0; i < d->layers_to_use_.size(); ++i )
  {
    if( d->layers_to_use_[i] == d->final_layer_str_ )
    {
      continue;
    }

    if( !d->cnn_->has_blob( d->layers_to_use_[i] ) )
    {
      LOG_ERROR( "Invalid Layer ID: " << d->layers_to_use_[i] );
      return false;
    }
  }

  return true;
}

void
cnn_descriptor
::compute( const input_image_t& image, output_t& features )
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
::batch_compute( const std::vector< input_image_t >& images,
                 std::vector< output_t >& output )
{
  if( images.empty() )
  {
    output.clear();
    return;
  }

  output.clear();
  output.resize( images.size(), output_t( d->layers_to_use_.size(), feature_vec_t() ) );

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
  const std::vector< Blob< feature_t >* >& cnn_output = d->cnn_->Forward( input_vector, &loss );

  if( cnn_output.empty() )
  {
    LOG_ERROR( "Invalid CNN result!" );
    return;
  }

  // Retrieve descriptors from internal CNN layers
  for( unsigned l = 0; l < d->layers_to_use_.size(); ++l )
  {
    if( d->layers_to_use_[l] == d->final_layer_str_ )
    {
      const unsigned batch_size = cnn_output[0]->num();
      const unsigned dim_features = cnn_output[0]->count() / batch_size;

      if( d->final_layer_inds_.empty() )
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
          for( unsigned r = 0; r < d->final_layer_inds_.size(); ++r )
          {
            const feature_t* start = cnn_output[0]->cpu_data() + cnn_output[0]->offset( i );
            output[i][l].push_back( start[ d->final_layer_inds_[r] ] );
          }
        }
      }
    }
    else
    {
      const boost::shared_ptr< Blob< feature_t > > feature_blob =
        d->cnn_->blob_by_name( d->layers_to_use_[l] );

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

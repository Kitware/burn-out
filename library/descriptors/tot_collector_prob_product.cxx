/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tot_collector_prob_product.h"

#include <descriptors/online_descriptor_helper_functions.h>

#include <vector>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "tot_collector_prob_product" );

pvo_probability_sptr
tot_collector_prob_product
::classify( const track_sptr_t& /*track_object*/,
            const raw_descriptor::vector_t& descriptors )
{
  descriptor_sptr merged_output = compute_product( descriptors );

  if( !merged_output || merged_output->features_size() < 3 )
  {
    return NULL;
  }

  descriptor_data_t data = merged_output->get_features();
  pvo_probability_sptr pvo = pvo_probability_sptr( new pvo_probability() );

  pvo->set_probability_person( data[0] );
  pvo->set_probability_vehicle( data[1] );
  pvo->set_probability_other( data[2] );

  pvo->normalize();

  return pvo;
}

descriptor_sptr
tot_collector_prob_product
::compute_product( const raw_descriptor::vector_t& input_scores )
{
  if( input_scores.empty() )
  {
    return descriptor_sptr();
  }

  unsigned dims = input_scores[0]->features_size();

  descriptor_sptr output_tot = create_descriptor( "tot_collector" );
  output_tot->get_features() = descriptor_data_t( dims, 1.0 );

  raw_descriptor::vector_t::const_iterator vrd;

  for( vrd=input_scores.begin(); vrd!=input_scores.end(); ++vrd )
  {
    if( (*vrd)->features_size() != dims )
    {
      LOG_ERROR( "Input descriptor dimensions don't match" );
      return descriptor_sptr();
    }

    std::vector<double>::const_iterator it_input = (*vrd)->get_features().begin();
    std::vector<double>::iterator it_output = output_tot->get_features().begin();

    for( ; it_input != (*vrd)->get_features().end(); ++it_input, ++it_output )
    {
      *it_output *= *it_input;
    }
  }

  normalize_descriptor( output_tot );

  return output_tot;
}

} // end namespace vidtk

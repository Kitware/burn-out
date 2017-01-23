/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_learner_data_simple_wrapper_h_
#define vidtk_learner_data_simple_wrapper_h_

#include <learning/learner_data.h>

namespace vidtk
{

/// \brief A simple learner class which stores a pointer to external data.
///
/// This class is does not currently support all of the functionality of
/// the base learner class, in particular the stream operators for it are
/// incomplete.
class classifier_data_wrapper : public learner_training_data
{
public:

  classifier_data_wrapper( double const* ptr, unsigned total_size, int id = 1 )
   : learner_training_data( id ),
     ptr_( ptr ),
     count_( total_size )
  {
  }

  virtual vnl_vector<double> get_value( int ) const
  {
    return vnl_vector<double>( ptr_, count_ );
  }

  virtual double get_value_at( int, unsigned int loc ) const
  {
    return ptr_[ loc ];
  }

  virtual unsigned int number_of_descriptors() const
  {
    return 1;
  }

  virtual unsigned int number_of_components( unsigned int ) const
  {
    return count_;
  }

  virtual vnl_vector<double> vectorize() const
  {
    return vnl_vector<double>( ptr_, count_ );
  }

  virtual void write( std::ostream& ) const
  {
  }

  double const* ptr_;
  unsigned count_;
};

} //namespace vidtk

#endif

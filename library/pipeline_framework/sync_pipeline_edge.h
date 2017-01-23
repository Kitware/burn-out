/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sync_pipeline_edge_h_
#define vidtk_sync_pipeline_edge_h_

#include <boost/function.hpp>

#include <pipeline_framework/pipeline_edge.h>

#include <string>


namespace vidtk
{

template<class T>
class pipeline_edge_data
{
public:
  pipeline_edge_data( T d )
    : datum_( d )
  {
  }

  T& datum()
  {
    return datum_;
  }

private:
  T datum_;
};


template<class T>
class pipeline_edge_data<T&>
{
public:
  pipeline_edge_data( T& d )
    : datum_( &d )
  {
  }

  T& datum()
  {
    return *datum_;
  }

private:
  T* datum_;
};


template<class OT, class IT>
struct sync_pipeline_edge_impl
  : pipeline_edge_impl<OT, IT>
{
  typedef pipeline_edge_data<OT> datum_type;
  datum_type* datum_;

  // edge_capacity supposed to be always 1 in case of sync pipeline
  sync_pipeline_edge_impl( unsigned /*edge_capacity*/ )
    : datum_( NULL )
  {
  }

  ~sync_pipeline_edge_impl()
  {
    delete datum_;
  }

  // We can't simply do
  //
  //    set_input_( get_output_() );
  //
  // in execute() because it is possible that the output type is a
  // temporary object, and that the input pipe will keep a reference
  // to it.  To allow for this, the pipeline will keep a copy of that
  // object, and give the input pipe a reference to the copy.  The
  // copy will persist beyond the call to the input node's execute,
  // so everyone is happy.
  //
  // If OT is a reference or a pointer, keeping a copy is cheap, and
  // so it's not a big deal.  If OT is a full object, keeping a copy
  // may be expensive, but is unavoidable if IT is a reference.
  // However, if the input node does not keep a pointer to the data
  // object, then it is a waste to copy the object.  The pipeline
  // cannot know that, so it may be useful to allow a property to
  // disable the copying when the user insists.
  //
  // *** Also, if the IT is not by reference, but is by copy, then
  // there is no need for the pipeline to keep a copy.  We should
  // automatically detect this case can not keep a copy around.  We
  // don't do that right now.

  virtual void pull_data()
  {
    if( datum_ )
    {
      *datum_ = datum_type( this->get_output_() );
    }
    else
    {
      datum_ = new datum_type( this->get_output_() );
    }
  }

  virtual process::step_status push_data()
  {
    this->set_input_( datum_->datum() );
    return process::SUCCESS;
  }

};


} // end namespace vidtk


#endif // sync_pipeline_edge_h_

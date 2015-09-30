/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sample_nodes.h"

#include <vcl_algorithm.h>
#include <pipeline/sync_pipeline.h>
#include <pipeline/async_pipeline.h>
#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

struct fail_gen
  : public vidtk::process
{
  typedef fail_gen self_type;

  fail_gen( vcl_string const& name,
            unsigned fail_value = 15,
            unsigned constant = 5 )
    : vidtk::process( name, "fail_gen" ),
    generated_failure_( false ),
    value_( 0 ),
    fail_value_( fail_value ),
    constant_( constant )
  {
  }

  vidtk::config_block params() const
  {
    return vidtk::config_block();
  }

  bool set_params( vidtk::config_block const& )
  {
    return true;
  }

  bool initialize()
  {
    return true;
  }

  bool reset()
  {
    this->generated_failure_ = false;
    return true;
  }

  bool step()
  {
    if( fail_value_ == this->value_ )
    {
      generated_failure_ = true;
      return false;
    }

    this->value_ += this->constant_;
    return true;
  }

  void set_value( unsigned d )
  {
    this->value_ = d;
  }

  VIDTK_INPUT_PORT( set_value, unsigned );

  unsigned value() const
  {
    return this->value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, value);

  bool generated_failure_;
  unsigned value_;
  unsigned fail_value_;
  unsigned constant_;
};


template< class P >
struct recoverable_sp
  : public vidtk::super_process
{
  typedef recoverable_sp<P> self_type;
  typedef vidtk::super_process_pad_impl<unsigned> unsigned_pad;

  recoverable_sp( vcl_string const& name )
    : vidtk::super_process( name, "recoverable_sp" ),
      fail_gen1_( new fail_gen( "fail_gen1", 5 ) ),
      fail_gen2_( new fail_gen( "fail_gen2", 15 ) ),
      in_pad_( new unsigned_pad( "input", 0 ) ),
      out_pad_( new unsigned_pad( "output", 0 ) )
  {
    this->pipe_ = new P;
    this->pipeline_ = this->pipe_;
  }

  vidtk::config_block params() const
  {
    return vidtk::config_block();
  }

  bool set_params( vidtk::config_block const& )
  {
    return true;
  }

  bool initialize()
  {
    this->pipeline_->add( this->fail_gen1_ );
    this->pipeline_->add( this->fail_gen2_ );
    this->pipeline_->add( this->in_pad_ );
    this->pipeline_->add( this->out_pad_ );

    // input pad connections
    this->pipe_->connect( this->in_pad_->value_port(),
                          this->fail_gen1_->set_value_port() );

    // internal pipeline connections
    this->pipe_->connect( this->fail_gen1_->value_port(),
                          this->fail_gen2_->set_value_port() );

    // output pad connections
    this->pipe_->connect( this->fail_gen2_->value_port(),
                          this->out_pad_->set_value_port() );

    return this->pipeline_->initialize();
  }

  step_status step2()
  {
    return this->pipeline_->execute();
  }

  bool fail_recover()
  {
    if( fail_gen2_->generated_failure_ )
    {
      vcl_cout << "reseting pipeline at "<<fail_gen2_->name()<<vcl_endl;
      return pipeline_->reset_downstream( fail_gen2_.ptr() );
    }
    else if( fail_gen1_->generated_failure_ )
    {
      vcl_cout << "reseting pipeline at "<<fail_gen1_->name()<<vcl_endl;
      return pipeline_->reset_downstream( fail_gen1_.ptr() );
    }
    return false;
  }

  void set_value( unsigned d )
  {
    this->in_pad_->set_value(d);
  }

  VIDTK_INPUT_PORT( set_value, unsigned );

  unsigned value() const
  {
    return this->out_pad_->value();
  }

  VIDTK_OUTPUT_PORT( unsigned, value );


private:
  vidtk::process_smart_pointer< fail_gen > fail_gen1_;
  vidtk::process_smart_pointer< fail_gen > fail_gen2_;

  vidtk::process_smart_pointer< unsigned_pad > in_pad_;
  vidtk::process_smart_pointer< unsigned_pad > out_pad_;
  P* pipe_;
};


template <class P1, class P2>
void
test_reset()
{
  P1 p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", 20 ) );
  process_smart_pointer< recoverable_sp< P2 > > rec( new recoverable_sp< P2 >( "recoverable" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( rec );
  p.add( collect );

  p.connect( nums->value_port(),
             rec->set_value_port() );
  p.connect( rec->value_port(),
             collect->set_value_port() );

  unsigned correct_values[] = {10, 11, 12, 13, 14, 16, 17, 18, 19,
                               21, 22, 23, 24, 25, 26, 27, 28, 29 };
  unsigned num_values = sizeof(correct_values) / sizeof(unsigned);

  TEST( "Pipeline initialize", p.initialize(), true );
  bool run_status = p.run();
  TEST( "Run", run_status, true);
  TEST( "Number of steps", collect->values_.size(), num_values );

  bool values_correct = true;
  unsigned num_itr = vcl_min(static_cast<unsigned>(collect->values_.size()), 18u);
  for( unsigned i=0; i<num_itr; ++i)
  {
    if( collect->values_[i] != correct_values[i] )
    {
      values_correct = false;
      break;
    }
  }
  TEST( "Values correct", values_correct, true);

  if( !values_correct || num_values != collect->values_.size() )
  {
    vcl_cout << "pipeline values: ";
    for( unsigned i=0; i<collect->values_.size(); ++i)
    {
      vcl_cout << collect->values_[i]<< " ";
    }
    vcl_cout << vcl_endl;
    vcl_cout << "pipeline values: ";
    for( unsigned i=0; i<18; ++i)
    {
      vcl_cout << correct_values[i]<< " ";
    }
    vcl_cout << vcl_endl;
  }
}


} // end anonymous namespace

int test_reset_pipeline( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_reset_pipeline" );

  vcl_cout << "-------------------------------------------------\n"
           << "Testing sync_pipeline nested within sync_pipeline\n"
           << "-------------------------------------------------" << vcl_endl;
  test_reset< sync_pipeline, sync_pipeline >();

  vcl_cout << "--------------------------------------------------\n"
           << "Testing sync_pipeline nested within async_pipeline\n"
           << "--------------------------------------------------" << vcl_endl;
  test_reset< async_pipeline, sync_pipeline >();

  vcl_cout << "---------------------------------------------------\n"
           << "Testing async_pipeline nested within async_pipeline\n"
           << "---------------------------------------------------" << vcl_endl;
  test_reset< async_pipeline, async_pipeline >();


  return testlib_test_summary();
}

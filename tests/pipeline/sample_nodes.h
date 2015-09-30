/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_vector.h>
#include <utilities/config_block.h>
#include <pipeline/super_process.h>
#include <process_framework/process.h>

namespace
{
bool is_odd(unsigned n)
{
  return n % 2;
}
}

struct const_value
  : public vidtk::process
{
  typedef const_value self_type;

  const_value( vcl_string const& name, unsigned value )
    : vidtk::process( name, "const_value" ),
      value_( value )
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

  bool step()
  {
    return true;
  }

  unsigned value() const
  {
    return this->value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

  unsigned value_;
};

struct collect_value
  : public vidtk::process
{
  typedef collect_value self_type;

  collect_value( vcl_string const& name,
                 unsigned max_size = static_cast<unsigned>(-1) )
    : vidtk::process( name, "collect_value" ),
    max_size_( max_size )
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

  bool step()
  {
    if( max_size_ != static_cast<unsigned>(-1) && values_.size() >= max_size_ )
    {
      return false;
    }
    return true;
  }

  void set_value( unsigned d )
  {
    values_.push_back(d);
  }

  VIDTK_INPUT_PORT( set_value, unsigned );

  vcl_vector<unsigned> values_;
  unsigned max_size_;
};

struct multi_push
  : public vidtk::process
{
  typedef multi_push self_type;

  multi_push( vcl_string const& name, unsigned num_push = 0)
    : vidtk::process( name, "collect_value" ),
      num_push_( num_push )
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

  process::step_status step2()
  {
    for( unsigned i=0; i<num_push_; ++i )
    {
      this->push_output(process::SUCCESS);
      this->value_ += 1;
    }
    return process::SUCCESS;
  }

  void set_value( unsigned d )
  {
    value_ = d;
  }

  VIDTK_INPUT_PORT( set_value, unsigned );

  unsigned value() const
  {
    return this->value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

  unsigned value_;
  unsigned num_push_;
};

template< class P, class Child >
struct simple_sp
  : public vidtk::super_process
{
  typedef simple_sp<P, Child> self_type;
  typedef vidtk::super_process_pad_impl<unsigned> unsigned_pad;

  simple_sp( vcl_string const& name,
             const vidtk::process_smart_pointer< Child >& child,
             P* pipeline)
    : vidtk::super_process( name, "simple_sp" ),
      child_( child ),
      in_value_pad_( new unsigned_pad( "in_value", 0 ) ),
      out_value_pad_( new unsigned_pad( "out_value", 0 ) ),
      pipe_(pipeline)
  {
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
    this->pipeline_->add( this->child_ );

    this->pipeline_->add( this->in_value_pad_ );
    this->pipeline_->add( this->out_value_pad_ );

    // input pad connections
    this->pipe_->connect( this->in_value_pad_->value_port(),
                          this->child_->set_value_port() );
    this->pipe_->connect( this->child_->value_port(),
                          this->out_value_pad_->set_value_port() );

    return this->pipeline_->initialize();
  }

  step_status step2()
  {
    return this->pipeline_->execute();
  }

  void set_value( unsigned d )
  {
    this->in_value_pad_->set_value(d);
  }

  VIDTK_INPUT_PORT( set_value, unsigned );

  unsigned value() const
  {
    return this->out_value_pad_->value();
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

private:
  vidtk::process_smart_pointer< Child > child_;

  vidtk::process_smart_pointer< unsigned_pad > in_value_pad_;
  vidtk::process_smart_pointer< unsigned_pad > out_value_pad_;
  P* pipe_;
};

struct collect_value_community
  : public vidtk::process
{
  typedef collect_value_community self_type;

  collect_value_community( vcl_string const& name)
    : vidtk::process( name, "collect_value_community" )
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

  bool step()
  {
    return true;
  }

  void set_value( unsigned d )
  {
    values_.push_back(d);
  }

  VIDTK_INPUT_PORT( set_value, unsigned );

  void set_value_dummy1( unsigned )
  {
  }

  VIDTK_INPUT_PORT( set_value_dummy1, unsigned );

  void set_value_dummy2( unsigned )
  {
  }

  VIDTK_INPUT_PORT( set_value_dummy2, unsigned );

  void set_value_dummy3( unsigned )
  {
  }

  VIDTK_INPUT_PORT( set_value_dummy3, unsigned );

  vcl_vector<unsigned> values_;
};

struct add
  : public vidtk::process
{
  typedef add self_type;

  add( vcl_string const& name )
    : vidtk::process( name, "add" )
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
    this->value1_ = 0;
    this->value2_ = 0;
    return true;
  }

  bool step()
  {
    return true;
  }

  void set_value1( unsigned d )
  {
    this->value1_ = d;
  }

  VIDTK_INPUT_PORT( set_value1, unsigned );

  void set_value2( unsigned d )
  {
    this->value2_ = d;
  }

  VIDTK_INPUT_PORT( set_value2, unsigned );

  unsigned sum() const
  {
    return this->value1_ + this->value2_;
  }

  VIDTK_OUTPUT_PORT( unsigned, sum );

  unsigned value1_;
  unsigned value2_;
};

struct multiply
  : public vidtk::process
{
  typedef multiply self_type;

  multiply( vcl_string const& name )
    : vidtk::process( name, "multiply" )
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
    this->value1_ = 0;
    this->value2_ = 0;
    return true;
  }

  bool step()
  {
    return true;
  }

  void set_value1( unsigned d )
  {
    this->value1_ = d;
  }

  VIDTK_INPUT_PORT( set_value1, unsigned );

  void set_value2( unsigned d )
  {
    this->value2_ = d;
  }

  VIDTK_INPUT_PORT( set_value2, unsigned );

  unsigned product() const
  {
    return this->value1_ * this->value2_;
  }

  VIDTK_OUTPUT_PORT( unsigned, product );

  unsigned value1_;
  unsigned value2_;
};

struct numbers
  : public vidtk::process
{
  typedef numbers self_type;

  numbers( vcl_string const& name, unsigned num_steps = 10, unsigned start = 0 )
    : vidtk::process( name, "numbers" ),
      value_( start ),
      old_value_( start ),
      num_steps_( num_steps ),
      step_count_( 0 )
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

  bool step()
  {
    this->old_value_ = this->value_++;

    return ( step_count_++ < num_steps_ );
  }

  unsigned value() const
  {
    return this->old_value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

  unsigned value_;
  unsigned old_value_;

  unsigned num_steps_;
  unsigned step_count_;
};

struct numbers_pred
  : public vidtk::process
{
  typedef numbers_pred self_type;
  typedef bool (*pred)(unsigned);

  numbers_pred( vcl_string const& name, pred predicate, unsigned num_steps = 10, unsigned start = 0 )
    : vidtk::process( name, "numbers_pred" ),
      predicate_(predicate),
      value_( start ),
      old_value_( start ),
      num_steps_( num_steps ),
      step_count_( 0 )
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

  bool step()
  {
    this->old_value_ = this->value_++;

    return ( step_count_++ < num_steps_ );
  }

  step_status step2()
  {
    if( !this->predicate_( this->value_ ) )
    {
      ++this->value_;
      return SKIP;
    }

    return vidtk::process::step2();
  }

  unsigned value() const
  {
    return this->old_value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

  pred predicate_;

  unsigned value_;
  unsigned old_value_;

  unsigned num_steps_;
  unsigned step_count_;
};

template< class P >
struct product_of_increments
  : public vidtk::super_process
{
  typedef product_of_increments<P> self_type;
  typedef vidtk::super_process_pad_impl<unsigned> unsigned_pad;

  product_of_increments( vcl_string const& name )
    : vidtk::super_process( name, "product_of_increments" ),
      multiplier_( new multiply( "multiplier" ) ),
      adder_( new add( "adder" ) ),
      subtotal_( new add( "subtotal" ) ),
      one_( new const_value( "one", 1 ) ),
      total_( new add( "total" ) ),
      value_pad_( new unsigned_pad( "value", 0 ) ),
      value1_pad_( new unsigned_pad( "value1", 0 ) ),
      value2_pad_( new unsigned_pad( "value2", 0 ) )
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
    this->pipeline_->add( this->multiplier_ );
    this->pipeline_->add( this->adder_ );
    this->pipeline_->add( this->subtotal_ );
    this->pipeline_->add( this->one_ );
    this->pipeline_->add( this->total_ );

    this->pipeline_->add( this->value_pad_ );
    this->pipeline_->add( this->value1_pad_ );
    this->pipeline_->add( this->value2_pad_ );

    // input pad connections
    this->pipe_->connect( this->value1_pad_->value_port(),
                          this->adder_->set_value1_port() );
    this->pipe_->connect( this->value1_pad_->value_port(),
                          this->multiplier_->set_value1_port() );
    this->pipe_->connect( this->value2_pad_->value_port(),
                          this->adder_->set_value2_port() );
    this->pipe_->connect( this->value2_pad_->value_port(),
                          this->multiplier_->set_value2_port() );

    // internal pipeline connections
    this->pipe_->connect( this->multiplier_->product_port(),
                          this->subtotal_->set_value1_port() );
    this->pipe_->connect( this->adder_->sum_port(),
                          this->subtotal_->set_value2_port() );
    this->pipe_->connect( this->subtotal_->sum_port(),
                          this->total_->set_value1_port() );
    this->pipe_->connect( this->one_->value_port(),
                          this->total_->set_value2_port() );

    // output pad connections
    this->pipe_->connect( this->total_->sum_port(),
                          this->value_pad_->set_value_port() );

    return this->pipeline_->initialize();
  }

  step_status step2()
  {
    return this->pipeline_->execute();
  }

  void set_value1( unsigned d )
  {
    this->value1_pad_->set_value(d);
  }

  VIDTK_INPUT_PORT( set_value1, unsigned );

  void set_value2( unsigned d )
  {
    this->value2_pad_->set_value( d );
  }

  VIDTK_INPUT_PORT( set_value2, unsigned );

  unsigned value() const
  {
    return this->value_pad_->value();
  }

  VIDTK_OUTPUT_PORT( unsigned, value );


private:
  vidtk::process_smart_pointer< multiply > multiplier_;
  vidtk::process_smart_pointer< add > adder_;
  vidtk::process_smart_pointer< add > subtotal_;
  vidtk::process_smart_pointer< const_value > one_;
  vidtk::process_smart_pointer< add > total_;

  vidtk::process_smart_pointer< unsigned_pad > value_pad_;
  vidtk::process_smart_pointer< unsigned_pad > value1_pad_;
  vidtk::process_smart_pointer< unsigned_pad > value2_pad_;
  P* pipe_;
};

template< class P >
struct product_of_increments_pred
  : public vidtk::super_process
{
  typedef product_of_increments_pred<P> self_type;
  typedef vidtk::super_process_pad_impl<unsigned> unsigned_pad;
  typedef bool (*pred)(unsigned);

  product_of_increments_pred( vcl_string const& name, pred predicate )
    : vidtk::super_process( name, "product_of_increments_pred" ),
      predicate_( predicate ),
      multiplier_( new multiply( "multiplier" ) ),
      adder_( new add( "adder" ) ),
      subtotal_( new add( "subtotal" ) ),
      one_( new const_value( "one", 1 ) ),
      total_( new add( "total" ) ),
      value_pad_( new unsigned_pad( "value", 0 ) ),
      value1_pad_( new unsigned_pad( "value1", 0 ) ),
      value2_pad_( new unsigned_pad( "value2", 0 ) )
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
    this->pipeline_->add( this->multiplier_ );
    this->pipeline_->add( this->adder_ );
    this->pipeline_->add( this->subtotal_ );
    this->pipeline_->add( this->one_ );
    this->pipeline_->add( this->total_ );

    this->pipeline_->add( this->value_pad_ );
    this->pipeline_->add( this->value1_pad_ );
    this->pipeline_->add( this->value2_pad_ );

    // input pad connections
    this->pipe_->connect( this->value1_pad_->value_port(),
                          this->adder_->set_value1_port() );
    this->pipe_->connect( this->value1_pad_->value_port(),
                          this->multiplier_->set_value1_port() );
    this->pipe_->connect( this->value2_pad_->value_port(),
                          this->adder_->set_value2_port() );
    this->pipe_->connect( this->value2_pad_->value_port(),
                          this->multiplier_->set_value2_port() );

    // internal pipeline connections
    this->pipe_->connect( this->multiplier_->product_port(),
                          this->subtotal_->set_value1_port() );
    this->pipe_->connect( this->adder_->sum_port(),
                          this->subtotal_->set_value2_port() );
    this->pipe_->connect( this->subtotal_->sum_port(),
                          this->total_->set_value1_port() );
    this->pipe_->connect( this->one_->value_port(),
                          this->total_->set_value2_port() );

    // output pad connections
    this->pipe_->connect( this->total_->sum_port(),
                          this->value_pad_->set_value_port() );

    return this->pipeline_->initialize();
  }

  step_status step2()
  {
    step_status st = this->pipeline_->execute();

    if( !predicate_( value() ) )
    {
      return SKIP;
    }

    return st;
  }

  void set_value1( unsigned d )
  {
    this->value1_pad_->set_value(d);
  }

  VIDTK_INPUT_PORT( set_value1, unsigned );

  void set_value2( unsigned d )
  {
    this->value2_pad_->set_value( d );
  }

  VIDTK_INPUT_PORT( set_value2, unsigned );

  unsigned value() const
  {
    return this->value_pad_->value();
  }

  VIDTK_OUTPUT_PORT( unsigned, value );


private:
  pred predicate_;

  vidtk::process_smart_pointer< multiply > multiplier_;
  vidtk::process_smart_pointer< add > adder_;
  vidtk::process_smart_pointer< add > subtotal_;
  vidtk::process_smart_pointer< const_value > one_;
  vidtk::process_smart_pointer< add > total_;

  vidtk::process_smart_pointer< unsigned_pad > value_pad_;
  vidtk::process_smart_pointer< unsigned_pad > value1_pad_;
  vidtk::process_smart_pointer< unsigned_pad > value2_pad_;
  P* pipe_;
};

template< class P >
struct product_of_increments_count
  : public vidtk::super_process
{
  typedef product_of_increments_count<P> self_type;
  typedef vidtk::super_process_pad_impl<unsigned> unsigned_pad;

  product_of_increments_count( vcl_string const& name )
    : vidtk::super_process( name, "product_of_increments_count" ),
      multiplier_( new multiply( "multiplier" ) ),
      adder_( new add( "adder" ) ),
      subtotal_( new add( "subtotal" ) ),
      one_( new const_value( "one", 1 ) ),
      total_( new add( "total" ) ),
      count_( new numbers_pred( "counter", is_odd ) ),
      value_pad_( new unsigned_pad( "value", 0 ) ),
      count_pad_( new unsigned_pad( "count", 0 ) ),
      value1_pad_( new unsigned_pad( "value1", 0 ) ),
      value2_pad_( new unsigned_pad( "value2", 0 ) )
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
    this->pipeline_->add( this->multiplier_ );
    this->pipeline_->add( this->adder_ );
    this->pipeline_->add( this->subtotal_ );
    this->pipeline_->add( this->one_ );
    this->pipeline_->add( this->total_ );
    this->pipeline_->add( this->count_ );

    this->pipeline_->add( this->value_pad_ );
    this->pipeline_->add( this->count_pad_ );
    this->pipeline_->add( this->value1_pad_ );
    this->pipeline_->add( this->value2_pad_ );

    // input pad connections
    this->pipe_->connect( this->value1_pad_->value_port(),
                          this->adder_->set_value1_port() );
    this->pipe_->connect( this->value1_pad_->value_port(),
                          this->multiplier_->set_value1_port() );
    this->pipe_->connect( this->value2_pad_->value_port(),
                          this->adder_->set_value2_port() );
    this->pipe_->connect( this->value2_pad_->value_port(),
                          this->multiplier_->set_value2_port() );

    // internal pipeline connections
    this->pipe_->connect( this->multiplier_->product_port(),
                          this->subtotal_->set_value1_port() );
    this->pipe_->connect( this->adder_->sum_port(),
                          this->subtotal_->set_value2_port() );
    this->pipe_->connect( this->subtotal_->sum_port(),
                          this->total_->set_value1_port() );
    this->pipe_->connect( this->one_->value_port(),
                          this->total_->set_value2_port() );

    // output pad connections
    this->pipe_->connect( this->total_->sum_port(),
                          this->value_pad_->set_value_port() );
    this->pipe_->connect( this->count_->value_port(),
                          this->count_pad_->set_value_port() );

    return this->pipeline_->initialize();
  }

  process::step_status step2()
  {
    return this->pipeline_->execute();
  }

  void set_value1( unsigned d )
  {
    this->value1_pad_->set_value(d);
  }

  VIDTK_INPUT_PORT( set_value1, unsigned );

  void set_value2( unsigned d )
  {
    this->value2_pad_->set_value( d );
  }

  VIDTK_INPUT_PORT( set_value2, unsigned );

  unsigned value() const
  {
    return this->value_pad_->value();
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

  unsigned count() const
  {
    return this->count_pad_->value();
  }

  VIDTK_OUTPUT_PORT( unsigned, count );


private:
  vidtk::process_smart_pointer< multiply > multiplier_;
  vidtk::process_smart_pointer< add > adder_;
  vidtk::process_smart_pointer< add > subtotal_;
  vidtk::process_smart_pointer< const_value > one_;
  vidtk::process_smart_pointer< add > total_;
  vidtk::process_smart_pointer< numbers_pred > count_;

  vidtk::process_smart_pointer< unsigned_pad > value_pad_;
  vidtk::process_smart_pointer< unsigned_pad > count_pad_;
  vidtk::process_smart_pointer< unsigned_pad > value1_pad_;
  vidtk::process_smart_pointer< unsigned_pad > value2_pad_;
  P* pipe_;
};

struct busy_loop
  : public vidtk::process
{
  typedef busy_loop self_type;

  busy_loop( vcl_string const& name, unsigned num_steps )
    : vidtk::process( name, "busy_loop" ),
    value_( 1 ),
    num_steps_(num_steps)
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

  bool step()
  {
    // Mark as volatile so the optimizer doesn't get smart with us.
    volatile unsigned a;

    for( a = 0; a < num_steps_; ++a )
    {
      // Do some math workouts to take some time.
      a *= 20;
      a /= 5;
      a >>= 2;
    }

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

  unsigned value_;
  unsigned num_steps_;
};


template< class P >
struct busy_sequence
  : public vidtk::super_process
{
  typedef busy_sequence<P> self_type;
  typedef vidtk::super_process_pad_impl<unsigned> unsigned_pad;

  busy_sequence( vcl_string const& name,
                 unsigned num_steps1, unsigned num_steps2)
    : vidtk::super_process( name, "product_of_increments" ),
      busy1_( new busy_loop( "busy1", num_steps1 ) ),
      busy2_( new busy_loop( "busy2", num_steps2 ) ),
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
    this->pipeline_->add( this->busy1_ );
    this->pipeline_->add( this->busy2_ );
    this->pipeline_->add( this->in_pad_ );
    this->pipeline_->add( this->out_pad_ );

    // input pad connections
    this->pipe_->connect( this->in_pad_->value_port(),
                          this->busy1_->set_value_port() );

    // internal pipeline connections
    this->pipe_->connect( this->busy1_->value_port(),
                          this->busy2_->set_value_port() );

    // output pad connections
    this->pipe_->connect( this->busy2_->value_port(),
                          this->out_pad_->set_value_port() );

    return this->pipeline_->initialize();
  }

  step_status step2()
  {
    return this->pipeline_->execute();
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
  vidtk::process_smart_pointer< busy_loop > busy1_;
  vidtk::process_smart_pointer< busy_loop > busy2_;

  vidtk::process_smart_pointer< unsigned_pad > in_pad_;
  vidtk::process_smart_pointer< unsigned_pad > out_pad_;
  P* pipe_;
};

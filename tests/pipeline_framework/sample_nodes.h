/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vector>
#include <utilities/config_block.h>
#include <pipeline_framework/super_process.h>
#include <process_framework/process.h>
#include <boost/thread/thread.hpp>

#ifdef __GNUC__
#define VIDTK_TEST_UNUSED __attribute__((__unused__))
#else
#define VIDTK_TEST_UNUSED
#endif

namespace
{
VIDTK_TEST_UNUSED
bool is_odd(unsigned n)
{
  return ((n % 2) == 1);
}
}

struct const_value
  : public vidtk::process
{
  typedef const_value self_type;

  const_value( std::string const& _name, unsigned _value, unsigned num_steps = 100)
    : vidtk::process( _name, "const_value" ),
      value_( _value ),
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
    step_count_ = 0;
    return true;
  }

  bool step()
  {
    return ( step_count_++ < num_steps_ );
  }

  unsigned value() const
  {
    return this->value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

  unsigned value_;
  unsigned num_steps_;
  unsigned step_count_;
};

struct collect_value
  : public vidtk::process
{
  typedef collect_value self_type;

  collect_value( std::string const& _name,
                 unsigned max_size = static_cast<unsigned>(-1) )
    : vidtk::process( _name, "collect_value" ),
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

  std::vector<unsigned> values_;
  unsigned max_size_;
};

struct step_counter
  : public vidtk::process
{
  typedef step_counter self_type;

  step_counter( std::string const& _name )
    : vidtk::process( _name, "step_counter" ),
      counter_(0)
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
    counter_++;
    return true;
  }

  void set_value( unsigned ) {}
  VIDTK_INPUT_PORT( set_value, unsigned );

  unsigned get_count() { return counter_; }
  VIDTK_OUTPUT_PORT( unsigned, get_count );

  unsigned counter_;
};

struct multi_push
  : public vidtk::process
{
  typedef multi_push self_type;

  multi_push( std::string const& _name, unsigned num_push = 0)
    : vidtk::process( _name, "multi_push" ),
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
      this->value_ += 10;
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

  simple_sp( std::string const& _name,
             const vidtk::process_smart_pointer< Child >& child,
             P* pipeline)
    : vidtk::super_process( _name, "simple_sp" ),
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

  collect_value_community( std::string const& _name)
    : vidtk::process( _name, "collect_value_community" )
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

  std::vector<unsigned> values_;
};

struct add
  : public vidtk::process
{
  typedef add self_type;

  add( std::string const& _name )
    : vidtk::process( _name, "add" )
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

  multiply( std::string const& _name )
    : vidtk::process( _name, "multiply" )
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

  numbers( std::string const& _name, unsigned num_steps = 10, unsigned start = 0 )
    : vidtk::process( _name, "numbers" ),
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

  numbers_pred( std::string const& _name, pred predicate, unsigned num_steps = 10, unsigned start = 0 )
    : vidtk::process( _name, "numbers_pred" ),
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

struct pred_proc
  : public vidtk::process
{
  typedef pred_proc self_type;
  typedef bool (*pred)(unsigned);

  pred_proc( std::string const& _name, pred predicate )
    : vidtk::process( _name, "pred" ),
      predicate_(predicate)
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

    return true;
  }

  step_status step2()
  {
    if( !this->predicate_( this->value_ ) )
    {
      return SKIP;
    }

    return vidtk::process::step2();
  }

  void set_input(unsigned value)
  {
    this->value_ = value;
  }

  VIDTK_INPUT_PORT( set_input, unsigned );

  unsigned output() const
  {
    return this->old_value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, output );

  pred predicate_;

  unsigned value_;
  unsigned old_value_;
};

template< class P >
struct product_of_increments
  : public vidtk::super_process
{
  typedef product_of_increments<P> self_type;
  typedef vidtk::super_process_pad_impl<unsigned> unsigned_pad;

  product_of_increments( std::string const& _name )
    : vidtk::super_process( _name, "product_of_increments" ),
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

  product_of_increments_pred( std::string const& _name, pred predicate )
    : vidtk::super_process( _name, "product_of_increments_pred" ),
      multiplier_( new multiply( "multiplier" ) ),
      adder_( new add( "adder" ) ),
      subtotal_( new add( "subtotal" ) ),
      one_( new const_value( "one", 1 ) ),
      total_( new add( "total" ) ),
      pred_( new pred_proc( "pred", predicate ) ),
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
    this->pipeline_->add( this->pred_ );

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

    this->pipe_->connect( this->total_->sum_port(),
                          this->pred_->set_input_port() );

    // output pad connections
    this->pipe_->connect( this->pred_->output_port(),
                          this->value_pad_->set_value_port() );

    return this->pipeline_->initialize();
  }

  step_status step2()
  {
    step_status st = this->pipeline_->execute();

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
  vidtk::process_smart_pointer< multiply > multiplier_;
  vidtk::process_smart_pointer< add > adder_;
  vidtk::process_smart_pointer< add > subtotal_;
  vidtk::process_smart_pointer< const_value > one_;
  vidtk::process_smart_pointer< add > total_;
  vidtk::process_smart_pointer< pred_proc > pred_;

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

  product_of_increments_count( std::string const& _name )
    : vidtk::super_process( _name, "product_of_increments_count" ),
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

  busy_loop( std::string const& _name, unsigned num_steps )
    : vidtk::process( _name, "busy_loop" ),
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

  busy_sequence( std::string const& _name,
                 unsigned num_steps1, unsigned num_steps2)
    : vidtk::super_process( _name, "product_of_increments" ),
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

struct skip_adder
  : public vidtk::process
{
  typedef skip_adder self_type;

  skip_adder( std::string const& _name )
    : vidtk::process( _name, "skip_adder" ),
    value_( 1 )
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

  bool skipped()
  {
    ++this->value_;
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
};

struct special_output
  : public vidtk::process
{
  typedef special_output self_type;

  special_output( std::string const& _name,
                  process::step_status status_to_produce,
                  unsigned success_frames_before_status = 9,
                  unsigned status_frames_to_send = 1,
                  bool await_signal_before_status = false )
  : vidtk::process( _name, "special_output" ),
    value_( 0 ),
    success_steps_( success_frames_before_status ),
    step_count_( 0 ),
    special_status_( status_to_produce )
  {
    total_steps_ = success_steps_ + status_frames_to_send;
    signals_received_ = ( await_signal_before_status ? 0 : 1 );
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
    step_count_ = 0;
    return true;
  }

  process::step_status step2()
  {
    process::step_status ret_val = process::FAILURE;
    step_count_++;

    if( step_count_ <= success_steps_ )
    {
      ret_val = process::SUCCESS;
    }
    else if( step_count_ <= total_steps_ )
    {
      // Spin lock. This is a test so who cares.
      while( !signals_received_ )
      {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
      }
      ret_val = special_status_;
    }

    return ret_val;
  }

  unsigned value() const
  {
    return this->value_;
  }

  VIDTK_OUTPUT_PORT( unsigned, value );

  void send_status_signals()
  {
    signals_received_++;
  }

  unsigned value_;
  unsigned success_steps_;
  unsigned total_steps_;
  unsigned step_count_;
  process::step_status special_status_;
  unsigned signals_received_;
};

struct infinite_looper
  : public vidtk::process
{
  typedef infinite_looper self_type;

  infinite_looper( std::string const& _name )
  : vidtk::process( _name, "infinite_looper" ),
    loop_flag_( 1 )
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
    // Spin lock. This is a test so who cares.
    while( loop_flag_ )
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
    return process::FAILURE;
  }

  void set_value( unsigned ) {}
  VIDTK_INPUT_PORT( set_value, unsigned );

  void exit_loop()
  {
    loop_flag_ = 0;
  }

  unsigned loop_flag_;
};

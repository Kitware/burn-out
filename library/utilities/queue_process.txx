/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/queue_process.h>

#include <cassert>

#include <logger/logger.h>
VIDTK_LOGGER("queue_process_txx");


namespace vidtk
{


template <class TData>
queue_process<TData>
::queue_process( std::string const& _name )
  : process( _name, "queue_process" ),
    disable_read_( false ),
    input_datum_( NULL ),
    output_datum_()
{
  config_.add_parameter(
    "disable_read",
    "false",
    "If set to \"true\", it will not place data on the output ports."
    );
  config_.add_parameter(
    "max_length",
    "0",
    "Max length of the queue after which we start dropping frames"
    " that were added first to the queue, i.e. from the reading side."
    " This option is merely used managing memory better."
    " 0 value means OFF (default)."
    );
}


template <class TData>
queue_process<TData>
::~queue_process()
{
}


template <class TData>
config_block
queue_process<TData>
::params() const
{
  return config_;
}


template <class TData>
bool
queue_process<TData>
::set_params( config_block const& blk )
{
  try
  {
    disable_read_ = blk.get<bool>( "disable_read" );
    max_length_ = blk.get<unsigned>( "max_length" );
  }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class TData>
bool
queue_process<TData>
::initialize()
{
  return true;
}


template <class TData>
bool
queue_process<TData>
::is_close_to_full() const
{
  return max_length_ != 0 && queue_.size()+1 == max_length_;
}

// ----------------------------------------------------------------
/** Sync pipeline processing.
 *
 *
 */
template <class TData>
bool
queue_process<TData>
::step()
{
#ifdef DEBUG
  LOG_INFO( this->name() << ": length: " << queue_.size());
#endif

  // If there is new data, push onto back of queue
  if ( input_datum_ )
  {
    queue_.push_back(*input_datum_);

    // If we have exceeded the configured length, then remove the
    // oldest element in the queue.
    if ( ( max_length_ > 0 ) && ( queue_.size() > max_length_ ) )
    {
#ifdef DEBUG
      LOG_DEBUG( this->name() << ": flushing frame, length: " << queue_.size());
#endif
      queue_.erase( queue_.begin() );
    }
  }

  if ( queue_.empty() )
  {
    LOG_WARN( this->name() << ": queue is empty, process failed.");
    return ( false );
  }

  // Put values on the output port if we are enabled.
  if ( !disable_read_ )
  {
    output_datum_ = queue_.front();
    queue_.erase( queue_.begin() );
  }
  else
  {
    // Set the output data for the current iteration to default so
    // that the left-over from last iteration doesn't get
    // used. Process downstream should handle this explicitly.
    output_datum_ = TData();
  }

  // clear input datum so we can detect the presence of new inputs.
  input_datum_ = NULL;

  return true;
}


// ----------------------------------------------------------------
/** Async pipeline processing.
 *
 *
 */
template <class TData>
process::step_status
queue_process<TData>
::step2()
{
  // Leaving the step() function intact as it is being used by
  // VIRAT Ph1 tracker (frameDiffMAASTracker3 in VIBRANT repo)
  if( !step() ) // call basic step to handle data movement.
  {
    return FAILURE;
  }

  if( disable_read_ )
  {
    return SKIP;
  }

  return SUCCESS;
}


// ----------------------------------------------------------------
/** Input method.
 *
 * This method supplies us with a single input datum.
 */
template <class TData>
void
queue_process<TData>
::set_input_datum( TData const& item )
{
  input_datum_ = &item;
}


// ----------------------------------------------------------------
/** Output method.
 *
 * This method returns the current output datum.
 */
template <class TData>
TData
queue_process<TData>
::get_output_datum()
{
  return output_datum_;
}


// ----------------------------------------------------------------
/** Get current queue length.
 *
 * This method returns the number of elements currently in the queue.
 */
template <class TData>
unsigned
queue_process<TData>
::length()
{
  return static_cast<unsigned>(queue_.size());
}


// ----------------------------------------------------------------
/** Clear queue contents.
 *
 * This method removes all of the elements currently in the queue.
 */
template <class TData>
void
queue_process<TData>
::clear()
{
  queue_ = std::vector<TData>();
}


// ----------------------------------------------------------------
/** Return specific datum.
 *
 * This method returns the value from a specific location in the
 * queue.  This implements gandom access into the queue.
 *
 * @param[in] idx - index into queue (0..n)
 */
template <class TData>
TData const &
queue_process<TData>
::datum_at( unsigned idx ) const
{
  return queue_[idx];
}


// ----------------------------------------------------------------
/** Disable/enable output.
 *
 * This method enables of disables the output from this queue process.
 * If the output is disabled, then this process produces no values on
 * its output.
 *
 * @param[in] disable_pop - true disabled output; false enables output
 */
template <class TData>
void
queue_process<TData>
::set_disable_read( bool disable_pop )
{
  disable_read_ = disable_pop;
}


} // end namespace vidtk

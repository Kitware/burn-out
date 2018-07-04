/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_threaded_image_transform_h_
#define vidtk_threaded_image_transform_h_

#include <vil/vil_image_view.h>

#include <utilities/point_view_to_region.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include <tracking_data/image_border.h>

#include <vgl/vgl_intersection.h>

#include <queue>

namespace vidtk
{

/// \brief Helper class which grids up an image and calls some function on
/// each tile in a distributed fashion via threading.
///
/// This class is designed to accept an arbitrary number of arguments via
/// templating both the function that processes the images, and the actual
/// image types themselves. It can optionally handle accepting an image border
/// as input which will ensure no tiles cross the border boundary.
template < typename Arg1 = vil_image_view< vxl_byte >,
           typename Arg2 = vil_image_view< vxl_byte >,
           typename Arg3 = vil_image_view< vxl_byte >,
           typename Arg4 = vil_image_view< vxl_byte >,
           typename Arg5 = vil_image_view< vxl_byte > >
class threaded_image_transform
{
public:

  typedef boost::function5< void, Arg1, Arg2, Arg3, Arg4, Arg5 > function_t;
  typedef threaded_image_transform< Arg1, Arg2, Arg3, Arg4, Arg5 > self_t;

  explicit threaded_image_transform( unsigned tx, unsigned ty )
  : tx_( tx ),
    ty_( ty ),
    process_border_regions_( true ),
    border_pixel_ignore_count_( 0 ),
    inner_bbox_expansion_( 0 )
  {
    thread_count_ = tx * ty;

    thread_worker_queues_= new std::queue< task_t >[ thread_count_ ];
    thread_cond_var_ = new boost::condition_variable[ thread_count_ ];
    thread_muti_ = new boost::mutex[ thread_count_ ];
    thread_status_ = new thread_status[ thread_count_ ];

    // Launch worker threads
    for( unsigned id = 1; id < thread_count_; id++ )
    {
      thread_status_[ id ] = THREAD_WAITING;

      threads_.create_thread(
        boost::bind(
          &self_t::worker_thread,
          this,
          id,
          false
        )
      );
    }
  }

  ~threaded_image_transform()
  {
    if( thread_worker_queues_ != NULL )
    {
      threads_.interrupt_all();
      threads_.join_all();

      delete[] thread_worker_queues_;
      delete[] thread_cond_var_;
      delete[] thread_muti_;
      delete[] thread_status_;
    }
  }

  void set_function( function_t f )
  {
    func_ = f;
  }

  void set_border_mode( bool process_border_regions = true,
                        int border_pixel_ignore_count = 0,
                        int inner_bbox_expansion = 0 )
  {
    process_border_regions_ = process_border_regions;
    border_pixel_ignore_count_ = border_pixel_ignore_count;
    inner_bbox_expansion_ = inner_bbox_expansion;
  }

  void apply_function( Arg1 a1 = Arg1(),
                       Arg2 a2 = Arg2(),
                       Arg3 a3 = Arg3(),
                       Arg4 a4 = Arg4(),
                       Arg5 a5 = Arg5() )
  {
    apply_function( image_border(), a1, a2, a3, a4, a5 );
  }

  void apply_function( const image_border& border,
                       Arg1 a1 = Arg1(),
                       Arg2 a2 = Arg2(),
                       Arg3 a3 = Arg3(),
                       Arg4 a4 = Arg4(),
                       Arg5 a5 = Arg5() )
  {
    // Generate tasks
    std::vector< task_t > tasks;
    std::vector< image_border > required_regions;

    if( border.volume() > 0 )
    {
      image_border adjusted_border(
        border.min_x() + border_pixel_ignore_count_,
        border.max_x() - border_pixel_ignore_count_,
        border.min_y() + border_pixel_ignore_count_,
        border.max_y() - border_pixel_ignore_count_ );

      if( adjusted_border.volume() > 0 )
      {
        required_regions.push_back( adjusted_border );
      }

      if( process_border_regions_ )
      {
        image_border border_reg1(
          0, border.min_x() - border_pixel_ignore_count_,
          0, a1.nj() );

        image_border border_reg2(
          border.max_x() + border_pixel_ignore_count_, a1.ni(),
          0, a1.nj() );

        image_border border_reg3(
          border.min_x() - border_pixel_ignore_count_,
          border.max_x() + border_pixel_ignore_count_,
          0, border.min_y() - border_pixel_ignore_count_ );

        image_border border_reg4(
          border.min_x() - border_pixel_ignore_count_,
          border.max_x() + border_pixel_ignore_count_,
          border.max_y() + border_pixel_ignore_count_, a1.nj() );

        if( border_reg1.volume() > 0 )
        {
          required_regions.push_back( border_reg1 );
        }
        if( border_reg2.volume() > 0 )
        {
          required_regions.push_back( border_reg2 );
        }
        if( border_reg3.volume() > 0 )
        {
          required_regions.push_back( border_reg3 );
        }
        if( border_reg4.volume() > 0 )
        {
          required_regions.push_back( border_reg4 );
        }
      }
    }
    else
    {
      required_regions.push_back( image_border( 0, a1.ni(), 0, a1.nj() ) );
    }

    for( unsigned r = 0; r < required_regions.size(); ++r )
    {
      image_border& region = required_regions[r];

      for( unsigned i = 0; i < tx_; ++i )
      {
        for( unsigned j = 0; j < ty_; ++j )
        {
          int inner_adj_x = ( i != tx_-1 ? inner_bbox_expansion_ : 0 );
          int inner_adj_y = ( j != ty_-1 ? inner_bbox_expansion_ : 0 );

          image_border subregion( region.min_x() + i * region.width() / tx_,
                                  region.min_x() + ( i + 1 ) * region.width() / tx_ + inner_adj_x,
                                  region.min_y() + j * region.height() / ty_,
                                  region.min_y() + ( j + 1 ) * region.height() / ty_ + inner_adj_y );

          if( subregion.volume() > 0 )
          {
            Arg1 a1_reg = ( a1 ? point_view_to_region( a1, subregion ) : a1 );
            Arg2 a2_reg = ( a2 ? point_view_to_region( a2, subregion ) : a2 );
            Arg3 a3_reg = ( a3 ? point_view_to_region( a3, subregion ) : a3 );
            Arg4 a4_reg = ( a4 ? point_view_to_region( a4, subregion ) : a4 );
            Arg5 a5_reg = ( a5 ? point_view_to_region( a5, subregion ) : a5 );

            tasks.push_back( task_t( func_, a1_reg, a2_reg, a3_reg, a4_reg, a5_reg ) );
          }
        }
      }
    }

    // Assign tasks to threads
    for( unsigned i = 0; i < tasks.size(); i++ )
    {
      thread_worker_queues_[ i % thread_count_ ].push( tasks[i] );
    }

    // Start threads
    for( unsigned thread_id = 0; thread_id < thread_count_; thread_id++ )
    {
      if( thread_worker_queues_[ thread_id ].size() != 0 )
      {
        thread_status_[ thread_id ] = THREAD_TASKED;
        thread_cond_var_[ thread_id ].notify_one();
      }
      else
      {
        thread_status_[ thread_id ] = THREAD_FINISHED;
      }
    }

    // Complete all tasks assigned to group 0 on the current thread
    worker_thread( 0, true );

    // Wait for all threads to finish their jobs
    for( unsigned thread_id = 1; thread_id < thread_count_; thread_id++ )
    {
      boost::unique_lock<boost::mutex> lock( this->thread_muti_[ thread_id ] );

      while( thread_status_[ thread_id ] != THREAD_FINISHED )
      {
        thread_cond_var_[ thread_id ].timed_wait( lock, boost::posix_time::milliseconds(1000) );
      }

      thread_status_[ thread_id ] = THREAD_WAITING;
    }
  }

private:

  // Function to call on each thread
  function_t func_;

  // Total thread counts
  unsigned tx_;
  unsigned ty_;

  unsigned thread_count_;

  // Other parameters
  bool process_border_regions_;
  int border_pixel_ignore_count_;
  int inner_bbox_expansion_;

  // Thread status definition
  enum thread_status
  {
    THREAD_WAITING = 0,
    THREAD_TASKED = 1,
    THREAD_RUNNING = 2,
    THREAD_FINISHED = 3,
    THREAD_TERMINATED = 4
  };

  // Thread task definition
  class task_t
  {
  public:

    explicit task_t( function_t& fn,
                     Arg1& a1,
                     Arg2& a2,
                     Arg3& a3,
                     Arg4& a4,
                     Arg5& a5 )
    : fn_( fn ),
      a1_( a1 ),
      a2_( a2 ),
      a3_( a3 ),
      a4_( a4 ),
      a5_( a5 )
    {
    }

    virtual ~task_t()
    {
    }

    virtual void execute()
    {
      fn_( a1_, a2_, a3_, a4_, a5_ );
    }

    function_t fn_;
    Arg1 a1_;
    Arg2 a2_;
    Arg3 a3_;
    Arg4 a4_;
    Arg5 a5_;
  };

  // Each of these will have thread_count instances
  boost::thread_group threads_;
  boost::condition_variable *thread_cond_var_;
  boost::mutex *thread_muti_;
  thread_status *thread_status_;
  std::queue< task_t > *thread_worker_queues_;

  // Worker thread main execution definition
  void worker_thread( unsigned thread_id, bool single_exec )
  {
    try
    {
      // Get pointers to required objects
      boost::mutex *thread_mutex = &thread_muti_[thread_id];
      std::queue< task_t > *thread_queue = &thread_worker_queues_[thread_id];
      boost::condition_variable *thread_cond_var = &thread_cond_var_[thread_id];
      thread_status *current_status = &thread_status_[thread_id];

      // Initiate main loop which will execute until interrupted when we are shutting down
      while( *current_status != THREAD_TERMINATED )
      {
        // Wait until we have new data to process
        {
          boost::unique_lock<boost::mutex> lock( *thread_mutex );
          while( *current_status != THREAD_TASKED )
          {
            thread_cond_var->timed_wait( lock, boost::posix_time::milliseconds(1000) );
          }
        }

        // Update thread status
        *current_status = THREAD_RUNNING;

        while( !thread_queue->empty() )
        {
          // Pop an entry
          task_t to_do = thread_queue->front();
          thread_queue->pop();
          to_do.execute();
        }

        // Signal that processing is complete for this thread
        *current_status = THREAD_FINISHED;
        thread_cond_var->notify_one();

        // If running in single execute mode, exit
        if( single_exec )
        {
          break;
        }
      }
    }
    catch( boost::thread_interrupted const& )
    {
      // Shutdown gracefully
    }
  }
};

}

#endif // vidtk_threaded_image_transform_h_

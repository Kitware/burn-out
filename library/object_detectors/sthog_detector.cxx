/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>

#include <object_detectors/sthog_detector.h>
#include <utilities/kwocv_utility.h>
#include <vul/vul_file.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/highgui/highgui.hpp>

#include <logger/logger.h>

VIDTK_LOGGER( "sthog_detector_cxx" );

namespace vidtk
{

sthog_detector::sthog_detector( std::string model_file,
                                float hit_threshold,
                                std::string output_file,
                                int group_threshold,
                                show_image_t show_image,
                                write_image_t write_image,
                                std::string output_pattern,
                                int hx, int hy,
                                float resize_scale,
                                int tile_xsize,
                                int tile_ysize,
                                int tile_margin )
  : current_file_( "" ),
  current_index_( 0 ),
  output_file_( output_file ),
  output_pattern_( output_pattern ),
  show_image_( show_image ),
  write_image_( write_image ),
  group_threshold_( group_threshold ),
  rhog_( 0 ),
  hog_width_( hx ),
  hog_height_( hy ),
  resize_scale_( resize_scale ),
  tile_xsize_( tile_xsize ),
  tile_ysize_( tile_ysize ),
  tile_margin_( tile_margin )
{
  rhog_ = new sthog_descriptor( 1, cv::Size( hog_width_, hog_height_ ), cv::Size( 16, 16 ),
                                cv::Size( 8, 8 ), cv::Size( 8, 8 ), 9, 1, -1, 0, 0.2, false );

  std::vector< std::string > model_files;
  model_files.push_back( model_file );

  set_models( model_files );
  init();

  while ( hit_threshold_vec_.size() < rhogmodel_vec_.size() )
  {
    hit_threshold_vec_.push_back( hit_threshold );
  }
}


sthog_detector::sthog_detector( std::vector< std::string > model_files,
                                float hit_threshold,
                                std::string output_file,
                                int group_threshold,
                                show_image_t show_image,
                                write_image_t write_image,
                                std::string output_pattern,
                                int hx, int hy,
                                float resize_scale,
                                int tile_xsize,
                                int tile_ysize,
                                int tile_margin )
  : current_file_( "" ),
  current_index_( 0 ),
  output_file_( output_file ),
  output_pattern_( output_pattern ),
  show_image_( show_image ),
  write_image_( write_image ),
  group_threshold_( group_threshold ),
  rhog_( 0 ),
  hog_width_( hx ),
  hog_height_( hy ),
  resize_scale_( resize_scale ),
  tile_xsize_( tile_xsize ),
  tile_ysize_( tile_ysize ),
  tile_margin_( tile_margin )
{
  rhog_ = new sthog_descriptor( 1, cv::Size( hog_width_, hog_height_ ),
                                cv::Size( 16, 16 ), cv::Size( 8, 8 ),
                                cv::Size( 8, 8 ), 9, 1, -1, 0, 0.2,
                                false );

  set_models( model_files );
  init();

  while ( hit_threshold_vec_.size() < rhogmodel_vec_.size() )
  {
    hit_threshold_vec_.push_back( hit_threshold );
  }
}


sthog_detector::sthog_detector( std::vector< std::string > model_files,
                                std::vector< float > hit_thresholds,
                                std::string output_file,
                                int group_threshold,
                                show_image_t show_image,
                                write_image_t write_image,
                                std::string output_pattern,
                                int hx, int hy, float resize_scale,
                                int tile_xsize, int tile_ysize, int tile_margin )
  :
  current_file_( "" ),
  current_index_( 0 ),
  output_file_( output_file ),
  output_pattern_( output_pattern ),
  show_image_( show_image ),
  write_image_( write_image ),
  hit_threshold_vec_( hit_thresholds ),
  group_threshold_( group_threshold ),
  rhog_( 0 ),
  hog_width_( hx ),
  hog_height_( hy ),
  resize_scale_( resize_scale ),
  tile_xsize_( tile_xsize ),
  tile_ysize_( tile_ysize ),
  tile_margin_( tile_margin )
{
  rhog_ = new sthog_descriptor( 1, cv::Size( hog_width_, hog_height_ ),
                                cv::Size( 16, 16 ), cv::Size( 8, 8 ),
                                cv::Size( 8, 8 ), 9, 1, -1, 0, 0.2,
                                false );

  set_models( model_files );
  init();
}


void
sthog_detector::init()
{
  rhogmodel_.clear();

  files_.clear();

  for ( unsigned i = 0; i < rhogmodel_vec_.size(); i++ )
  {
    rhog_->setSVMDetector( rhogmodel_vec_[i] );
    rhog_->svm_linear_push_back();
  }

  ofs_.open( output_file_.c_str(), std::fstream::out );
}


sthog_detector::~sthog_detector()
{
  if ( ofs_.good() )
  {
    ofs_.close();
  }
}


bool
sthog_detector::read_file_list( const std::string& s )
{
  std::fstream ifs;
  std::string fname;

  if ( ! s.empty() )
  {
    input_list_file_ = s;
  }

  // png or jpg image files
  if ( ( input_list_file_.rfind( ".png" ) != std::string::npos )
       || ( input_list_file_.rfind( ".jpg" ) != std::string::npos ) )
  {
    files_.push_back( input_list_file_ );
    return true;
  }

  // image list file
  ifs.open( input_list_file_.c_str(), std::fstream::in );

  while ( ifs.good() )
  {
    fname.clear();

    ifs >> fname;

    if ( fname[0] == '#' )
    {
      continue;
    }

    if ( fname.length() > 0 )
    {
      files_.push_back( fname );
    }
  }

  ifs.close();

  return true;
}


bool
sthog_detector::detect( const std::string& image_fname, int levels, cv::Size stride, bool heatmap )
{
  if ( image_fname.empty() )
  {
    return false;
  }

  files_.clear();
  files_.push_back( image_fname );

  bool rc = detect( levels, stride, heatmap );

  return rc;
}


bool
sthog_detector::detect( std::vector< std::string >& image_fname_vec, int levels,
                        cv::Size stride, bool heatmap )
{
  files_.clear();

  std::vector< std::string >::iterator it;
  for ( it = image_fname_vec.begin(); it != image_fname_vec.end(); ++it )
  {
    files_.push_back( *it );
  }

  bool rc = detect( levels, stride, heatmap );

  return rc;
}


bool
sthog_detector::detect( int levels, cv::Size stride, bool heatmap )
{
  if ( levels < 0 )
  {
    return false;
  }

  if ( show_image_ )
  {
    cv::namedWindow( "object detector", 1 );
  }

  // loop through all images
  for ( unsigned k = 0; k < files_.size(); ++k )
  {

    cv::Mat img = cv::imread( files_[k].c_str() );
    if ( ! img.data )
    {
      LOG_INFO( files_[k] << " ... skipped ... " );
      continue;
    }

    LOG_ASSERT( img.type() == CV_MAKETYPE( cv::DataType< uchar >::depth, 3 ), "image type" );

    current_file_ = files_[k];
    current_index_ = k;

    bool rc = detect( img, levels, stride, heatmap );

    if ( ! rc )
    {
      return false;
    }
  }
  return true;
}


bool
sthog_detector::detect( cv::Mat& img, int levels, cv::Size stride, bool heatmap )
{
  int dx = tile_xsize_ - tile_margin_;
  int dy = tile_ysize_ - tile_margin_;

  std::vector< cv::Rect > found;

  found_filtered_.clear();
  hit_scores_filtered_.clear();
  hit_models_filtered_.clear();

  cv::Mat heat_count;
  if ( heatmap )
  {
    heat_count = cv::Mat::zeros( img.rows, img.cols, CV_32SC1 );

    heat_maps_.clear();
    for ( unsigned i = 0; i < rhogmodel_vec_.size(); ++i )
    {
      cv::Mat im = cv::Mat::zeros( img.rows, img.cols, CV_32FC1 );
      heat_maps_.push_back( im );
    }
  }

  std::vector< cv::Rect > found_all;
  std::vector< double > hit_scores_all;
  std::vector< int > hit_models_all;

  // break into image tiles if necessary
  int xstep = ( dx > 0 ? dx : img.cols );
  int ystep = ( dy > 0 ? dy : img.rows );
  for ( int starty = 0; starty < img.rows; starty = starty + ystep )
  {
    int height = ( dy > 0 ? std::min( ( img.rows - starty ), tile_ysize_ ) : img.rows );
    for ( int startx = 0; startx < img.cols; startx = startx + xstep )
    {
      int width = ( dx > 0 ? std::min( ( img.cols - startx ), tile_xsize_ ) : img.cols );
      cv::Rect tile_rect( startx, starty, width, height );

      cv::Mat tile_im( img, tile_rect );

      // resize image
      cv::Mat tile_resized;
      if ( fabs( resize_scale_ - 1.0 ) < 1e-6 )
      {
        tile_resized = tile_im;
      }
      else
      {
        resize( tile_im, tile_resized, cv::Size( tile_im.cols * resize_scale_, tile_im.rows * resize_scale_ ) );
      }

      std::vector< cv::Mat > imgs;
      imgs.push_back( tile_resized );

      std::vector< double > detection_scores;
      std::vector< int >    hit_models;

      if ( ! heatmap )
      {
        LOG_INFO( "#[" << tile_rect.x << " " << tile_rect.y << " "
                  << tile_rect.width << " " << tile_rect.height << "]" );

        // normal detection
        rhog_->detectMultiScale( imgs, found, hit_threshold_vec_[0], stride, cv::Size( 8, 8 ), 1.1, levels, group_threshold_ );

        detection_scores = rhog_->get_hit_scores();
        LOG_ASSERT( detection_scores.size() == found.size(), "detection score size" );

        hit_models = rhog_->get_hit_models();

        std::vector< cv::Rect >::iterator itr;
        for ( itr = found.begin(); itr != found.end(); ++itr )
        {
          cv::Rect r = *itr;
          if ( fabs( resize_scale_ - 1.0 ) < 1e-6 )
          {
            r.x += startx;
            r.y += starty;
            found_all.push_back( r );
          }
          else
          {
            r.x = startx + r.x / resize_scale_;
            r.y = starty + r.y / resize_scale_;
            r.width /= resize_scale_;
            r.height /= resize_scale_;
            found_all.push_back( r );
          }
        } // end for

        std::vector< double >::iterator itd;
        for ( itd = detection_scores.begin(); itd != detection_scores.end(); ++itd )
        {
          hit_scores_all.push_back( *itd );
        }

        std::vector< int >::iterator iti;
        for ( iti = hit_models.begin(); iti != hit_models.end(); ++iti )
        {
          hit_models_all.push_back( *iti );
        }
      }
      else
      {
        // generate heat map
        std::vector< cv::Point > locations;
        std::vector< cv::Point > foundlocations;
        for ( int ii = 0; ii < img.rows - hog_height_; ii = ii + stride.height )
        {
          for ( int jj = 0; jj < img.cols - hog_width_; jj = jj + stride.width )
          {
            locations.push_back( cv::Point( jj, ii ) );
          }
        }

        rhog_->initialize_detector();
        rhog_->detect( imgs, foundlocations, -1e3, stride, cv::Size( 0, 0 ), locations );

        detection_scores = rhog_->get_hit_scores();

        if ( ! output_file_.empty() )
        {
          // write out heatmap in text format
          const std::string folder = vul_file::dirname( output_file_ );
          const std::string basename = vul_file::basename( current_file_ );

          /// @note Not a totally portable way of constructing file paths
          const std::string filename = folder + "/" + basename + ".dat";

          std::ofstream ofs_heatmap;
          ofs_heatmap.open( filename.c_str(), std::fstream::out );

          for ( unsigned ii = 0; ii < foundlocations.size(); ++ii )
          {
            ofs_heatmap << detection_scores[ii] << " " << foundlocations[ii].x << " " << foundlocations[ii].y << std::endl;
          }
          ofs_heatmap.close();
        }

        // write out heatmap in image stacks
        for ( unsigned ii = 0; ii < foundlocations.size(); ++ii )
        {
          int x = foundlocations[ii].x + startx;
          int y = foundlocations[ii].y + starty;
          unsigned layer = heat_count.at< int > ( y, x );

          if ( ( layer <= rhogmodel_vec_.size() ) &&
               ( x >= 0 ) && ( x < img.cols ) &&
               ( y >= 0 ) && ( y < img.rows ) )
          {
            heat_maps_[layer].at< float > ( y, x ) = detection_scores[ii];
            heat_count.at< int > ( y, x ) += 1;
          }
        }
      }
    } // tile_xsize
  } // tile_ysize

  std::vector< int > flag( found_all.size(), 0 );
  for ( unsigned i = 0; i < found_all.size(); ++i )
  {
    if ( flag[i] > 0 )
    {
      continue;
    }

    cv::Rect r = found_all[i];
    double s = 0;
    unsigned j;
    for ( j = 0; j < found_all.size(); ++j )
    {
      if ( ( j != i ) && ( ( r & found_all[j] ) == r ) )
      {
        // maintain the max score
        flag[j] = 1;
        s = std::max( hit_scores_all[i], hit_scores_all[j] );
        break;
      }
    }

    if ( j == found_all.size() )
    {
      hit_scores_filtered_.push_back( hit_scores_all[i] );
      hit_models_filtered_.push_back( hit_models_all[i] );
    }
    else
    {
      hit_scores_filtered_.push_back( hit_scores_all[s] );
      hit_models_filtered_.push_back( hit_models_all[s] );
    }

    found_filtered_.push_back( r );
  } // end for

  // write out ROI's
  std::stringstream fname;
  if ( write_image_ == WRITE_DETECTION_REGION_ONLY )
  {
    // write out detected regions
    for ( unsigned i = 0; i < found_filtered_.size(); ++i )
    {
      cv::Rect r = found_filtered_[i];

      // pad pixels around margin
      r.x -= static_cast< int > ( 0.5 + r.width  / 6.0 );
      r.y -= static_cast< int > ( 0.5 + r.height / 6.0 );
      r.width  += static_cast< int > ( 0.5 + r.width  / 3.0 );
      r.height += static_cast< int > ( 0.5 + r.height / 3.0 );

      if ( ( r.x < 0 )
           || ( r.y < 0 )
           || ( ( r.x + r.width ) >= img.cols )
           || ( ( r.y + r.height ) >= img.rows ) )
      {
        continue;
      }

      cv::Mat roi = cv::Mat( img, r );
      fname << output_pattern_ << std::setw( 5 ) << std::setfill( '0' ) << current_index_ << ".png";
      LOG_INFO( "Constructed file name: " << fname.str() );
    }
  }

  // write out detection results
  if ( ofs_.good() )
  {
    if ( ! heatmap )
    {
      for ( unsigned i = 0; i < found_filtered_.size(); ++i )
      {
        cv::Rect r = found_filtered_[i];
        rectangle( img, r.tl(), r.br(), cv::Scalar( 0, 0, 255 ), 3 );

        ofs_ << current_file_ << " ";
        ofs_ << hit_scores_filtered_[i] << " ";
        ofs_ << r.x << " " << r.y << " ";
        ofs_ << r.x << " " << r.y + r.height << " ";
        ofs_ << r.x + r.width << " " << r.y + r.height << " ";
        ofs_ << r.x + r.width << " " << r.y << " ";
        ofs_ << hit_models_filtered_[i] << std::endl;
      }
    }
    else
    {
      ofs_ << current_file_ << " ";
      for ( unsigned i = 0; i < hit_scores_filtered_.size(); ++i )
      {
        ofs_ << hit_scores_filtered_[i] << " ";
      }
      ofs_ << std::endl;
    }
  }

  if ( show_image_ )
  {
    imshow( "sthog detector", img );
    int c;
    if ( show_image_ == SHOW_PAUSE )
    {
      LOG_INFO( "Showing detection " << current_file_ << " in pause mode. Press a key to continue. " );
      c = cv::waitKey( 0 ) & 0xFF;
    }
    else
    {
      LOG_INFO( "Showing detection " << current_file_ );
      c = cv::waitKey( 2 ) & 0xFF;
    }
    if ( ( c == 'q' ) || ( c == 'Q' ) )
    {
      return false;
    }
  }

  switch ( write_image_ )
  {
    case WRITE_ALL:
      // write out all images with or without detection
      fname << output_pattern_ << std::setw( 5 ) << std::setfill( '0' ) << current_index_ << ".png";
      LOG_INFO( "Writing results to image " << fname.str() );
      imwrite( fname.str(), img );
      break;

    case WRITE_DETECTION_ONLY:
      // write out images only with detections
      if ( found_filtered_.size() > 0 )
      {
        fname << output_pattern_ << std::setw( 5 ) << std::setfill( '0' ) << current_index_ << ".png";
        LOG_INFO( "Writing detection to image " << fname.str() );
        imwrite( fname.str(), img );
      }
      break;

    default:
      break;
  }

  return true;
} // sthog_detector::detect


bool
sthog_detector::set_models( const std::vector< std::string >& model_files, const int verbose )
{
  bool rc = true;

  for ( unsigned i = 0; i < model_files.size(); ++i )
  {
    if ( set_model( model_files[i], verbose ) )
    {
      rhogmodel_vec_.push_back( rhogmodel_ );
    }
    else
    {
      rc = false;
    }
  }
  return rc;
}


bool
sthog_detector::set_model( const std::string& model_file, const int verbose )
{
  std::ifstream ifs;

  ifs.open( model_file.c_str() );
  if ( ! ifs.is_open() )
  {
    return false;
  }

  int counts = rhog_->get_sthog_descriptor_size();
  float t;
  rhogmodel_.clear();
  std::string line;

  int i = 0;
  while ( i < counts && std::getline( ifs, line ) )
  {
    std::vector< std::string > col;

    if ( line == "" )
    {
      continue;
    }

    // split space delimited tokens
    kwocv::tokenize( line, col );

    if ( col.size() == 1 )
    {
      t = atof( col[0].c_str() );
      rhogmodel_.push_back( t );
    }

  }

  ifs.close();

  if ( rhogmodel_.size() <= 0 )
  {
    return false;
  }

  if ( verbose )
  {
    for ( unsigned index = 0; index < rhogmodel_.size(); ++index )
    {
      LOG_DEBUG( index << " " << rhogmodel_[index] );
    }
  }

  return true;
} // sthog_detector::set_model


}

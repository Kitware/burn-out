/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/kwocv_utility.h>

#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/photo/photo.hpp"

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <math.h>

#include <logger/logger.h>
VIDTK_LOGGER( "kwocv_utility_cxx" );

namespace vidtk {
namespace kwocv {

void
tokenize( const std::string& str, std::vector< std::string >& out_tok,
          const std::string& delimiters )
{
  boost::char_separator< char > sep( delimiters.c_str() );
  boost::tokenizer< boost::char_separator< char > > tokens( str, sep );
  BOOST_FOREACH( std::string const& t, tokens )
  {
    out_tok.push_back( t );
  }
}


bool
calc_image_feat( const cv::Mat& roi, std::vector< float >& attributes )
{
  // (mean, std, visibility, entropy, kurtosis, skewness, px, py //, pxx, pyy, pxy)
  attributes.clear();

  cv::Mat im;
  roi.convertTo( im, CV_32FC1, 1.0 / 255.0 );

  int elements = im.rows * im.cols;
  if ( elements < 16 )
  {
    for ( int i = 0; i < 11; ++i )
    {
      attributes.push_back( 0.0 );
    }
    return false;
  }

  // mean & std
  cv::Scalar sm, ss;
  float mean, std;
  cv::meanStdDev( im, sm, ss );
  mean = sm[0];
  std = ss[0];
  attributes.push_back( mean );
  attributes.push_back( std );

  // contrast/visibility
  double maxVal, minVal;
  minMaxLoc( im, &minVal, &maxVal );
  attributes.push_back( ( maxVal - minVal ) / ( maxVal + minVal + 1e-9 ) );

  // entropy
  float hist[256];
  for ( int i = 0; i < 256; ++i )
  {
    hist[i] = 0.0;
  }

  float delta = 1.0 / elements;
  float m3 = 0.0;
  float m4 = 0.0;

  for ( int i = 0; i < roi.rows; ++i )
  {
    for ( int j = 0; j < roi.cols; ++j )
    {
      int pixel = roi.at< uchar > ( i, j );

      hist[ pixel ] += delta;
    }
  }

  float entropy = 0;
  for ( int i = 0; i < 256; ++i )
  {
    if ( hist[i] > 1e-6 )
    {
      entropy -= hist[i] * std::log( hist[i] ) / std::log( 2.0 );
    }
  }
  attributes.push_back( entropy );

  for ( int i = 0; i < im.rows; ++i )
  {
    for ( int j = 0; j < im.cols; ++j )
    {
      float pixel = im.at< float > ( i, j );
      float diff = pixel - mean;
      float d3 = diff * diff * diff;

      m3 += d3;
      m4 += d3 * diff;
    }
  }

  m3 /= elements;
  m4 /= elements;

  float s3 = std * std * std;

  // kurtosis
  attributes.push_back( m4 / ( s3 * std ) );

  // skewness
  attributes.push_back( m3 / s3 );

  // textureness
  cv::Mat px, py, pxx, pxy, pyy;
  cv::Sobel( roi, px, -1, 1, 0 );
  cv::Sobel( roi, py, -1, 0, 1 );

  attributes.push_back( static_cast< float > ( countNonZero( px ) ) / static_cast< float > ( elements ) );
  attributes.push_back( static_cast< float > ( countNonZero( py ) ) / static_cast< float > ( elements ) );

  return true;
} // calc_image_feat


bool
extract_chip( const cv::Mat& image, cv::Mat& chip, const cv::Point2d& position, double angle, double scale, bool boundary_mirror )
{
  // transform matrix
  cv::Mat affine = getRotationMatrix2D( position, -angle * 180 / 3.14159, scale );

  affine.at< double > ( 0, 2 ) += chip.size().width / 2.0 - position.x;
  affine.at< double > ( 1, 2 ) += chip.size().height / 2.0 - position.y;

  if ( boundary_mirror )
  {
    affine.row( 1 ) *= -1.0;
    affine.at< double > ( 1, 2 ) += chip.size().height;
  }

  cv::warpAffine( image, chip, affine, chip.size(), cv::INTER_LINEAR, cv::BORDER_REFLECT );

  return true;
}


void
vec_mean_std( const std::vector< float >& vec, float& mean, float& std )
{
  mean = 0.0;
  std  = 0.0;

  if ( vec.empty() )
  {
    return;
  }

  for ( unsigned i = 0; i < vec.size(); ++i )
  {
    mean += vec[i];
  }
  mean /= vec.size();

  float val;
  for ( unsigned i = 0; i < vec.size(); ++i )
  {
    val = vec[i] - mean;
    std += val * val;
  }
  std = std::sqrt( std / vec.size() );
}


bool
save_matrix_libsvm_format( const cv::Mat& data, const cv::Mat& label, const std::string& fname )
{
  if ( data.empty() || label.empty() || fname.empty() || ( data.rows != label.rows ) )
  {
    return false;
  }

  std::ofstream ofs;
  ofs.open( fname.c_str() );
  if ( ! ofs.is_open() )
  {
    return false;
  }

  for ( int i = 0; i < data.rows; ++i )
  {
    ofs << label.at< int > ( i, 0 );

    const float* arow = data.ptr< float > ( i );
    for ( int c = 0; c < data.cols; c++ )
    {
      ofs << " " << c + 1 << ":" << arow[c];
    }
    ofs << std::endl;
  }

  ofs.close();
  return true;
}


bool
save_matrix_text_format( const cv::Mat& mat, const std::string& fname )
{
  if ( mat.empty() || fname.empty() )
  {
    return false;
  }

  std::ofstream ofs;
  ofs.open( fname.c_str() );
  if ( ! ofs.is_open() )
  {
    return false;
  }

  for ( int i = 0; i < mat.rows; ++i )
  {
    const float* arow = mat.ptr< float > ( i );
    for ( int c = 0; c < mat.cols - 1; ++c )
    {
      ofs << arow[c] << " ";
    }
    ofs << arow[mat.cols - 1] << std::endl;
  }

  ofs.close();
  return true;
}


bool
read_matrix_libsvm_format( cv::Mat& data, cv::Mat& label, const std::string& fname )
{
  if ( fname.empty() )
  {
    return false;
  }

  std::ifstream ifs( fname.c_str() );
  std::string line;

  std::vector< std::vector< float > > matrix;
  std::vector< int > first_col;

  if ( ifs.is_open() )
  {
    std::vector< std::string > col;
    while ( std::getline( ifs, line ) )
    {
      if ( ( line == "" ) || ( line[0] == '#' ) )
      {
        continue;
      }
      col.clear();
      tokenize( line, col );

      std::vector< float > vec;

      first_col.push_back( atoi( col[0].c_str() ) );

      for ( unsigned i = 1; i < col.size(); ++i )
      {
        std::vector< std::string > fields;
        tokenize( col[i], fields, ":" );
        if ( fields.size() != 2 )
        {
          continue;
        }

        unsigned index = atoi( fields[0].c_str() );
        while ( vec.size() + 1 < index )
        {
          vec.push_back( 0.0 );
        }
        vec.push_back( atof( fields[1].c_str() ) );

      }

      matrix.push_back( vec );
    }
  }

  ifs.close();

  if ( matrix.empty() )
  {
    return false;
  }

  int cols = 0;
  for ( unsigned i = 0; i < matrix.size(); ++i )
  {
    if ( matrix[i].size() > static_cast< unsigned > ( cols ) )
    {
      cols = matrix[i].size();
    }
  }

  data = cv::Mat::zeros( matrix.size(), cols, CV_32FC1 );
  label = cv::Mat::zeros( matrix.size(), 1, CV_32FC1 );

  for ( unsigned i = 0; i < matrix.size(); ++i )
  {
    std::vector< float > vec = matrix[i];
    float* arow = data.ptr< float > ( i );
    for ( int c = 0; c < cols; ++c )
    {
      arow[c] = vec[c];
    }

    label.at< int > ( i, 0 ) = first_col[i];
  }

  return true;
} // read_matrix_libsvm_format


bool
read_matrix_text_format( cv::Mat& mat, const std::string& fname )
{
  if ( fname.empty() )
  {
    return false;
  }

  std::ifstream ifs( fname.c_str() );
  std::string line;

  std::vector< std::vector< float > > matrix;

  if ( ifs.is_open() )
  {
    std::vector< std::string > col;
    while ( std::getline( ifs, line ) )
    {
      if ( ( line == "" ) || ( line[0] == '#' ) )
      {
        continue;
      }
      col.clear();
      tokenize( line, col );

      std::vector< float > vec;
      for ( unsigned i = 0; i < col.size(); ++i )
      {
        vec.push_back( atof( col[i].c_str() ) );
      }

      matrix.push_back( vec );
    }
  }

  ifs.close();

  if (  matrix.empty() )
  {
    return false;
  }

  int cols = matrix[0].size();

  mat = cv::Mat::zeros( matrix.size(), cols, CV_32FC1 );

  for ( unsigned i = 0; i < matrix.size(); ++i )
  {
    std::vector< float > vec = matrix[i];
    if ( vec.size() != static_cast< unsigned > ( cols ) )
    {
      LOG_DEBUG( "row " << i << " has " << vec.size() << " columns " );
      return false;
    }

    float* arow = mat.ptr< float > ( i );
    for ( int c = 0; c < cols; ++c )
    {
      arow[c] = vec[c];
    }
  }

  return true;
} // read_matrix_text_format


bool
concatenate_matrices( const std::vector< cv::Mat >& srcs, cv::Mat& des, int dims )
{
  int rows = 0;
  int cols = 0;

  if ( dims == 0 )
  {
    cols = srcs[0].cols;
  }
  else
  {
    rows = srcs[0].rows;
  }

  for ( unsigned i = 0; i < srcs.size(); ++i )
  {
    if ( &srcs[i] == &des )
    {
      return false;
    }

    if ( dims == 0 )
    {
      rows += srcs[i].rows;
      if ( cols != srcs[i].cols )
      {
        return false;
      }
    }
    else
    {
      cols += srcs[i].cols;
      if ( rows != srcs[i].rows )
      {
        return false;
      }
    }
  }

  des = cv::Mat::zeros( rows, cols, CV_32FC1 );

  if ( dims == 0 )
  {
    // concatenate rows
    int k = 0;
    for ( unsigned i = 0; i < srcs.size(); ++i )
    {
      for ( int j = 0; j < srcs[i].rows; ++j, ++k )
      {
        float* pdes = des.ptr< float > ( k );
        float* psrc = const_cast< float* > ( srcs[i].ptr< float > ( j ) );

        for ( int c = 0; c < cols; ++c )
        {
          pdes[c] = psrc[c];
        }
      }
    }
  }
  else
  {
    // concatenate columns
    for ( int i = 0; i < rows; ++i )
    {
      float* pdes = des.ptr< float > ( i );
      int k = 0;

      for ( unsigned j = 0; j < srcs.size(); ++j )
      {
        float* psrc = const_cast< float* > ( srcs[j].ptr< float > ( i ) );

        for ( int c = 0; c < srcs[j].cols; ++c, ++k )
        {
          pdes[k] = psrc[c];
        }
      }
    }
  }

  return true;
} // concatenate_matrices


bool
concatenate_matrices( const cv::Mat& src_a, const cv::Mat& src_b, cv::Mat& des, int dims )
{
  std::vector< cv::Mat > all;

  all.push_back( src_a );
  all.push_back( src_b );

  return concatenate_matrices( all, des, dims );
}


bool
normalize_matrix( const cv::Mat& src, cv::Mat& des, cv::Mat& mean_std, int dims )
{
  cv::Scalar sm, ss;
  float mean, std;

  des = cv::Mat::zeros( src.rows, src.cols, CV_32FC1 );

  if ( dims == 0 )
  {
    // row normalization

    mean_std = cv::Mat::zeros( 2, src.rows, CV_32FC1 );

    for ( int i = 0; i < src.rows; ++i )
    {
      cv::Mat arow = src.row( i );
      meanStdDev( arow, sm, ss );

      mean = sm[0];
      std  = ss[0] + 1e-9;

      mean_std.at< float > ( 0, i ) = mean;
      mean_std.at< float > ( 1, i ) = std;

      cv::Mat unit = des.row( i );
      arow.convertTo( unit, CV_32FC1, 1.0 / std, -mean / std );
    }
  }
  else
  {
    // column normalization

    mean_std = cv::Mat::zeros( 2, src.cols, CV_32FC1 );

    for ( int i = 0; i < src.cols; ++i )
    {
      cv::Mat acol = src.col( i );
      meanStdDev( acol, sm, ss );

      mean = sm[0];
      std  = ss[0] + 1e-9;

      mean_std.at< float > ( 0, i ) = mean;
      mean_std.at< float > ( 1, i ) = std;

      cv::Mat unit = des.col( i );
      acol.convertTo( unit, CV_32FC1, 1.0 / std, -mean / std );
    }
  }
  return true;
} // normalize_matrix


void
group_rectangle_scores( std::vector< cv::Rect >&  rectList,
                        std::vector< double >&    scores,
                        std::vector< int >&       model_index,
                        int                       group_threshold,
                        double                    eps,
                        std::vector< int >*       weights )
{
  bool non_empty_index = ( ! model_index.empty() );

  if ( ( group_threshold <= 0 ) || rectList.empty() )
  {
    if ( weights )
    {
      size_t i, sz = rectList.size();
      weights->resize( sz );
      for ( i = 0; i < sz; ++i )
      {
        ( *weights )[i] = 1;
      }
    }
    return;
  }

  std::vector< int > labels;
  int nclasses = partition( rectList, labels, SimilarRects( eps ) );

  std::vector< cv::Rect > rrects( nclasses );
  std::vector< int > rweights( nclasses, 0 );
  std::vector< double > rscore( nclasses, 0 );
  std::vector< int > mindex( nclasses, 0 );

  int i, j;
  int nlabels = static_cast< int > ( labels.size() );
  for ( i = 0; i < nlabels; ++i )
  {
    int cls = labels[i];
    rrects[cls].x += rectList[i].x;
    rrects[cls].y += rectList[i].y;
    rrects[cls].width += rectList[i].width;
    rrects[cls].height += rectList[i].height;
    rweights[cls]++;

    rscore[cls] += scores[i];
//  rscore[cls] = max( rscore[cls], scores[i] );

    if ( non_empty_index )
    {
      mindex[cls] = model_index[cls];
    }
  }

  for ( i = 0; i < nclasses; ++i )
  {
    cv::Rect r = rrects[i];
    float s = 1.f / rweights[i];
    rrects[i] = cv::Rect( cv::saturate_cast< int > ( r.x * s ),
                          cv::saturate_cast< int > ( r.y * s ),
                          cv::saturate_cast< int > ( r.width * s ),
                          cv::saturate_cast< int > ( r.height * s ) );

    rscore[i] *= s;
  }

  rectList.clear();
  scores.clear();
  model_index.clear();

  if ( weights )
  {
    weights->clear();
  }

  for ( i = 0; i < nclasses; ++i )
  {
    cv::Rect r1 = rrects[i];
    int n1 = rweights[i];
    if ( n1 <= group_threshold )
    {
      continue;
    }
    // filter out small rectangles inside large rectangles
    for ( j = 0; j < nclasses; ++j )
    {
      int n2 = rweights[j];

      if ( ( j == i ) || ( n2 <= group_threshold ) )
      {
        continue;
      }
      cv::Rect r2 = rrects[j];

      int dx = cv::saturate_cast< int > ( r2.width * eps );
      int dy = cv::saturate_cast< int > ( r2.height * eps );

      if ( ( i != j ) &&
           ( r1.x >= r2.x - dx ) &&
           ( r1.y >= r2.y - dy ) &&
           ( r1.x + r1.width <= r2.x + r2.width + dx ) &&
           ( r1.y + r1.height <= r2.y + r2.height + dy ) &&
           ( ( n2 > std::max( 3, n1 ) ) || ( n1 < 3 ) ) )
      {
        break;
      }
    }

    if ( j == nclasses )
    {
      rectList.push_back( r1 );
      if ( weights )
      {
        weights->push_back( n1 );
      }
      scores.push_back( rscore[i] );
      if ( non_empty_index )
      {
        model_index.push_back( mindex[i] );
      }
    }
  }
} // group_rectangle_scores


std::vector< cv::Rect > read_rect_from_kw18( const std::string& fname, int frame_number )
{
  std::vector< cv::Rect > rts;

  if ( fname.empty() )
  {
    return rts;
  }

  std::ifstream ifs( fname.c_str() );
  std::string line;

  if ( ifs.is_open() )
  {
    std::vector< std::string > col;
    while ( std::getline( ifs, line ) )
    {
      if ( ( line == "" ) || ( line[0] == '#' ) )
      {
        continue;
      }
      col.clear();
      tokenize( line, col );

      if ( ( frame_number > 0 ) && ( atoi( col[2].c_str() ) != frame_number ) )
      {
        continue;
      }

      cv::Rect rt( atoi( col[9].c_str() ),  atoi( col[10].c_str() ),  atoi( col[11].c_str() ),  atoi( col[12].c_str() ) );

      LOG_DEBUG( "Resulting rectangle: " << rt.x << " " << rt.y << " " << rt.width << " " << rt.height );

      rts.push_back( rt );
    }
  }

  ifs.close();
  LOG_DEBUG( "Read in " << rts.size() << "rectangles" );

  return rts;
} // read_rect_from_kw18


std::map< int, cv::Rect >
rect_map_from_kw18( const std::string& fname, int frame_number )
{
  std::map< int, cv::Rect > rts;

  if ( fname.empty() )
  {
    return rts;
  }

  std::ifstream ifs( fname.c_str() );
  std::string line;

  if ( ifs.is_open() )
  {
    std::vector< std::string > col;
    while ( std::getline( ifs, line ) )
    {
      if ( ( line == "" ) || ( line[0] == '#' ) )
      {
        continue;
      }
      col.clear();
      tokenize( line, col );

      if ( ( frame_number > 0 ) && ( atoi( col[2].c_str() ) != frame_number ) )
      {
        continue;
      }

      cv::Rect rt( cv::Point( atoi( col[9].c_str() ),  atoi( col[10].c_str() ) ),  cv::Point( atoi( col[11].c_str() ),  atoi( col[12].c_str() ) ) );
      int id = atoi( col[0].c_str() );

      LOG_DEBUG( "Track: " << id << " " << rt.x << " " << rt.y << " " << rt.width << " " << rt.height );

      rts[id] = rt;
    }
  }
  ifs.close();
  LOG_DEBUG( "Read in " << rts.size() << "rectangles" );

  return rts;
} // rect_map_from_kw18


std::vector< cv::Rect >
read_rect_from_box( const std::string& fname, const std::string& imgname )
{
  std::vector< cv::Rect > rts;

  if ( fname.empty() )
  {
    return rts;
  }

  std::ifstream ifs( fname.c_str() );
  std::string line;

  if ( ifs.is_open() )
  {
    std::vector< std::string > col;
    while ( std::getline( ifs, line ) )
    {
      if ( ( line == "" ) || ( line[0] == '#' ) )
      {
        continue;
      }
      col.clear();
      tokenize( line, col );

      if ( ( col.size() != 10 ) && ( ! imgname.empty() && imgname.compare( col[2] ) ) )
      {
        continue;
      }

      cv::Rect rt( cv::Point( atoi( col[2].c_str() ),  atoi( col[3].c_str() ) ),  cv::Point( atoi( col[6].c_str() ),  atoi( col[7].c_str() ) ) );

      LOG_DEBUG( "Rectangle: " << rt.x << " " << rt.y << " " << rt.width << " " << rt.height );

      rts.push_back( rt );
    }
  }

  ifs.close();
  LOG_DEBUG( "Read in " << rts.size() << "rectangles" );

  return rts;
} // read_rect_from_box


bool
plot_detection( const std::string& match_file, cv::Mat im, std::map< int, cv::Rect > gt,  std::map< int, cv::Rect > dt )
{
  std::ifstream ifs( match_file.c_str() );
  std::string line;

  cv::Scalar red = cv::Scalar( 0, 0, 255 );
  cv::Scalar green = cv::Scalar( 0, 255, 0 );
  cv::Scalar yellow = cv::Scalar( 0, 255, 255 );

  if ( ifs.is_open() )
  {
    std::vector< std::string > col;
    while ( std::getline( ifs, line ) )
    {
      if ( ( line == "" ) || ( line[0] == '#' ) )
      {
        continue;
      }
      col.clear();
      tokenize( line, col );

      if ( ! col[0].compare( "gt" ) && ( atoi( col[3].c_str() ) <= 0 ) )
      {
        // miss detection
        cv::Rect rt = gt[ atoi( col[1].c_str() ) ];
        LOG_DEBUG( "miss " << rt.x << " " << rt.y << " " << rt.width << " " << rt.height );
        rectangle( im, rt.tl(), rt.br(), yellow, 5 );
      }

      if ( ! col[0].compare( "ct" ) )
      {
        if ( atoi( col[3].c_str() ) <= 0 )
        {
          // false detection
          cv::Rect rt = dt[ atoi( col[1].c_str() ) ];
          LOG_DEBUG( "false " << rt.x << " " << rt.y << " " << rt.width << " " << rt.height );
          rectangle( im, rt.tl(), rt.br(), red, 5 );
        }
        else
        {
          // true detection
          cv::Rect rt = dt[ atoi( col[1].c_str() ) ];
          LOG_DEBUG( "true " << rt.x << " " << rt.y << " " << rt.width << " " << rt.height );
          rectangle( im, rt.tl(), rt.br(), green, 5 );
        }
      }
    }
  }

  ifs.close();

  return true;
} // plot_detection


bool
plot_detection( cv::Mat im, std::map< int, cv::Rect > dt )
{
  cv::Scalar red = cv::Scalar( 0, 0, 255 );
  std::map< int, cv::Rect >::iterator it;

  for ( it = dt.begin(); it != dt.end(); ++it )
  {
    cv::Rect rt = ( *it ).second;
    rectangle( im, rt.tl(), rt.br(), red, 5 );
  }

  return true;
}


bool
plot_detection( cv::Mat im, std::vector< cv::Rect > rts )
{
  cv::Scalar red = cv::Scalar( 0, 0, 255 );
  std::vector< cv::Rect >::iterator it;

  for ( it = rts.begin(); it != rts.end(); ++it )
  {
    rectangle( im, it->tl(), it->br(), red, 5 );
  }

  return true;
}

} // end namesapce kwocv
} // namespace vidtk

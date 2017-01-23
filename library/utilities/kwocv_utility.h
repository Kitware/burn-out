/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kwocv_utility_h_
#define vidtk_kwocv_utility_h_

#include <vector>
#include <string>
#include <map>

#include <opencv2/core/core.hpp>

namespace vidtk {
namespace kwocv {


enum MAT_FORMATS
{
  MAT_OCV    = 0,
  MAT_TEXT   = 1,
  MAT_LIBSVM = 2
};


// ----------------------------------------------------------------
/**
 * @brief Functor for grouping object candidates.
 *
 * When the upper-left and bottom right corners of two rects are close
 * to each other by delta pixels, they are considered as the similar
 * rectangles, where delta is determined by a percentage of rectangle
 * dimensions.
 */
class SimilarRects
{
public:

  /**
   * @brief Constructor
   *
   * @param _eps Epsilon value
   */
  SimilarRects( double _eps ) : eps( _eps ) { }

  inline bool operator()( const cv::Rect& r1, const cv::Rect& r2 ) const
  {
    double delta = eps * ( std::min( r1.width, r2.width ) + std::min( r1.height, r2.height ) ) * 0.5;

    return std::abs( r1.x - r2.x ) <= delta &&
           std::abs( r1.y - r2.y ) <= delta &&
           std::abs( r1.x + r1.width - r2.x - r2.width ) <= delta &&
           std::abs( r1.y + r1.height - r2.y - r2.height ) <= delta;
  }

  double eps;
};


// ==================================================================
// (mean, std, visibility, entropy, kurtosis, skewness, px, py, pxx, pyy, pxy)
bool calc_image_feat( const cv::Mat& im, std::vector< float >& attributes );

bool extract_chip( const cv::Mat& image, cv::Mat& chip, const cv::Point2d& position,
                   double angle, double scale, bool boundary_mirror = false );

/**
 * @brief Break string into tokens.
 *
 * @param[in] str String to break into tokens
 * @param[out] tokens List of tokens
 * @param[in] delimiters List of delimiters for tokens
 */
void tokenize( const std::string& str, std::vector< std::string >& tokens, const std::string& delimiters = " " );

void vec_mean_std( const std::vector< float >& vec, float& mean, float& std );

bool save_matrix_libsvm_format( const cv::Mat& data, const cv::Mat& label, const std::string& fname );
bool save_matrix_text_format( const cv::Mat& mat, const std::string& fname );

bool read_matrix_libsvm_format( cv::Mat& data, cv::Mat& label, const std::string& fname );
bool read_matrix_text_format( cv::Mat& mat, const std::string& fname );

bool concatenate_matrices( const std::vector< cv::Mat >& srcs, cv::Mat& des, int dims = 0 );  // row:0 column:1
bool concatenate_matrices( const cv::Mat& src_a, const cv::Mat& src_b, cv::Mat& des, int dims = 0 ); // row: 0 column:1

bool normalize_matrix( const cv::Mat& src, cv::Mat& des, cv::Mat& mean_std, int dims = 1 ); // row: 0  column: 1

void group_rectangle_scores( std::vector< cv::Rect >& rectList,
                             std::vector< double >&   scores,
                             std::vector< int >&      index,
                             int                      group_threshold,
                             double                   eps,
                             std::vector< int >*      weights );

std::vector< cv::Rect > read_rect_from_kw18( const std::string& fname, int frame_number = 0 );

std::vector< cv::Rect > read_rect_from_box( const std::string& fname_description,
                                            const std::string& fname_image = "" );

std::map< int, cv::Rect > rect_map_from_kw18( const std::string& fname, int frame_number = 0  );

bool plot_detection( const std::string& match_file, cv::Mat im, std::map< int, cv::Rect > gt, std::map< int, cv::Rect > dt );

bool plot_detection( cv::Mat im, std::map< int, cv::Rect > dt );
bool plot_detection( cv::Mat im, std::vector< cv::Rect > rts );

} // end namespace kwocv
} // namespace vidtk

#endif  // vidtk_kwocv_utility_h_

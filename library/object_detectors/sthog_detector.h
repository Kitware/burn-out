/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sthog_detector_h_
#define vidtk_sthog_detector_h_

#include <descriptors/sthog_descriptor.h>

#include <string>
#include <vector>
#include <fstream>

namespace vidtk
{

// Object detection using histogram of oriented gradients
class sthog_detector
{
public:

  // Options for showing images during processing
  enum show_image_t
  {
    SHOW_NONE,
    SHOW_PAUSE,
    SHOW_CONTINUOUS,

    SHOW_INVALID
  };

  // Options for writing detection images
  enum write_image_t
  {
    WRITE_NONE,
    WRITE_ALL,
    WRITE_DETECTION_ONLY,
    WRITE_DETECTION_REGION_ONLY,

    WRITE_INVALID
  };

  sthog_detector( std::string model_file,
                  float hit_threshold = 1.0,
                  std::string output_file = "output.txt",
                  int group_threshold = 2,
                  show_image_t show_image = SHOW_NONE,
                  write_image_t write_image = WRITE_NONE,
                  std::string output_pattern = "img",
                  int hx = 64,
                  int hy = 64,
                  float resize_scale=1.0,
                  int tile_xsize=0,
                  int tile_ysize=0,
                  int tile_margin=0);

  sthog_detector( std::vector<std::string> model_files,
                  float hit_threshold,
                  std::string output_file = "output.txt",
                  int group_threshold = 2,
                  show_image_t show_image = SHOW_NONE,
                  write_image_t write_image = WRITE_NONE,
                  std::string output_pattern = "img",
                  int hx = 64,
                  int hy = 64,
                  float resize_scale=1.0,
                  int tile_xsize=0,
                  int tile_ysize=0,
                  int tile_margin=0);

  sthog_detector( std::vector<std::string> model_files,
                  std::vector<float> hit_thresholds,
                  std::string output_file = "output.txt",
                  int group_threshold = 2,
                  show_image_t show_image = SHOW_NONE,
                  write_image_t write_image = WRITE_NONE,
                  std::string output_pattern = "img",
                  int hx = 64,
                  int hy = 64,
                  float resize_scale=1.0,
                  int tile_xsize=0,
                  int tile_ysize=0,
                  int tile_margin=0);

  virtual ~sthog_detector();

  virtual bool detect(int levels=1, cv::Size stride=cv::Size(8,8), bool heatmap=false);
  virtual bool detect(cv::Mat& im, int levels=1, cv::Size stride=cv::Size(8,8), bool heatmap=false);
  virtual bool detect(const std::string& image_fname, int levels=1, cv::Size stride=cv::Size(8,8), bool heatmap=false);
  virtual bool detect(std::vector<std::string>& image_fname_vec, int levels=1, cv::Size stride=cv::Size(8,8), bool heatmap=false);

  void set_hog_config(int hx = 64, int hy = 64) {hog_width_ = hx; hog_height_ = hy;}

  std::vector<cv::Rect>& get_found_filtered() { return found_filtered_; }
  std::vector<double>& get_hit_scores_filtered() { return hit_scores_filtered_; }
  std::vector<cv::Mat>& get_heat_maps() { return heat_maps_; }

protected:
  void init();
  virtual bool set_models( const std::vector<std::string> & model_file, const int verbose = 0 );
  virtual bool set_model( const std::string & model_file, const int verbose = 0 );

  bool read_file_list(const std::string& s);
  void display_image();

  std::string input_list_file_;
  std::string current_file_;
  int current_index_;
  std::string model_file_;
  std::string output_file_;
  std::string output_pattern_;
  show_image_t show_image_;
  write_image_t write_image_;
  std::vector<float> hit_threshold_vec_;
  int   group_threshold_;

  std::vector<std::string> files_;
  std::ofstream ofs_;

  cv::Ptr<sthog_descriptor> rhog_;

  int hog_width_;
  int hog_height_;

  std::vector<float> rhogmodel_;
  std::vector< std::vector<float> > rhogmodel_vec_;

  // resize image to a fixed scale
  float resize_scale_;

  // break image into tiles to deal with large images
  int tile_xsize_;
  int tile_ysize_;
  int tile_margin_;

  std::vector<cv::Rect> found_filtered_;
  std::vector<double> hit_scores_filtered_;
  std::vector<int> hit_models_filtered_;

  std::vector<cv::Mat> heat_maps_;

};

}

#endif // vidtk_sthog_detector_h_

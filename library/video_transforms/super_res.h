/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef super_res_h_
#define super_res_h_

#include <vil/vil_image_view.h>
#include <vector>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#include "adjoint_image_op.h"

namespace vidtk
{

struct super_res_params
{
  double lambda;                        //Coefficient on the regularization
  double epsilon_data, epsilon_reg;     //cutoff between 1-norm and 2-norm
  double tau, sigma;                    //gradient ascent / descent parameters
  unsigned int s_ni, s_nj, l_ni, l_nj;  //sizes of super resolved and original sized reference img
  double scale_factor;                  //amount resulting image is super resolved
};

class super_resolution_monitor;

/// Computes a super resolved image from a set of images and warps
/// \param frames is the image sequence to compute super res from
/// \param warps specify the warping of each pixel in each frame to the super resolved image
/// \param super_img is the resulting super resolved image (can be initialized)
/// \param max_iterations is the number of iterations to quit after
/// \param srp are the super resolution parameters
/// \param srm is the optional super resolution monitor for writing out intermediate results
void super_resolve(const std::vector<vil_image_view<double> > &frames,
                   const std::vector<adjoint_image_ops_func<double> > &warps,
                   vil_image_view<double> &super_img,
                   unsigned int max_iterations,
                   const super_res_params &srp,
                   super_resolution_monitor *srm = NULL);


/// Monitors the progress of the super resolution algorithm by providing two ways
/// to access intermediate results.  It can be provided a call back function that
/// will be called every specified number of iterations (interval) with the current
/// super resolution result and iteration number.  Intermediate results can also be
/// asynchronously accessed by the get_update function whenever desired.
class super_resolution_monitor
{
public:

  struct update_data
  {
    vil_image_view<double> current_result;
    unsigned int num_iterations;
  };

  super_resolution_monitor(boost::function<void (update_data)> callback,
                           unsigned int interval,
                           boost::shared_ptr<bool> interrupted) : callback_(callback),
                                                                  interval_(interval),
                                                                  interrupted_(interrupted),
                                                                  current_result_(NULL) {}

  /// Get deep copy of the current image and iteration
  void get_update(update_data &update);

private:

    /// Set the current image and iteration
  void set_monitored_data(vil_image_view<double> *super_img,
                          const boost::shared_ptr<unsigned int> &iter);

  //Only super_resolve can set the monitored data
  friend void super_resolve(const std::vector<vil_image_view<double> > &frames,
                            const std::vector<adjoint_image_ops_func<double> > &warps,
                            vil_image_view<double> &super_img,
                            unsigned int max_iterations,
                            const super_res_params &srp,
                            super_resolution_monitor *srm);

  boost::function<void (update_data)> callback_;
  unsigned int interval_;
  boost::shared_ptr<bool const> interrupted_;

  boost::mutex m_data_;

  //Cannot rely on internal pointers because data is swapped in u's update
  vil_image_view<double> *current_result_;
  boost::shared_ptr<unsigned int> num_iterations_;
};


} // end namespace vidtk

#endif //super_res_h_

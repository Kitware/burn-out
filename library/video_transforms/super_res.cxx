/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "super_res.h"
#include "adjoint_image_derivs.h"

#include <vil/vil_math.h>

namespace vidtk
{

//Super resolution based on the paper:
//Markus Unger, Thomas Pock, Manuel Werlberger, and Horst Bischof.
//A Convex Approach for Variational Super-Resolution.
//DAGM-Symposium, volume 6376 of Lecture Notes in Computer Science, page 313-322. Springer, (2010)

/// Gradient ascent and projection on the dual variable p
void dual_step_p(const vil_image_view<double> &u_bar,
                 vil_image_view<double> &p,
                 const super_res_params &srp)
{
  const double denom = 1.0 + (srp.sigma * srp.epsilon_reg) / srp.lambda;
  vil_image_view<double> work;
  forward_gradient(u_bar, work);
  vil_math_add_image_fraction(p, 1.0/denom, work, srp.sigma/denom);

  for (unsigned int j = 0; j < srp.s_nj; j++)
  {
    for (unsigned int i = 0; i < srp.s_ni; i++)
    {
      for (unsigned int k = 0; k < u_bar.nplanes(); k++)
      {
        double &x = p(i,j,2*k), &y = p(i,j,2*k+1);

        //truncate vectors
        const double mag = sqrt(x*x + y*y)/srp.lambda;
        if (mag > 1.0)
        {
          x /= mag;
          y /= mag;
        }
      }
    }
  }
}


/// Gradient ascent and projection on the dual variables q
void dual_step_q(const std::vector<vil_image_view<double> > &frames,
                 const std::vector<vidtk::adjoint_image_ops_func<double> > &warps,
                 const std::vector<vil_image_view<double> > &weights,
                 const vil_image_view<double> &u_bar,
                 std::vector<vil_image_view<double> > &q,
                 const super_res_params &srp)
{
  vil_image_view<double> l_u, temp;
  const double sf_2 = 1.0/(srp.scale_factor * srp.scale_factor);
  const double denom = 1.0 + (srp.sigma * srp.epsilon_data) / sf_2;

  for (unsigned int f = 0; f < frames.size(); f++)
  {
    const unsigned int ni = weights[f].ni();
    const unsigned int nj = weights[f].nj();
    const unsigned int np = q[f].nplanes();

    // apply the linear operator to warp, blur, and downsample
    warps[f].apply_A(u_bar, l_u);

    for (unsigned int j = 0; j < nj; j++)
    {
      for (unsigned int i = 0; i < ni; i++)
      {
        for (unsigned int k = 0; k < np; k++)
        {
          double &qfijk = q[f](i, j, k);
          qfijk = (qfijk + srp.sigma * sf_2 * (l_u(i, j, k) - frames[f](i, j, k) * weights[f](i,j)))/denom;
          qfijk = std::max(qfijk, -sf_2);
          qfijk = std::min(qfijk, sf_2);
        }
      }
    }
  }
}


/// Perform a gradient descent step on the primal variable u
void primal_step_u(const std::vector<vil_image_view<double> > &q,
                   const std::vector<adjoint_image_ops_func<double> > &warps,
                   const vil_image_view<double> &p,
                   vil_image_view<double> &u,
                   vil_image_view<double> &u_bar,
                   const super_res_params &srp)
{
  const double sf_2 = 1.0/(srp.scale_factor * srp.scale_factor);
  vil_image_view<double> sum_super_q(srp.s_ni, srp.s_nj, u.nplanes());
  sum_super_q.fill(0.0);
  vil_image_view<double> super_q(srp.s_ni, srp.s_nj, u.nplanes()), temp;
  for (unsigned int i = 0; i < q.size(); i++)
  {
    // apply transpose linear operator to upsample, blur, and warp
    warps[i].apply_At(q[i], super_q);
    vil_math_image_sum(sum_super_q, super_q, sum_super_q);
  }

  vil_image_view<double> work;
  backward_divergence(p, work);
  vil_math_add_image_fraction(work, srp.tau, sum_super_q, -srp.tau * sf_2);
  vil_math_image_sum(u, work, work);
  u_bar.deep_copy(work);
  vil_math_add_image_fraction(u_bar, 2.0, u, -1.0);
  std::swap(u, work);
}


/// Computes a super resolved image from a set of images and warps
/// \param frames is are the image sequence to comput super res from
/// \param warps specify the warping of each pixel in each frame to the super resolved image
/// \param u is the super resolved image
/// \param srp are parameters for super resolution
/// \iterations is the number of iterations to quit after
void
super_resolve(const std::vector<vil_image_view<double> > &frames,
              const std::vector<adjoint_image_ops_func<double> > &warps,
              vil_image_view<double> &u,
              unsigned int max_iterations,
              const super_res_params &srp,
              super_resolution_monitor *srm)
{
  assert(frames.size() == warps.size());

  if (frames.empty())
  {
    std::cerr << "No frames were supplied.\n";
    return;
  }

  const unsigned int np = frames[0].nplanes();
  for (unsigned int i = 1; i < frames.size(); i++)
  {
    if (frames[i].nplanes() != np)
    {
      std::cerr << "All frames must have the same number of planes.\n";
      return;
    }
  }

  //If the size was correct assume that it was pre-initialized
  if (u.ni() != srp.s_ni || u.nj() != srp.s_nj || u.nplanes() != np)
  {
    u.set_size(srp.s_ni, srp.s_nj, np);
    u.fill(0.0);
  }

  boost::shared_ptr<unsigned int> itr(new unsigned int);
  *itr = 1;

  if (srm)
  {
    srm->set_monitored_data(&u, itr);
  }

  //Dual variable for u
  vil_image_view<double> p(srp.s_ni, srp.s_nj, 2*np);
  p.fill(0.0);

  //Dual variables for the second dualization
  std::vector<vil_image_view<double> > q(frames.size());
  std::vector<vil_image_view<double> > weights;
  for (unsigned int i = 0; i < frames.size(); i++)
  {
    q[i].set_size(warps[i].dst_ni(), warps[i].dst_nj(), np);
    q[i].fill(0.0);
    weights.push_back(warps[i].weight_map());
  }

  vil_image_view<double> u_bar;
  u_bar.deep_copy(u);

  while (*itr <= max_iterations)
  {
    dual_step_p(u, p, srp);
    dual_step_q(frames, warps, weights, u, q, srp);

    if (srm)
    {
      if (*srm->interrupted_)
      {
        return;
      }

      if (srm->callback_ && !(*itr % srm->interval_))
      {
        super_resolution_monitor::update_data data;
        srm->get_update(data);
        srm->callback_(data);
      }

      boost::lock_guard<boost::mutex> lock(srm->m_data_);
      primal_step_u(q, warps, p, u, u_bar, srp);
      (*itr)++;
    }
    else
    {
      primal_step_u(q, warps, p, u, u_bar, srp);
      (*itr)++;
    }
  }
}

void super_resolution_monitor::set_monitored_data(vil_image_view<double> *super_img,
                                                  const boost::shared_ptr<unsigned int> &iter)
{
  boost::lock_guard<boost::mutex> lock(m_data_);
  current_result_ = super_img;
  num_iterations_ = iter;
}

void super_resolution_monitor::get_update(update_data &update)
{
  boost::lock_guard<boost::mutex> lock(m_data_);
  update.current_result.deep_copy(*current_result_);
  update.num_iterations = *num_iterations_;
}


} // end namespace vidtk

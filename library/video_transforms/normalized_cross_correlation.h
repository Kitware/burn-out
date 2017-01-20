/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_VIDEO_NORMALIZED_CROSS_CORRELATION_H
#define VIDTK_VIDEO_NORMALIZED_CROSS_CORRELATION_H

#include "vil_for_each.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

namespace vidtk
{

// vil_algo "implements" this algorithm, but not properly.

/*
 * \frac{1}{n} \sum_{x,y} (\frac{(f(x,y)-\bar{f})(t(x,y)-\bar{t})}{\sigma_f\sigma_t})
 */

namespace detail
{

typedef double stat_t;

typedef boost::accumulators::accumulator_set<stat_t, boost::accumulators::stats
  < boost::accumulators::tag::mean
  , boost::accumulators::tag::variance(boost::accumulators::immediate)
  > > nxcorr_stats_t;

template <typename SrcT, typename KernelT, typename AccumT>
struct correlation_functor
{
  correlation_functor(stat_t src_mean_, stat_t src_stdev_,
                      stat_t kernel_mean_, stat_t kernel_stdev_)
    : src_mean(src_mean_)
    , kernel_mean(kernel_mean_)
    , stdev(kernel_stdev_ * src_stdev_)
  {
  }

  void
  operator () (SrcT s, KernelT k)
  {
    stats((s - src_mean) * (k - kernel_mean));
  }

  AccumT
  sum() const
  {
    return boost::accumulators::sum(stats) / stdev;
  }

  typedef boost::accumulators::accumulator_set<AccumT, boost::accumulators::stats
    < boost::accumulators::tag::sum
    > > corr_stats_t;

  stat_t const src_mean;
  stat_t const kernel_mean;
  stat_t const stdev;
  corr_stats_t stats;
};

template <typename SrcT, typename KernelT, typename AccumT>
inline
AccumT
normalized_cross_correlation_px(vil_image_view<SrcT> const& src,
                                vil_image_view<KernelT> const& kernel,
                                stat_t kernel_stdev,
                                stat_t kernel_mean)
{
  nxcorr_stats_t src_stats;

  src_stats = vil_for_each(src, src_stats);

  stat_t const src_stdev = std::sqrt(boost::accumulators::variance(src_stats));
  stat_t const src_mean = boost::accumulators::mean(src_stats);

  typedef correlation_functor<SrcT, KernelT, AccumT> correlation_functor_t;

  correlation_functor_t corr_func(src_mean, src_stdev, kernel_mean, kernel_stdev);

  correlation_functor_t const result(vil_for_each_2(src, kernel, corr_func));

  return result.sum();
}

}

template <typename SrcT, typename DestT, typename KernelT, typename AccumT>
inline
void
normalized_cross_correlation(vil_image_view<SrcT> const& src,
                             vil_image_view<DestT>& dest,
                             vil_image_view<KernelT> const& kernel,
                             AccumT const&)
{
  size_t const sni = src.ni();
  size_t const snj = src.nj();

  size_t const kni = kernel.ni();
  size_t const knj = kernel.nj();
  size_t const knp = kernel.nplanes();

  if ((sni < kni) ||
      (snj < knj))
  {
    // The caller must handle that dest is not modified.
    return;
  }

  size_t const dni = 1 + sni - kni;
  size_t const dnj = 1 + snj - knj;

  assert(src.nplanes() == knp);
  assert(dni > 0);
  assert(dnj > 0);

  detail::nxcorr_stats_t kernel_stats;

  kernel_stats = vil_for_each(kernel, kernel_stats);

  detail::stat_t const kernel_stdev = std::sqrt(boost::accumulators::variance(kernel_stats));
  detail::stat_t const kernel_mean = boost::accumulators::mean(kernel_stats);

  dest.set_size(dni, dnj, 1);

  ptrdiff_t const d_i = dest.istep();
  ptrdiff_t const d_j = dest.jstep();

  ptrdiff_t const s_i = src.istep();
  ptrdiff_t const s_j = src.jstep();
  ptrdiff_t const s_p = src.planestep();

  size_t const ksz = kernel.size();

  SrcT const* sr = src.top_left_ptr();
  DestT* dr = dest.top_left_ptr();

  for (size_t j = 0; j < dnj; ++j, sr += s_j, dr += d_j)
  {
    SrcT const* sp = sr;
    DestT* dp = dr;

    for (size_t i = 0; i < dni; ++i, sp += s_i, dp += d_i)
    {
      vil_image_view<SrcT> const src_view(src.memory_chunk(), sp,
                                          kni, knj, knp,
                                          s_i, s_j, s_p);
      *dp = DestT(detail::normalized_cross_correlation_px<SrcT, KernelT, AccumT>(src_view, kernel, kernel_stdev, kernel_mean) / ksz);
    }
  }
}

}

#endif // VIDTK_VIDEO_NORMALIZED_CROSS_CORRELATION_H

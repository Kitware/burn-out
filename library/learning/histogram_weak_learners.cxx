/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "histogram_weak_learners.h"

#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_sstream.h>
#include <vcl_iostream.h>
#include <vcl_iomanip.h>
#include <vcl_fstream.h>

namespace vidtk
{

weak_learner_sptr
histogram_weak_learner::train( training_feature_set const & datas,
                               vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
//   vnl_vector<double> histogram_positive, histogram_negative;
  histogram_weak_learner * classifer = new histogram_weak_learner(axis_,name_,descriptor_);
  min_ = datas[0]->get_value(descriptor_)[axis_];
  max_ = datas[0]->get_value(descriptor_)[axis_];
  for( unsigned int i = 1; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(v[axis_] < min_)
    {
      min_ = v[axis_];
    }
    if(v[axis_] > max_)
    {
    max_ = v[axis_];
    }
  }
  if(min_==max_) max_+= 1.;
//   vcl_cout << "min max " << min_ << " " << max_ << "   " << descriptor_ << " " << axis_ << vcl_endl;
//   assert(min_ != max_);
  vnl_vector<double> histogram_positive(1000,0), histogram_negative(1000,0);
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == 1)
    {
      histogram_positive[bin(v[axis_])] += weights[i];
    }
    else
    {
      histogram_negative[bin(v[axis_])] += weights[i];
    }
  }
  classifer->min_ = min_;
  classifer->max_ = max_;
  vnl_vector<double> smoothed_histogram_positive(1000,0), smoothed_histogram_negative(1000,0);
/*  classifer->histogram_positive_ = histogram_positive;*/
//   classifer->histogram_negative_ = histogram_negative;
  for(unsigned int i = 0; i < 1000; ++i)
  {
    smoothed_histogram_positive[i] = smoothed_value(i,histogram_positive);
    smoothed_histogram_negative[i] = smoothed_value(i,histogram_negative);
  }
  vcl_vector<char> bin_class(1000,0);
  for(unsigned int i = 0; i < 1000; ++i)
  {
    if(smoothed_histogram_positive[i] > smoothed_histogram_negative[i])
    {
      bin_class[i] = 1;
    }
    else if(smoothed_histogram_positive[i] < smoothed_histogram_negative[i])
    {
      bin_class[i] = -1;
    }
  }
//   vcl_vector< char > old = bin_class;
  for(unsigned int i = 0; i < 1000; ++i)
  {
    if(bin_class[i] != 0){ continue; }
    unsigned int begin = i;
    unsigned int end = begin;
    while(end != 1000 && bin_class[end+1] == 0){end++;}
    if(begin == 0)
    {
      assert(end+1 < 1000);
      for(unsigned int j = begin; j <= end; ++j)
      {
        bin_class[j] = bin_class[end+1];
      }
    }
    else if(end == 1000)
    {
      assert(begin != 0);
      for(unsigned int j = begin; j < end; ++j)
      {
        bin_class[j] = bin_class[begin - 1];
      }
    }
    else
    {
      assert(begin != 0);
      assert(end+1 < 1000);
      if(bin_class[end+1]==bin_class[begin - 1])
      {
        for(unsigned int j = begin; j <= end; ++j)
        {
          bin_class[j] = bin_class[begin - 1];
        }
      }
      else
      {
        unsigned int midpoint = (unsigned int)((begin+end)*0.5);
        for(unsigned int j = begin; j <= midpoint; ++j)
        {
          bin_class[j] = bin_class[begin - 1];
        }
        for(unsigned int j = midpoint+1; j <= end; ++j)
        {
          bin_class[j] = bin_class[end+1];
        }
      }
    }
  }
  classifer->bin_class_ = bin_class;
  return (weak_learner*)(classifer);
}

int histogram_weak_learner::classify(learner_data const & data)
{
//   if(!(axis_ < data.get_value(descriptor_).size()))
//   {
//     vcl_cout << name_ << " " << descriptor_ << " " << axis_ << "  " << data.get_value(descriptor_).size() << vcl_endl;
//   }
  assert(axis_ < data.get_value(descriptor_).size());
  vnl_vector<double> v = data.get_value(descriptor_);
  unsigned int at = bin(v[axis_]);
  assert(at < bin_class_.size());
  return this->bin_class_[at];
}

void histogram_weak_learner::debug(unsigned int /*t*/) const
{
//     vcl_string fname_prefix("adaboost_debug_lm_");
//     vcl_stringstream ss;
//     ss << fname_prefix << vcl_setw( 4 ) << vcl_setfill( '0' ) << t << ".csv";
//     ss >> fname_prefix;
//     vcl_ofstream out(fname_prefix.c_str());
//     for(unsigned int i = 0; i < histogram_positive_.size(); ++i)
//     {
//       out << histogram_positive_[i] << "," << histogram_negative_[i]<< vcl_endl;
//     }
//     out.close();
}

double histogram_weak_learner::smoothed_value(int at, vnl_vector<double> & hist)
{
  double k[] = {0.006, 0.061, 0.242, 0.383, 0.242, 0.061, 0.006};
  double result = 0;
  for (int i = 0; i < 7; ++i)
  {
    int t = at+i-3;
    if (t>=0)
    {
      result += k[i]*hist[t];
    }
  }
  return result;
}

bool
histogram_weak_learner::read(vcl_istream & in)
{
  bool r = weak_learner::read(in);
  in >> min_ >> max_ >> axis_;
  bin_class_.resize(1000);
  for(unsigned int i = 0; i < 1000; ++i)
  {
    int tmp;
    in >> tmp;
    bin_class_[i] = static_cast<char>(tmp);
  }
  return r;
}

bool
histogram_weak_learner::write(vcl_ostream & out) const
{
  bool r = weak_learner::write(out);
  out << min_ << " " << max_ << " " << axis_ << vcl_endl;
  for(unsigned int i = 0; i < 1000; ++i)
  {
    out << static_cast<int>(bin_class_[i]) << " ";
  }
  out << vcl_endl;
  return r;
}

weak_learner_sptr
histogram_weak_learner_inSTD::train( training_feature_set const & datas,
                                  vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  histogram_weak_learner_inSTD * classifer = new histogram_weak_learner_inSTD(axis_,name_,descriptor_);
  double mean = 0;
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    mean += weights[i]*v[axis_];
  }
  double var = 0;
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    double d = v[axis_] - mean;
    var += weights[i]*(d*d);
  }
  double std = vcl_sqrt(var);
  min_ = mean-1.3*std;
  max_ = mean+1.3*std;
  if(min_ == max_) {max_ = min_+1;}
//   vcl_cout << "min max " << min_ << " " << max_ << "   " << descriptor_ << " " << axis_ << vcl_endl;
//   assert(min_ != max_);
  vnl_vector<double> histogram_positive(1000,0), histogram_negative(1000,0);
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == 1)
    {
      histogram_positive[bin(v[axis_])] += weights[i];
    }
    else
    {
      histogram_negative[bin(v[axis_])] += weights[i];
    }
  }
  classifer->min_ = min_;
  classifer->max_ = max_;
  vnl_vector<double> smoothed_histogram_positive(1000,0), smoothed_histogram_negative(1000,0);
/*  classifer->histogram_positive_ = histogram_positive;*/
//   classifer->histogram_negative_ = histogram_negative;
  for(unsigned int i = 0; i < 1000; ++i)
  {
    smoothed_histogram_positive[i] = smoothed_value(i,histogram_positive);
    smoothed_histogram_negative[i] = smoothed_value(i,histogram_negative);
  }
  vcl_vector<char> bin_class(1000,0);
  for(unsigned int i = 0; i < 1000; ++i)
  {
    if(smoothed_histogram_positive[i] > smoothed_histogram_negative[i])
    {
      bin_class[i] = 1;
    }
    else if(smoothed_histogram_positive[i] < smoothed_histogram_negative[i])
    {
      bin_class[i] = -1;
    }
  }
//   vcl_vector<char> old = bin_class;
  for(unsigned int i = 0; i < 1000; ++i)
  {
    if(bin_class[i] != 0){ continue; }
    unsigned int begin = i;
    unsigned int end = begin;
    while(end+1 != 1000 && bin_class[end+1] == 0){end++;}
    if(end == 999){ end = 1000; }
    if(begin == 0)
    {
      assert(end+1 < 1000);
      for(unsigned int j = begin; j <= end; ++j)
      {
        bin_class[j] = bin_class[end+1];
      }
    }
    else if(end == 1000)
    {
      assert(begin != 0);
      for(unsigned int j = begin; j < end; ++j)
      {
        bin_class[j] = bin_class[begin - 1];
      }
    }
    else
    {
      assert(begin != 0);
      if(!(end+1 < 1000))
      {
        vcl_cout << begin << "  " << end << "    " << bin_class[end] << vcl_endl;
        assert(false);
      }
      if(bin_class[end+1]==bin_class[begin - 1])
      {
        for(unsigned int j = begin; j <= end; ++j)
        {
          bin_class[j] = bin_class[begin - 1];
        }
      }
      else
      {
        unsigned int midpoint = (unsigned int)((begin+end)*0.5);
        for(unsigned int j = begin; j <= midpoint; ++j)
        {
          bin_class[j] = bin_class[begin - 1];
        }
        for(unsigned int j = midpoint+1; j <= end; ++j)
        {
          bin_class[j] = bin_class[end+1];
        }
      }
    }
  }
  classifer->bin_class_ = bin_class;
  return (weak_learner*)(classifer);
}

} //namespace vidtk

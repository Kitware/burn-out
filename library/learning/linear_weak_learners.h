/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_adaboost_linear_weak_learners_h
#define vidtk_adaboost_linear_weak_learners_h

#include <vnl/algo/vnl_matrix_inverse.h>
#include <vnl/vnl_least_squares_function.h>
#include <vnl/algo/vnl_levenberg_marquardt.h>

#include <learning/weak_learner.h>

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_linear_weak_learners_h__
VIDTK_LOGGER("linear_weak_learners_h");


namespace vidtk
{

///A simple linear learner
class linear_weak_learner : public weak_learner
{
  public:
    linear_weak_learner( std::string const & _name, int desc )
      : weak_learner(_name, desc), sign_(1)
    {}
    linear_weak_learner( )
    {}
    virtual weak_learner_sptr clone() const
    { return new linear_weak_learner(*this); }
    virtual weak_learner_sptr
      train( training_feature_set const & datas,
             vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data);
    virtual std::string gplot_command() const;
    virtual weak_learners::learner get_id() const
    { return weak_learners::linear_weak_learner; }
    virtual bool read(std::istream & in);
    virtual bool write(std::ostream & out) const;
  protected:
    vnl_vector< double > beta_;
    int sign_;
};

/// Uses LM to move the line along the normal.  The error is the distance off the line.
class lm_linear_weak_learner : public linear_weak_learner
{
  public:
    lm_linear_weak_learner( std::string const & _name, int desc ) : linear_weak_learner(_name, desc){}
    virtual weak_learner_sptr train( training_feature_set const & datas,
                                     vnl_vector<double> const & weights );
    virtual weak_learner_sptr clone() const
    { return new lm_linear_weak_learner(*this); }

  protected:
    class function : public vnl_least_squares_function
    {
      public:
        function( vnl_vector<double> const & dir,
                  training_feature_set const & datas,
                  vnl_vector<double> const & w )
          : vnl_least_squares_function(1,1,no_gradient), weights_(w)
        {
          values_ = vnl_vector<double>(datas.size());
          labels_ = vnl_vector<int>(datas.size());
          for(unsigned int i = 0; i < datas.size(); ++i)
          {
            vnl_vector<double> v = datas[i]->get_value(0);//TODO: FIX ME
            values_[i] = dot_product(v,dir);
            labels_[i] = datas[i]->label();
          }
        }
        virtual void f(vnl_vector<double> const& x, vnl_vector<double>& fx)
        {
          LOG_INFO( "begin" );
          assert(fx.size() == 1.0);
          assert(x.size() == 1.0);
          assert(weights_.size() == values_.size());
          assert(labels_.size() == values_.size());
          fx[0] = 0;
          for( unsigned int i = 0; i < values_.size(); ++i )
          {
            double dp = values_[i] + x[0];
            if( (dp >= 0 && labels_[i] == 1) || (dp < 0 && labels_[i] == -1) )
            {
            }
            else
            {
              fx[0] += weights_[i] * dp * dp;
            }
          }
          LOG_INFO( "end" );
        }
      protected:
        vnl_vector<double> const & weights_;
        vnl_vector<double> values_;
        vnl_vector<int> labels_;
    };
};

///simular to lm_linear_weak_learner.  Uses weighted distance from line divided by vector megnitude.
class lm_linear_weak_learner_v2 : public linear_weak_learner
{
  public:
    lm_linear_weak_learner_v2( std::string const & _name, int desc ) : linear_weak_learner(_name, desc){}
    virtual weak_learner_sptr clone() const
    { return new lm_linear_weak_learner_v2(*this); }
    virtual weak_learner_sptr
      train( training_feature_set const & datas,
             vnl_vector<double> const & weights );

  protected:
    class function : public vnl_least_squares_function
    {
      public:
        function( unsigned int n,
                  training_feature_set const & datas,
                  vnl_vector<double> const & w )
          : vnl_least_squares_function(n,datas.size(),no_gradient), weights_(w)
        {
          labels_ = vnl_vector<int>(datas.size());
          for(unsigned int i = 0; i < datas.size(); ++i)
          {
            vnl_vector<double> v = datas[i]->get_value(0);//TODO: FIX ME
            vnl_vector<double> v_hom(n,1);
            for(unsigned int j = 0; j < n-1; ++j)
            {
              v_hom[j] = v[j];
            }
            values_.push_back(v_hom);
            labels_[i] = datas[i]->label();
          }
        }
        virtual void f(vnl_vector<double> const& x, vnl_vector<double>& fx)
        {
//           LOG_INFO( "begin" );
          assert(fx.size() == values_.size());
          assert(x.size() == values_[0].size());
          assert(weights_.size() == values_.size());
          assert(labels_.size() == values_.size());
          assert(x.squared_magnitude());
          for( unsigned int i = 0; i < values_.size(); ++i )
          {
            double dp = dot_product(x,values_[i]);
            if( (dp >= 0 && labels_[i] == 1) || (dp < 0 && labels_[i] == -1) )
            {
              fx[i] = 0;
            }
            else if( labels_[i] == 1 )
            {
              if(dp<0) dp = -dp;
              fx[i] = 20.0*(weights_[i] * dp * weights_[i] * dp)/values_[i].squared_magnitude();
            }
            else
            {
              if(dp<0) dp = -dp;
              fx[i] = 0.05*(weights_[i] * dp * weights_[i] * dp)/values_[i].squared_magnitude();
            }
          }
//           LOG_INFO( "end" );
        }
      protected:
        vnl_vector<double> const & weights_;
        std::vector< vnl_vector<double> > values_;
        vnl_vector<int> labels_;
    };
};

/// Move the line such that all positive examples are explaned correctly
class lm_bias_positive_linear_weak_learner : public linear_weak_learner
{
  public:
    lm_bias_positive_linear_weak_learner( std::string const & _name, int desc ) : linear_weak_learner(_name, desc){}
    virtual weak_learner_sptr clone() const
    { return new lm_bias_positive_linear_weak_learner(*this); }
    virtual weak_learner_sptr train( training_feature_set const & datas,
                                     vnl_vector<double> const & weights );

  protected:

    class function : public vnl_least_squares_function
    {
      public:
        function( vnl_vector<double> const & dir,
                  training_feature_set const & datas,
                  vnl_vector<double> const & w )
          : vnl_least_squares_function(1,1,no_gradient), weights_(w)
        {
          values_ = vnl_vector<double>(datas.size());
          labels_ = vnl_vector<int>(datas.size());
          for(unsigned int i = 0; i < datas.size(); ++i)
          {
            vnl_vector<double> v = datas[i]->get_value(0);//TODO: FIX ME
            values_[i] = dot_product(v,dir);
            labels_[i] = datas[i]->label();
          }
        }
        virtual void f(vnl_vector<double> const& x, vnl_vector<double>& fx)
        {
//           LOG_INFO( "begin" );
          assert(fx.size() == 1.0);
          assert(x.size() == 1.0);
          assert(weights_.size() == values_.size());
          assert(labels_.size() == values_.size());
          fx[0] = 0;
          for( unsigned int i = 0; i < values_.size(); ++i )
          {
            double dp = values_[i] + x[0];
            if( (dp < 0 && labels_[i] == 1) )
            {
              fx[0] += weights_[i] *  weights_[i] * dp * dp;
            }
          }
//           LOG_INFO( "end" );
        }
      protected:
        vnl_vector<double> const & weights_;
        vnl_vector<double> values_;
        vnl_vector<int> labels_;
    };
};

/// Move the line such that all negative examples are explaned correctly
class lm_bias_negative_linear_weak_learner : public linear_weak_learner
{
  public:
    lm_bias_negative_linear_weak_learner( std::string const & _name, int desc ) : linear_weak_learner(_name, desc){}
    virtual weak_learner_sptr clone() const
    { return new lm_bias_negative_linear_weak_learner(*this); }
    virtual weak_learner_sptr train( training_feature_set const & datas,
                                     vnl_vector<double> const & weights );

  protected:
    class function : public vnl_least_squares_function
    {
      public:
        function( vnl_vector<double> const & dir,
                  training_feature_set const & datas,
                  vnl_vector<double> const & w )
          : vnl_least_squares_function(1,1,no_gradient), weights_(w)
        {
          values_ = vnl_vector<double>(datas.size());
          labels_ = vnl_vector<int>(datas.size());
          for(unsigned int i = 0; i < datas.size(); ++i)
          {
            vnl_vector<double> v = datas[i]->get_value(0);//TODO: FIX ME
            values_[i] = dot_product(v,dir);
            labels_[i] = datas[i]->label();
          }
        }
        virtual void f(vnl_vector<double> const& x, vnl_vector<double>& fx)
        {
//           LOG_INFO( "begin" );
          assert(fx.size() == 1.0);
          assert(x.size() == 1.0);
          assert(weights_.size() == values_.size());
          assert(labels_.size() == values_.size());
          fx[0] = 0;
          for( unsigned int i = 0; i < values_.size(); ++i )
          {
            double dp = values_[i] + x[0];
            if( !(dp < 0 && labels_[i] == -1) )
            {
            }
            else
            {
              fx[0] += weights_[i] * weights_[i] * dp * dp;
            }
          }
//           LOG_INFO( "end" );
        }
      protected:
        vnl_vector<double> const & weights_;
        vnl_vector<double> values_;
        vnl_vector<int> labels_;
    };
};

}//namespace vidtk

#endif

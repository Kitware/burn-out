/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pvo_probability_h_
#define vidtk_pvo_probability_h_

#include <vcl_ostream.h>

#include <vnl/vnl_vector.h>

namespace vidtk
{

class pvo_probability
{
  public:
    enum id{person = 0, vehicle = 1, other = 2}; 
    ///constructors
    pvo_probability( );
    //pvo_probability( pvo_probability const & rhs );
    pvo_probability( double probability_person,
                     double probability_vehicle,
                     double probability_other );

    /// Get and set methods
    double get_probability( pvo_probability::id i );
    vnl_vector<double> get_probabilities();
    double get_probability_person() const;
    void set_probability_person( double p );
    double get_probability_vehicle() const;
    void set_probability_vehicle( double p );
    double get_probability_other() const;
    void set_probability_other( double p );
    void set_probabilities( double pperson,
                            double pvehicle,
                            double pother);
    void adjust( double pper, double pveh, double poth);
    ///Normalize the probabilities so they sum to one
    void normalize();

    pvo_probability & operator *=(const pvo_probability & b);
    //void operator =( pvo_probability const & rhs);
  protected:
    double probability_person_;
    double probability_vehicle_;
    double probability_other_;
};

}

vidtk::pvo_probability operator*(const vidtk::pvo_probability & l, const vidtk::pvo_probability & r);

vcl_ostream& operator<<(vcl_ostream& os, const vidtk::pvo_probability &p);

#endif // vidtk_pvo_probability_h_

/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pvo_probability_h_
#define vidtk_pvo_probability_h_

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <ostream>

namespace vidtk
{

class pvo_probability;

/// Type name of smart pointer used to manage objects.
typedef vbl_smart_ptr< pvo_probability > pvo_probability_sptr;

// ----------------------------------------------------------------
/**
 * @brief PVO (TOT) probability
 *
 * This class represents Target Object Type (TOT) probabilities for
 *  different classes of objects.  There are three built-in target
 *  types; Person, Vehicle, and Other, which is why this class is
 *  called PVO.
 *
 * These three probabilities are usually normalized so they sum to
 * one. It is possible to put an object in an unnormalized state by
 * setting individual probabilities, so it is best to call normalize()
 * after setting component probabilities.
 *
 * In the application domain, it is undesirable to have a zero
 * probability, even though in a pure mathematical realm probabilities
 * can be zero. If a component probability gets below a minimum value
 * (as reported by get_minimum_value()), it is set to that value.
 */
class pvo_probability
  : public vbl_ref_count
{
public:
  /**
   * @brief Target Object Type code.
   *
   * These symbols are used to select the desired component from this object.
   * Currently only three types are supported, but the possibility of adding
   * other application types exists
   */
  enum id {
    person,
    vehicle,
    other
  };

  /**
   * @brief Default constructor.
   *
   * A default constructed PVO object is created. All probabilities
   * are set to 1/3. The new object has all probabilities
   * normalized().
   */
  pvo_probability();

  /**
   * @brief Initializing constructor.
   *
   * This constructor creates a PVO object with the specified
   * probabilities. The resulting object has all probabilities
   * normalized().
   *
   * @param probability_person Probability for person.
   * @param probability_vehicle Probability for vehicle.
   * @param probability_other Probability for other.
   */
  pvo_probability( double probability_person,
                   double probability_vehicle,
                   double probability_other );

  /**
   * @brief Get an individual probability value.
   *
   * This method returns the probability for an individual Target
   * Object Type.
   *
   * @param i Target Object Type symbol (e.g.pvo_probability::person)
   *
   * @return The probability value for selected target object type.
   */
  double get_probability( pvo_probability::id i ) const;

  /**
   * @brief Set individual probability value.
   *
   * This method sets the probability for a single Target Object
   * Type. The resulting object is not normalized, since the typical
   * use case is to set the probabilities for multiple Target Object
   * Types, so don't forget to normalize() when you are done setting
   * probabilities,
   *
   * @param i Target Object Type symbol (e.g.pvo_probability::person)
   * @param val The new probability value for selected TOT.
   */
  void set_probability( pvo_probability::id i, double val );

  // These methods do not support an extensible interface where there
  // may be more application specific probability types.
  double get_probability_person() const { return get_probability( pvo_probability::person ); }
  void set_probability_person( double p ) { set_probability( pvo_probability::person, p ); }
  double get_probability_vehicle() const { return get_probability( pvo_probability::vehicle ); }
  void set_probability_vehicle( double p ) { set_probability( pvo_probability::vehicle, p ); }
  double get_probability_other() const { return get_probability( pvo_probability::other ); }
  void set_probability_other( double p ) { set_probability( pvo_probability::other, p ); }

  /**
   * @brief Multiply PVO by supplied values.
   *
   * The internal PVO values are adjusted by multiplying each
   * probability by the specified constant. This object is normalized
   * after this multiplication.
   *
   * @param pper Value to be applied to person probability
   * @param pveh Value to be applied to vehicle probability
   * @param poth Value to be applied to other probability
   */
  void scale_pvo( double pper, double pveh, double poth );

  /**
   * @brief Normalize the probabilities.
   *
   * The component probabilities are normalized so they sum to one.
   * Probabilities that are less than the minimum value supported are
   * set to the minimum value.
   */
  void normalize();

  /**
   * @brief Have probabilities been set?
   *
   * This method returns \b true if probability values have been
   * set. A default constructed object will report \b false indicating
   * that values have not been set.
   *
   * @return \b true of values have been set, \b false otherwise.
   */
  bool is_set() const;

  /**
   * @brief Multiply this PVO by another.
   *
   * This operator multiplies this PVO object by the supplied object
   * and updates the values in this object. The resulting values are
   * normalized.
   *
   * @param b Other probabilities to incorporate.
   */
  pvo_probability& operator*=( pvo_probability const& b );

  /**
   * @brief Return minimum probability supported.
   *
   * This method returns the minimum probability value that is
   * supported. Probability values less than this value will be set to
   * this value. This is to deal with application level problems that
   * arise when a probability of zero is encountered.
   *
   * @return Minimum probability value.
   */
  static double get_minimum_value();

protected:
  double probability_person_;
  double probability_vehicle_;
  double probability_other_;

  bool is_set_;

private:
  /// Explicitly define minimum value, above zero
  const static double min_probability_;
};


/**
 * @brief Combine two probabilities.
 *
 * This operator multiplies two TOT (PVO) probabilities and returns
 * the normalized result as a new object.
 *
 * @param l Left hand operand
 * @param r Right hand operand
 *
 * @return Normalized product of probabilities.
 */
vidtk::pvo_probability operator*( const vidtk::pvo_probability& l, const vidtk::pvo_probability& r );


/**
 * @brief Output operator
 *
 * @param os Stream to format on
 * @param p PVO to format
 *
 * The supplied PVO object is formatted to the supplied stream as
 * three values (P V O) separated by a space.
 *
 * @return A string of the format "p.pp v.vv o.oo" is returned.
 */
std::ostream& operator<<( std::ostream& os, const vidtk::pvo_probability& p );

} // end namespace

#endif // vidtk_pvo_probability_h_

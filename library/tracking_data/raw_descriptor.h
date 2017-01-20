/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_raw_descriptor_
#define vidtk_raw_descriptor_

#include <vector>
#include <string>

#include <vgl/vgl_box_2d.h>
#include <vbl/vbl_smart_ptr.h>

#include <utilities/timestamp.h>

#include <tracking_data/track.h>

namespace vidtk
{

class descriptor_history_entry;
class raw_descriptor;

// Helper typedefs
typedef std::string descriptor_id_t;
typedef vbl_smart_ptr< raw_descriptor > descriptor_sptr_t;
typedef std::vector< double > descriptor_data_t;
typedef descriptor_sptr_t descriptor_sptr;
typedef std::vector< descriptor_history_entry > descriptor_history_t;


// ----------------------------------------------------------------
/** \brief Raw descriptor.
 *
 * A raw descriptor typically represents some measurements taken
 * either from image contents (such as a BoW model over some region, a
 * HoG descriptor, a HoOF, some shape model...) or other source.
 *
 * It could have been computed using time-series image data, from data
 * from just tracks, 2D image data, or something else. Descriptors are
 * typically used as an intermediate form before storing or as input
 * for classification for higher level recognition tasks.
 *
 * Note that object of this class are created using factory methods.
 */
class raw_descriptor
  : public vbl_ref_count
{
public:
  // -- TYPES --
  typedef std::vector< descriptor_sptr_t > vector_t;

  /**
   * \brief Raw descriptor factory method.
   *
   * This method creates a new raw descriptor with the supplied
   * type. This factory method ensures that the new descriptor is
   * managed with a smart pointer.
   *
   * @param type Identifier for the new descriptor.
   *
   * @return A smart pointer to the new descriptor is returned.
   */
  static descriptor_sptr create( std::string const& type );


  /**
   * \brief Raw descriptor factory method.
   *
   * This method creates a new raw descriptor with the via performing
   * a deep copy of another raw descriptor.
   *
   * @param to_copy A smart pointer to another descriptor.
   *
   * @return A smart pointer to the new descriptor is returned.
   */
  static descriptor_sptr create( descriptor_sptr to_copy );


  ~raw_descriptor();


  /**
   * \brief Override the descriptor type for this descriptor.
   *
   * Sets a new identifier for this descriptor.
   *
   * @param type The descriptor identifier
   */
  void set_type( const descriptor_id_t& type );


  /**
   * \brief Returns the descriptor type.
   *
   * This function returns the descriptor category identifier.
   *
   * @return The descriptor identifier.
   */
  descriptor_id_t const& get_type() const;


  /**
   * \brief Add new track id to raw descriptor.
   *
   * The track id is added to the end of the list of track IDs in
   * this object.
   *
   * @param id Track id to add
   */
  void add_track_id( track::track_id_t id );


  /**
   * \brief Adds multiple new track ids to raw descriptor.
   *
   * The track ids are added to the end of the list of track IDs in
   * this object.
   *
   * @param id Track id to add
   */
  void add_track_ids( const std::vector< track::track_id_t >& ids );


  /**
   * \brief Get list of track ID's.
   *
   * The list of track ID's is returned. The list may be empty.
   *
   * @return List of track ID's in this object.
   */
  std::vector< track::track_id_t > const& get_track_ids() const;


  /**
   * \brief Set descriptor data vector.
   *
   * The supplied raw data vector is copied into this descriptor
   * replacing any existing descriptor data.
   *
   * @param data Descriptor data vector
   */
  void set_features( descriptor_data_t const& data );


  /**
   * \brief Get read only access to data.
   *
   * This method supplies read only access to the descriptor data
   * vector.
   *
   * @return Reference to descriptor data vector.
   */
  descriptor_data_t const& get_features() const;


  /**
   * \brief Get read/write access to data.
   *
   * This method provides read and write access to the internal
   * descriptor data vector. It may not be the best approach to
   * provide wide open access to the internal data vector, but due to
   * compatibility issues and some deference to efficiency, creators
   * of these descriptors are allowed unfettered access to the data
   * vector.
   *
   * @return Reference to descriptor data vector.
   */
  descriptor_data_t& get_features();


  //@{
  /**
   * \brief Index into feature data vector.
   *
   * This method indexes into the feature vector and returns a
   * reference to a single element. If the index is out of range, an
   * out of range exception is thrown.
   *
   * Providing a subscript operator adds some data abstraction to this
   * class which is a good thing.
   *
   * @param idx Feature vector index.
   *
   * @return Reference to feature data element.
   * @throw std::out_of_range exception
   */
  double& at( size_t idx );
  double const& at( size_t idx ) const;
  //@}


  /**
   * \brief Get size of feature vector.
   *
   * This method returns the number of elements in the feature vector.
   *
   * @return Number of elements in feature vector.
   */
  size_t features_size() const;


  /**
   * \brief Resize features vector.
   *
   * This method resizes the feature vector so that it contains \c n
   * elements.
   *
   * If \c n is smaller than the current container size, the content is
   * reduced to its first \c n elements, removing those beyond (and
   * destroying them).
   *
   * If \c n is greater than the current container size, the content is
   * expanded by inserting at the end as many elements as needed to
   * reach a size of \c n.
   *
   * If \cn is also greater than the current container capacity, an
   * automatic reallocation of the allocated storage space takes
   * place.
   *
   * Notice that this function changes the actual content of the
   * container by inserting or erasing elements from it.
   *
   * @param n New size of features vector.
   */
  void resize_features( size_t n );


  /**
   * \brief Resize features vector.
   *
   * This method resizes the feature vector so that it contains \c n
   * elements.
   *
   * If \c n is smaller than the current container size, the content is
   * reduced to its first \c n elements, removing those beyond (and
   * destroying them).
   *
   * If \c n is greater than the current container size, the content is
   * expanded by inserting at the end as many elements as needed to
   * reach a size of \c n.
   *
   * If \cn is also greater than the current container capacity, an
   * automatic reallocation of the allocated storage space takes
   * place.
   *
   * Notice that this function changes the actual content of the
   * container by inserting or erasing elements from it.
   *
   * @param n New size of features vector.
   * @param init_value Value to initialize the (potentially) added slots.
   */
  void resize_features( size_t n, int init_value );


  /**
   * \brief Does the feature vector contain any features?
   *
   * @return Whether or not the feature vector is empty.
   */
  bool has_features() const;


  /**
   * \brief Set history vector.
   *
   * This method sets the descriptor history to the supplied
   * vector. Any existing history will be replaced.
   *
   * @param hist New history vector.
   */
  void set_history( descriptor_history_t const& hist );


  /**
   * \brief Add history entry.
   *
   * A new history element is added to the end of any existing
   * history.
   *
   * @param hist New history element to add.
   */
  void add_history_entry( descriptor_history_entry const& hist );


  /**
   * \brief Get history vector.
   *
   * This method returns the history vector for this descriptor.
   *
   * @return Current history vector.
   */
  descriptor_history_t const& get_history() const;


  /**
   * \brief Set PVO flag.
   *
   * This method sets the PVO flag as specified.
   *
   * @param f
   */
  void set_pvo_flag( bool f );


  /**
   * \brief Return PVO flag.
   *
   * This method returns the current state of the internal PVO flag.
   * This flag is set to indicate that this descriptor is useful for
   * PVO classification, but will shortly be deprecated.
   *
   * @return PVO status of this descriptor.
   */
  bool is_pvo() const;


private:
  /// Default constructor
  raw_descriptor();

  /// Descriptor type ID
  descriptor_id_t type_;

  /// IDs of tracks this descriptor came from, if exists.
  std::vector< track::track_id_t > track_ids_;

  /// Actual descriptor data contents
  descriptor_data_t data_;

  /// History of descriptor, if known
  descriptor_history_t history_;

  /// Is this descriptor useful for PVO?
  bool is_pvo_;
};


// ----------------------------------------------------------------
/*
 * \brief Descriptor history entry.
 *
 * If the full history of some descriptor is recorded, one of these
 * should be created for every frame which the descriptor covers (see
 * raw_descriptor documentation). Only quanities which get used downstream
 * need be filled.
 */
class descriptor_history_entry
{
public:
  // -- TYPES --
  typedef vgl_box_2d< unsigned > image_bbox_t;
  typedef vgl_box_2d< double > world_bbox_t;

  /// Constructors
  ~descriptor_history_entry();

  /**
   * Create new object.
   *
   * @param ts Timestamp for this entry
   * @param img_loc Image location for object
   * @param world_loc World location for image
   */
  descriptor_history_entry( const vidtk::timestamp& ts,
                            const image_bbox_t& img_loc,
                            const world_bbox_t& world_loc );

/**
 * Create new object.
 *
 * @param ts Timestamp for object.
 * @param img_loc Image location for object.
 */
  descriptor_history_entry( const vidtk::timestamp& ts,
                            const image_bbox_t& img_loc );

  /**
   * \brief Get timestamp.
   *
   *
   * @return timestamp for this entry
   */
  vidtk::timestamp get_timestamp() const;


  /**
   * \brief Get image location.
   *
   *
   * @return Bounding box in image coordinates (pixels).
   */
  image_bbox_t const& get_image_location() const;


  /**
   * \brief Get world location.
   *
   *
   * @return Bounding box in world coordinates, usually in meters.
   */
  world_bbox_t const& get_world_location() const;


private:
  descriptor_history_entry(); /* not implemented */

  /// Frame ID and timestamp of the current frame
  vidtk::timestamp ts_;

  /// Image location (pixels)
  image_bbox_t img_loc_;

  /// World location (world units)
  world_bbox_t world_loc_;
};


} // end namespace

#endif

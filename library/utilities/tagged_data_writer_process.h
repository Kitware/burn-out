/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _TAGGED_DATA_WRITER_PROCESS_H_
#define _TAGGED_DATA_WRITER_PROCESS_H_

#include "base_writer_process.h"
#include "tagged_data_ports.h"

#include <utilities/timestamp.h>
#include <utilities/video_modality.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>
#include <tracking_data/shot_break_flags.h>
#include <vil/vil_image_view.h>


namespace vidtk
{

// ----------------------------------------------------------------
/** Write various metadata to file.
 *
 * This class represents a process that writes one or more values to a
 * file.
 *
 *
 * Open issues:
 * - currently do not handle timestamp vectors
 *
 * Dynamic behaviour
\msc
hscale="1.5";

app [ label="pipeline" ],
c [ label="  tagged_data_writer_process" ],
b [ label="  base_writer_process" ],
a [ label="  base_io_process" ],
g [ label="  group_reader_writer" ],
w [ label="  <data writer>" ];

---[ label="collect parameters" ];
app=>c [ label="params()" ];
c=>a   [ label="params()" ];

---[ label="retrieve parameters" ];
app=>c [ label="set_params()" ];
c=>a   [ label="set_params()" ];

---[ label="initialize process" ];
app=>a [ label="initialize()" ];
a=>b   [ label="open_file()" ];
a=>c   [ label="initialize(internal)" ];
c=>a   [ label="add_data_io()", ID="for each writer"];
a=>g   [ label="add_data_reader_writer()" ];
c=>b   [ label="initialize(internal)" ];
b=>g   [ label="write_header()" ];

---[ label="process step" ];
app=>a [ label="step()" ];
a=>b   [ label="do_file_step()" ];
b=>g   [ label="write_object()" ];
g=>w   [ label="write_object()", ID="for each writer"];

\endmsc
 */
class tagged_data_writer_process
  : public base_writer_process
{
public:
  typedef tagged_data_writer_process self_type;

  tagged_data_writer_process(std::string const& name);
  virtual ~tagged_data_writer_process();

  // Process interface
  virtual bool set_params(config_block const&);
  virtual bool initialize_internal();


/** \fn set_input_xxx (T val)
 * \brief Supply the input value for the 'xxx' datum.
 * The value supplied by this function will be written to output on the next process
 * iteration.
 */

/** \fn set_xxx_connected ()
 * \brief Indicate that the specified 'xxx' input port is connected.
 * This method indicates that the 'xxx' input port has been connected to
 * another process.  If an input port is not connected, then it can not
 * be enabled via the configuration.
 */

/** \fn force_xxx_enable()
 * \brief Force the 'xxx' input to be written.
 * This method forces the specified 'xxx' input to be written to the output file.
 * This is useful when temporarily adding a writer and you do not want to
 * modify the config file.  When an input has been force enabled, it can not be
 * disabled by a config file entry.
 */

#define MDRW_INPUT(N,T,W,I)                                             \
public:                                                                 \
  void set_input_ ## N(T const & val ) { m_ ## N = val; }               \
  VIDTK_INPUT_PORT( set_input_ ## N, T const& );                        \
  void set_ ## N ## _connected (bool v) { m_ ## N ## _connected = v; }  \
  void set_force_ ## N ## _enabled () { m_ ## N ## _force_enabled = true; } \
private:                                                                \
  T m_ ## N;                                                            \
  bool m_ ## N ## _enabled;                                             \
  bool m_ ## N ## _force_enabled;                                       \
  bool m_ ## N ## _connected;                                           \
public:

  MDRW_SET  // apply macro over inputs
#undef MDRW_INPUT

  int last;

}; // end class tagged_data_writer_process

} // end namespace

#endif /* _TAGGED_DATA_WRITER_PROCESS_H_ */

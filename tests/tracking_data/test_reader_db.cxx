/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <testlib/testlib_test.h>

#include <tracking_data/io/track_reader_db.h>
#include <tracking_data/io/track_reader_process.h>

#include <logger/logger.h>
VIDTK_LOGGER( "test_reader_db.cxx" );

using namespace vidtk;

namespace
{

void test_sqlite_connection(std::string db_path, vxl_int_32 session_id)
{
  // Test connection with track reader
  // /!\ Do not change the database here !
  {
    // 1-  Correct connection -> Should work
    {
      ns_track_reader::track_reader_db reader;

      ns_track_reader::track_reader_options options;
      options.set_db_type("sqlite3");
      options.set_db_user("");
      options.set_db_name(db_path);
      options.set_db_pass("");
      options.set_db_host("");
      options.set_db_port("");
      options.set_db_session_id(session_id);
      reader.update_options(options);
      TEST("sqlite3 connection | track_reader_db | Good connection: ",
        reader.open(""), true);
      TEST("sqlite3 connection | track_reader_db | Good connection, 2nd attempt: ",
        reader.open(""), false);
    }
    // 2-  Correct db_name, wrong db_type -> Should fail
    {
      ns_track_reader::track_reader_db reader;

      ns_track_reader::track_reader_options options;
      options.set_db_type("postegresql");
      options.set_db_user("");
      options.set_db_name(db_path);
      options.set_db_pass("");
      options.set_db_host("");
      options.set_db_port("");
      options.set_db_session_id(session_id);
      reader.update_options(options);
      TEST("sqlite3 connection | track_reader_db | Bad connection: ",
        reader.open(""), false);
      TEST("sqlite3 connection | track_reader_db | Good connection, 2nd attempt: ",
        reader.open(""), false);
    }
    // 3-  Correct db_name, unecessary parameters -> Should work
    {
      ns_track_reader::track_reader_db reader;

      ns_track_reader::track_reader_options options;
      options.set_db_type("sqlite3");
      options.set_db_user("TotallyAuthorizedUser");
      options.set_db_name(db_path);
      options.set_db_pass("not_my_pet_name");
      options.set_db_host("ServerInTheGarage");
      options.set_db_port("starboard");
      options.set_db_session_id(session_id);
      reader.update_options(options);
      TEST("sqlite3 connection | track_reader_db | Good connectionn && unecessary params: ",
        reader.open(""), true);
      TEST("sqlite3 connection | track_reader_db | Good connectionn && unecessary params, 2nd attempt: ",
        reader.open(""), false);
    }
    // 4-  Correct db_name, wrong session id -> Should Fail
    {
      ns_track_reader::track_reader_db reader;

      ns_track_reader::track_reader_options options;
      options.set_db_type("sqlite3");
      options.set_db_user("");
      options.set_db_name(db_path);
      options.set_db_pass("");
      options.set_db_host("");
      options.set_db_port("");
      options.set_db_session_id(-42);
      reader.update_options(options);
      TEST("sqlite3 connection | track_reader_db | Good connectionn && wrong session id: ",
        reader.open(""), false);
      TEST("sqlite3 connection | track_reader_db | Good connectionn && wrong session id, 2nd attempt: ",
        reader.open(""), false);
    }
  }

    // Test connection with track_reader_process
  {
    // 1-  Correct connection -> Should Work
    {
      track_reader_process reader("reader");
      config_block blk = reader.params();
      blk.set("disabled", "false");
      blk.set("db_type", "sqlite3");
      blk.set("db_user", "");
      blk.set("db_name", db_path);
      blk.set("db_pass", "");
      blk.set("db_host", "");
      blk.set("db_port", "");
      blk.set("db_session_id", session_id);
      blk.set("filename", "");

      TEST("sqlite3 connection | track_reader_process | Good connection | set_params: ",
        reader.set_params(blk), true);
      TEST("sqlite3 connection | track_reader_process | Good connection | initialize: ",
        reader.initialize(), true);
    }
    // 2-  Bad connection -> Should fail
    {
      track_reader_process reader("reader");
      config_block blk = reader.params();
      blk.set("disabled", "false");
      blk.set("db_type", "postegresql");
      blk.set("db_user", "");
      blk.set("db_name", db_path);
      blk.set("db_pass", "");
      blk.set("db_host", "");
      blk.set("db_port", "");
      blk.set("db_session_id", session_id);
      blk.set("filename", "");

      TEST("sqlite3 connection | track_reader_process | Bad connection | set_params: ",
        reader.set_params(blk), true);
      TEST("sqlite3 connection | track_reader_process | Bad connection | initialize: ",
        reader.initialize(), false);
    }
    // 3-  Good connection, unecessary parameters -> Should work
    {
      track_reader_process reader("reader");
      config_block blk = reader.params();
      blk.set("disabled", "false");
      blk.set("db_type", "sqlite3");
      blk.set("db_user", "PleaseLetMeIn");
      blk.set("db_name", db_path);
      blk.set("db_pass", "qwerty");
      blk.set("db_host", "me");
      blk.set("db_port", "wine");
      blk.set("db_session_id", session_id);
      blk.set("filename", "");

      TEST("sqlite3 connection | track_reader_process | Good connection  && unecessary params | set_params: ",
        reader.set_params(blk), true);
      TEST("sqlite3 connection | track_reader_process | Good connection && unecessary params | initialize: ",
        reader.initialize(), true);
    }
    // 4-  Good connection, wrong db_session -> Should fail
    {
      track_reader_process reader("reader");
      config_block blk = reader.params();
      blk.set("disabled", "false");
      blk.set("db_type", "sqlite3");
      blk.set("db_user", "");
      blk.set("db_name", db_path);
      blk.set("db_pass", "");
      blk.set("db_host", "");
      blk.set("db_port", "");
      blk.set("db_session_id", -8);
      blk.set("filename", "");

      TEST("sqlite3 connection | track_reader_process | Good connection  && wrong db_session | set_params: ",
        reader.set_params(blk), true);
      TEST("sqlite3 connection | track_reader_process | Good connection && wrong db_session | initialize: ",
        reader.initialize(), false);
    }
  }
}

void test_compare_tracks(vidtk::track::vector_t& ref_tracks,
                         const vidtk::track::vector_t& tracks)
{
  vidtk::track::vector_t::const_iterator ref_trackIt = ref_tracks.begin();
  for (; ref_trackIt != ref_tracks.end(); ++ref_trackIt)
  {
    unsigned int reference_id = (*ref_trackIt)->id();
    vidtk::track::vector_t::const_iterator trackIt = tracks.begin();
    for (; trackIt != tracks.end(); ++trackIt)
    {
      if ((*trackIt)->id() == reference_id)
      {
        break;
      }
    }
    if (trackIt == tracks.end())
    {
      // Setup this test so it would fail because we did not find the right id.
      //testlib_test_begin()
      TEST("Could not find the track with ID: " + reference_id, true, false);
    }
  }
}

void test_sqlite_read(const std::string& db_path,
  vidtk::track::vector_t& ref_tracks, vxl_int_32 db_session_id)
{
  {
    ns_track_reader::track_reader_db reader;

    ns_track_reader::track_reader_options options;
    options.set_db_type("sqlite3");
    options.set_db_user("");
    options.set_db_name(db_path);
    options.set_db_pass("");
    options.set_db_host("");
    options.set_db_port("");
    options.set_db_session_id(db_session_id);
    reader.update_options(options);
    TEST("sqlite3 read | track_reader_db | Good connection: ",
      reader.open(""), true);

    vidtk::track::vector_t tracks;
    reader.read_all(tracks);
    TEST_EQUAL("sqlite3 read | track_reader_db | track size: ",
      static_cast<unsigned int>(tracks.size()), 110);
    test_compare_tracks(ref_tracks, tracks);
  }

  {
    track_reader_process reader("reader");
    config_block blk = reader.params();
    blk.set("disabled", "false");
    blk.set("db_type", "sqlite3");
    blk.set("db_user", "");
    blk.set("db_name", db_path);
    blk.set("db_pass", "");
    blk.set("db_host", "");
    blk.set("db_port", "");
    blk.set("db_session_id", db_session_id);
    blk.set("filename", "");
    reader.set_batch_mode(true);

    TEST("sqlite3 read | track_reader_process | set_params: ",
      reader.set_params(blk), true);
    TEST("sqlite3 read | track_reader_process | initialize: ",
      reader.initialize(), true);
    TEST("sqlite3 read | track_writer_process | step2: ",
      reader.step2(), process::SUCCESS);
    TEST_EQUAL("sqlite3 read | track_writer_process | step2: ",
      static_cast<unsigned int>(reader.tracks().size()), 110);
    test_compare_tracks(ref_tracks, reader.tracks());
  }
}

void test_read_sqlite(
  const std::string& db_path,
  const std::string track_filename,
  vxl_int_32 db_session_id)
{
  // Get the tracks:
  vidtk::track::vector_t ref_tracks;
  {
    vidtk::track_reader reader(track_filename.c_str());
    TEST("Opening vsl file", reader.open(), true);

    reader.read_all(ref_tracks);
  }

  // 1- test connection
  test_sqlite_connection(db_path, db_session_id);
  test_sqlite_read(db_path, ref_tracks, db_session_id);
}

void test_postgresql_connection(vxl_int_32 db_session_id)
{
  // /!\ Do not change the database here !

  ns_track_reader::track_reader_options correct_options;
  correct_options.set_db_type("postgresql");
  correct_options.set_db_user("perseas");
  correct_options.set_db_name("perseas");
  correct_options.set_db_pass("");
  correct_options.set_db_host("maximegalon");
  correct_options.set_db_port("12350");
  correct_options.set_db_session_id(db_session_id);

  // Test connection with track reader
  {
    // 1-  Correct connection -> Should work
    {
      ns_track_reader::track_reader_db reader;
      reader.update_options(correct_options);
      TEST("PostgreSQL connection | track_reader_db | Good connection: ",
        reader.open(""), true);
      TEST("PostgreSQL connection | track_reader_db | Good connection, 2nd attempt: ",
        reader.open(""), false);
    }
    // 2-  Correct db_name, wrong db_type -> Should fail
    {
      ns_track_reader::track_reader_db reader;
      ns_track_reader::track_reader_options wrong_db_type_options = correct_options;
      wrong_db_type_options.set_db_type("sqlite3");
      reader.update_options(wrong_db_type_options);
      TEST("PostgreSQL connection | track_db_reader | wrong db_type: ",
        reader.open(""), false);
      TEST("PostgreSQL connection | track_db_reader | wrong db_type, 2nd attempt: ",
        reader.open(""), false);
    }
    // 3-  Correct connection, wrong name
    {
      ns_track_reader::track_reader_db reader;
      ns_track_reader::track_reader_options wrong_db_name_options = correct_options;
      wrong_db_name_options.set_db_user("wrong_name");
      reader.update_options(wrong_db_name_options);
      TEST("PostgreSQL connection | track_db_reader | wrong db_name: ",
        reader.open(""), false);
      TEST("PostgreSQL connection | track_db_reader | wrong db_name, 2nd attempt: ",
        reader.open(""), false);
    }
    // 4-  Correct connection, wrong user
    {
      ns_track_reader::track_reader_db reader;
      ns_track_reader::track_reader_options wrong_db_user_options = correct_options;
      wrong_db_user_options.set_db_user("wrong_user");
      reader.update_options(wrong_db_user_options);
      TEST("PostgreSQL connection | track_db_reader | wrong db_user: ",
        reader.open(""), false);
      TEST("PostgreSQL connection | track_db_reader | wrong db_user, 2nd attempt: ",
        reader.open(""), false);
    }
    // 5-  Correct connection, wrong host
    {
      ns_track_reader::track_reader_db reader;
      ns_track_reader::track_reader_options wrong_db_host_options = correct_options;
      wrong_db_host_options.set_db_host("wrong_host");
      reader.update_options(wrong_db_host_options);
      TEST("PostgreSQL connection | track_db_reader | wrong db_host: ",
        reader.open(""), false);
      TEST("PostgreSQL connection | track_db_reader | wrong db_host, 2nd attempt: ",
        reader.open(""), false);
    }
    // 6-  Correct connection, wrong port
    {
      ns_track_reader::track_reader_db reader;
      ns_track_reader::track_reader_options wrong_db_port_options = correct_options;
      wrong_db_port_options.set_db_port("wrong_port");
      reader.update_options(wrong_db_port_options);
      TEST("PostgreSQL connection | track_db_reader | wrong db_port: ",
        reader.open(""), false);
      TEST("PostgreSQL connection | track_db_reader | wrong db_port, 2nd attempt: ",
        reader.open(""), false);
    }
    // 7-  Correct connection, wrong session_id
    {
      ns_track_reader::track_reader_db reader;
      ns_track_reader::track_reader_options wrong_session_id_options = correct_options;
      wrong_session_id_options.set_db_session_id(-8);
      reader.update_options(wrong_session_id_options);
      TEST("PostgreSQL connection | track_reader_db | wrong session_id: ",
        reader.open(""), false);
      TEST("PostgreSQL connection | track_reader_db | wrong session_id, 2nd attempt: ",
        reader.open(""), false);
    }
  }

    // Test connection with track_reader_process
  {
    // 1-  Correct connection -> Should Work
    {
      track_reader_process reader("reader");
      config_block blk = reader.params();
      blk.set("disabled", "false");
      blk.set("db_type", correct_options.get_db_type());
      blk.set("db_user", correct_options.get_db_user());
      blk.set("db_pass", correct_options.get_db_pass());
      blk.set("db_host", correct_options.get_db_host());
      blk.set("db_port", correct_options.get_db_port());
      blk.set("db_session_id", correct_options.get_db_session_id());
      blk.set("filename", "");

      TEST("PostgreSQL connection | track_reader_process | Good connection | set_params: ",
        reader.set_params(blk), true);
      TEST("PostgreSQL connection | track_reader_process | Good connection | initialize: ",
        reader.initialize(), true);
    }
    // 2-  Bad connection -> Should fail
    {
      track_reader_process reader("reader");
      config_block blk = reader.params();
      blk.set("disabled", "false");
      blk.set("db_type", "sqlite3");
      blk.set("db_user", correct_options.get_db_user());
      blk.set("db_pass", correct_options.get_db_pass());
      blk.set("db_host", correct_options.get_db_host());
      blk.set("db_port", correct_options.get_db_port());
      blk.set("db_session_id", correct_options.get_db_session_id());
      blk.set("filename", "");

      TEST("PostgreSQL connection | track_reader_process | Bad connection | set_params: ",
        reader.set_params(blk), true);
      TEST("PostgreSQL connection | track_reader_process | Bad connection | initialize: ",
        reader.initialize(), false);
    }
  }
}

void test_read_postgresql(vxl_int_32 db_session_id)
{
  // 1- test connection
  test_postgresql_connection(db_session_id);
}

vxl_int_32 get_sqlite_clean_session_id(std::string db_path)
{
  db_connection_sptr connection;
  db_connection_factory::get_connection(
    "sqlite3",
    db_path,
    "", "", "", "", "",
    connection);

  connection->connect();
  std::vector<db_session_sptr> sessions;
  connection->get_session_db()->get_all_session_ids(sessions);
  vxl_int_32 max_session_id = -1;
  for (size_t i = 0; i < sessions.size(); ++i)
  {
    vxl_int_32 current_session_id = sessions[i]->get_session_meta_id();
    max_session_id = current_session_id > max_session_id ?
    current_session_id : max_session_id;
  }

  return max_session_id + 1;
}

vxl_int_32 get_postgresql_first_session_id()
{
  db_connection_sptr connection;
  db_connection_factory::get_connection(
    "postgresql",
    "perseas",
    "perseas",
    "",
    "maximegalon",
    "12350",
    "",
    connection);

  connection->connect();
  std::vector<db_session_sptr> sessions;
  connection->get_session_db()->get_all_session_ids(sessions);
  return (sessions.front())->get_session_meta_id();
}

} // end namespace

int test_reader_db(int argc, char *argv[])
{
  if ( argc < 2)
  {
    LOG_ERROR( "Missing required argument" );
    return EXIT_FAILURE;
  }

  std::string data_dir = argv[1];

  // SQLite
  test_read_sqlite(
    data_dir + "/default_vidtk_db.spatialite",
    data_dir + "/lair_test_tracks_with_ll.vsl",
    get_sqlite_clean_session_id(data_dir + "/default_vidtk_db.spatialite")
    );

  // Postgresql
  test_read_postgresql(
    get_postgresql_first_session_id()
    );

  return testlib_test_summary();
}

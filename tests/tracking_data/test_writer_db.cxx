/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <tracking_data/io/track_reader.h>
#include <tracking_data/io/track_writer_db.h>
#include <tracking_data/io/track_writer_process.h>

#include <logger/logger.h>
VIDTK_LOGGER( "test_writer_dbcxx" );

using namespace vidtk;

namespace
{

void copy_file(const std::string& src, const std::string dst)
{
  /// Copy the database file to a temp file, so if something goes wrong, we
  /// don't mess up the original file.
  std::ifstream  src_stream(src.c_str(), std::ios::binary);
  std::ofstream  dst_stream(dst.c_str(), std::ios::binary);
  dst_stream << src_stream.rdbuf();
  dst_stream.close();
}

void test_sqlite_connection( std::string db_path )
{
  // Test connection with track writer
  // /!\ Do not change the database here !
  {
    // 1-  Correct connection -> Should work
    {
      vidtk::track_writer_db writer;

      track_writer_options options;
      options.db_type_ = "sqlite3";
      options.db_user_ = "";
      options.db_pass_ = "";
      options.db_host_ = "";
      options.db_port_ = "";
      writer.set_options(options);
      TEST("sqlite3 connection | track_db_writer | Good connection: ",
        writer.open(db_path), true);
      TEST("sqlite3 connection | track_db_writer | Good connection, 2nd attempt: ",
        writer.open(db_path), false);
      writer.close();
    }

    // 2-  Correct db_name, wrong db_type -> Should fail
    {
      vidtk::track_writer_db writer;

      track_writer_options options;
      options.db_type_ = "postgresql";
      options.db_user_ = "";
      options.db_pass_ = "";
      options.db_host_ = "";
      options.db_port_ = "";
      writer.set_options(options);
      TEST("sqlite3 connection | track_db_writer | Bad connection: ",
        writer.open(db_path), false);
      TEST("sqlite3 connection | track_db_writer | Good connection, 2nd attempt: ",
        writer.open(db_path), false);
      writer.close();
    }

    // 3-  Correct db_name, unecessary parameters -> Should work
    {
      vidtk::track_writer_db writer;

      track_writer_options options;
      options.db_type_ = "sqlite3";
      options.db_user_ = "TotallyAuthorizedUser";
      options.db_pass_ = "not_my_pet_name";
      options.db_host_ = "ServerInTheGarage";
      options.db_port_ = "starboard";
      writer.set_options(options);
      TEST("sqlite3 connection | track_db_writer | Good connectionn && unecessary params: ",
        writer.open(db_path), true);
      TEST("sqlite3 connection | track_db_writer | Good connectionn && unecessary params, 2nd attempt: ",
        writer.open(db_path), false);
      writer.close();
    }
  }

  // Test connection with track_writer_process
  {
    // 1-  Correct connection -> Should Work
    {
      track_writer_process writer("writer");
      config_block blk = writer.params();
      blk.set("disabled", "false");
      blk.set("overwrite_existing", "true");
      blk.set("db_type", "sqlite3");
      blk.set("db_user", "");
      blk.set("db_pass", "");
      blk.set("db_host", "");
      blk.set("db_port", "");
      blk.set("filename", db_path);

      TEST("sqlite3 connection | track_writer_process | Good connection | set_params: ",
        writer.set_params(blk), true);
      TEST("sqlite3 connection | track_writer_process | Good connection | initialize: ",
        writer.initialize(), true);
    }
    // 2-  Bad connection -> Should fail
    {
      track_writer_process writer("writer");
      config_block blk = writer.params();
      blk.set("disabled", "false");
      blk.set("overwrite_existing", "true");
      blk.set("db_type", "postgresql");
      blk.set("db_user", "");
      blk.set("db_pass", "");
      blk.set("db_host", "");
      blk.set("db_port", "");
      blk.set("filename", db_path);

      TEST("sqlite3 connection | track_writer_process | Bad connection | set_params: ",
        writer.set_params(blk), true);
      TEST("sqlite3 connection | track_writer_process | Bad connection | initialize: ",
        writer.initialize(), false);
    }
    // 3-  Good connection, unecessary parameters -> Should work
    {
      track_writer_process writer("writer");
      config_block blk = writer.params();
      blk.set("disabled", "false");
      blk.set("overwrite_existing", "true");
      blk.set("db_type", "sqlite3");
      blk.set("db_user", "PleaseLetMeIn");
      blk.set("db_pass", "qwerty");
      blk.set("db_host", "me");
      blk.set("db_port", "wine");
      blk.set("filename", db_path);

      TEST("sqlite3 connection | track_writer_process | Good connection  && unecessary params | set_params: ",
        writer.set_params(blk), true);
      TEST("sqlite3 connection | track_writer_process | Good connection && unecessary params | initialize: ",
        writer.initialize(), true);
    }
  }
}

void test_sqlite_writer_db(std::string db_path, vidtk::track::vector_t& tracks_all)
{
  // 1-  track_writer_db -> Should work
  {
    vidtk::track_writer_db writer;

    track_writer_options options;
    options.db_type_ = "sqlite3";
    options.db_user_ = "";
    options.db_pass_ = "";
    options.db_host_ = "";
    options.db_port_ = "";
    writer.set_options(options);
    TEST("sqlite3 write | track_db_writer | open: ", writer.open(db_path), true);
    TEST("sqlite3 write | track_db_writer | write(track_all): ", writer.write(tracks_all), true);

    // Make sure all the tracks have been written correctly
    db_connection_sptr db_conn = writer.get_db_conn();
    TEST("sqlite3 write | track_db_writer | db_conn is open: ",
      db_conn->is_connection_open(), true);

    track_db_sptr track_db = db_conn->get_track_db();
    TEST("sqlite3 write | track_db_writer | track_db is valid: ", (track_db.ptr() != NULL), true);


    vidtk::track::vector_t db_tracks;
    TEST("sqlite3 write | track_db_writer | track_db.get_all_tracks(): ",
      track_db->get_all_tracks(db_tracks), true);
    TEST_EQUAL("sqlite3 write | track_db_writer | db_track.size() == tracks_all.size(): ",
      static_cast<unsigned int>(db_tracks.size()),
      static_cast<unsigned int>(tracks_all.size()));

    // Make sure all the tracks of database are in the db_tracks
    vidtk::track::vector_t::const_iterator trackIt = tracks_all.begin();
    for (; trackIt != tracks_all.end(); ++trackIt)
    {
      unsigned int reference_id = (*trackIt)->id();
      vidtk::track::vector_t::const_iterator db_trackIt = db_tracks.begin();
      for (; db_trackIt != db_tracks.end(); ++db_trackIt)
      {
        if ((*db_trackIt)->id() == reference_id)
        {
          break;
        }
      }
      if (db_trackIt == db_tracks.end())
      {
        // Setup this test so it would fail because we did not find the right id.
        //testlib_test_begin()
        TEST("Could not find the track with ID: " + reference_id, true, false);
      }
    }

    writer.close();
  }
}

void test_sqlite_writer_process_db(std::string db_path, vidtk::track::vector_t& tracks_all)
{
  // 1-  track_writer_process -> Should work
  {
    track_writer_process writer("writer");
    config_block blk = writer.params();
    blk.set("disabled", "false");
    blk.set("overwrite_existing", "true");
    blk.set("db_type", "sqlite3");
    blk.set("db_user", "");
    blk.set("db_pass", "");
    blk.set("db_host", "");
    blk.set("db_port", "");
    blk.set("filename", db_path);

    TEST("sqlite3 connection | track_writer_process | set_params: ",
      writer.set_params(blk), true);
    TEST("sqlite3 connection | track_writer_process | initialize: ",
      writer.initialize(), true);
    writer.set_tracks(tracks_all);
    TEST("sqlite3 connection | track_writer_process | step2: ",
      writer.step2(), process::SUCCESS);
  }
}

void test_sqlite_writing(std::string db_path, std::string data_dir)
{
  // Get the tracks:
  vidtk::track::vector_t tracks_all;
  {
    std::string vslPath = data_dir + "/lair_test_tracks_with_ll.vsl";
    vidtk::track::vector_t tracks;

    vidtk::track_reader reader(vslPath.c_str());
    TEST("Opening vsl file", reader.open(), true);

    reader.read_all(tracks_all);
  }

  std::string writer_db = "./temp_sqlite_writer_db.db";
  copy_file(db_path, writer_db);
  test_sqlite_writer_db(writer_db, tracks_all);
  std::remove(writer_db.c_str());

  std::string writer_process_db = "./temp_sqlite_writer_process_db.db";
  copy_file(db_path, writer_process_db);
  test_sqlite_writer_process_db(writer_process_db, tracks_all);
  std::remove(writer_process_db.c_str());
}

void test_postgresql_connection()
{
  track_writer_options correct_options;
  correct_options.db_type_ = "postgresql";
  correct_options.db_user_ = "perseas";
  correct_options.db_name_ = "perseas";
  correct_options.db_pass_ = "";
  correct_options.db_host_ = "maximegalon";
  correct_options.db_port_ = "12350";
  //correct_options.aoi_id_ = "1"; // No aoi_id test for now.

  // Test connection with track writer
  {

    // 1-  Correct connection
    {
      vidtk::track_writer_db writer;
      writer.set_options(correct_options);
      TEST("PostgreSQL connection | track_db_writer | Good connection: ",
        writer.open(""), true);
      TEST("PostgreSQL connection | track_db_writer | Good connection, 2nd attempt: ",
        writer.open(""), false);
      writer.close();
    }
    // 2-  Correct connection, wrong db_type
    {
    vidtk::track_writer_db writer;
    track_writer_options wrong_db_type_options = correct_options;
    wrong_db_type_options.db_type_ = "sqlite3";
    writer.set_options(wrong_db_type_options);
    TEST("PostgreSQL connection | track_db_writer | wrong db_type: ",
    writer.open(""), false);
    TEST("PostgreSQL connection | track_db_writer | Good connection, 2nd attempt: ",
    writer.open(""), false);
    writer.close();
    }
    // 3-  Correct connection, wrong name
    {
    vidtk::track_writer_db writer;
    track_writer_options wrong_name_options = correct_options;
    wrong_name_options.db_name_ = "wrong_name";
    writer.set_options(wrong_name_options);
    TEST("PostgreSQL connection | track_db_writer | Bad db_name: ",
      writer.open(""), false);
    writer.close();
    }
    // 4-  Correct connection, wrong user
    {
    vidtk::track_writer_db writer;
    track_writer_options wrong_user_options = correct_options;
    wrong_user_options.db_user_ = "wrong_user";
    writer.set_options(wrong_user_options);
    TEST("PostgreSQL connection | track_db_writer | Bad db_user: ",
    writer.open(""), false);
    writer.close();
    }
    // 5-  Correct connection, wrong host
    {
    vidtk::track_writer_db writer;
    track_writer_options wrong_host_options = correct_options;
    wrong_host_options.db_host_ = "wrong_host";
    writer.set_options(wrong_host_options);
    TEST("PostgreSQL connection | track_db_writer | Bad db_host: ",
    writer.open(""), false);
    writer.close();
    }
    // 6-  Correct connection, wrong port
    {
    vidtk::track_writer_db writer;
    track_writer_options wrong_port_options = correct_options;
    wrong_port_options.db_port_ = "0000";
    writer.set_options(wrong_port_options);
    TEST("PostgreSQL connection | track_db_writer | Bad db_port: ",
    writer.open(""), false);
    writer.close();
    }
  }

  // track writer process postgresql connection
  {
    // 1-  Good connection
    {
      track_writer_process writer("writer");
      config_block blk = writer.params();
      blk.set("disabled", "false");
      blk.set("overwrite_existing", "true");
      blk.set("db_type", "postgresql");
      blk.set("db_user", "perseas");
      blk.set("db_pass", "");
      blk.set("db_host", "maximegalon");
      blk.set("db_port", "12350");
      blk.set("format", "db");
      blk.set("filename", "");

      TEST("PostgreSQL connection | track_writer_process | Good connection | set_params: ",
        writer.set_params(blk), true);
      TEST("PostgreSQL connection | track_writer_process | Good connection | initialize: ",
        writer.initialize(), true);
    }
  }
}

} // end namespace

int test_writer_db(int argc, char *argv[])
{
  if ( argc < 2)
  {
    LOG_ERROR( "Missing required argument: spatialite database" );
    return EXIT_FAILURE;
  }

  /// \todo Add testing for the aoi_id

  /// First run the db connection tests with SQLite
  test_sqlite_connection(argv[1]);
  test_sqlite_writing(argv[1], argv[2]);

  /// Then run the db connection tests with Postegresql
  test_postgresql_connection();

  return testlib_test_summary();
}

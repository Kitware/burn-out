/*ckwg +5
* Copyright 2010- 2011 by Kitware, Inc. All Rights Reserved. Please refer to
* KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
* Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
*/


#include <tracking/kw18_reader.h>
#include <tracking/tracking_keys.h>
#include <testlib/testlib_test.h>

using namespace vidtk;

int test_track(int argc, char* argv[])
{
  testlib_test_start("test vidtk track data structure");
  if ( argc < 2 )
  {

    TEST("DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  //Load in kw18 track with pixel information
  vcl_string dataPath(argv[1]);
  dataPath += "/vidtk_track_test.kw18";
  vcl_string imagePath(argv[1]);
  imagePath += "/score_tracks_data/test_frames";
  vcl_vector< vidtk::track_sptr > tracks;
  vidtk::kw18_reader* reader = new vidtk::kw18_reader( dataPath.c_str() );
  reader->set_path_to_images(imagePath);

  //Test the deep copy of the track
  TEST("Read kw18 file", reader->read(tracks), true);
  TEST("There is 1 Track", tracks.size(), 1);
  vidtk::track_sptr track_orig = tracks[0];
  vidtk::track_sptr track_copy = track_orig->clone();

  vcl_cout << "Testing Deep Copy: " << vcl_endl;
  vcl_cout << "Pointers are differnt for:" << vcl_endl;
  TEST("track pointers", track_orig == track_copy, false);

  vcl_vector< track_state_sptr > orig_history = track_orig->history();
  vcl_vector< track_state_sptr > copy_history = track_copy->history();
  TEST("history pointers", orig_history ==  copy_history, false);

  track_state_sptr orig_state = orig_history[0];
  track_state_sptr copy_state = copy_history[0];
  TEST("state pointers", orig_state == copy_state, false);

  vcl_vector< vidtk::image_object_sptr > orig_objs, copy_objs;

  orig_state->data_.key_map().begin();
  orig_state->data_.get(vidtk::tracking_keys::img_objs, orig_objs);
  copy_state->data_.get(vidtk::tracking_keys::img_objs, copy_objs);

  TEST("object pointers", orig_objs == copy_objs, false);

  vil_image_view< vxl_byte > orig_pxl_data;
  vil_image_view< vxl_byte > copy_pxl_data;
  orig_objs[0]->data_.get(tracking_keys::pixel_data, orig_pxl_data);
  copy_objs[0]->data_.get(tracking_keys::pixel_data, copy_pxl_data);
  TEST("pxl data pointers", orig_pxl_data.top_left_ptr() == copy_pxl_data.top_left_ptr(), false);

  //Make sure the data is the same but the pointers are different
  bool data_equal = true;
  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < 10; j++)
    {
      for (int p = 0; p < 3; p++)
      {
        if ( orig_pxl_data(i, j, p) != copy_pxl_data(i, j, p) )
        {
          data_equal = false;
        }
      }
    }
  }
  TEST("pxl data values", data_equal, true);

  unsigned int orig_pxl_data_buff = 0;
  unsigned int copy_pxl_data_buff = 1;

  orig_objs[0]->data_.get(tracking_keys::pixel_data_buffer, orig_pxl_data_buff);
  copy_objs[0]->data_.get(tracking_keys::pixel_data_buffer, copy_pxl_data_buff);
  TEST("pxl buffer", orig_pxl_data_buff == copy_pxl_data_buff, true);

  delete reader;
  return testlib_test_summary();
} /* test_track */



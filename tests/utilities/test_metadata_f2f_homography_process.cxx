/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <iomanip>
#include <testlib/testlib_test.h>
#include <utilities/metadata_f2f_homography_process.h>
#include <utilities/video_metadata_util.h>

#include <vil/vil_image_view.h>

using namespace vidtk;

namespace //anon
{

std::string META_DATA[] = {
    "1256157138146 444 444 0 0 0 0 0 0 0 "
    "39.785857850005371006 -84.100535666192243411 "
    "39.785857475525553184 -84.096862203685418535 "
    "39.781291508076066066 -84.096862213843294853 "
    "39.781291882545168903 -84.100535676337230484 "
    "0 0 0 "
    "39.783573343260144384 -84.098697844090594344 ",

    "1256157138946 444 444 0 0 0 0 0 0 0 "
    "39.785858692790476709 -84.100536362816853853 "
    "39.785858317874769341 -84.096861164562966451 "
    "39.781289650089163956 -84.096861175260713139 "
    "39.781290024994142129 -84.100536373501043386 "
    "0 0 0 "
    "39.783573343260144384 -84.098697844090594344 ",

    "1256157140546 444 444 0 0 0 0 0 0 0 "
    "39.785856046984164891 -84.100535350923635747 "
    "39.785855666622502724 -84.096865404739020278 "
    "39.781293586286068376 -84.096865417037193424 "
    "39.781293966636852133 -84.100535363206248007 "
    "0 0 0 "
    "39.783573343260144384 -84.098697844090594344 ",

    "1256157141546 444 444 0 0 0 0 0 0 0 "
    "39.785856046984164891 -84.100535350923635747 "
    "39.785856046984164891 -84.100535350923635747 "
    "39.785856046984164891 -84.100535350923635747 "
    "39.785856046984164891 -84.100535350923635747 "
    "0 0 0 "
    "39.783573343260144384 -84.098697844090594344 "
};

void
test_homog_valid( vnl_double_3x3 H,
                  double h00, double h01, double h02,
                  double h10, double h11, double h12,
                  double h20, double h21, double h22 )
{
  vnl_double_3x3 trueH;
  trueH(0,0) = h00;  trueH(0,1) = h01;  trueH(0,2) = h02;
  trueH(1,0) = h10;  trueH(1,1) = h11;  trueH(1,2) = h12;
  trueH(2,0) = h20;  trueH(2,1) = h21;  trueH(2,2) = h22;
  TEST_NEAR( "Test homography", (H/H(2,2) - trueH/trueH(2,2)).fro_norm(),
             0, 1e-6 );
}


void test_ll()
{
  metadata_f2f_homography_process mf2fhp("test_ll");
  config_block blk = mf2fhp.params();
  blk.set("disabled", "false");
  blk.set("use_latlon_for_xfm", "true");
  TEST("Can set config ll", mf2fhp.set_params(blk), true);
  TEST("Can init ll", mf2fhp.initialize(), true);

  video_metadata vm;
  {
    std::stringstream test_meta(META_DATA[0]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  }
  mf2fhp.set_metadata(vm);
  vil_image_view<vxl_byte> img(1427, 1774);
  mf2fhp.set_image(img);

  TEST("Can step ll", mf2fhp.step(), true);
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                    1, 0, 0,   0, 1, 0,   0, 0, 1);
  {
    std::stringstream test_meta(META_DATA[1]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  }
  mf2fhp.set_metadata(vm);
  TEST("Can step ll", mf2fhp.step(), true);
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                    0.999519223571515, 1.16617833342399e-07, 0.270293183624744,
                    -7.04355193275663e-08, 0.999400455563183, 0.32703610509634,
                    8.64745091876553e-16, -9.84490164665989e-14, 0.99999150620738 );
  vm = video_metadata();
  mf2fhp.set_metadata(vm);
  TEST("Can step ll", mf2fhp.step(), true);
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                    1, 0, 0,   0, 1, 0,   0, 0, 1);
  std::vector<video_metadata> vms;
  {
    std::stringstream test_meta(META_DATA[2]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
    vms.push_back(vm);
    vms.push_back(video_metadata());
  }
  mf2fhp.set_metadata_v(vms);
  TEST("Can step ll (vms)", mf2fhp.step(), true);
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                    1.00143202161106, 3.54186418974594e-07, -0.393185928463936,
                    -1.63258391692157e-06, 1.00144487579841, -1.02822234295309,
                    -2.89873022910761e-14, -3.15928857195577e-13, 1.00000091781449 );
  std::cout << std::setprecision(15) << mf2fhp.h_prev_2_cur() << std::endl;

  {
    std::stringstream test_meta(META_DATA[3]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  }

  mf2fhp.set_metadata(vm);
  TEST("Can failed step ll(1)", mf2fhp.step(), true);
}

void test_utm()
{
  metadata_f2f_homography_process mf2fhp("test_utm");
  config_block blk = mf2fhp.params();
  blk.set("disabled", "false");
  blk.set("use_latlon_for_xfm", "false");
  TEST("Can set config utm", mf2fhp.set_params(blk), true);
  TEST("Can init utm", mf2fhp.initialize(), true);

  video_metadata vm;
  {
    std::stringstream test_meta(META_DATA[0]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  }
  mf2fhp.set_metadata(vm);
  vil_image_view<vxl_byte> img(1427, 1774);
  mf2fhp.set_image(img);

  TEST("Can step utm", mf2fhp.step(), true);
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                    1, 0, 0,   0, 1, 0,   0, 0, 1);
  {
    std::stringstream test_meta(META_DATA[1]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  }
  mf2fhp.set_metadata(vm);
  TEST("Can step utm", mf2fhp.step(), true);
  std::cout << std::setprecision(15) << mf2fhp.h_prev_2_cur() << std::endl;
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                    0.99952030636432376287, 1.1706377218070472719e-07, 0.27029439810826261237,
                    -6.8963805910887150048e-08, 0.99940155292249166497, 0.32706250853323126648,
                    -5.8153135311817992066e-15, -1.0066612404621632393e-13, 0.999992589430990896911 );
  vm = video_metadata();
  mf2fhp.set_metadata(vm);
  TEST("Can step utm", mf2fhp.step(), true);
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                    1, 0, 0,   0, 1, 0,   0, 0, 1);
  std::vector<video_metadata> vms;
  {
    std::stringstream test_meta(META_DATA[2]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
    vms.push_back(vm);
    vms.push_back(video_metadata());
  }
  mf2fhp.set_metadata_v(vms);
  TEST("Can step utm (vms)", mf2fhp.step(), true);
  test_homog_valid( mf2fhp.h_prev_2_cur().get_matrix(),
                     1.001431912376318456,       3.536180218690510729e-07, -0.39318832643664336501,
                    -1.6466437514146927538e-06,  1.0014447486569675316,    -1.0282968578075042387,
                    -1.4013425665273958177e-13, -3.172267268890409881e-13,  1.0000008097333048518 );
  std::cout << std::setprecision(15) << mf2fhp.h_prev_2_cur() << std::endl;

  {
    std::stringstream test_meta(META_DATA[3]);
    TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  }

  mf2fhp.set_metadata(vm);
  TEST("Bad corner points ll(2)", mf2fhp.step(), true);
}

void test_bad_config()
{
  metadata_f2f_homography_process mf2fhp("test_bad_config");
  config_block blk = mf2fhp.params();
  blk.set("disabled", "BOB");
  TEST("Cannot set config disabled", mf2fhp.set_params(blk), false);
}

void test_disable()
{
  metadata_f2f_homography_process mf2fhp("test_disable");
  config_block blk = mf2fhp.params();
  blk.set("disabled", "true");
  TEST("Can set config disabled", mf2fhp.set_params(blk), true);
  TEST("Can init disabled", mf2fhp.initialize(), true);
  TEST("Can step disabled", mf2fhp.step(), true);
}

}//namespace anon

int test_metadata_f2f_homography_process( int, char*[] )
{
  testlib_test_start( "test metadata f2f homog process" );

  test_ll();
  test_utm();
  test_bad_config();
  test_disable();

  return testlib_test_summary();
}

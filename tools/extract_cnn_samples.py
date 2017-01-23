#!python

import sys
import os
import glob
import numpy as np
import random
import cv2
import argparse
import h5py
import random
import math

from scipy.io import loadmat

from matplotlib import pyplot as plt
from sklearn.utils import shuffle
from idl_utils import idl_reader

from PIL import Image

max_pos_count_per_image = 20001

# Is the region outside image bounds?
def is_outside_bounds( ll, ur, imgdims, dist=0 ):

  if ll[0] < dist or ll[1] < dist:
    return True

  if ur[0] >= imgdims[0]-dist or ur[1] >= imgdims[1]-dist:
    return True

  return False


# Does the region contain any truth samples?
def contains_any_point( ll, ur, track_data ):

  for point in track_data:
    if point[0] <= ur[0] and point[0] >= ll[0] and point[1] <= ur[1] and point[1] >= ll[1]:
      return True

  return False


# Helper class to list files with a given extension in a directory
def list_files_in_dir( folder, extension ):
  return glob.glob( folder + '/*' + extension )


# Create a directory if it doesn't exist
def create_dir( dirname ):

  if not os.path.exists( dirname ):
    os.makedirs( dirname )

# For pixel groundtruth is this a color we ignore
def is_ignore_color( color ):

  if color[0] == 255 and color[1] == 0 and color[2] == 0:
    return True

  if color[0] == 0 and color[1] == 0 and color[2] == 255:
    return True

  if color[0] == 255 and color[1] == 255 and color[2] == 0:
    return True

  if color[0] == 255 and color[1] == 0 and color[2] == 255:
    return True

  if color[0] == 0 and color[1] == 255 and color[2] == 255:
    return True

  return False

# Main Function
if __name__ == "__main__" :

  parser = argparse.ArgumentParser(description="Extract pos and neg target image chips for training/validation",
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)

  parser.add_argument("-i", dest="root_folder", default=None, required=True,
                      help="Input folder with training sequences, each folder containing target ASCII files "
                      "and image source (SAV file or image sequence).")

  parser.add_argument("-i2", dest="root_folder2", default="",
                      help="Optional secondary input folder.")

  parser.add_argument("-o", dest="output_folder", default=".",
                      help="Output folder where training samples will be placed.")

  parser.add_argument("-s", dest="img_source", default="sav",
                      help="Where to get the source images from? Available choices are "
                      "sav: extract 'imgs' sequence from the IDL .sav file; "
                      "img: sequence of images; "
                      "mat: matlab files; "
                      "fld: gt folder with image annotations ")

  parser.add_argument("-s2", dest="img_source2", default="sav",
                      help="Optional secondary input source.")

  parser.add_argument("-ts", dest="trk_source", default="txt",
                      help="Where to get the source images from? Available choices are either "
                      "txt or kw18 when img_source is img, and only txt for all other modes.")

  parser.add_argument("-of", dest="output_format", nargs='*', default="h5",
                      help="Output format, can be h5, img, npy, or all.")

  parser.add_argument("-n", dest="negative_samples", default="10000", type=int,
                      help="Number of negative samples to be extracted across the input data.")

  parser.add_argument("-d", dest="chip_dimension", default="16", type=int,
                      help="Width and height (in pixels) of image chips to be extracted.")

  parser.add_argument("-b", dest="batch_size", default="25600", type=int,
                      help="Batch size, number of samples per output file for h5 files")

  parser.add_argument("-f", dest="frames_per_sample", default="1", type=int,
                      help="Number of frames in each extracted sample.")

  parser.add_argument("-t", dest="intensity_threshold", default="9999999", type=float,
                      help="If the center pixel on the center frame of a positive data chip has "
                      "an intensity higher than this threshold, it will not be included as a "
                      "positive sample.")

  parser.add_argument("-mo", dest="mask_offset", default="8", type=int,
                      help="Mask offset of frame IDs for training on false positives, if available")

  parser.add_argument("-tp", dest="transpose_prob", default="0.0", type=float,
                      help="[0,1] threshold of likeliness to use transpose augmentation")

  parser.add_argument("-norm", dest="norm_method", default="none",
                      help="Can either be none, hist, or mstd_pf")

  parser.add_argument("-scale", dest="scale", default="1.0", type=float,
                      help="Whether or not to scale input images by a fixed amount")

  parser.add_argument("-negative_shift", dest="negative_shift", default="0.0", type=float,
                      help="Whether or not to offset input images by a fixed amount")

  parser.add_argument("-int_shifts", dest="int_shifts", nargs='*', default="",
                      help="Optional intensity scale factor augmentation levels")

  parser.add_argument("-int_flux", dest="int_flux", nargs='*', default="",
                      help="Optional intensity random shift factor augmentation levels")

  parser.add_argument("--extend", dest="extend", action="store_true",
                      help="Whether or not to persist h5 training samples already in output folder")

  parser.add_argument("--negatives_only", dest="negatives_only", action="store_true",
                      help="Whether or not to histogram equalize input images")

  parser.add_argument("--3d-conv", dest="conv_3d", action="store_true",
                      help="Generate output data in a format better suited for 3D convolution")

  parser.add_argument("--depth-last", dest="depth_last", action="store_true",
                      help="Makes depth be the last channel instead of the first")

  parser.add_argument("--depth-last-np", dest="depth_last_np", action="store_true",
                      help="Makes depth be the last channel instead of the first")

  parser.set_defaults( negatives_only=False )
  parser.set_defaults( extend=False )
  parser.set_defaults( conv_3d=False )
  parser.set_defaults( depth_last=False )
  parser.set_defaults( depth_last_np=False )

  args = parser.parse_args()

  # Static constant, don't change
  missing_entry_id = 0

  # Check track source
  if args.trk_source != 'txt' and args.trk_source != 'kw18':
    print "TRK_SOURCE can only be txt or kw18"
    sys.exit()

  # Common folder and variable management
  positive_dir = os.path.join( args.output_folder, 'positive' )
  negative_dir = os.path.join( args.output_folder, 'negative' )

  if not os.path.exists( args.output_folder ):
    create_dir( args.output_folder )

  create_dir( args.output_folder + '/verification' )

  output_h5 = "h5" in args.output_format or "all" in args.output_format
  output_npy = "npy" in args.output_format or "all" in args.output_format
  output_img = "img" in args.output_format or "all" in args.output_format

  #for item in args.output_format:
  #  if item != "h5" and item != "img" and item != "npy" and item != "all":
  #    print( "Invalid output format " + item )
  #    sys.exit()

  # Process an input folder
  def process_folder( input_folder, img_source ):

    if img_source != 'sav' and img_source != 'seq1' and img_source != 'mat' \
       and img_source != 'img' and img_source != 'imgs' and img_source != 'fld':
      print "IMG_SOURCE is invalid."
      sys.exit()

    use_pixel_gt = ( img_source == 'fld' )

    samples = []
    labels = []

    frame_border = int( args.frames_per_sample / 2 )

    if not os.path.exists(input_folder) or len( os.listdir( input_folder ) ) == 0:
      print 'Invalid input folder: ' + input_folder
      sys.exit()
    else:
      negative_samples_per_dir = args.negative_samples / len( os.listdir( input_folder ) )

    for dirname in os.listdir( input_folder ):

      positive_sample_count = 0

      gt_img_count = 0

      print 'Processing ' + dirname
      full_path = os.path.join( input_folder, dirname )

      mask_dir = os.path.join( full_path, 'masks' )
      use_mask = os.path.exists( mask_dir )

      print( "Using mask flag: " + str( use_mask ) )

      track_filenames = list_files_in_dir( full_path, args.trk_source )

      if len( track_filenames ) == 0 and img_source != 'fld':
        print 'No track files found in directory: ' + dirname
        continue

      if img_source == 'sav' or img_source == 'mat':

        path_to_scan = img_source

        if img_source == 'mat':
          path_to_scan = 'Frames.mat'

        sav_filenames = list_files_in_dir( full_path, path_to_scan )

        if len( sav_filenames ) != 1:
          print 'There must be a single sav/mat file in directory: ' + dirname
          sys.exit()

        reader = idl_reader( sav_filenames[0], False, False )
        reader.read_file()
        raw_imgs = reader.get_sim_output_images()

      else:
        search_pattern = full_path + '/frames/*.tif'

        if use_pixel_gt:
          search_pattern = full_path + '/*'

        img_filenames = glob.glob( search_pattern )
        gt_filenames = []

        raw_imgs = []

        if use_pixel_gt:
          gt_filenames = [x for x in img_filenames if os.path.basename(x).startswith('gt-')]
          img_filenames = [x for x in img_filenames if not os.path.basename(x).startswith('gt-')]
          img_filenames = [x for x in img_filenames if not os.path.isdir(x)]

        if len( img_filenames ) == 0:
          print 'ERROR: No images found matching pattern: ' + search_pattern
          sys.exit()

        print "Loading images"

        for file_id, i_file in enumerate( sorted( img_filenames ) ):

          im = cv2.imread( i_file, -1 )

          if len( np.shape( im ) ) > 2 and np.shape( im )[2] != 1 and not use_pixel_gt:
            print 'WARN: Image contains multiple planes!'

          raw_imgs.append( im )

        gt_imgs = [ np.zeros( 0 ) ] * len( raw_imgs )

        print "Loading gt images"

        for file_id, i_file in enumerate( sorted( gt_filenames ) ):

          im = cv2.imread( i_file, -1 )

          if len( np.shape( im ) ) > 2 and np.shape( im )[2] != 1 and not use_pixel_gt:
            print 'WARN: Image contains multiple planes!'

          num = int( os.path.basename( i_file )[3:][:-4] )
          gt_imgs[ num-1 ] = im
          gt_img_count = gt_img_count + 1

      # Filter images
      imgs = []

      if args.norm_method == "mstd_pf":

        for i, img in enumerate( raw_imgs ):

          hist,bins = np.histogram( img.flatten(), 65535, [0,65535] )
          total = sum( hist )

          print( "Org: " + str( np.mean( img ) ) + " " + str( np.std( img ) ) )

          count = 0

          avg_sum = 0
          avg_cnt = 0

          for i, amt in enumerate( hist ):
            count = count + amt

            if float( count ) / total < 0.10 or float( count ) / total > 0.90:
              hist[i] = 0

            avg_sum = avg_sum + i * hist[i]
            avg_cnt = avg_cnt + hist[i]

          avg = float( avg_sum ) / avg_cnt

          dev = 0

          for i, amt in enumerate( hist ):

            dev = dev + hist[i] * ( ( i - avg ) * ( i - avg ) )

          dev = math.sqrt( dev / avg_cnt )

          print( "Adj: " + str( avg ) + " " + str( dev ) )

          img = img - avg
          img = img / dev

          imgs.append( img.astype( 'float32' ) )

      elif args.norm_method == "hist":

        for i, img in enumerate( raw_imgs ):
          hist,bins = np.histogram( img.flatten(), 65535, [0,65535] )
          cdf = hist.cumsum()
          cdf_m = np.ma.masked_equal(cdf,0)
          cdf_m = (cdf_m - cdf_m.min())*65535/(cdf_m.max()-cdf_m.min())
          cdf = np.ma.filled(cdf_m,0).astype( 'uint16' )
          img2 = cdf[img]

          imgs.append( img2.astype( 'float32' ) )

      elif args.norm_method == "none":

        for img in raw_imgs:
          imgs.append( img.astype( 'float32' ) )

      else:
        print( "ERROR: Invalid norm method " + args.norm_method )
        sys.exit()

      nimgs = len( imgs )
      nvalid = nimgs - 2 * frame_border

      if negative_samples_per_dir < nvalid:
        print 'Number of negative samples per directory (' + str(negative_samples_per_dir) + ') should be more than the number of usable frames (' + str(nvalid) + ').'
        sys.exit()

      imgs_marked = []
      for i in imgs:
         img = i.copy()
         if img.min() < 0.0:
           img = img - img.min()
         if img.max() != 0:
           scale_factor = 255.0 / img.max()
           img = img * scale_factor
         imgs_marked.append(img.astype('uint8'))

      if use_pixel_gt and gt_img_count != 0:
        nvalid = gt_img_count

      negative_samples_per_image = int( negative_samples_per_dir / nvalid )
      imgdims = np.shape( imgs[0] );

      # Read Track Files
      track_file_count = len( track_filenames )
      track_count = track_file_count

      # Get total track count if using kw18s or pixel classifications
      if args.trk_source == 'kw18':
        for filename in track_filenames:
          for line in open( filename ):
            parsed_line = filter( None, line.split() )

            if len( parsed_line ) < 3:
              continue

            if parsed_line[0] == '#':
              continue

            if len( parsed_line[0] ) > 1 and parsed_line[0][0] == '#':
              continue

            if int( parsed_line[0] ) + 1 > track_count:
              track_count = int( parsed_line[0] ) + 1

      elif use_pixel_gt:
        track_count = max_pos_count_per_image

      # Create container for all tracks
      track_data = missing_entry_id * np.ones( ( nimgs, track_count, 2 ) ).astype( 'int_' )

      # Load track files
      for file_id, filename in enumerate( track_filenames ):
        for line_id, line in enumerate( open( filename ) ):

          parsed_line = filter( None, line.split() )

          if len( parsed_line ) < 3:
            continue

          if parsed_line[0] == '#':
            continue

          if len( parsed_line[0] ) > 1 and parsed_line[0][0] == '#':
            continue

          if img_source == 'sav' :
            track_data[ line_id, file_id, 0 ] = int( ( float( parsed_line[2] ) - 7 ) * 0.5 + 0.5 )
            track_data[ line_id, file_id, 1 ] = int( ( float( parsed_line[1] ) - 7 ) * 0.5 + 0.5 )
          elif img_source == 'mat':
            track_data[ line_id, file_id, 0 ] = int( float( parsed_line[2] ) )
            track_data[ line_id, file_id, 1 ] = int( float( parsed_line[1] ) )
          elif args.trk_source == 'txt':
            track_data[ line_id, file_id, 0 ] = int( float( parsed_line[2] ) )
            track_data[ line_id, file_id, 1 ] = int( float( parsed_line[1] ) )
          else:

            # Format is kw18, handle accordingly
            if len( parsed_line ) < 9:
              print( 'ERROR: kw18 contains error at line ' + str( line_id+1 ) )
              continue

            track_id = int( parsed_line[0] )
            frame_num = int( parsed_line[2] )

            track_data[ frame_num, track_id, 0 ] = int( float( parsed_line[8] ) )
            track_data[ frame_num, track_id, 1 ] = int( float( parsed_line[7] ) )

      if use_pixel_gt:
        for i in range( frame_border, nimgs - frame_border ):

          counter1 = 0
          if np.shape( gt_imgs[i] ) < 2:
            continue

          gt = gt_imgs[i]

          for x in range( np.shape( gt )[ 0 ] ):
            for y in range( np.shape( gt )[ 1 ] ):
              if gt[x,y,0] == 0 and gt[x,y,1] == 255 and gt[x,y,2] == 0:

                track_data[ i, counter1, 0 ] = x
                track_data[ i, counter1, 1 ] = y
                counter1 = counter1 + 1
                if counter1 >= max_pos_count_per_image:
                  counter1 = 0

      # Generate Positive Training Data
      if not args.negatives_only:

        for i in range( frame_border, nimgs - frame_border ):

          found_entry = False

          for j in range( 0, track_count ):

            center = track_data[i,j,:]

            if center[0] == missing_entry_id:
              continue

            found_entry = True

            ll = [ center[0] - args.chip_dimension / 2, center[1] - args.chip_dimension / 2 ]
            ur = [ ll[0] + args.chip_dimension, ll[1] + args.chip_dimension ]

            if is_outside_bounds( ll, ur, imgdims, args.chip_dimension / 2 ) == True:
              continue

            center_intensity = imgs[ i + int( args.frames_per_sample / 2 ) ][ center[0], center[1] ]
            print( 'Positive sample center intensity ' + str( center_intensity ) )

            if not use_pixel_gt and center_intensity > args.intensity_threshold:
              continue

            raw_data = []

            cv2.circle( imgs_marked[ i ], ( center[1],center[0] ), args.chip_dimension/2, (255) )

            for k in range( 0, args.frames_per_sample ):
              cropped_region = imgs[ k + i - frame_border ][ ll[0]:ur[0], ll[1]:ur[1] ]

              if len( np.shape( cropped_region ) ) <= 2 or np.shape( cropped_region )[2] == 1:
                raw_data.append( cropped_region )
              else:
                adapted_crop = np.swapaxes(cropped_region,0,2)
                adapted_crop = np.swapaxes(adapted_crop,1,2)

                for plane in adapted_crop:
                  raw_data.append( plane )

            if args.conv_3d:
              samples.append( np.array( [ raw_data ] ) );
            else:
              samples.append( np.array( raw_data ) );

            positive_sample_count = positive_sample_count + 1

          if not found_entry:
            imgs_marked[ i ] = []

      # Generate Negative Training Data
      for i in range( frame_border, nimgs - frame_border ):

        if use_mask:

          if args.mask_offset + i < 0:
            continue

          mask_fn = "frame_" + str( args.mask_offset + i ).zfill( 5 ) + "_net_map_0.png"
          mask_path = os.path.join( mask_dir, mask_fn )
          mask = cv2.imread( mask_path, 0 )

          if mask is None:
            print( "Unable to read " + mask_fn )
            continue

          print( "Read mask " + mask_fn )

        if use_pixel_gt and len( np.shape( gt_imgs[i] ) ) < 2:
          continue

        counter = 0
        while counter < negative_samples_per_image:

          center = [ random.randint( 0, imgdims[0] ), random.randint( 0, imgdims[1] ) ]

          ll = [ center[0] - args.chip_dimension / 2, center[1] - args.chip_dimension / 2 ]
          ur = [ ll[0] + args.chip_dimension, ll[1] + args.chip_dimension ]

          if is_outside_bounds( ll, ur, imgdims ) == True:
            continue

          if use_mask and mask[ center[0], center[1] ] < 50:
            continue

          if not use_pixel_gt and contains_any_point( ll, ur, track_data[i,:,:] ) == True:
            continue

          if use_pixel_gt and is_ignore_color( gt_imgs[i][center[0],center[1]] ):
            continue

          raw_data = []

          for k in range( 0, args.frames_per_sample ):
            cropped_region = imgs[ k + i - frame_border ][ ll[0]:ur[0], ll[1]:ur[1] ]

            if len( np.shape( cropped_region ) ) <= 2 or np.shape( cropped_region )[2] == 1:
              raw_data.append( cropped_region )
            else:
              adapted_crop = np.swapaxes(cropped_region,0,2)
              adapted_crop = np.swapaxes(adapted_crop,1,2)

              for plane in adapted_crop:
                raw_data.append( plane )

          if args.conv_3d:
            samples.append( np.array( [ raw_data ] ) );
          else:
            samples.append( np.array( raw_data ) );

          counter = counter + 1

      counter = 0
      for i in imgs_marked:
        mf = os.path.join( args.output_folder, "verification/" + dirname + '_' + str( counter ).zfill(6) + '.tif' )
        if len( np.shape( i ) ) > 1:
          cv2.imwrite( mf, i )
        counter = counter + 1

      # Create labels vector
      new_labels = np.zeros( len( samples ) - len( labels ) )
      new_labels[ 0 : positive_sample_count ] = 1

      labels.extend( new_labels )

    return [ samples, labels ]


  # ------------------ Write out HDF5 files -------------------

  # Read all samples from all inputs

  batch_size = args.batch_size
  batch_counter = 0
  sample_counter = 0;

  [ samples, labels ] = process_folder( args.root_folder, args.img_source )

  if len( args.root_folder2 ) > 0:

    [ samples2, labels2 ] = process_folder( args.root_folder2, args.img_source2 )
    samples.extend( samples2 )
    labels = np.concatenate( (labels, labels2), axis=0 )

  # Read in existing data in output folder
  if args.extend:
    for fn in glob.glob( args.output_folder + '/*.h5' ):

      ifile = h5py.File( fn, 'r' )
      data2 = ifile['data'][:]
      labels2 = ifile['label'][:]
      ifile.close()

      samples.extend( data2 )
      labels = np.concatenate( (labels, labels2), axis=0 )

      print( "Loading existing data with: " + str( len( data2 ) ) + " samples." )

  # Perform augmentation
  print( "Performing data augmentation" )

  aug_samples = []
  aug_labels = []

  if args.transpose_prob > 0.0:
    for j, sample in enumerate( samples ):
      if random.random() < args.transpose_prob:
        for k, inner in enumerate( sample ):
          sample[k] = np.transpose( inner )
        samples[j] = sample


  for i, shift in enumerate( args.int_shifts ):

    aug_labels.extend( labels )

    for j, sample in enumerate( samples ):

      if random.random() < args.transpose_prob:
        for k, inner in enumerate( sample ):
          sample[k] = np.transpose( inner )
        samples[j] = sample

      aug_sample = sample * ( float( shift ) + ( random.uniform( 0.0, 1.0 ) * float( args.int_flux[i] ) ) )

      if random.random() < args.transpose_prob:
        for k, inner in enumerate( aug_sample ):
          aug_sample[k] = np.transpose( inner )

      aug_samples.append( aug_sample )

  samples.extend( aug_samples )
  labels = np.concatenate( (labels, aug_labels), axis=0 )

  for ind, sample in enumerate( samples ):
    samples[ind] = ( args.scale * sample ) - args.negative_shift

  # Randomize inputs
  labels, samples = shuffle( labels, samples )

  # Change channel order if set and output samples
  if args.depth_last:
    samples = np.swapaxes( samples, 1, 3 )
    samples = np.swapaxes( samples, 1, 2 )

  if output_img:

    print( "Writing out image files" )

    for i, sample in enumerate( samples ):

        if labels[i] == 1:
          sample_dir = os.path.join( positive_dir, 'sample' + str( i+1 ).zfill(6) + '/' )
        else:
          sample_dir = os.path.join( negative_dir, 'sample' + str( i+1 ).zfill(6) + '/' )

        create_dir( sample_dir )

        for plane in sample:
          cv2.imwrite( sample_dir + "/frame" + str( k+1 ).zfill( 3 ) + ".tif", plane )

  if output_h5:

    print( "Writing out h5 files" )

    while sample_counter < len( samples ):

      batch_counter = batch_counter + 1
      filename = os.path.join( args.output_folder, "samples-" ) + str( batch_counter ).zfill( 4 ) + ".h5"

      lower_bound = ( batch_counter-1 ) * batch_size
      upper_bound = min( batch_counter * batch_size, len( samples ) )

      fout = h5py.File( filename, 'w' )

      fout[ 'data' ] = samples[ lower_bound : upper_bound ]
      fout[ 'label' ] = labels[ lower_bound : upper_bound ]

      fout.close()

      batch_count = batch_counter + 1
      sample_counter = sample_counter + ( upper_bound - lower_bound )

  if args.depth_last_np and not args.depth_last:
    samples = np.swapaxes( samples, 1, 3 )
    samples = np.swapaxes( samples, 1, 2 )

  if output_npy:

    print( "Writing out numpy files" )

    np.save( args.output_folder + '/training-data.npy', samples )
    np.save( args.output_folder + '/training-labels.npy', labels )


  print('         0_')
  print('          \\`.     ___')
  print('           \\ \\   / __>0')
  print('       /\\  /  |/\' / ')
  print('      /  \\/   `  ,`\'--.')
  print('     / /(___________)_ \\')
  print('     |/ //.-.   .-.\\\\ \\ \\')
  print('     0 // :@ ___ @: \\\\ \\/')
  print('       ( o ^(___)^ o ) 0')
  print('        \\ \\_______/ /')
  print('    /\\   \'._______.\'--.')
  print('    \\ /|  |<_____>    |')
  print('     \\ \\__|<_____>____/|__')
  print('      \\____<_____>_______/')
  print('          |<_____>    |')
  print('          |<_____>    |')
  print('          :<_____>____:')
  print('         / <_____>   /|')
  print('        /  <_____>  / |')
  print('       /___________/  |')
  print('       |           | _|__')
  print('       |           | ---||_')
  print('       |   |L\\/|/  |  | [__]')
  print('       |  \\|||\\|\\  |  /')
  print('       |           | /')
  print('       |___________|/')
  print('')
  print('    ~~ !!! Success !!! ~~')
  print('')

/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_string.h>
#include <vcl_vector.h>
#include <vcl_map.h>
#include <vcl_cstdlib.h>
#include <vcl_cstdio.h>
#include <vcl_iostream.h>
#include <vcl_fstream.h>
#include <vcl_sstream.h>
#include <vcl_utility.h>

#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_image_view.h>
#include <vcl_algorithm.h>

#include <vul/vul_arg.h>

// some reserved cluster labels
const unsigned LABEL_NULL_DATA = 0; // i.e. pixels off the homography plane
const unsigned LABEL_LOW_DATA = 1;  // i.e. not enough traffic to generate a model

typedef vil_image_view< vxl_uint_16 > ImageType;

// label index -> frequency  [ aka a histogram ]
typedef vcl_map< unsigned, unsigned > LabelCountType;
typedef vcl_map< unsigned, unsigned >::iterator LCIt;
typedef vcl_map< unsigned, unsigned >::const_iterator LCCIt;

// cell index / class label / whatever -> histogram of (label, frequency)
typedef vcl_map< unsigned, LabelCountType > IndexLabelHistogramType;
typedef vcl_map< unsigned, LabelCountType >::iterator IndexLabelHistogramIt;
typedef vcl_map< unsigned, LabelCountType >::const_iterator IndexLabelHistogramCIt;

struct GTIndexType {
  // ordinal index -> class label
  vcl_vector< vcl_string > classList;

  // unique image region label index -> class label index
  LabelCountType indexToClassMap;

  // unique label of the "NONE" class and "NOMATCH" class
  unsigned noneLabel;
  unsigned noMatchLabel;
};

ImageType LoadImageOrFail( const vcl_string& fn, const vcl_string& msg );
unsigned MaxHistogramKey( const LabelCountType& hist );
GTIndexType LoadGTClassMap( const vcl_string& fn );
ImageType LabelCellImageAsGT( const ImageType& cellImage,
                              const ImageType& gtIndexImage,
                              const GTIndexType& gtIndex );
IndexLabelHistogramType LabelImageHistogram( const ImageType& keyImage,
                                             const ImageType& valueImage,
                                             bool filterClusterLabels );
IndexLabelHistogramType MergeImageHistograms( const IndexLabelHistogramType& keyHistogram,
                                              const IndexLabelHistogramType& valueHistogram,
                                              bool filterClusterLabels = true );

void debugDumpImage( const ImageType& image );
void debugDumpImageIndex( const IndexLabelHistogramType& ilht );

int main( int argc, char *argv[] )
{
  vul_arg< vcl_string > clusterImageFilename( "-c", "cluster index image" );
  vul_arg< vcl_string > cellIndexImageFilename( "-i", "cell index image" );
  vul_arg< vcl_string > gtIndexImageFilename( "-gt", "ground-truth index image" );
  vul_arg< vcl_string > gtClassMapFilename( "-gtcm", "ground-truth class map" );
  vul_arg< vcl_string > outputClassImage( "-out", "output cell classification result image" );
  vul_arg< vcl_string > dumpIndexGT( "-dumpgti", "file to hold cell index -> ground truth labels" );

  vul_arg_parse( argc, argv );

  if ( ! ( clusterImageFilename.set() &&
           cellIndexImageFilename.set() &&
           gtIndexImageFilename.set() &&
           gtClassMapFilename.set() ))
  {
    vul_arg_base::display_usage_and_exit( "All of -c, -i, -gt, and -gtcm must be set" );
  }

  // load 'em up
  ImageType clusterImage = LoadImageOrFail( clusterImageFilename(), "cluster index image" );
  ImageType cellIndexImage = LoadImageOrFail( cellIndexImageFilename(), "cell index image" );
  ImageType gtIndexImage = LoadImageOrFail( gtIndexImageFilename(), "ground-truth index image" );
  GTIndexType gtClassMap = LoadGTClassMap( gtClassMapFilename() );


  /*
  vcl_cerr << "Cluster:\n";
  debugDumpImage( clusterImage );

  vcl_cerr << "Cell index:\n";
  debugDumpImage( cellIndexImage );

  vcl_cerr << "gtIndexImage:\n";
  debugDumpImage( gtIndexImage );
  */

  // Establish ground truth, class-level labeling of the cell index
  // cellClassImage(i,j) is an index for gtClassMap.ClassList
  ImageType cellClassImage = LabelCellImageAsGT( cellIndexImage, gtIndexImage, gtClassMap );

  /*
  vcl_cerr << "cell class\n";
  debugDumpImage( cellClassImage );
  */

  // for each cell index, build a histogram of cluster indices associated with that index
  IndexLabelHistogramType cellIndex2ClusterHist = LabelImageHistogram( cellIndexImage, clusterImage, /*filter =*/ true );

  // for each cell index, build a histogram of class labels associated with that index
  IndexLabelHistogramType cellIndex2ClassHist = LabelImageHistogram( cellIndexImage, cellClassImage, /*filter =*/ false );

  // for each cluster index (not cell index): build a histogram of class labels associated
  // with that cluster
  IndexLabelHistogramType clusterIndex2ClassLabel = LabelImageHistogram( clusterImage, cellClassImage, false );
  vcl_cerr << "clusterIndex\n";
  debugDumpImageIndex( clusterIndex2ClassLabel );
  vcl_cerr << "-----\n";

  if ( dumpIndexGT.set() )
  {
    vcl_ofstream os( dumpIndexGT().c_str() );
    if ( ! os )
    {
      vcl_cerr << "Couldn't write '" << dumpIndexGT() << "'; exiting\n";
      return EXIT_FAILURE;
    }

    for ( IndexLabelHistogramCIt i = cellIndex2ClassHist.begin();
          i != cellIndex2ClassHist.end();
          ++i )
    {
      unsigned gtClassIndex = MaxHistogramKey( i->second );
      os << i->first << " " << gtClassMap.classList[ gtClassIndex ] << "\n";
    }
  }
  /*
  vcl_cerr << "cellIndex2Class:\n";
  debugDumpImageIndex( cellIndex2ClassHist );
  */

  // So, raster scanning the cluster image biases in favor of larger
  // regions in the near field because they have more pixels.
  // Instead, we observe that the cell class image and the cell
  // cluster image are exactly aligned, and iterate over them, giving
  // each cell (not each pixel) one vote regarding which cluster
  // should represent the cell class.
#undef rgcv_raster_scan
#ifdef rgcv_raster_scan
  // for each class-level label, build a histogram of cluster indices associated with that
  // class label across all cells.
  IndexLabelHistogramType classLabel2ClusterHist = LabelImageHistogram( cellClassImage, clusterImage, /*filter =*/ true );
#else
  IndexLabelHistogramType classLabel2ClusterHist = MergeImageHistograms( cellIndex2ClassHist, cellIndex2ClusterHist );
#endif

  //
  // 

  vcl_cerr << "classLabel2Cluster:\n";
  debugDumpImageIndex( classLabel2ClusterHist );

  // Given a class c, we can get to a single cluster index, assuming the histogram is sorted:
  // classLabel2ClusterHist[c]->second[0]
  //
  // Given a cell index i, we can get to a single cluster index, assuming the histogram is sorted:
  // cellIndex2ClusterHist[i]->second[0]
  //
  // Given a cell index i, we can get to a class index, which represents the most popular GT class
  // in the pixels associated with i:
  // cellIndex2ClassHist[i]->second[0]
  //
  // So... generate a confusion matrix of (GT class, found class)
  // whose sum is the number of cells.  For each cell index i: find x
  // = i->GT, y = i->cell_cluster->cluster_class; increment (x,y).  x
  // and y are ground-truth classes (y can be "other" if the dominant
  // cluster is not associated with a class.


  // clusterIndex -> classIndex
  vcl_map< unsigned, unsigned > cluster2GTClassMap;
  for (IndexLabelHistogramCIt i = classLabel2ClusterHist.begin();
       i != classLabel2ClusterHist.end();
       ++i )
  {
    unsigned classCluster = MaxHistogramKey( i->second );
    vcl_cerr << "Class " << i->first << " has cluster " << classCluster << "\n";
    cluster2GTClassMap[ classCluster ] = i->first;
  }


  vcl_map< vcl_pair<unsigned, unsigned>, unsigned > confusionMatrix; // (x,y) -> count
  typedef vcl_map< vcl_pair< unsigned, unsigned >, unsigned >::iterator CMIt;

  // cellIndex -> (0,1,2,3): nothing, nomatch, wrong, right
  vcl_map< unsigned, unsigned > cellIndexResults;

  for (IndexLabelHistogramCIt i = cellIndex2ClassHist.begin();
       i != cellIndex2ClassHist.end();
       ++i)
  {
    unsigned cellIndex = i->first;

    // get the cluster associated with this cell's ground-truth label
    IndexLabelHistogramCIt gtClass = cellIndex2ClassHist.find( cellIndex );

    if ( gtClass == cellIndex2ClassHist.end() )
    {
      vcl_cerr << "No ground-truth for cell index " << cellIndex << "\n";
      return EXIT_FAILURE;
    }
    unsigned gtClassIndex = MaxHistogramKey( gtClass->second );
    IndexLabelHistogramCIt gtCluster = classLabel2ClusterHist.find( gtClassIndex );
    if (gtCluster == classLabel2ClusterHist.end() )
    {
      vcl_cerr << "No ground-truth clusters for class " << gtClassIndex << "\n";
      return EXIT_FAILURE;
    }
    unsigned gtClusterIndex = MaxHistogramKey( gtCluster->second );

    // get the cluster associated with this cell's pixel-level "computed" label
    IndexLabelHistogramCIt testCluster = cellIndex2ClusterHist.find( cellIndex );

    unsigned testClusterClass = gtClassMap.noneLabel;
    if ( testCluster != cellIndex2ClusterHist.end() )
    {
      unsigned testClusterIndex = MaxHistogramKey( testCluster->second );

      // does the testClusterIndex map to a class?
      LCCIt testClusterProbe = cluster2GTClassMap.find( testClusterIndex );
      testClusterClass = (testClusterProbe == cluster2GTClassMap.end())
        ? gtClassMap.noMatchLabel
        : testClusterProbe->second;

    if (cellIndex == 766)
    {
      vcl_cerr << "Cell index " << cellIndex
               << ": gtClass " << gtClassIndex
               << "; gtCluster " << gtClusterIndex
               << "; testCluster " << testClusterIndex
               << " class " << testClusterClass
               << "\n";
    }
    }


    // record the result for possible graphical output
    if (testClusterClass == gtClassMap.noneLabel)
    {
      cellIndexResults[ cellIndex ] = 0;
    }
    else if (testClusterClass == gtClassMap.noMatchLabel)
    {
      cellIndexResults[ cellIndex ] = 1;
    }
    else
    {
      cellIndexResults[ cellIndex ] = (gtClassIndex == testClusterClass) ? 3 : 2;
    }

    // increment the confusion matrix entry
    vcl_pair< unsigned, unsigned > key( gtClassIndex, testClusterClass );
    CMIt cm = confusionMatrix.find( key );
    if ( cm == confusionMatrix.end() )
    {
      vcl_pair<CMIt, bool> ins = confusionMatrix.insert( vcl_make_pair( key, 0 ) );
      if ( ! ins.second )
      {
        vcl_cerr << "Couldn't insert into confusion matrix?  exiting\n";
        return EXIT_FAILURE;
      }
      cm = ins.first;
    }
    ++cm->second;
  } // ...for all cell indices

  // output:
  // first, the class labels
  unsigned nClasses = gtClassMap.classList.size();
  for (unsigned i=0; i<nClasses; ++i)
  {
    vcl_cout << i << " : " << gtClassMap.classList[i] << "\n";
  }
  vcl_cout << "\n";

  // next, the matrix
  for (unsigned i=0; i<nClasses; ++i)
  {
    for (unsigned j=0; j<nClasses; ++j)
    {
      vcl_pair< unsigned, unsigned > key( i, j );
      CMIt cm = confusionMatrix.find( key );
      unsigned v = (cm == confusionMatrix.end()) ? 0 : cm->second;
      vcl_printf( "%4d ", v );
    }
    vcl_printf( "\n" );
  }

#if 0
  // matrix in latex format

  vcl_vector<vcl_string> labelOrder;
  labelOrder.push_back("road");
  labelOrder.push_back("parking");
  labelOrder.push_back("sidewalk");
  labelOrder.push_back("doorway");
  labelOrder.push_back("NOMATCH");
  vcl_vector<unsigned> latexIndices;
  for (unsigned i=0; i<labelOrder.size(); i++)
  {
    bool found = false;
    for (unsigned j=0; (!found) && (j<gtClassMap.classList.size()); j++)
    {
      if (gtClassMap.classList[j] == labelOrder[i])
      {
        latexIndices.push_back(j);
        found = true;
      }
    }
    if ( ! found )
    {
      vcl_cerr << "Found no latex entry for '" << labelOrder[i] << "'; exiting\n";
      vcl_exit( EXIT_FAILURE );
    }
  }

  vcl_cout << "\hline\n\n";
#endif

  // optionally output a cell classification image: blue for nothing,
  // red for wrong, green for right
  unsigned rgbRed[] = {255,0,0};
  unsigned rgbGreen[] = {0,255,0};
  unsigned rgbBlue[] = {0,0,255};
  unsigned rgbYellow[] = {255,255,0};
  if ( outputClassImage.set() )
  {
    vil_image_view<vxl_byte> outputImage( clusterImage.ni(), clusterImage.nj(), 3 );
    for (unsigned i=0; i<outputImage.ni(); ++i)
    {
      for (unsigned j=0; j<outputImage.nj(); ++j)
      {
        unsigned cellIndex = cellIndexImage( i, j );
        unsigned* colorTable = 0;
        switch (cellIndexResults[cellIndex])
        {
        case 0: colorTable = rgbBlue; break;
        case 1: colorTable = rgbYellow; break;
        case 2: colorTable = rgbRed; break;
        case 3: colorTable = rgbGreen; break;
        default:
          vcl_cerr << "Bad output color " << cellIndexResults[cellIndex]
                   << " for " << cellIndex << " at " << i << ", " << j << "\n";
          vcl_exit( EXIT_FAILURE );
        }
        for (unsigned p=0; p<3; p++)
        {
          outputImage( i, j, p ) = colorTable[p];
        }
      }
    }

    vil_save( outputImage, outputClassImage().c_str() );
  }

  // all done
}

//
// subs
//

ImageType
LoadImageOrFail( const vcl_string& fn, const vcl_string& msg )
{
  ImageType img = vil_load( fn.c_str() );
  if ( ! img )
  {
    vcl_cerr << msg << ": couldn't load '" << fn << "'; trying as 8-bit...\n";
    vil_image_view<vxl_byte> img8 = vil_load( fn.c_str() );
    if ( ! img8 )
    {
      vcl_cerr << "...couldn't load as 8-bit, either; exiting\n";
      vcl_exit( EXIT_FAILURE );
    }
    vcl_cerr << "...8-bit load succeeded; converting to 16-bit\n";
    img.set_size( img8.ni(), img8.nj(), img8.nplanes() );
    for (unsigned i=0; i<img8.ni(); ++i)
    {
      for (unsigned j=0; j<img8.nj(); ++j)
      {
        for (unsigned p=0; p<img8.nplanes(); ++p)
        {
          img( i, j, p ) = static_cast< vxl_uint_16 >( img8( i, j, p ));
        }
      }
    }
  }
  return img;
}


// class map contains lines such as:
// classname  n
// classname  m n
//
// ... where m and n are region indices.  Parse this into a dictionary
// of classes and map of region-indices to dictionary entries.
//
// 'NONE' and 'NOMATCH' are reserved.
//

GTIndexType
LoadGTClassMap( const vcl_string& fn )
{
  vcl_ifstream is( fn.c_str() );
  if ( ! is )
  {
    vcl_cerr << "Couldn't open ground-truth class map '" << fn << "'; exiting\n";
    vcl_exit( EXIT_FAILURE );
  }

  vcl_map< unsigned, vcl_string > index2ClassMap;
  vcl_map< vcl_string, unsigned > classnameMap;

  vcl_string tmp;
  unsigned max_n = 0;
  while ( getline( is, tmp ).good() )
  {
    vcl_string classname;
    unsigned n, m;
    vcl_istringstream iss( tmp );
    if ( iss >> classname >> m )
    {
      if ((classname == "NONE") || (classname == "NOMATCH"))
      {
        vcl_cerr << "'" << classname << "' is a reserved class name; exiting\n";
        vcl_exit( EXIT_FAILURE );
      }

      if ( iss >> n )  // line contained "classname m n"
      {
        // do nothing
      }
      else // line contained "classname m", which is the same as "classname m m"
      {
        n = m;
      }
      if (n > max_n)
      {
        max_n = n;
      }
      for (unsigned i=m; i<=n; ++i)
      {
        classnameMap[ classname ] = 0;
        vcl_pair< vcl_map<unsigned, vcl_string>::iterator, bool > ins =
          index2ClassMap.insert( vcl_make_pair( i, classname ));
        if ( ! ins.second )
        {
          vcl_cerr << "GTI insert of " << i << ", " << classname << " failed (duplicate " << i << "?); exiting\n";
          vcl_exit( EXIT_FAILURE );
        }
      }
    }
    else
    {
      vcl_cerr << "Couldn't parse '" << tmp << "'; exiting\n";
      vcl_exit( EXIT_FAILURE );
    }
  }

  // insert NONE and NOMATCH classes
  classnameMap[ "NOMATCH" ] = 0;
  index2ClassMap.insert( vcl_make_pair( max_n+1, "NOMATCH" ));
  classnameMap[ "NONE" ] = 0;
  index2ClassMap.insert( vcl_make_pair( max_n+2, "NONE" ));

  // assemble the ground truth index object
  GTIndexType gti;

  // (1) assign indices to the class names
  for ( vcl_map< vcl_string, unsigned >::iterator i = classnameMap.begin();
        i != classnameMap.end();
        ++i )
  {
    gti.classList.push_back( i->first );
    i->second = gti.classList.size() - 1;
    if (i->first == "NONE")
    {
      gti.noneLabel = i->second;
    }
    if (i->first == "NOMATCH")
    {
      gti.noMatchLabel = i->second;
    }
  }

  // (2) invert the index2class map, converting class to string-indices
  for ( vcl_map< unsigned, vcl_string >::iterator i = index2ClassMap.begin();
        i != index2ClassMap.end();
        ++i )
  {
    // get the string index
    vcl_map< vcl_string, unsigned >::const_iterator s = classnameMap.find( i->second );
    if (s == classnameMap.end())
    {
      vcl_cerr << "Lost class name '" << i->second << "'?\n";
      vcl_exit( EXIT_FAILURE );
    }
    unsigned stringIndex = s->second;

    // get the region index
    unsigned regionIndex = i->first;
    vcl_pair< LCIt, bool> ins = gti.indexToClassMap.insert( vcl_make_pair( regionIndex, stringIndex ));
    if ( ! ins.second )
    {
      vcl_cerr << "Insert into gti for " << regionIndex << " failed? exiting\n";
      vcl_exit( EXIT_FAILURE );
    }
  }

  // all done
  return gti;
}

unsigned
MaxHistogramKey( const LabelCountType& hist )
{
  unsigned maxVotes = 0;
  unsigned maxKey = 0;
  for (LCCIt ballotLine = hist.begin(); ballotLine != hist.end(); ++ballotLine)
  {
    if (ballotLine->second > maxVotes)
    {
      maxKey = ballotLine->first;
      maxVotes = ballotLine->second;
    }
  }
  return maxKey;
}


//
// cellImage is an image of cell indices; gtIndexImage is an image of
// ground truth region indices.  Use the gtIndex object to convert each
// cell into its corresponding most-general ground truth class index.
//
ImageType LabelCellImageAsGT( const ImageType& cellImage,
                              const ImageType& gtIndexImage,
                              const GTIndexType& gti)
{
  if ( (cellImage.ni() != gtIndexImage.ni()) ||
       (cellImage.nj() != gtIndexImage.nj()) )
  {
    vcl_cerr << "Cell image and GT index image incommensurate; exiting\n";
    vcl_exit( EXIT_FAILURE );
  }

  ImageType img( cellImage.ni(), cellImage.nj(), 1 );

  for (unsigned i=0; i < cellImage.ni(); ++i)
  {
    for (unsigned j=0; j < cellImage.nj(); ++j)
    {
      // what's the pixel's class index?
      unsigned gtIndex = gtIndexImage( i, j );
      LCCIt i2c = gti.indexToClassMap.find( gtIndex );
      if (i2c == gti.indexToClassMap.end())
      {
        vcl_cerr << "No index->class entry for " << gtIndex << " at "
                 << i << ", " << j << "; exiting\n";
        vcl_exit( EXIT_FAILURE );
      }
      unsigned gtClassIndex = i2c->second;

      img( i, j ) = gtClassIndex;
    }
  }

  // all done
  return img;
}


// for the range of keys in keyImage, return a histogram of the
// values in the valueImage
IndexLabelHistogramType LabelImageHistogram( const ImageType& keyImage,
                                             const ImageType& valueImage,
                                             bool filterClusterLabels )
{
  if ( (keyImage.ni() != valueImage.ni()) ||
       (keyImage.nj() != valueImage.nj()) )
  {
    vcl_cerr << "Key image and value image incommensurate; exiting\n";
    vcl_exit( EXIT_FAILURE );
  }

  IndexLabelHistogramType r;
  for (unsigned i=0; i<keyImage.ni(); ++i)
  {
    for (unsigned j=0; j<keyImage.nj(); ++j)
    {
      unsigned value = valueImage( i, j );
      if ( (filterClusterLabels) &&
           ((value == LABEL_NULL_DATA) || (value == LABEL_LOW_DATA)))
      {
        continue;
      }

      unsigned key = keyImage( i, j );
      IndexLabelHistogramIt probe = r.find( key );
      if (probe == r.end())
      {
        vcl_pair< IndexLabelHistogramIt, bool > ins = r.insert( vcl_make_pair( key, LabelCountType() ));
        if ( ! ins.second )
        {
          vcl_cerr << "Couldn't insert histogram vote? exiting\n";
          vcl_exit( EXIT_FAILURE );
        }
        probe = ins.first;
      }
      LCIt histProbe = probe->second.find( value );
      if (histProbe == probe->second.end())
      {
        vcl_pair< LCIt, bool > ins = probe->second.insert( vcl_make_pair( value, 0 ));
        if ( ! ins.second )
        {
          vcl_cerr << "Couldn't insert histogram ballot line? exiting\n";
          vcl_exit( EXIT_FAILURE );
        }
        histProbe = ins.first;
      }
      ++histProbe->second;
    }
  }
  return r;
}




// instead of raster-scanning the pixels, merge two histograms:
// assume key is a map of cellIndex -> [classLabelHistogram],
// and value is a map of cellIndex -> [clusterIndexHistogram].
// Factor out the cellIndex: for each cell index, generate pair
// (classLabel, clusterIndex), use classLabel as key, and increment
// the clusterIndex as the value in the histogram.

IndexLabelHistogramType
MergeImageHistograms( const IndexLabelHistogramType& keyHistogram,
                      const IndexLabelHistogramType& valueHistogram,
                      bool filterClusterLabels )
{
  IndexLabelHistogramType r;

  for (IndexLabelHistogramCIt keyIt = keyHistogram.begin(); keyIt != keyHistogram.end(); ++keyIt )
  {
    IndexLabelHistogramCIt valIt = valueHistogram.find( keyIt->first );
    if (valIt == valueHistogram.end()) continue;

    unsigned returnVal = MaxHistogramKey( valIt->second );

    if ( (filterClusterLabels) &&
         ((returnVal == LABEL_NULL_DATA) || (returnVal == LABEL_LOW_DATA)))
    {
      continue;
    }

    // get the return-key as the max of the left-hand-side's histogram
    unsigned returnKey = MaxHistogramKey( keyIt->second );

    IndexLabelHistogramIt returnIt = r.find( returnKey );
    if (returnIt == r.end())
    {
      vcl_pair< IndexLabelHistogramIt, bool > ins = r.insert( vcl_make_pair( returnKey, LabelCountType() ));
      if ( ! ins.second )
      {
        vcl_cerr << "Merge failed to create a slot for " << returnKey << "; exiting\n";
        vcl_exit( EXIT_FAILURE );
      }
      returnIt = ins.first;
    }

    // increment the bin for the right-hand-side's max histogram value

    LCIt returnHistIt = returnIt->second.find( returnVal );
    if (returnHistIt == returnIt->second.end())
    {
      vcl_pair< LCIt, bool > ins = returnIt->second.insert( vcl_make_pair( returnVal, 0 ));
      if ( ! ins.second )
      {
        vcl_cerr << "Failed to increment merged bin for " << returnVal << "; exiting\n";
        vcl_exit( EXIT_FAILURE );
      }
      returnHistIt = ins.first;
    }
    ++(returnHistIt->second);
  } // for each key (on the LHS)

  return r;
}


void
debugDumpImage( const ImageType& image )
{
  vcl_cerr << "image: " << image.nj() << " rows; " << image.ni() << " cols\n";
  for (unsigned row = 0; row < image.nj(); ++row)
  {
    for (unsigned col = 0; col < image.ni(); ++col)
    {
      unsigned v = image( col, row );
      vcl_cerr << v << " ";
    }
    vcl_cerr << "\n";
  }
}

void
debugDumpImageIndex( const IndexLabelHistogramType& ilht )
{
  for (IndexLabelHistogramCIt i = ilht.begin(); i != ilht.end(); ++i)
  {
    vcl_cerr << "Index key " << i->first << ": " << i->second.size() << " entries:\n";
    for (LCCIt j = i->second.begin(); j != i->second.end(); ++j)
    {
      vcl_cerr << "  key " << j->first << "; value " << j->second << "\n";
    }
  }
}

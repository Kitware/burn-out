To build:  

1. build Qt 4.5.0 or greater.  

For windows, you can get qt4.5.0 from here:
\\kitwarenas2\Share\Downloads\Windows\Qt4.5 open source\qt-win-opensource-src-4.5.0.zip
To build, run configure.exe from a visual studio command prompt. Then wait...  

2. Put the bin directory from qt build in your PATH.

3. build VTK, or enable VTK_USE_QVTK on an existing build of VTK.  Make sure DESIRED_QT_VERSION is set to 4, and that it picks up the correct qmake, should be in your PATH from 2.

4 configure VIRATGeovis project

- It should find VTK and Qt at this point.

5. build VIRATGeovis



To add buttons and menus:

1. run designer on VIRATGeovis.ui (designer should be in the PATH from 2.)
2. add an action in the action editor
3. connect the action in VIRATGeovis.cxx
// something like this:
 connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
 connect(this->ui->actionPlay, SIGNAL(triggered()), this, SLOT(slotPlay()));

The Play button is the only one hooked up right now.


To run
Run VIRATGeovis.exe <path to NE2_ps_bath_small.jpg or world.topo.bathy.200407.3x10000x5000.jpg or world.topo.bathy.200407.3x5000x2500.jpg

Issues:
Just zooming in and out still seems a bit slow without even any video

Video needs to be implemented

- mouse intereaction, and zoom to rectangle needs to be put back or worked on

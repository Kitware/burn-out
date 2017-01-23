import glob
import os 

print(glob.glob("*.xml"))
for xml in glob.glob("*.xml"):
  os.system("C:/vidtkBuild/bin/Release/convert_track_format.exe --input-tracks \""+xml+" --output-format kw18 --frequency .5")

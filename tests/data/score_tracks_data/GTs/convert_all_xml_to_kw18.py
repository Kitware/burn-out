import glob
import os 

print glob.glob("*.xml")
for xml in glob.glob("*.xml"):
  execute = "C:/vidtkBuild/bin/Release/convert_track_format.exe --input-tracks \""+os.getcwd()+"\\"+xml+"\" --output-tracks \""+os.getcwd()+"\\"+xml[:-3]+"kw18\" --frequency .5"
  print execute 
  os.system(execute)

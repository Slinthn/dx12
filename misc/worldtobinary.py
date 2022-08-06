import sys
from array import array
import json

if len(sys.argv) != 3:
  print("ERROR: Syntax: worldtobinary <input file> <output file>")
  exit()

inputfilename = sys.argv[1]
outputfilename = sys.argv[2]

inputfile = open(inputfilename, "r")
outputfile = open(outputfilename, "wb")

data = json.load(inputfile)

# Header
outputfile.write(b"SW")
outputfile.write(array("i", [len(data["models"])]))
outputfile.write(array("i", [len(data["objects"])]))

# Models
for model in data["models"]:
    outputfile.write(array("h", [model.get("modelid", 0)]))

# Objects
for obj in data["objects"]:
  outputfile.write(array("h", [obj.get("modelid", 0)]))
  outputfile.write(array("h", [obj.get("textureid", 0)]))
  outputfile.write(array("f", obj.get("position", 0)))
  outputfile.write(array("f", obj.get("rotation", 0)))
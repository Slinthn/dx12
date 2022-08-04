import sys
from array import array

if len(sys.argv) != 3:
  print("ERROR: Syntax: plysm <input file> <output file>")
  exit()

inputfilename = sys.argv[1]
outputfilename = sys.argv[2]

inputfile = open(inputfilename, "r")
outputfile = open(outputfilename, "wb")

propertycount = 0
vertexcount = 0
facecount = 0
lines = inputfile.readlines()

outputfile.write(b"S")
outputfile.write(b"M")

i = 0
for i, line in enumerate(lines):
  if line.startswith("property float"):
    propertycount += 1
  elif line.startswith("element vertex"):
    vertexcount = int(line.rsplit(" ", 1)[1])
  elif line.startswith("element face"):
    facecount = int(line.rsplit(" ", 1)[1])
  elif line.startswith("end_header"):
    i += 1
    break

outputfile.write(array("i", [vertexcount, facecount]))

lines = lines[i:]
for i, line in enumerate(lines):
  if i == vertexcount:
    break
  floats = []
  for element in line.split(" "):
    floats.append(float(element))  
  outputfile.write(array("f", floats))


lines = lines[(i):]
for i, line in enumerate(lines):
  if not line.startswith("3"):
    print("ERROR: Can only parse triangles, other shape detected in faces section")
    exit()
  ints = []
  for element in line.split(" ")[1:]:
    ints.append(int(element))
    
  outputfile.write(array("i", ints))

if propertycount != 8:
  print("The number of properties must equal 8 (i.e. x, y, z, nx, ny, nz, s, t)")
  exit()

if vertexcount == 0:
  print("Vertex count must be greater than 0")
  exit()

if facecount == 0:
  print("Face count must be greater than 0")
  exit()

print("compiled SM " + outputfilename)

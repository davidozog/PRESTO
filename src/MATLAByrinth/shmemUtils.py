import sys
PY_MAJOR_VERSION = sys.version_info[0]

if PY_MAJOR_VERSION > 2:
  NULL_CHAR = 0
else:
  NULL_CHAR = '\0'  

def read_from_memory(mapfile, bytes):
  """Reads a string from the mapfile and returns that string"""
  mapfile.seek(0)
  #s = [ ]
  #c = mapfile.read_byte()
  c = mapfile.read(bytes)
  #while c != NULL_CHAR:
  #for i in range(0, bytes):
  #s.append(c)
  #c = mapfile.read_byte()

  if PY_MAJOR_VERSION > 2:
    s = [chr(c) for c in s]
  #s = ''.join(s)

  #return s
  return c

def write_to_memory(mapfile, s):
  """Writes the string s to the mapfile"""
  #say("writing %s " % s)
  mapfile.seek(0)
  # I append a trailing NULL in case I'm communicating with a C program.
  s += '\0'
  if PY_MAJOR_VERSION > 2:
    s = s.encode()
  mapfile.write(s)

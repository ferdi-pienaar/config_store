""" C++ code generator
From XML descriptor, generate .h file(s) containing user data structures
and the .cpp file that implements the initialization of the metadata
describing the contents of those structures.

Usage: python code_gen.py [source XML file]

"""

from xml.dom import minidom
import sys


f = open(sys.argv[1])
print "Reading", sys.argv[1]

xmldoc = minidom.parse(f).documentElement

# We're only interested in the child elements, not the other nodes like text (including
# carriage returns) and comments.
for element in [node for node in xmldoc.childNodes if node.nodeType == minidom.Node.ELEMENT_NODE]:
    print element.toxml()


f.close()

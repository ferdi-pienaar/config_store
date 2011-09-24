""" C++ code generator
From YAML descriptor, generate .h file(s) containing user data structures
and the .cpp file that implements the initialization of the metadata
describing the contents of those structures.

Usage: python yaml_code_gen.py [source YAML file]


"""

import yaml
import sys

indent = "    "

class Id_generator():
    """Generate ID for TLV.  This is the simplest thing that could work, but not the best solution"""
    def __init__(self):
        self.id = 0
        
    def get_id(self):
        id_string = str(self.id)
        self.id += 1
        return id_string
    

class Item:
    def __init__(self, d):
        self.d = d
    
class SimpleItem(Item):
    def __init__(self, d):
        Item.__init__(self, d)
        
    def get_type (self):
        return self.d['type']
        
    def get_definition(self):
        return ""
        
class CompositeItem(Item):
    def __init__(self, d):
        Item.__init__(self, d)
        
    def get_type(self):
        return "t_" + self.d['name']
        
    def get_definition(self):
        """Return string representing definition, including pre-pended definitions of referenced structs"""
        str = ""
        for aggr in self.d['aggregate']:
            item = getItem(aggr['item'])
            str += item.get_definition()
        str += "\nstruct " + self.d['name'] + "\n{\n"
        for aggr in self.d['aggregate']:
            item = getItem(aggr['item'])
            str += indent + item.get_type() + " " + item.d['name'] + ";\n"
        str += "};\n"
        return str
        
        
def getItem(d):
    """Factory method returns an Item object of the right type"""
    if 'aggregate' in d: 
        return CompositeItem(d)
    else:
        return SimpleItem(d)

f = open(sys.argv[1])
finit = open("out.cpp", "w")
fdecl = open("out.h", "w")
print "Reading", sys.argv[1]
data = yaml.load(f)
print "============================================="
print data
print "============================================="

f.close()

try:
    i = data['item']
except:
    print "Root element in", sys.argv[1], "is not an item: exit"
    sys.exit()
    
print i
print

aggr = i['aggregate']
for a in aggr:
   print "++++++++++++", a['item']['name']
   print a
   
print
print
print "~~~~~~~~~~~~~~~~~~~~~~~~"
baseItem = getItem(i)
print baseItem.get_definition()

finit.close()
fdecl.close()

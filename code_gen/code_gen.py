""" C++ code generator
From XML descriptor, generate .h file(s) containing user data structures
and the .cpp file that implements the initialization of the metadata
describing the contents of those structures.

Usage: python code_gen.py [source XML file]

"""

from xml.dom import minidom
import sys

def get_child_elements(parent):
    """Return a list of child nodes that are elements, not text (incl carriage returns) or comments."""
    return [node for node in parent.childNodes if node.nodeType == minidom.Node.ELEMENT_NODE]
    
    
class Id_generator():
    """Generate ID for TLV.  This is the simplest thing that could work, but not the best solution"""
    def __init__(self):
        self.id = 0
        
    def get_id(self):
        id_string = str(self.id)
        self.id += 1
        return id_string
        
        
def get_element_content(element):
    """Get the content of an element, i.e. the first of its nodes of type TEXT_NODE"""
    return [node.data for node in element.childNodes if node.nodeType == minidom.Node.TEXT_NODE][0]
    

class Item():
    """Class representing some properties of an item; we don't need to represent all, because
       minidom's Node does most of the job for us, such as the parent/child relationships."""
       
    def __init__(self):
        self.aggr_list = []
        
       
    def process(self, node, prefix):
        """Process a minidom Node representing an XML element with tag 'item'.
           Return the item's name; it's useful to the aggregate it's in.
        """
        self.prefix = prefix
        print "process_item", node, "PREFIX",prefix
        # For all the elements within the item: name, ID, aggregates(s), etc...
        for element in get_child_elements(node):
            print "TAGNAME", element.tagName
            # print element.toxml()      
            # Assume each item has at least one "name" child element
            if (element.tagName == "name"):
                self.nameText = get_element_content(element)
                
            if (element.tagName == "print-function"):
                prt_fn = get_element_content(element)
                print "prt_fn", prt_fn
                
            if (element.tagName == "aggregate"):
               newPrefix = prefix + self.nameText
               print "NEWPREFIX", newPrefix
               self.aggr_list.append(process_aggregate(element, newPrefix))
                    
        if len(self.aggr_list) > 0:
            desc_init_str = self.get_composite_init_str()
            decl_str = "composite struct\n"

        else:
            # Simple item
            desc_init_str = self.get_simple_init_str()
            decl_str = "component member\n"

        finit.write(desc_init_str)
        fdecl.write(decl_str)
        return self.nameText
                
    def get_simple_init_str(self):
        """Simple"""
        prt_str = "\nconst cm_basic_item_descriptor " + self.prefix + self.nameText + " = cm_basic_item_descriptor\n"
        prt_str += "(\n    \"" + self.nameText + "\",\n"
        prt_str += "    " + id_gen.get_id() + ",\n);\n"
        return prt_str
        
    def get_composite_init_str(self):
        """ Composite item"""
        aggr_list_string = "\n// List of aggregates in composite item " + self.nameText + "\n"
        aggr_list_string += "const cm_aggregate * const " + self.nameText + "aggrList[] = {\n"
        for aggr in self.aggr_list:
            # list of aggregates in this composite item
            aggr_list_string += "    &" + aggr + ",\n"
        aggr_list_string += "};\n"
        prt_str = aggr_list_string
        prt_str += "\nconst cm_composite_item_descriptor " + self.prefix + self.nameText + " = cm_composite_item_descriptor\n"
        prt_str += "(\n    \"" + self.nameText + "\",\n"
        prt_str += "    " + id_gen.get_id() + ",\n);\n"
        return prt_str        


def process_aggregate(aggr, prefix):
    """Process XML element with tag 'aggregate'. Return the aggregate's name; it's useful to the composite item it's in"""
    print "process_aggregate", aggr, prefix
    
    # Assume each aggregate has at least 1 "item" child element
    itemNode = [node for node in get_child_elements(aggr) if node.tagName == "item"][0]
    item = Item()
    item_name = item.process(itemNode, prefix)
    
    aggr_name = item_name + "_aggregate"
    aggr_init = "\nconst cm_"+ "aggregate " + aggr_name + "(" + ");\n"
    finit.write(aggr_init)
    return aggr_name


f = open(sys.argv[1])
finit = open("out.cpp", "w")
fdecl = open("out.h", "w")
print "Reading", sys.argv[1]

xmldoc = minidom.parse(f).documentElement

id_gen = Id_generator()

if (xmldoc.tagName == "item"):
    item = Item()
    item.process(xmldoc, "device")
else:
    print "Root element is", xmldoc.tagName

f.close()
finit.close()
fdecl.close()

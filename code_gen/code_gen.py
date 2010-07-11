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
       minidom's Element does most of the job for us, such as the parent/child relationships.
       """       
    def __init__(self, element, prefix):
        self.element = element
        self.prefix = prefix

        # For all the elements within the item: name, ID, aggregates(s), etc...
        for element in get_child_elements(self.element):
            if (element.tagName == "name"):
                self.nameText = get_element_content(element)
                
            if (element.tagName == "print-function"):
                self.prt_fn = get_element_content(element)            
            
            if (element.tagName == "type"):
                self.type = get_element_content(element)

        
    def get_aggr_list(self):
        """Return the list of child elements with tagName aggregate"""
        return  [element for element in get_child_elements(self.element) if element.tagName == "aggregate"]
        
        
    def print_init_str(self):
        """.
           Return the item's name; it's useful to the aggregate it's in.
        """
        print "print_init_str", self.element, "PREFIX", self.prefix
        aggr_name_list = []
        if len(self.get_aggr_list()) > 0:
            
            finit.write("\n// Start composite")
        else:
            # Simple item
            desc_init_str = self.get_simple_init_str()
        
        for aggrElement in self.get_aggr_list():
           newPrefix = self.prefix + self.nameText
           print "NEWPREFIX", newPrefix
           aggr = Aggregate(aggrElement, newPrefix)
           aggr_name_list.append(aggr.init_str())
                    
        if len(aggr_name_list) > 0:
            desc_init_str = self.get_composite_init_str(aggr_name_list)

        finit.write(desc_init_str)
        return self.nameText

        
    def print_decl_str(self):
        """
        If composite, print the struct, using component aggregates to print
        the members, and return the structure name to the parent aggregate.
        If simple, return the name to parent aggregate.
        """
        print "print_decl_str", self.element, "PREFIX", self.prefix, "NAME", self.nameText
        if len(self.get_aggr_list()) > 0:
            decl_str = "\ntypedef struct\n{\n"
        for aggrElement in self.get_aggr_list():
            newPrefix = self.prefix + self.nameText
            print "NEWPREFIX", newPrefix
            aggr = Aggregate(aggrElement, newPrefix)
            decl_str += "    " + aggr.decl_str() + ";\n"
                    
        if len(self.get_aggr_list()) > 0:
            struct_name = "t_"+self.nameText
            decl_str += "} " + struct_name +";\n"
            fdecl.write(decl_str)
            return struct_name + " " + self.nameText
        else:
            return self.type + " " + self.nameText
                
        
    def get_simple_init_str(self):
        """Simple"""
        prt_str = "\nconst cm_basic_item_descriptor " + self.prefix + self.nameText + " = cm_basic_item_descriptor\n"
        prt_str += "(\n    \"" + self.nameText + "\",\n"
        prt_str += "    sizeof(" + self.type + "),\n"
        prt_str += "    " + id_gen.get_id() + ",\n"
        prt_str += "    " + self.prt_fn + ",\n);\n"
        return prt_str
        
    def get_composite_init_str(self, aggr_name_list):
        """ Composite item"""
        aggr_list_string = "\n// List of aggregates in composite item " + self.nameText + "\n"
        aggr_list_string += "const cm_aggregate * const " + self.nameText + "aggrList[] = {\n"
        for aggr_name in aggr_name_list:
            # list of aggregates in this composite item
            aggr_list_string += "    &" + aggr_name + ",\n"
        aggr_list_string += "};\n"
        prt_str = aggr_list_string
        prt_str += "\nconst cm_composite_item_descriptor " + self.prefix + self.nameText + " = cm_composite_item_descriptor\n"
        prt_str += "(\n    \"" + self.nameText + "\",\n"
        prt_str += "    " + id_gen.get_id() + ",\n);\n"
        return prt_str        


class Aggregate():
    """Class represent XML element with tagName 'aggregate'"""
    def __init__(self, element, prefix):
        self.element = element
        self.prefix = prefix        
        # For all the elements within the aggregate...
        for element in get_child_elements(self.element):
            if (element.tagName == "item"):
                self.item = Item(element, prefix)
                
            if (element.tagName == "count"):
                self.countText = get_element_content(element)            
            
            if (element.tagName == "type"):
                self.type = get_element_content(element)
    
    def init_str(self):
        print "aggregate init_str"
        item_name = self.item.print_init_str()
        aggr_name = item_name + "_aggregate"
        aggr_init = "\nconst cm_"+ "aggregate " + aggr_name + "(" + ");\n"
        finit.write(aggr_init)
        return aggr_name
        
    def decl_str(self):
        print "aggregate decl_str"
        item_name = self.item.print_decl_str()
        if int(self.countText) > 1:
            return item_name + "[" + self.countText + "]"
        else:
            return item_name


f = open(sys.argv[1])
finit = open("out.cpp", "w")
fdecl = open("out.h", "w")
print "Reading", sys.argv[1]
xmldoc = minidom.parse(f).documentElement
f.close()

if (xmldoc.tagName == "item"):
    id_gen = Id_generator()
    item = Item(xmldoc, "device")
    item.print_init_str()
    item.print_decl_str()
else:
    print "Root element is not an item:", xmldoc.tagName

finit.close()
fdecl.close()

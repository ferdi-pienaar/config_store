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


def process_item(item, prefix):
    """Process XML element with tag 'item'"""
    print "process_item", item, "PREFIX",prefix
    aggr_list = []
    # For all the elements within the item: name, ID, aggregates(s), etc...
    for element in get_child_elements(item):
        print "TAGNAME", element.tagName
        # Assume each item has at least one "name" child element
        if (element.tagName == "name"):
            nameText = [node.data for node in element.childNodes if node.nodeType == minidom.Node.TEXT_NODE][0]
            print "NAMETEXT", nameText

        # print element.toxml()
        if (element.tagName == "aggregate"):
           print "NAMETEXT2", nameText
           newPrefix = prefix + nameText
           print "NEWPREFIX", newPrefix
           aggr_list.append(process_aggregate(element, newPrefix))
           
    print "NAMETEXT3", nameText
            
    if len(aggr_list) > 0:
        # Composite item
        for aggr in aggr_list:
            # list of aggregates in this composite item
            fout.write(aggr)
            fout.write(",")
            fout.write("\n")
        desc_init = "const cm_composite_item_descriptor " + prefix + nameText + " = cm_composite_item_descriptor\n"
        desc_init += "(\n\"" + nameText + "\",\n"
    else:
        # Simple item
        desc_init = "const cm_basic_item_descriptor " + prefix + nameText + " = cm_basic_item_descriptor\n"
        desc_init += "(\n\"" + nameText + "\",\n"
    
    desc_init += ");\n"
    fout.write(desc_init)
    return nameText


def process_aggregate(aggr, prefix):
    print "process_aggregate", aggr, prefix
    """Process XML element with tag 'aggregate'"""
    # Assume each aggregate has at least 1 "item" child element
    item = [node for node in get_child_elements(aggr) if node.tagName == "item"][0]
    item_name = process_item(item, prefix)
    
    aggr_name = item_name + "_aggregate"
    aggr_init = aggr_name + ";\n"
    fout.write(aggr_init)
    return aggr_name


f = open(sys.argv[1])
outfname = "out.cpp"
fout = open(outfname, "w")
print "Reading", sys.argv[1]

xmldoc = minidom.parse(f).documentElement

if (xmldoc.tagName == "item"):
    process_item(xmldoc, "device")
else:
    print "Document has", xmldoc, "at top level"

f.close()
fout.close()

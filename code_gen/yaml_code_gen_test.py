"""Unit test for yaml_code_gen.py
"""

import yaml_code_gen
import yaml
import unittest

class Test_Id_generator(unittest.TestCase):
    def test_get_id(self):
        "From empty, ID returned is 0"
        g = yaml_code_gen.Id_generator(None)
        self.assertEqual('0', g.get_id("f"))
    
    def test_add_id(self):
        "New ID is added to the dictionary"
        g = yaml_code_gen.Id_generator(None)
        g.get_id("f")
        self.assertEqual({'f':0}, g.get_ids())
               
    def test_get_id_from_list0(self):
        "New ID is not one that's already in the dictionary"
        i = [
            {'name': 'home', 'id': 0}
        ]
        g = yaml_code_gen.Id_generator(i)
        self.assertEqual('1', g.get_id("x"))

    def test_get_ids_from_list0(self):
        "get_ids returns the list passed to the constructor"
        i = [
            {'name': 'home', 'id': 4}
        ]
        g = yaml_code_gen.Id_generator(i)
        self.assertEqual({'home': 4}, g.get_ids())
        
    def test_get_id_from_list01(self):
        "New ID is not one that's already in the dictionary"
        i = [
            {'name': 'home', 'id': 0},
            {'name': 'sade', 'id': 1}
        ]
        g = yaml_code_gen.Id_generator(i)
        self.assertEqual('2', g.get_id("x"))
        
    def test_get_id_from_list20(self):
        "New ID is the first one that's missing from the dictionary"
        i = [
            {'name': 'home', 'id': 2},
            {'name': 'sade', 'id': 0}
        ]
        g = yaml_code_gen.Id_generator(i)
        self.assertEqual('1', g.get_id("x"))

class Test_simple_gen(unittest.TestCase):
    cfg = \
    ("item:\n"
    "  name: alf\n"
    "  persistent: true\n"
    "  type: int\n")
    
    def test_init_with_id(self):
        "Check C++ init code produced from a YAML config descriptor and YAML ID file"
        id = \
        ("name: alf\n"
        "id: 55\n")
        b = yaml_code_gen.makeBaseItem(self.cfg, id)
        expected_init = ('const cm_simple_metadata alf_data =\n'
        '{\n'
        '    {\n'
        '        "alf",\n'
        '        55,\n'
        '        sizeof(int),\n'
        '        true\n'
        '    },\n'
        '    NULL,\n'
        '    NULL,\n'
        '    NULL,\n'
        '};\n'
        'const cm_simple_descriptor alf(&alf_data);\n')
        self.assertEqual(expected_init, b.get_init())
        
    def test_init_without_id(self):
        "Check C++ init code produced from a YAML config descriptor and YAML ID file"
        b = yaml_code_gen.makeBaseItem(self.cfg, None)
        expected_init = ('const cm_simple_metadata alf_data =\n'
        '{\n'
        '    {\n'
        '        "alf",\n'
        '        0,\n'
        '        sizeof(int),\n'
        '        true\n'
        '    },\n'
        '    NULL,\n'
        '    NULL,\n'
        '    NULL,\n'
        '};\n'
        'const cm_simple_descriptor alf(&alf_data);\n')
        self.assertEqual(expected_init, b.get_init())        
        
class Test_contained_array_gen(unittest.TestCase):
    id = """
    name: alf
    id: 0
    components:
    - name: ipaddr
      id: 0
    """
    cfg = """
    item:
      name: alf
      persistent: true
      aggregate:
      - count: 2
        type: contained
        item:
          name: ipaddr
          persistent: true
          type: unsigned long
    """
    
    def test_contained_array_definition(self):
        b = yaml_code_gen.makeBaseItem(self.cfg, self.id)
        expected_def = ('\n'
        'struct t_alf\n'
        '{\n'
        '    unsigned long ipaddr[2];\n'
        '};\n')
        self.assertEqual(expected_def, b.get_definition())
        
    def test_contained_array_init(self):
        b = yaml_code_gen.makeBaseItem(self.cfg, self.id)
        print b.get_init()
        
if __name__ == "__main__":
    unittest.main()

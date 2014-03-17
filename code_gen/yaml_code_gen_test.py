"""Unit test for yaml_code_gen.py
"""

import yaml_code_gen
import unittest
from pprint import pprint

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

class Test_Item_gen(unittest.TestCase):
        
    def test_simple_init(self):
        c = {'name': 'alf', 'persistent': 'true', 'type': 'int'}
        i = {'name': 'alf', 'id': 0}
        b = yaml_code_gen.makeBaseItem(c, i)
        print b.get_init()
        
if __name__ == "__main__":
    unittest.main()

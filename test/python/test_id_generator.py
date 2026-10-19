"""Unit test for id_generator.py
"""

from yaml_code_gen import Id_generator
import unittest

class Test_Id_generator(unittest.TestCase):
    def test_get_id(self):
        "From empty, ID returned is 0"
        g = Id_generator(None)
        self.assertEqual('0', g.get_id("f"))

    def test_add_id(self):
        "New ID is added to the dictionary"
        g = Id_generator(None)
        g.get_id("f")
        self.assertEqual({'f':0}, g.get_ids())

    def test_get_id_from_list0(self):
        "New ID is not one that's already in the dictionary"
        i = [
            {'name': 'home', 'id': 0}
        ]
        g = Id_generator(i)
        self.assertEqual('1', g.get_id("x"))

    def test_get_ids_from_list0(self):
        "get_ids returns the list passed to the constructor"
        i = [
            {'name': 'home', 'id': 4}
        ]
        g = Id_generator(i)
        self.assertEqual({'home': 4}, g.get_ids())

    def test_get_id_from_list01(self):
        "New ID is not one that's already in the dictionary"
        i = [
            {'name': 'home', 'id': 0},
            {'name': 'sade', 'id': 1}
        ]
        g = Id_generator(i)
        self.assertEqual('2', g.get_id("x"))

    def test_get_id_from_list20(self):
        "New ID is the first one that's missing from the dictionary"
        i = [
            {'name': 'home', 'id': 2},
            {'name': 'sade', 'id': 0}
        ]
        g = Id_generator(i)
        self.assertEqual('1', g.get_id("x"))

if __name__ == "__main__":
    unittest.main()
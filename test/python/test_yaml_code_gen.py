"""Unit test for yaml_code_gen.py
"""

import yaml_code_gen
import yaml
import unittest
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
TEST_CONFIG_DIR = TEST_DIR/"schema_validator_configs"

# Helper function opens file and returns its data
def get_file_data(file_path):
    with open(file_path) as f:
        data = f.read()
    return data

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
        yaml_code_gen.store_type = "tlv"
        b = yaml_code_gen.makeBaseItem("script", self.cfg, id)
        expected_init = ('namespace alf { // Simple item initialization\n'
        'const cfg_mgr::Simple_metadata data =\n'
        '{\n'
        '    {\n'
        '        "alf", // name\n'
        '        55, // id\n'
        '        sizeof(int) // size\n'
        '    },\n'
        '    nullptr, // set\n'
        '    nullptr, // setdef\n'
        '    nullptr, // print\n'
        '};\n'
        'cfg_mgr::Simple_descriptor desc(&data);\n'
        '} // namespace alf, simple item initialization\n')
        self.assertEqual(expected_init, b.get_init())

    def test_init_without_id(self):
        "Check C++ init code produced from a YAML config descriptor and YAML ID file"
        yaml_code_gen.store_type = "tlv"
        b = yaml_code_gen.makeBaseItem("script", self.cfg, None)
        expected_init = ('namespace alf { // Simple item initialization\n'
        'const cfg_mgr::Simple_metadata data =\n'
        '{\n'
        '    {\n'
        '        "alf", // name\n'
        '        0, // id\n'
        '        sizeof(int) // size\n'
        '    },\n'
        '    nullptr, // set\n'
        '    nullptr, // setdef\n'
        '    nullptr, // print\n'
        '};\n'
        'cfg_mgr::Simple_descriptor desc(&data);\n'
        '} // namespace alf, simple item initialization\n')
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
        b = yaml_code_gen.makeBaseItem("script", self.cfg, self.id)
        expected_def = ('struct t_alf\n'
        '{\n'
        '    unsigned long ipaddr[2];\n'
        '};\n')
        self.assertEqual(expected_def, b.get_definition())

    def test_contained_array_init(self):
        b = yaml_code_gen.makeBaseItem("script", self.cfg, self.id)
        #print(b.get_init())

class Test_invalid_config(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        self.id = get_file_data(TEST_CONFIG_DIR/"cfg_id_counter.yaml")

    def test_invalid_owned_counter_ref(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_ref.yaml")
        b = yaml_code_gen.makeBaseItem("script", cfg, self.id)
        self.assertFalse(b.validate())

    def test_invalid_owned_counter_multiple(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_multiple.yaml")
        b = yaml_code_gen.makeBaseItem("script", cfg, self.id)
        self.assertFalse(b.validate())

    def test_invalid_owned_counter_owned(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_owned.yaml")
        b = yaml_code_gen.makeBaseItem("script", cfg, self.id)
        self.assertFalse(b.validate())

    def test_invalid_owned_counter_setdef(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_setdef.yaml")
        b = yaml_code_gen.makeBaseItem("script", cfg, self.id)
        self.assertFalse(b.validate())

if __name__ == "__main__":
    unittest.main()

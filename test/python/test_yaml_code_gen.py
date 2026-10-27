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

    def setUp(self):
        # Code executed before each test
        yaml_code_gen.scriptName = "script"

    def test_init_with_id(self):
        "Check C++ init code produced from a YAML config descriptor and ID dictionary."
        id = \
        ("name: alf\n"
        "id: 55\n")
        yaml_code_gen.store_type = "tlv"
        b = yaml_code_gen.makeBaseItem(self.cfg)
        yaml_code_gen.setBaseItemIds(b, id)
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
        "Check C++ init code produced from a YAML config descriptor without ID dictionary."
        yaml_code_gen.store_type = "tlv"
        b = yaml_code_gen.makeBaseItem(self.cfg)
        yaml_code_gen.setBaseItemIds(b, None)
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

    def test_init_json(self):
        "Check C++ init code produced from a YAML config descriptor."
        yaml_code_gen.store_type = "json"
        b = yaml_code_gen.makeBaseItem(self.cfg)
        expected_init = ('namespace alf { // Simple item initialization\n'
        'const cfg_mgr::Simple_metadata data =\n'
        '{\n'
        '    {\n'
        '        "alf", // name\n'
        '        cfg_mgr::PERSISTENT,\n'
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
    def setUp(self):
        # Code executed before each test
        yaml_code_gen.scriptName = "script"

    def test_contained_array_definition(self):
        b = yaml_code_gen.makeBaseItem(self.cfg)
        expected_def = ('struct t_alf\n'
        '{\n'
        '    unsigned long ipaddr[2];\n'
        '};\n')
        self.assertEqual(expected_def, b.get_definition())

    def test_contained_array_init(self):
        b = yaml_code_gen.makeBaseItem(self.cfg)
        #print(b.get_init())

class Test_owned_with_key_gen(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        self.cfg = get_file_data(TEST_CONFIG_DIR/"cfg_owned_w_key.yaml")
        yaml_code_gen.scriptName = "script"

    def test_definition(self):
        b = yaml_code_gen.makeBaseItem(self.cfg)
        expected_def = ('struct t_alf\n'
        '{\n'
        '    unsigned long ipaddr[2];\n'
        '};\n')
        self.assertEqual(expected_def, b.get_definition())

    def test_init(self):
        b = yaml_code_gen.makeBaseItem(self.cfg)
        #print(b.get_init())

class Test_invalid_config(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        self.id = get_file_data(TEST_CONFIG_DIR/"cfg_id_counter.yaml")
        yaml_code_gen.scriptName = "script"

    def test_invalid_owned_counter_ref(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_ref.yaml")
        b = yaml_code_gen.makeBaseItem(cfg)
        self.assertFalse(b.validate())

    def test_invalid_owned_counter_multiple(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_multiple.yaml")
        b = yaml_code_gen.makeBaseItem(cfg)
        self.assertFalse(b.validate())

    def test_invalid_owned_counter_owned(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_owned.yaml")
        b = yaml_code_gen.makeBaseItem(cfg)
        self.assertFalse(b.validate())

    def test_invalid_owned_counter_setdef(self):
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_invalid_owned_counter_setdef.yaml")
        b = yaml_code_gen.makeBaseItem(cfg)
        self.assertFalse(b.validate())

if __name__ == "__main__":
    unittest.main()

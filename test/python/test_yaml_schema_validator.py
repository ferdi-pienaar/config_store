""" Unit test for YAML schema validator.
Verify that valid YAML files are accepted by the Yamale schemas.
Verify that invalid YAML files are rejected by the Yamale schemas.
"""

import yaml
import yamale
import unittest
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
TEST_CONFIG_DIR = TEST_DIR/"schema_validator_configs"

# Helper function opens file and returns its yamala_data
def get_yamale_data(cfg_file_path):
    with open(cfg_file_path) as f:
        yaml_data = f.read()
    return yamale.make_data(content=yaml_data)

class Test_config_schema_validator(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        self.schema = yamale.make_schema(TEST_DIR/"../../code_gen/cfg_schema.yaml")

    def test_simple(self):
        "A simple descriptor is valid config."
        yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_simple.yaml"))

    def test_contained(self):
        "A composite descriptor that contains a simple descriptor is valid config."
        yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_contained.yaml"))

    def test_contained_symbolic_count(self):
        "A composite descriptor that contains a simple descriptor with symbolic count is valid config."
        yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_contained_symbolic_count.yaml"))

    def test_owned(self):
        "A composite descriptor that owns a single simple descriptor is valid config."
        yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_owned.yaml"))

    def test_owned_with_counter(self):
        "A composite descriptor that owns, with counter, more than 1 simple descriptors is valid config."
        yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_owned_w_counter.yaml"))

    def test_invalid_simple_no_name(self):
        "A simple descriptor needs 'name'."
        with self.assertRaises(yamale.yamale_error.YamaleError):
           yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_invalid_simple_no_name.yaml"))

    def test_invalid_owned_no_counter(self):
        "A composite descriptor that owns more than 1 simple descriptors needs a counter."
        with self.assertRaises(yamale.yamale_error.YamaleError):
            yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_invalid_owned_no_counter.yaml"))

class Test_id_schema_validator(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        self.schema = yamale.make_schema(TEST_DIR/"../../code_gen/cfg_id_schema.yaml")

    def test_simple(self):
        "An id file with a name and ID is valid."
        yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_id_simple.yaml"))

    def test_composite(self):
        "An id file with a name and ID and components."
        yamale.validate(self.schema, get_yamale_data(TEST_CONFIG_DIR/"cfg_id_composite.yaml"))

if __name__ == "__main__":
    unittest.main()

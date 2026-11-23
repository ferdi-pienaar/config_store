"""Unit test for front_end_model.py
"""

import front_end_model
import unittest
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
TEST_CONFIG_DIR = TEST_DIR/"config"

# Helper function opens file and returns its data
def get_file_data(file_path):
    with open(file_path) as f:
        data = f.read()
    return data

class Test_next_words_owned_w_key(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_owned_w_key.yaml")
        self.baseDescriptor = front_end_model.makeBaseDescriptor(cfg)
        self.item = {}

    def test_empty_config_first_word(self):
        "An empty command passed to an empty config."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, [])
        self.assertEqual(len(words), 8)
        self.assertTrue('load' in words)
        self.assertTrue('save' in words)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)
        self.assertTrue('add' in words)
        self.assertTrue('userCnt' in words) # component name
        self.assertTrue('user' in words) # component name

    def test_owned_item_first_word(self):
        "An empty command passed to a config with an item that can be deleted."
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, [])
        self.assertEqual(len(words), 9)
        self.assertTrue('del' in words)

    def test_owned_item_get_keys(self):
        "Descriptor name passed to descriptor with 2 keys."
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        self.baseDescriptor.set_data(self.item, 'user "Barney" id = 11'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['user'])
        self.assertEqual(words, ['"Fred"', '"Barney"'])

    def test_owned_item_add(self):
        "Add command passed to an empty config: the only item that can be added is 'user'."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['add'])
        self.assertEqual(words, ['user'])

    def test_empty_del(self):
        "Del command passed to an empty config: nothing can be deleted."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['del'])
        self.assertEqual(len(words), 0)

    def test_owned_item_del(self):
        "Del command passed to an config that contains a deletable item: return name of the item."
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['del'])
        self.assertEqual(words, ['user'])

    def test_owned_item_del_get_key(self):
        "Del command with item name passed to an config that contains a deletable item: return possible keys."
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, "del user".split())
        self.assertEqual(words, ['"Fred"'])

    def test_owned_item_del_get_keys(self):
        "Del command with item name passed to an config that contains a deletable item: return 2 possible keys."
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        self.baseDescriptor.set_data(self.item, 'user "Wilma" id = 9'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, "del user".split())
        self.assertEqual(words, ['"Fred"', '"Wilma"'])

    def test_empty_owned_2_words(self):
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, 'user "Fred"'.split())
        self.assertEqual(len(words), 5)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)
        self.assertTrue('uname' in words) # component name
        self.assertTrue('id' in words) # component name

    def test_simple_component_of_owned(self):
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, 'user "Fred" id'.split())
        self.assertEqual(len(words), 4)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)
        self.assertTrue('=' in words)

    def test_component_without_set_fn(self):
        "simple has no set fn, so word '=' is not included."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['userCnt'])
        self.assertEqual(len(words), 3)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)

    def test_change_context(self):
        self.baseDescriptor.set_data(self.item, 'user "Fred" id = 8'.split())
        ctxt, component_item = self.baseDescriptor.get_context(self.item, 'user "Fred"'.split())
        words = front_end_model.get_next_words(ctxt, component_item, ['id'])
        self.assertEqual(len(words), 4)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)
        self.assertTrue('=' in words)


class Test_next_words_owned_w_counter(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_owned_w_counter.yaml")
        self.baseDescriptor = front_end_model.makeBaseDescriptor(cfg)
        self.item = {}

    def test_empty_config_first_word(self):
        "0 is proposed as index for empty owned."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['ipaddr'])
        self.assertEqual(words, ["0"])

    def test_one_owned_config_first_word(self):
        "next index is included in list of proposed indexes."
        self.baseDescriptor.set_data(self.item, 'ipaddr 0 = 8009'.split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['ipaddr'])
        self.assertEqual(words, ["0", "1"])


class Test_next_words_owned_2_single(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_owned_2_single.yaml")
        self.baseDescriptor = front_end_model.makeBaseDescriptor(cfg)
        self.item = {}

    def test_owned_item_add(self):
        "Add command passed to an empty config: there are 2 items that can be added."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['add'])
        self.assertEqual(words, ['ipaddr', 'port'])

    def test_owned_item_exist(self):
        "cmd references an owned item that doesn't need a key."
        self.baseDescriptor.set_data(self.item, "port = 333".split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['port'])
        self.assertEqual(len(words), 4)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)
        self.assertTrue('=' in words)

    def test_owned_item_not_exist(self):
        "cmd references a keyless owned item that doesn't exist: implicit add would create it."
        # xxx setdef prt and prtc would be rejected, since the item doesn't exist yet.
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['port'])
        self.assertEqual(len(words), 4)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)
        self.assertTrue('=' in words)


class Test_next_words_contained_owned_2_single(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_contained_owned_2_single.yaml")
        self.baseDescriptor = front_end_model.makeBaseDescriptor(cfg)
        self.item = {}

    def test_owned_item_add(self):
        "Add command passed to an empty config: there are 2 items that can be added."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, 'network add'.split())
        self.assertEqual(words, ['ipaddr', 'port'])

    def test_owned_item_add_one_created(self):
        "Add command passed to a config with one item added: only the other one can be added."
        self.baseDescriptor.set_data(self.item, "network ipaddr = 14".split())
        self.assertEqual(self.item, {'network': {'ipaddr': {}}})
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, 'network add'.split())
        self.assertEqual(words, ['port'])

    def test_owned_item_options_one_created(self):
        "Add command passed to a config with one item added: del and add are offered as 2nd word."
        self.baseDescriptor.set_data(self.item, "network ipaddr = 15".split())
        self.assertEqual(self.item, {'network': {'ipaddr': {}}})
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['network'])
        #self.assertEqual(len(words), 4)
        self.assertTrue('del' in words)
        self.assertTrue('add' in words)

    def test_owned_item_options_both_created(self):
        "Add command passed to a config with one item added: del but not add is offered as 2nd word."
        self.baseDescriptor.set_data(self.item, "network ipaddr = 16".split())
        self.baseDescriptor.set_data(self.item, "network port = 21".split())
        self.assertEqual(self.item, {'network': {'ipaddr': {}, 'port': {}}})
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['network'])
        print(words)
        #self.assertEqual(len(words), 4)
        self.assertFalse('add' in words)
        self.assertTrue('del' in words)

    def test_owned_item_del_one_created(self):
        "Add command passed to a config with one item added: only this item can be deleted."
        self.baseDescriptor.set_data(self.item, "network ipaddr = 17".split())
        self.assertEqual(self.item, {'network': {'ipaddr': {}}})
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, 'network del'.split())
        self.assertEqual(words, ['ipaddr'])

class Test_next_words_contained(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_contained.yaml")
        self.baseDescriptor = front_end_model.makeBaseDescriptor(cfg)
        self.item = {}

    def test_empty_config_first_word(self):
        "An empty command passed to an empty config."
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, [])
        self.assertEqual(len(words), 6)
        self.assertTrue('load' in words)
        self.assertTrue('save' in words)
        self.assertTrue('setdef' in words)
        self.assertTrue('prt' in words)
        self.assertTrue('prtc' in words)
        self.assertTrue('ipaddr' in words) # component name

    def test_get_indexes(self):
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, ['ipaddr'])
        self.assertEqual(words, ["0", "1"])


class Test_next_words_owned_in_owned(unittest.TestCase):
    def setUp(self):
        # Code executed before each test
        cfg = get_file_data(TEST_CONFIG_DIR/"cfg_owned_in_owned.yaml")
        self.baseDescriptor = front_end_model.makeBaseDescriptor(cfg)
        self.item = {}

    def test_different_data_in_owned_items(self):
        "Two components have same metadata but different keys."
        self.baseDescriptor.set_data(self.item, "user 77 temp 0 = 44".split())
        self.baseDescriptor.set_data(self.item, "user 89 temp 0 = 56".split())
        self.baseDescriptor.set_data(self.item, "user 89 temp 1 = 78".split())
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, 'user 77 temp'.split())
        # temp is indexed, so implicit add allows the next index after the existing one.
        self.assertEqual(words, ["0", "1"])
        words = front_end_model.get_next_words(self.baseDescriptor, self.item, 'user 89 temp'.split())
        self.assertEqual(words, ["0", "1", "2"])


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
""" Data model for front end, with metadata from YAML descriptor and data
 from the app's response to a 'prt' command.

Inputs:
 0. Client calls makeBaseDescriptor to initialize base Descriptor with YAML metadata, and gets base Descriptor.
 1. Client gets current data state from app (in app's response cmd 'prt') and calls context.set_data() for each line.
 2. Client repeatedly calls get_next_words(context, cmd) to construct command to send to app.
 3. Client sends the constructed command to the app
 4. Client calls baseDescriptor.get_context to update its context, based on prompt that app returns to the command.
 5. Repeat steps 1-4.

This module has no global state. State is held by the client:
- base state, consisting of:
  - base Descriptor, on which client calls get_context.
  - base item
- context state, consisting of
  - context Descriptor, on which client calls get_next_words and set_data
  - context item

"""

import yaml

class Descriptor:
    # command words that are valid as next words for any Descriptor.
    common_command_words = ['prt', 'prtc', 'setdef']
    def __init__(self, d):
        self.d = d


class SimpleDescriptor(Descriptor):
    def __init__(self, d):
        Descriptor.__init__(self, d)
        #print("create simple {}".format(d['name']))

    def init_data(self, item):
        pass

    def set_data(self, item, cmd):
        "Handle an executed valid config command line for this Descriptor."
        #print("set_data simple '{}' {}".format(self.d['name'], locals()))

    def get_context(self, item, prompt):
        "Return this Descriptor if it matches cmd."
        #print("get_context simple '{}' {}".format(self.d['name'], item, locals()))
        return self, item

    def get_next_words(self, item, cmd):
        "Based on a data model from YAML and calls to fn config, return set of possible next words in partial command line."
        #print("simple get_next_words '{}' {}".format(self.d['name'], locals()))
        if item is None:
            # For implicit add only one command is supported.
            if 'set' in self.d:
                return ['=']
            else:
                return []
        words = Descriptor.common_command_words[:]
        if 'set' in self.d:
            words.append('=')
        return words

    def support_set(self):
        "Return true if descriptor has set function."
        return 'set' in self.d


class CompositeDescriptor(Descriptor):
    def __init__(self, d):
        Descriptor.__init__(self, d)
        #print("create composite {}".format(d['name']))
        self.aggregates = []
        for a in self.d['aggregate']:
            self.aggregates.append(makeAggregate(a))

    def init_data(self, item):
        "Create data that exists before any command is executed."
        #print("init_data composite '{}' {}".format(self.d['name'], locals()))
        for a in self.aggregates:
            a.init_data(item)

    def set_data(self, item, cmd):
        "Handle an executed valid set command line for this Descriptor."
        #print("set_data composite '{}' {}".format(self.d['name'], locals()))
        for a in self.aggregates:
            # Pass the command on to each component.
            if cmd[0] == a.descriptor.d['name']:
                # We pass the full item to the aggregate since it may need to extend it.
                a.set_data(item, cmd[1:])

    def get_context(self, item, prompt):
        "Return this Descriptor or the child that matches cmd."
        #print("get_context composite '{}' {}".format(self.d['name'], item, locals()))
        if len(prompt) == 0:
            return self, item
        for aggr in self.aggregates:
            d, i = aggr.get_context(item, prompt)
            if d is not None:
                return d, i
        return None, None

    def get_next_words(self, item, cmd):
        "Based on a data model from YAML and calls to fn config, return set of possible next words in partial command line."
        #print("composite get_next_words '{}' {}".format(self.d['name'], locals()))
        if len(cmd) == 0:
            # The last word in (partial?) cmd was this Descriptor's name, so it returns next words.
            # Start with a copy of the words available in a non-implicit-add context.
            words = []
            if item is not None:
                words = Descriptor.common_command_words[:]
            if self.allow_aggr_resize_op('add', item):
                words.append('add')
            if self.allow_aggr_resize_op('del', item):
                words.append('del')
            for a in self.aggregates:
                if item is not None or a.descriptor.support_set():
                    # If the item exists, or if the component supports '=', add the descriptor name.
                    # The reason is that if the item doesn't exist, we still support implicit add.
                    words.append(a.descriptor.d['name'])
            return words

        # There are command words to pass to components, one or more of which may return next words.
        words = []
        for a in self.aggregates:
            # Add words returned by aggregate, which may be an empty list.
            # We call the aggregate even if the item doesn't contain it because
            # if the command is 'add' it may return the descriptor name.
            words += a.get_next_words(item, cmd)
        return words

    def support_set(self):
        "Set is supported if any components support it."
        for a in self.aggregates:
            if a.support_set():
                return True
        return False

    def allow_aggr_resize_op(self, op, item):
        "Return True if possible for this Descriptor in its current state, else False"
        if item is None:
            # Not supported for items that don't exist, i.e. implicit add.
            return False
        for a in self.aggregates:
            component_item = None
            if a.descriptor.d['name'] in item:
                component_item = item[a.descriptor.d['name']]
            if a.allow_resize_op(op, component_item):
                # This aggregate can add, so our search is over.
                return True
        # No aggregate allows the operation.
        return False


class Aggregate:
    def __init__(self, d):
        self.d = d
        self.descriptor = makeDescriptor(self.d['descriptor'])

    def get_context(self, item, prompt):
        #print("get_context {} aggr '{}' {}".format(self.d['type'], self.descriptor.d['name'], locals()))
        if prompt[0] != self.descriptor.d['name']:
            return None, None
        if (self.get_max_count() > 1) or ('key' in self.d):
            # next cmd word is a key (which may be an integer index).
            if prompt[1] in item[prompt[0]]:
                # The command contains an identifier (key or index) that identifies an item..
                return self.descriptor.get_context(item[prompt[0]][prompt[1]], prompt[2:])
        else:
            # No key is used because there is only 1 item, and identifier is index, not key.
            return self.descriptor.get_context(item[prompt[0]], prompt[1:])
        return None, None

    def common_get_next_words(self, item, cmd):
        "Apart from 'add' and 'del', next words are common to Contained and Owned."
        #print("common_get_next_words '{}' {}".format(self.descriptor.d['name'], locals()))
        if cmd[0] != self.descriptor.d['name']:
            return []
        if self.get_max_count() == 1:
            # No identifier is needed, pass remainder to descriptor.
            return self.descriptor.get_next_words(item, cmd[1:])
        # An identifier is required.
        if len(cmd) == 1:
            # The descriptor name is the last cmd word: the next word should be an identifier.
            return self.get_next_identifiers(item)
        # An identifier is required and the command has another word after the name.
        component_item = None
        if item is not None and cmd[1] in item.keys():
            # A valid identifier is in cmd
            component_item = item[cmd[1]]
        return self.descriptor.get_next_words(component_item, cmd[2:])

    def get_max_count(self):
        "Return numerical value for maximum number of instances of aggregate"
        count = self.d['count']
        if type(count) == dict:
            # The value of the symbolic constant
            return count['value']
        # Simple case: return the value
        return count


class ContainedAggregate(Aggregate):
    def __init__(self, d):
        #print("create contained",)
        Aggregate.__init__(self, d)

    def init_data(self, item):
        "A contained aggregate has items because its parent item exists."
        #print("init_data {} aggr '{}' {}".format(self.d['type'], self.descriptor.d['name'], locals()))
        item[self.descriptor.d['name']] = {}
        if self.get_max_count() == 1:
            self.descriptor.init_data(item[self.descriptor.d['name']])
        else:
            # More than one items exist, so indexes are used.
            for index in self.get_next_identifiers(None):
                item[self.descriptor.d['name']][index] = {}
                self.descriptor.init_data(item[self.descriptor.d['name']][index])

    def set_data(self, item, cmd):
        "Handle an executed valid set command line for this aggregate's descriptor."
        # item is the parent item; cmd has been stripped of the name identifying this component.
        #print("set_data {} aggr '{}' {}".format(self.d['type'], self.descriptor.d['name'], locals()))
        if (self.get_max_count() > 1):
            # next cmd word is an index, so remove it from cmd.
            self.descriptor.set_data(item[self.descriptor.d['name']][cmd[0]], cmd[1:])
        else:
            # No identifier is used because there is only 1 item.
            self.descriptor.set_data(item[self.descriptor.d['name']], cmd)

    def get_next_words(self, item, cmd):
        "The cmd word may apply to this aggregate: return the Descriptor name if so."
        #print("contained aggregate get_next_words '{}' {}".format(self.descriptor.d['name'], locals()))
        if item is None:
            component_item = None
        elif self.descriptor.d['name'] in item:
            component_item = item[self.descriptor.d['name']]
        else:
            # The parent item exists, but not this component.
            component_item = {}
        return self.common_get_next_words(component_item, cmd)

    def get_next_identifiers(self, item):
        "Contained aggregate has a fixed range of indexes that act as identifiers."
        return [str(i) for i in range(0, self.get_max_count())]

    def allow_resize_op(self, op, item):
        "A contained aggregate has fixed size, so does not support these operations."
        return False


class OwnedAggregate(Aggregate):
    def __init__(self, d):
        #print("create owned",)
        Aggregate.__init__(self, d)

    def init_data(self, item):
        "A owned aggregate has no items until configured."
        #print("init_data {} aggr '{}' {}".format(self.d['type'], self.descriptor.d['name'], locals()))

    def set_data(self, item, cmd):
        "Handle an executed valid set command line for this aggregate's descriptor."
        # item is the parent item; cmd has been stripped of the name identifying this component.
        #print("set_data {} aggr '{}' {}".format(self.d['type'], self.descriptor.d['name'], locals()))
        if not self.descriptor.d['name'] in item:
            # No entry for this component was created by a previous set_data, so create it.
            item[self.descriptor.d['name']] = {}
        if (self.get_max_count() > 1) or ('key' in self.d):
            # next cmd word is a key (which may be an integer index), so remove it from cmd.
            if not cmd[0] in item[self.descriptor.d['name']]:
                # No entry for this key was created by a previous set_data, so create it.
                item[self.descriptor.d['name']][cmd[0]] = {}
                self.descriptor.init_data(item[self.descriptor.d['name']][cmd[0]])
            self.descriptor.set_data(item[self.descriptor.d['name']][cmd[0]], cmd[1:])
        else:
            # No identifier is used because there is only 1 item and identifier is index, not key.
            self.descriptor.init_data(item[self.descriptor.d['name']])
            self.descriptor.set_data(item[self.descriptor.d['name']], cmd)

    def get_next_words(self, item, cmd):
        "The cmd word may apply to this aggregate: return the Descriptor name if so."
        #print("owned aggregate get_next_words '{}' {}".format(self.descriptor.d['name'], locals()))
        component_item = None
        if item is not None and self.descriptor.d['name'] in item:
            component_item = item[self.descriptor.d['name']]
        num_components = self.get_num_component_items(component_item)
        if len(cmd) == 1:
            if cmd[0] == 'add' and num_components < self.get_max_count():
                return [self.descriptor.d['name']]
            elif cmd[0] == 'del' and num_components > 0:
                return [self.descriptor.d['name']]
        if len(cmd) == 2 and cmd[0] == 'del' and cmd[1] == self.descriptor.d['name']:
            return list(component_item.keys())
        return self.common_get_next_words(component_item, cmd)

    def get_num_component_items(self, item):
        if item is None:
            return 0
        if self.get_max_count() > 1:
            # There can be more than one item, so an identifier is needed.
            return len(item.keys())
        # No identifier is needed, so the fact that item is not None means the single instance exists.
        return 1

    def get_next_identifiers(self, item):
        "Owned aggregate has a resizable range of items."
        if 'key' in self.d:
            # If keys are used, return the set of keys that are installed.
            return list(item.keys())
        # If index is used instead, the possible identifiers include the next index too.
        if item is None or len(item.keys()) == 0:
            # Special case where no items exist: return the first index.
            return ["0"]
        # Indexed items exist: return current indexes and the next one.
        indexes = [int(i) for i in item.keys()]
        indexes.append(max(indexes) + 1)
        return [str(i) for i in indexes]

    def allow_resize_op(self, op, item):
        "Return True iff operation is possible."
        #print("allow_resize_op d={} locals={}".format(self.d, locals()))
        # If the number of items (represented by their keys) hasn't reached max, add is allowed.
        num_components = self.get_num_component_items(item)
        if op == 'add':
            return num_components < self.get_max_count()
        elif op == 'del':
            return num_components > 0


def makeDescriptor(d):
    "Factory method returns an Descriptor object of the requested class"
    if 'aggregate' in d: 
        return CompositeDescriptor(d)
    else:
        return SimpleDescriptor(d)


def makeAggregate(d):
    "Factory method returns an Aggregate object of the requested class"
    if d['type'] == 'owned': 
        return OwnedAggregate(d)
    else:
        return ContainedAggregate(d)


def makeBaseDescriptor(yaml_cfg_text):
    "Convert YAML text to a Python data structure."
    try:
        cfg = yaml.load(yaml_cfg_text, Loader=yaml.SafeLoader)
    except:
        print("{}: text is not YAML".format(__name__))
        return
    try:
        base_descriptor_config = cfg['descriptor']
    except:
        print("{}: Root element in configuration is not a descriptor".format(scriptName))
        return
    return makeDescriptor(base_descriptor_config)


def get_next_words(descriptor, item, cmd):
    if len(cmd) == 0:
        # Present options for first word in addition to the context-specific ones.
        return ["save", "load", "<"] + descriptor.get_next_words(item, cmd)
    return descriptor.get_next_words(item, cmd)


if __name__ == "__main__":
    pass

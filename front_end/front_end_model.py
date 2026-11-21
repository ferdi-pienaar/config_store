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
        print("create simple {}".format(d['name']))

    def set_data(self, item, cmd):
        "Handle an executed valid config command line for this Descriptor."
        #print("set_data simple '{}' item {} cmd {}".format(self.d['name'], item, cmd))

    def get_context(self, item, cmd):
        "Return this Descriptor if it matches cmd."
        if cmd[0] == self.d['name'] and len(cmd) == 1:
            return self, item

    def get_next_words(self, item, cmd):
        "Based on a data model from YAML and calls to fn config, return set of possible next words in partial command line."
        #print("get_next_words simple '{}' item {} cmd {}".format(self.d['name'], item, cmd))
        words = Descriptor.common_command_words[:]
        if 'set' in self.d:
            words.append('=')
        return words


class CompositeDescriptor(Descriptor):
    def __init__(self, d):
        Descriptor.__init__(self, d)
        print("create composite {}".format(d['name']))
        self.aggregates = []
        for a in self.d['aggregate']:
            self.aggregates.append(makeAggregate(a))

    def set_data(self, item, cmd):
        "Handle an executed valid set command line for this Descriptor."
        #print("set_data composite '{}' item {} cmd {}".format(self.d['name'], item, cmd))
        for a in self.aggregates:
            # Pass the command on to the matching component.
            if cmd[0] == a.descriptor.d['name']:
                if not a.descriptor.d['name'] in item:
                    # No entry exists for this component, so create it.
                    item[a.descriptor.d['name']] = {}
                return a.set_data(item[a.descriptor.d['name']], cmd[1:])

    def get_context(self, item, cmd):
        "Return this Descriptor or the child that matches cmd."
        print("get_context composite '{}' item {} cmd {}".format(self.d['name'], item, cmd))
        if len(cmd) == 0:
            return self, item
        for aggr in self.aggregates:
            if cmd[0] == aggr.descriptor.d['name']:
                d, i = aggr.get_context(item[aggr.descriptor.d['name']], cmd[1:])
                if d is not None:
                    return d, i

    def get_next_words(self, item, cmd):
        "Based on a data model from YAML and calls to fn config, return set of possible next words in partial command line."
        #print("composite get_next_words '{}' {}".format(self.d['name'], locals()))
        if len(cmd) == 0:
            # The last word in (partial?) cmd was this Descriptor's name, so it returns next words.
            # Start with a copy of the words available in any context.
            words = Descriptor.common_command_words[:]
            if self.allow_aggr_resize_op('add', item):
                words.append('add')
            if self.allow_aggr_resize_op('del', item):
                words.append('del')
            for a in self.d['aggregate']:
                words.append(a['descriptor']['name'])
            return words

        # There are command words to pass to components, one or more of which may return next words.
        words = []
        for a in self.aggregates:
            # Add words returned by aggregate, which may be an empty list.
            component_item = {}
            if a.descriptor.d['name'] in item:
                component_item = item[a.descriptor.d['name']]
            words += a.get_next_words(component_item, cmd)
        return words

    def allow_aggr_resize_op(self, op, item):
        "Return True if possible for this Descriptor in its current state, else False"
        for a in self.aggregates:
            component_item = {}
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

    def set_data(self, item, cmd):
        "Handle an executed valid set command line for this aggregate's Descriptor."
        #print("set_data {} aggr item {} cmd {}".format(self.d['type'], item, cmd))
        if (self.get_max_count() > 1) or ('key' in self.d):
            # next cmd word is a key (which may be an integer index), so remove it from cmd.
            if not cmd[0] in item:
                # No entry exists for this key, so create it.
                item[cmd[0]] = {}
            self.descriptor.set_data(item[cmd[0]], cmd[1:])
        else:
            # No key is used because there is only 1 item, and identifier is index, not key.
            self.descriptor.set_data(item, cmd)

    def get_context(self, item, cmd):
        print("get_context {} aggr '{}' item {} cmd {}".format(self.d['type'], self.descriptor.d['name'], item, cmd))
        if (self.get_max_count() > 1) or ('key' in self.d):
            # next cmd word is a key (which may be an integer index).
            if cmd[0] in item:
                # The command contains an identifier (key or index) that identifies an item..
                return self.descriptor.get_context(item[cmd[0]], cmd[1:])
        else:
            # No key is used because there is only 1 item, and identifier is index, not key.
            self.descriptor.set_data(item, cmd)

    def common_get_next_words(self, item, cmd):
        "Apart from 'add' and 'del', next words are common to Contained and Owned."
        #print("common_get_next_words '{}' item {} cmd {}".format(self.descriptor.d['name'], item, cmd))
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
        if cmd[1] in item.keys():
            # A valid identifier is in cmd, pass the rest to descriptor.
            return self.descriptor.get_next_words(item[cmd[1]], cmd[2:])

    def get_max_count(self):
        "Return numerical value for maximum number of instances of aggregate"
        count = self.d['count']
        if type(count) == dict:
            # The value of the symbolic constant
            return count['value']
        # Simple case: return the value
        return self.d['count']


class ContainedAggregate(Aggregate):
    def __init__(self, d):
        print("create contained",)
        Aggregate.__init__(self, d)

    def get_next_words(self, item, cmd):
        "The cmd word may apply to this aggregate: return the Descriptor name if so."
        #print("contained aggregate get_next_words '{}' item {} cmd {}".format(self.descriptor.d['name'], item, cmd))
        return self.common_get_next_words(item, cmd)

    def get_next_identifiers(self, item):
        "Contained aggregate has a fixed range of indexes that act as identifiers."
        return [str(i) for i in range(0, self.get_max_count())]

    def allow_resize_op(self, op, item):
        "A contained aggregate has fixed size, so does not support these operations."
        return False


class OwnedAggregate(Aggregate):
    def __init__(self, d):
        print("create owned",)
        Aggregate.__init__(self, d)

    def get_next_words(self, item, cmd):
        "The cmd word may apply to this aggregate: return the Descriptor name if so."
        #print("owned aggregate get_next_words '{}' item {} cmd {}".format(self.descriptor.d['name'], item, cmd))
        if len(cmd) == 1:
            if cmd[0] == 'add' and len(item.keys()) < self.get_max_count():
                return [self.descriptor.d['name']]
            elif cmd[0] == 'del' and len(item.keys()) > 0:
                return [self.descriptor.d['name']]
        if len(cmd) == 2 and cmd[0] == 'del' and cmd[1] == self.descriptor.d['name']:
            return list(item.keys())
        return self.common_get_next_words(item, cmd)

    def get_next_identifiers(self, item):
        "Owned aggregate has a resizable range of items."
        if 'key' in self.d:
            # If keys are used, return the set of keys that are installed.
            return list(item.keys())
        # If index is used instead, the possible identifiers include the next index too.
        if len(item.keys()) == 0:
            # Special case where no items exist: return the first index.
            return ["0"]
        indexes = [int(i) for i in item.keys()]
        indexes.append(max(indexes) + 1)
        return [str(i) for i in indexes]

    def allow_resize_op(self, op, item):
        "Return True iff operation is possible."
        # If the number of items (represented by their keys) hasn't reached max, add is allowed.
        if op == 'add':
            return len(item.keys()) < self.get_max_count()
        elif op == 'del':
            return len(item.keys()) > 0


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
        # Options for first word include 'save' and 'load' in addition to the context-specific ones.
        return ["save", "load"] + descriptor.get_next_words(item, cmd)
    return descriptor.get_next_words(item, cmd)


if __name__ == "__main__":
    pass

#!/usr/bin/env python3
"""
Run config_store executable in sub-process. Update front_end_model so it can provide next words for command.

Parameters
 yaml file
 executable
 cfg (tlv or json)

"""
import sys
import subprocess
import front_end_model
import readline
from fe_readline import WordCompleter

def loadFile(file_path):
    "Load configuration metadata from YAML config file"
    with open(file_path) as f:
        data = f.read()
    return data

class Interaction():
    # Class variables define constants.
    PROMPT_END = ">\n"
    def __init__(self, base_descriptor):
        process_path = sys.argv[2]
        nvram_file_path = sys.argv[3]
        # Launch the app in a subprocess.
        self.process = subprocess.Popen(
            [process_path, nvram_file_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True)
        self.base_descriptor = base_descriptor
        self.base_item = {}
        self.base_descriptor.init_data(self.base_item)
        # Initially, context is the base context.
        self.ctxt_descriptor = base_descriptor
        self.ctxt_item = self.base_item

    def serve(self):
        "Repeat: wait for prompt from app and send command from operator to app."
        prompt = self.receive_until_prompt('user')
        while True:
            # Rebuild the data part of the model with each interaction.
            # The previous prompt (or the initial one) begins the next (or first) interaction.
            prompt = self.interact(prompt)

    def interact(self, prompt):
        "Present prompt to operator, send operator command to app, gather app state for model."
        cmd = input(prompt)
        self.send_cmd(cmd)
        prompt = self.receive_until_prompt('user')
        # Use the prompt returned after sent command to set the context in which we'll generate next words for commands.
        prompt_words = prompt.split()[1:-1]
        self.ctxt_descriptor, self.ctxt_item = self.base_descriptor.get_context(self.base_item, prompt_words)
        # Send prompt so update_data_state can restore the context, but discard '> ' at the end.
        restore_prompt = ""
        for w in prompt_words:
            restore_prompt += w + ' '
        self.update_data_state(restore_prompt)
        return prompt

    def update_data_state(self, previous_prompt):
        "Get state from app by sending it command 'prt' and use that to update the model's data."
        # We get a new data model after each command sent to app, so start with fresh data.
        self.item = {}
        self.base_descriptor.init_data(self.base_item)
        # Temporarily change the app's context to top-level, so 'prt' gives returns global state.
        self.send_cmd("<")
        self.receive_until_prompt('discard')
        self.send_cmd("prt")
        self.receive_until_prompt('self')
        # Return app to the context that user set it to.
        self.send_cmd(previous_prompt)
        self.receive_until_prompt('discard')

    def receive_until_prompt(self, dest):
        "Take output from app and pass to operator, until app ends its output by sending a user prompt."
        while True:
            output = self.process.stdout.readline()
            if output.endswith(Interaction.PROMPT_END):
                # Strip the endline from the prompt and replace with a space.
                return output[:-1] + " "
            if dest == 'user':
                # Pass app output to operator.
                print(output, end='')
            elif dest == 'self':
                # Feed app output to our data model.
                self.base_descriptor.set_data(self.base_item, output.split())

    def send_cmd(self, cmd):
        self.process.stdin.write(cmd + "\n")
        self.process.stdin.flush()

    def get_commands(self):
        "Return the commands available in current state."
        cmd = self.get_line_complete_words()
        # Get next words from the model, given the current line and current context.
        return front_end_model.get_next_words(self.ctxt_descriptor, self.ctxt_item, cmd)

    def get_line_complete_words(self):
        "Return words in line that are complete, i.e. followed by a space."
        line = readline.get_line_buffer()
        endlast = line.rfind(" ")
        if endlast == -1:
            # No space found, so no complete words.
            return []
        return line[:endlast].split()

if __name__ == '__main__':
    cfgFilePath = sys.argv[1]

    cfg_text = loadFile(cfgFilePath)
    if cfg_text is None:
        sys.exit()

    baseDescriptor = front_end_model.makeBaseDescriptor(cfg_text)
    interaction = Interaction(baseDescriptor)

    wordCompleter = WordCompleter(interaction.get_commands)
    readline.set_completer(wordCompleter.complete)
    readline.parse_and_bind("tab: complete")

    interaction.serve()

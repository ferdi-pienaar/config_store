#!/usr/bin/env python3
"""
Run config_store executable in sub-process. Update front_end_model so it can provide next words for command.

Parameters
 yaml file
 executable
 cfg file (tlv or json)

"""
import sys
import subprocess
import readline
from enum import Enum
from pathlib import Path
# To install this script: look for modules where they are installed.
module_dir = Path(__file__).resolve().parent.parent / "share" / "config_store"
sys.path.insert(0, str(module_dir))
from fe_readline import WordCompleter
import front_end_model

def loadFile(file_path):
    "Load configuration metadata from YAML config file"
    with open(file_path) as f:
        data = f.read()
    return data

class Interaction():
    # Class variables define constants.
    PROMPT_END = ">\n"
    class Output_dest(Enum):
        OPERATOR = 1 # Pass output from app to stdout, for operator.
        MODEL = 2 # Use output from app to update our internal data model.
        DISCARD = 3 # Discard output from app because we don't need it.

    def __init__(self, base_descriptor, process_path, nvram_file_path):
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
        prompt = self.receive_until_prompt(Interaction.Output_dest.OPERATOR)
        while True:
            # Rebuild the data part of the model with each interaction.
            # The previous prompt (or the initial one) begins the next (or first) interaction.
            prompt = self.interact(prompt)

    def interact(self, prompt):
        "Present prompt to operator, send operator command to app, gather app state for model."
        cmd = input(prompt)
        prompt = self.interact_with_app(cmd, Interaction.Output_dest.OPERATOR)
        prompt_words = prompt.split()[1:-1]
        # Pass prompt to update_data_state so it can restore the context, but discard '> ' at the end.
        current_prompt = " ".join(prompt_words)
        self.update_data_state(current_prompt)
        # Use the prompt returned after sent command to set the context in which we'll generate next words.
        self.ctxt_descriptor, self.ctxt_item = self.base_descriptor.get_context(self.base_item, prompt_words)
        return prompt

    def interact_with_app(self, cmd, output_dest):
        "Send command to app and wait and handle its response, and return the prompt received from the app."
        self.process.stdin.write(cmd + "\n")
        self.process.stdin.flush()
        return self.receive_until_prompt(output_dest)

    def receive_until_prompt(self, output_dest):
        "Take output from app and pass to operator or our data model, until app ends its output by sending a user prompt."
        while True:
            output = self.process.stdout.readline()
            if output.endswith(Interaction.PROMPT_END):
                # Strip the endline from the prompt and replace with a space.
                return output[:-1] + " "
            if output_dest == Interaction.Output_dest.OPERATOR:
                print(output, end='')
            elif output_dest == Interaction.Output_dest.MODEL:
                # Feed app output to our data model.
                self.base_descriptor.set_data(self.base_item, output.split())

    def update_data_state(self, previous_prompt):
        "Get state from app by sending it command 'prt' and use that to update the model's data."
        # We get a new data model after each command sent to app, so start with fresh data.
        self.base_item = {}
        self.base_descriptor.init_data(self.base_item)
        # Temporarily change the app's context to top-level, so 'prt' gives returns global state.
        self.interact_with_app("<", Interaction.Output_dest.DISCARD)
        self.interact_with_app("prt", Interaction.Output_dest.MODEL)
        # Return app to the context that user set it to.
        self.interact_with_app(previous_prompt, Interaction.Output_dest.DISCARD)

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
    cfg_file_path, process_path, nvram_file_path = sys.argv[1:4]

    cfg_text = loadFile(cfg_file_path)
    if cfg_text is None:
        sys.exit()

    baseDescriptor = front_end_model.makeBaseDescriptor(cfg_text)
    interaction = Interaction(baseDescriptor, process_path, nvram_file_path)

    wordCompleter = WordCompleter(interaction.get_commands)
    readline.set_completer(wordCompleter.complete)
    readline.parse_and_bind("tab: complete")

    interaction.serve()

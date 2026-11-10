"""
tab-completion part for front end -- still to be integrated with the model.

Usage by client module: import this and create an instance of the class, passing
a function that returns a list of commands as parameter, and register its complete
method with readline.set_completer(). When operator presses tab while inputting a line
(using input()) the complete() method is called (by the framework behin input())
to get words to offer as completions.
"""

class WordCompleter():
    def __init__(self, get_commands):
        self.commands = []
        self.get_commands_fn = get_commands

    def complete(self, text, state):
        "Called repeatedly, with increasing 'state', when user presses tab (or another assigned key)."
        if state == 0:
            # Front_end_model's get_next_words() returns next words given a partial command, i.e. complete words.
            # We don't need to call it every time this function is called, since this function is called repeatedly
            # for every completion request, so we only call it when state is 0.
            self.commands = self.get_commands_fn()
        # On each call (with increasing state) return the next word, then None to indicate there are no more.
        matches = [cmd for cmd in self.commands if cmd.startswith(text)] + [None]
        return matches[state]

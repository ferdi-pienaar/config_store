"""
tab-completion part for front end -- still to be integrated with the model.

Usage: python fe_readline.py <source YAML file>.

"""
import readline
from fe_readline import WordCompleter

def get_commands():
    "Return the commands available in current state."
    line_words = get_line_complete_words()
    # A sample function that given a set of words, returns the possible next words.
    # In this case, the next word depends on how many words there are in line_words.
    index = len(line_words)
    command_set = [["able", "ant", "ale"], ["bat", "bear", "bosun"], ["cat", "cave", "crate"]]
    return command_set[index]

def get_line_complete_words():
    "Return words in line that are complete, i.e. followed by a space."
    line = readline.get_line_buffer()
    endlast = line.rfind(" ")
    if endlast == -1:
        # No space found, so no complete words.
        return []
    return line[:endlast].split()

if __name__ == '__main__':
    wordCompleter = WordCompleter(get_commands)
    readline.set_completer(wordCompleter.complete)
    readline.parse_and_bind("tab: complete")
    while True:
        line = input("LINE > ")
        print("send this: '{}'".format(line))

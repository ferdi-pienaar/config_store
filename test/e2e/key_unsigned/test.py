"""End-to-end unit test.
 Not a unit test, but uses Python module unittest to run tests.
 Start the C++ executable in a subprocess
 Send commands to it via stdout.
 Receive responses on stdint.
"""
import sys
import unittest
import subprocess

process_path = sys.argv[1]
nvram_file_path = sys.argv[2]

class Test_simple_gen(unittest.TestCase):

    def setUp(self):
        # Before each test, launch the process-under-test.
        self.process = subprocess.Popen(
            [process_path, nvram_file_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True)

    def tearDown(self):
        self.process.terminate()
        self.process.wait()

    def send_command(self, command):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def get_output_line(self):
        return self.process.stdout.readline()

    def test_get_prompt(self):
        self.assertEqual(self.get_output_line(), "device >\n")

    def test_prt_empty(self):
        self.assertEqual(self.get_output_line(), "device >\n")
        self.send_command("prt")
        self.assertEqual(self.get_output_line(), "userCnt = 0\n")

    def test_add(self):
        self.assertEqual(self.get_output_line(), "device >\n")
        self.send_command("add user 18")
        self.assertEqual(self.get_output_line(), "device >\n")
        self.send_command("prt")
        self.assertEqual(self.get_output_line(), "userCnt = 1\n")
        self.assertEqual(self.get_output_line(), "user 18 id = 18\n")
        self.assertEqual(self.get_output_line(), "device >\n")

if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])

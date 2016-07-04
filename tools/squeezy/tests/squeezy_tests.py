#!/usr/bin/env python2.4
#
# squeezy_tests.py

""" Script for running lc tests. """

import sys
import os
import os.path
import subprocess
import time


squeezy = "../squeezy"

srcdir = os.getenv("SRCDIR", "")
topsrcdir = os.getenv("TOPSRCDIR", "")

sys.path.append(os.path.join(topsrcdir, "testsuite"))

try:
    from lydia_tests import *
except ImportError:
    print "You must set the TOPSRCDIR environment variable"
    print "to the location of your Lydia source tree"
    sys.exit(1)


def run_squeezy_command(test):

    starttime = time.time()
    if Config.echo:
        pipe = subprocess.Popen([squeezy, test.input],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        stdout, _ = pipe.communicate()
        print
        print stdout
        sys.exit(0)
    else:
        pipe = subprocess.Popen([squeezy, test.input],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    output, errors = pipe.communicate()
    stoptime = time.time()
    deltatime = stoptime - starttime
    result = pipe.returncode
    if result < 0:
        raise ToolCrashError(output+errors)

    return (result,
            output.splitlines(True),
            errors.splitlines(True),
            deltatime)


if __name__ == "__main__":

    run_tests(srcdir, run_squeezy_command)


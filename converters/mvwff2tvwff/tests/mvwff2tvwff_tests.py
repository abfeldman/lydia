#!/usr/bin/env python2.4
#
# mvwff2tvwff_tests.py

""" Script for running lc tests. """

import sys
import os
import os.path
import subprocess

mvwff2tvwff = "../mvwff2tvwff"

srcdir = os.getenv("SRCDIR", "")
topsrcdir = os.getenv("TOPSRCDIR", "")

sys.path.append(os.path.join(topsrcdir, "testsuite"))

try:
    from lydia_tests import *
except ImportError:
    print "You must set the TOPSRCDIR environment variable"
    print "to the location of your Lydia source tree"
    sys.exit(1)


def run_mvwff2tvwff_command(test):

    if Config.echo:
        pipe = subprocess.Popen([mvwff2tvwff, test.input],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        stdout, _ = pipe.communicate()
        print
        print stdout
        sys.exit(0)
    else:
        pipe = subprocess.Popen([mvwff2tvwff, test.input],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    output, errors = pipe.communicate()
    result = pipe.returncode
    if result < 0:
        raise ToolCrashError(output+errors)

    return (result,
            output.splitlines(True),
            errors.splitlines(True))


if __name__ == "__main__":

    run_tests(srcdir, run_mvwff2tvwff_command)


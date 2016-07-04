#!/usr/bin/env python2.4
#
# lc_tests.py

""" Script for running lc tests. """

import sys
import os
import os.path
import subprocess

lc = "../lc"

srcdir = os.getenv("SRCDIR", "")
topsrcdir = os.getenv("TOPSRCDIR", "")

lc_lib = "-I" + os.path.join(topsrcdir, "components");

sys.path.append(os.path.join(topsrcdir, "testsuite"))

try:
    from lydia_tests import *
except ImportError:
    print "You must set the TOPSRCDIR environment variable"
    print "to the location of your Lydia source tree"
    sys.exit(1)


def run_lc_command(test):

    try:
	lc_src = "-I" + srcdir
        pipe = subprocess.Popen([lc, lc_lib, lc_src, test.model],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    except OSError, eeks:
        raise PipeOSError("%s %s" % (lc, test.model), eeks)
    output, errors = pipe.communicate()
    result = pipe.returncode
    if result < 0:
        raise ToolCrashError(output+errors)

    errors = errors.replace(test.model, os.path.basename(test.model))
    return (result,
            output.splitlines(True),
            errors.splitlines(True))


if __name__ == "__main__":

    run_tests(srcdir, run_lc_command)


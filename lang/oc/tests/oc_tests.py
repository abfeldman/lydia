#!/usr/bin/env python2.4
#
# oc_tests.py

""" Script for running oc tests. """

import sys
import os
import os.path
import subprocess

oc = "../oc"

srcdir = os.getenv("SRCDIR", "")
topsrcdir = os.getenv("TOPSRCDIR", "")

oc_lib = "-I" + os.path.join(topsrcdir, "components");

sys.path.append(os.path.join(topsrcdir, "testsuite"))

try:
    from lydia_tests import *
except ImportError:
    print "You must set the TOPSRCDIR environment variable"
    print "to the location of your Lydia source tree"
    sys.exit(1)


def run_oc_command(test):

    try:
	oc_src = "-I" + srcdir
        pipe = subprocess.Popen([oc, oc_lib, oc_src, test.model, test.observation],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    except OSError, eeks:
        raise PipeOSError("%s %s" % (oc, test.model), eeks)
    output, errors = pipe.communicate()
    result = pipe.returncode
    if result < 0:
        raise ToolCrashError(output + errors)

    errors = errors.replace(test.model, os.path.basename(test.model))
    return (result,
            output.splitlines(True),
            errors.splitlines(True))


if __name__ == "__main__":

    run_tests(srcdir, run_oc_command)


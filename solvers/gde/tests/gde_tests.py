#!/usr/bin/env python2.4
#
# gde_tests.py

""" Script for running gde tests. """


import sys
import os
import os.path
import tempfile
import subprocess
import time

gde = "../gde"

srcdir = os.getenv("SRCDIR", "")
topsrcdir = os.getenv("TOPSRCDIR", "")

sys.path.append(os.path.join(topsrcdir, "testsuite"))

try:
    from lydia_tests import *
except ImportError:
    print "You must set the TOPSRCDIR environment variable"
    print "to the location of your Lydia source tree"
    sys.exit(1)


def create_default_input(test):
    tmp_filename = tempfile.mktemp('-tmp.input',
                                   'gde-test-%s-' % os.path.basename(test.model),
                                   '/tmp/')
    tmp_file = open(tmp_filename, 'w')
    tmp_file.writelines(["fm\n", "quit\n"])
    tmp_file.close()
    return tmp_filename

def run_gde_command(test):

    temporary_infile = None
    try:
        infile = open(test.input)
    except AttributeError:
        temporary_infile = create_default_input(test)
        infile = open(temporary_infile)

    starttime = time.time()
    pipe = subprocess.Popen([gde, test.model, test.observation],
                            stdin=infile,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    output, errors = pipe.communicate()
    stoptime = time.time()
    deltatime = stoptime - starttime
    result = pipe.returncode
    if result < 0:
        raise ToolCrashError(output+errors)

    infile.close()

    try:
        os.remove(temporary_infile)
    except TypeError:
        pass

    errors = errors.replace(test.model, os.path.basename(test.model))
    return (result,
            output.splitlines(True),
            errors.splitlines(True),
            deltatime)

    
if __name__ == "__main__":

    run_tests(srcdir, run_gde_command)

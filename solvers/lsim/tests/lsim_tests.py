#!/usr/bin/env python2.4
#
# lsim_tests.py

""" Script for running lsim tests. """


import sys
import os
import os.path
import tempfile
import subprocess


lc = "../../../lc/lc"
smoothy = "../../../smoothy/smoothy"
lsim = "../lsim"

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
                                   'lsim-test-%s-' % os.path.basename(test.model),
                                   '/tmp/')
    tmp_file = open(tmp_filename, 'w')
    # We need to simulate "simulate to infinity or at least until
    # the queue is empty" here. We used to use "simulate inf",
    # but parsing 'inf' fails on cygwin. So we use 32000 as an
    # entirely arbitray "large enough" number.
    tmp_file.writelines(["simulate 32000\n", "quit\n"])
    tmp_file.close()
    return tmp_filename

def run_lsim_command(test):

    temporary_infile = None
    try:
        infile = open(test.input)
    except AttributeError:
        temporary_infile = create_default_input(test)
        infile = open(temporary_infile)

    pipe = subprocess.Popen([lsim, test.model, test.observation],
                            stdin=infile,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    output, errors = pipe.communicate()
    result = pipe.returncode
    if result < 0:
      raise PipeGoneError(result)

    infile.close()

    try:
        os.remove(temporary_infile)
    except TypeError:
        pass

    errors = errors.replace(test.model, os.path.basename(test.model))
    return (result,
            output.splitlines(True),
            errors.splitlines(True))

    
if __name__ == "__main__":

    run_tests(srcdir, run_lsim_command)

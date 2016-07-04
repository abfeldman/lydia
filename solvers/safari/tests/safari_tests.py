#!/usr/bin/env python2.4
#
# safari_tests.py

""" Script for running safari tests. """


import sys
import os
import os.path
import tempfile
import subprocess
import time


lc = "../../../../lc/lc"
lcm2wff = "../../lcm2wff/lcm2wff"
wff2cnf = "../../wff2cnf/wff2cnf" 
smoothy = "../../../../smoothy/smoothy"
safari = "../safari"


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
                                   'safari-test-%s-' % os.path.basename(test.model),
                                   '/tmp/')
    tmp_file = open(tmp_filename, 'w')
    tmp_file.writelines(["fm\n", "quit\n"])
    tmp_file.close()
    return tmp_filename


def preprocess_file(filename):
    """ Load a Lydia system model from 'filename' and convert it
    to CNF format suitable for inputting to safari."""
    

    cnf_filename = tempfile.mktemp('.tmp.cnf',
                                   'safari-test-%s' % os.path.basename(filename),
                                   '/tmp/')
    cnf_file = open(cnf_filename, 'w')

    filename = os.path.expanduser(filename)

    try:
        p1 = subprocess.Popen([lc, filename], stdout=subprocess.PIPE)
    except OSError, eeks:
        raise PipeOSError("lc", eeks)
    try:
        p2 = subprocess.Popen([lcm2wff], stdin=p1.stdout, stdout=subprocess.PIPE)
    except OSError, eeks:
        raise PipeOSError("lcm2wff", eeks)
    try:
        p3 = subprocess.Popen([wff2cnf], stdin=p2.stdout, stdout=subprocess.PIPE)
    except OSError, eeks:
        raise PipeOSError("wff2cnf", eeks)
    try:
        p4 = subprocess.Popen([smoothy], stdin=p3.stdout, stdout=cnf_file)
    except OSError, eeks:
        raise PipeOSError("smoothy", eeks)
    result = p1.wait()
    if result != 0:
        raise PipeError(result, ' '.join(["lc", filename]))
    result = p2.wait()
    if result != 0:
        raise PipeError(result, "lcm2wff")
    result = p3.wait()
    if result != 0:
        raise PipeError(result, "wff2cnf")
    result = p4.wait()
    if result != 0:
        raise PipeError(result, "smoothy")
    cnf_file.close()

    return cnf_filename


def run_safari_command(test):

    if test.model.endswith(".sys"):
        cnffilename = preprocess_file(test.model)
        delete_cnf = True
    else:
        cnffilename = test.model
        delete_cnf = False

    temporary_infile = None
    try:
        infile = open(test.input)
    except AttributeError:
        temporary_infile = create_default_input(test)
        infile = open(temporary_infile)

    starttime = time.time()
    if Config.echo:
        pipe = subprocess.Popen([safari, cnffilename],
                                stdin=infile,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        stdout, _ = pipe.communicate()
        print
        print stdout
        sys.exit(0)
    else:
        pipe = subprocess.Popen([safari, cnffilename],
                                stdin=infile,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    output, errors = pipe.communicate()
    stoptime = time.time()
    deltatime = stoptime - starttime
    result = pipe.returncode
    if result < 0:
        raise ToolCrashError(output+errors)

    if delete_cnf:
        os.remove(cnffilename)
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

    run_tests(srcdir, run_safari_command)

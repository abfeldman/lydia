# lydia_tests.py

""" A framework for running Lydia test suite scripts.

    Usage is explained in the README file, but probably best
    shown by example. Look in any Lydia tool test directory (e.g.
    lsim/tests, smoothy/tests, wwf2cnf/tests, etc. etc.), and
    find in the file <toolname>_tests.py the actual testing
    function that will be plugged into the framework defined by
    lydia_tests.py (this file).

    Note: We do not use Python's unittest support, because we are
    not testing Python code, but rather a large collection of
    filters and command-line utilities that process files and
    commands on the standard input and output. Also, we want the
    tests to be Makefile-driven, not Python-driven.
    
"""

import sys
import os.path
import difflib
import optparse

# Support for reading in the XML test config file.
import readconfig 


# Exceptions that can occur during testing.

class ToolCrashError(Exception):
    """ Shouldn't happen, but sometimes the underlying tool
    actually crashes."""
    def __init__(self, data):
        Exception.__init__(self)
        self._data = data
    def __str__(self):
        return "INTERNAL ERROR (tool crash):\n%s" % self._data

class PipeError(Exception):
    """ Problems with the usage of pipes and subprocesses."""
    def __init__(self, errcode, command):
        Exception.__init__(self)
        self._errcode = errcode
        self._command = command
    def __str__(self):
        return "INTERNAL ERROR (pipe error, code %d). Command: '%s'" % \
               (self._errcode, self._command)

class PipeOSError(Exception):
    """ Problems with starting up a pipe or subprocess."""
    def __init__(self, command, msg):
        Exception.__init__(self)
        self._command = command
        self._msg = msg
    def __str__(self):
        return "ERROR (could not create pipe). Command: '%s'  OS Error: %s" % \
               (self._command, self._msg)

class PipeGoneError(Exception):
    """ Problems with unepected exits of an established pipe."""
    def __init__(self, value):
        Exception.__init__(self)
        self._value = value
    def __str__(self):
        if self._value < 0:
            return "INTERNAL ERROR (child process terminated by signal). Signal: %s" % \
               (-self._value)
        else:
            return "INTERNAL ERROR (child process disappeared unexpectedly). Child exit value: %s" % \
               (self._value)


class Test(object):
    """ Stores a single Lydia test entry. This is just a simple
    record, a Python version of an XML test description. """

    def __init__(self):
        self.input = "default.input.cmd"
        self.result = 0

    def __str__(self):
        result = ""
        for key, value in self.__dict__.iteritems():
            result = result + "%s = %s\n" % (key, value)
        return result.strip()

def read_expected_output(test):
    """ Read the expected stdout output expected for Test 'test',
    from the file test.output. Returns a list of strings, one per
    line, or an empty list if the file does not exist. """

    try:
        outputfile = open(test.output)
    except IOError:
        expected_output = []
    else:
        expected_output = outputfile.readlines()
        outputfile.close()
    return expected_output


def read_expected_errors(test):
    """ Read the expected stderr error output expected for Test
    'test', as found in the file test.errors. Returns a list of
    strings, one per line, or an empty list if the file does not
    exist. """

    try:
        errorfile = open(test.errors)
    except IOError:
        expected_error = []
    else:
        expected_error = errorfile.readlines()
        errorfile.close()
    return expected_error


def errors_as_expected(test, errors):
    """ Returns True if the errors encountered on stderr during
    the execution of test 'Test' are identical to the expected
    ones listed in the error file specified in the test
    description. """

    testerror = errors

    if not getattr(test, "errors", False):
        if errors:
            if not Config.quiet:
                print
                print
                sys.stdout.writelines(errors)
                print
                print "  The test generated errors, but no errors file was"
                print "  specified in the test description."
            
            return False
        else:
            return True
        
    expectederror = read_expected_errors(test)
    if testerror == expectederror:
        return True

    testerrfilename = os.path.basename(test.errors) + "-test.err"
    errfile = open(testerrfilename, "w")
    errfile.writelines(errors)
    errfile.close()
    diff = difflib.unified_diff(testerror, expectederror,
                                testerrfilename, test.errors,
                                n=0)

    if not Config.quiet:
        print
        print
        sys.stdout.writelines(diff)
        print "  Test error output differs from expected error output"
        print "  If you think the test error output is correct, update with:"
        print "    mv -f %s %s" % (testerrfilename, test.errors)

    return False



def output_as_expected(test, output):

    testoutput = output

    if not getattr(test, "output", False):
        if output:
            if not Config.quiet:
                print
                print
                sys.stdout.writelines(output)
                print
                print "  The test generated output, but no no output file was"
                print "  specified in the test description."
            
            return False
        else:
            return True
    
    expectedoutput = read_expected_output(test)
    if testoutput == expectedoutput:
        return True

    testoutfilename = os.path.basename(test.output) + "-test.out"
    outfile = open(testoutfilename, "w")
    outfile.writelines(output)
    outfile.close()
    diff = difflib.unified_diff(testoutput, expectedoutput,
                                testoutfilename, test.output,
                                n=0)

    if not Config.quiet:
        print
        print
        sys.stdout.writelines(diff)
        print
        print "  The test output differs from the expected output."
        print "  If you think the test output is correct, update with:"
        print "    mv -f %s %s" % (testoutfilename, test.output)

    return False


def find_test(tests, title):
    for t in tests:
        if t.title == title:
            return t
    return None


def print_duration(duration):    
    hours = duration // 3600
    minutes = (duration % 3600) // 60
    seconds = (duration % 3600) % 60
    if hours:
        result = "%2d:%2d:%02d" \
                 % (hours, minutes, seconds)
    else:
        result = "%2d:%02d" \
                 % (minutes, seconds)
    return result


class Config:
    """ Hold configuration info supplied on or derived from
    command-line options."""
    echo = False
    quiet = False
    file = ""



def run_test(test, test_function):
    """ Run a Lydia test case."""

    run_string = "  Running '%s'... " % test.title
    print run_string,
    sys.stdout.flush()

    if test.skip:
        print "SKIP (because of <skip/> in test description)"
        return 0

    try:
        result = test_function(test)
        status,output,errors,duration = result
    except ValueError, eeks:
        # Not all test functions return duration info.
        status,output,errors = result
        duration = False
    except ToolCrashError, eeks:
        print eeks
        return 5
    except PipeGoneError, eeks:
        print eeks
        return 5
    except PipeError, eeks:
        print eeks
        return 5
    except PipeOSError, eeks:
        print eeks
        return 5

    if Config.echo:
        return 0

    if not errors_as_expected(test, errors):
        return 3

    if not output_as_expected(test, output):
        return 2

    max_string = 60
    padding = max_string - len(run_string)
    
    print padding * " " + "Ok",
    if duration:
        print "  [%s]" % print_duration(duration)
    else:
        print
    
    return 0



def run_tests(srcdir, test_function):

    parser = optparse.OptionParser(usage="lydia_tests.py [options] [test-title]")
    parser.add_option("-e", "--echo", action="store_true",
                      help="run test, don't compare, output to console")
    parser.add_option("-q", "--quiet", action="store_true",
                      help="minimum output -- just pass/fail")
    parser.add_option("-f", "--file", metavar="FILE", default="tests.xml",
                      help="use test configuration file FILE")
                                   
    opts, args = parser.parse_args(sys.argv[1:])

    # Turn the options into attributes of the Config class.
    for opt in vars(opts):
        setattr(Config, opt, getattr(opts, opt))

    if len(args) > 1:
        print "Too many arguments"
        print parser.get_usage()
        sys.exit(1)

    all_tests = readconfig.readconfig(Config.file, srcdir)
    if len(args) == 0:
        tests = all_tests

    if len(args) == 1:
        test_title = args[0]
        user_test = find_test(all_tests, test_title)        
        if user_test:
            tests = [user_test]        
        else:
            print "Unknown test title: '%s'" % test_title
            sys.exit(1)

    okay = True
    for test in tests:
        if isinstance(test, readconfig.Separator):
            print test.title
            continue
        result = run_test(test, test_function)
        if result != 0:
            okay = False
            break

    testname = test_function.__name__
    testname = testname.replace("run_", "")
    testname = testname.replace("_command", "")

    if okay:
        result_str = "%s: All tests in `%s' were passed successfully" % (testname, Config.file)
    else:
        result_str = "%s: Test suite `%s' failed to complete" % (testname, Config.file)

    if len(args) == 0:
        print len(result_str) * "="
        print result_str
        print len(result_str) * "="

    if okay:
        sys.exit(0)
    else:
        sys.exit(1)

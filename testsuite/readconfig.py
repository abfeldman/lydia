# readconfig.py

""" Read an XML configuration files describing a Lydia testsuite."""

import sys
import os.path
import xml.dom.minidom

class Test:
    def __str__(self):
        result = ""
        for key, value in self.__dict__.iteritems():
            result = result + "%s = %s\n" % (key, value)
        return result.strip()

class Separator:
    pass

def getText(nodelist):
    rc = ""
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc = rc + node.data
    return rc

def handleTestSuite(testsuite, path):
    testlist = []
    for n in filter(lambda x: x.nodeType == xml.dom.Node.ELEMENT_NODE,
                    testsuite.getElementsByTagName("testsuite")[0].childNodes):
        if n.tagName == "separator":
            testlist.append(handleSeparator(n, path))
            continue
        if n.tagName == "test":
            testlist.append(handleTest(n, path))
            continue
    return testlist

def handleSeparator(separator, path):

    result = Separator()
    result.title = getText(separator.childNodes)
    return result

def handleTest(test, path):

    result = Test()

    title = getText(test.getElementsByTagName("title")[0].childNodes)
    result.title = title

    try:
        model = getText(test.getElementsByTagName("model")[0].childNodes)
        result.model = os.path.join(path, model)
    except IndexError:
        pass
    
    try:
        inputfile = getText(test.getElementsByTagName("input")[0].childNodes)
        result.input = os.path.join(path, inputfile)
    except IndexError:
        pass

    try:
        outputfile = getText(test.getElementsByTagName("output")[0].childNodes)
        result.output = os.path.join(path, outputfile)
    except IndexError:
        pass

    try:
        errorsfile = getText(test.getElementsByTagName("errors")[0].childNodes)
        result.errors = os.path.join(path, errorsfile)
    except IndexError:
        pass

    try:
        description = getText(test.getElementsByTagName("description")[0].childNodes)
        result.description = description
    except IndexError:
        pass

    try:
        observation = getText(test.getElementsByTagName("observation")[0].childNodes)
        result.observation = os.path.join(path, observation)
    except IndexError:
        pass

    try:
        getText(test.getElementsByTagName("skip")[0].childNodes)
        result.skip = True
    except IndexError:
        result.skip = False

    return result


def readconfig(file, path=''):
    if path == "./":
        path = ""
    dom = xml.dom.minidom.parse(os.path.join(path, file))
    tests = handleTestSuite(dom, path)
    return tests


if __name__ == "__main__":
    try:
        filename = sys.argv[1]
    except IndexError:
        filename = 'tests.xml'
    tests = readconfig(filename)
    for test in tests:
        if isinstance(test, Separator):
            continue
        print test
        print "NEW Model (%s): %s" % (test.title, test.model)
        try:
            print "  Input: %s" % test.input
        except AttributeError:
            pass
        try:
            print "  Output: %s" % test.output
        except AttributeError:
            pass
        try:
            print "  Errors: %s" % test.errors
        except AttributeError:
            pass

#!/usr/bin/python

from __future__ import print_function

import sys
import CppHeaderParser

sys.path = ["../"] + sys.path

try:
    fixnumbers = CppHeaderParser.CppHeader("FixFieldNumbers.h")
    fixvalues = CppHeaderParser.CppHeader("FixValues.h")
except CppHeaderParser.CppParseError as e:
    print(e)
    sys.exit(1)

with open('fixenums.q', 'a') as f:
    for var in fixnumbers.finalize_vars.im_self.variables:
        print(".fix.tags.%s: %s" % (var['name'], var['default']), file = f);
        

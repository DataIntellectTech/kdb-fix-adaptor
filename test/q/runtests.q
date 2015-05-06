\l test/k4unit.q

.KU.VERBOSE:0;
.KU.DEBUG:0;

KUltf`:test/tests.csv;
KUrt[];

issues:count results:select timestamp, code, action, file from KUTR where not ok;

$[issues;
    -1 "\033[0;31mFAILURE in ", (string issues), " test(s):\033[1;37m\n\n",(.Q.s results),"\033[0m";
    -1 "\033[0;32mPASSED ",(string count KUTR), " tests without any issues\033[0m"];

exit issues;

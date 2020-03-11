#!/bin/bash

#Script to run kdb-fix-adaptor unit tests
#change to main directory
cd ..

#declare arrays of FIX configs
fixbegin=("FIX.4.2" "FIX.4.3" "FIX.4.4" "FIXT.1.1" "FIXT.1.1" "FIXT.1.1")
fixspec=("./spec/FIX42.xml" "./spec/FIX43.xml" "./spec/FIX44.xml" "./spec/FIX50.xml" "./spec/FIX50SP1.xml" "./spec/FIX50SP2.xml")
fixconf=("./tests/sessions/FIX42.ini" "./tests/sessions/FIX43.ini" "./tests/sessions/FIX44.ini" "./tests/sessions/FIX50.ini" "./tests/sessions/FIX50SP1.ini" "./tests/sessions/FIX50SP2.ini")

for i in {0..0};do
	export FIXBEGIN=${fixbegin[$i]}
	export FIXSPEC=${fixspec[$i]}
	export FIXCONF=${fixconf[$i]}
	
	nohup q ./tests/fixtest.q a -p 5500 >> ./tests/acceptor.log 2>&1 &
	sleep 2
	nohup q ./tests/fixtest.q i >> ./tests/initiator.log 2>&1 &
	wait
done
#run test script that will load k4unit and other process
#q ./tests/.q 

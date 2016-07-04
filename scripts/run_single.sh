#!/bin/sh
# First parameter is the name of tye system file that we want to experiment on
# Second parameter is the name of the observation that we want to run
# Third parameter is the name of the solver we want to use
# Forth parameter is the type of system file that is used (e.g., cnf)
# Fifth parameter is the type of observation file that is used (e.g., dnf)
# Six parameter is an optional parameter for the solver


python runDiag.py $1 obs $2 | ../build/solvers/$3 $6 ../examples/diagnosis/compiled/$1.$4 ../examples/diagnosis/compiled/$1.$5 

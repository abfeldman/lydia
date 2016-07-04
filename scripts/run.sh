# First parameter is the name of system file that we want to experiment on
# Second parameter is the name of the solver we want to use
# Third parameter is the type of system format (e.g. cnf)
# Forth parameter is the type of observation format (e.g. dnf)
# Fifth parameter is the an optional parameter for the solver

python runDiag.py $1 | ../build/solvers/$2 $5 ../examples/diagnosis/compiled/$1.$3 ../examples/diagnosis/compiled/$1.$4 

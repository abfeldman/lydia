lc $1.sys | lcm2wff | wff2cnf | smoothy | cnf2horn > $2/$1.horn
oc $2/$1.horn $1.obs | lcm2wff | wff2obdd | obdd2dnf > $2/$1.dnf 
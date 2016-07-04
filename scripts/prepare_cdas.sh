lc $1.sys | lcm2wff | wff2cnf | smoothy  > $2/$1.cnf
oc $2/$1.cnf $1.obs | lcm2wff | wff2obdd | obdd2dnf > $2/$1.dnf 
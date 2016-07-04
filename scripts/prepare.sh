lc $1.sys | lcm2wff | wff2cnf | smoothy > $1.cnf
oc $1.cnf $1.obs | lcm2wff | wff2obdd | obdd2dnf > $1.dnf 
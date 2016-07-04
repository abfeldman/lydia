lc $1.sys | lcm2wff | wff2obdd | obdd2dnf  > $2/$1.hdnf
oc $2/$1.hdnf $1.obs | lcm2wff | wff2obdd | obdd2dnf > $2/$1.dnf 
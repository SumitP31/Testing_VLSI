#!/bin/sh
set -eu
# Change these paths to match your BDD package installation.
BDD_INCLUDE=${BDD_INCLUDE:-../include}
BDD_LIB=${BDD_LIB:-../lib}
gcc -Wall -Wextra -O2 -o reachability_bdd reachability_bdd.c -I "$BDD_INCLUDE" -L "$BDD_LIB" -lbdd -lmem

#!/bin/sh
set -u

./sat_encode "$@"
for cnf in \
    property_a_counterexample.cnf \
    property_b_counterexample.cnf \
    property_c_counterexample.cnf \
    property_b_p_zero_counterexample.cnf
do
    minisat -verb=0 "$cnf"
    status=$?
    if [ "$status" -ne 10 ] && [ "$status" -ne 20 ]; then
        exit "$status"
    fi
done

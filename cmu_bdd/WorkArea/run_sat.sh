#!/bin/sh
set -eu
./sat_encode "$@"
solver=${SAT_SOLVER:-minisat}
for cnf in \
    property_a_counterexample.cnf \
    property_b_counterexample.cnf \
    property_c_counterexample.cnf \
    property_b_p_zero_counterexample.cnf
do
    echo "===== $cnf ====="
    "$solver" -verb=0 "$cnf" || status=$?
    status=${status:-0}
    if [ "$status" -ne 10 ] && [ "$status" -ne 20 ] && [ "$status" -ne 0 ]; then
        exit "$status"
    fi
    unset status
done

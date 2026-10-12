# EE 709 Assignment 1 — starter code

This project separates the BDD reachability work from the bounded SAT work.

## Files

- `part1_bdd/circuit_bdd.h`: shared declarations for the BDD circuit model.
- `part1_bdd/circuit_bdd.c`: output equations and transition relation.
- `part1_bdd/reachability.c`: image computation and reachable-state enumeration.
- `part2_sat/sat_encode.c`: emits DIMACS CNF for counterexample checks of (a), (b), (c), and (b) with `p=0`.
- `Makefile`: build targets.

## Important modeling issue to resolve

The assignment states `reset(0)=1` and `reset(k)=0` for `k>0`, but the extracted PDF text does not specify an initial-state set. The BDD starter currently uses `000` as an *illustrative* initial state (`INITIAL_STATE_MODE 0`). This is not asserted by the assignment and should be confirmed before treating results as the official answer. Setting `INITIAL_STATE_MODE 1` makes all eight states initially possible, which naturally makes the union of reachable states include all eight.

The SAT encoding leaves the state at cycle 0 unconstrained for the same reason. If the instructor specifies an initial state, add unit clauses for the initial state bits in `encode_trace`.

## Build

The BDD manual says programs should link with `-lbdd -lmem`. From this directory:

```sh
make bdd
make sat
```

If the library/header is not in the compiler's default search path, add the appropriate `-I/path/to/include` and `-L/path/to/lib` flags in the Makefile.

## Run

```sh
./reachability_bdd
./sat_encode
```

The SAT program writes four files:

- `property_a_counterexample.cnf`
- `property_b_counterexample.cnf`
- `property_c_counterexample.cnf`
- `property_b_p_zero_counterexample.cnf`

Run your course's DIMACS-compatible SAT solver on each generated file. The program generates CNF; it does not itself solve it.

## Interpreting SAT results

Each CNF encodes a counterexample to the property, not the property directly:

- SAT: a violating trace exists in the 8-cycle encoding.
- UNSAT: no violating trace exists in the 8-cycle encoding.

For (a) and (b), the finite-window interpretation used here treats a violation as an asserted request at cycle `t` with the corresponding output false for every cycle from `t` through cycle 7. If your course uses a different bounded-temporal-logic convention, revise this encoding accordingly.

## Verification before submission

This is a commented starter implementation, not a claim that the assignment has been fully validated against your course environment. Compile it with the provided `bdduser.h` installation, inspect generated CNF, run the intended SAT solver, and retain actual terminal logs for the report. Do not claim a solver result until the solver has been run.

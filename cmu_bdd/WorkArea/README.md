# EE 709 Assignment 1 — corrected code

This package corrects the output equation for `g1` to match the overbar in the assignment PDF:

- `g0 = p ? (q0 AND NOT q1) : q0`
- `g1 = p ? q1 : (q1 AND NOT q0)`

The next-state equations are encoded as shown in the PDF:

- `q0_next = NOT reset AND ((r0 AND NOT q0) OR (NOT g0 AND q0))`
- `q1_next = NOT reset AND ((r1 AND NOT q1) OR (NOT g1 AND q1))`
- `p_next  = NOT reset AND (p XOR (g0 OR g1))`

## Files

- `reachability_bdd.c`: BDD image computation and state enumeration. It starts from `(p,q0,q1)=000` as an explicit working assumption, because the assignment does not specify an initial-state set.
- `part2_sat.c`: creates DIMACS CNF files for the counterexample checks in Part 2. By default, the initial state is unconstrained, matching the absence of an initial-state condition in the PDF. Pass `--initial-zero` to constrain it to `000`.
- `compile_bdd.sh`: example compile command; edit include/library paths for your BDD installation.
- `compile_sat.sh`: compiles the SAT encoder.
- `run_sat.sh`: generates CNFs and calls `minisat` on them.

## Build and run

```sh
chmod +x compile_bdd.sh compile_sat.sh run_sat.sh
./compile_sat.sh
./run_sat.sh
```

For the BDD program, set the include/library paths in `compile_bdd.sh` to your `bdduser.h`, `libbdd`, and `libmem` installation, then run:

```sh
./compile_bdd.sh
./reachability_bdd
```

The SAT files encode *counterexamples*:

- Property (a): SAT means a bounded trace violates `AG(r0 -> F g0)`; UNSAT means no such counterexample exists in the encoded eight-cycle window.
- Property (b): SAT means a bounded trace violates `AG(r1 -> F g1)`.
- Property (c): SAT means a trace reaches a cycle with `g0 AND g1`; this is a counterexample to `NOT EF(g0 AND g1)`.
- Modified property (b): SAT means a counterexample exists when `p` is forced to zero.

For properties (a) and (b), the encoder disallows requests at cycle 7 because the trace contains no later sample in which to observe a response. Requests at cycles 0..6 must be answered by the corresponding output by cycle 7. This is an explicit bounded-response convention; interpret SAT as a counterexample within this window, not automatically as proof of an unbounded liveness failure.

**Initial-state note:** The assignment PDF gives the reset schedule but does not explicitly specify the initial state. The BDD program uses `000` as an explicit starting assumption; the SAT encoder defaults to an unconstrained cycle-0 state. Make these assumptions consistent with your instructor's interpretation before presenting results as directly comparable.

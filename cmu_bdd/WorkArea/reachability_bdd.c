/*
 * EE 709 Assignment 1, Part 1: BDD reachability model.
 *
 * State: (p,q0,q1). Inputs: (reset,r0,r1).
 * The assignment gives reset(0)=1 and reset(k)=0 for k>0, but no initial
 * state set. For this run, assume q0=q1=p=0 initially.
 * It prints the reachable state set at each cycle and stops once every state
 * valuation has been encountered.
 *
 * Build using the supplied ./compile.sh.
 */
#include <stdio.h>
#include <bdduser.h>

#define EQ(a, b) bdd_not(bddm, bdd_xor(bddm, (a), (b)))

static bdd state_cube(bdd_manager bddm, bdd *s, int p, int q0, int q1)
{
    return bdd_and(bddm,
        p ? s[0] : bdd_not(bddm, s[0]),
        bdd_and(bddm,
            q0 ? s[1] : bdd_not(bddm, s[1]),
            q1 ? s[2] : bdd_not(bddm, s[2])));
}

/* Image(S)(next_state) = exists current_state,inputs . (S AND T),
   followed by simultaneous renaming next-state variables to state vars. */
static bdd image(bdd_manager bddm, bdd states, bdd transition,
                 int eliminate_current_and_inputs, int rename_next_to_state)
{
    bdd quantified;
    bdd_assoc(bddm, eliminate_current_and_inputs);
    quantified = bdd_exists(bddm, bdd_and(bddm, states, transition));
    bdd_assoc(bddm, rename_next_to_state);
    return bdd_substitute(bddm, quantified);
}

int main(void)
{
    bdd_manager bddm = bdd_init();
    bdd s[3], n[3], u[3];
    bdd g0, g1, f_p, f_q0, f_q1, transition;
    bdd eq_p, eq_q0, eq_q1, frontier, next_frontier, seen, cube;
    bdd quantify[7], rename_pairs[7];
    int quantify_assoc, rename_assoc;
    int i, p, q0, q1, cycle;

    /* Current state, next state, then inputs reset,r0,r1. */
    for (i = 0; i < 3; ++i) s[i] = bdd_new_var_last(bddm);
    for (i = 0; i < 3; ++i) n[i] = bdd_new_var_last(bddm);
    for (i = 0; i < 3; ++i) u[i] = bdd_new_var_last(bddm);

    /* Outputs from the assignment (overbars are Boolean complements). */
    g0 = bdd_ite(bddm, s[0],
                 bdd_and(bddm, s[1], bdd_not(bddm, s[2])), s[1]);
    /* g1 = p ? q1 : (q1 AND NOT q0). */
    g1 = bdd_ite(bddm, s[0], s[2],
                 bdd_and(bddm, s[2], bdd_not(bddm, s[1])));

    /*
     * Print the general ROBDDs for the two output functions.
     * These depend on the current state variables (p,q0,q1), not on
     * the reachability frontier at any particular cycle.
     */
    printf("ROBDD for g0:\n");
    bdd_print_bdd(bddm, g0, NULL, NULL, NULL, stdout);
    printf("----------------------------------------------------\n");
    
    printf("ROBDD for g1:\n");
    bdd_print_bdd(bddm, g1, NULL, NULL, NULL, stdout);
    printf("----------------------------------------------------\n");

    /* Next-state functions. The PDF has reset complemented, and also
       complements q0/q1 and g0/g1 in the corresponding product terms. */
    f_q0 = bdd_and(bddm, bdd_not(bddm, u[0]),
        bdd_or(bddm,
            bdd_and(bddm, u[1], bdd_not(bddm, s[1])),
            bdd_and(bddm, bdd_not(bddm, g0), s[1])));
    f_q1 = bdd_and(bddm, bdd_not(bddm, u[0]),
        bdd_or(bddm,
            bdd_and(bddm, u[2], bdd_not(bddm, s[2])),
            bdd_and(bddm, bdd_not(bddm, g1), s[2])));
    f_p = bdd_and(bddm, bdd_not(bddm, u[0]),
        bdd_xor(bddm, s[0], bdd_or(bddm, g0, g1)));

    /* T(s,u,n): each next-state variable equals its next-state function. */
    eq_p = EQ(n[0], f_p);
    eq_q0 = EQ(n[1], f_q0);
    eq_q1 = EQ(n[2], f_q1);
    transition = bdd_and(bddm, eq_p, bdd_and(bddm, eq_q0, eq_q1));

    /* Initial state (p,q0,q1)=(0,0,0), i.e. all state bits are zero. */
    frontier = state_cube(bddm, s, 0, 0, 0);
    seen = bdd_zero(bddm);

    /* Existentially eliminate current state and all three inputs. */
    for (i = 0; i < 3; ++i) quantify[i] = s[i];
    for (i = 0; i < 3; ++i) quantify[i + 3] = u[i];
    quantify[6] = 0;
    quantify_assoc = bdd_new_assoc(bddm, quantify, 0);

    /* Simultaneous substitution n0->s0, n1->s1, n2->s2. */
    for (i = 0; i < 3; ++i) {
        rename_pairs[2 * i] = n[i];
        rename_pairs[2 * i + 1] = s[i];
    }
    rename_pairs[6] = 0;
    rename_assoc = bdd_new_assoc(bddm, rename_pairs, 1);

    printf("Assignment FSM: state=(p,q0,q1), inputs=(reset,r0,r1)\n");
    printf("Initial-state assumption: (p,q0,q1)=(0,0,0)\n");

    /* At cycle 0, reset=1; at every later transition, reset=0. */
    for (cycle = 0; ; ++cycle) {
        printf("Cycle %d: {", cycle);
        {
            int first = 1;
        for (p = 0; p <= 1; ++p)
            for (q0 = 0; q0 <= 1; ++q0)
                for (q1 = 0; q1 <= 1; ++q1) {
                    cube = state_cube(bddm, s, p, q0, q1);
                    if (bdd_intersects(bddm, frontier, cube) != bdd_zero(bddm)) {
                        printf("%s%d%d%d", first ? "" : ", ", p, q0, q1);
                        first = 0;
                    }
                }
        }
        printf("}\n");
        seen = bdd_or(bddm, seen, frontier);
        if (seen == bdd_one(bddm)) {
            printf("All 8 states reached by cycle %d.\n", cycle);
            break;
        }

        {
            bdd reset_constraint = (cycle == 0) ? u[0] : bdd_not(bddm, u[0]);
            next_frontier = image(bddm, frontier,
                bdd_and(bddm, transition, reset_constraint),
                quantify_assoc, rename_assoc);
            frontier = next_frontier;
        }
    }

    bdd_quit(bddm);
    return 0;
}

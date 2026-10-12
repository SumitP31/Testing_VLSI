/*
 * EE 709 Assignment 1, Part 2: bounded SAT counterexample encoder.
 *
 * Generates four DIMACS CNFs for an 8-state-sample window (cycles 0..7):
 *   property_a_counterexample.cnf       violation of AG(r0 -> F g0)
 *   property_b_counterexample.cnf       violation of AG(r1 -> F g1)
 *   property_c_counterexample.cnf       witness for EF(g0 AND g1)
 *   property_b_p_zero_counterexample.cnf violation of AG(r1 -> F g1), p=0
 *
 * By default state at cycle 0 is unconstrained, since the assignment does
 * not specify an initial-state set. Pass --initial-zero to constrain it to
 * (p,q0,q1)=000. Input reset is fixed to 1 at cycle 0 and 0 at cycles 1..7;
 * r0 and r1 are unconstrained.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HORIZON 8

typedef struct {
    int *lit;
    int count;
} Clause;

typedef struct {
    int variables;
    int clauses;
    int capacity;
    Clause *items;
} CNF;

typedef struct {
    int p[HORIZON];
    int q0[HORIZON];
    int q1[HORIZON];
    int r0[HORIZON];
    int r1[HORIZON];
    int reset[HORIZON];
    int g0[HORIZON];
    int g1[HORIZON];
} Trace;

static int new_var(CNF *cnf)
{
    return ++cnf->variables;
}

static void add_clause(CNF *cnf, int count, ...)
{
    va_list ap;
    Clause *clause;
    int i;

    if (cnf->clauses == cnf->capacity) {
        int new_capacity = cnf->capacity ? cnf->capacity * 2 : 128;
        Clause *new_items = (Clause *)realloc(cnf->items,
            (size_t)new_capacity * sizeof(Clause));
        if (!new_items) {
            fprintf(stderr, "out of memory while growing CNF\n");
            exit(2);
        }
        cnf->items = new_items;
        cnf->capacity = new_capacity;
    }

    clause = &cnf->items[cnf->clauses++];
    clause->count = count;
    clause->lit = (int *)malloc((size_t)count * sizeof(int));
    if (!clause->lit) {
        fprintf(stderr, "out of memory while adding clause\n");
        exit(2);
    }
    va_start(ap, count);
    for (i = 0; i < count; ++i)
        clause->lit[i] = va_arg(ap, int);
    va_end(ap);
}

static void unit(CNF *cnf, int lit)
{
    add_clause(cnf, 1, lit);
}

/* Return z <-> (a AND b), where a and b may be signed literals. */
static int gate_and(CNF *cnf, int a, int b)
{
    int z = new_var(cnf);
    add_clause(cnf, 3, -a, -b, z);
    add_clause(cnf, 2, a, -z);
    add_clause(cnf, 2, b, -z);
    return z;
}

/* Return z <-> (a OR b), where a and b may be signed literals. */
static int gate_or(CNF *cnf, int a, int b)
{
    int z = new_var(cnf);
    add_clause(cnf, 3, a, b, -z);
    add_clause(cnf, 2, -a, z);
    add_clause(cnf, 2, -b, z);
    return z;
}

/* Return z <-> (a XOR b), where a and b may be signed literals. */
static int gate_xor(CNF *cnf, int a, int b)
{
    int z = new_var(cnf);
    add_clause(cnf, 3, a, b, -z);
    add_clause(cnf, 3, -a, -b, -z);
    add_clause(cnf, 3, a, -b, z);
    add_clause(cnf, 3, -a, b, z);
    return z;
}

static void equivalent(CNF *cnf, int a, int b)
{
    add_clause(cnf, 2, -a, b);
    add_clause(cnf, 2, a, -b);
}

static int and_all(CNF *cnf, const int *lits, int count)
{
    int result = lits[0];
    int i;
    for (i = 1; i < count; ++i)
        result = gate_and(cnf, result, lits[i]);
    return result;
}

static int or_all(CNF *cnf, const int *lits, int count)
{
    int result = lits[0];
    int i;
    for (i = 1; i < count; ++i)
        result = gate_or(cnf, result, lits[i]);
    return result;
}

static void build_trace(CNF *cnf, Trace *tr, int initial_zero, int p_zero)
{
    int t;
    for (t = 0; t < HORIZON; ++t) {
        tr->p[t] = new_var(cnf);
        tr->q0[t] = new_var(cnf);
        tr->q1[t] = new_var(cnf);
        tr->r0[t] = new_var(cnf);
        tr->r1[t] = new_var(cnf);
        tr->reset[t] = new_var(cnf);
        tr->g0[t] = new_var(cnf);
        tr->g1[t] = new_var(cnf);
        /* reset(0)=1; reset(k)=0 for k>0. */
        unit(cnf, t == 0 ? tr->reset[t] : -tr->reset[t]);
        /* Do not initiate a new request on the final sampled cycle: the
           bounded window has no later cycle in which to observe its response.
           Requests at cycles 0..6 must be answered by the end of cycle 7. */
        if (t == HORIZON - 1) {
            unit(cnf, -tr->r0[t]);
            unit(cnf, -tr->r1[t]);
        }
        if (initial_zero && t == 0) {
            unit(cnf, -tr->p[t]);
            unit(cnf, -tr->q0[t]);
            unit(cnf, -tr->q1[t]);
        }
        if (p_zero)
            unit(cnf, -tr->p[t]);
    }

    for (t = 0; t < HORIZON; ++t) {
        int term0, term1, g0_inner, g1_inner;
        int q0_update_a, q0_update_b, q1_update_a, q1_update_b;

        /* g0 = p ? (q0 AND NOT q1) : q0
           g1 = p ? q1 : (q1 AND NOT q0). */
        g0_inner = gate_or(cnf, -tr->p[t], -tr->q1[t]);
        equivalent(cnf, tr->g0[t], gate_and(cnf, tr->q0[t], g0_inner));
        g1_inner = gate_or(cnf, tr->p[t], -tr->q0[t]);
        equivalent(cnf, tr->g1[t], gate_and(cnf, tr->q1[t], g1_inner));

        if (t == HORIZON - 1)
            continue; /* outputs at cycle 7 are needed; no cycle-8 state */

        {
            int q0_next = new_var(cnf);
            int q1_next = new_var(cnf);
            int next_and_not_reset = -tr->reset[t]; /* overbar(reset) */

            /* q0+ = !reset AND (r0*!q0 OR !g0*q0). */
            q0_update_a = gate_and(cnf, tr->r0[t], -tr->q0[t]);
            q0_update_b = gate_and(cnf, -tr->g0[t], tr->q0[t]);
            term0 = gate_or(cnf, q0_update_a, q0_update_b);
            equivalent(cnf, q0_next,
                       gate_and(cnf, next_and_not_reset, term0));

            /* q1+ = !reset AND (r1*!q1 OR !g1*q1). */
            q1_update_a = gate_and(cnf, tr->r1[t], -tr->q1[t]);
            q1_update_b = gate_and(cnf, -tr->g1[t], tr->q1[t]);
            term1 = gate_or(cnf, q1_update_a, q1_update_b);
            equivalent(cnf, q1_next,
                       gate_and(cnf, next_and_not_reset, term1));

            equivalent(cnf, tr->q0[t + 1], q0_next);
            equivalent(cnf, tr->q1[t + 1], q1_next);
            if (!p_zero) {
                int p_xor_arg = gate_or(cnf, tr->g0[t], tr->g1[t]);
                int p_update = gate_xor(cnf, tr->p[t], p_xor_arg);
                int p_next = gate_and(cnf, next_and_not_reset, p_update);
                equivalent(cnf, tr->p[t + 1], p_next);
            }
        }
    }
}

static void add_request_miss(CNF *cnf, Trace *tr, int request_is_r0)
{
    int terms[HORIZON];
    int t, j;

    /*
     * For the property AG(r0 -> F g0), the counterexample condition is
     * r0=1 at the selected starting cycle and g0 remains 0 thereafter.
     * Here the user's intended assumption is stronger: r0 is held high
     * throughout this bounded trace. Property (b) analogously holds r1 high.
     */
    for (t = 0; t < HORIZON; ++t) {
        int chain[HORIZON - t + 1];
        int count = 0;
        chain[count++] = request_is_r0 ? tr->r0[t] : tr->r1[t];
        for (j = t; j < HORIZON; ++j)
            chain[count++] = request_is_r0 ? -tr->g0[j] : -tr->g1[j];
        terms[t] = and_all(cnf, chain, count);
    }

    /* Keep the corresponding request asserted at every sampled cycle. */
    for (t = 0; t < HORIZON; ++t)
        unit(cnf, request_is_r0 ? tr->r0[t] : tr->r1[t]);

    unit(cnf, or_all(cnf, terms, HORIZON));
}

static void add_simultaneous_outputs(CNF *cnf, Trace *tr)
{
    int terms[HORIZON];
    int t;
    for (t = 0; t < HORIZON; ++t)
        terms[t] = gate_and(cnf, tr->g0[t], tr->g1[t]);
    unit(cnf, or_all(cnf, terms, HORIZON));
}

static void write_cnf(const char *path, CNF *cnf)
{
    FILE *fp = fopen(path, "w");
    int i, j;
    if (!fp) {
        perror(path);
        exit(2);
    }
    fprintf(fp, "p cnf %d %d\n", cnf->variables, cnf->clauses);
    for (i = 0; i < cnf->clauses; ++i) {
        for (j = 0; j < cnf->items[i].count; ++j)
            fprintf(fp, "%d ", cnf->items[i].lit[j]);
        fprintf(fp, "0\n");
    }
    fclose(fp);
}

static void free_cnf(CNF *cnf)
{
    int i;
    for (i = 0; i < cnf->clauses; ++i)
        free(cnf->items[i].lit);
    free(cnf->items);
}


static int model_value(const int *values, int lit)
{
    int v = values[abs(lit)];
    return lit > 0 ? v : !v;
}

static void print_trace(const char *label, const Trace *tr,
                        const int *values)
{
    int t;
    printf("\nCOUNTEREXAMPLE / WITNESS: %s\n", label);
    printf("cycle | reset r0 r1 | p q0 q1 | g0 g1\n");
    printf("------+-------------+---------+------\n");
    for (t = 0; t < HORIZON; ++t) {
        printf("  %d   |   %d   %d  %d | %d  %d  %d |  %d  %d\n",
               t,
               model_value(values, tr->reset[t]),
               model_value(values, tr->r0[t]),
               model_value(values, tr->r1[t]),
               model_value(values, tr->p[t]),
               model_value(values, tr->q0[t]),
               model_value(values, tr->q1[t]),
               model_value(values, tr->g0[t]),
               model_value(values, tr->g1[t]));
    }
}

/*
 * Run MiniSat if it is installed. MiniSat writes SAT/UNSAT and a model
 * to the requested output file. A SAT model is decoded using the variable
 * IDs stored in Trace, so the output is a concrete cycle-by-cycle trace.
 */
static void solve_and_print(const char *cnf_path, const char *label,
                            const CNF *cnf, const Trace *tr)
{
    char model_path[256];
    char command[600];
    FILE *fp;
    char status[32];
    int *values;
    int lit;

    snprintf(model_path, sizeof(model_path), "%s.model", cnf_path);
    snprintf(command, sizeof(command),
             "minisat \"%s\" \"%s\" > /dev/null 2>&1",
             cnf_path, model_path);
    {
        int solver_status = system(command);
        if (solver_status == -1) {
            perror("Failed to launch MiniSat");
            printf("Could not execute MiniSat for %s.\n", cnf_path);
            return;
        }
    }

    fp = fopen(model_path, "r");
    if (!fp) {
        printf("Solver output unavailable for %s. Install MiniSat and run:\n"
               "  minisat %s %s\n",
               cnf_path, cnf_path, model_path);
        return;
    }

    if (fscanf(fp, "%31s", status) != 1) {
        fclose(fp);
        printf("Could not read solver result for %s.\n", cnf_path);
        return;
    }

    if (strcmp(status, "UNSAT") == 0 ||
        strcmp(status, "UNSATISFIABLE") == 0) {
        printf("\nNO COUNTEREXAMPLE FOUND: %s is UNSAT within cycles 0..7.\n",
               label);
        fclose(fp);
        return;
    }

    if (strcmp(status, "SAT") != 0 &&
        strcmp(status, "SATISFIABLE") != 0) {
        printf("Unrecognized MiniSat result '%s' for %s.\n", status, cnf_path);
        fclose(fp);
        return;
    }

    values = (int *)calloc((size_t)cnf->variables + 1, sizeof(int));
    if (!values) {
        fclose(fp);
        fprintf(stderr, "out of memory while reading SAT model\n");
        exit(2);
    }

    while (fscanf(fp, "%d", &lit) == 1) {
        int var;
        if (lit == 0)
            continue;
        var = abs(lit);
        if (var <= cnf->variables)
            values[var] = (lit > 0) ? 1 : 0;
    }
    fclose(fp);

    print_trace(label, tr, values);
    printf("Interpretation: this trace satisfies the encoded bounded "
           "counterexample condition.\n");
    free(values);
}

static void generate(const char *path, int query, int initial_zero)
{
    CNF cnf = {0, 0, 0, NULL};
    Trace tr;
    build_trace(&cnf, &tr, initial_zero, query == 3);

    if (query == 0) add_request_miss(&cnf, &tr, 1);
    if (query == 1 || query == 3) add_request_miss(&cnf, &tr, 0);
    if (query == 2) add_simultaneous_outputs(&cnf, &tr);

    write_cnf(path, &cnf);
    printf("Wrote %-42s (%d variables, %d clauses)\n",
           path, cnf.variables, cnf.clauses);

    {
        const char *labels[] = {
            "property (a): request r0 is not followed by g0",
            "property (b): request r1 is not followed by g1",
            "property (c): simultaneous outputs g0 AND g1 (violates !EF)",
            "property (b) with p=0: request r1 is not followed by g1"
        };
        solve_and_print(path, labels[query], &cnf, &tr);
    }
    free_cnf(&cnf);
}

int main(int argc, char **argv)
{
    int initial_zero = 0;
    if (argc == 2 && strcmp(argv[1], "--initial-zero") == 0)
        initial_zero = 1;
    else if (argc != 1) {
        fprintf(stderr, "usage: %s [--initial-zero]\n", argv[0]);
        return 2;
    }

    printf("8-cycle counterexample CNFs; initial state: %s\n",
           initial_zero ? "000" : "unconstrained");
    generate("property_a_counterexample.cnf", 0, initial_zero);
    generate("property_b_counterexample.cnf", 1, initial_zero);
    generate("property_c_counterexample.cnf", 2, initial_zero);
    generate("property_b_p_zero_counterexample.cnf", 3, initial_zero);
    return 0;
}

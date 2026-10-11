#include <stdlib.h>
#include <stdio.h>
#include <bdduser.h>

int main (int argc, char* argv[])
{
	bdd_manager bddm = bdd_init();	

	// declare variables
	bdd x1 = bdd_new_var_last(bddm);
	bdd x2 = bdd_new_var_last(bddm);
	bdd x3 = bdd_new_var_last(bddm);
	bdd y1 = bdd_new_var_last(bddm);
	bdd y2 = bdd_new_var_last(bddm);

	
	//declare negations
	bdd x3_  = bdd_not (bddm,x3);
	bdd x1_  = bdd_not (bddm,x1);
	
	//declaring f
	bdd a = bdd_or (bddm,x3,x2);
	a = bdd_and (bddm,a,x1_);
	a = bdd_xnor (bddm,a,y1);
	bdd b = bdd_or (bddm,x1,x2);
	b = bdd_and (bddm,x3_,b);
	b = bdd_xnor (bddm,b,y2);
	bdd f = bdd_and (bddm,a,b);
	bdd c = bdd_or (bddm, x1,x2);
	c = bdd_or (bddm, x3,c);
	bdd d = bdd_xor(y1,y2);
	bdd f1 = bdd_and(bddm,f,c);
	bdd f2 = bdd_and(bddm,f,d);
	
	//creating new variable association x1,x2,x3,x4
	bdd Q[4];
	Q[0] = x1; Q[1] = x2; Q[2] = x3; Q[3] = 0;
	int assoc = bdd_new_assoc(bddm,Q,0);
	bdd_assoc(bddm,assoc);

	bdd exists = bdd_exists(bddm,f1);
	bdd_print_bdd(bddm,exists,NULL, NULL,NULL, stdout);
	
	bdd Q[3];
	Q[0] = y1; Q[1] = y2; Q[2] = 0;
	int assoc = bdd_new_assoc(bddm,Q,0);
	bdd_assoc(bddm,assoc);

	bdd exists = bdd_exists(bddm,f2);
	bdd_print_bdd(bddm,exists,NULL, NULL,NULL, stdout)
	return(0);
}

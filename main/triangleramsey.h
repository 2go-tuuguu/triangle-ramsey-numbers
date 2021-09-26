/**
 * triangleramsey.h
 *
 * TriangleRamsey: a generator for maximimal triangle-free graphs. 
 * This generator can also be used to generate triangle Ramsey graphs 
 * for a given testgraph efficiently.
 *
 * The latest version of TriangleRamsey can be found here:
 * http://caagt.ugent.be/triangleramsey/
 *
 * Author: Jan Goedgebeur (jan.goedgebeur@ugent.be)
 * In collaboration with Gunnar Brinkmann (gunnar.brinkmann@ugent.be)
 *
 * ----------------------------------------
 * 
 * Copyright (c) 2012 Ghent University
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TRIANGLERAMSEY_H
#define	TRIANGLERAMSEY_H

/********************************DEFINES***************************************/

/******************************Configuration***********************************/

//Number of times that the program tries to reopen a file if fopen fails
#define NUMTRIES_FOPEN 5

//If the number of vertices is large enough, the splitlevel should be at least MIN_SPLITLEVEL
//Value 18 seems to be quite good for this (common part takes about 6 secs, 
//and about 15 sec when using 'ramsey' option)
#define MIN_SPLITLEVEL 18


//Configuration for the 'ramsey' option:

/* 
 * Instead of constructing all sets for supergraphset, first test if one of the
 * NUM_PREV_SETS previous subgraph sets is not a subgraph.
 * 100 seems to be a good trade-off for this.
 */
#define NUM_PREV_SETS 100

//The max num almost good sets which are saved
//These are stored in good_sets_temp[]
#define MAX_NUM_GOOD_SETS_TEMP (3*NUM_PREV_SETS)

//How many vertices a good set from good_sets_temp[] may have in common with the
//previous set in good_sets_temp[]
#define SETS_TEMP_MAX_NUM_COMMON 5

//Min number of previous sets which can be added to num_good_sets_v[]
#define MIN_NUM_PREV_SETS 10

//The max number of induced subgraphs of testgraph_compl_v which are saved.
//Important: must be larger than MIN_NUM_PREV_SETS, since using if(num_good_sets_v[nv] == MAX_NUM_GOOD_SETS_V - MIN_NUM_PREV_SETS)
#define MAX_NUM_GOOD_SETS_V (NUM_PREV_SETS / 2)


/***************You should not change anything below this line*****************/

/*********************************Typedefs*************************************/

//The maximum number of vertices
#define MAXN1 WORDSIZE
#define MAXM1 1 //Do not change this!

typedef unsigned char GRAPH[MAXN1][MAXN1];
typedef unsigned char ADJACENCY[MAXN1];

typedef struct gg { 
             GRAPH g;                 /*  The adjacency lists of the graph */
	     ADJACENCY adj;           /*  Degrees of each vertex */
        } GRAPH_STRUCT;

typedef unsigned char EDGE[2];

/*******************************Other defines**********************************/

//For the timing
#define time_factor sysconf(_SC_CLK_TCK)        

/*****************************Some useful macros*******************************/

//Is useful if x is often 0
#define P_POPCOUNT(x) ((x) ? POPCOUNT(x) : 0)

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
        
//size of multicode is nv + ne        
#define MAX_MUTLICODE_SIZE(a) ((a) + (a)*((a)-1)/2)
        
/* Some marks */

#define MAXVAL (INT_MAX - 1)

static int markvalue = MAXVAL;
unsigned int marks[MAXN1];
#define RESETMARKS {int mki; if ((markvalue += 1) > MAXVAL) \
      { markvalue = 1; for (mki=0;mki<MAXN1;++mki) marks[mki]=0;}}
#define MARK(v) marks[v] = markvalue
//#define UNMARK(v) marks[v] = markvalue - 1
#define ISMARKED(v) (marks[v] == markvalue)

        
/****************************Global variables**********************************/

GRAPH_STRUCT current_graph;

static int nv; /* Number of vertices of the current graph */

static int maxnv; /* The maximum number of vertices */

/* For nauty */
/* Nauty worksize */

//For graphs which can have at most MAXN1 vertices

#define WORKSIZE (50 * MAXM1)

int lab[MAXN1], ptn[MAXN1], orbits[MAXN1];
static DEFAULTOPTIONS_GRAPH(options);
statsblk stats;
setword workspace[WORKSIZE];

//TODO: in the future one could always use defaultptn as this doesn't make much difference
static int use_defaultptn = 1;

graph nautyg[MAXN1*MAXM1];
graph nautyg_canon[MAXN1*MAXM1];
graph nautyg_loops[MAXN1*MAXM1];

permutation generators[MAXN1][MAXN1];
int number_of_generators;


//For graphs which can have more than MAXN1 vertices

#define WORKSIZEBIG (50 * MAXM)

int lab_big[MAXN], ptn_big[MAXN], orbits_big[MAXN];
static DEFAULTOPTIONS_GRAPH(options_big);
statsblk stats_big;
setword workspace_big[WORKSIZEBIG];

graph nautyg_big[MAXN*MAXM];
graph nautyg_big_canon[MAXN*MAXM];


/* A dynamically expandable list of dominating sets */
#define DEFAULT_MAX_DOM_SETS_SIZE 512

ADJACENCY *dominating_sets_global;
int *dominating_sets_size_global;
static int number_of_dominating_sets_global = 0;
int max_dominating_sets_global = DEFAULT_MAX_DOM_SETS_SIZE;

/* The neighbourhoods of the double vertices */
static setword dv_neighbourhoods[MAXN1];
static int num_dv_neighbourhoods = 0;

//The degree of the double vertex with the largest degree
static int max_degree_dv = 0;

//The double vertices which have a certain neighbourhood
static unsigned char double_vertices[MAXN1][MAXN1];
static int num_double_vertices[MAXN1];

static int total_num_double_vertices;

//The canonical removable vertex from the parent graph (if it exists)
//is always nv - 1
//probably no longer needed since saving removable vertices??
static unsigned char last_removable_vertex;

static unsigned char removable_vertices[MAXN1];
static int num_removable_vertices = 0;

static setword removable_vertices_bitvector = 0;

//The removable vertex which caused the rejection in is_best_dom_set_no_remove_reduction()
static unsigned char previous_best_removable_vertex;

static int num_edges_removed = 0;
static EDGE removed_edge;


/* Parameters of the program */
static int outputlevel; /* Only outputs graphs with >= outputlevel vertices */
static int noout = 0; /* If noout == 1, the generated graphs arent output, but only counted */

/* Specific parameters for modulo */
static int modulo = 1;
static int rest = 0;
static int splitcounter = 0;
static int splitlevel = 0; /* For modolo, determines at what level/depth of the recursion tree the calculation should be split */

/* Specific parameters for ramsey */
static int ramsey = 0;
static int write_all_ramseygraphs = 0;
static int ramseygraph_found = 0;

//The name of the file which contains the testgraph (for option "ramsey")
static char *testgraph_filename = NULL;

//The name of the file which contains the inputgraphs (for option "files")
static char *inputgraphs_filename = NULL;
static int number_of_inputgraphs_read = 0;

/* Name of the file where the ramseygraphs are written to */
#define MAX_FILENAME_LENGTH 200 
static char ramseygraphs_filename[MAX_FILENAME_LENGTH];


static unsigned long long int number_of_graphs_generated[MAXN1 + 1] = {0};
static unsigned long long int number_of_ramsey_graphs_generated[MAXN1 + 1] = {0};
static unsigned long long int number_of_graphs_output = 0;

static unsigned long long int number_of_graphs_generated_double_vertex[MAXN1 + 1] = {0};
static unsigned long long int number_of_graphs_generated_rem_vertex[MAXN1 + 1] = {0};
static unsigned long long int number_of_graphs_generated_no_rem_vertex[MAXN1 + 1] = {0};


/* Global variables for the 'ramsey' option: */

/* The testgraph */

GRAPH_STRUCT testgraph;
GRAPH_STRUCT testgraph_compl;

static int nv_testgraph = 0;

//The number of edges in the complement of the testgraph
static int ne_compl_testgraph = 0;

graph nautyg_testg[MAXN1*MAXM1];
graph nautyg_testg_compl[MAXN1*MAXM1];

/* Some heuristics for testing if a graph can be a subgraph of the complement of the testgraph */

/*
 * Remark: these heuristics are only useful if the testgraph doesn't have too much
 * edges (i.e. the complement isn't too sparse). But these cases can already be
 * solved pretty quickly, so that's why we're not using these heuristics
 * in make_induced_subgraphs().
 */

/* The maximum degree of the complement of the testgraph */
static int max_degree_compl_testgraph = 0;

/* The number of vertices with max degree in the complement of the testgraph */
static int num_max_degree_compl_testgraph = 0;

/* The number of vertices with degree > 0 in the complement of the testgraph */
static int num_nonzero_degree_compl_testgraph = 0;

/* The adjacency list of the complement of the testgraph in descending order */
//Important: The first element is at index 0
ADJACENCY ordered_adj_compl_testgraph;

/* 
 * The graph obtained by removing a vertex from testgraph_compl.
 * The vertex which was removed from testgraph_compl is removed_vertex.
 */

GRAPH_STRUCT testgraph_compl_v;
graph nautyg_testg_compl_v[MAXN1*MAXM1];

static unsigned char removed_vertex;
static unsigned char degree_removed_vertex;
static int nv_testgraph_v = 0;
static int ne_compl_testgraph_v = 0;
graph nautyg_testg_compl_v[MAXN1*MAXM1];

static int max_degree_compl_testgraph_v = 0;
static int num_max_degree_compl_testgraph_v = 0;
static int num_nonzero_degree_compl_testgraph_v = 0;
ADJACENCY ordered_adj_compl_testgraph_v;

/*
 * A greedy algorithm first tries to construct a set with greedy_set_size vertices
 * and then tries to complete the set using make_induced_subgraphs().
 * The hope is that this greedy set is often a subgraph.
 * nv_testgraph - 3 seems to be a good trade-off for this.
 * This variable is set in read_testgraph().
 */
static int greedy_set_size;

//Each time a set is an induced subgraph of the testgraph it is saved in a list
//Before calling make_induced_subgraphs() it is first tested if the sets
//from previous_time_subgraph_sets are no induced subgraphs
static setword previous_time_subgraph_sets[MAXN1+1][NUM_PREV_SETS] = {0};
static unsigned char previous_time_subgraph_actual_set[MAXN1+1][NUM_PREV_SETS][MAXN1] = {0};

//Sets with nv_testgraph_v vertices which MIGHT be an induced subgraph of testgraph_v
static setword good_sets_temp[MAX_NUM_GOOD_SETS_TEMP] = {0};
static unsigned char good_sets_temp_actual_set[MAX_NUM_GOOD_SETS_TEMP][MAXN1] = {0};
static int num_good_sets_temp = 0;

//Sets which are an induced subgraph of testgraph_v
static setword good_sets_v[MAXN1+1][MAX_NUM_GOOD_SETS_V] = {0};
static int num_good_sets_v[MAXN1+1] = {0};


/***************************Method declarations********************************/

static void printgraph();
static void printgraph_given(GRAPH_STRUCT *g, int num_vertices);
static void printgraph_nauty(graph *g, int current_number_of_vertices);
static void printgraph_nauty_big(graph *g, int current_number_of_vertices);
static void print_generators(permutation generators[][MAXN1], int num_of_generators);

static void expand_dominating_set_remove(unsigned char dominating_set[], int set_size,
        EDGE removed_edges[], int *num_removed_edges);
static void reduce_dominating_set_remove(unsigned char dominating_set[], int set_size,
        EDGE removed_edges[], int num_removed_edges);


#endif	/* TRIANGLERAMSEY_H */


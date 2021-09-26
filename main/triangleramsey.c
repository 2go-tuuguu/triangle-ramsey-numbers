#define VERSION "1.0 - June 28 2012"

/**
 * triangleramsey.c
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
 * -----------------------------------
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

/**
 * Compile with:
 *
 * cc -DMAXN=1024 -DWORDSIZE=32 -O4 nauty.c nautil.c naugraph.c triangleramsey.c -o triangleramsey
 *
 * Or:
 *
 * cc -DMAXN=1024 -DWORDSIZE=64 -O4 nauty.c nautil.c naugraph.c triangleramsey.c -o triangleramsey
 *
 * -DWORDSIZE=32 is slightly faster, but limits the order of the graphs to 32.
 *
 */

#include <limits.h>
#include <sys/times.h>
#include "nauty.h"
#include "triangleramsey.h"

/******************************************************************************/

/*
 * Converts g to a graph in the nauty format and stores the resulting graph
 * in nauty_graph.
 */
static void
copy_nauty_graph(GRAPH_STRUCT *g, int num_vertices, graph *nauty_graph) {
    int i, j;
    for(i = 0; i < num_vertices; i++) {
        nauty_graph[i] = 0;
        for(j = 0; j < g->adj[i]; j++)
            ADDELEMENT1(&nauty_graph[i], g->g[i][j]);
    }
}

/******************************************************************************/

/*
 * Tries to open a file. If the file didn't open correctly, it tries to
 * open it again a little later. It tries to open it at most NUMTRIES_FOPEN
 * times and then the file is returned. 
 * So it can happend that the returned file is NULL.
 */
static FILE *repeated_fopen(char *filename, char *mode) {
    int i = 0;
    FILE *fil = NULL;
    do {
        fil = fopen(filename, mode);
        i++;
        if(fil == NULL && i < NUMTRIES_FOPEN) {
            sleep(5);
        }
    } while(fil == NULL && i < NUMTRIES_FOPEN);

    return (fil); //Can happen that NULL is returned
}

/******************************************************************************/

/*
 * Adds an edge between v and w.
 */
static void 
add_edge(GRAPH_STRUCT *g, unsigned char v, unsigned char w) {
    g->g[v][g->adj[v]] = w;
    g->g[w][g->adj[w]] = v;
    g->adj[v]++;
    g->adj[w]++;
}

/******************************************************************************/

/**
 * Decodes the code (which is in multicode format) of a graph.
 */
static void 
decode_multicode(unsigned char code[], int codelength, GRAPH_STRUCT *g) {
    int i;
    for(i = 0; i < code[0]; i++) {
        g->adj[i] = 0;
    }

    int currentvertex = 1;
    for(i = 1; i < codelength; i++) {
        if(code[i] == 0) {
            currentvertex++;
        } else {
            add_edge(g, currentvertex - 1, code[i] - 1);
        }
    }

}

/******************************************************************************/

/**
 * Sorts a set in descending order using bubblesort.
 */
static void 
sort_set_descending(unsigned char set[], int length) {
    unsigned char i, temp;
    int modified = 1;
    while(length > 1 && modified) {
        modified = 0;
        for(i = 0; i < length - 1; i++) {
            if(set[i] < set[i + 1]) {
                temp = set[i];
                set[i] = set[i + 1];
                set[i + 1] = temp;
                modified = 1;
            }
        }
        length--;
    }
    
}

/******************************************************************************/

/**
 * Sorts a set in ascending order using bubblesort.
 */
static void 
sort_set_ascending(unsigned char set[], int length) {
/*
    int i, j, temp;
    for(j = 0; j < length; j++) {
        for(i = 1; i < length - j; i++) {
            if(set[i - 1] > set[i]) {
                temp = set[i];
                set[i] = set[i - 1];
                set[i - 1] = temp;
            }
        }
    }
*/

    unsigned char i, temp;
    int modified = 1;
    while(length > 1 && modified) {
        modified = 0;
        for(i = 0; i < length - 1; i++) {
            if(set[i] > set[i + 1]) {
                temp = set[i];
                set[i] = set[i + 1];
                set[i + 1] = temp;
                modified = 1;
            }
        }
        length--;
    }
    
}

/******************************************************************************/

/**
 * Makes the inverse of the testgraph.
 */
static void
make_inverse_graphs_testgraph() {
    int i, j;
    for(i = 0; i < nv_testgraph; i++) {
        nautyg_testg_compl[i] = 0;
        testgraph_compl.adj[i] = 0;
    }
    
    ne_compl_testgraph = 0;
    for(i = 0; i < nv_testgraph; i++)
        for(j = i + 1; j < nv_testgraph; j++)
            if(!ISELEMENT1(&nautyg_testg[i], j)) {
                ADDELEMENT1(&nautyg_testg_compl[i], j);
                ADDELEMENT1(&nautyg_testg_compl[j], i);
                add_edge(&testgraph_compl, i, j);
                ne_compl_testgraph++;
            }
    
    memcpy(ordered_adj_compl_testgraph, testgraph_compl.adj, sizeof(ADJACENCY));
    sort_set_descending(ordered_adj_compl_testgraph, nv_testgraph);
/*
    for(i = 0; i < nv_testgraph; i++)
        fprintf(stderr, "%d ", ordered_adj_compl_testgraph[i]);
    fprintf(stderr, "\n");
*/

    max_degree_compl_testgraph = ordered_adj_compl_testgraph[0];
    num_max_degree_compl_testgraph = 0;
    num_nonzero_degree_compl_testgraph = 0;
    for(i = 0; i < nv_testgraph; i++) {
        if(ordered_adj_compl_testgraph[i] == max_degree_compl_testgraph)
            num_max_degree_compl_testgraph++;
        if(ordered_adj_compl_testgraph[i] > 0)
            num_nonzero_degree_compl_testgraph++;
    }
    //fprintf(stderr, "Properties of complement of testgraph: max deg: %d, num max deg: %d, num nonzero: %d\n", 
    //        max_degree_compl_testgraph, num_max_degree_compl_testgraph, num_nonzero_degree_compl_testgraph);
    
    //Now remove a vertex from testgraph_compl_v
    //For now we remove the vertex with smallest degree
    for(removed_vertex = 0; removed_vertex < nv_testgraph; removed_vertex++)
        if(testgraph_compl.adj[removed_vertex] == ordered_adj_compl_testgraph[nv_testgraph-1])
        //Now remove vertex with max degree! No, removing min_degree turned out to be more useful!
        //if(testgraph_compl.adj[removed_vertex] == ordered_adj_compl_testgraph[0])
            break;
    degree_removed_vertex = testgraph_compl.adj[removed_vertex];
    
    ne_compl_testgraph_v = ne_compl_testgraph - degree_removed_vertex;
    nv_testgraph_v = nv_testgraph - 1;
    
    for(i = 0; i < nv_testgraph_v; i++) {
        nautyg_testg_compl_v[i] = 0;
        testgraph_compl_v.adj[i] = 0;
    }
    
    unsigned char v1, v2;
    for(i = 0; i < nv_testgraph; i++)
        if(i != removed_vertex) {
            if(i > removed_vertex)
                v1 = i - 1;
            else
                v1 = i;
            for(j = 0; j < testgraph_compl.adj[i]; j++) {
                v2 = testgraph_compl.g[i][j];
                if(v2 > i && v2 != removed_vertex) {
                    if(v2 > removed_vertex)
                        v2--;
                    ADDELEMENT1(&nautyg_testg_compl_v[v1], v2);
                    ADDELEMENT1(&nautyg_testg_compl_v[v2], v1);
                    add_edge(&testgraph_compl_v, v1, v2);
                }
            }
        }
    
    memcpy(ordered_adj_compl_testgraph_v, testgraph_compl_v.adj, sizeof(ADJACENCY));
    sort_set_descending(ordered_adj_compl_testgraph_v, nv_testgraph_v);

    max_degree_compl_testgraph_v = ordered_adj_compl_testgraph_v[0];
    num_max_degree_compl_testgraph_v = 0;
    num_nonzero_degree_compl_testgraph_v = 0;
    for(i = 0; i < nv_testgraph_v; i++) {
        if(ordered_adj_compl_testgraph_v[i] == max_degree_compl_testgraph_v)
            num_max_degree_compl_testgraph_v++;
        if(ordered_adj_compl_testgraph_v[i] > 0)
            num_nonzero_degree_compl_testgraph_v++;
    }    
    
}

/******************************************************************************/

/**
 * Code the graph using the multicode format and returns the length of the code.
 */
int code_multicode(unsigned char code[], GRAPH_STRUCT *g, int num_of_vertices) {
    int i, j;
    int codelength = 0;
    code[codelength++] = num_of_vertices;
    for(i = 0; i < num_of_vertices - 1; i++) {
        for(j = 0; j < g->adj[i]; j++)
            if(g->g[i][j] > i)
                code[codelength++] = g->g[i][j] + 1;
        code[codelength++] = 0;
    }

    return codelength;
}

/******************************************************************************/

static void
output_graph(GRAPH_STRUCT *g, int num_vertices) {

    if(!noout) {
        unsigned char codebuffer[MAX_MUTLICODE_SIZE(num_vertices)];
        int codelength = code_multicode(codebuffer, g, num_vertices);

/*
        if(codelength > MAX_MUTLICODE_SIZE(num_vertices)) {
            fprintf(stderr, "Error: codelength too big\n");
            exit(1);
        }
        
        int num_edges = 0;
        int i;
        for(i = 0; i < num_vertices; i++)
            num_edges += g->adj[i];
        int expected_codelength = num_vertices + num_edges / 2;
        if(codelength != expected_codelength) {
            fprintf(stderr, "Error: expected codelength %d but code has length: %d\n", expected_codelength, codelength);
            exit(1);
        }
*/

        if(!write_all_ramseygraphs) {
            if(fwrite(codebuffer, sizeof(unsigned char), codelength, stdout)
                    < sizeof(unsigned char) * codelength) {
                fprintf(stderr, "Error: couldn't write graph! \n\n");
                exit(1);
            }
        } else {
            //Writing ramseygraphs to a file instead of to stdout and fclosing it 
            //immediately to make sure all ramseygraphs are actually written
            FILE *fil = repeated_fopen(ramseygraphs_filename, "ab");
            if(fil == NULL) {
                fprintf(stderr, "Error: can't open %s\n", ramseygraphs_filename);
                exit(1);
            }
            if(fwrite(codebuffer, sizeof(unsigned char), codelength, fil)
                    < sizeof(unsigned char) * codelength) {
                fprintf(stderr, "Error: couldn't write graph! \n\n");
                exit(1);
            }
            fclose(fil);
        }
        
        number_of_graphs_output++;
    }
}

/*****************************************************************************
 *                                                                            *
 *  nextelement(set1,m,pos) = the position of the first element in set set1   *
 *  which occupies a position greater than pos.  If no such element exists,   *
 *  the value is -1.  pos can have any value less than n, including negative  *
 *  values.                                                                   *
 *                                                                            *
 *  GLOBALS ACCESSED: none                                                    *
 *                                                                            *
 *****************************************************************************/

/* Assumes that MAXM is 1 (i.e. one_word_sets) */
int
nextelement1(set *set1, int m, int pos) {
    setword setwd;

    if(pos < 0) setwd = set1[0];
    else setwd = set1[0] & BITMASK(pos);

    if(setwd == 0) return -1;
    else return FIRSTBIT(setwd);
}

/******************************************************************************/

/**
 * Method which is called each time nauty finds a generator.
 */
static void
save_generators(int count, permutation perm[], nvector orbits[],
        int numorbits, int stabvertex, int n) {
    memcpy(generators + number_of_generators, perm, sizeof(permutation) * n);

    number_of_generators++;
}

/******************************************************************************/

/**
 * Returns 1 if the last double vertex is canonical, else returns 0.
 * Important: good_vertices[] must contain the last vertex.
 * 
 * Nauty is called to check the canonicity.
 * The last double vertex is canonical iff it is in the same orbit as the good
 * vertex with the largest label in the canonically labelled graph.
 * 
 * good_vertices[] contains the double vertices which fullfill the cheap criteria.
 * orbits[] contains the orbits of the vertices of the graph.
 * 
 * This method is also used to determine the canonical removable vertex.
 * 
 * Remark: lab[x] = y means that y has label x in the canonically labelled graph.
 */
static int
is_canonical_vertex(unsigned char good_vertices[], int num_good_vertices,
        int lab[], int orbits[]) {
    //Remark: lab_rev would be useful
    //Could also mark on beforehand, but this is not a bottleneck!
    RESETMARKS;
    int i;
    for(i = 0; i < num_good_vertices; i++)
        MARK(good_vertices[i]);

    for(i = 0; i < nv; i++)
        if(ISMARKED(lab[i]))
            break;

    if(i < nv) {
        return orbits[lab[i]] == orbits[nv - 1];
    } else {
        fprintf(stderr, "Error: no good_vertices found\n");
        exit(1);
    }
}

/******************************************************************************/
/**
 * Uses some cheap heuristics to determine the canonical double vertex reduction.
 * Returns 1 if the last vertex is among the best double vertices, else returns 0.
 * 
 * The canonical double vertex must have max degree. If there are multiple double
 * vertices (from different neighbourhoods) with the same max degree, the canonical
 * one must have the largest (descendinly) ordered valency vector.
 * 
 * If 1 is returned and has_to_call_nauty == 0, this means that the last vertex was
 * the only good_vertex. 
 * If there are multiple good vertices (which have a different neighbourhood 
 * than the last vertex), all good vertices are stored in good_vertices and 
 * has_to_call_nauty is set to 1.
 */
static int
is_best_double_vertex_reduction(unsigned char good_vertices[], int *num_good_vertices,
        int *has_to_call_nauty) {

    if(num_dv_neighbourhoods == 1) {
        //Remark: necessary for nauty partitions
        *num_good_vertices = 0;
        int j;
        for(j = 0; j < num_double_vertices[0]; j++)
            good_vertices[(*num_good_vertices)++] = double_vertices[0][j];
        
        *has_to_call_nauty = 0;
        return 1;
    }

    //Remark: maybe it's better for "ramsey" to use the smallest valency vector?
    //Or sorted ascendingly?
    //For option "mtf" it doesn't really matter that much
    
    //Remark: also use other cheap heuristics, such as eg in most number of 4-cycles?
    //No, this is not a bottleneck!

    //Could postpone this till we're sure there is more than one good neighbourhood
    //But this is probably not a bottleneck
    unsigned char valencies_last[current_graph.adj[nv - 1]];
    int sum_last = 0;
    int already_sorted = 0;
    int i;
    for(i = 0; i < current_graph.adj[nv - 1]; i++)
        sum_last += (valencies_last[i] = current_graph.adj[current_graph.g[nv - 1][i]]);
    //sort_set_ascending(valencies_last, current_graph.adj[nv - 1]);
    
    //Wait with this:
    //sort_set_descending(valencies_last, current_graph.adj[nv - 1]);

    unsigned char valencies_other[current_graph.adj[nv - 1]];
    int sum_other;    
    int last_vertex_neighbourhood_index;

    *num_good_vertices = 0;
    int j;
    for(i = 0; i < num_dv_neighbourhoods; i++)
        if(current_graph.adj[double_vertices[i][0]] == current_graph.adj[nv - 1]) {
            //Avoid doing this if is same neighbourhood as last vertex...
            if(nautyg[double_vertices[i][0]] != nautyg[nv - 1]) {
                sum_other = 0;
                for(j = 0; j < current_graph.adj[nv - 1]; j++)
                    sum_other += (valencies_other[j] = current_graph.adj[current_graph.g[double_vertices[i][0]][j]]);
                
                if(sum_other > sum_last)
                    return 0;
                else if(sum_other == sum_last) {
                    if(!already_sorted) {
                        sort_set_descending(valencies_last, current_graph.adj[nv - 1]);
                        already_sorted = 1;
                    }                    

                    //sort_set_ascending(valencies_other, current_graph.adj[nv - 1]);
                    sort_set_descending(valencies_other, current_graph.adj[nv - 1]);

                    /*
                     * memcmp(a,b)
                     * if(a > b)
                     *   return > 0
                     * else if(a < b)
                     *   return < 0
                     * else 
                     *   return 0
                     */
                    //Last vertex must have the largest valency vector
                    int res = memcmp(valencies_last, valencies_other, current_graph.adj[nv - 1]);
                    if(res < 0)
                        return 0;
                    else if(res == 0) {
                        for(j = 0; j < num_double_vertices[i]; j++)
                            good_vertices[(*num_good_vertices)++] = double_vertices[i][j];
                    }
                }
            } else {
                last_vertex_neighbourhood_index = i;
            }
        } else if(current_graph.adj[double_vertices[i][0]] > current_graph.adj[nv - 1])
            return 0; //Can sometimes happen that this operation lets the old max degree increase

    if(*num_good_vertices == 0) {
        *has_to_call_nauty = 0;
    } else {
        *has_to_call_nauty = 1;
        //for(j = 0; j < num_double_vertices[last_vertex_neighbourhood_index]; j++)
        //    good_vertices[(*num_good_vertices)++] = double_vertices[last_vertex_neighbourhood_index][j];
    }
    
    //For nauty partitions
    for(j = 0; j < num_double_vertices[last_vertex_neighbourhood_index]; j++)
        good_vertices[(*num_good_vertices)++] = double_vertices[last_vertex_neighbourhood_index][j];    

    return 1;
}

/******************************************************************************/

/**
 * Updates the list of double vertices after a double vertex operation.
 */
static void
update_double_vertices(set neighbourhood, unsigned char vertex) {
    //Must add the new vertex to other dv_neighbourhoods if they contained vertex
    int i;
    for(i = 0; i < num_dv_neighbourhoods; i++)
        if(ISELEMENT1(&dv_neighbourhoods[i], vertex))
            ADDELEMENT1(&dv_neighbourhoods[i], nv);    
    if(num_dv_neighbourhoods > 0) {
        //First check if neighbourhood didn't already exist
        //Remark: could use marks for this, but this is probably not a bottleneck
        for(i = 0; i < num_dv_neighbourhoods; i++)
            if(neighbourhood == dv_neighbourhoods[i]) {
                //Vertex was already a double vertex before
                double_vertices[i][num_double_vertices[i]++] = nv;
                total_num_double_vertices++;

                return;
            }
    }

    //In this case the neighbourhood didn't exist yet
    dv_neighbourhoods[num_dv_neighbourhoods] = neighbourhood;
    double_vertices[num_dv_neighbourhoods][0] = vertex;
    double_vertices[num_dv_neighbourhoods][1] = nv;
    num_double_vertices[num_dv_neighbourhoods] = 2;
    num_dv_neighbourhoods++;
    total_num_double_vertices += 2;
}

/******************************************************************************/

/**
 * Updates the list of double vertices after a double vertex reduction.
 */
static void
restore_double_vertices(set neighbourhood) {
    int i;
    if(neighbourhood == dv_neighbourhoods[num_dv_neighbourhoods - 1]) {
        if(num_double_vertices[num_dv_neighbourhoods - 1] == 2) {
            num_dv_neighbourhoods--;
            total_num_double_vertices -= 2;
        } else {
            num_double_vertices[num_dv_neighbourhoods - 1]--;
            total_num_double_vertices--;
        }
    } else {
        for(i = 0; i < num_dv_neighbourhoods - 1; i++)
            if(neighbourhood == dv_neighbourhoods[i]) {
                num_double_vertices[i]--;
                total_num_double_vertices--;

                //return;
                break;
            }
    }
    //Remove nv from other dv_neighbourhoods
    for(i = 0; i < num_dv_neighbourhoods; i++)
        DELELEMENT1(&dv_neighbourhoods[i], nv);

}

/******************************************************************************/

/**
 * "Doubles" a vertex. This is create a new vertex with the same neighbourhood
 * as the given vertex.
 */
static void
expand_double_vertex(unsigned char vertex) {
    int i;
    unsigned char neighbour;
    current_graph.adj[nv] = current_graph.adj[vertex];

    //Remark: or wait to update nautyg till we're certain that the graph is canonical?
    //But this is not a bottleneck...
    nautyg[nv] = nautyg[vertex];
    for(i = 0; i < current_graph.adj[vertex]; i++) {
        neighbour = current_graph.g[vertex][i];
        current_graph.g[nv][i] = neighbour;
        current_graph.g[neighbour][current_graph.adj[neighbour]++] = nv;

        ADDELEMENT1(&nautyg[neighbour], nv);
        //ADDELEMENT1(&nautyg[nv], neighbour);
    }

    //Remark: can probably postpone this, but it's not a bottleneck.
    update_double_vertices(nautyg[vertex], vertex);

    nv++;
}

/******************************************************************************/

/**
 * Reduction for the "double vertex" operation.
 */
static void
reduce_double_vertex(unsigned char vertex) {
    nv--;

    restore_double_vertices(nautyg[vertex]);
    //nautyg[nv] = 0; //Is not necessary!
    int i;
    unsigned char neighbour;
    for(i = 0; i < current_graph.adj[vertex]; i++) {
        neighbour = current_graph.g[vertex][i];
        DELELEMENT1(&nautyg[neighbour], nv);
        current_graph.adj[neighbour]--;
    }
}

/******************************************************************************/

/**
 * Constructs the sets for operation of type 2: the vertices of the sets should
 * not have neighbours which are also in the set (i.e. no vertices may be removed
 * by the construction).
 */
static void
construct_dominating_sets_no_remove(unsigned char current_set[], int current_set_size, setword current_set_bitvector,
        setword covered_vertices, setword forbidden_vertices, int min_set_size, int max_set_size) {

    //Remark: min and max set size are not used at the moment...
    
    if(covered_vertices == ALLMASK(nv)) {
        /*
         * Remark: all dominating sets are good: since the set is dominating
         * all vertices are on distance at most 1 from a vertex from the set.
         * And since no edges are removed, the new vertex is on distance at most 2
         * from each other vertex of the graph.
         */
        
        /*
         * Don't add set if all vertices of the set would have the same common
         * neighbour and the neighbourhood of that common neighbour is exactly
         * the dominating set. Since that would yield a double vertex.
         * 
         * Note: the neighbourhood of the common vertices will always be exactly
         * the dominating set, since if the common vertex would contain another
         * neighbour, it has to be on distance 1 from the dominating set.
         * But in that case the graph would contain a triangle, so this is not
         * possible!
         * So if the set contains common neighbours, we know we can't expand
         * this set, otherwise we'll have a double vertex.
         */
        int i;
        setword common_neighbours = nautyg[current_set[0]];
        for(i = 1; i < current_set_size; i++) {
            common_neighbours &= nautyg[current_set[i]];
            if(common_neighbours == 0)
                break;
        }
        if(common_neighbours == 0) {
            //Array is full, so expand it!
            if(number_of_dominating_sets_global == max_dominating_sets_global) {
                ADJACENCY dominating_sets_temp[number_of_dominating_sets_global];
                memcpy(dominating_sets_temp, dominating_sets_global, sizeof(ADJACENCY) * number_of_dominating_sets_global);
                free(dominating_sets_global);
                
                int dominating_sets_size_temp[number_of_dominating_sets_global];
                memcpy(dominating_sets_size_temp, dominating_sets_size_global, sizeof(int) * number_of_dominating_sets_global);
                free(dominating_sets_size_global);
                
                //Remark: could also do x2, but this doesn't matter much.
                max_dominating_sets_global += DEFAULT_MAX_DOM_SETS_SIZE;

                dominating_sets_global = (ADJACENCY *) malloc(sizeof(ADJACENCY) * max_dominating_sets_global);
                if(dominating_sets_global == NULL) {
                    fprintf(stderr, "Error: out of memory while expanding dominating_sets_global\n");
                    exit(1);
                }

                dominating_sets_size_global = (int *) malloc(sizeof(int) * max_dominating_sets_global);
                if(dominating_sets_size_global == NULL) {
                    fprintf(stderr, "Error: out of memory while expanding dominating_sets_size_global\n");
                    exit(1);
                }
                
                //Now fill arrays again
                memcpy(dominating_sets_global, dominating_sets_temp, sizeof(ADJACENCY) * number_of_dominating_sets_global);
                memcpy(dominating_sets_size_global, dominating_sets_size_temp, sizeof(int) * number_of_dominating_sets_global);
            }
            
            memcpy(dominating_sets_global[number_of_dominating_sets_global], current_set, sizeof(unsigned char) * current_set_size);
            dominating_sets_size_global[number_of_dominating_sets_global] = current_set_size;

            //Important: has to be sorted for determine_vertexset_orbits!
            sort_set_ascending(dominating_sets_global[number_of_dominating_sets_global], current_set_size);
            number_of_dominating_sets_global++;            
        }
    } else {

        /*
            int next_vertex = 0;
            //Eventueel xor met alle bits of een not en dan firstbit?
            while(ISELEMENT1(&covered_vertices, next_vertex))
                next_vertex++;
         */
        //Important: it is assumed that not all vertices are covered!
        setword not_covered_vertices = ~covered_vertices;
        int next_vertex = FIRSTBIT(not_covered_vertices);

        setword current_set_bitvector_new;
        
        setword covered_vertices_new;

        //Step 1: include next_vertex in the dominating set
        if(!ISELEMENT1(&forbidden_vertices, next_vertex)) {
            ADDELEMENT1(&forbidden_vertices, next_vertex);

            covered_vertices_new = covered_vertices | nautyg_loops[next_vertex];

            current_set_bitvector_new = current_set_bitvector;
            ADDELEMENT1(&current_set_bitvector_new, next_vertex);

            current_set[current_set_size] = next_vertex;

            construct_dominating_sets_no_remove(current_set, current_set_size + 1, current_set_bitvector_new,
                    covered_vertices_new, forbidden_vertices | nautyg_loops[next_vertex], min_set_size, max_set_size);
            //current_set_size--;
        }

        //Step 2: next_vertex is forbidden, so at least one of the neighbours
        //of next_vertex must be included in the dominating set, otherwise it cannot
        //become a dominating set
        if((nautyg[next_vertex] & (~forbidden_vertices)) > 0) {
            //next_vertex has neighbours which are not forbidden
            int i;
            unsigned char neighbour;
            for(i = 0; i < current_graph.adj[next_vertex]; i++) {
                neighbour = current_graph.g[next_vertex][i];
                //Remark: could use marks instead, but this won't make much difference
                if(!ISELEMENT1(&forbidden_vertices, neighbour)) {
                    ADDELEMENT1(&forbidden_vertices, neighbour);

                    covered_vertices_new = covered_vertices | nautyg_loops[neighbour];

                    current_set_bitvector_new = current_set_bitvector;
                    ADDELEMENT1(&current_set_bitvector_new, neighbour);

                    current_set[current_set_size] = neighbour;

                    construct_dominating_sets_no_remove(current_set, current_set_size + 1, current_set_bitvector_new,
                            covered_vertices_new, forbidden_vertices | nautyg_loops[neighbour], min_set_size, max_set_size);
                }
            }
        }

    }
    
}

/******************************************************************************/

/**
 * Returns 1 if the graph obtained after removing vertex and all its incident
 * edges is still mtf, else returns 0.
 * 
 * Important: it is assumed that graph before removing vertex is MTF.
 * So one only has to check the neighbours of vertex, otherwise the original
 * graph was not MTF.
 */
static int 
graph_is_mtf_after_removing_vertex(unsigned char vertex) {
    /*
     * Check if the graph is maximal, i.e. check if the diameter is 2.
     * So for every vertex of the graph the union of the neighbourhoods of its
     * neighbours should be the entire vertexset of the graph.
     * But since we assume the graph is mtf, we only have to check if each
     * neighbour of vertex has a common neighbour with each other neighbour
     * of vertex (other than vertex).
     */
/*
    int i, j;
    setword covered_vertices;
    unsigned char neighbour;
    for(i = 0; i < current_graph.adj[vertex]; i++) {
        neighbour = current_graph.g[vertex][i];
        covered_vertices = nautyg[neighbour];
        for(j = 0; j < current_graph.adj[neighbour]; j++)
            if(current_graph.g[neighbour][j] != vertex)
                covered_vertices |= nautyg[current_graph.g[neighbour][j]];
        if(covered_vertices != ALLMASK(nv))
            return 0;
    }
*/
    int i, j;
    setword neighbourhood1;
    for(i = 0; i < current_graph.adj[vertex] - 1; i++) {
        neighbourhood1 = nautyg[current_graph.g[vertex][i]];
        DELELEMENT1(&neighbourhood1, vertex);
        for(j = i + 1; j < current_graph.adj[vertex]; j++)
            //If the 2 vertices of the set only have vertex as common neighbour, 
            //the resulting graph is not mtf
            if((neighbourhood1 & nautyg[current_graph.g[vertex][j]]) == 0)
                return 0;
    }
    
    return 1;
}

/******************************************************************************/

/**
 * Returns 1 if the graph obtained after removing vertex and all its incident
 * edges is still mtf, else returns 0.
 * 
 * Important: it is assumed that the graph before removing vertex is MTF.
 * So one only has to check the neighbours of vertex, otherwise the original
 * graph was not MTF.
 * 
 * This special version is used by determine_possible_sets_of_edges_to_insert(),
 * since in that case edges are already inserted, so there may already be an edge 
 * between neighbours of vertex.
 * This method is slightly slower than graph_is_mtf_after_removing_vertex(),
 * but this is not a problem since it is only very rarely called.
 * 
 */
static int 
graph_is_mtf_after_removing_vertex_inserted_edges(unsigned char vertex) {
    /*
     * Check if the graph is maximal, i.e. check if the diameter is 2.
     * So for every vertex of the graph the union of the neighbourhoods of its
     * neighbours should be the entire vertexset of the graph.
     * Since the graph is mtf, we only have to investigate the neighbours
     * of vertex.
     */
    int i, j;
    setword covered_vertices;
    unsigned char neighbour;
    for(i = 0; i < current_graph.adj[vertex]; i++) {
        neighbour = current_graph.g[vertex][i];
        covered_vertices = nautyg[neighbour];
        for(j = 0; j < current_graph.adj[neighbour]; j++)
            if(current_graph.g[neighbour][j] != vertex)
                covered_vertices |= nautyg[current_graph.g[neighbour][j]];
        if(covered_vertices != ALLMASK(nv))
            return 0;
    }
    
    return 1;
}

/******************************************************************************/

/**
 * Returns 1 if the expanded graph can be a ramseygraph, else returns 0.
 * 
 * The graph can only be a ramseygraph if vertex nv - 1 has at least one neighbour
 * in every good_set_v.
 */
static int
can_be_ramseygraph(setword neighbourhood, int max_nv) {
    int i, j;
    //for(j = nv_testgraph; j <= max_nv; j++)
    for(j = max_nv; j >= nv_testgraph; j--)
        for(i = 0; i < num_good_sets_v[j]; i++)
            if((neighbourhood & good_sets_v[j][i]) == 0) {
                return 0;
            }

    return 1;
}

/******************************************************************************/

/*
 * Returns 1 if the dominating set is also good, else returns 0.
 * A set is good if after the deletion of all edges joining two vertices of S,
 * the distance between any two vertices which are not in S
 * is at most 2. (The distance between the new vertex and any other vertex, is 
 * always at most 2). Or alternatively: if no edges can be added. 
 * Only possibility: an edge between a vertex from the set and a vertex which is
 * not in the set.
 * 
 * Important: it is assumed that at least one edge is removed between the edges.
 */
static int
is_good_dominating_set_remove(unsigned char current_set[], int current_set_size, setword current_set_bitvector) {
    //Remark: don't do this yet, since the order in which the graphs are generated will be different
    //Also problem with determine orbits? Only do this if no dv's
    //And then if vertexset is mapped to a nonexistent vertexset, then this vertexset
    //also cannot lead to a ramseygraph!
/*
    if(ramsey && !can_be_ramseygraph(current_set_bitvector, nv)) {
        return 0;
    }
*/
    
/*
    int i, j;
    for(i = 0; i < nv; i++) {
        //Remark: could use nextelement1 instead, but this is not a bottleneck
        if(!ISELEMENT1(&current_set_bitvector, i)) {
            setword neighbourhood = nautyg_loops[i] & (~current_set_bitvector);
            for(j = 0; j < current_set_size; j++) {
                //This is the most expensive line
                if((neighbourhood & nautyg[current_set[j]]) == 0) {
                    times_la_good_set_rejected_full_test++;
                    return 0;
                }
            }
        }
    }
    times_la_good_set_not_rejected_full_test++;
*/
    //This is a tiny bit faster (but it is only a very little faster since this not really a bottleneck)
    
    /*
     * Note: actually we only have to investigate the vertices of the set which
     * have a removed edge. But computing these vertices is slower than just investigating
     * all of them.
     */
    int i, j;
    i = -1;
    setword current_set_bitvector_compl = (~current_set_bitvector) & ALLMASK(nv);
    while((i = nextelement1(&current_set_bitvector_compl, 1, i)) >= 0) {
        setword neighbourhood = nautyg_loops[i] & current_set_bitvector_compl;
        for(j = 0; j < current_set_size; j++) {
            //This is the most expensive line
            if((neighbourhood & nautyg[current_set[j]]) == 0) {
                return 0;
            }
        }
    }
    
    //Misschien goedkoper om eerst goedkope heuristieken canoniciteit uit te proberen?
    EDGE removed_edges[MAXN1 * (MAXN1 - 1) / 2];
    int num_removed_edges = 0;
    
    //Remove edges between elements of dominating set
    ADJACENCY adj_expanded;
    memcpy(adj_expanded, current_graph.adj, sizeof(ADJACENCY));
    int is_best = 1;
    for(i = 0; i < current_set_size - 1; i++)
        for(j = i + 1; j < current_set_size; j++)
            if(ISELEMENT1(&nautyg[current_set[i]], current_set[j])) {
                DELELEMENT1(&nautyg[current_set[i]], current_set[j]);
                DELELEMENT1(&nautyg[current_set[j]], current_set[i]);
                
                removed_edges[num_removed_edges][0] = current_set[i];
                removed_edges[num_removed_edges][1] = current_set[j];
                num_removed_edges++;
                
                adj_expanded[current_set[i]]--;
                adj_expanded[current_set[j]]--;
                if(adj_expanded[current_set[i]] + 1 < current_set_size
                        || adj_expanded[current_set[j]] + 1 < current_set_size) {
                    //return 0; //Last vertex does not have min degree
                    is_best = 0;
                    i = j = current_set_size;
                }
            }
    
    //Don't check if the graph contains double vertices, since these are almost
    //always already destroyed!
    //Of enkel de oude double vertices onderzoeken (want zal meestal geen nieuwe double vertices genereren?)
    //Maar de oude dv's zullen meestal al kapotgemaakt zijn en die constructie is ook al vrij restrictief??
    /*
     * It is fastest only to check if there are removable vertices after the
     * "is good set" test. It is also fastest only to investigate the old removable
     * vertices. Testing all vertices is a lot slower!
     */
    if(num_removable_vertices > 0 && is_best) {
        for(i = 0; i < num_removable_vertices; i++)
            if(!ISELEMENT(&current_set_bitvector, removable_vertices[i]))
                if(graph_is_mtf_after_removing_vertex(removable_vertices[i])) {
                    is_best = 0;
                    break;
                }
    }
    
    if(is_best) {
        for(i = 0; i < current_set_size; i++)
            adj_expanded[current_set[i]]++;

        unsigned char valencies_last[current_set_size];
        int sum_last = 0;
        int already_sorted = 0;
        for(i = 0; i < current_set_size; i++)
            sum_last += (valencies_last[i] = adj_expanded[current_set[i]]);
        
        //Postpone sorting
        //sort_set_descending(valencies_last, current_set_size);

        unsigned char valencies_other[current_set_size];
        int sum_other;

        //Important: it is assumed that last vertex has min degree 
        //(was enforced by lookahead during construction)

        //Remark: all vertices are valid reductions!
        for(i = 0; i < nv; i++)
            if(adj_expanded[i] == current_set_size) {
                int other_size = 0;
                sum_other = 0;
                for(j = 0; j < nv; j++)
                    if(j != i && ISELEMENT1(&nautyg[i], j)) {
                        sum_other += (valencies_other[other_size++] = adj_expanded[j]);
                        if(other_size == current_set_size)
                            break;
                    }
                
                //It still has to be connected to the new vertex
                if(other_size < current_set_size) {
                    sum_other += (valencies_other[other_size++] = current_set_size);
                }
                
                if(sum_other > sum_last) {
                    is_best = 0;
                    break;
                } else if(sum_other == sum_last) {
                    if(!already_sorted) {
                        sort_set_descending(valencies_last, current_set_size);
                        already_sorted = 1;
                    }
                    
                    //sort_set_ascending(valencies_other, current_graph.adj[nv - 1]);
                    sort_set_descending(valencies_other, current_set_size);

                    //Last vertex must have the largest valency vector
                    int res = memcmp(valencies_last, valencies_other, current_set_size);
                    if(res < 0) {
                        //return 0;
                        is_best = 0;
                        break;
                    }
                }                
            }
    }
    
    //Restore edges
    for(i = 0; i < num_removed_edges; i++) {
        ADDELEMENT1(&nautyg[removed_edges[i][0]], removed_edges[i][1]);
        ADDELEMENT1(&nautyg[removed_edges[i][1]], removed_edges[i][0]);
    }

    return is_best;
    
}

/******************************************************************************/

/**
 * Returns 1 if the current graph contains a removable vertex, else returns 0.
 * 
 * Important: it is assumed that the vertex with label nv - 1 is NOT removable.
 */
static int
contains_removable_vertex() {
    int i;
    for(i = 0; i < nv - 1; i++) {
        if(graph_is_mtf_after_removing_vertex(i))
            return 1;
    }
    return 0;
}

/******************************************************************************/

static void construct_dominating_sets_all_covered(unsigned char current_set[], int current_set_size, setword current_set_bitvector,
        setword forbidden_vertices, int min_set_size, int max_set_size, int has_neighours_in_set) {
    
    if(current_set_size >= min_set_size && has_neighours_in_set) {
        //Investigating forbidden_vertices^current_set_bitvector here doesn't help much...
        //But still helps a little so...
        
        //Test of good set is, als ja: die opslaan
        
        if(is_good_dominating_set_remove(current_set, current_set_size, current_set_bitvector)) {

            //Remark: better use a look-ahead instead of actually constructing and reducing??
            //But this is not a bottleneck...
            //Also no need to perform this test anymore when actually expanding!
            //Also avoid code duplication with construct_dominating_sets()
            EDGE removed_edges[MAXN1 * (MAXN1 - 1) / 2];
            int num_removed_edges;
            expand_dominating_set_remove(current_set, current_set_size,
                    removed_edges, &num_removed_edges);

            //Don't check if the graph contains double vertices, since these are almost
            //always already destroyed!            
            //int can_be_canonical = !contains_double_vertices_debug() && !contains_removable_vertex();
            int can_be_canonical = !contains_removable_vertex();
            
            reduce_dominating_set_remove(current_set, current_set_size,
                    removed_edges, num_removed_edges);

            if(can_be_canonical) {
                //Array is full, so expand it!
                if(number_of_dominating_sets_global == max_dominating_sets_global) {
                    ADJACENCY dominating_sets_temp[number_of_dominating_sets_global];
                    memcpy(dominating_sets_temp, dominating_sets_global, sizeof(ADJACENCY) * number_of_dominating_sets_global);
                    free(dominating_sets_global);

                    int dominating_sets_size_temp[number_of_dominating_sets_global];
                    memcpy(dominating_sets_size_temp, dominating_sets_size_global, sizeof(int) * number_of_dominating_sets_global);
                    free(dominating_sets_size_global);

                    max_dominating_sets_global += DEFAULT_MAX_DOM_SETS_SIZE;

                    dominating_sets_global = (ADJACENCY *) malloc(sizeof(ADJACENCY) * max_dominating_sets_global);
                    if(dominating_sets_global == NULL) {
                        fprintf(stderr, "Error: out of memory while expanding dominating_sets_global\n");
                        exit(1);
                    }

                    dominating_sets_size_global = (int *) malloc(sizeof(int) * max_dominating_sets_global);
                    if(dominating_sets_size_global == NULL) {
                        fprintf(stderr, "Error: out of memory while expanding dominating_sets_size_global\n");
                        exit(1);
                    }

                    //Now fill arrays again
                    memcpy(dominating_sets_global, dominating_sets_temp, sizeof(ADJACENCY) * number_of_dominating_sets_global);
                    memcpy(dominating_sets_size_global, dominating_sets_size_temp, sizeof(int) * number_of_dominating_sets_global);
                }

                memcpy(dominating_sets_global[number_of_dominating_sets_global], current_set, sizeof(unsigned char) * current_set_size);
                dominating_sets_size_global[number_of_dominating_sets_global] = current_set_size;

                //Important: has to be sorted for determine_vertexset_orbits!
                sort_set_ascending(dominating_sets_global[number_of_dominating_sets_global], current_set_size);
                number_of_dominating_sets_global++;
            }
        }        
        
    }

    if(current_set_size < max_set_size) {
        setword forbidden_set_compl;
        if(has_neighours_in_set && (forbidden_set_compl = forbidden_vertices^current_set_bitvector)) {
            int i, j;
            for(i = 0; i < nv; i++) {
                //Remark: could use nextelement1 instead, but this is not a bottleneck
                if(ISELEMENT1(&forbidden_set_compl, i)) { //i ist verboten und in S^c
                    setword neighbourhood = nautyg_loops[i] & (~current_set_bitvector);
                    for(j = 0; j < current_set_size; j++) {
                        //This line is now the most expensive...
                        if((neighbourhood & nautyg[current_set[j]]) == 0)
                            return;
                    }
                }
            }
        }
        
        //Build supersets
        int i;
        int has_neighours_in_set_new;
        setword current_set_bitvector_new;
        for(i = 0; i < nv; i++) {
            if(!ISELEMENT1(&forbidden_vertices, i)) {
                ADDELEMENT1(&forbidden_vertices, i);

                if(has_neighours_in_set)
                    has_neighours_in_set_new = has_neighours_in_set;
                else
                    has_neighours_in_set_new = (nautyg[i] & current_set_bitvector) > 0;

                current_set_bitvector_new = current_set_bitvector;
                ADDELEMENT1(&current_set_bitvector_new, i);

                current_set[current_set_size] = i;
                
                //recursie met curr set size + 1
                construct_dominating_sets_all_covered(current_set, current_set_size + 1, current_set_bitvector_new,
                        forbidden_vertices, min_set_size, max_set_size, has_neighours_in_set_new);
            }
        }
    }
    
}

/******************************************************************************/

//Important: it is assumed that at least one vertex isn't covered yet
static void
construct_dominating_sets_remove(unsigned char current_set[], int current_set_size, setword current_set_bitvector,
        setword covered_vertices, setword forbidden_vertices, int min_set_size, int max_set_size, int has_neighours_in_set) {
    
    //Geen setword *current_set_bitvector omdat dan delement nodig, nu gewoon kopie
    
    if(current_set_size >= max_set_size) {
        fprintf(stderr, "Error: set is too big\n");
        exit(1);
    }
    
    /*
     * First check if no edges can be inserted between vertices from the set
     * and forbidden vertices which are not in the dominating set without creating
     * a triangle (where the edges between vertices of the set are ignored, since
     * they will be removed by the operation).
     * If such an edge is found, this set cannot be good and also in all "supersets"
     * of this set it will still be possible to add that edge, so then we can 
     * abort the recursion.
     * 
     * Note 1: one only has to check this for vertices of the set where at least one
     * edge has been removed.
     * Note 2: it is still possible that the set is not "good", namely if one can
     * add an edge between a vertex from S an a nonforbidden vertex from S^c.
     */
/*
    setword forbidden_set_compl;
    if(has_neighours_in_set && (forbidden_set_compl = forbidden_vertices^current_set_bitvector)) {
        int i, j;
        for(i = 0; i < nv; i++) {
            if(ISELEMENT1(&forbidden_set_compl, i)) { //i ist verboten und in S^c
                for(j = 0; j < current_set_size; j++) {
                    if((nautyg[i] & nautyg[current_set[j]]&~current_set_bitvector) == 0 &&
                            !ISELEMENT1(&nautyg[i], current_set[j])) {
                        return;
                    }                    
                }
            }
        }
    }
*/
    setword forbidden_set_compl;
    if(has_neighours_in_set && (forbidden_set_compl = forbidden_vertices^current_set_bitvector)) {
        int i, j;
        for(i = 0; i < nv; i++) {
            //could use nextelement1 instead, but this is not a bottleneck
            if(ISELEMENT1(&forbidden_set_compl, i)) { //i ist verboten und in S^c
                setword neighbourhood = nautyg_loops[i] & (~current_set_bitvector);
                for(j = 0; j < current_set_size; j++) {
                    //This line is now the most expensive...
                    if((neighbourhood & nautyg[current_set[j]]) == 0) {
                        return;
                    }                    
                }
            }
        }
    }
    
/*
    int next_vertex = 0;
    //Remark: eventueel xor met alle bits of een not en dan firstbit?
    while(ISELEMENT1(&covered_vertices, next_vertex))
        next_vertex++;
*/
    //Important: it is assumed that not all vertices are covered!
    setword not_covered_vertices = ~covered_vertices;
    int next_vertex = FIRSTBIT(not_covered_vertices);
    
/*
    if(next_vertex >= nv) {
        fprintf(stderr, "Error: all vertices were already covered!\n");
        exit(1);
    }    
*/
    
    setword current_set_bitvector_new;
    setword covered_vertices_new;
    int has_neighours_in_set_new;
    
    //Step 1: include next_vertex in the dominating set
    if(!ISELEMENT1(&forbidden_vertices, next_vertex)) {
        ADDELEMENT1(&forbidden_vertices, next_vertex);

        covered_vertices_new = covered_vertices | nautyg_loops[next_vertex];
        if(has_neighours_in_set)
            has_neighours_in_set_new = has_neighours_in_set;
        else
            has_neighours_in_set_new = (nautyg[next_vertex] & current_set_bitvector) > 0;        
        
        current_set_bitvector_new = current_set_bitvector;
        ADDELEMENT1(&current_set_bitvector_new, next_vertex);
        
        current_set[current_set_size] = next_vertex;
        
        if(covered_vertices_new == ALLMASK(nv)) {
            //Further complete the sets
            construct_dominating_sets_all_covered(current_set, current_set_size + 1, current_set_bitvector_new,
                    forbidden_vertices, min_set_size, max_set_size, has_neighours_in_set_new);
        } else if(current_set_size + 1 < max_set_size) {
            //Set can still be expanded
            construct_dominating_sets_remove(current_set, current_set_size + 1, current_set_bitvector_new,
                covered_vertices_new, forbidden_vertices, min_set_size, max_set_size, has_neighours_in_set_new);
        }
        //current_set_size--;
    }
    
    
    
    //Step 2: next_vertex is forbidden, so at least one of the neighbours
    //of next_vertex must be included in the dominating set, otherwise it cannot
    //become a dominating set
    if((nautyg[next_vertex] & (~forbidden_vertices)) > 0) {
        //next_vertex has neighbours which are not forbidden
        int i;
        unsigned char neighbour;
        for(i = 0; i < current_graph.adj[next_vertex]; i++) {
            neighbour = current_graph.g[next_vertex][i];
            //Remark: of eventueel met marks werken... Is geen bottleneck!
            if(!ISELEMENT1(&forbidden_vertices, neighbour)) {
                ADDELEMENT1(&forbidden_vertices, neighbour);

                covered_vertices_new = covered_vertices | nautyg_loops[neighbour];
                if(has_neighours_in_set)
                    has_neighours_in_set_new = has_neighours_in_set;
                else
                    has_neighours_in_set_new = (nautyg[neighbour] & current_set_bitvector) > 0;

                current_set_bitvector_new = current_set_bitvector;
                ADDELEMENT1(&current_set_bitvector_new, neighbour);

                current_set[current_set_size] = neighbour;

                //Recursie
                //Remark: oppassen: neem current_set_size + 1 !!!
                if(covered_vertices_new == ALLMASK(nv)) {
                    //Further complete sets
                    construct_dominating_sets_all_covered(current_set, current_set_size + 1, current_set_bitvector_new,
                            forbidden_vertices, min_set_size, max_set_size, has_neighours_in_set_new);                    
                } else if(current_set_size + 1 < max_set_size) {
                    //Set can still be expanded
                    construct_dominating_sets_remove(current_set, current_set_size + 1, current_set_bitvector_new,
                            covered_vertices_new, forbidden_vertices, min_set_size, max_set_size, has_neighours_in_set_new);
                }
            }
        }
    }
    
}

/******************************************************************************/

/**
 * Find with simple path compression.
 */
static int 
find_root(int orbits[], int i) {
    while(i != orbits[i]) {
        orbits[i] = orbits[orbits[i]]; //Simple variant of path compression
        i = orbits[i];
    }
    return i;

}

/******************************************************************************/

/**
 * Unions the roots of a and b (if they weren't already equal) using union by size.
 */
static void 
union_elements(int orbits[], int root_orbits_size[], int *number_of_orbits, int a, int b) {
    int root_a = find_root(orbits, a);
    int root_b = find_root(orbits, b);
    //fprintf(stderr, "Union %d and %d (roots: %d and %d)\n", a, b, root_a, root_b);
    if(root_a != root_b) {
        if(root_orbits_size[root_a] < root_orbits_size[root_b]) {
            orbits[root_a] = root_b;
            root_orbits_size[root_b] += root_orbits_size[root_a];
        } else {
            orbits[root_b] = root_a;
            root_orbits_size[root_a] += root_orbits_size[root_b];
        }
        (*number_of_orbits)--;
    }
/*
    else {
        fprintf(stderr, "root of %d and %d are equal: %d\n", a, b, root_a);
    }
*/
}

/******************************************************************************/

/**
 * Determines the orbits of sets of vertices.
 *
 * Uses union by size with path compression for this.
 */
static void 
determine_vertexset_orbits(permutation generators_local[][MAXN1], int number_of_generators_local,
        unsigned char vertexset[][MAXN1], int number_of_vertexsets, int vertexset_sizes[],
        int vertexset_orbits[], int *number_of_orbits) {
//determine_vertexset_orbits(permutation generators_local[][MAXN1], int number_of_generators_local,
//        int num_vertices_in_set, unsigned char vertexset[][num_vertices_in_set], int vertexset_size, int vertexset_orbits[], int *number_of_orbits) {
    int i, j, k;
    unsigned char single_vertexset[MAXN1];

    //Remark: it is not a problem to create an array of size 0 or memcpy size 0
    int root_orbits_size[number_of_vertexsets];
    for(i = 0; i < number_of_vertexsets; i++) {
        vertexset_orbits[i] = i;
        root_orbits_size[i] = 1;
    }
    *number_of_orbits = number_of_vertexsets;

    //TODO: also no need to apply generator if all sets with the same size are already in the same orbit!
    permutation *perm;
    for(i = 0; i < number_of_generators_local; i++) {
        if(*number_of_orbits == 1)
            break;

        perm = generators_local[i];
        for(j = 0; j < number_of_vertexsets; j++) {
            if(*number_of_orbits == 1)
                break;
            
            for(k = 0; k < vertexset_sizes[j]; k++) {
                single_vertexset[k] = perm[vertexset[j][k]];
            }
            
            sort_set_ascending(single_vertexset, vertexset_sizes[j]);

/*
            for(k = 0; k < vertexset_sizes[j]; k++) {
                if(single_vertexset[k] != vertexset[j][k]) {
                    break;
                }
            }
            
            if(k == vertexset_sizes[j]) //Go to the next iteration if the vertexset wasn't modified by the generator
                continue;
*/
            
            //Go to the next iteration if the vertexset wasn't modified by the generator
            if(memcmp(single_vertexset, vertexset[j], vertexset_sizes[j]) == 0)
                continue;

            for(k = 0; k < number_of_vertexsets; k++) {
                if(vertexset_sizes[k] == vertexset_sizes[j]) {
/*
                    for(l = 0; l < vertexset_sizes[j]; l++) {
                        if(single_vertexset[l] != vertexset[k][l])
                            break;
                    }
                    if(l == vertexset_sizes[j]) //Vertexset found
                        break;
*/
                    //Will always be different from vertexset[j]
                    if(memcmp(single_vertexset, vertexset[k], vertexset_sizes[j]) == 0)
                        break; //Vertexset found
                }
            }
            if(k < number_of_vertexsets) {
                union_elements(vertexset_orbits, root_orbits_size, number_of_orbits, j, k);
                //break; //break not allowed!
            } 
            /*
             * Important: it can now happen that transformed sets are not found,
             * since we're not making all sets when there are double vertices.
             * But all equivalent sets will be found, since the generators 
             * are of the form (a b) for the double vertices.
             */
/*
            else {
                fprintf(stderr, "Error: transformed vertexset not found in vertexset list\n");
                printgraph();
                exit(1);
            }
*/
        }
    }

/*
    //Not needed: if the vertexset doesn't point to itself, it will never point to itself
    if(*number_of_orbits > 1) {
        for(i = 0; i < number_of_vertexsets; i++)
             vertexset_orbits[i] = find_root(vertexset_orbits, i);
    }
*/

}

/******************************************************************************/

/**
 * Uses some cheap heuristics to determine the canonical dominating set reduction.
 * Returns 1 if the last vertex is among the best dominating set vertices, else returns 0.
 * 
 * Important: it is assumed that the graph does not contain any double vertices
 * (since the double vertex operation has a higher priority)
 * 
 * If 1 is returned and has_to_call_nauty == 0, this means that the last vertex was
 * the only good_vertex. 
 * If there are multiple good vertices (which have a different neighbourhood 
 * than the last vertex), all good vertices are stored in good_vertices and 
 * has_to_call_nauty is set to 1.
 */
static int
is_best_dom_set_no_remove_reduction(unsigned char good_vertices[], int *num_good_vertices,
        int *has_to_call_nauty) {

    unsigned char valencies_last[current_graph.adj[nv - 1]];
    int sum_last = 0;
    int already_sorted = 0;
    int i;
    for(i = 0; i < current_graph.adj[nv - 1]; i++)
        sum_last += (valencies_last[i] = current_graph.adj[current_graph.g[nv - 1][i]]);
    //sort_set_ascending(valencies_last, current_graph.adj[nv - 1]);
    
    //Wait with this:
    //sort_set_descending(valencies_last, current_graph.adj[nv - 1]);

    unsigned char valencies_other[current_graph.adj[nv - 1]];
    int sum_other;

    *num_good_vertices = 0;
    
    //First test previous best vertices...
    if(last_removable_vertex != MAXN1 
            && current_graph.adj[last_removable_vertex] >= current_graph.adj[nv-1]) {
        if(graph_is_mtf_after_removing_vertex(last_removable_vertex)) {
            if(current_graph.adj[last_removable_vertex] > current_graph.adj[nv - 1]) {
                return 0;
            } else {
                int j;
                sum_other = 0;
                for(j = 0; j < current_graph.adj[nv - 1]; j++)
                    sum_other += (valencies_other[j] = current_graph.adj[current_graph.g[last_removable_vertex][j]]);

                if(sum_other > sum_last) {
                    return 0;
                } 
            }
        }
    }
    
    if(previous_best_removable_vertex != MAXN1 
            && current_graph.adj[previous_best_removable_vertex] >= current_graph.adj[nv-1]) {
        if(graph_is_mtf_after_removing_vertex(previous_best_removable_vertex)) {
            if(current_graph.adj[previous_best_removable_vertex] > current_graph.adj[nv - 1]) {
                return 0;
            } else {
                int j;
                sum_other = 0;
                for(j = 0; j < current_graph.adj[nv - 1]; j++)
                    sum_other += (valencies_other[j] = current_graph.adj[current_graph.g[previous_best_removable_vertex][j]]);

                if(sum_other > sum_last) {
                    return 0;
                } 
            }
        }
    }    
    
    
    num_removable_vertices = 0;
    removable_vertices_bitvector = 0;
    
    //Remark: excluden last en previous rejector?
    //Maar zal niet veel verschil maken aangezien het toch geen bottleneck is.
    for(i = 0; i < nv - 1; i++) {
        //TODO: lookaheads: als max deg > deg[nv-1] -> wanneer kan het gebeuren dat die removable zijn??
        if(current_graph.adj[i] >= current_graph.adj[nv-1]
                && graph_is_mtf_after_removing_vertex(i)) {
            if(current_graph.adj[i] > current_graph.adj[nv-1]) {
                previous_best_removable_vertex = i;
                return 0;
            } else {
                int j;
                sum_other = 0;
                for(j = 0; j < current_graph.adj[nv - 1]; j++)
                    sum_other += (valencies_other[j] = current_graph.adj[current_graph.g[i][j]]);
                
                if(sum_other > sum_last) {
                    previous_best_removable_vertex = i;
                    return 0;
                } else if(sum_other == sum_last) {
                    if(!already_sorted) {
                        sort_set_descending(valencies_last, current_graph.adj[nv - 1]);
                        already_sorted = 1;
                    }
                    
                    //sort_set_ascending(valencies_other, current_graph.adj[nv - 1]);
                    sort_set_descending(valencies_other, current_graph.adj[nv - 1]);

                    //Last vertex must have the largest valency vector
                    int res = memcmp(valencies_last, valencies_other, current_graph.adj[nv - 1]);
                    if(res < 0) {
                        previous_best_removable_vertex = i;
                        return 0;
                    } else if(res == 0)
                        good_vertices[(*num_good_vertices)++] = i;
                }
                removable_vertices[num_removable_vertices++] = i;
                ADDELEMENT1(&removable_vertices_bitvector, i);
            }
        }
    }
    
    if(*num_good_vertices == 0)
        *has_to_call_nauty = 0;
    else {
        *has_to_call_nauty = 1;
        //good_vertices[(*num_good_vertices)++] = nv - 1;
    }
    
    //Necessary for partitions
    good_vertices[(*num_good_vertices)++] = nv - 1;
    removable_vertices[num_removable_vertices++] = nv - 1;
    ADDELEMENT1(&removable_vertices_bitvector, nv - 1);
    
    return 1;
}

/******************************************************************************/

/**
 * Performs operation 2: adds a new vertex and connects it will all vertices of
 * the dominating set. 
 */
static void
expand_dominating_set_no_remove(unsigned char dominating_set[], int set_size) {
    nautyg[nv] = 0;
    int i;
    for(i = 0; i < set_size; i++) {
        current_graph.g[dominating_set[i]][current_graph.adj[dominating_set[i]]++] = nv;
        current_graph.g[nv][i] = dominating_set[i];
        ADDELEMENT1(&nautyg[dominating_set[i]], nv);
        ADDELEMENT1(&nautyg[nv], dominating_set[i]);
    }
    current_graph.adj[nv] = set_size;
    
    nv++;
}

/******************************************************************************/

/**
 * Reduction for operation 2.
 */
static void
reduce_dominating_set_no_remove(unsigned char dominating_set[], int set_size) {
    nv--;
    
    int i;
    for(i = 0; i < set_size; i++) {
        current_graph.adj[dominating_set[i]]--;
        DELELEMENT1(&nautyg[dominating_set[i]], nv);
    }
    
}

/******************************************************************************/

/**
 * Uses some cheap heuristics to determine the canonical dominating set remove reduction.
 * Returns 1 if the last vertex is among the best dominating set vertices, else returns 0.
 * 
 * The canonical vertex has minimal degree.
 * 
 * Important: it is assumed that the graph does not contain any double vertices
 * nor any other removable vertices.
 * 
 * If 1 is returned and has_to_call_nauty == 0, this means that the last vertex was
 * the only good_vertex. 
 * If there are multiple good vertices (which have a different neighbourhood 
 * than the last vertex), all good vertices are stored in good_vertices and 
 * has_to_call_nauty is set to 1.
 */
static int
is_best_dom_set_remove_reduction(unsigned char good_vertices[], int *num_good_vertices,
        int *has_to_call_nauty) {
    
    unsigned char valencies_last[current_graph.adj[nv - 1]];
    int sum_last = 0;
    int already_sorted = 0;
    int i;
    for(i = 0; i < current_graph.adj[nv - 1]; i++)
        sum_last += (valencies_last[i] = current_graph.adj[current_graph.g[nv - 1][i]]);
    //sort_set_ascending(valencies_last, current_graph.adj[nv - 1]);
    
    //Wait with this:
    //sort_set_descending(valencies_last, current_graph.adj[nv - 1]);

    unsigned char valencies_other[current_graph.adj[nv - 1]];
    int sum_other;

    *num_good_vertices = 0;
    
    //Important: it is assumed that last vertex has min degree 
    //(was enforced by lookahead during construction)

    //Remark: all vertices are valid reductions!
    for(i = 0; i < nv - 1; i++)
        if(current_graph.adj[i] == current_graph.adj[nv - 1]) {
            int j;
            sum_other = 0;
            for(j = 0; j < current_graph.adj[nv - 1]; j++)
                sum_other += (valencies_other[j] = current_graph.adj[current_graph.g[i][j]]);

            if(sum_other > sum_last)
                return 0;
            else if(sum_other == sum_last) {
                if(!already_sorted) {
                    sort_set_descending(valencies_last, current_graph.adj[nv - 1]);
                    already_sorted = 1;
                }

                //sort_set_ascending(valencies_other, current_graph.adj[nv - 1]);
                sort_set_descending(valencies_other, current_graph.adj[nv - 1]);

                //Last vertex must have the largest valency vector
                int res = memcmp(valencies_last, valencies_other, current_graph.adj[nv - 1]);
                if(res < 0)
                    return 0;
                else if(res == 0)
                    good_vertices[(*num_good_vertices)++] = i;
            }
        }
    
    if(*num_good_vertices == 0)
        *has_to_call_nauty = 0;
    else {
        *has_to_call_nauty = 1;
        //good_vertices[(*num_good_vertices)++] = nv - 1;
    }
    
    //Necessary for partitions
    good_vertices[(*num_good_vertices)++] = nv - 1;    
    
    return 1;
}

/******************************************************************************/

static int
find_best_insertable_edge_index(EDGE edge, EDGE best_insertable_edges[], int *num_best_insertable_edges) {
    int i;
    for(i = 0; i < *num_best_insertable_edges; i++)
        if(edge[0] == best_insertable_edges[i][0] && edge[1] == best_insertable_edges[i][1])
            return i;
    
    best_insertable_edges[*num_best_insertable_edges][0] = edge[0];
    best_insertable_edges[*num_best_insertable_edges][1] = edge[1];
    (*num_best_insertable_edges)++;
    
    return (*num_best_insertable_edges) - 1;
}

/******************************************************************************/

static void
fill_best_insertable_edges(int num_removed_edges, EDGE insertable_sets_of_edges[][num_removed_edges],
        int num_insertable_sets, EDGE best_insertable_edges[], int *num_best_insertable_edges, 
        int insertable_sets_of_edges_index[][num_removed_edges]) {
    int i, j;
    for(i = 0; i < num_insertable_sets; i++)
        for(j = 0; j < num_removed_edges; j++)
            insertable_sets_of_edges_index[i][j] = find_best_insertable_edge_index(insertable_sets_of_edges[i][j], 
                    best_insertable_edges, num_best_insertable_edges);
}

/******************************************************************************/

static int
is_canonical_set_of_insertable_edges_nauty(unsigned char dominating_set[], int set_size,
        int num_removed_edges, EDGE insertable_sets_of_edges[][num_removed_edges], 
        int num_insertable_sets, int removed_set_index) {
    
    int insertable_sets_of_edges_index[num_insertable_sets][num_removed_edges];
    EDGE best_insertable_edges[MAXN1*(MAXN1 - 1) / 2];
    int num_best_insertable_edges = 0;
    fill_best_insertable_edges(num_removed_edges, insertable_sets_of_edges, num_insertable_sets,
            best_insertable_edges, &num_best_insertable_edges, insertable_sets_of_edges_index);
    
/*
    fprintf(stderr, "best insertable edges:\n");
    int k, l;
    for(k = 0; k < num_insertable_sets; k++) 
        for(l = 0; l < num_removed_edges; l++) {
                fprintf(stderr, "[%d][%d]: %d and %d\n", k, l, insertable_sets_of_edges[k][l][0],
                        insertable_sets_of_edges[k][l][1]);
                fprintf(stderr, "index: %d\n", insertable_sets_of_edges_index[k][l]);
        }
    fprintf(stderr, "\n");
*/
    
    int nv_big = nv + num_best_insertable_edges + num_insertable_sets;
    if(nv_big > MAXN) {
        fprintf(stderr, "Error: intermediate big graph is too big: has %d vertices while MAXN is %d\n", 
                nv_big, MAXN);
        exit(1);
    }
    
    //Remark: is it not a problem that vertex with label nv-1 is included here???
    
    //Make the various colour classes

    //Class 1: the vertices of the dominating set
    //Contains at least 1 vertex
    RESETMARKS;
    int lab_index = 0;
    for(lab_index = 0; lab_index < set_size; lab_index++) {
        lab_big[lab_index] = dominating_set[lab_index];
        ptn_big[lab_index] = 1;
        MARK(dominating_set[lab_index]);
    }
    ptn_big[lab_index - 1] = 0;

    //Class 2: the other vertices of the graph
    //Also works if all vertices are in the dominating set
    int i;
    for(i = 0; i < nv; i++)
        if(!ISMARKED(i)) {
            lab_big[lab_index] = i;
            ptn_big[lab_index] = 1;
            lab_index++;
        }
    ptn_big[lab_index - 1] = 0;
    
/*
    if(lab_index != nv) {
        fprintf(stderr, "Error: lab index != nv\n");
        exit(1);
    }
*/
    
    //Class 3: new vertices which are in the middle of the inserted edges
    for(; lab_index < nv + num_best_insertable_edges; lab_index++) {
        lab_big[lab_index] = lab_index;
        ptn_big[lab_index] = 1;
    }
    ptn_big[lab_index - 1] = 0;
    
    //Class 4: new vertices which represent the possibilities
    for(; lab_index < nv_big; lab_index++) {
        lab_big[lab_index] = lab_index;
        ptn_big[lab_index] = 1;
    }
    ptn_big[lab_index - 1] = 0;
    
    //Copy original graph
    int j;
    set *gv;
    for(i = 0; i < nv; i++) {
        gv = GRAPHROW(nautyg_big, i, MAXM);
        EMPTYSET(gv, MAXM);
        for(j = 0; j < current_graph.adj[i]; j++)
            ADDELEMENT(gv, current_graph.g[i][j]);
    }

    set *gv_other;    
    //Insert vertices in the middle of the best insertable edges
    for(i = nv; i < nv + num_best_insertable_edges; i++) {
        gv = GRAPHROW(nautyg_big, i, MAXM);
        EMPTYSET(gv, MAXM);
        ADDELEMENT(gv, best_insertable_edges[i - nv][0]);
        ADDELEMENT(gv, best_insertable_edges[i - nv][1]);
        
        gv_other = GRAPHROW(nautyg_big, best_insertable_edges[i - nv][0], MAXM);
        ADDELEMENT(gv_other, i);
        
        gv_other = GRAPHROW(nautyg_big, best_insertable_edges[i - nv][1], MAXM);
        ADDELEMENT(gv_other, i);        
    }
    
    //TODO: beter van i = nv + num_best vertrekken zodat uniform is?
    //Insert vertices which represent the possibilities
    for(i = 0; i < num_insertable_sets; i++) {
        gv = GRAPHROW(nautyg_big, nv + num_best_insertable_edges + i, MAXM);
        EMPTYSET(gv, MAXM);
        for(j = 0; j < num_removed_edges; j++) {
            ADDELEMENT(gv, nv + insertable_sets_of_edges_index[i][j]);
            
            gv_other = GRAPHROW(nautyg_big, nv + insertable_sets_of_edges_index[i][j], MAXM);
            ADDELEMENT(gv_other, nv + num_best_insertable_edges + i);            
        }
    }
    
/*
    fprintf(stderr, "Org graph:\n");
    printgraph();
    
    fprintf(stderr, "dom set:\n");
    for(i = 0; i < set_size; i++)
        fprintf(stderr, "%d ", dominating_set[i]);
    fprintf(stderr, "\n");
    
    fprintf(stderr, "Big graph:\n");    
    printgraph_nauty_big(nautyg_big, nv_big);
*/
    
    
    nauty(nautyg_big, lab_big, ptn_big, NULL, orbits_big, &options_big, &stats_big, 
            workspace_big, WORKSIZEBIG, MAXM, nv_big, nautyg_big_canon);
    
    //Remark: removed_edges staat in bepaalde volgorde???
    //Gewoon de orbits bepalen en sorteren??
    //Canonical vertex is the one with the smallest label
    for(i = 0; i < nv_big; i++)
        if(lab_big[i] >= nv + num_best_insertable_edges)
            break;
    
    
    return orbits_big[lab_big[i]] == orbits_big[nv + num_best_insertable_edges + removed_set_index];
}

/******************************************************************************/

/**
 * Determines all possible sets with at most num_removed_edges edges which can be inserted
 * such that the reduced graph is mtf.
 * 
 * The canonical set has the smallest cardinality, so if a set with num_removed_edges
 * is found, the set with num_removed_edges cannot be canonical.
 * 
 * If better_set_found = 1, the last set was not canonical, else it is possibly
 * canonical and insertable_sets_of_edges contains the set which are also possibly
 * canonical.
 */
static void
determine_possible_sets_of_edges_to_insert(EDGE insertable_edges[], int num_insertable_edges,
        int next_index, EDGE current_set[], int current_set_size,
        int num_removed_edges, EDGE insertable_sets_of_edges[][num_removed_edges],
        int *num_insertable_sets, int *better_set_found) {
    
    //Test if set is sufficient, if so: return...
    //TODO: maybe no need to test the entire graph?
    if(graph_is_mtf_after_removing_vertex_inserted_edges(nv - 1)) {
        if(current_set_size < num_removed_edges)
            *better_set_found = 1;
        else { //current_set_size == num_removed_edges
            //TODO: temp boundary!
            if(*num_insertable_sets >= MAXN) {
                fprintf(stderr, "Error: too much insertable sets of edges!\n");
                exit(1);
            }
            
            int i;
            for(i = 0; i < current_set_size; i++) {
                insertable_sets_of_edges[*num_insertable_sets][i][0] = current_set[i][0];
                insertable_sets_of_edges[*num_insertable_sets][i][1] = current_set[i][1];
            }
            (*num_insertable_sets)++;
        }
        
        return;
    }
    
    if(current_set_size == num_removed_edges)
        return;

    int i;
    for(i = next_index; i < num_insertable_edges; i++)
        //First check if the edge is really insertable
        //Since it might become uninsertable by the insertion of an earlier edge
        if((nautyg[insertable_edges[i][0]] & nautyg[insertable_edges[i][1]] & ALLMASK(nv - 1)) == 0) {
            ADDELEMENT1(&nautyg[insertable_edges[i][0]], insertable_edges[i][1]);
            ADDELEMENT1(&nautyg[insertable_edges[i][1]], insertable_edges[i][0]);
            
            current_graph.g[insertable_edges[i][0]][current_graph.adj[insertable_edges[i][0]]] = insertable_edges[i][1];
            current_graph.adj[insertable_edges[i][0]]++;
            current_graph.g[insertable_edges[i][1]][current_graph.adj[insertable_edges[i][1]]] = insertable_edges[i][0];
            current_graph.adj[insertable_edges[i][1]]++;            
            
            current_set[current_set_size][0] = insertable_edges[i][0];
            current_set[current_set_size][1] = insertable_edges[i][1];
            current_set_size++;
            
            //Recursion
            determine_possible_sets_of_edges_to_insert(insertable_edges, num_insertable_edges,
                i+1, current_set, current_set_size, num_removed_edges, 
                    insertable_sets_of_edges, num_insertable_sets, better_set_found);
            
            DELELEMENT1(&nautyg[insertable_edges[i][0]], insertable_edges[i][1]);
            DELELEMENT1(&nautyg[insertable_edges[i][1]], insertable_edges[i][0]);            
            
            current_graph.adj[insertable_edges[i][0]]--;
            current_graph.adj[insertable_edges[i][1]]--;

            current_set_size--;
            
            if(*better_set_found)
                return;            
        }
        
    
}

/******************************************************************************/

/**
 * Returns 1 if the way the edges were removed were canonical, else returns 0.
 * 
 * We can determine the canonical vertex, but there are usually multiple possibilities
 * to insert edges to make the reduced graph mtf. The canonical possibility inserts
 * as few edges as possible. If there is more than one such possibility, we use
 * nauty to determine the canonical one.
 */
static int
is_canonical_way_to_remove_edges(unsigned char dominating_set[], int set_size,
        EDGE removed_edges[], int num_removed_edges) {
    EDGE insertable_edges[MAXN1*(MAXN1 - 1) / 2];
    int num_insertable_edges = 0;
    int i, j;
    //An edge is insertable if it has no common neighbours other than the vertex with label nv - 1
    for(i = 0; i < set_size - 1; i++)
        for(j = i + 1; j < set_size; j++) {
            if((nautyg[dominating_set[i]] & nautyg[dominating_set[j]] & ALLMASK(nv-1)) == 0) {
                insertable_edges[num_insertable_edges][0] = dominating_set[i];
                insertable_edges[num_insertable_edges][1] = dominating_set[j];
                num_insertable_edges++;
            }
        }
    
    //Not really necessary, but just in case
    if(num_insertable_edges < num_removed_edges) {
        fprintf(stderr, "Error: too few insertable edges: expected at least %d, but only found %d\n",
                num_removed_edges, num_insertable_edges);
        exit(1);
    }
    
    if(num_insertable_edges == 1)
        return 1;
    
    //fprintf(stderr, "Num removed edges: %d, num insertable: %d\n", num_removed_edges, num_removed_edges);
    
    
    //Try to insert minimal number of edges in order to make graph mtf
    //TODO: MAXN is slechte, tijdelijke grens!! Eigenlijk (n choose k)
    EDGE insertable_sets_of_edges[MAXN][num_removed_edges];
    EDGE current_set[num_removed_edges];
    int num_insertable_sets = 0;
    int better_set_found = 0;
    
    //Important: the sets determined by determine_possible_sets_of_edges_to_insert()
    //must be in the same order as the removed edges!
    determine_possible_sets_of_edges_to_insert(insertable_edges, num_insertable_edges,
                0, current_set, 0, num_removed_edges, insertable_sets_of_edges,
                &num_insertable_sets, &better_set_found);
    if(better_set_found)
        return 0;
    else {
        if(num_insertable_sets == 1)
            return 1;
        //fprintf(stderr, "More than one possibility: %d\n", num_insertable_sets);
        
        //More than one possibility so now we have to call nauty to determine the canonical one
        int original_set_index;
        for(original_set_index = 0; original_set_index < num_insertable_sets; original_set_index++) {
            for(i = 0; i < num_removed_edges; i++)
                if(removed_edges[i][0] != insertable_sets_of_edges[original_set_index][i][0]
                        || removed_edges[i][1] != insertable_sets_of_edges[original_set_index][i][1])
                    break;
            if(i == num_removed_edges)
                break;
        }
        if(original_set_index == num_insertable_sets) {
            fprintf(stderr, "Error: removed edges not found in sets of insertable edges\n");
            exit(1);
        }
        
        return is_canonical_set_of_insertable_edges_nauty(dominating_set, set_size,
                num_removed_edges, insertable_sets_of_edges, 
                num_insertable_sets, original_set_index);
        
    }
    
}

/******************************************************************************/

/**
 * Removes removed_vertex from the neighbourhood of vertex.
 */
static void 
remove_vertex_from_neighbourhood(unsigned char vertex, unsigned char removed_vertex) {
    /*
     * Remark: adjacency lists are always ordered, so could use binary search for this,
     * but that wouldn't really help since the degrees are usually not that big...
     */
    int i;
    for(i = 0; i < current_graph.adj[vertex]; i++)
        if(current_graph.g[vertex][i] == removed_vertex)
            break;
    if(i < current_graph.adj[vertex]) {
        i++;
        while(i < current_graph.adj[vertex]) {
            current_graph.g[vertex][i-1] = current_graph.g[vertex][i];
            i++;
        }
        current_graph.adj[vertex]--;
    } else {
        fprintf(stderr, "Error: vertex which is going to be removed was not a neighbour!\n");
        exit(1);
    }
}

/******************************************************************************/

/**
 * Performs operation 3: removes all edges between vertices of the set and
 * adds a new vertex and connects it will all vertices of the dominating set. 
 * It is assumed that the vertices of the dominating set have no common neighbours
 * (before expansion).
 */
static void
expand_dominating_set_remove(unsigned char dominating_set[], int set_size,
        EDGE removed_edges[], int *num_removed_edges) {
    int i, j;
    //Remove edges between vertices of the set
    *num_removed_edges = 0;
    for(i = 0; i < set_size - 1; i++)
        for(j = i + 1; j < set_size; j++)
            if(ISELEMENT1(&nautyg[dominating_set[i]], dominating_set[j])) {
                DELELEMENT1(&nautyg[dominating_set[i]], dominating_set[j]);
                DELELEMENT1(&nautyg[dominating_set[j]], dominating_set[i]);
                
                removed_edges[*num_removed_edges][0] = dominating_set[i];
                removed_edges[*num_removed_edges][1] = dominating_set[j];
                (*num_removed_edges)++;
                
                remove_vertex_from_neighbourhood(dominating_set[i], dominating_set[j]);
                remove_vertex_from_neighbourhood(dominating_set[j], dominating_set[i]);
            }
    
    //Add the new vertex and connect it
    nautyg[nv] = 0;
    for(i = 0; i < set_size; i++) {
        current_graph.g[dominating_set[i]][current_graph.adj[dominating_set[i]]++] = nv;
        current_graph.g[nv][i] = dominating_set[i];
        ADDELEMENT1(&nautyg[dominating_set[i]], nv);
        ADDELEMENT1(&nautyg[nv], dominating_set[i]);
    }
    current_graph.adj[nv] = set_size;
    
    nv++;
}

/******************************************************************************/

/**
 * Adds new_neighbour to the neighbourhood of vertex. It is placed in such a way
 * that the neighbourhood is sorted ascendingly
 */
static void 
add_vertex_to_neighbourhood(unsigned char vertex, unsigned char new_neighbour) {
    /*
     * Remark: adjacency lists are always ordered, so could use binary search for this,
     * but that wouldn't really help since the degrees are usually not that big...
     */
    int i;
    for(i = 0; i < current_graph.adj[vertex]; i++)
        if(new_neighbour < current_graph.g[vertex][i])
            break;
    
    int j;
    for(j = current_graph.adj[vertex]; j > i ;j--)
        current_graph.g[vertex][j] = current_graph.g[vertex][j-1];
    current_graph.g[vertex][j] = new_neighbour;
    current_graph.adj[vertex]++;
}

/******************************************************************************/

/**
 * Reduction for operation 3.
 * 
 * Important: it is assumed that the adjacencylists are restored elsewhere
 * (since it's a bit complicated to make sure the adjacencylists are identical
 * as before reduction).
 */
static void
reduce_dominating_set_remove(unsigned char dominating_set[], int set_size,
        EDGE removed_edges[], int num_removed_edges) {
    nv--;
    
    int i;
    for(i = 0; i < set_size; i++) {
        current_graph.adj[dominating_set[i]]--;
        DELELEMENT1(&nautyg[dominating_set[i]], nv);
    }
    
    for(i = 0; i < num_removed_edges; i++) {
        add_vertex_to_neighbourhood(removed_edges[i][0], removed_edges[i][1]);
        add_vertex_to_neighbourhood(removed_edges[i][1], removed_edges[i][0]);

        ADDELEMENT1(&nautyg[removed_edges[i][0]], removed_edges[i][1]);
        ADDELEMENT1(&nautyg[removed_edges[i][1]], removed_edges[i][0]);
    }
    
}

/******************************************************************************/

/*
 * Tries to map the vertices of subgraph to the vertices of testgraph_compl such
 * that subgraph is a subgraph of testgraph_compl.
 * If such a mapping is found, mapping_found is set to 1.
 * 
 * Important: it is assumed that the vertices with degree 0 are all at the end of rhf[].
 */
static void
try_to_map_vertices(unsigned char mapping[], setword mapped_vertices, unsigned char rhf[], int next_index, 
        setword available_vertices, GRAPH_STRUCT *subgraph, int *mapping_found,
        int nv_testgraph, GRAPH_STRUCT *testgraph_compl, graph *nautyg_testg_compl) {
    
    if(next_index == nv_testgraph) {
        *mapping_found = 1;
        return;
    }
    
    int next_vertex = rhf[next_index];
    
    //The remaining vertices are all isolated
    if(subgraph->adj[next_vertex] == 0) {
        *mapping_found = 1;
        return;        
    }    
    
    int i;
    setword possible_images = available_vertices;
    for(i = 0; i < subgraph->adj[next_vertex]; i++)
        if(ISELEMENT1(&mapped_vertices, subgraph->g[next_vertex][i]))
            possible_images &= nautyg_testg_compl[mapping[subgraph->g[next_vertex][i]]];
    
    //Now we're going to map next_vertex
    ADDELEMENT1(&mapped_vertices, next_vertex);

    setword available_vertices_new;
    i = -1;
    while((i = nextelement1(&possible_images, 1, i)) >= 0) {
        if(testgraph_compl->adj[i] >= subgraph->adj[next_vertex]) {
            mapping[next_vertex] = i;
            available_vertices_new = available_vertices;
            DELELEMENT1(&available_vertices_new, i);

            try_to_map_vertices(mapping, mapped_vertices, rhf, next_index + 1,
                    available_vertices_new, subgraph, mapping_found,
                    nv_testgraph, testgraph_compl, nautyg_testg_compl);
            
            if(*mapping_found)
                return;            
        }
    }
}

/******************************************************************************/

/*
 * Tries to map the vertices of subgraph to the vertices of testgraph_compl such
 * that subgraph is a subgraph of testgraph_compl.
 * If such a mapping is found, mapping_found is set to 1.
 * 
 * Important: it is assumed that the vertices with degree 0 are all at the end of rhf[].
 */
static void
try_to_map_vertices_exact(unsigned char mapping[], setword mapped_vertices, unsigned char rhf[], int next_index, 
        setword available_vertices, GRAPH_STRUCT *subgraph, int *mapping_found,
        int nv_testgraph, GRAPH_STRUCT *testgraph_compl, graph *nautyg_testg_compl) {
    
    if(next_index == nv_testgraph) {
        *mapping_found = 1;
        return;
    }
    
    int next_vertex = rhf[next_index];
    
    //The remaining vertices are all isolated
    if(subgraph->adj[next_vertex] == 0) {
        *mapping_found = 1;
        return;        
    }    
    
    int i;
    setword possible_images = available_vertices;
    for(i = 0; i < subgraph->adj[next_vertex]; i++)
        if(ISELEMENT1(&mapped_vertices, subgraph->g[next_vertex][i]))
            possible_images &= nautyg_testg_compl[mapping[subgraph->g[next_vertex][i]]];
    
    //Now we're going to map next_vertex
    ADDELEMENT1(&mapped_vertices, next_vertex);

    setword available_vertices_new;
    i = -1;
    while((i = nextelement1(&possible_images, 1, i)) >= 0) {
        if(testgraph_compl->adj[i] == subgraph->adj[next_vertex]) {
            mapping[next_vertex] = i;
            available_vertices_new = available_vertices;
            DELELEMENT1(&available_vertices_new, i);

            try_to_map_vertices_exact(mapping, mapped_vertices, rhf, next_index + 1,
                    available_vertices_new, subgraph, mapping_found,
                    nv_testgraph, testgraph_compl, nautyg_testg_compl);
            
            if(*mapping_found)
                return;            
        }
    }
}


/******************************************************************************/

/*
 * Sorts rhf in a greedy way.
 * It always takes the vertex which has most vertices in common with the
 * other vertices.
 * 
 * It does not sort the first start_sorting_from elements of rhf.
 * 
 * Important: it is assumed that rhf is already sorted according to descending valencies.
 */
static void 
sort_rhf_greedy_descending(ADJACENCY rhf, int start_sorting_from, int nv_graph, graph *nauty_graph) {
    setword testset = 0;
    ADJACENCY rhf_new;
    int rhf_new_size = 0;
    int i;
    RESETMARKS;
    for(i = 0; i < start_sorting_from; i++) {
        ADDELEMENT1(&testset, rhf[i]);
        rhf_new[rhf_new_size++] = rhf[i];      
        MARK(i); //rhf[i]        
    }

    int num_common;
    int max_num_common;
    int max_num_common_index;    
    while(rhf_new_size < nv_graph) {
        max_num_common = -1;
        for(i = start_sorting_from; i < nv_graph; i++)
            if(!ISMARKED(i)) {
                num_common = P_POPCOUNT(testset & nauty_graph[rhf[i]]);
                if(num_common > max_num_common) {
                    max_num_common = num_common;
                    max_num_common_index = i;
                }
            }
        ADDELEMENT1(&testset, rhf[max_num_common_index]);
        MARK(max_num_common_index);        
        rhf_new[rhf_new_size++] = rhf[max_num_common_index];
    }
    
    memcpy(rhf, rhf_new, sizeof(ADJACENCY));
}

/******************************************************************************/

/*
 * Returns 1 if subgraph is a subgraph of testgraph_complement, else returns 0.
 * Important: it is assumed that subgraph has nv_testgraph vertices.
 */
//Remark: voorlopig niet zo'n efficient algoritme, maar dit is wellicht geen bottleneck?
static int
is_subgraph_of_testgraph_compl(GRAPH_STRUCT *subgraph, graph *subgraph_nauty, 
        int num_edges_in_subgraph, int nv_testgraph, GRAPH_STRUCT *testgraph_compl, graph *nautyg_testg_compl) {
    ADJACENCY vertices;
    int i;
    for(i = 0; i < nv_testgraph; i++)
        vertices[i] = i;

    unsigned char temp;
    int modified = 1;
    int length = nv_testgraph;
    while(length > 1 && modified) {
        modified = 0;
        for(i = 0; i < length - 1; i++) {
            if(subgraph->adj[vertices[i]] < subgraph->adj[vertices[i + 1]]) { //sort descending
            //if(subgraph.adj[vertices[i]] > subgraph.adj[vertices[i + 1]]) { //sort ascending
                temp = vertices[i];
                vertices[i] = vertices[i + 1];
                vertices[i + 1] = temp;
                modified = 1;
            }
        }
        length--;
    }
    //Sorting it in this greedy way helps the algorithm to bound even sooner
    sort_rhf_greedy_descending(vertices, 1, nv_testgraph, subgraph_nauty);
    
    //Important: it is assumed that the vertices with degree 0 are all at the end of vertices[]
    
    unsigned char mapping[MAXN1];
    int mapping_found = 0;
    setword available_vertices = ALLMASK(nv_testgraph);
    
    //Remark: the subgraph has at least as much degree zero vertices as testgraph_compl
    
    if(num_edges_in_subgraph == ne_compl_testgraph)
        try_to_map_vertices_exact(mapping, 0, vertices, 0, available_vertices, 
                subgraph, &mapping_found, nv_testgraph, testgraph_compl, nautyg_testg_compl);
    else
        try_to_map_vertices(mapping, 0, vertices, 0, available_vertices, 
                subgraph, &mapping_found, nv_testgraph, testgraph_compl, nautyg_testg_compl);
    
    return mapping_found;
}

/******************************************************************************/

/**
 * Returns 1 if the current set is an induced subgraph in g of testgraph_compl,
 * else returns 0.
 * Remark: it is assumed that current_set contains nv_testgraph elements.
 */
static int 
set_is_induced_subgraph_of_testgraph(unsigned char current_set[], setword current_set_bitvector) {
    
    ADJACENCY adj_induced_subg;
    int i;
    int num_nonzero = 0;
    int num_edges_in_subgraph = 0;
    //TODO: also useful to test num max degree?
    for(i = 0; i < nv_testgraph; i++) {
        //P_POPCOUNT is faster than POPCOUNT?
        adj_induced_subg[i] = P_POPCOUNT(current_set_bitvector & nautyg[current_set[i]]);
        num_edges_in_subgraph += adj_induced_subg[i];
        if(adj_induced_subg[i] > max_degree_compl_testgraph) {
            return 0;
        }
        if(adj_induced_subg[i] > 0) {
            num_nonzero++;
            if(num_nonzero > num_nonzero_degree_compl_testgraph) {
                return 0;
            }
        }
    }
    
    num_edges_in_subgraph /= 2;
    
    sort_set_descending(adj_induced_subg, nv_testgraph);

    if(num_edges_in_subgraph == ne_compl_testgraph) {
        //The valency vector of the compl of the testraph must "dominate" the valency vector of the induced graph
        for(i = 0; i < nv_testgraph; i++)
            if(adj_induced_subg[i] != ordered_adj_compl_testgraph[i]) {
                return 0;
            }
    } else {
        //The valency vector of the compl of the testraph must "dominate" the valency vector of the induced graph
        for(i = 0; i < nv_testgraph; i++)
            if(adj_induced_subg[i] > ordered_adj_compl_testgraph[i]) {
                return 0;
            }
    }
    
    //Of bouwen induced subgraph voor een stuk al eerder doen?
    //Maakt niet echt een verschil...
    GRAPH_STRUCT induced_subgraph;
    graph nautyg_induced_subgraph[MAXN1*MAXM1];
    
    for(i = 0; i < nv_testgraph; i++) {
        nautyg_induced_subgraph[i] = 0;
        induced_subgraph.adj[i] = 0;
    }
    
    int j;
    for(i = 0; i < nv_testgraph; i++)
        for(j = i + 1; j < nv_testgraph; j++)
            if(ISELEMENT1(&nautyg[current_set[i]], current_set[j])) {
                ADDELEMENT1(&nautyg_induced_subgraph[i], j);
                ADDELEMENT1(&nautyg_induced_subgraph[j], i);
                add_edge(&induced_subgraph, i, j);
            }
    
    //printgraph_given(&induced_subgraph, nv_testgraph);
    //printgraph_nauty(nautyg_induced_subgraph, nv_testgraph);
    return is_subgraph_of_testgraph_compl(&induced_subgraph, nautyg_induced_subgraph, num_edges_in_subgraph,
                nv_testgraph, &testgraph_compl, nautyg_testg_compl);
}

/******************************************************************************/

/**
 * Returns 1 if the current set is an induced subgraph in g of testgraph_compl,
 * else returns 0.
 * Remark: it is assumed that current_set contains nv_testgraph elements.
 */
static int 
set_is_induced_subgraph_of_testgraph_v(unsigned char current_set[], setword current_set_bitvector) {
    
    ADJACENCY adj_induced_subg;
    int i;
    int num_nonzero = 0;
    int num_edges_in_subgraph = 0;
    for(i = 0; i < nv_testgraph_v; i++) {
        adj_induced_subg[i] = P_POPCOUNT(current_set_bitvector & nautyg[current_set[i]]);
        num_edges_in_subgraph += adj_induced_subg[i];
        if(adj_induced_subg[i] > max_degree_compl_testgraph_v) {
            return 0;
        }
        if(adj_induced_subg[i] > 0) {
            num_nonzero++;
            if(num_nonzero > num_nonzero_degree_compl_testgraph_v) {
                return 0;
            }
        }
    }
    
    num_edges_in_subgraph /= 2;
    
    sort_set_descending(adj_induced_subg, nv_testgraph_v);

    if(num_edges_in_subgraph == ne_compl_testgraph_v) {
        //The valency vector of the compl of the testraph must "dominate" the valency vector of the induced graph
        for(i = 0; i < nv_testgraph_v; i++)
            if(adj_induced_subg[i] != ordered_adj_compl_testgraph_v[i]) {
                return 0;
            }
    } else {
        //The valency vector of the compl of the testraph must "dominate" the valency vector of the induced graph
        for(i = 0; i < nv_testgraph_v; i++)
            if(adj_induced_subg[i] > ordered_adj_compl_testgraph_v[i]) {
                return 0;
            }
    }
    
    //TODO: of bouwen induced subgraph voor een stuk al eerder?
    GRAPH_STRUCT induced_subgraph;
    graph nautyg_induced_subgraph[MAXN1*MAXM1];
    
    for(i = 0; i < nv_testgraph_v; i++) {
        nautyg_induced_subgraph[i] = 0;
        induced_subgraph.adj[i] = 0;
    }
    
    int j;
    for(i = 0; i < nv_testgraph_v; i++)
        for(j = i + 1; j < nv_testgraph_v; j++)
            if(ISELEMENT1(&nautyg[current_set[i]], current_set[j])) {
                ADDELEMENT1(&nautyg_induced_subgraph[i], j);
                ADDELEMENT1(&nautyg_induced_subgraph[j], i);
                add_edge(&induced_subgraph, i, j);
            }
    
    return is_subgraph_of_testgraph_compl(&induced_subgraph, nautyg_induced_subgraph, num_edges_in_subgraph,
                nv_testgraph_v, &testgraph_compl_v, nautyg_testg_compl_v);
}

/******************************************************************************/

static void
make_induced_subgraphs_no_remaining_edges(unsigned char current_set[], int current_set_size, setword current_set_bitvector,
        ADJACENCY vertices, int next_index, setword possible_vertices, int num_possible_vertices, int *is_subgraph) {
    
    if(*is_subgraph) {
        fprintf(stderr, "Error: is subgraph, but still continuing with recursion.\n");
        exit(1);
    }
    
    if(current_set_size == nv_testgraph) {
        //Test if the graph induced by the set is a subgraph of testgraph_compl
        
        *is_subgraph = set_is_induced_subgraph_of_testgraph(current_set, current_set_bitvector);
        if(*is_subgraph) {
            //Save the "good" set
            int i;
            for(i = NUM_PREV_SETS - 1; i > 0; i--)
                previous_time_subgraph_sets[nv][i] = previous_time_subgraph_sets[nv][i - 1];
            previous_time_subgraph_sets[nv][0] = current_set_bitvector;

            //memcpy is a bit faster
            for(i = NUM_PREV_SETS - 1; i > 0; i--)
                memcpy(previous_time_subgraph_actual_set[nv][i], previous_time_subgraph_actual_set[nv][i - 1], sizeof(unsigned char) * MAXN1);

            //It is better to use MAXN2 instead of smallg->g[0][0], so optimizer can perform some optimizations
            memcpy(previous_time_subgraph_actual_set[nv][0], current_set, sizeof(unsigned char) * MAXN1);
        }        
        
        return;
    }
    
    if(current_set_size == nv_testgraph_v && num_good_sets_temp < MAX_NUM_GOOD_SETS_TEMP) {
        //Should not have too much vertices common with previous set!
        if(num_good_sets_temp == 0 ||
                POPCOUNT(current_set_bitvector & good_sets_temp[num_good_sets_temp - 1]) <= SETS_TEMP_MAX_NUM_COMMON) {
            good_sets_temp[num_good_sets_temp] = current_set_bitvector;
            memcpy(good_sets_temp_actual_set[num_good_sets_temp], current_set, sizeof(unsigned char) * MAXN1);
            num_good_sets_temp++;
        }
    }
    
    //Just in case...
    if(current_set_size > nv_testgraph) {
        fprintf(stderr, "Error: current set too big: %d vs max %d\n", current_set_size, nv_testgraph);
        exit(1);
    }
    
    int i;
    unsigned char vertex;
    setword current_set_bitvector_new;
    setword possible_vertices_new;
    int num_possible_vertices_new;
    //int max_size = nv - nv_testgraph + current_set_size + 1;
    int num_required = nv_testgraph - current_set_size;
    //Does not happen that i >= max_size!
    //for(i = next_index; (i < max_size) && num_possible_vertices >= num_required &&  !(*is_subgraph) ; i++) {
    for(i = next_index; (num_possible_vertices >= num_required) && !(*is_subgraph) ; i++) {
        vertex = vertices[i];
        if(ISELEMENT1(&possible_vertices, vertex)) {
            num_possible_vertices--;
            DELELEMENT1(&possible_vertices, vertex);
            //Neighbours of vertex are also forbidden since we can't have any more edges
            //P_POPCOUNT is faster than POPCOUNT?
            int num_extra_forbidden = P_POPCOUNT(possible_vertices & nautyg[vertex]);
            num_possible_vertices_new = num_possible_vertices - num_extra_forbidden;
            if(num_extra_forbidden > 0)
                possible_vertices_new = possible_vertices & ~nautyg[vertex];
            else
                possible_vertices_new = possible_vertices;

            current_set_bitvector_new = current_set_bitvector;
            ADDELEMENT1(&current_set_bitvector_new, vertex);
            current_set[current_set_size] = vertex;

            make_induced_subgraphs_no_remaining_edges(current_set, current_set_size + 1, 
                    current_set_bitvector_new, vertices, i + 1, possible_vertices_new, 
                    num_possible_vertices_new, is_subgraph);
        }
    }
}

/******************************************************************************/

/**
 * Makes all sets with nv_testgraph vertices which could be a subgraph of testgraph_compl.
 * If a set was found for which the induced graph is a subgraph of testgraph_compl,
 * is_subgraph is set to 1.
 */
//Remark: veronderstelt hier dat tenminste 1 remaining edge
static void
make_induced_subgraphs(unsigned char current_set[], int current_set_size, setword current_set_bitvector,
        setword possible_vertices, int num_possible_vertices, int max_num_remaining_edges, 
        ADJACENCY vertices, int next_index, int *is_subgraph) {
    
    if(*is_subgraph) {
        fprintf(stderr, "Error: is subgraph, but still continuing with recursion.\n");
        exit(1);
    }
    
    if(current_set_size == nv_testgraph) {
        //Test if the graph induced by the set is a subgraph of testgraph_compl
        
        *is_subgraph = set_is_induced_subgraph_of_testgraph(current_set, current_set_bitvector);
        if(*is_subgraph) {
            //Save the "good" set
            int i;
            for(i = NUM_PREV_SETS - 1; i > 0; i--)
                previous_time_subgraph_sets[nv][i] = previous_time_subgraph_sets[nv][i - 1];
            previous_time_subgraph_sets[nv][0] = current_set_bitvector;

            //memcpy is a bit faster
            for(i = NUM_PREV_SETS - 1; i > 0; i--)
                memcpy(previous_time_subgraph_actual_set[nv][i], previous_time_subgraph_actual_set[nv][i - 1], sizeof(unsigned char) * MAXN1);

            //It is better to use MAXN2 instead of smallg->g[0][0], so optimizer can perform some optimizations
            memcpy(previous_time_subgraph_actual_set[nv][0], current_set, sizeof(unsigned char) * MAXN1);
        }
        
        return;
    }
    
    if(current_set_size == nv_testgraph_v && num_good_sets_temp < MAX_NUM_GOOD_SETS_TEMP) {
        //Should not have too much vertices common with previous set!
        if(num_good_sets_temp == 0 ||
                POPCOUNT(current_set_bitvector & good_sets_temp[num_good_sets_temp - 1]) <= SETS_TEMP_MAX_NUM_COMMON) {
            good_sets_temp[num_good_sets_temp] = current_set_bitvector;
            memcpy(good_sets_temp_actual_set[num_good_sets_temp], current_set, sizeof(unsigned char) * MAXN1);
            num_good_sets_temp++;
        }
    }
    
    if(current_set_size > nv_testgraph) {
        fprintf(stderr, "Error: current set too big: %d vs max %d\n", current_set_size, nv_testgraph);
        exit(1);
    }
    
    int i;
    int num_new_edges;
    unsigned char vertex;
    setword current_set_bitvector_new;
    setword possible_vertices_new;
    int num_possible_vertices_new;
    //int max_size = nv - nv_testgraph + current_set_size + 1;
    int num_required = nv_testgraph - current_set_size;
    //Does not happen that i >= max_size
    //for(i = next_index; (i < max_size) && num_possible_vertices >= num_required &&  !(*is_subgraph) ; i++) {
    for(i = next_index; num_possible_vertices >= num_required &&  !(*is_subgraph); i++) {
        //Next vertex is never forbidden...
        vertex = vertices[i];
        DELELEMENT1(&possible_vertices, vertex);
        num_possible_vertices--;
        num_new_edges = POPCOUNT(current_set_bitvector & nautyg[vertex]);
        if(num_new_edges <= max_num_remaining_edges
                && num_new_edges <= max_degree_compl_testgraph) {
            current_set_bitvector_new = current_set_bitvector;
            ADDELEMENT1(&current_set_bitvector_new, vertex);
            current_set[current_set_size] = vertex;
            
            possible_vertices_new = possible_vertices;
            num_possible_vertices_new = num_possible_vertices;
            
            int num_remaining_edges = max_num_remaining_edges - num_new_edges;
            if(num_remaining_edges == 0) {
                //TODO: als current_set_size == nv_testgraph gewoon trysubgraph doen zonder
                //die sets te berekenen?
                //P_POPCOUNT is faster than POPCOUNT?
                int j;
                for(j = 0; j <= current_set_size; j++)
                    possible_vertices_new &= ~(nautyg[current_set[j]]);
                num_possible_vertices_new = P_POPCOUNT(possible_vertices_new); //possible_vertices_new is usually 0                
                
                make_induced_subgraphs_no_remaining_edges(current_set, current_set_size + 1,
                        current_set_bitvector_new, vertices, i + 1, possible_vertices_new, 
                        num_possible_vertices_new, is_subgraph);
            } else {
                make_induced_subgraphs(current_set, current_set_size + 1, current_set_bitvector_new,
                        possible_vertices_new, num_possible_vertices_new, 
                        num_remaining_edges, vertices, i + 1, is_subgraph);
            }
            
            //current_set_size-- and other restores not necessary now!
        }
    }
}

/******************************************************************************/

/*
 * Constructs a set with desired_set_size elements and also stores the
 * elements in current_set[].
 * This set is quite "sparse" in the sense that it tries to add vertices
 * which have as few vertices as possible in common with the other vertices.
 */
static void
construct_sparse_set(unsigned char current_set[], int *current_set_size, 
        setword *current_set_bitvector, int desired_set_size, int *ne_in_set, ADJACENCY rhf) {
    //The new vertex must certainly be in the set
    ADDELEMENT1(current_set_bitvector, rhf[0]);
    *current_set_size = 0;
    current_set[(*current_set_size)++] = rhf[0];
    RESETMARKS;
    MARK(0);
    
    int i;
    int num_common;
    int min_num_common;
    int min_num_common_index;
    *ne_in_set = 0;
    while(*current_set_size < desired_set_size) {
        min_num_common = INT_MAX;
        for(i = 1; i < nv; i++)
            if(!ISMARKED(i)) {
                num_common = P_POPCOUNT(*current_set_bitvector & nautyg[rhf[i]]);
                if(num_common < min_num_common) {
                    min_num_common = num_common;
                    min_num_common_index = i;
                }
                if(num_common == 0)
                    break;
            }
        ADDELEMENT1(current_set_bitvector, rhf[min_num_common_index]);
        MARK(min_num_common_index);
        (*ne_in_set) += min_num_common;
        current_set[(*current_set_size)++] = rhf[min_num_common_index];
    }
    
}

/******************************************************************************/

/*
 * Constructs some sets which might be subgraphs using a greedy heuristic.
 */
int is_subgraph_greedy() {
    ADJACENCY rhf;
    rhf[0] = nv - 1;
    int i;
    for(i = 1; i < nv; i++) 
        rhf[i] = i - 1;
    
    //Sorting by ascending degree
    unsigned char temp;
    int modified = 1;
    int length = nv;
    while(length > 1 && modified) {
        modified = 0;
        for(i = 1; i < length - 1; i++) {
            //if(current_graph.adj[rhf[i]] < current_graph.adj[rhf[i + 1]]) { //sort descending
            if(current_graph.adj[rhf[i]] > current_graph.adj[rhf[i + 1]]) { //sort ascending
                temp = rhf[i];
                rhf[i] = rhf[i + 1];
                rhf[i + 1] = temp;
                modified = 1;
            }
        }
        length--;
    }
    
    int ne_in_set = 0;
    ADJACENCY current_set;
    int current_set_size;
    setword current_set_bitvector = 0;
    construct_sparse_set(current_set, &current_set_size, &current_set_bitvector, 
            greedy_set_size, &ne_in_set, rhf);
    
    if(current_set_size != greedy_set_size) {
        fprintf(stderr, "Error: greedy set too small!\n");
        exit(1);
    }
    
    //If it already has too many edges it cant be a subgraph
    int rest_kantenzahl = ne_compl_testgraph - ne_in_set;
    if(rest_kantenzahl < 0) {
        return 0;
    }
    
    setword possible_vertices = ALLMASK(nv);
    possible_vertices &= ~current_set_bitvector;
    int num_possible_vertices = nv - greedy_set_size;    
    
    int num_remaining_edges = nv_testgraph - greedy_set_size;
    int is_subgraph = 0;
    int rhf_new_size = 0;
    for(i = 0; i < nv; i++)
        if(!ISMARKED(i))
            rhf[rhf_new_size++] = rhf[i];

    if(num_remaining_edges == 0) {
        int j;
        for(j = 0; j < greedy_set_size; j++)
            possible_vertices &= ~(nautyg[current_set[j]]);
        num_possible_vertices = P_POPCOUNT(possible_vertices); //possible_vertices_new is usually 0                

        make_induced_subgraphs_no_remaining_edges(current_set, current_set_size + 1,
                current_set_bitvector, rhf, 0, possible_vertices,
                num_possible_vertices, &is_subgraph);
    } else {
        make_induced_subgraphs(current_set, current_set_size, current_set_bitvector,
                possible_vertices, num_possible_vertices,
                num_remaining_edges, rhf, 0, &is_subgraph);
    }    
    
    return is_subgraph;
}

/******************************************************************************/

/*
 * Returns 1 if testg <= g^c.
 * 
 * For sparse graphs it is more efficient to test if testg^c or an induced
 * subgraph of testg^c appears as an induced subgraph in g.
 * 
 * More specifically constructs sets of nv_testgraph vertices and tests if the graph induced
 * by these vertices is a subgraph of testgraph_compl. If such a set is found, 1 is
 * returned and if no such set exists 0 is returned.
 * These sets can of course have at most ne_compl_testgraph edges otherwise
 * it cannot be a subgraph. We also use some other bounding criteria such as
 * max_degree_compl_testgraph.
 */
//Remark: could also add support for sparse graphs, but this is not a bottleneck
//as these graphs tend to have a small ramsey number
static int
complement_of_testgraph_is_contained_in_graph() {
    
    if(nv < nv_testgraph)
        return 0;

    int i;
    for(i = 0; i < NUM_PREV_SETS; i++)
        if(previous_time_subgraph_sets[nv][i] > 0
                && set_is_induced_subgraph_of_testgraph(previous_time_subgraph_actual_set[nv][i],
                previous_time_subgraph_sets[nv][i])) {
            return 1;
        }

    /* Use some greedy heuristic to try to find a good set */
    if(is_subgraph_greedy()) {
        return 1;
    }  
    
    ADJACENCY vertices;
    //Voorlopig gewoon standaard volgorde
    vertices[0] = nv - 1;
    for(i = 1; i < nv; i++)
        vertices[i] = i - 1;
    
    unsigned char temp;
    int modified = 1;
    int length = nv;
    while(length > 1 && modified) {
        modified = 0;
        for(i = 1; i < length - 1; i++) {
            if(current_graph.adj[vertices[i]] < current_graph.adj[vertices[i + 1]]) { //sort descending
            //if(current_graph.adj[vertices[i]] > current_graph.adj[vertices[i + 1]]) { //sort ascending
                temp = vertices[i];
                vertices[i] = vertices[i + 1];
                vertices[i + 1] = temp;
                modified = 1;
            }
        }
        length--;
    }
    //Sorting it in this greedy way helps the algorithm to bound even sooner
    sort_rhf_greedy_descending(vertices, 1, nv, nautyg);
    
    //Remark: this optimization doesn't help much at the moment since operation 3
    //is almost never the canonical operation for small vertex numbers
    ADJACENCY vertices_1_removed;
    if(num_edges_removed == 1) {
        vertices_1_removed[0] = nv - 1;
        vertices_1_removed[1] = removed_edge[0];
        vertices_1_removed[2] = removed_edge[1];
        for(i = 1; i < nv; i++)
            vertices_1_removed[i] = i - 1;
        
        unsigned char vertex = 0;
        for(i = 3; i < nv; i++) {
            while(vertex == removed_edge[0] || vertex == removed_edge[1])
                vertex++;
            vertices_1_removed[i] = vertex++;            
        }

        modified = 1;
        length = nv;        
        while(length > 1 && modified) {
            modified = 0;
            for(i = 3; i < length - 1; i++) {
                if(current_graph.adj[vertices_1_removed[i]] < current_graph.adj[vertices_1_removed[i + 1]]) { //sort descending
                    //if(current_graph.adj[vertices_1_removed[i]] > current_graph.adj[vertices_1_removed[i + 1]]) { //sort ascending
                    temp = vertices_1_removed[i];
                    vertices_1_removed[i] = vertices_1_removed[i + 1];
                    vertices_1_removed[i + 1] = temp;
                    modified = 1;
                }
            }
            length--;
        }
        sort_rhf_greedy_descending(vertices_1_removed, 3, nv, nautyg);
    }

    
    num_good_sets_temp = 0;
    
    setword possible_vertices = ALLMASK(nv);
    int num_possible_vertices = nv;
    
    unsigned char current_set[MAXN1];
    int is_subgraph = 0;
    int current_set_size = 0;
    int max_size = nv - nv_testgraph + current_set_size + 1;
    if(ne_compl_testgraph > 0) {
        for(i = 0; i < max_size && !is_subgraph; i++) {
            if(num_edges_removed != 1 || i != 1) {
                current_set[current_set_size] = vertices[i];
                setword current_set_bitvector = 0;
                ADDELEMENT1(&current_set_bitvector, vertices[i]);

                DELELEMENT1(&possible_vertices, current_set[current_set_size]);
                num_possible_vertices--;

                make_induced_subgraphs(current_set, current_set_size + 1, current_set_bitvector,
                        possible_vertices, num_possible_vertices, ne_compl_testgraph, vertices, i + 1, &is_subgraph);

                /**
                 * If no edges were removed, the sets must contain the last vertex (nv - 1),
                 * otherwise they cannot be subgraphs since the parent was a ramseygraph.
                 */
                if(total_num_double_vertices > 0 || num_removable_vertices > 0)
                    break;
                //return is_subgraph;
            } else {
                current_set[0] = removed_edge[0];
                current_set[1] = removed_edge[1];
                setword current_set_bitvector = 0;
                ADDELEMENT1(&current_set_bitvector, removed_edge[0]);
                ADDELEMENT1(&current_set_bitvector, removed_edge[1]);

                DELELEMENT1(&possible_vertices, removed_edge[0]);
                DELELEMENT1(&possible_vertices, removed_edge[1]);
                num_possible_vertices -= 2;

                //Since edge is removed, these 2 vertices are no neighbours
                make_induced_subgraphs(current_set, 2, current_set_bitvector,
                        possible_vertices, num_possible_vertices, ne_compl_testgraph, vertices_1_removed, 3, &is_subgraph);

                //return is_subgraph;
                break;
            }
        }
    } else {
        setword possible_vertices_new;
        for(i = 0; i < max_size && !is_subgraph; i++) {
            if(num_edges_removed != 1 || i != 1) {
                current_set[current_set_size] = vertices[i];
                setword current_set_bitvector = 0;
                ADDELEMENT1(&current_set_bitvector, vertices[i]);

                DELELEMENT1(&possible_vertices, current_set[current_set_size]);
                //num_possible_vertices--;
                
                possible_vertices_new = possible_vertices & ~(nautyg[current_set[current_set_size]]);
                //TODO: use P_POPCOUNT?
                num_possible_vertices = POPCOUNT(possible_vertices_new);

                make_induced_subgraphs_no_remaining_edges(current_set, current_set_size + 1,
                        current_set_bitvector, vertices, i + 1, possible_vertices_new, 
                        num_possible_vertices, &is_subgraph);

                /**
                 * If no edges were removed, the sets must contain the last vertex (nv - 1),
                 * otherwise they cannot be subgraphs since the parent was a ramseygraph.
                 */
                if(total_num_double_vertices > 0 || num_removable_vertices > 0)
                    break;
                //return is_subgraph;
            } else {
                current_set[0] = removed_edge[0];
                current_set[1] = removed_edge[1];
                setword current_set_bitvector = 0;
                ADDELEMENT1(&current_set_bitvector, removed_edge[0]);
                ADDELEMENT1(&current_set_bitvector, removed_edge[1]);

                DELELEMENT1(&possible_vertices, removed_edge[0]);
                DELELEMENT1(&possible_vertices, removed_edge[1]);
                //num_possible_vertices -= 2;
                
                possible_vertices_new = possible_vertices & ~(nautyg[current_set[0]])
                        & ~(nautyg[current_set[1]]);
                //POPCOUNT is faster than P_POPCOUNT?
                num_possible_vertices = POPCOUNT(possible_vertices_new);

                //Since edge is removed, these 2 vertices are no neighbours
                //Warning BUGFIX: has to use vertices_1_removed instead of vertices here!
                make_induced_subgraphs_no_remaining_edges(current_set, 2,
                        current_set_bitvector, vertices_1_removed, 3, possible_vertices_new, 
                        num_possible_vertices, &is_subgraph);                

                //return is_subgraph;
                break;
            }
        }
    }
    
    //The graph is a ramseygraph
    if(!is_subgraph) {
        num_good_sets_v[nv] = 0;
        
        /**
         * Finding good sets is quite expensive, so we just investigate previous sets 
         * and test if there are any good ones among them.
         */
        
        //fprintf(stderr, "num_good_sets_temp: %d\n", num_good_sets_temp);
        
        //First check the good_sets with nv_testgraph_v vertices which we saved during the construction
        for(i = 0; i < num_good_sets_temp; i++)
            if(set_is_induced_subgraph_of_testgraph_v(good_sets_temp_actual_set[i], good_sets_temp[i])) {
                good_sets_v[nv][num_good_sets_v[nv]] = good_sets_temp[i];
                //memcpy(good_sets_v_actual_set[num_good_sets_v], testset, sizeof(unsigned char) * MAXN1);
                num_good_sets_v[nv]++;

                //- MIN_NUM_PREV_SETS so we still can add some previous sets
                if(num_good_sets_v[nv] == MAX_NUM_GOOD_SETS_V - MIN_NUM_PREV_SETS)
                    break;
            }
        
        
        unsigned char testset[MAX_NUM_GOOD_SETS_V];
        setword testset_bitvector;
        
        int j;
        for(i = 0; i < NUM_PREV_SETS; i++)
            if(previous_time_subgraph_sets[nv][i] > 0) {
                testset_bitvector = 0;
                for(j = 0; j < nv_testgraph_v; j++) {
                    testset[j] = previous_time_subgraph_actual_set[nv][i][j];
                    ADDELEMENT1(&testset_bitvector, testset[j]);
                }
                if(set_is_induced_subgraph_of_testgraph_v(testset, testset_bitvector)) {
                    good_sets_v[nv][num_good_sets_v[nv]] = testset_bitvector;
                    //memcpy(good_sets_v_actual_set[num_good_sets_v], testset, sizeof(unsigned char) * MAXN1);
                    num_good_sets_v[nv]++;

                    if(num_good_sets_v[nv] == MAX_NUM_GOOD_SETS_V)
                        break;
                }
            }
        //fprintf(stderr, "num_good_sets_v: %d\n", num_good_sets_v);
        
    }
    
    return is_subgraph;
}

/******************************************************************************/

//Remark: defaultptn is also quite good since the degrees are very different
//So maybe better to use defaultptn instead?
static void
make_nauty_partitions(unsigned char good_vertices[], int num_good_vertices) {
    //Remark: ook partities opdelen volgens graad of gebeurt dat al onmiddellijk door nauty??
    //Hoogstwaarschijnlijk wel ja!
    
    //More advanced partitions: aantal buren gemeenschappelijk met good vertices
    int i;
    int lab_index = 0;
    setword good_vertices_bitvector = 0;
    RESETMARKS;
    for(i = 0; i < num_good_vertices; i++) {
        lab[lab_index] = good_vertices[i];
        ptn[lab_index] = 1;
        lab_index++;
        MARK(good_vertices[i]);
        ADDELEMENT1(&good_vertices_bitvector, good_vertices[i]);
    }
    ptn[lab_index - 1] = 0;
    
    unsigned char number_good_neighbours[nv];
    int max_num_good_neighbours = 0;
    //vertex with label nv - 1 is always good
    for(i = 0; i < nv - 1; i++)
        if(!ISMARKED(i)) {
            //TODO: P_POPCOUNT?
            number_good_neighbours[i] = POPCOUNT(nautyg[i] & good_vertices_bitvector);
            if(number_good_neighbours[i] > max_num_good_neighbours)
                max_num_good_neighbours = number_good_neighbours[i];
        } else {
            number_good_neighbours[i] = MAXN1;
        }
    
    int j;
    for(i = 0; i <= max_num_good_neighbours; i++) {
        for(j = 0; j < nv - 1; j++)
            if(number_good_neighbours[j] == i) {
                lab[lab_index] = j;
                ptn[lab_index] = 1;
                lab_index++;
            }
        ptn[lab_index - 1] = 0;
    }
    
    use_defaultptn = 0;
    
}

/******************************************************************************/

/**
 * Main method of the generation algorithm.
 * Takes the current graph and performs all possible (canonical) extensions on it.
 * It is assumed that the current graph is canonical.
 * 
 * If call_nauty is 1, this means that the automorphism group of the current
 * graph is not determined yet.
 */
static void
construct(int call_nauty) {
    
    if(ramseygraph_found)
        return;
    
    if(nv == splitlevel) {
        splitcounter++;
        
        //Is cheaper than using % ?
        if(splitcounter == modulo)
            splitcounter = 0;
        
        if(splitcounter != rest)
            return;
    }
    
    number_of_graphs_generated[nv]++;
    
    if(ramsey && complement_of_testgraph_is_contained_in_graph()) {
        return;
    }
    
    if(ramsey)
        number_of_ramsey_graphs_generated[nv]++;

    if(nv >= outputlevel) {
        output_graph(&current_graph, nv);
    }

    if(nv == maxnv) {
        if(ramsey && !write_all_ramseygraphs)
            ramseygraph_found = 1;
        return;
    }

    if(call_nauty) {
        options.getcanon = FALSE;
        
        if(use_defaultptn)
           options.defaultptn = TRUE;
        else
            options.defaultptn = FALSE;

        number_of_generators = 0;

        //nautyg is updated dynamically
        //copy_nauty_graph();

        //printgraph_nauty(nautyg, nv);
        nauty(nautyg, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM1, nv, NULL);
        //print_generators(generators, number_of_generators);

    }
    
    
    /**
     * There are 3 "operations" which have the following priority
     * (actually the 3 operations are basically the same):
     * 
     * 1. Double a vertex.
     * 2. Add a new vertex and connect it to a good dominating set without removing
     *    edges.
     * 3. Add a new vertex and connect it to a good dominating set where edges
     *    have to be removed.
     */

    //Backup generators and orbits because they will be overwritten
    int orbits_local[nv];
    memcpy(orbits_local, orbits, sizeof(int) * nv);
    int number_of_vertex_orbits = stats.numorbits;
    int num_orbits = 0;

    //Remark: it is not a problem to create an array of size 0 or memcpy size 0
    permutation generators_local[number_of_generators][MAXN1];
    memcpy(generators_local, generators, sizeof(permutation) * number_of_generators * MAXN1);
    int number_of_generators_local = number_of_generators;
    
    int max_degree_dv_local = max_degree_dv;

    //Remark: betere grens mogelijk...
    unsigned char good_vertices[MAXN1];
    int num_good_vertices;
    int has_to_call_nauty;    
    
    unsigned char last_removable_vertex_local = last_removable_vertex;
    unsigned char removable_vertices_local[MAXN1];
    int num_removable_vertices_local = num_removable_vertices;
    memcpy(removable_vertices_local, removable_vertices, sizeof(unsigned char) * num_removable_vertices);
    setword removable_vertices_bitvector_local = removable_vertices_bitvector;

    /* "Operation" 1: double a vertex */

    //Perform operation 1 on one vertex of each equivalence class of vertices
    int i;
    for(i = 0; i < nv; i++) {
        //Canonical double vertex must have max degree
        if(orbits_local[i] == i && current_graph.adj[i] >= max_degree_dv_local) {
            //Remark: could perform more LA's in advance, but expand_double_vertex() is not a bottleneck
            //Perhaps don't update every field in expand_double_vertex()?
            
            expand_double_vertex(i);
            
            //dubbele vertices altijd zoeken is misschien goedkoper dan dynamisch updaten?

            if(is_best_double_vertex_reduction(good_vertices, &num_good_vertices, &has_to_call_nauty)
                    && (!ramsey || can_be_ramseygraph(nautyg[nv-1], nv - 1))) {
                int is_canonical = 1;
                
/*
                if(has_to_call_nauty || nv < maxnv) {
                    //Put the double vertices in a separate partition
                    
                    //TODO: also use size of their neighbourhood? (doesn't really cost anything)
                    //Or put good vertices in other partition as well?
                    //For the remaining vertices: eg according to num of dv neighbours?
                    
                    //Important: must put dv's of the same size in the same partition because
                    //they might be in the same orbit after expansion, even if they weren't before expansion
                    
                    //Also can't really predict the group afterwards, since vertices which
                    //were in the same orbit before expansion, might be in a different one after expansion
                    
                    int j, k;
                    int lab_index = 0;
                    RESETMARKS;
                    for(j = 0; j < num_dv_neighbourhoods; j++)
                        for(k = 0; k < num_double_vertices[j]; k++) {
                            lab[lab_index] = double_vertices[j][k];
                            ptn[lab_index] = 1;
                            lab_index++;
                            MARK(double_vertices[j][k]);
                        }
                    ptn[lab_index - 1] = 0;
                    
                    for(j = 0; j < nv - 1; j++)
                        if(!ISMARKED(j)) {
                            lab[lab_index] = j;
                            ptn[lab_index] = 1;
                            lab_index++;
                        }
                    ptn[lab_index - 1] = 0;
                    use_defaultptn = 0;
                }
*/

                if(has_to_call_nauty) {
                    if(number_of_generators_local == 0) {
                        //All vertices in separate orbit, except i and nv-1
                        int j;
                        int lab_index = 0;
                        for(j = 0; j < nv - 1; j++)
                            if(j != i) {
                                lab[lab_index] = j;
                                ptn[lab_index] = 0;
                                lab_index++;                                
                            }

                        lab[lab_index] = i;
                        ptn[lab_index] = 1;
                        lab_index++;    
                        
                        lab[lab_index] = nv - 1;
                        ptn[lab_index] = 0;
                        lab_index++;
                        
                        use_defaultptn = 0;
                    } else
                        make_nauty_partitions(good_vertices, num_good_vertices);
                }

                //i.e. cheap heuristics were not enough to determine canonicity
                if(has_to_call_nauty) {
                    options.getcanon = TRUE;
                    //options.defaultptn = TRUE;
                    options.defaultptn = FALSE;

                    number_of_generators = 0;
                    nauty(nautyg, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM1, nv, nautyg_canon);
                    
                    is_canonical = is_canonical_vertex(good_vertices, num_good_vertices, lab, orbits);
                } 

                if(is_canonical) {
                    number_of_graphs_generated_double_vertex[nv]++;
                    max_degree_dv = current_graph.adj[i];
                    
                    if(!has_to_call_nauty && nv < maxnv) {
                        //Knowns exactly what the orbits will be (but this doesn't help much since it's not a bottleneck?)
                        if(number_of_generators_local == 0) {
                            int j;
                            for(j = 0; j < nv - 1; j++) {
                                generators[0][j] = j;
                                orbits[j] = j;
                            }
                            generators[0][i] = nv - 1;
                            generators[0][nv - 1] = i;
                            orbits[nv - 1] = i;
                            stats.numorbits = nv - 1;

                            number_of_generators = 1;

                            has_to_call_nauty = 1;
                        } else
                            make_nauty_partitions(good_vertices, num_good_vertices);
                    }
                    last_removable_vertex = MAXN1;
                    num_removable_vertices = 0;
                    num_edges_removed = 0;
                    construct(!has_to_call_nauty);
                }
            }
            
            reduce_double_vertex(i);
            
            //Remark: not sure if this will really help since mostly trivial group?
            num_orbits++;
            if(num_orbits == number_of_vertex_orbits) {
                last_removable_vertex = last_removable_vertex_local;
                num_removable_vertices = num_removable_vertices_local;
                memcpy(removable_vertices, removable_vertices_local, sizeof(unsigned char) * num_removable_vertices);
                removable_vertices_bitvector = removable_vertices_bitvector_local;
                break;
            }
        }
    }


    /* 
     * "Operations" 2 and 3: search for good dominating sets where not all elements
     * of the set have a common neighbour (this is a set from "operation" 1).
     * TODO: maybe merge this with operation 1 as well?
     */    

    /* "Operation" 2: add a new vertex and connect it to a good dominating set without removing edges. */
    
    /*
     * Since this operation does not remove any edges, it cannot destroy any existing
     * operations of type 1:
     * If there are 2 double vertices and the set contains one vertex, it must
     * also contain the other vertex otherwise the resulting graph is not maximal.
     */
    if(total_num_double_vertices == 0) {
        //Remark: could already do this during construction or reuse it for operation 3
        //But this is not a bottleneck...
        for(i = 0; i < nv; i++) {
            nautyg_loops[i] = nautyg[i];
            ADDELEMENT1(&nautyg_loops[i], i);
        }
        
        unsigned char current_set[MAXN1];
        number_of_dominating_sets_global = 0;
        
        //kan restrictiever want weer bv dat canonische top max degree moet hebben
        //(onder de removable vertices)
        //Kan bestaande removable vertices kapotmaken? Removable vertices bijhouden??
        
        construct_dominating_sets_no_remove(current_set, 0, 0, 0, 0, 0, 0);
        
        //Copy the dominating sets to local variables
        //Remark: it is not a problem to create an array of size 0 or memcpy size 0
        int number_of_dominating_sets = number_of_dominating_sets_global;
        ADJACENCY dominating_sets[number_of_dominating_sets];
        int dominating_sets_size[number_of_dominating_sets];
        memcpy(dominating_sets, dominating_sets_global, sizeof(ADJACENCY) * number_of_dominating_sets_global);
        memcpy(dominating_sets_size, dominating_sets_size_global, sizeof(int) * number_of_dominating_sets_global);
        
/*
        fprintf(stderr, "Printing dominating sets:\n");
        for(i = 0; i < number_of_dominating_sets; i++) {
            fprintf(stderr, "%d: ", i);
            int j;
            for(j = 0; j < dominating_sets_size[i]; j++)
                fprintf(stderr, "%d ", dominating_sets[i][j]);
            fprintf(stderr, "\n");
        }
*/
        
        //TODO: already use some lookaheads to limit number of vertexsets before 
        //computing the orbits!
        //Maybe also store the vertexsets per size? Or use linked lists? See mtf...
        //But computing orbits is not expensive since it's usually a trivial group
        //(otherwise mostly a double vertex)?
        
        int number_of_dom_set_orbits;
        int dominating_sets_orbits[number_of_dominating_sets];
        //TODO: kan aparte orbits bepalen volgens size?
        determine_vertexset_orbits(generators_local, number_of_generators_local,
                dominating_sets, number_of_dominating_sets, dominating_sets_size,
                dominating_sets_orbits, &number_of_dom_set_orbits);

        //TODO: enkel als tenminste 1 element in set? -> continue?
        int total_num_double_vertices_local = total_num_double_vertices;
        unsigned char double_vertices_local[MAXN1][MAXN1];
        int num_double_vertices_local[MAXN1];
        if(total_num_double_vertices_local > 0) {
            memcpy(double_vertices_local, double_vertices, sizeof(unsigned char) * MAXN1 * MAXN1);
            memcpy(num_double_vertices_local, num_double_vertices, sizeof(int) * MAXN1);
        }
        setword dv_neighbourhoods_local[MAXN1];
        int num_dv_neighbourhoods_local = num_dv_neighbourhoods;
        if(num_dv_neighbourhoods_local > 0)
            memcpy(dv_neighbourhoods_local, dv_neighbourhoods, sizeof(setword) * MAXN1);   
        
        unsigned char previous_best_removable_vertex_local = MAXN1;
        
        num_orbits = 0;
        for(i = 0; i < number_of_dominating_sets; i++) {
            if(dominating_sets_orbits[i] == i) {
                
                //fprintf(stderr, "graph before expansion:\n");
                //printgraph();
                
                expand_dominating_set_no_remove(dominating_sets[i], dominating_sets_size[i]);
                
                //fprintf(stderr, "graph after expansion:\n");
                //printgraph();

                /*
                 * Remark: only possibility to generate a double vertex is if all vertices 
                 * of the set have one common neighbour and the neighbourhood of that
                 * neighbour is the dominating set. But these sets were already 
                 * filtered out during the construction. 
                 */
                previous_best_removable_vertex = previous_best_removable_vertex_local;
                
                //Remark: it is better to perform can_be_ramseygraph(nautyg[nv-1], nv - 1), since then often the
                //more expensive graph_is_mtf_after_removing_vertex() in is_best_dom_set_no_remove_reduction()
                //doesn't have to be performed
                if((!ramsey || can_be_ramseygraph(nautyg[nv-1], nv - 1)) 
                        && is_best_dom_set_no_remove_reduction(good_vertices, &num_good_vertices, &has_to_call_nauty)) {
                    int is_canonical = 1;

                    if(has_to_call_nauty || nv < maxnv)
                        make_nauty_partitions(good_vertices, num_good_vertices);                    

                    //i.e. cheap heuristics were not enough to determine canonicity
                    if(has_to_call_nauty) {
                        options.getcanon = TRUE;
                        //options.defaultptn = TRUE;
                        options.defaultptn = FALSE;

                        number_of_generators = 0;
                        nauty(nautyg, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM1, nv, nautyg_canon);

                        is_canonical = is_canonical_vertex(good_vertices, num_good_vertices, lab, orbits);
                    } 

                    //Kan misschien ook vermijden dat in volgende stap nauty moet oproepen?
                    //Want kan groep al voorspellen??
                    if(is_canonical) {
                        total_num_double_vertices = num_dv_neighbourhoods = max_degree_dv = 0;
                        number_of_graphs_generated_rem_vertex[nv]++;
                        last_removable_vertex = nv - 1;
                        num_edges_removed = 0;
                        construct(!has_to_call_nauty);
                    }
                } else {
                    previous_best_removable_vertex_local = previous_best_removable_vertex;
                }
                
                
                reduce_dominating_set_no_remove(dominating_sets[i], dominating_sets_size[i]);
                
                last_removable_vertex = last_removable_vertex_local;
                
                //fprintf(stderr, "restored graph:\n");
                //printgraph();
                
                //Not sure if this will really help since mostly trivial group?
                num_orbits++;
                if(num_orbits == number_of_dom_set_orbits) {
                    //Restore lists of double vertices
                    total_num_double_vertices = total_num_double_vertices_local;
                    if(total_num_double_vertices_local > 0) {
                        memcpy(double_vertices, double_vertices_local, sizeof(unsigned char) * MAXN1 * MAXN1);
                        memcpy(num_double_vertices, num_double_vertices_local, sizeof(int) * MAXN1);
                    }
                    num_dv_neighbourhoods = num_dv_neighbourhoods_local;
                    if(num_dv_neighbourhoods_local > 0)
                        memcpy(dv_neighbourhoods, dv_neighbourhoods_local, sizeof(setword) * MAXN1);
                    
                    //last_removable_vertex = last_removable_vertex_local;
                    num_removable_vertices = num_removable_vertices_local;
                    memcpy(removable_vertices, removable_vertices_local, sizeof(unsigned char) * num_removable_vertices);
                    removable_vertices_bitvector = removable_vertices_bitvector_local;
                    break;
                }
            }
        }
        
    }

    /* "Operation" 3: add a new vertex and connect it to a good dominating which involves removing edges. */
    
    //Triple vertices cannot be destroyed by operation 3
    for(i = 0; i < num_dv_neighbourhoods; i++)
        if(num_double_vertices[i] > 2)
            return;    
    
    //Search ALL removable vertices, not just the "good" ones (with max degree)
    //These removable vertices are used as lookahead in is_good_dominating_set_remove
    if(num_removable_vertices > 0) {
        int i;
        //nv - 1 is certainly removable and already in list!
        for(i = 0; i < nv - 1; i++)
            if(!ISELEMENT1(&removable_vertices_bitvector, i)
                    && graph_is_mtf_after_removing_vertex(i)) {
                removable_vertices[num_removable_vertices++] = i;
                ADDELEMENT1(&removable_vertices_bitvector, i);
            }
    }

    int minval = MAXN1;
    int num_minval = 0;
    for(i = 0; i < nv; i++) {
        if(current_graph.adj[i] == minval)
            num_minval++;
        else if(current_graph.adj[i] < minval) {
            minval = current_graph.adj[i];
            num_minval = 1;
        }
        
        nautyg_loops[i] = nautyg[i];
        ADDELEMENT1(&nautyg_loops[i], i);
    }
            
    number_of_dominating_sets_global = 0;
    unsigned char current_set[minval];
    setword current_set_bitvector = 0;
    
    //First construct all good dominating sets with <= minval elements
    
    if(total_num_double_vertices == 0) {
        construct_dominating_sets_remove(current_set, 0, 0, 0, 0, 0, minval, 0);
        
    } else if(num_dv_neighbourhoods <= minval) {
        //The minimal number of elements in a set in order to destroy all double vertices
        /*
         * In order to destroy all double vertices, the dominating set must
         * contain at least num_double_vertices[i] - 1 vertices of each
         * double vertex neighbourhood, otherwise there would still be at least
         * 2 double vertices in that neighbourhood left.
         * 
         * Unfortunately it is not necessary to destroy all double vertices of
         * a double vertex neighbourhood, since it's possible that the last
         * double vertex isn't removable anymore (see C4 example).
         */
        /*
         * Note: operation 3 can't destroy triple vertices, so these are already filtered out.
         * So each dv_neighbourhood contains exactly 2 vertices.
         */
/*
        int min_num_elements_in_set = 0;
        for(i = 0; i < num_dv_neighbourhoods; i++)
            min_num_elements_in_set += num_double_vertices[i] - 1;
*/
        
        //Cannot use these extra bounding criteria since the neighbourhoods may have common neighbours
        //And there may even be a different dv in the neighbourhood of a dv
        current_set_bitvector = 0;
        setword covered_vertices = 0;
        int current_set_size = 0;
        int has_neighours_in_set = 0;
        if(num_dv_neighbourhoods == 1) {
            
            /*
             * Special optimization (atm only used if there is just one dv_neighbourhood,
             * otherwise it is too much hassle). But appearently this optimization
             * such help much. This is probably because the bottleneck is
             * construct_dominating_sets_remove() when there are no double vertices?
             */
            
            //Add the first vertex of the dv neighbourhood
            //All sets have size 2
            if(2 <= minval) {
                current_set[current_set_size++] = double_vertices[0][0];
                covered_vertices |= nautyg_loops[double_vertices[0][0]];
                ADDELEMENT1(&current_set_bitvector, double_vertices[0][0]);
                setword forbidden_vertices = current_set_bitvector;
                setword covered_vertices_new;
                unsigned char neighbour;
                for(i = 0; i < current_graph.adj[double_vertices[0][0]]; i++) {
                    //Try to put each neighbour once in the set
                    neighbour = current_graph.g[double_vertices[0][0]][i];
                    current_set[current_set_size++] = neighbour;
                    covered_vertices_new = covered_vertices | nautyg_loops[neighbour];
                    ADDELEMENT1(&current_set_bitvector, neighbour);
                    ADDELEMENT1(&forbidden_vertices, neighbour);
                    
                    if(covered_vertices == ALLMASK(nv))
                        construct_dominating_sets_all_covered(current_set, current_set_size, current_set_bitvector,
                            forbidden_vertices, 0, minval, 1);
                    else if(current_set_size < minval) //Not all covered yet and set is still expandable
                        construct_dominating_sets_remove(current_set, current_set_size, current_set_bitvector,
                            covered_vertices_new, forbidden_vertices, 0, minval, 1);
                    
                    current_set_size--;
                    DELELEMENT1(&current_set_bitvector, neighbour);
                    //But neighbour stays forbidden!
                }
            }            
        } else {
            for(i = 0; i < num_dv_neighbourhoods; i++) {
                //Add the first vertex of the dv neighbourhood
                //All sets have size 2
                current_set[current_set_size++] = double_vertices[i][0];
                covered_vertices |= nautyg_loops[double_vertices[i][0]];
                if(!has_neighours_in_set)
                    has_neighours_in_set = (nautyg[double_vertices[i][0]] & current_set_bitvector) > 0;
                ADDELEMENT1(&current_set_bitvector, double_vertices[i][0]);
            }
            //if(current_set_size <= minval) {
            if(covered_vertices == ALLMASK(nv))
                construct_dominating_sets_all_covered(current_set, current_set_size, current_set_bitvector,
                    current_set_bitvector, 0, minval, has_neighours_in_set);
            else if(current_set_size < minval) //Not all covered yet and set is still expandable
                construct_dominating_sets_remove(current_set, current_set_size, current_set_bitvector,
                    covered_vertices, current_set_bitvector, 0, minval, has_neighours_in_set);
        }
    }
    
    //Now construct the sets with minval + 1 elements
    
    if(num_minval <= minval + 1) {
        current_set_bitvector = 0;
        setword covered_vertices = 0;
        int current_set_size = 0;
        int has_neighours_in_set = 0;
        for(i = 0; i < nv; i++)
            if(current_graph.adj[i] == minval) {
                current_set[current_set_size++] = i;
                covered_vertices |= nautyg_loops[i];
                if(!has_neighours_in_set)
                    has_neighours_in_set = (nautyg[i] & current_set_bitvector) > 0;
                ADDELEMENT1(&current_set_bitvector, i);
            }
        if(total_num_double_vertices > 0) {
            for(i = 0; i < num_dv_neighbourhoods && current_set_size < minval + 1; i++) {
                if(!ISELEMENT1(&current_set_bitvector, double_vertices[i][0])) {
                    //Add the first vertex of the dv neighbourhood
                    //All sets have size 2
                    current_set[current_set_size++] = double_vertices[i][0];
                    covered_vertices |= nautyg_loops[double_vertices[i][0]];
                    if(!has_neighours_in_set)
                        has_neighours_in_set = (nautyg[double_vertices[i][0]] & current_set_bitvector) > 0;
                    ADDELEMENT1(&current_set_bitvector, double_vertices[i][0]);
                    if(current_set_size == minval + 1)
                        break;
                }
            }
        }
        if(current_set_size <= minval + 1) {
            if(covered_vertices == ALLMASK(nv))
                construct_dominating_sets_all_covered(current_set, current_set_size, current_set_bitvector,
                    current_set_bitvector, minval + 1, minval + 1, has_neighours_in_set);
            else if(current_set_size < minval + 1) //Not all covered yet and set is still expandable
                construct_dominating_sets_remove(current_set, current_set_size, current_set_bitvector,
                    covered_vertices, current_set_bitvector, minval + 1, minval + 1, has_neighours_in_set);
        }        
    }
    
    int total_num_double_vertices_local = total_num_double_vertices;
    unsigned char double_vertices_local[MAXN1][MAXN1];
    int num_double_vertices_local[MAXN1];
    if(total_num_double_vertices_local > 0) {
        memcpy(double_vertices_local, double_vertices, sizeof(unsigned char) * MAXN1 * MAXN1);
        memcpy(num_double_vertices_local, num_double_vertices, sizeof(int) * MAXN1);
    }
    setword dv_neighbourhoods_local[MAXN1];
    int num_dv_neighbourhoods_local = num_dv_neighbourhoods;
    if(num_dv_neighbourhoods_local > 0)
        memcpy(dv_neighbourhoods_local, dv_neighbourhoods, sizeof(setword) * MAXN1);

    //Dominating sets were  determined, so now copy them to a local array
    //Remark: it is not a problem to create an array of size 0 or memcpy size 0
    int number_of_dominating_sets = number_of_dominating_sets_global;
    ADJACENCY dominating_sets[number_of_dominating_sets];
    int dominating_sets_size[number_of_dominating_sets];
    memcpy(dominating_sets, dominating_sets_global, sizeof(ADJACENCY) * number_of_dominating_sets_global);
    memcpy(dominating_sets_size, dominating_sets_size_global, sizeof(int) * number_of_dominating_sets_global);    

    int number_of_dom_set_orbits;
    int dominating_sets_orbits[number_of_dominating_sets];
    determine_vertexset_orbits(generators_local, number_of_generators_local,
            dominating_sets, number_of_dominating_sets, dominating_sets_size,
            dominating_sets_orbits, &number_of_dom_set_orbits);
    
    //TODO: sharper bound possible...
    EDGE removed_edges[MAXN1*(MAXN1 - 1) / 2];
    int num_removed_edges;
    
    num_orbits = 0;
    for(i = 0; i < number_of_dominating_sets; i++) {
        if(dominating_sets_orbits[i] == i) {

/*
            fprintf(stderr, "graph before expansion:\n");
            printgraph();
*/
            
            expand_dominating_set_remove(dominating_sets[i], dominating_sets_size[i],
                    removed_edges, &num_removed_edges);

/*
            fprintf(stderr, "graph after expansion:\n");
            printgraph();
*/
            
            
            //Remark: it is best to first test if it can be a ramseygraph, like that
            //we can avoid calling nauty in is_canonical_way_to_remove_edges() if it cannot be a ramseygraph.
            if((!ramsey || can_be_ramseygraph(nautyg[nv-1], nv - 1)) && is_best_dom_set_remove_reduction(good_vertices, &num_good_vertices, &has_to_call_nauty)
                    && is_canonical_way_to_remove_edges(dominating_sets[i], dominating_sets_size[i],
                            removed_edges, num_removed_edges)) {
                
                int is_canonical = 1;
                
                if(has_to_call_nauty || nv < maxnv)
                    make_nauty_partitions(good_vertices, num_good_vertices);

                //i.e. cheap heuristics were not enough to determine canonicity
                if(has_to_call_nauty) {
                    options.getcanon = TRUE;
                    //options.defaultptn = TRUE;
                    options.defaultptn = FALSE;

                    number_of_generators = 0;
                    nauty(nautyg, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM1, nv, nautyg_canon);

                    is_canonical = is_canonical_vertex(good_vertices, num_good_vertices, lab, orbits);
                } 

                //TODO: kan misschien ook vermijden dat in volgende stap nauty moet oproepen?
                //Want kan groep al voorspellen?? Nee, niet echt!
                if(is_canonical) {
                    total_num_double_vertices = num_dv_neighbourhoods = max_degree_dv = 0;
                    num_removable_vertices = 0;
                    number_of_graphs_generated_no_rem_vertex[nv]++;
                    last_removable_vertex = MAXN1;
                    num_edges_removed = num_removed_edges;
                    if(num_removed_edges == 1) {
                        removed_edge[0] = removed_edges[0][0];
                        removed_edge[1] = removed_edges[0][1];
                    }
                    construct(!has_to_call_nauty);
                }
            }
            
            reduce_dominating_set_remove(dominating_sets[i], dominating_sets_size[i],
                    removed_edges, num_removed_edges);
            
/*
            fprintf(stderr, "restored graph:\n");
            printgraph();
*/

            //Remark: not sure if this will really help since mostly trivial group?
            num_orbits++;
            if(num_orbits == number_of_dom_set_orbits) {
                //Restore lists of double vertices
                total_num_double_vertices = total_num_double_vertices_local;
                if(total_num_double_vertices_local > 0) {
                    memcpy(double_vertices, double_vertices_local, sizeof(unsigned char) * MAXN1 * MAXN1);
                    memcpy(num_double_vertices, num_double_vertices_local, sizeof(int) * MAXN1);
                }
                num_dv_neighbourhoods = num_dv_neighbourhoods_local;
                if(num_dv_neighbourhoods_local > 0)
                    memcpy(dv_neighbourhoods, dv_neighbourhoods_local, sizeof(setword) * MAXN1);                
                last_removable_vertex = last_removable_vertex_local;
                num_removable_vertices = num_removable_vertices_local;
                memcpy(removable_vertices, removable_vertices_local, sizeof(unsigned char) * num_removable_vertices);                
                removable_vertices_bitvector = removable_vertices_bitvector_local;
                break;
            }
        }
    }    


}

/******************************************************************************/

/**
 * Reads a graph from the file and stores it into g and nautyg. It is assumed
 * that the file is already opened and on return the file is still left open.
 * 
 * Returns EOF if EOF is met, else 1 is returned.
 */
static int
read_inputgraph(FILE *fil, GRAPH_STRUCT *g, int *nv, graph *nauty_graph) {
    
    *nv = getc(fil);
    if(*nv == EOF)
        return EOF;
    
    if(*nv > maxnv) {
        fprintf(stderr, "Error: input graph is too big: can have at most %d vertices, but has %d vertices \n",
                maxnv, *nv);
        exit(1);
    }
    
    if(*nv < 3) {
        fprintf(stderr, "Error: input graph should have at least 3 vertices, but has only %d vertices\n", nv_testgraph);
        exit(1);
    }
    
    unsigned char code[MAX_MUTLICODE_SIZE(*nv)];
    code[0] = *nv;
    int codelength = 1;
    int num_zeros = 0;
    int temp;
    
    while(num_zeros < *nv - 1) {
        //code is unsigned char, so cannot be EOF == -1
        temp = getc(fil);
        
        if(temp == EOF) {
            fprintf(stderr, "Error: premature EOF\n");
            exit(1);
        }
        
        code[codelength] = temp;
            
        if(code[codelength] == 0) 
            num_zeros++;
        codelength++;
    }
    
    //fclose(fil);
    
    //Now decode the multicode
    //Remark: misschien nuttig om toppen van de graaf te sorteren volgens dalende graad e.d.?
    decode_multicode(code, codelength, g);
    
    //printgraph_given(g, *nv);
    
    copy_nauty_graph(g, *nv, nauty_graph);
    //printgraph_nauty(nauty_graph, *nv);
    
    return 1;
    
}

/******************************************************************************/

/**
 * Sets current_graph to P3.
 * 
 * Graph:
 * 0---1---2
 */
static void
read_P3() {
    current_graph.g[0][0] = 1;
    current_graph.adj[0] = 1;

    current_graph.g[1][0] = 0;
    current_graph.g[1][1] = 2;
    current_graph.adj[1] = 2;

    current_graph.g[2][0] = 1;
    current_graph.adj[2] = 1;

    nv = 3;

    copy_nauty_graph(&current_graph, nv, nautyg);

    dv_neighbourhoods[0] = nautyg[0];
    double_vertices[0][0] = 0;
    double_vertices[0][1] = 2;
    num_double_vertices[0] = 2;
    num_dv_neighbourhoods = 1;
    
    total_num_double_vertices = 2;

}

/******************************************************************************/

/**
 * Reads the testgraph for option 'ramsey' from testgraph_filename and also
 * fills the required global variables such as testgraph_compl.
 */
static void
read_testgraph() {
    
    FILE *fil = repeated_fopen(testgraph_filename, "rb");
    if(fil == NULL) {
        fprintf(stderr, "Error: can't open %s\n", testgraph_filename);
        exit(1);
    }
    
    if(read_inputgraph(fil, &testgraph, &nv_testgraph, nautyg_testg) == EOF) {
        fprintf(stderr, "Error: file %s is empty\n", testgraph_filename);
        exit(1);
    }
    
    if(getc(fil) != EOF)  {
        fprintf(stderr, "Warning: file '%s' contains multiple graphs, but only the first one will be used\n", testgraph_filename);
    }
    
    fclose(fil);
    
    greedy_set_size = nv_testgraph - 3;
    if(greedy_set_size < 3) {
        fprintf(stderr, "Error: greedy set size is too small to be useful (%d)\n", greedy_set_size);
        exit(1);
    }
    
    //printgraph_given(&testgraph, nv_testgraph);
    //printgraph_nauty(nautyg_testg, nv_testgraph);
    
    make_inverse_graphs_testgraph();
    
/*
    fprintf(stderr, "inverse graphs:\n");
    printgraph_nauty(nautyg_testg_compl, nv_testgraph);
    printgraph_given(&testgraph_compl, nv_testgraph);
*/
    
}

/******************************************************************************/

static void
init_nauty_options() {
    options.userautomproc = save_generators;
    
    options_big.getcanon = TRUE;
    options_big.defaultptn = FALSE;    
}

/******************************************************************************/

/**
 * Sets the global variables for the graph (such as the double vertices etc.).
 */
static void
set_global_variables() {
    num_dv_neighbourhoods = total_num_double_vertices = 0;
    max_degree_dv = 0;
    int i, j;
    RESETMARKS;
    for(i = 0; i < nv - 1; i++)
        if(!ISMARKED(i)) {
            dv_neighbourhoods[num_dv_neighbourhoods] = nautyg[i];
            double_vertices[num_dv_neighbourhoods][0] = i;
            num_double_vertices[num_dv_neighbourhoods] = 1;
            for(j = i + 1; j < nv; j++)
                if(nautyg[j] == dv_neighbourhoods[num_dv_neighbourhoods]) {
                    double_vertices[num_dv_neighbourhoods][num_double_vertices[num_dv_neighbourhoods]++] = j;
                    total_num_double_vertices++;
                    MARK(j);
                }
            if(num_double_vertices[num_dv_neighbourhoods] > 1) {
                total_num_double_vertices++;
                num_dv_neighbourhoods++;
                if(current_graph.adj[i] > max_degree_dv)
                    max_degree_dv = current_graph.adj[i];
            }
        }
    
    //removable vertices are only used as lookahead, so it's not a problem if this is 0
    last_removable_vertex = MAXN1;
    num_removable_vertices = 0;
    
    //Only used for option 'ramsey', even if edges were removed it's not a problem
    //since the saved graphs are always ramseygraphs!
    num_edges_removed = 0;

    if(ramsey) {
        //Do this just in case not all input graphs have the same nv
        for(i = 0; i <= nv; i++)
            num_good_sets_v[i] = 0;
    }
    
    use_defaultptn = 1;
}


/******************************************************************************/

static void
init_construction() {
    init_nauty_options();

    if(!noout && write_all_ramseygraphs) {
        sprintf(ramseygraphs_filename, "Ramseygraphs_%d_%s", maxnv, testgraph_filename);

        if(modulo > 1) {
            char strbuffer[50];
            sprintf(strbuffer, ".mod_%d_%d", rest, modulo);
            strcat(ramseygraphs_filename, strbuffer);
        }
    }

    dominating_sets_global = (ADJACENCY *) malloc(sizeof(ADJACENCY) * max_dominating_sets_global);
    if(dominating_sets_global == NULL) {
        fprintf(stderr, "Error: out of memory while creating dominating_sets_global\n");
        exit(1);
    }

    dominating_sets_size_global = (int *) malloc(sizeof(int) * max_dominating_sets_global);
    if(dominating_sets_size_global == NULL) {
        fprintf(stderr, "Error: out of memory while creating dominating_sets_size_global\n");
        exit(1);
    }
    number_of_dominating_sets_global = 0;

    
    //Start construction
    if(inputgraphs_filename == NULL) {
        //The construction starts from P3
        read_P3();
        construct(1);
    } else {
        FILE *fil = repeated_fopen(inputgraphs_filename, "rb");
        if(fil == NULL) {
            fprintf(stderr, "Error: can't open %s\n", inputgraphs_filename);
            exit(1);
        }
        
        while(read_inputgraph(fil, &current_graph, &nv, nautyg) != EOF) {
            number_of_inputgraphs_read++;
            
            set_global_variables();
            construct(1);
        }
        
        fclose(fil);
    }

}

/******************************************************************************/

static void
initialize_splitting() {
    if(modulo == 1)
        splitlevel = 0;
    else {
        splitlevel = MAX(maxnv * 2/3, MIN_SPLITLEVEL);
        splitlevel = MIN(splitlevel, maxnv - 1);
        fprintf(stderr, "Splitlevel is %d\n", splitlevel);
    }
}

/******************************************************************************/

static void
print_generators(permutation generators[][MAXN1], int num_of_generators) {
    int i, j;
    fprintf(stderr, "Number of generators: %d\n", num_of_generators);
    for(i = 0; i < num_of_generators; i++) {
        for(j = 0; j < nv; j++)
            fprintf(stderr, "%d ", generators[i][j]);
        fprintf(stderr, "\n");
    }
}

/******************************************************************************/

/**
 * Prints the given graph.
 */
static void
printgraph_given(GRAPH_STRUCT *g, int num_vertices) {
    int i, j;
    fprintf(stderr, "Printing graph:\n");
    for(i = 0; i < num_vertices; i++) {
        fprintf(stderr, "%d :", i);
        for(j = 0; j < g->adj[i]; j++)
            fprintf(stderr, " %d", g->g[i][j]);
        fprintf(stderr, "\n");
    }
}

/******************************************************************************/

/**
 * Prints the current graph.
 */
static void
printgraph() {
    printgraph_given(&current_graph, nv);
}

/******************************************************************************/

/**
 * Prints the given nauty graph.
 */
static void
printgraph_nauty(graph *g, int current_number_of_vertices) {
    int i;
    fprintf(stderr, "Printing graph nauty:\n");
    for(i = 0; i < current_number_of_vertices; i++) {
        fprintf(stderr, "%d :", i);
        set *gv = GRAPHROW1(g, i, MAXM1);
        int neighbour = -1;
        while((neighbour = nextelement(gv, MAXM1, neighbour)) >= 0) {
            fprintf(stderr, " %d", neighbour);
        }
        fprintf(stderr, "\n");
    }
}

/******************************************************************************/

/**
 * Prints the given nauty graph.
 */
static void
printgraph_nauty_big(graph *g, int current_number_of_vertices) {
    int i;
    fprintf(stderr, "Printing graph nauty:\n");
    for(i = 0; i < current_number_of_vertices; i++) {
        fprintf(stderr, "%d :", i);
        set *gv = GRAPHROW(g, i, MAXM);
        int neighbour = -1;
        while((neighbour = nextelement(gv, MAXM, neighbour)) >= 0) {
            fprintf(stderr, " %d", neighbour);
        }
        fprintf(stderr, "\n");
    }
}

/******************************************************************************/

static void
print_help(char * argv0) {
    fprintf(stderr, "TriangleRamsey version %s\n", VERSION);
    fprintf(stderr, "Usage: %s <number_of_vertices> [options]\n", argv0);
    fprintf(stderr, "At least the number of vertices must be specified.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Valid options are:\n");
    fprintf(stderr, "  outputlevel <x>: Output all graphs with at least <x> vertices (default value is <number_of_vertices>).\n");
    fprintf(stderr, "  n: No graphs will be output (i.e. the graphs are only counted).\n");
    fprintf(stderr, "  file <filename>: Starts the generation process from the graphs in <filename>, else the generation is started from P3.\n");
    fprintf(stderr, "  ramsey <filename>: Generates ramseygraphs for the graph in <filename>\n");
    fprintf(stderr, "  write_all_ramseygraphs: Can only be used in combination with option 'ramsey'. Generates all mtf ramseygraphs for the testgraph instead of stopping when one ramseygraph was found.\n");
    fprintf(stderr, "  When this option is used, the ramseygraphs are written to a file instead of to stdout (unless if option 'n' is used).\n");
    fprintf(stderr, "  mod <rest> <modulo>: Splits the generation in <modulo> (more or less equally big) parts. Here part <rest> will be executed.\n");
}

/******************************************************************************/

int main(int argc, char *argv[]) {
    int i;

    /* Checks to test if the WORDSIZE and sizeof setwords is valid */
    if(WORDSIZE != 32 && WORDSIZE != 64) {
        fprintf(stderr, "Error: invalid value for wordsize: %d\n", WORDSIZE);
        fprintf(stderr, "Valid values are 32 and 64\n");
        exit(1);
    }

    if(MAXN1 != WORDSIZE) {
        fprintf(stderr, "Error: MAXN1 is not equal to WORDSIZE: %d vs %d\n", MAXN1, WORDSIZE);
        exit(1);
    }

    if(WORDSIZE == 32) {
        //fprintf(stderr, "32 bit mode\n"); //Temp not to limit spam
        if(sizeof(unsigned int) < 4) {
            fprintf(stderr, "Error: unsigned ints should be at least 32 bit -- sorry.\n");
            exit(1);
        }
        if(sizeof(setword) < 4) {
            fprintf(stderr, "Error: this version relies on 32 bit setwords -- sorry.\n");
            exit(1);
        }
    } else {
        //fprintf(stderr, "64 bit mode\n");
        if(sizeof(setword) != 8) {
            fprintf(stderr, "Error: this version relies on 64 bit setwords -- sorry.\n");
            exit(1);
        }
        if(sizeof(unsigned long long int) != 8) {
            fprintf(stderr, "Error: this version relies on 64 bit unsigned long long ints -- sorry.\n");
            exit(1);
        }
    }

    if(argc == 2 && argv[1][0] == '-' && argv[1][1] == 'h') {
        print_help(argv[0]);
        exit(1);
    }

    if(argc < 2) {
        fprintf(stderr, "Error: invalid number of arguments. At least the number of vertices must be specified.\n");
        fprintf(stderr, "Usage: %s <number_of_vertices> [options]\n", argv[0]);
        fprintf(stderr, "Execute '%s -h' for more extensive help.\n", argv[0]);
        exit(1);
    } else {
        if(sscanf(argv[1], "%d", &maxnv)) {
            if(maxnv > MAXN1) {
                if(MAXN1 == 32) {
                    fprintf(stderr, "Error: maximum number of vertices is 32 in this version.\n");
                    fprintf(stderr, "Compile with option -DWORDSIZE=64 in order to be able to generate graphs with a higher order.\n");
                    fprintf(stderr, "Or alternatively use the command 'make 64bit'\n");
                    exit(1);
                } else {
                    fprintf(stderr, "Error: number of vertices is too big (limit is %d).\n", MAXN1);
                    exit(1);
                }
            }
            if(MAXN1 == 64 && maxnv <= 32) {
                fprintf(stderr, "Info: it is recommended to use the 32 bit version for generating graphs with order <= 32\n");
            }

        }
        if(maxnv < 3) {
            fprintf(stderr, "Error: number of vertices should be an integer > 2.\n");
            exit(1);
        }
    }
    
    //Default outputlevel is maxnv
    outputlevel = maxnv;

    for(i = 2; i < argc; i++) {
        switch(argv[i][0]) {
            case 'n':
            {
                noout = 1;
                break;
            }            
            case 'o':
            {
                if(strcmp(argv[i], "outputlevel") == 0) {
                    if(i + 1 < argc) {
                        i++;
                        outputlevel = atoi(argv[i]);
                        if(outputlevel < 0 || outputlevel > maxnv) {
                            fprintf(stderr, "Error: %d is an impossible outputlevel\n", outputlevel);
                            exit(1);
                        }
                    } else {
                        fprintf(stderr, "Error: option 'outputlevel': missing outputlevel\n");
                        exit(1);
                    }
                } else {
                    fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                    fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                    exit(1);
                }
                break;
            }
            case 'm':
            {
                if(strcmp(argv[i], "mod") == 0) {
                    if(i + 2 < argc) {
                        i++;
                        rest = atoi(argv[i]);
                        i++;
                        modulo = atoi(argv[i]);
                        if(rest >= modulo) {
                            fprintf(stderr, "Error: rest (%d) must be smaller than modulo (%d).\n", rest, modulo);
                            exit(1);
                        }
                    } else {
                        fprintf(stderr, "Error: option 'mod': missing rest or modulo\n");
                        exit(1);
                    }
                } else {
                    fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                    fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                    exit(1);
                }
                break;
            }
            case 'f':
            {
                if(strcmp(argv[i], "file") == 0) {
                    if(i + 1 < argc) {
                        inputgraphs_filename = argv[++i];
                    } else {
                        fprintf(stderr, "Error: option 'file': missing filename of inputgraphs\n");
                        exit(1);
                    }
                } else {
                    fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                    fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                    exit(1);
                }
                break;
            }            
            case 'r':
            {
                if(strcmp(argv[i], "ramsey") == 0) {
                    if(i + 1 < argc) {
                        testgraph_filename = argv[++i];
                        ramsey = 1;
                    } else {
                        fprintf(stderr, "Error: option 'ramsey': missing filename of testgraph\n");
                        exit(1);
                    }
                } else {
                    fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                    fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                    exit(1);
                }
                break;
            }
            case 'w':
            {
                if(strcmp(argv[i], "write_all_ramseygraphs") == 0) {
                    write_all_ramseygraphs = 1;
                } else {
                    fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                    fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                    exit(1);
                }
                break;
            }            
            default:
            {
                fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                exit(1);
            }
        }
    }
    
    if(noout) {
        if(outputlevel < maxnv) {
            fprintf(stderr, "Warning: option 'outputlevel' was ignored because of option 'n'.\n");
            outputlevel = maxnv;
        }
    }
    
    if(write_all_ramseygraphs && !ramsey) {
        fprintf(stderr, "Error: option 'write_all_ramseygraphs' can only be used in combination with option 'ramsey' \n");
        exit(1);
    }
    
    if(ramsey) {
        read_testgraph();
        
        if(MAX_NUM_GOOD_SETS_V <= MIN_NUM_PREV_SETS) {
            fprintf(stderr, "Error: MAX_NUM_GOOD_SETS_V should be > MIN_NUM_PREV_SETS (%d vs %d)\n",
                    MAX_NUM_GOOD_SETS_V, MIN_NUM_PREV_SETS);
            exit(1);
        }
    }

    nauty_check(WORDSIZE, MAXM, MAXN, NAUTYVERSIONID);
    nauty_check(WORDSIZE, MAXM1, MAXN1, NAUTYVERSIONID);

    //For the timing
    struct tms TMS;
    unsigned int oldtime = 0;

    for(i = 0; i < argc; i++)
        fprintf(stderr, "%s ", argv[i]);
    fprintf(stderr, "\n");
    
    initialize_splitting();

    init_construction();


    times(&TMS);
    unsigned int savetime = oldtime + (unsigned int) TMS.tms_utime;
    
    if(number_of_inputgraphs_read > 0)
        fprintf(stderr, "Read %d input graphs\n", number_of_inputgraphs_read);

    for(i = 3; i <= maxnv; i++)
        fprintf(stderr, "Number of graphs generated with %d vertices: %llu\n", i, number_of_graphs_generated[i]);
    
/*
    fprintf(stderr, "Number of graphs generated with %d vertices by double vertex: %llu\n", maxnv, number_of_graphs_generated_double_vertex[maxnv]);
    fprintf(stderr, "Number of graphs generated with %d vertices by rem vertex: %llu\n", maxnv, number_of_graphs_generated_rem_vertex[maxnv]);
    fprintf(stderr, "Number of graphs generated with %d vertices by other vertex: %llu\n", maxnv, number_of_graphs_generated_no_rem_vertex[maxnv]);
*/

    if(ramsey) {
        for(i = 3; i <= maxnv; i++)
            fprintf(stderr, "Number of Ramsey graphs generated with %d vertices: %llu\n",
                i, number_of_ramsey_graphs_generated[i]);
            //fprintf(stderr, "Number of Ramsey graphs generated with %d vertices: %llu (perc. no subgraph: %.2f%%)\n",
              //  i, number_of_ramsey_graphs_generated[i], (double) 100 * number_of_ramsey_graphs_generated[i] / number_of_graphs_generated[i]);        
        
        fprintf(stderr, "Number of testgraphs with Ramsey number larger than %d: %d\n", maxnv, (number_of_ramsey_graphs_generated[maxnv] > 0));        

        //WARNING: this is added for tr_emu2: write 1 to stdout if no ramsey graph was found.
        if(number_of_ramsey_graphs_generated[maxnv] == 0) {
            if(number_of_graphs_generated[maxnv] > 0)
                fprintf(stderr, "Info: generated graphs on last level but it were no Ramsey graphs\n");
            int value = 1;
            if(fwrite(&value, sizeof(value), 1, stdout) != 1) {
                fprintf(stderr, "Error: fwrite failed\n");
                exit(1);
            }
            fprintf(stderr, "Info: no Ramsey graph with %d vertices found, so wrote 1 to stdout\n", maxnv);
        }
    }
    
    fprintf(stderr, "Number of graphs output: %llu\n", number_of_graphs_output);
    
    fprintf(stderr, "CPU time: %.1f seconds.\n", (double) savetime / time_factor);
    

    return(EXIT_SUCCESS);

}

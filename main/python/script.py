import subprocess
import cProfile
import pstats
import io
from networkx import Graph, read_graph6, to_graph6_bytes

INPUT_FILE = 'input.g6'
OUTPUT_FILE = 'output.g6'
NAUTY = False

veritces = input('Enter number of vertices: ')
output_level = input('Enter output level: ')
nauty = input('Use nauty or not (y/n): ')

if nauty == 'y':
    NAUTY = True
elif nauty == 'n':
    NAUTY = False

result = subprocess.run(['./triangleramsey', veritces, 'outputlevel', output_level], stdout=subprocess.PIPE)
print('Done generating graphs.')
l = list(result.stdout)

def choose_sublists(lst, k):
    if len(lst) == k:
        return [lst] 
    if  k == 0:
        return [[]]
    if k == 1:
        return [[i] for i in lst]
    sub_lst1=choose_sublists(lst[1:], k-1)
    for i in sub_lst1:
        i.append(lst[0])
    sub_lst2=choose_sublists(lst[1:], k)
    final_lst=[]
    final_lst.extend(sub_lst1)
    final_lst.extend(sub_lst2)
    return final_lst

def check_for_ind(graph, ind_nodes, new_nodes):

    for i in ind_nodes:
        for n in new_nodes:
            if graph.has_edge(i, n):
                return False

    for n in new_nodes:
        for m in new_nodes:
            if graph.has_edge(n, m):
                return False

    return True

def extract_elements(lst):
    return list(map(lambda el:[el], lst))

def has_ind_set_k(graph, k, nodes_incl_set, prev_nodes_list):

    if k == 1:
        return False

    if len(nodes_incl_set) == 0:
        sublists = choose_sublists(list(graph.nodes), k)
        for sub in sublists:
            if check_for_ind(graph, sub, sub):
                return True
        return False
    else:
        if not check_for_ind(graph, nodes_incl_set, nodes_incl_set):
            return False

        other_nodes = set(graph.nodes) - nodes_incl_set
        other_k = k - len(nodes_incl_set)

        if other_k == 1:
            sublists = extract_elements(other_nodes)
        elif other_k == 0:
            sublists = []
        else:
            sublists = choose_sublists(list(other_nodes), other_k)

        if sublists:
            for sub in sublists:

                if sub in prev_nodes_list:
                    if check_for_ind(graph, nodes_incl_set, sub):
                        return True
                else:
                    if check_for_ind(graph, nodes_incl_set, sub):
                        if len(prev_nodes_list) > 100:
                            prev_nodes_list.pop(0)
                        prev_nodes_list.append(sub)
                        return True

            return False
        else:
            return False

def get_all_one_edge_removed(graph, size):

    result_graphs = []
    result_nodes = []
    for i in range(size):
        for j in range(i + 1, size):
            if (graph.has_edge(i, j)):
                graph.remove_edge(i, j)
                result_graphs.append(graph.copy())
                result_nodes.append({i, j})
                graph.add_edge(i, j)

    return result_graphs, result_nodes

def nauty_isomorphs():
    subprocess.run(['./shortg', '-g', INPUT_FILE, OUTPUT_FILE], stderr=subprocess.DEVNULL)

def print_graph_matrix(graph, size):
    for i in range(size):
        for j in range(size):
            if graph.has_edge(i, j):
                print(1, end=' ')
            else:
                print(0, end=' ')
        print()

# Recursively remove an edge and check if the graph has n independent set of k
def find_e(graph, k, size, edges, results, mtf_edges, nodes_incl, prev_nodes_list):

    if has_ind_set_k(graph, k, nodes_incl, prev_nodes_list):
        if (edges != mtf_edges):
            results.add(edges + 1)
        return
    else:
        if edges > 0:
            graphs, adj_nodes = get_all_one_edge_removed(graph, size)

            if NAUTY:
                # write graphs to file in graph6 format
                with open(INPUT_FILE, 'wb') as f:
                    for g in graphs:
                        f.write(to_graph6_bytes(g, header=False))
                
                # call shortg with the created file
                # here to exclude the isomorphic graphs
                nauty_isomorphs()

                # read graphs from output file
                graphs = read_graph6(OUTPUT_FILE)

                if not isinstance(graphs, list):
                    graphs = [graphs]

                for g in graphs:
                    if has_ind_set_k(g, k, set(), []):
                        results.add(edges)
                    else:
                        return find_e(g, k, size, edges - 1, results, mtf_edges, set(), [])

            else:

                for g, n in zip(graphs, adj_nodes):
                    if has_ind_set_k(g, k, n, prev_nodes_list):
                        results.add(edges)
                    else:
                        return find_e(g, k, size, edges - 1, results, mtf_edges, n, prev_nodes_list)

        else:
            return

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

def main():

    next_graph = True
    curr_vertex = 0
    vertex_count = 0
    graph = Graph()

    # Prepare the result array
    size = 20
    results = []
    for i in range(size):
        results.append([100 for i in range(size)])

    total_len = len(l)

    # Initial call to print 0% progress 
    printProgressBar(0, total_len, prefix = 'Progress:', suffix = 'Complete', length = 50)

    # for every number in the mtf method output
    for i, x in enumerate(l):

        # Update Progress Bar
        printProgressBar(i + 1, total_len, prefix = 'Progress:', suffix = 'Complete', length = 50)

        if next_graph:
            vertex_count = int(x)
            graph = Graph()
            next_graph = False
            continue
        if x == 0:
            curr_vertex += 1
            if curr_vertex == vertex_count-1:
                curr_vertex = 0
                next_graph = True
                
                n = graph.number_of_nodes()

                for k in range(3, int(veritces) + 1):

                    if k == n:
                        results[n - 3][k - 3] = 1
                    elif k > n:
                        results[n - 3][k - 3] = 0
                    else:

                        temp_res = set()
                        edges = graph.size()

                        find_e(graph, k, n, edges, temp_res, edges, set(), [])

                        temp_res.add(results[n - 3][k - 3])
                        results[n - 3][k - 3] = min(temp_res)

        # Construct a graph by adding edges
        else:
            graph.add_edge(curr_vertex, int(x)-1)

    file_name = f'results/results_{veritces}.txt'
    # file_name = 'results/test.txt'

    with open(file_name, 'w+') as f:
        for i in range(int(veritces) - 2):
            for j in range(int(veritces) - 2):
                if (results[i][j] == 100):
                    f.write(' {0:2}'.format('-1'))
                else:
                    f.write('{0:3}'.format(results[i][j]))
            f.write('\n')


pr = cProfile.Profile()
pr.enable()

main()

pr.disable()
s = io.StringIO()
ps = pstats.Stats(pr, stream=s).sort_stats('cumtime')
ps.print_stats()

nauty_or_not = ''

if NAUTY:
    nauty_or_not = 'with_nauty'
else:
    nauty_or_not = 'without_nauty'

file_name = f'profiling_results/prof_output_{veritces}_{nauty_or_not}.txt'
# file_name = 'profiling_results/test.txt'

with open(file_name, 'w+') as f:
    f.write(s.getvalue())

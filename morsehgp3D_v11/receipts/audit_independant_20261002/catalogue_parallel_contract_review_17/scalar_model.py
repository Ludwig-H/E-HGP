"""Independent bounded arithmetic/ownership model; never imports or executes product."""
from fractions import Fraction
from itertools import combinations, product
import json

checks = 0

def need(value, message):
    global checks
    checks += 1
    if not value:
        raise ValueError(message)


def dominates(x, y, lo, hi):
    # An affine distance difference is positive everywhere on a closed box
    # iff positive at every vertex; independent of catalogue's local expression.
    return all(sum((x[a]-c[a])**2 for a in range(3)) >
               sum((y[a]-c[a])**2 for a in range(3))
               for c in product(*zip(lo, hi)))


# Closed-box membership shortcut, including contact on hi (owner is still half-open).
lo, hi = (0, 0, 0), (4, 4, 4)
inside = list(product((0, 1, 4), repeat=3))
witnesses = list(product((-1, 0, 2, 4, 5), repeat=3))
for x in inside:
    for y in witnesses:
        need(not dominates(x, y, lo, hi), 'strict dominator for a point in closed box')
need(dominates((5, 0, 0), (4, 0, 0), lo, hi), 'outside case must remain testable')
need(not dominates((4, 1, 1), (4, 1, 0), lo, hi), 'hi contact shortcut')


# Exact 1D K1 model using ALL witnesses, not the production reservoir.
# A Ready owns its logical subset, paid storage is parent's cardinality.
def ready(parent, box, depth):
    l, h = box
    sites = tuple(x for x in parent if not any(
        (x-l)**2 > (y-l)**2 and (x-h)**2 > (y-h)**2 for y in parent))
    if not sites:
        return None
    adjusted = max(l, min(sites)), min(h, max(sites)+1)
    if adjusted[0] >= adjusted[1]:
        return None
    return (sites, adjusted, depth, len(parent))


def children(node, leaf):
    sites, (l, h), d, capacity = node
    if len(sites) <= leaf or h-l <= 1:
        return []
    middle = l+(h-l)//2
    return [(sites, (l, middle), d+1), (sites, (middle, h), d+1)]


def full(parent, box, depth, leaf):
    node = ready(parent, box, depth)
    if node is None:
        return 1, []
    split = children(node, leaf)
    if not split:
        return 1, [node]
    count, leaves = 1, []
    for args in split:
        n, part = full(*args, leaf)
        count += n
        leaves.extend(part)
    return count, leaves


def frontier(parent, box, depth, cut, leaf):
    node = ready(parent, box, depth)
    if node is None:
        return 1, []
    split = children(node, leaf)
    if depth == cut or not split:
        return 1, [node]
    count, tasks = 1, []
    for args in split:
        n, part = frontier(*args, cut, leaf)
        count += n
        tasks.extend(part)
    return count, tasks


for n in (5, 9, 17, 65):
    cloud = tuple(range(n))
    base_nodes, base_leaves = full(cloud, (0, n), 0, 4)
    for cut in range(9):
        prelude_nodes, tasks = frontier(cloud, (0, n), 0, cut, 4)
        actual_nodes, leaves = prelude_nodes, []
        for task in tasks:
            split = children(task, 4)
            if not split:
                leaves.append(task)
            for args in split:
                nodes, part = full(*args, 4)
                actual_nodes += nodes
                leaves.extend(part)
        need(actual_nodes == base_nodes, 'prepared node counted twice or lost')
        need(leaves == base_leaves, 'cut changes suffix DFS')
        for i in range(n-1):
            c = Fraction(2*i+1, 2)
            owners = [t for t in tasks if t[1][0] <= c < t[1][1]]
            need(len(owners) == 1, 'critical midpoint not uniquely owned')
            need(i in owners[0][0] and i+1 in owners[0][0], 'support lost from owner list')
    if n == 5:
        _, tasks = frontier(cloud, (0, n), 0, 1, 4)
        need([list(t[0]) for t in tasks] == [[0,1,2],[2,3,4]], 'overlap witness')
        need(sum(len(t[0]) for t in tasks) == 6, 'logical lengths are not a site partition')
        need(sum(t[3] for t in tasks) == 10, 'paid capacities are not logical lengths')


# Independent scalar admission bounds: no virtual/real large arrays.
limit = 8*2**30
need(limit//1064 == 8073246, 'U0 threshold')
need((limit-8)//1092 == 7866240, 'unit Cloud same-budget threshold')
for n in (8073246, 7866240):
    if n == 8073246:
        need(1064*n <= limit < 1064*(n+1), 'first boundary')
    else:
        need(1092*n+8 <= limit < 1092*(n+1)+8, 'Cloud boundary')
capacity = [{'n':n, 'frontier_pre_admission_bytes':1064*n,
             'unit_cloud_retained_bytes':28*n+8,
             'same_budget_first_admission_bytes':1092*n+8,
             'frontier_Go':1064*n/1e9, 'frontier_Gio':1064*n/2**30}
            for n in (39885, 30_000_000, 50_000_000)]

# The top W suffix bounds cover every possible simultaneously active W-task set.
for values in ((0, 3, 2, 7), (2, 2, 2, 1, 0), (0, 0, 0)):
    for w in range(1, len(values)+1):
        bound = sum(sorted(values, reverse=True)[:w])
        for subset in combinations(values, w):
            need(sum(subset) <= bound, 'suffix coexistence underestimated')

# Heterogeneous populations; global sorting must follow rebased local offsets.
tasks = [([10,11,12,13,14], [(Fraction(2), (1,2), 0,2), (Fraction(1), (3,4),2,3)]),
         ([20,21,22,23,24,25,26], [(Fraction(1), (5,6),0,4), (Fraction(3), (7,8),4,3)]),
         ([], []), ([30,31], [(Fraction(1), (9,10),0,2)])]
population, emissions, expected = [], [], {}
for pop, records in tasks:
    start = len(population)
    population.extend(pop)
    for level, support, offset, length in records:
        expected[support] = pop[offset:offset+length]
        emissions.append((level, support, start+offset, length))
for level, support, offset, length in sorted(emissions):
    need(population[offset:offset+length] == expected[support], 'rebased global population mismatch')
# An omitted rebase is killed by the second task, even without huge allocation.
need(population[0:4] != expected[(5,6)], 'rebase mutant survives')
# Offsets are u64 scalars, independent from u32 ball ordinals.
need((2**32+17)+23 == 2**32+40, 'wide offset sum')
need(((2**32+17)+23) % 2**32 != 2**32+40, 'u32 offset mutant survives')

report = {'status':'PASS','checks':checks,'native':False,'build':False,'gcp':False,
          'scope':'Independent bounded scalar model, not execution/qualification of WIP catalogue',
          'capacity':capacity,'limit8GiB_bytes':limit,'nmax_U0':8073246,'nmax_unit_cloud_same_budget':7866240,
          'frontier_depth_bytes_per_site':{str(d):4*((1<<d)+d+2) for d in (0,1,4,8)},
          'root_filter_tests_per_pass_39885_K5':39885*15,
          'root_filter_tests_two_passes_39885_K5':2*39885*15,
          'root_shortcut_proof':'x in closed Q implies c=x is a witness against every strict uniform dominator',
          'false_claims_excluded':['observed peak equals admission','massive allocation or native benchmark','FULL or GPU qualification']}
print(json.dumps(report,sort_keys=True))

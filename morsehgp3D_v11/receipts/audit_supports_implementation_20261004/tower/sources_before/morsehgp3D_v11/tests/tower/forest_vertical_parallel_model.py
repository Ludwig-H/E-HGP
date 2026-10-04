"""Modèle indépendant des graines anticipées : Gamma_k/Fraction et marches de parents.

La graine modèle est une naissance du composant Gamma_(k-1) à beta(partie),
pas le terminal choisi par la descente native. Le juge exige seulement sa bonne
image fermée et la même affectation ordinale aux lanes pour toutes les fenêtres.
"""
import copy
from fractions import Fraction
import forest_oracle as oracle


def parents(nodes):
    result = [None] * len(nodes)
    for i, node in enumerate(nodes):
        for child in node.children:
            oracle.require(result[child] is None, 'parent unique')
            result[child] = i
    return result


def climb(nodes, parent, seed, level, closed=True):
    oracle.require(0 <= seed < len(nodes) and nodes[seed].level <= level, 'graine disponible')
    while parent[seed] is not None:
        date = nodes[parent[seed]].level
        if not (date <= level if closed else date < level):
            break
        seed = parent[seed]
    return seed


def setup(records, k):
    sites, _, orders = oracle.truth(records, k)
    ref = oracle.reference(sites)
    low, high = orders[-2:]
    entries = []
    for node in high.nodes:
        if node.children:
            break
        population = [i for i, point in enumerate(sites)
                      if sum((Fraction(x)-c)**2 for x,c in zip(point,node.center)) <= node.level]
        part = tuple(population[:k-1])
        oracle.require(len(part) == k-1, 'partie verticale entière')
        initial = ref.beta(part)
        seed = ref.node_at(k-1,part,initial)
        while low.nodes[seed].children:
            seed = low.nodes[seed].children[0]
        entries.append((seed,initial,node.level))
    return low,high,entries


def staged(low, high, entries, q, lanes, workers, closed=True, omit_last=False):
    oracle.require(q > 0 and lanes > 0 and workers > 0, 'paramètres positifs')
    seeds = [None]*len(high.nodes)
    lane_order = [[] for _ in range(lanes)]
    # Inverser la visite des jobs simule une autre attribution physique ; aucune dépendance entre lanes.
    for begin in range(0,len(entries),q):
        end = min(begin+q,len(entries))
        slots = list(range(min(lanes,end-begin)))
        if workers > 1:
            slots.reverse()
        assigned = [(begin+slot)%lanes for slot in slots]
        oracle.require(len(set(assigned)) == len(assigned), 'lanes injectives après découpage')
        for slot in slots:
            lane = (begin+slot)%lanes
            for i in range(begin+slot,end,lanes):
                seed,initial,date = entries[i]
                oracle.require(initial <= date, 'date initiale, pas seulement date terminale')
                oracle.require(not low.nodes[seed].children and low.nodes[seed].level <= initial, 'naissance basse')
                oracle.require(seeds[i] is None, 'case de naissance écrite une fois')
                seeds[i] = seed
                lane_order[lane].append(i)
    parent = parents(low.nodes)
    checks = 0
    for i in sorted(range(len(high.nodes)), key=lambda j:(high.nodes[j].level,j)):
        node = high.nodes[i]
        if not node.children:
            seeds[i] = climb(low.nodes,parent,seeds[i],node.level,closed)
        else:
            children = node.children[:-1] if omit_last else node.children
            images = [climb(low.nodes,parent,seeds[c],node.level,closed) for c in children]
            oracle.require(images and len(set(images)) == 1, 'toutes images des enfants')
            checks += len(images)
            seeds[i] = images[0]
    oracle.equal(seeds,list(high.lower))
    oracle.equal(checks,sum(len(node.children) for node in high.nodes))
    oracle.equal(lane_order,[list(range(lane,len(entries),lanes)) for lane in range(lanes)])
    return tuple(seeds),lane_order,checks


def run():
    fixtures = [((0,0,0),(2,0,0),(4,0,0)), ((0,0,0),(4,0,0),(0,4,0),(4,4,0)),
                ((0,0,0),(2,2,0),(2,0,2),(0,2,2)), tuple((i,0,0) for i in (0,1,2,6,9))]
    cases = checks = equal_dates = strict_dates = corruptions = 0
    for points in fixtures:
        records = oracle.data.fixtures.records(points)
        for k in range(2,len(points)+1):
            low,high,entries = setup(records,k)
            equal_dates += sum(initial == date for _,initial,date in entries)
            strict_dates += sum(initial < date for _,initial,date in entries)
            for q in (1,2,4096):
                for lanes in (1,4,48):
                    for workers in (1,4,48):
                        result,assignment,paid = staged(low,high,entries,q,lanes,workers)
                        cases += 1; checks += len(result)+sum(map(len,assignment))+paid+3
            wrong = copy.deepcopy(entries)
            seed,_,date = wrong[0]; wrong[0] = (seed,date+1,date)
            for bad_entries,closed,omitted in ((wrong,True,False),(entries,True,True)):
                if omitted and not any(n.children for n in high.nodes):
                    continue
                try:
                    staged(low,high,bad_entries,2,4,4,closed,omitted)
                except ValueError:
                    corruptions += 1
                else:
                    raise ValueError('corruption de date ou de travail acceptée')
    low,high,entries = setup(oracle.data.fixtures.records(fixtures[0]),2)
    try:
        staged(low,high,entries,1,4,4,closed=False)
    except ValueError:
        corruptions += 1
    else:
        raise ValueError('coupe ouverte acceptée à égalité')
    oracle.require(equal_dates > 0 and strict_dates > 0 and corruptions > 10, 'témoins non vacants')
    print(f'vertical_parallel_model_verdict conforme cases{cases} checks{checks} corruptions{corruptions} '
          f'equal_dates{equal_dates} strict_dates{strict_dates} native0')


if __name__ == '__main__':
    run()

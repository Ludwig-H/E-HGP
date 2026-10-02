"""Modele autonome d'une frontiere adaptative proposee, sans appel natif ni arbre geometrique produit.

Les donnees sont des arbres binaires immuables deja determines par un generateur abstrait. Chaque liste fille
est une sous-liste du parent ; deux listes peuvent se recouvrir. La population interieure represente les sites
dans la boite demi-ouverte : elle ne decide JAMAIS qu'un sous-arbre est vide. Le modele ne prouve pas G1/G2,
ne predit aucun temps et ne devient pas une qualification du futur raccord C++.
Le compte memoire porte uniquement sur les listes ; workspaces numeriques, sorties et metadonnees devront etre
ajoutes par le pilote. Le repli conserve la recherche, sans promettre que son admission ulterieure reussira.

Politique fixee : parmi les ReadyNode divisibles, priorite (-population interieure, -taille de liste, chemin).
La capacite borne les feuilles du plan, fantomes vides compris, independamment de W. Une ronde remplace au plus
capacite-feuilles parents par leurs deux enfants ; seuls les Buffers vides sont rendus apres join. Capacite pleine
=> conserver les suffixes pour DFS exact. Ainsi I splits impliquent exactement I+1 feuilles et 2I+1 noeuds.
Si la coexistence de la ronde ne peut etre admise, meme repli DFS ; aucune branche de recherche n'est supprimee.
La coupe finale est une antichaine de chemins, y compris les terminaux vides. Le rejeu conserve les rondes
du plan gele, sans relancer le choix de charge ; meme les branches entierement eteintes sont representees.
"""
from dataclasses import dataclass, replace
from itertools import combinations
import copy
import json


CHECKS = 0


def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


@dataclass(frozen=True)
class Node:
    path: str
    sites: tuple
    inside: tuple
    capacity: int
    children: tuple = ()
    payload: tuple = ()


def build(shape, sites, inside=None, path='', capacity=None, concentrated=False):
    """Fixture de protocole ; payloads distincts par feuille, jamais boules pretendues geometriques."""
    inside = sites if inside is None else inside
    capacity = len(sites) if capacity is None else capacity
    if shape is None:
        return Node(path, (), (), capacity)
    if type(shape) is int:
        return Node(path, sites, inside, capacity, payload=tuple((path, i) for i in range(shape)))
    split = 0 if concentrated and len(path) < 3 else len(inside) // 2
    if shape[0] is None:
        split = 0
    if shape[1] is None:
        split = len(inside)
    parts = (inside[:split], inside[split:])
    children = []
    for bit, (child, part) in enumerate(zip(shape, parts)):
        # Le halo commun garde des candidats hors de la boite, y compris quand sa population est nulle.
        candidates = tuple(sorted(set(part + sites[:2] + sites[-2:]))) if child is not None else ()
        children.append(build(child, candidates, part, path + str(bit), len(sites), concentrated))
    return Node(path, sites, inside, capacity, tuple(children))


def balanced(depth):
    return 1 if depth == 0 else (balanced(depth - 1), balanced(depth - 1))


def comb(depth):
    return 3 if depth == 0 else (1, comb(depth - 1))


def inventory(root):
    nodes = {}

    def walk(node):
        need(node.path not in nodes, 'chemin repete')
        nodes[node.path] = node
        need(node.sites == tuple(sorted(set(node.sites))) and set(node.inside) <= set(node.sites), 'sites/inside')
        need(node.capacity >= len(node.sites), 'capacite logique confondue avec physique')
        need(not node.children or len(node.children) == 2 and not node.payload and node.sites, 'noeud binaire')
        if node.children:
            left, right = node.children
            need(set(left.inside).isdisjoint(right.inside) and
                 set(left.inside + right.inside) == set(node.inside), 'population demi-ouverte partitionnee')
        for bit, child in enumerate(node.children):
            need(child.path == node.path + str(bit) and set(child.sites) <= set(node.sites) and
                 child.capacity == len(node.sites), 'fille preparee depuis la liste parente')
            walk(child)
    walk(root)
    return nodes


def reference(root):
    """Oracle DFS complet, sans choix de frontiere ni charge."""
    visited, leaves, output = [], [], []

    def walk(node):
        visited.append(node.path)
        if not node.sites:
            return
        if not node.children:
            leaves.append(node.path)
            output.extend(node.payload)
        else:
            for child in node.children:
                walk(child)
    walk(root)
    return tuple(visited), tuple(leaves), tuple(output)


def priority(node):
    return -len(node.inside), -len(node.sites), node.path


def prepare(root, capacity, memory_limit=2**64-1, reverse_completion=False):
    need(type(capacity) is int and capacity >= 1, 'capacite positive')
    nodes = inventory(root)
    root_bytes = 4 * root.capacity
    need(2 * root_bytes <= memory_limit, 'racine et liste entree simultanees')
    active = [root]
    prepared, rounds = [root.path], []
    used, peak = root_bytes if root.sites else 0, 2 * root_bytes
    stop = 'terminal'
    while active:
        eligible = sorted((n for n in active if n.children), key=priority)
        room = capacity - len(active)
        if not eligible:
            break
        if room == 0:
            stop = 'capacity_dfs'
            break
        selected = eligible[:room]
        extra = 8 * sum(len(n.sites) for n in selected)
        if used + extra > memory_limit:
            stop = 'memory_dfs'
            break
        record = dict(before=tuple(n.path for n in active), selected=tuple(n.path for n in selected),
                      used_before=used, admitted=extra)
        # Toutes les listes filles (y compris vides) coexistent avec leurs parents jusqu'au join.
        for parent in (reversed(selected) if reverse_completion else selected):
            for child in parent.children:
                prepared.append(child.path)
                used += 4 * child.capacity
                peak = max(peak, used)
        record['round_peak'] = used
        replaced = {n.path for n in selected}
        following = [n for n in active if n.path not in replaced]
        for parent in selected:
            used -= 4 * parent.capacity
            for child in parent.children:
                following.append(child)
                if not child.sites:
                    used -= 4 * child.capacity
        active = sorted(following, key=lambda n: n.path)
        record.update(after=tuple(n.path for n in active), used_after=used)
        rounds.append(record)
    return dict(jobs=tuple(n.path for n in active if n.sites), leaves=tuple(n.path for n in active),
                prepared=tuple(sorted(prepared)), rounds=rounds,
                owned=used, peak=peak, stop=stop, capacity=capacity, memory_limit=memory_limit,
                descriptors={n.path: n for n in active if n.sites}, nodes=nodes)


def validate(root, plan):
    nodes = inventory(root)
    jobs = plan['jobs']
    leaves = plan['leaves']
    need(leaves == tuple(sorted(set(leaves))) and len(leaves) <= plan['capacity'], 'feuilles et fantomes bornes')
    need(jobs == tuple(p for p in leaves if nodes[p].sites), 'ordinaux uniques croissants')
    need(len(plan['prepared']) == 2 * len(leaves) - 1, 'arbre binaire complet du plan')
    need(all(path in nodes and nodes[path].sites for path in jobs), 'jobs reels non vides')
    for a, b in combinations(leaves, 2):
        need(not a.startswith(b) and not b.startswith(a), 'frontiere antichaine')
    seen, reached = [], []

    def replay(node):
        seen.append(node.path)
        if node.path in jobs:
            need(plan['descriptors'][node.path] == node, 'descripteur exact de la coupe')
            reached.append(node.path)
        elif node.path in leaves:
            need(not node.sites, 'terminal vide attend un fantome')
        else:
            need(bool(node.children) and node.sites, 'feuille ou fantome omis par la coupe')
            for child in node.children:
                replay(child)
    replay(root)
    need(tuple(reached) == jobs and tuple(sorted(seen)) == plan['prepared'], 'rejeu du seul preambule')
    active = ('',)
    used, peak = 4 * root.capacity if root.sites else 0, 8 * root.capacity
    for record in plan['rounds']:
        need(record['before'] == active and record['used_before'] == used, 'debut de ronde')
        eligible = [nodes[p] for p in active if nodes[p].children]
        wanted = []
        while eligible and len(wanted) < plan['capacity'] - len(active):
            best = min(eligible, key=priority)
            wanted.append(best.path)
            eligible.remove(best)
        need(record['selected'] == tuple(wanted) and wanted, 'choix deterministe')
        child_bytes = sum(4 * child.capacity for p in wanted for child in nodes[p].children)
        need(record['admitted'] == child_bytes == 8 * sum(len(nodes[p].sites) for p in wanted),
             'coexistence reelle parents et enfants')
        need(used + child_bytes == record['round_peak'] <= plan['memory_limit'], 'pic de ronde paye')
        peak = max(peak, record['round_peak'])
        following = set(active) - set(wanted)
        for p in wanted:
            following.update(c.path for c in nodes[p].children)
        active = tuple(sorted(following))
        used = sum(4 * nodes[p].capacity for p in active if nodes[p].sites)
        need(record['after'] == active and record['used_after'] == used, 'publication apres join')
    need(active == leaves and used == plan['owned'] and peak == plan['peak'], 'cloture du plan')
    return nodes


def execute(root, plan, reverse_fill=False, quota=None):
    """Count par job puis spans fixes : le remplissage peut arriver dans tout ordre."""
    nodes = validate(root, plan)
    prepared = set(plan['prepared'])
    suffix, counts, parts = [], [], []
    for path in plan['jobs']:
        visited, _, output = reference(nodes[path])
        suffix.extend(visited[1:])  # le ReadyNode appartient deja au preambule
        counts.append(len(output))
        parts.append(output)
    expected_nodes, _, expected = reference(root)
    need(len(set(suffix)) == len(suffix) and prepared.isdisjoint(suffix), 'aucun ancetre refait')
    need(prepared | set(suffix) == set(expected_nodes), 'couverture exacte de toute recherche')
    visits = len(prepared) + len(suffix)
    if quota is not None and visits > quota:
        return dict(status='node_budget', output=(), required_visits=visits)
    offsets = [0]
    for count in counts:
        offsets.append(offsets[-1] + count)
    output, filled = [None] * offsets[-1], [False] * offsets[-1]
    for i in (reversed(range(len(parts))) if reverse_fill else range(len(parts))):
        # Recompter le meme suffixe, sans connaitre l'ordre d'arrivee de la premiere passe.
        actual = reference(nodes[plan['jobs'][i]])[2]
        need(actual == parts[i] and len(actual) == counts[i], 'compte/remplissage identiques')
        for j, value in enumerate(actual, offsets[i]):
            need(j < offsets[i+1] and not filled[j], 'spans exacts et disjoints')
            output[j], filled[j] = value, True
    need(all(filled) and tuple(output) == expected, 'sortie DFS complete identique')
    return dict(status='ok', output=tuple(output), required_visits=visits)


def memory(root, plan, workers):
    """Rejeu en coexistence et borne des suffixes ; aucune taille logique substituee a une capacite."""
    nodes = validate(root, plan)
    jobs = set(plan['jobs'])
    owned = plan['owned']
    # Ordre de rondes fige : aucun planning ni scan de priorite au retour.
    replay_extra = max([8 * root.capacity] + [record['round_peak'] for record in plan['rounds']])
    active = ('',)
    replay_peak = 8 * root.capacity
    for record in plan['rounds']:
        current = sum(4 * nodes[p].capacity for p in active if nodes[p].sites)
        allocated = sum(4 * child.capacity for p in record['selected'] for child in nodes[p].children)
        replay_peak = max(replay_peak, current + allocated)
        active = record['after']
    need(active == plan['leaves'] and replay_peak == replay_extra, 'borne du rejeu parallele gelee')
    peak, used = owned + replay_peak, owned
    need(sum(4 * nodes[p].capacity for p in active if nodes[p].sites) == owned, 'rejeu rend ses listes finales')
    depth = max(map(len, nodes))

    def extra(node):
        return max((4 * child.capacity + extra(child) for child in node.children), default=0)
    actual, bounds = [], []
    for path in jobs:
        node = nodes[path]
        actual.append(extra(node))
        bounds.append(4 * len(node.sites) * (depth-len(path)) if node.children else 0)
        need(actual[-1] <= bounds[-1], 'borne locale du suffixe')
    require_workers = min(workers, len(jobs))
    total = sum(sorted(bounds, reverse=True)[:require_workers])
    need(sum(sorted(actual, reverse=True)[:require_workers]) <= total, 'W plus grands suffixes simultanes')
    # Les listes conservees appartiennent au plan : leur destruction rend exactement owned, pas le pic.
    used -= sum(4 * nodes[p].capacity for p in jobs)
    need(used == 0, 'destruction de tous les proprietaires')
    return dict(replay_peak=peak, suffix_bound=total, released=used,
                suffix_admitted=owned + total <= plan['memory_limit'])


def main():
    fixtures = [build(2, tuple(range(3))), build(balanced(4), tuple(range(17))),
                build(comb(9), tuple(range(13)), concentrated=True),
                build(((None, 2), (1, (None, 3))), tuple(range(7))),
                build(((2, 3), (4, 5)), tuple(range(5)), concentrated=True),
                build(((None, None), 1), tuple(range(9)), concentrated=True),
                build((None, (None, (None, (None, 2)))), tuple(range(11)))]
    cases, refusals, corruptions = 0, 0, 0
    saved = None
    for root in fixtures:
        full_nodes = len(reference(root)[0])
        for capacity in (1, 2, 3, 8, 32):
            plan = prepare(root, capacity)
            other = prepare(root, capacity, reverse_completion=True)
            need(plan == other, 'ordre de fin des jobs sans effet sur le plan')
            first = execute(root, plan, quota=full_nodes)
            second = execute(root, plan, reverse_fill=True, quota=full_nodes)
            need(first == second and first['status'] == 'ok', 'rejeu complet stable')
            refused = execute(root, plan, quota=full_nodes-1)
            need(refused['status'] == 'node_budget' and refused['output'] == (), 'quota global exclusif du succes')
            refusals += 1
            for workers in (1, 2, 4, 8):
                memory(root, plan, workers)
            if capacity == 1 and root.children:
                need(plan['stop'] == 'capacity_dfs' and first['required_visits'] == full_nodes,
                     'capacite pas quota de recherche')
            if root is fixtures[4] and capacity == 2:
                need('0' in plan['jobs'] and not plan['nodes']['0'].inside and
                     bool(reference(plan['nodes']['0'])[2]), 'population nulle conserve un suffixe productif')
            if root is fixtures[5] and capacity >= 8:
                need(plan['leaves'] == ('00', '01', '1') and plan['jobs'] == ('1',) and
                     plan['prepared'] == ('', '0', '00', '01', '1'), 'branche eteinte integralement rejouee')
            if root is fixtures[6] and capacity == 3:
                need(len(plan['leaves']) == 3 and len(plan['jobs']) == 1 and len(plan['prepared']) == 5,
                     'chaine unaire consomme aussi le plafond')
            cases += 1
            if root is fixtures[4] and capacity == 3:
                saved = root, plan
        if root.children:
            # La racine tient, mais pas ses deux enfants avec elle : optimisation abandonnee, DFS conserve.
            constrained = prepare(root, 32, memory_limit=8*root.capacity)
            need(constrained['stop'] == 'memory_dfs', 'repli avant allocation de la ronde')
            execute(root, constrained)
            need(not memory(root, constrained, 1)['suffix_admitted'],
                 'repli exact ne promet pas admission ulterieure des listes DFS')
            cases += 1
    root, plan = saved
    def ancestor_and_child(p):
        parent = next(path for path in p['jobs'] if p['nodes'][path].children)
        child = next(n.path for n in p['nodes'][parent].children if n.sites)
        p['capacity'] += 1
        p['jobs'] = tuple(sorted(p['jobs'] + (child,)))

    for change in (
            ancestor_and_child,
            lambda p: p.__setitem__('jobs', p['jobs'][:-1]),
            lambda p: p.__setitem__('jobs', tuple(reversed(p['jobs']))),
            lambda p: p.__setitem__('jobs', p['jobs'] + (p['jobs'][0],)),
            lambda p: p.__setitem__('prepared', p['prepared'][1:]),
            lambda p: p.__setitem__('owned', p['owned']-4),
            lambda p: p.__setitem__('peak', p['peak']-4),
            lambda p: p['rounds'][0].__setitem__('admitted', p['rounds'][0]['admitted']-4),
            lambda p: p['rounds'][0].__setitem__('round_peak', p['rounds'][0]['round_peak']-4),
            lambda p: p['rounds'][0].__setitem__('used_after', 0),
            lambda p: p['rounds'][0].__setitem__('selected', ()),
            lambda p: p['descriptors'].__setitem__(p['jobs'][0], replace(p['descriptors'][p['jobs'][0]], capacity=0))):
        bad = copy.deepcopy(plan)
        change(bad)
        try:
            execute(root, bad)
        except ValueError:
            corruptions += 1
        else:
            raise ValueError('corruption de plan non detectee')
    ghost_root = fixtures[5]
    ghost_plan = prepare(ghost_root, 8)
    for change in (
            lambda p: p.__setitem__('leaves', ('1',)),
            lambda p: p.__setitem__('prepared', ('', '0', '1')),
            lambda p: p['rounds'][-1].__setitem__('after', ('1',)),
            lambda p: p.__setitem__('capacity', 2)):
        bad = copy.deepcopy(ghost_plan)
        change(bad)
        try:
            execute(ghost_root, bad)
        except ValueError:
            corruptions += 1
        else:
            raise ValueError('corruption des fantomes non detectee')
    need(cases == 41 and refusals == 35 and corruptions == 16, 'non vacuite du modele')
    print(json.dumps(dict(schema='ehgp.v11.adaptive_frontier_model.v2', cases=cases, quota_refusals=refusals,
                         corruptions=corruptions, checks=CHECKS, native=0, geometric_qualification=False,
                         policy='inside_desc_count_desc_path_asc', resource_fallback='exact_suffix_dfs'), sort_keys=True))


if __name__ == '__main__':
    main()

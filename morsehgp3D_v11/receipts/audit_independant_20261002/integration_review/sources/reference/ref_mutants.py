"""Mutants de la reference : correctifs appliques a une COPIE des sources de hgp11_ref (aucun crochet dans les
sources). Chaque mutant reel doit etre tue par le juge sur ses fixtures ; chaque mutant declare equivalent (une
liberte de l'algorithme, sans effet sur l'objet) doit survivre.

Un mutant : file (fichier de hgp11_ref), old (texte present UNE SEULE fois), new, fixtures (nuages graves joues),
equivalent, why. load(name, package_dir) rend le paquet mute, importe sous un nom propre, a cote du paquet intact.

    python3 ref_mutants.py --socle-manifest    ecrit le manifeste des mutants reels au format de
                                               tests/mutants/run_mutants.py (porte mhgp11_reference_fast)
"""
import importlib
import importlib.util
import json
import os
import shutil
import sys
import tempfile


class StalePatch(Exception):
    """Le motif d'un mutant n'est plus present exactement une fois : le mutant est a reecrire."""


def _mutant(file, old, new, fixtures, why, equivalent=False, silent=False):
    """silent : la copie mutee reste coherente (aucun invariant ne la refuse) ; seule la comparaison a l'autre
    etage doit la tuer, sinon le mutant n'exerce pas ce qu'il annonce."""
    return dict(file=file, old=old, new=new, fixtures=fixtures, why=why, equivalent=equivalent, silent=silent)


B = 'constructive.py'
MUTANTS = {
    # ---- fautes de la tour (classes de l'audit L06 de la v10 : trois d'entre elles traversaient ses portes)
    'binarize': _mutant(
        B, '                    top[root] = nb + len(merges)\n                    merges.append((level, kids))\n',
        '                    acc = kids[0]\n                    for c in kids[1:]:\n'
        '                        merges.append((level, [acc, c]))\n                        acc = nb + len(merges) - 1\n'
        '                    top[root] = acc\n',
        ['two_triangles', 'octa'], 'multifusion remplacee par une chaine de fusions binaires de meme niveau'),
    'plateau_split': _mutant(
        B, '            while j < len(joins) and joins[j][0].level == level:\n                j += 1\n',
        '            j += 1\n',
        ['line5', 'square'], 'jonctions d\'un meme niveau exact traitees une a une : plateau non atomique'),
    'point_open': _mutant(
        B, '            node = _ancestor(res, start, level)\n            core[self.inp[x]] = Entry(level, node)\n',
        '            node = start\n'
        '            while res.parent[node] >= 0 and res.nodes[res.parent[node]].level < level:\n'
        '                node = res.parent[node]\n            core[self.inp[x]] = Entry(level, node)\n',
        ['line5', 'line024'], 'entree core lue a la coupe ouverte : un rang trop bas quand D_k est un niveau de fusion'),
    'core_next_neighbour': _mutant(
        B, '            level = Fraction(near[k - 1][0])\n',
        '            level = Fraction(near[min(k, self.n - 1)][0])\n',
        ['e5', 'two_triangles'], 'entree core a D_(k+1) au lieu de D_k'),
    'vertical_open': _mutant(
        B, '                    lower[v] = _ancestor(prev, below[self.descend(part, k - 1)], node.level)\n',
        '                    lower[v] = below[self.descend(part, k - 1)]\n',
        ['audit3', 'e5'], 'image verticale d\'une naissance sans remontee au niveau de la boule'),
    'vertical_merge_child': _mutant(
        B, '                    images = set(_ancestor(prev, lower[c], node.level) for c in node.children)\n',
        '                    images = set(lower[c] for c in node.children[:1])\n',
        ['audit3', 'e5'], 'image verticale d\'une fusion : celle d\'un enfant, sans remontee'),
    'join_missing': _mutant(
        B, "                elif kind == 'join':\n",
        "                elif kind == 'join' and not (ball.qmin == 3 and ball.index % 2 == 0):\n",
        ['e5'], 'jonction manquante : une fusion est deplacee, la tour mutee reste coherente (constat L01 de la v10)',
        silent=True),
    'window_low': _mutant(
        B, '            b.lo, b.hi = max(b.p + b.qmin - 1, 1), min(b.p + b.m, self.orders)\n',
        '            b.lo, b.hi = max(b.p + b.qmin, 1), min(b.p + b.m, self.orders)\n',
        ['pair', 'e5'], 'fenetre basse p + q_min : la jonction a t = q_min - 1 est perdue'),
    'drop_last_rep': _mutant(
        B, '                for r in roots[1:]:\n                    x, y = find(roots[0]), find(r)\n',
        '                for r in (roots[1:-1] if len(roots) >= 3 else roots[1:]):\n'
        '                    x, y = find(roots[0]), find(r)\n',
        ['e5', 'triangle_far'], 'derniere union omise dans les jonctions d\'au moins trois representants'),
    'regular_join_pieces': _mutant(
        B, "                return 'join', tuple(shell[:j] + shell[j + 1:] for j in range(m))\n",
        "                return 'join', tuple(shell[:j] + shell[j + 1:] for j in range(m - 1))\n",
        ['pair', 'e5'], 'coquille reguliere : un morceau oublie a t = m - 1'),
    'admission_strict': _mutant(
        B, '                    admitted = ball.p + ball.qmin <= self.kmax + 1\n',
        '                    admitted = ball.p + ball.qmin <= self.kmax\n',
        ['e5', 'line5'], 'admission p + q_min <= K : les jonctions de l\'ordre K manquent'),
    'cover_population': _mutant(
        B, '            if ball.p + ball.m < k:\n                continue\n',
        '            if ball.p + ball.m <= k:\n                continue\n',
        ['pair', 'e5'], 'boule couvrante exigee de population > k'),
    'cover_node_open': _mutant(
        B, '            node = _ancestor(res, birth_node[self.descend(tuple(pop[:k]), k)], ball.level)\n',
        '            node = birth_node[self.descend(tuple(pop[:k]), k)]\n',
        ['audit3', 'e5'], 'boule couvrante attachee a la naissance de sa descente, sans remontee a son niveau'),
    'meb_three': _mutant(
        B, '        for q in range(1, min(4, len(sites)) + 1):\n            for support in combinations(sites, q):\n'
        '                sph = self._sphere(support)\n                if sph is None or (best',
        '        for q in range(1, min(3, len(sites)) + 1):\n            for support in combinations(sites, q):\n'
        '                sph = self._sphere(support)\n                if sph is None or (best',
        ['tetra_center', 'generic6'], 'boule minimale cherchee parmi les supports d\'au plus trois sites'),
    'weighted_as_regular': _mutant(
        B, '        b.regular = b.m == b.qmin\n', '        b.regular = len(shell) == b.qmin\n',
        ['pair_weighted', 'triangle_weighted_311'], 'coquille ponderee traitee par le raccourci des coquilles regulieres'),
    # ---- theoreme de Gordan : temoins de l'enveloppe convexe fermee
    'hull_no_pairs': _mutant(
        'intgeom.py', '    for i in range(m):\n        for j in range(i + 1, m):\n'
        '            if is_midpoint(shell[i], shell[j], anchor, center):\n                out.append(1 << i | 1 << j)\n',
        '', ['square_weighted', 'firstcov_k3_n6'], 'paires antipodales oubliees dans le test de separabilite'),
    'hull_no_tetra': _mutant(
        'intgeom.py', '                    if orient(t[0], t[1], t[2], t[3]) != 0 and tetra_position(t, anchor, center) >= 0:\n'
        '                        out.append(1 << i | 1 << j | 1 << k | 1 << l)\n',
        '                    if orient(t[0], t[1], t[2], t[3]) != 0 and tetra_position(t, anchor, center) >= 0:\n'
        '                        continue\n',
        ['q3_tetra_form'], 'tetraedres contenant le centre oublies dans le test de separabilite'),
    # ---- etage A : une definition fausse doit contredire l'etage B
    'edge_at_vertex_level': _mutant(
        'definition.py', '                bucket(self.meb(union)[0])[1].append(union)\n',
        '                bucket(max(self.meb(union[:j] + union[j + 1:])[0] for j in range(k + 1)))[1].append(union)\n',
        ['two_triangles', 'audit3'], 'arete de Gamma_k au niveau de ses sommets, pas a celui de leur reunion'),
    'meb_largest': _mutant(
        'definition.py', '                    if cand is None or (best is not None and cand[0] >= best[0]) or inside & ~cand[2]:\n',
        '                    if cand is None or (best is not None and cand[0] <= best[0]) or inside & ~cand[2]:\n',
        ['e5', 'generic6'], 'boule minimale : la plus grande sphere candidate qui contient la partie', silent=True),
    # ---- mutants equivalents : libertes de la descente et redondance des temoins, sans effet sur l'objet
    'jump_any': _mutant(
        B, '                near = sorted((G.side_key(anchor, ctr, self.sites[self.site_of[x]]), x) for x in ball.I)\n',
        '                near = [(0, x) for x in ball.I]\n',
        ['e5', 'generic6', 'cube', 'firstcov_k3_n6'],
        'saut vers k points interieurs quelconques au lieu des k plus proches du centre', True),
    'first_rep_last': _mutant(
        B, '            return shell[1:] if t + 1 == m else shell[:t]\n',
        '            return shell[:-1] if t + 1 == m else shell[m - t:]\n',
        ['e5', 'generic6', 'cube', 'firstcov_k3_n6'], 'descente par le dernier representant au lieu du premier', True),
    'hull_strict_triangle': _mutant(
        'intgeom.py', '                if cross(sub(b, a), sub(c, a)) == (0, 0, 0) or not non_obtuse(a, b, c):\n',
        '                if cross(sub(b, a), sub(c, a)) == (0, 0, 0) or not acute(a, b, c):\n',
        ['square', 'right_triangle', 'cube', 'octa_weighted'],
        'triangles rectangles retires des temoins : leur hypotenuse est deja un temoin antipodal', True),
}


def load(name, package_dir):
    """Copie le paquet dans un dossier temporaire, applique le correctif, importe la copie sous hgp11_mutant_<nom>."""
    mutant = MUTANTS[name]
    tmp = tempfile.mkdtemp(prefix='hgp11_mutant_')
    try:
        target = os.path.join(tmp, 'hgp11_ref')
        shutil.copytree(package_dir, target, ignore=shutil.ignore_patterns('__pycache__'))
        path = os.path.join(target, mutant['file'])
        with open(path, encoding='ascii') as f:
            text = f.read()
        if text.count(mutant['old']) != 1:
            raise StalePatch('motif present %d fois dans %s' % (text.count(mutant['old']), mutant['file']))
        with open(path, 'w', encoding='ascii') as f:
            f.write(text.replace(mutant['old'], mutant['new']))
        alias = 'hgp11_mutant_' + name
        spec = importlib.util.spec_from_file_location(alias, os.path.join(target, '__init__.py'),
                                                      submodule_search_locations=[target])
        module = importlib.util.module_from_spec(spec)
        sys.modules[alias] = module
        spec.loader.exec_module(module)
        module.judge = importlib.import_module(alias + '.judge')
        return module
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def socle_manifest():
    """Manifeste des mutants reels au format de tests/mutants/run_mutants.py."""
    real = sorted(name for name, m in MUTANTS.items() if not m['equivalent'])
    return dict(module='reference', plancher=len(real), mutants=[
        dict(id=name, fichier='reference/hgp11_ref/' + MUTANTS[name]['file'], cherche=MUTANTS[name]['old'],
             remplace=MUTANTS[name]['new'], porte='mhgp11_reference_fast', note=MUTANTS[name]['why']) for name in real])


if __name__ == '__main__':
    if sys.argv[1:] != ['--socle-manifest']:
        print(__doc__)
        sys.exit(2)
    print(json.dumps(socle_manifest(), indent=2, sort_keys=True))

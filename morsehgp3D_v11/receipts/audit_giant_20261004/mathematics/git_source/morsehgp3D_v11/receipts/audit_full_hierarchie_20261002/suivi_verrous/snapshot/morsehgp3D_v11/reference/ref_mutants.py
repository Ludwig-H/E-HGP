"""Mutants de la reference : correctifs appliques a une COPIE des sources de hgp11_ref (aucun crochet dans les
sources). Chaque mutant reel doit etre tue ; chaque mutant declare equivalent (une liberte de l'algorithme, sans effet
sur l'objet) doit survivre.

Un mutant : file (fichier de hgp11_ref), old (texte present UNE SEULE fois dans ce fichier), new, fixtures (nuages
graves de la suite rapide, par leur nom ; 'nom@K' impose l'ordre K), why.
  MUTANTS       juges par test_ref.py (etage B contre etage A) ;
  DUMP_MUTANTS  juges par test_dump_v10.py (serialisation contre le binaire fige de la v10).
load(name, package_dir) rend le paquet mute, importe sous un nom propre a cote du paquet intact.

    python3 ref_mutants.py --socle-manifest    ecrit les mutants reels de MUTANTS au format des manifestes de
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


def _dump_mutant(file, old, new, fixtures, why, writing_only=False):
    """writing_only : le mutant ne change que l'ecriture non reduite des niveaux ; il doit etre tue octet pour octet
    et survivre a la comparaison des dumps a niveaux reduits (dumps.reduce_tower_levels)."""
    return dict(file=file, old=old, new=new, fixtures=fixtures, why=why, writing_only=writing_only)


A, B, D, G = 'definition.py', 'constructive.py', 'dumps.py', 'intgeom.py'
S12, S16, S20 = ' ' * 12, ' ' * 16, ' ' * 20

MUTANTS = {
    # ---- fautes de la tour (classes de l'audit L06 de la v10 : trois d'entre elles traversaient ses portes)
    'binarize': _mutant(
        B, S20 + 'top[root] = nb + len(merges)\n' + S20 + 'merges.append((level, kids))\n',
        S20 + 'acc = kids[0]\n' + S20 + 'for c in kids[1:]:\n' + S20 + '    merges.append((level, [acc, c]))\n' +
        S20 + '    acc = nb + len(merges) - 1\n' + S20 + 'top[root] = acc\n',
        ['two_triangles', 'octa'], 'multifusion remplacee par une chaine de fusions binaires de meme niveau'),
    'plateau_split': _mutant(
        B, S12 + 'while j < len(joins) and joins[j][0].level == level:\n' + S16 + 'j += 1\n', S12 + 'j += 1\n',
        ['line5', 'square'], 'jonctions d\'un meme niveau exact traitees une a une : plateau non atomique'),
    'point_open': _mutant(
        B, S12 + 'node = tree.ancestor(start, level)\n' + S12 + 'core[self.inp[x]] = Entry(level, node)\n',
        S12 + 'node = start\n' +
        S12 + 'while tree.parent[node] >= 0 and tree.nodes[tree.parent[node]].level < level:\n' +
        S16 + 'node = tree.parent[node]\n' + S12 + 'core[self.inp[x]] = Entry(level, node)\n',
        ['line5', 'line024'], 'entree core lue a la coupe ouverte : un rang trop bas si D_k est un niveau de fusion'),
    'core_next_neighbour': _mutant(
        B, S12 + 'level = Fraction(near[k - 1][0])\n', S12 + 'level = Fraction(near[min(k, self.n - 1)][0])\n',
        ['e5', 'two_triangles'], 'entree core a D_(k+1) au lieu de D_k'),
    'vertical_open': _mutant(
        B, S20 + 'lower[v] = prev.ancestor(below[self.descend(part, k - 1)], node.level)\n',
        S20 + 'lower[v] = below[self.descend(part, k - 1)]\n',
        ['audit3', 'e5'], 'image verticale d\'une naissance sans remontee au niveau de la boule'),
    'vertical_merge_child': _mutant(
        B, S20 + 'images = set(prev.ancestor(lower[c], node.level) for c in node.children)\n',
        S20 + 'images = set(lower[c] for c in node.children[:1])\n',
        ['audit3', 'e5'], 'image verticale d\'une fusion : celle d\'un enfant, sans remontee'),
    'join_missing': _mutant(
        B, S16 + "elif kind == 'join':\n",
        S16 + "elif kind == 'join' and not (ball.qmin == 3 and ball.index % 2 == 0):\n",
        ['e5'], 'jonction manquante : une fusion est deplacee, la tour mutee reste coherente (constat L01 de la v10)',
        silent=True),
    'window_low': _mutant(
        B, S12 + 'b.lo, b.hi = max(b.p + b.qmin - 1, 1), min(b.p + b.m, self.orders)\n',
        S12 + 'b.lo, b.hi = max(b.p + b.qmin, 1), min(b.p + b.m, self.orders)\n',
        ['pair', 'e5'], 'fenetre basse p + q_min : la jonction a t = q_min - 1 est perdue'),
    'drop_last_rep': _mutant(
        B, S16 + 'for r in roots[1:]:\n' + S20 + 'x, y = find(roots[0]), find(r)\n',
        S16 + 'for r in (roots[1:-1] if len(roots) >= 3 else roots[1:]):\n' +
        S20 + 'x, y = find(roots[0]), find(r)\n',
        ['e5', 'triangle_far'], 'derniere union omise dans les jonctions d\'au moins trois representants'),
    'regular_join_pieces': _mutant(
        B, S16 + "return 'join', tuple(shell[:j] + shell[j + 1:] for j in range(m))\n",
        S16 + "return 'join', tuple(shell[:j] + shell[j + 1:] for j in range(m - 1))\n",
        ['pair', 'e5'], 'coquille reguliere : un morceau oublie a t = m - 1'),
    'admission_strict': _mutant(
        B, S20 + 'admitted = ball.p + ball.qmin <= self.kmax + 1\n',
        S20 + 'admitted = ball.p + ball.qmin <= self.kmax\n',
        ['e5', 'line5'], 'admission p + q_min <= K : les jonctions de l\'ordre K manquent'),
    'cover_population': _mutant(
        B, S12 + 'if ball.p + ball.m < k:\n', S12 + 'if ball.p + ball.m <= k:\n',
        ['pair', 'e5'], 'boule couvrante exigee de population > k'),
    'cover_node_open': _mutant(
        B, S12 + 'node = tree.ancestor(birth_node[self.descend(tuple(pop[:k]), k)], ball.level)\n',
        S12 + 'node = birth_node[self.descend(tuple(pop[:k]), k)]\n',
        ['audit3', 'e5'], 'boule couvrante attachee a la naissance de sa descente, sans remontee a son niveau'),
    'meb_three': _mutant(
        B, '        for q in range(1, min(4, len(sites)) + 1):\n',
        '        for q in range(1, min(3, len(sites)) + 1):\n',
        ['tetra_center', 'generic6'], 'boule minimale cherchee parmi les supports d\'au plus trois sites'),
    'weighted_as_regular': _mutant(
        B, '        b.regular = b.m == b.qmin\n', '        b.regular = len(shell) == b.qmin\n',
        ['pair_weighted', 'triangle_weighted_311'],
        'coquille ponderee traitee par le raccourci des coquilles regulieres'),
    'ancestor_open': _mutant(
        B, 'while self.parent[node] >= 0 and self.nodes[self.parent[node]].level <= level:\n',
        'while self.parent[node] >= 0 and self.nodes[self.parent[node]].level < level:\n',
        ['line5', 'audit3'], 'ancetre lu a la coupe ouverte dans toute la tour (attaches, verticales, couverture)'),
    'numbering_by_catalogue': _mutant(
        B, 'key=lambda i: (births[i].level, births[i].center))):\n', 'key=lambda i: (births[i].level, i))):\n',
        ['e5', 'generic6'], 'naissances d\'un meme niveau numerotees dans l\'ordre du catalogue, pas par centre'),
    # ---- theoreme de Gordan : temoins de l'enveloppe convexe fermee
    'hull_no_pairs': _mutant(
        G, S12 + 'if is_midpoint(shell[i], shell[j], anchor, center):\n' + S16 + 'out.append(1 << i | 1 << j)\n',
        S12 + 'if is_midpoint(shell[i], shell[j], anchor, center):\n' + S16 + 'continue\n',
        ['square_weighted', 'firstcov_k3_n6'], 'paires antipodales oubliees dans le test de separabilite'),
    'hull_no_tetra': _mutant(
        G, ' ' * 24 + 'out.append(1 << i | 1 << j | 1 << k | 1 << h)\n', ' ' * 24 + 'continue\n',
        ['q3_tetra_form'], 'tetraedres contenant le centre oublies dans le test de separabilite'),
    # ---- etage A : une definition fausse doit contredire l'etage B
    'edge_at_vertex_level': _mutant(
        A, S16 + 'bucket(self.meb(union)[0])[1].append(union)\n',
        S16 + 'bucket(max(self.meb(union[:j] + union[j + 1:])[0] for j in range(k + 1)))[1].append(union)\n',
        ['two_triangles', 'audit3'], 'arete de Gamma_k au niveau de ses sommets, pas a celui de leur reunion'),
    'meb_largest': _mutant(
        A, S20 + 'if cand is None or (best is not None and cand[0] >= best[0]) or inside & ~cand[2]:\n',
        S20 + 'if cand is None or (best is not None and cand[0] <= best[0]) or inside & ~cand[2]:\n',
        ['e5', 'generic6'], 'boule minimale : la plus grande sphere candidate qui contient la partie', silent=True),
    'numbering_by_sweep': _mutant(
        A, 'key=lambda j: (merges[j][0], first[-1 - j]))\n', 'key=lambda j: (merges[j][0], j))\n',
        ['grid3_11_5', 'coplanar_11_1'], 'fusions d\'un meme niveau numerotees dans l\'ordre du balayage'),
    # ---- mutants equivalents : libertes de la descente et redondance des temoins, sans effet sur l'objet
    'jump_any': _mutant(
        B, S16 + 'near = sorted((G.side_key(anchor, ctr, self.sites[self.site_of[x]]), x) for x in ball.inner)\n',
        S16 + 'near = [(0, x) for x in ball.inner]\n', ['e5', 'generic6', 'cube', 'firstcov_k3_n6'],
        'saut vers k points interieurs quelconques au lieu des k plus proches du centre', equivalent=True),
    'first_rep_last': _mutant(
        B, S12 + 'return shell[1:] if t + 1 == m else shell[:t]\n',
        S12 + 'return shell[:-1] if t + 1 == m else shell[m - t:]\n', ['e5', 'generic6', 'cube', 'firstcov_k3_n6'],
        'descente par le dernier representant au lieu du premier', equivalent=True),
    'hull_strict_triangle': _mutant(
        G, S16 + 'if cross(sub(b, a), sub(c, a)) == (0, 0, 0) or not non_obtuse(a, b, c):\n',
        S16 + 'if cross(sub(b, a), sub(c, a)) == (0, 0, 0) or not acute(a, b, c):\n',
        ['square', 'right_triangle', 'cube', 'octa_weighted'],
        'triangles rectangles retires des temoins : leur hypotenuse est deja un temoin antipodal', equivalent=True),
}

DUMP_MUTANTS = {
    'reduced_levels': _dump_mutant(
        D, "        return '%d %d' % self.table[self.rank_of[level]]\n",
        "        return '%d %d' % (level.numerator, level.denominator)\n",
        ['e5', 'square'], 'niveaux ecrits en fractions reduites', writing_only=True),
    'rank_last_ball': _dump_mutant(
        D, S16 + 'self.table.append(emitted_level(ref, b))\n',
        S16 + 'self.table.append(emitted_level(ref, b))\n' +
        S12 + 'self.table[self.rank_of[b.level]] = emitted_level(ref, b)\n',
        ['circle25_pair'], 'ecriture d\'un rang prise a sa derniere boule au lieu de la premiere', writing_only=True),
    'q3_form_from_support': _dump_mutant(
        D, S20 + 'if c4 is not None and G.tetra_position(t, t[0], c4) == 1:\n',
        S20 + 'if c4 is not None and G.tetra_position(t, t[0], c4) == 2:\n',
        ['q3_tetra_form'], 'boule q_min = 3 a coquille etendue ecrite dans la forme de S*', writing_only=True),
    'rank_by_double': _dump_mutant(
        D, S16 + 'self.rank_of[b.level] = len(self.table)\n',
        S16 + 'same = [lv for lv in self.rank_of if float(lv) == float(b.level)]\n' +
        S16 + 'self.rank_of[b.level] = self.rank_of[same[0]] if same else len(self.table)\n',
        ['double_collision'], 'rang decide sur le double du niveau : deux niveaux exacts distincts confondus'),
    'lexicographic_sites': _dump_mutant(
        B, 'key=lambda i: (G.morton(self.input[i]), i))  # interne -> entree\n',
        'key=lambda i: (self.input[i], i))  # interne -> entree\n',
        ['e5', 'generic6'], 'sites dans l\'ordre lexicographique des coordonnees au lieu de l\'ordre de Morton'),
    'weighted_flag_lost': _dump_mutant(
        D, '        return (EXTENDED_SHELL if ball.extended else 0) | (WEIGHTED_SHELL if ball.weighted else 0)\n',
        '        return EXTENDED_SHELL if ball.extended else 0\n',
        ['pair_weighted', 'octa_weighted'], 'drapeau de coquille ponderee perdu'),
    'admission_single': _dump_mutant(
        B, S16 + "if ball.weighted and self.admission == 'v10':\n",
        S16 + "if ball.weighted and self.admission == 'single':\n",
        ['triangle_weighted@1'], 'regle d\'admission unique : le binaire fige suit la regle ponderee de la v10'),
    'cover_last_ball': _dump_mutant(
        B, S20 + 'cover[i][1].add(node)\n', S20 + 'cover[i][1].add(node)\n' + S20 + 'first[i] = (ball.index, node)\n',
        ['line024', 'two_triangles'],
        'entree cover : derniere boule couvrante du premier niveau au lieu de la premiere'),
    'merge_numbering': _dump_mutant(
        D, '        merges.append((nodes[v].level, leaf[v], v))\n',
        '        merges.append((nodes[v].level, -leaf[v], v))\n',
        ['two_triangles', 'line5'], 'fusions d\'un meme niveau numerotees par plus grande naissance'),
    'support_last': _dump_mutant(
        D, 'for sites in (b.support, b.inner_sites, b.shell_sites)]\n',
        'for sites in (b.shell_sites[-b.qmin:], b.inner_sites, b.shell_sites)]\n',
        ['square', 'cube'], 'support publie : les derniers sites de la coquille au lieu du support canonique'),
}


def clouds_of(mutant, clouds):
    """Nuages d'un mutant, pris par leur nom dans une liste de nuages graves (fixtures et familles de la suite
    rapide) : 'nom' (a son ordre grave) ou 'nom@K' (a l'ordre K)."""
    by_name = dict((c.name, c) for c in clouds)
    out = []
    for spec in mutant['fixtures']:
        name, _at, kmax = spec.partition('@')
        out.append(by_name[name] if not kmax else by_name[name]._replace(kmax=int(kmax)))
    return out


def load(name, package_dir):
    """Copie le paquet dans un dossier temporaire, applique le correctif, importe la copie sous hgp11_mutant_<nom>."""
    mutant = MUTANTS[name] if name in MUTANTS else DUMP_MUTANTS[name]
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
        module.dumps = importlib.import_module(alias + '.dumps')
        return module
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def socle_manifest():
    """Manifeste des mutants reels de MUTANTS au format de tests/mutants/run_mutants.py."""
    real = sorted(name for name, m in MUTANTS.items() if not m['equivalent'])
    return dict(module='reference', plancher=len(real), mutants=[
        dict(id=name, fichier='reference/hgp11_ref/' + MUTANTS[name]['file'], cherche=MUTANTS[name]['old'],
             remplace=MUTANTS[name]['new'], porte='mhgp11_reference_fast', note=MUTANTS[name]['why']) for name in real])


if __name__ == '__main__':
    if sys.argv[1:] != ['--socle-manifest']:
        print(__doc__)
        sys.exit(2)
    print(json.dumps(socle_manifest(), indent=2, sort_keys=True))

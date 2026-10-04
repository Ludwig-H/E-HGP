#!/usr/bin/env python3
"""Exception du theoreme B : epsilon au-dessus d'une scission N-aire de la racine en au moins trois gros enfants.

Dendrogramme N-aire : A = {0,1,2}, B = {3,4,5}, C = {6,7,8} nes au niveau 1 (plateaux de trois singletons), racine
au niveau 10 avec trois enfants A, B, C ; mcs = 3. EOM retient A, B, C (feuilles ; racine exclue). Pour eps = 20 :
la regle N-aire remonte chaque feuille jusqu'a l'enfant de la racine (lui-meme) et garde {A, B, C} ; toute
binarisation de la racine cree un noeud intermediaire (union de deux enfants, ne au niveau 10) que traverse_upwards
de sklearn rend a la place des enfants. Le code compile de sklearn echoue ici (numpy 2.5) : transcription.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B root_eps.py > root_eps_out.txt
"""
import itertools

import numpy as np

import nary_head as nh
import sk_transcription as skt


def main():
    d = nh.Dendrogram(9)
    a = d.add([0, 1, 2], 1.0, 0)
    b = d.add([3, 4, 5], 1.0, 0)
    c = d.add([6, 7, 8], 1.0, 0)
    d.add([a, b, c], 10.0, 1)
    names = 'abcdefghi'

    def show(lab):
        part, noise = nh.canonical_partition(lab)
        s = ' | '.join(sorted(''.join(names[i] for i in sorted(g)) for g in part))
        return s + ('  ; bruit : ' + ''.join(names[i] for i in sorted(noise)) if noise else '')

    for eps in (0.0, 5.0, 20.0):
        print('eps = %4.1f | N-aire : %s' % (eps, show(nh.head(d, 3, 1.0, 'eom', eps))))
        seen = set()
        for order in itertools.permutations(range(3)):
            # binarisation de la racine dans l'ordre donne des enfants (les trios dans l'ordre canonique)
            rows = []
            n = 9
            ident = {}

            def emit(x, y, v, s):
                rows.append((x, y, v, s))
                return n + len(rows) - 1
            for t, base in enumerate((0, 3, 6)):
                u = emit(base, base + 1, 1.0, 2)
                ident[t] = emit(u, base + 2, 1.0, 3)
            o = list(order)
            u = emit(ident[o[0]], ident[o[1]], 10.0, 6)
            emit(u, ident[o[2]], 10.0, 9)
            tree = np.array(rows, dtype=nh.HIERARCHY_dtype)
            lab = skt.tree_to_labels(tree, 3, 'eom', False, eps)
            seen.add(show(lab))
        print('           | sklearn (transcription), 6 binarisations de la racine : %s' % sorted(seen))


if __name__ == '__main__':
    main()

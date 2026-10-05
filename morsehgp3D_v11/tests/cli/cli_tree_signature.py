#!/usr/bin/env python3
"""Porte mhgp11_cli_tree_signature (tranche S5 ; integration L1, point 3 des auditeurs du 5 octobre 2026) : le champ
tree_k_sha256 PUBLIE par l'executable mhgp11 dans manifeste.json, contre une serialisation independante de la
signature version 2 (docs/SORTIES.md, paragraphe 8), ecrite ici en bibliotheque standard d'apres le modele de
l'auditeur (receipts/audit_supports_implementation_20261004/evidence/check_d2_signature.py, reponse D.2).

    python3 cli_tree_signature.py --cli <mhgp11> --bits <18|21|24>

Les trois fixtures de l'auditeur, a deux ordres distincts : le singleton (3, 2, 1) et la paire (0, 2, 0), (2, 0, 0) a
K = 1, la ligne (0, 0, 0), (1, 0, 0), (2, 0, 0) a K = 2. Leur arbre d'ordre K (parents, rangs denses de Cat_K, genres,
naissances en SiteIdx, enfants) est ecrit a la main, sans le moteur : sites dans l'ordre de Morton, feuilles de K = 1
dans l'ordre lexicographique des sites, S* des naissances de boules. La signature est hachee ici (hashlib) pour le
profil de la construction ; aux profils 21 et 24 bits, elle doit aussi egaler la valeur publiee par l'auditeur (au
profil 18, celle de son modele). Chaque fixture est publiee deux fois par le CLI : entree dans l'ordre donne, puis
permutee et reetiquetee (PointId 0 et 0xFFFFFFFF compris) ; le champ publie doit egaler la signature les deux fois.
Une constante, une signature d'ordre 1 ou une version 1 publiees sont refusees.
Codes : 0 conforme, 1 ecart, 3 plancher. Dernieres lignes :
    cli_tree_signature_verdict conforme cas3 appels6
    cli_tree_signature_ok controles=<n>
Python 3.10 nu, aucun assert.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cli_support as cs  # noqa: E402

mhgp11_gate = cs.mhgp11_gate
NONE = (1 << 32) - 1

# (nom, K, points dans l'ordre de l'entree, sites dans l'ordre des SiteIdx, noeuds (parent, rang, genre, naissance)).
# Genre 0 : feuille-site a K = 1 (naissance : la ligne de son site) ; 1 : naissance de boule (naissance : S*) ;
# 2 : fusion (aucune naissance).
FIXTURES = (
    ('singleton', 1, [(3, 2, 1)], [(3, 2, 1)], [(NONE, 0, 0, (0,))]),
    ('paire', 1, [(0, 2, 0), (2, 0, 0)], [(2, 0, 0), (0, 2, 0)],
     [(2, 0, 0, (1,)), (2, 0, 0, (0,)), (NONE, 1, 2, ())]),
    ('ligne', 2, [(0, 0, 0), (1, 0, 0), (2, 0, 0)], [(0, 0, 0), (1, 0, 0), (2, 0, 0)],
     [(2, 1, 1, (0, 1)), (2, 1, 1, (1, 2)), (NONE, 2, 2, ())]),
)
# Valeurs de l'auditeur : 21 et 24 bits publiees par lui (reponse D.2, aef7182b3) ; 18 bits calculees par sa fonction
# signature avec bits = 18 (graves aussi dans tests/api/session_test.cpp).
AUDITOR = {
    'singleton': {18: 'c66a89f61f6ae1c86ac97ecf5ee1a7fbd3c8550e5db40af6f5d825a44f1121da',
                  21: '8e991c2147dd5344a2012998e4052bddec912f533989cc8bef0376f26d7d12e6',
                  24: '4ee1bfe286ebb0d15a9b19d649cb7233f83b790e4472d94ba9acf36252070996'},
    'paire': {18: '15d33ee089f3fe5303e3b73dbc1d58716032a7c2e32bfffe5ce5ba19872f3af8',
              21: '4878adc74b9cd5b03f6c5d95e60c95faeb0d4462e785ed01eeff093c214e5c46',
              24: 'eb765ff8c867e7fb001eccb6f618938de063d4d0526aa7b712d12197aea2f863'},
    'ligne': {18: 'd5161a76bcaf3850956f7f7d2bd5b75768d34d794ccceeea2ca81d3430eac5ea',
              21: '82f39093f987bd9dccaa10339251072d3c033e7f8dfa4dc34e0c7a17ba20a0a1',
              24: 'e6d408f77aa2acebd8c06e86cc476c515bbf893bd208699657b5d6bd45228961'},
}


def u32(value):
    return struct.pack('<I', value)


def u64(value):
    return struct.pack('<Q', value)


def morton(point):
    return sum(((point[axis] >> bit) & 1) << (3 * bit + axis) for axis in range(3) for bit in range(24))


def signature(bits, k, sites, nodes):
    """Signature version 2 : MHGP11TK, cinq u64, empreinte de la geometrie, puis chaque noeud (parent, rang, genre,
    arite et naissance, enfants recalcules depuis les parents, croissants)."""
    children = [[] for _ in nodes]
    for i, node in enumerate(nodes):
        if node[0] != NONE:
            children[node[0]].append(i)
    geometry = b'MHGP11GX' + u64(bits) + u64(len(sites)) + b''.join(u32(c) for p in sites for c in p)
    raw = b'MHGP11TK' + b''.join(u64(x) for x in (2, bits, k, len(sites), len(nodes)))
    raw += hashlib.sha256(geometry).digest()
    for i, (parent, rank, kind, birth) in enumerate(nodes):
        raw += u32(parent) + u32(rank) + bytes((kind, len(birth))) + b''.join(u32(x) for x in birth)
        raw += u32(len(children[i])) + b''.join(u32(c) for c in children[i])
    return hashlib.sha256(raw).hexdigest()


def structure_ok(k, sites, nodes):
    """Coherence de la structure ecrite a la main : sites en ordre de Morton, feuilles de K = 1 en ordre
    lexicographique des sites, une seule racine."""
    order = sorted(range(len(sites)), key=lambda s: sites[s])
    leaves = [birth for (_p, _r, kind, birth) in nodes if kind == 0]
    return (sites == sorted(sites, key=morton) and sum(1 for n in nodes if n[0] == NONE) == 1 and
            (k != 1 or leaves == [(s,) for s in order]))


def published(gate, args, root, name, k, points, ids):
    """Publie la fixture par le CLI ; rend le champ tree_k_sha256 du manifeste, ou None."""
    folder = tempfile.mkdtemp(prefix=name, dir=root)
    xyz, names = cs.write_inputs(folder, points, ids)
    directory = os.path.join(folder, 'D')
    result, rows = cs.run_cli(cs.cli_argv(args.cli, xyz, names, directory, k), timeout=120)
    line = rows[0] if len(rows) == 1 and rows[0] else {}
    if not gate.check(result.code == 0 and line.get('status') == 'ok', '%s : %s' % (name, result.describe())):
        return None
    try:
        manifest = cs.formats.check_directory(directory, args.bits)['manifest']
    except (OSError, ValueError) as error:
        gate.check(False, '%s : dossier non conforme : %s' % (name, error))
        return None
    with open(os.path.join(directory, cs.formats.MANIFEST), 'rb') as handle:
        raw = json.loads(handle.read())
    gate.check_eq(raw.get('tree_k_sha256'), manifest['tree_k_sha256'], '%s : lecture du manifeste' % name)
    return manifest['tree_k_sha256']


def main():
    parser = argparse.ArgumentParser(description='Champ tree_k_sha256 publie contre la signature version 2.')
    parser.add_argument('--cli', required=True)
    parser.add_argument('--bits', type=int, choices=(18, 21, 24), required=True)
    args = parser.parse_args()
    gate = mhgp11_gate.Gate('cli_tree_signature')
    cases = calls = 0
    with tempfile.TemporaryDirectory(prefix='mhgp11-cli-tree-') as root:
        for name, k, points, sites, nodes in FIXTURES:
            if not gate.check(structure_ok(k, sites, nodes), '%s : structure ecrite a la main' % name):
                continue
            want = signature(args.bits, k, sites, nodes)
            gate.check_eq(want, AUDITOR[name][args.bits], '%s : signature contre la valeur de l\'auditeur' % name)
            other = signature(args.bits, 1 if k > 1 else 2, sites, nodes)
            gate.check(other != want, '%s : la signature depend de K' % name)
            order = list(reversed(range(len(points))))
            for label, pts, ids in ((name, points, [7 + 13 * i for i in range(len(points))]),
                                    (name + '_permute', [points[i] for i in order],
                                     [[NONE, 0, 99][j % 3] for j in range(len(points))])):
                field = published(gate, args, root, label, k, pts, ids)
                calls += 1
                gate.check_eq(field, want, '%s : champ tree_k_sha256 publie, K = %d' % (label, k))
            cases += 1
    if gate.failures == 0 and cases == len(FIXTURES):
        print('cli_tree_signature_verdict conforme cas%d appels%d' % (cases, calls))
    return gate.finish(floor=20)


if __name__ == '__main__':
    sys.exit(main())

"""Rejeu des dumps archives par les contre-audits du 29 septembre 2026 contre le juge r1 et le juge r2.

    python3 rejeu_contre_audits.py CAPTURE SRC_R1 SRC_R2 [SORTIE.json]

CAPTURE : receipts/audit_continu_20260929/oracles_corrected (lecture seule). Les dumps y sont deja produits : aucun
binaire n'est lance. Catalogue : carre + (7,7,1), K = 3, dump archive et ses sept mutants exportes
(evidence/catalogue_mutants). Tour : AUDIT3, K = 2 (image fausse avant l'entree, attache au descendant mort, memes
mutations que scripts/replay_snapshots.py) et TRIANGLE, K = 3 (dump intact et mutant binarise, fichiers archives).
null : le juge accepte. Code de sortie : 0 si les temoins intacts sont acceptes par les deux juges, les controles
positifs rejetes par les deux, et chaque mutation nouvelle acceptee par r1 et rejetee par r2 ; 1 sinon.
"""
import copy
import importlib.util
import json
import os
import sys

CAT_NEW = ['duplicate_I', 'duplicate_U_extended', 'reverse_export_order', 'swap_equal_level_support_order',
           'reverse_shell_order']
CAT_POSITIVE = ['rank_offset_positive_control', 'noncanonical_support_positive_control']


def load(name, root, base):
    spec = importlib.util.spec_from_file_location(name, os.path.join(root, 'tests', 'oracle', base))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main():
    cap, src1, src2 = sys.argv[1], sys.argv[2], sys.argv[3]
    sys.dont_write_bytecode = True
    ev = os.path.join(cap, 'evidence')
    judges = {'r2': (load('t_r2', src2, 'test_tower_oracle.py'), load('c_r2', src2, 'test_catalogue_oracle.py')),
              'r1': (load('t_r1', src1, 'test_tower_oracle.py'), load('c_r1', src1, 'test_catalogue_oracle.py'))}
    out = {}
    for tag, (t, c) in judges.items():
        sites, weights = c.fixture('square')
        ref = c.Ref(sites)
        base = c.parse_dump(os.path.join(ev, 'normal', 'catalogue_base_archived.txt'))
        js = dict(balls=len(base), levels=len({b['rank'] for b in base}))
        cat = {'baseline': c.judge(ref, weights, 3, base, js)}
        for name in CAT_NEW + CAT_POSITIVE:
            got = c.parse_dump(os.path.join(ev, 'catalogue_mutants', name + '.txt'))
            cat[name] = c.judge(ref, weights, 3, got, dict(js, balls=len(got)))
        aud = t.parse(os.path.join(ev, 'normal', 'tower_base_archived.txt'))
        wrong = copy.deepcopy(aud)
        t.mut_vertical_audit(wrong)
        dead = copy.deepcopy(aud)
        pt, _v, e = dead[2]['points'][-1]
        dead[2]['points'][-1] = (pt, 1, e)
        nary = t.parse(os.path.join(ev, 'normal', 'nary', 'tower.txt'))
        nbin = t.parse(os.path.join(ev, 'normal', 'nary', 'tower_binarized_same_level.txt'))
        tower = dict(audit3_baseline=t.judge(t.AUDIT3, aud, 2), wrong_empty_vertical=t.judge(t.AUDIT3, wrong, 2),
                     point_attached_to_dead_descendant=t.judge(t.AUDIT3, dead, 2),
                     triangle_baseline=t.judge(t.TRIANGLE, nary, 3),
                     binarized_same_level=t.judge(t.TRIANGLE, nbin, 3))
        out[tag] = dict(catalogue=cat, tower=tower)
    ok = True
    for tag in ('r1', 'r2'):
        ok &= out[tag]['catalogue']['baseline'] is None
        ok &= out[tag]['tower']['audit3_baseline'] is None and out[tag]['tower']['triangle_baseline'] is None
        ok &= all(out[tag]['catalogue'][n] is not None for n in CAT_POSITIVE)
        ok &= out[tag]['tower']['wrong_empty_vertical'] is not None
    for n in CAT_NEW:
        ok &= out['r1']['catalogue'][n] is None and out['r2']['catalogue'][n] is not None
    for n in ('point_attached_to_dead_descendant', 'binarized_same_level'):
        ok &= out['r1']['tower'][n] is None and out['r2']['tower'][n] is not None
    out['null'] = 'le juge accepte le dump'
    out['verdict'] = 'ok' if ok else 'ECHEC'
    text = json.dumps(out, indent=1, ensure_ascii=False)
    print(text)
    if len(sys.argv) > 4:
        with open(sys.argv[4], 'w') as f:
            f.write(text + '\n')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())

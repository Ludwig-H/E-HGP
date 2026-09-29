"""Regression (29 septembre 2026, audit independant TETE_BANCS_PREUVES E1) : decide.py decidait sur un lot incomplet
(une scene sur 960, `complete=false`, code 0) et merge_sessions.py acceptait une scene hors du plan. La porte batit un
petit plan preenregistre sur l'espace `dev` (32 scenes, 3 methodes), ses variantes, et exige :
  - lot complet -> `decide.py --check-only` rend 0 ; les lots A et C archives (recus immuables) aussi ;
  - lot incomplet ou incoherent -> code 2, une ligne REFUS, aucun DECISION.json : scene manquante, couple manquant,
    doublon, scene ou methode hors plan, metadonnee fausse, score non fini, refus hors {0, 1}, colonne absente,
    run.json d'un autre plan, d'un autre nombre de scenes ou declare incomplet, plan altere sous la meme epingle,
    autre preenregistrement ; la sonde de l'audit (une scene du lot C, complete=false) en decision complete ;
  - fusion : scene hors plan ou autre plan -> 2 ; fusion partielle -> 0 mais complete=false, puis decide refuse ;
    sessions complementaires -> complete=true, puis decide accepte ; scene calculee deux fois -> 2.
Les scripts tournent sous `python3 -S` (sans site-packages, donc sans numpy, comme sur la VM G4 : label fast) ; un
refus doit donc preceder tout import de numpy. La porte reconstruit elle-meme les manifestes (oracle independant,
verifie sur les epingles des lots A et C) et ne genere aucune scene. Aucun assert : elle tient sous python3 -O.

  python3 test_decide_completeness.py <dossier de build>   -> code 0 si conforme, 1 sinon
"""
import ast
import csv
import gzip
import hashlib
import json
import os
import random
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
V10 = os.path.normpath(os.path.join(HERE, '..', '..'))
DECIDE = os.path.join(V10, 'bench', 'synthetic', 'decide.py')
MERGE = os.path.join(V10, 'bench', 'g4', 'merge_sessions.py')
SCENES = os.path.join(V10, 'bench', 'synthetic', 'scenes.py')
PREREG_DIR = os.path.join(V10, 'bench', 'synthetic', 'prereg')
LOTS = (('lot_A', 'PREREG_V10_KMATCH_A_20260928.json', 'test_kmatch_A_20260929', 'results.csv'),
        ('lot_C', 'PREREG_V10_COVER_C_20260929.json', 'test_cover_C_20260929', 'results.csv.gz'))
COLUMNS = ('unit', 'family', 'level', 'noise', 'n', 'seed', 'points', 'duplicates', 'zhat', 'method', 'ari_s',
           'ari_nc', 'ami_nc', 'coverage', 'clusters', 'refused', 'reason', 'seconds', 'shared_seconds')
METHODS = ('tour', 'hdb', 'temoin')
MINI_PLAN = dict(split='dev', families=['shells', 'spherical'], levels=['hard', 'easy'], sizes=[8000, 16000],
                 noises=[0.0, 0.1], replicates=2)


def axes():
    """FAMILIES et LEVELS de scenes.py (ordre canonique du manifeste), lus sans importer scenes (numpy)."""
    with open(SCENES) as f:
        tree = ast.parse(f.read())
    got = {}
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id in ('FAMILIES', 'LEVELS'):
                    got[target.id] = list(ast.literal_eval(node.value))
    return got['FAMILIES'], got['LEVELS']


def oracle_plan(plan):
    """Manifeste d'un plan (run_campaign.plan filtre, sha256 canonique), reecrit ici independamment de decide.py."""
    families, levels = axes()
    specs = []
    for family in [f for f in families if f in plan['families']]:
        for level in [v for v in levels if v in plan['levels']]:
            for n in plan['sizes']:
                for noise in plan['noises']:
                    base = {'family': family, 'n': n, 'groups': 8, 'level': level, 'noise_fraction': noise}
                    for r in range(plan['replicates']):
                        text = json.dumps(dict(base, split=plan['split'], replicate=r), sort_keys=True)
                        specs.append(dict(base, seed=int(hashlib.sha256(text.encode()).hexdigest()[:15], 16)))
    text = json.dumps(specs, sort_keys=True, separators=(',', ':'))
    return specs, hashlib.sha256(text.encode()).hexdigest()


def unit_of(spec):
    return '%s_n%d_%s_nu%g_s%d' % (spec['family'], spec['n'], spec['level'], spec['noise_fraction'], spec['seed'])


def sha256_file(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


def run(script, args):
    """Script du banc sous python3 -S : ni site-packages ni numpy."""
    r = subprocess.run([sys.executable, '-S', '-B', script] + args, capture_output=True, text=True, timeout=600)
    return r.returncode, r.stdout, r.stderr


def refused(code, out, run_dir=None):
    no_decision = run_dir is None or not any(os.path.exists(os.path.join(run_dir, f))
                                             for f in ('DECISION.json', 'DECISION.md'))
    return code == 2 and any(line.startswith('REFUS') for line in out.splitlines()) and no_decision


def accepted(code, out, run_dir):
    return (code == 0 and any(line.startswith('lot_conforme_au_plan') for line in out.splitlines()) and
            not os.path.exists(os.path.join(run_dir, 'DECISION.json')))


def write_lot(path, rows, info, fields=COLUMNS):
    os.makedirs(path)
    with open(os.path.join(path, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    with open(os.path.join(path, 'results.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=fields, extrasaction='ignore')
        w.writeheader()
        w.writerows(rows)


def mini_rows(specs):
    rng = random.Random(20260929)
    rows = []
    for i, s in enumerate(specs):
        for m in METHODS:
            refusal = 1 if (i == 0 and m == 'temoin') else 0  # un refus : compte comme ARI_s = 0, jamais omis
            rows.append(dict(unit=unit_of(s), family=s['family'], level=s['level'], noise=s['noise_fraction'],
                             n=s['n'], seed=s['seed'], points=s['n'], duplicates=0, zhat=3.0, method=m,
                             ari_s=round(rng.random(), 6), ari_nc=round(rng.random(), 6),
                             ami_nc=round(rng.random(), 6), coverage=1.0, clusters=8, refused=refusal,
                             reason='' if not refusal else 'refus de fixture', seconds=0.1, shared_seconds=0.2))
    return rows


def as_text(rows):
    """Lignes telles qu'un lecteur CSV les rend (chaines), pour les alterer champ par champ."""
    return [{k: str(v) for k, v in r.items()} for r in rows]


class Gate:
    def __init__(self, tmp):
        self.tmp = tmp
        self.results = []

    def check(self, name, ok, detail=''):
        self.results.append(ok)
        print('%-36s %s  %s' % (name, 'ok' if ok else 'ECHEC', detail), flush=True)

    def decide(self, name, prereg, run_dir, want, check_only=True):
        code, out, err = run(DECIDE, ['--prereg', prereg, '--run', run_dir] + (['--check-only'] if check_only else []))
        ok = accepted(code, out, run_dir) if want == 'accepte' else refused(code, out, run_dir)
        first = (out.strip().splitlines() or err.strip().splitlines() or [''])[0][:150]
        self.check(name, ok, '%s attendu, code=%d : %s' % (want, code, first))


def mini_cases(g):
    specs, digest = oracle_plan(MINI_PLAN)
    prereg = dict(id='PREREG_PORTE_COMPLETUDE', attribution_statement='Fixture de la porte de completude.',
                  plan=dict(MINI_PLAN, manifest_sha256=digest), methods=[dict(name=m) for m in METHODS],
                  decision=dict(alpha=0.05, delta_min=0.02, permutations=8, bootstrap=8, refusal_cap=0.01,
                                pairs=[dict(name='tour_contre_hdb', method='tour', adversary='hdb')]))
    ppath = os.path.join(g.tmp, 'PREREG_PORTE.json')
    with open(ppath, 'w') as f:
        json.dump(prereg, f, indent=1, sort_keys=True)
    psha = sha256_file(ppath)
    info = dict(prereg='PREREG_PORTE.json', prereg_sha256=psha, plan_sha256=digest, scenes=len(specs))
    rows = as_text(mini_rows(specs))
    units = [unit_of(s) for s in specs]
    g.check('plan_dev_de_la_porte', len(specs) == 32 and len(rows) == 96, '%d scenes, %d lignes' % (len(specs), len(rows)))

    def lot(name, lot_rows, **over):
        path = os.path.join(g.tmp, name)
        write_lot(path, lot_rows, dict(info, **over))
        return path

    def edit(index, **fields):
        out = [dict(r) for r in rows]
        out[index].update(fields)
        return out

    g.decide('complet', ppath, lot('complet', rows), 'accepte')
    g.decide('complet_fusion_declare', ppath, lot('complet_fusion', rows, complete=True, computed=32), 'accepte')
    one = [r for r in rows if r['unit'] == units[0]]
    g.decide('sonde_audit_une_scene', ppath, lot('une_scene', one, complete=False, computed=1), 'refuse')
    g.decide('sonde_audit_une_scene_decision', ppath, lot('une_scene_d', one, complete=False, computed=1), 'refuse',
             check_only=False)
    without = [r for r in rows if r['unit'] != units[5]]
    g.decide('scene_manquante', ppath, lot('scene_manquante', without), 'refuse')
    g.decide('scene_manquante_decision', ppath, lot('scene_manquante_d', without), 'refuse', check_only=False)
    g.decide('couple_manquant', ppath,
             lot('couple_manquant', [r for r in rows if (r['unit'], r['method']) != (units[3], 'hdb')]), 'refuse')
    twin = dict([r for r in rows if (r['unit'], r['method']) == (units[2], 'tour')][0], ari_s='0.999999')
    g.decide('doublon_autre_valeur', ppath, lot('doublon', rows + [twin]), 'refuse')
    outside = unit_of(dict(specs[7], seed=1))
    g.decide('scene_hors_plan', ppath,
             lot('hors_plan', [dict(r, unit=outside) if r['unit'] == units[7] else r for r in rows]), 'refuse')
    g.decide('scene_hors_plan_en_plus', ppath,
             lot('hors_plan_plus', rows + [dict(r, unit=outside) for r in rows if r['unit'] == units[7]]), 'refuse')
    g.decide('methode_hors_plan', ppath, lot('methode_hors_plan', edit(4, method='hdb_bis')), 'refuse')
    g.decide('metadonnee_famille', ppath, lot('famille', edit(0, family='hierarchical')), 'refuse')
    noisy = [i for i, r in enumerate(rows) if r['noise'] == '0.1'][0]
    g.decide('metadonnee_bruit_chaine', ppath, lot('bruit', edit(noisy, noise='0.10')), 'refuse')
    g.decide('metadonnee_graine', ppath, lot('graine', edit(9, seed=str(int(rows[9]['seed']) + 1))), 'refuse')
    g.decide('score_non_fini', ppath, lot('nan', edit(1, ari_s='nan')), 'refuse')
    g.decide('refus_hors_domaine', ppath, lot('refus2', edit(1, refused='2')), 'refuse')
    path = os.path.join(g.tmp, 'colonne')
    write_lot(path, rows, info, fields=tuple(c for c in COLUMNS if c != 'ami_nc'))
    g.decide('colonne_absente', ppath, path, 'refuse')
    g.decide('run_declare_incomplet', ppath, lot('declare_incomplet', rows, complete=False), 'refuse')
    g.decide('run_autre_plan', ppath, lot('autre_plan', rows, plan_sha256='0' * 64), 'refuse')
    g.decide('run_autre_nombre_de_scenes', ppath, lot('autre_nombre', rows, scenes=31), 'refuse')
    g.decide('autre_preenregistrement', ppath, lot('autre_prereg', rows, prereg_sha256='1' * 64), 'refuse')
    altered = dict(prereg, plan=dict(prereg['plan'], replicates=3))  # meme epingle, plan different
    apath = os.path.join(g.tmp, 'PREREG_ALTERE.json')
    with open(apath, 'w') as f:
        json.dump(altered, f, indent=1, sort_keys=True)
    g.decide('plan_altere_sous_la_meme_epingle', apath, lot('plan_altere', rows, prereg_sha256=sha256_file(apath)),
             'refuse')
    return ppath, psha, digest, specs, rows, units


def merge_cases(g, ppath, psha, digest, specs, rows, units):
    info = dict(prereg='PREREG_PORTE.json', prereg_sha256=psha, plan_sha256=digest, scenes=len(specs))
    half = set(units[:16])

    def session(name, lot_rows, **over):
        path = os.path.join(g.tmp, 'sessions', name)
        write_lot(path, lot_rows, dict(info, **over))
        return path

    s1 = session('s1', [r for r in rows if r['unit'] in half])
    s2 = session('s2', [r for r in rows if r['unit'] not in half])
    outside = unit_of(dict(specs[20], seed=1))
    sx = session('hors_plan', [dict(r, unit=outside) if r['unit'] == units[20] else r for r in rows
                               if r['unit'] not in half])
    sp = session('autre_plan', [r for r in rows if r['unit'] not in half], plan_sha256='0' * 64)

    def merge(name, sessions):
        out = os.path.join(g.tmp, 'fusion', name)
        code, stdout, _ = run(MERGE, ['--prereg', ppath, '--out', out] + sessions)
        merged = None
        if os.path.exists(os.path.join(out, 'run.json')):
            with open(os.path.join(out, 'run.json')) as f:
                merged = json.load(f)
        return out, code, stdout, merged

    _, code, out, _ = merge('hors_plan', [s1, sx])
    g.check('fusion_scene_hors_plan', refused(code, out), 'code=%d : %s' % (code, out.strip()[:120]))
    _, code, out, _ = merge('autre_plan', [s1, sp])
    g.check('fusion_autre_plan', refused(code, out), 'code=%d : %s' % (code, out.strip()[:120]))
    _, code, out, _ = merge('doublon', [s1, s1])
    g.check('fusion_scene_deux_fois', refused(code, out), 'code=%d : %s' % (code, out.strip()[:120]))
    path, code, out, merged = merge('partielle', [s1])
    g.check('fusion_partielle_ecrite', code == 0 and merged is not None and merged.get('complete') is False and
            merged.get('computed') == 16 and merged.get('scenes') == 32,
            'code=%d complete=%s' % (code, None if merged is None else merged.get('complete')))
    g.decide('fusion_partielle_refusee', ppath, path, 'refuse')
    path, code, out, merged = merge('complete', [s1, s2])
    g.check('fusion_complete_ecrite', code == 0 and merged is not None and merged.get('complete') is True and
            merged.get('computed') == 32, 'code=%d complete=%s' % (code, None if merged is None else merged.get('complete')))
    g.decide('fusion_complete_acceptee', ppath, path, 'accepte')


def archived_cases(g):
    digests = []
    for label, prereg_name, receipt, results in LOTS:
        ppath = os.path.join(PREREG_DIR, prereg_name)
        with open(ppath) as f:
            prereg = json.load(f)
        specs, digest = oracle_plan(prereg['plan'])
        digests.append(digest == prereg['plan']['manifest_sha256'])
        src = os.path.join(V10, 'receipts', receipt)
        path = os.path.join(g.tmp, label)
        os.makedirs(path)
        shutil.copy(os.path.join(src, 'run.json'), path)
        opener = gzip.open if results.endswith('.gz') else open
        with opener(os.path.join(src, results), 'rt', newline='') as fin, \
                open(os.path.join(path, 'results.csv'), 'w', newline='') as fout:
            fout.write(fin.read())
        g.decide('%s_archive' % label, ppath, path, 'accepte')
        if label != 'lot_C':
            continue
        with open(os.path.join(path, 'results.csv'), newline='') as f:
            reader = csv.DictReader(f)
            fields, rows = reader.fieldnames, list(reader)
        with open(os.path.join(path, 'run.json')) as f:
            info = json.load(f)
        cut = os.path.join(g.tmp, 'lot_C_moins_une_ligne')
        write_lot(cut, rows[:-1], info, fields=fields)
        g.decide('lot_C_moins_une_ligne', ppath, cut, 'refuse')
        probe = os.path.join(g.tmp, 'lot_C_sonde_audit')  # evidence_bench_completeness.py : une scene, incomplete
        write_lot(probe, [r for r in rows if r['unit'] == rows[0]['unit']],
                  dict(prereg_sha256=info['prereg_sha256'], plan_sha256=info['plan_sha256'], scenes=len(specs),
                       computed=1, complete=False), fields=fields)
        g.decide('lot_C_sonde_audit_decision', ppath, probe, 'refuse', check_only=False)
    g.check('oracle_epingles_A_C', digests == [True, True], 'manifestes reconstruits = epingles : %s' % digests)


def main():
    if len(sys.argv) != 2:
        print('usage : test_decide_completeness.py <dossier de build>')
        return 2
    with tempfile.TemporaryDirectory(prefix='mhgp10-decide-porte-') as tmp:
        g = Gate(tmp)
        for step in (lambda: merge_cases(g, *mini_cases(g)), lambda: archived_cases(g)):
            try:
                step()
            except Exception as exc:  # une API absente ou cassee est un echec, pas un crash de la porte
                g.check('exception', False, repr(exc))
    failures = g.results.count(False)
    print('decide_completeness_ok' if g.results and not failures else 'ECHECS %d' % failures)
    return 1 if failures or not g.results else 0


if __name__ == '__main__':
    sys.exit(main())

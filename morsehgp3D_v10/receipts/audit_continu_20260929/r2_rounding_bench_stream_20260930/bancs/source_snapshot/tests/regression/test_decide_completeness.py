"""Regression (29 septembre 2026, audit independant TETE_BANCS_PREUVES E1 ; completee le 30 septembre : verificateur
des bancs P3 a P7, contre-audits CONTRE_AUDIT_BANCS_CORRIGES_20260929 et CONTRE_AUDIT_TETE_BANCS) : decide.py decidait
sur un lot incomplet (une scene sur 960, `complete=false`, code 0) et merge_sessions.py acceptait une scene hors du
plan. La porte batit un petit plan preenregistre sur l'espace `dev` (32 scenes, 3 methodes), ses variantes, et exige :
  - lot complet -> accepte ; lots A et C archives (recus immuables) -> acceptes par --check-only ;
  - chaque mutation isolee -> code 2, une ligne REFUS, aucun DECISION.json : scene manquante, couple manquant, doublon
    (autre valeur, ou scene entiere recopiee comme la garderait run_test.py --resume), scene hors plan (remplacante ou
    en plus), methode hors plan (renommee, ou ligne ajoutee sans retirer de couple connu), metadonnee fausse, score non
    fini, refus hors {0, 1}, colonne absente, run.json d'un autre plan, d'un autre nombre de scenes ou declare
    incomplet, autre preenregistrement, plan altere sous l'epingle d'origine (autre nombre de scenes, et meme nombre :
    seule la comparaison a l'epingle l'arrete), plan aux specifications dupliquees (lot dedoublonne ou lot tel que
    run_test.py l'ecrirait) ; ARI_s hors [-1/2, 1] ou AMI_nc > 1 sur une ligne non refusee, au-dela de la tolerance
    declaree 1e-9 (1,25 du contre-audit ; 1,000001 ; -0,75 ; AMI 1,5) ; preenregistrement, run.json ou results.csv
    absent ou illisible, run.json qui n'est pas un objet ;
  - valeurs valides acceptees : bornes exactes (ARI_s 1 et -1/2, AMI 1), tolerance declaree, scores negatifs, temoin
    refused=1 a NaN (score substitue par zero, EVAL_v2 D8) ;
  - fusion : scene hors plan, deux plans, session unique sous un autre plan (le controle entre sessions ne la voit
    pas), epingle fausse, plan aux specifications dupliquees, scene calculee deux fois, scene incomplete, run.json ou
    results.csv de session absent, colonne absente -> 2 et rien d'ecrit ; fusion partielle -> 0 mais complete=false, puis decide
    refuse ; sessions complementaires -> complete=true, puis decide accepte ; lancee SANS -B, la fusion n'ecrit aucun
    __pycache__ (temoin : un import ordinaire de decide en ecrit un dans le meme environnement).
Chaque lot est juge en Python nu (-S, --check-only : contrat de la VM sans numpy ; un refus precede donc tout import
de numpy) ET, quand numpy est importable, en decision complete : sur un ancien decide.py, l'echec est alors causal (une
decision ecrite sur un lot invalide), pas seulement l'echec d'un import. La porte reconstruit elle-meme les manifestes
(oracle independant, verifie sur les epingles des lots A et C) et ne genere aucune scene. Aucun assert : elle tient
sous python3 -O.

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
FULL_MIN = 40  # plancher : decisions completes jouees quand numpy est importable
# une decision complete n'utilise qu'un fil BLAS (machine partagee) ; sans effet sur les refus
ENV = dict(os.environ, OPENBLAS_NUM_THREADS='1', OMP_NUM_THREADS='1', MKL_NUM_THREADS='1')


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


def numpy_importable():
    try:
        r = subprocess.run([sys.executable, '-c', 'import numpy'], capture_output=True, timeout=300, env=ENV)
    except (OSError, subprocess.SubprocessError):
        return False
    return r.returncode == 0


def run(script, args, site=False):
    """Script du banc : Python nu (-S : ni site-packages ni numpy), ou avec site-packages (decision complete)."""
    r = subprocess.run([sys.executable, '-B'] + ([] if site else ['-S']) + [script] + args, capture_output=True,
                       text=True, timeout=600, env=ENV)
    return r.returncode, r.stdout, r.stderr


def first(out, err):
    return (out.strip().splitlines() or err.strip().splitlines() or [''])[0][:150]


def no_decision(run_dir):
    return run_dir is None or not any(os.path.exists(os.path.join(run_dir, f)) for f in ('DECISION.json', 'DECISION.md'))


def refused(code, out, run_dir=None):
    return code == 2 and any(line.startswith('REFUS') for line in out.splitlines()) and no_decision(run_dir)


def accepted(code, out, run_dir):
    return (code == 0 and any(line.startswith('lot_conforme_au_plan') for line in out.splitlines()) and
            no_decision(run_dir))


def decided(code, run_dir, scenes):
    try:
        with open(os.path.join(run_dir, 'DECISION.json')) as f:
            decision = json.load(f)
    except (OSError, ValueError):
        return False
    return code == 0 and decision.get('scenes') == scenes and os.path.exists(os.path.join(run_dir, 'DECISION.md'))


def write_json(path, value):
    with open(path, 'w') as f:
        json.dump(value, f, indent=1, sort_keys=True)
    return path


def write_lot(path, rows, info, fields=COLUMNS):
    os.makedirs(path)
    write_json(os.path.join(path, 'run.json'), info)
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
    def __init__(self, tmp, numpy):
        self.tmp = tmp
        self.numpy = numpy
        self.full = 0
        self.results = []

    def check(self, name, ok, detail=''):
        self.results.append(ok)
        print('%-42s %s  %s' % (name, 'ok' if ok else 'ECHEC', detail), flush=True)

    def decide(self, name, prereg, run_dir, want, check_only=True, full=True, scenes=32):
        """Mode 1, Python nu (-S) : --check-only, ou decision complete si check_only=False (le refus doit alors
        preceder l'import de numpy). Mode 2, si numpy est importable et full : decision complete avec site-packages.
        Refus : code 2, ligne REFUS, aucune decision, dans chaque mode. Acceptation : lot_conforme_au_plan sans
        decision en mode 1, puis DECISION.json sur `scenes` scenes en mode 2."""
        args = ['--prereg', prereg, '--run', run_dir]
        code, out, err = run(DECIDE, args + (['--check-only'] if check_only else []))
        ok = accepted(code, out, run_dir) if want == 'accepte' else refused(code, out, run_dir)
        detail = '%s attendu ; nu code=%d : %s' % (want, code, first(out, err))
        if self.numpy and full:
            code, out, err = run(DECIDE, args, site=True)
            self.full += 1
            ok = ok and (decided(code, run_dir, scenes) if want == 'accepte' else refused(code, out, run_dir))
            detail += ' ; numpy code=%d %s : %s' % (code, 'sans decision' if no_decision(run_dir) else 'DECISION ECRITE',
                                                     first(out, err))
        self.check(name, ok, detail)


def mini_cases(g):
    specs, digest = oracle_plan(MINI_PLAN)
    prereg = dict(id='PREREG_PORTE_COMPLETUDE', attribution_statement='Fixture de la porte de completude.',
                  plan=dict(MINI_PLAN, manifest_sha256=digest), methods=[dict(name=m) for m in METHODS],
                  decision=dict(alpha=0.05, delta_min=0.02, permutations=8, bootstrap=8, refusal_cap=0.01,
                                pairs=[dict(name='tour_contre_hdb', method='tour', adversary='hdb')]))
    ppath = write_json(os.path.join(g.tmp, 'PREREG_PORTE.json'), prereg)
    psha = sha256_file(ppath)
    info = dict(prereg='PREREG_PORTE.json', prereg_sha256=psha, plan_sha256=digest, scenes=len(specs))
    rows = as_text(mini_rows(specs))
    units = [unit_of(s) for s in specs]
    valid = [i for i, r in enumerate(rows) if r['refused'] == '0']
    witness = [i for i, r in enumerate(rows) if r['refused'] == '1']
    g.check('plan_dev_de_la_porte', len(specs) == 32 and len(set(units)) == 32 and len(rows) == 96 and
            len(witness) == 1, '%d scenes, %d lignes, %d refus' % (len(specs), len(rows), len(witness)))

    def lot(name, lot_rows, fields=COLUMNS, **over):
        path = os.path.join(g.tmp, name)
        write_lot(path, lot_rows, dict(info, **over), fields=fields)
        return path

    def edit(changes):
        out = [dict(r) for r in rows]
        for index, fields in changes.items():
            out[index].update(fields)
        return out

    # lots valides
    g.decide('complet', ppath, lot('complet', rows), 'accepte')
    g.decide('complet_fusion_declare', ppath, lot('complet_fusion', rows, complete=True, computed=32), 'accepte')
    # incompletude et doublons (E1)
    one = [r for r in rows if r['unit'] == units[0]]
    g.decide('sonde_audit_une_scene', ppath, lot('une_scene', one, complete=False, computed=1), 'refuse')
    g.decide('sonde_audit_une_scene_decision_nu', ppath, lot('une_scene_d', one, complete=False, computed=1),
             'refuse', check_only=False)
    without = [r for r in rows if r['unit'] != units[5]]
    g.decide('scene_manquante', ppath, lot('scene_manquante', without), 'refuse')
    g.decide('scene_manquante_decision_nu', ppath, lot('scene_manquante_d', without), 'refuse', check_only=False)
    g.decide('couple_manquant', ppath,
             lot('couple_manquant', [r for r in rows if (r['unit'], r['method']) != (units[3], 'hdb')]), 'refuse')
    twin = dict([r for r in rows if (r['unit'], r['method']) == (units[2], 'tour')][0], ari_s='0.999999')
    g.decide('doublon_autre_valeur', ppath, lot('doublon', rows + [twin]), 'refuse')
    # run_test.py --resume garde toute scene complete en methodes, recopiee ou hors plan (P6) : lot final refuse
    g.decide('reprise_scene_entiere_recopiee', ppath,
             lot('reprise_doublon', rows + [dict(r) for r in rows if r['unit'] == units[11]]), 'refuse')
    outside = unit_of(dict(specs[7], seed=1))
    g.decide('scene_hors_plan_remplacante', ppath,
             lot('hors_plan', [dict(r, unit=outside) if r['unit'] == units[7] else r for r in rows]), 'refuse')
    g.decide('reprise_scene_hors_plan_en_plus', ppath,
             lot('hors_plan_plus', rows + [dict(r, unit=outside) for r in rows if r['unit'] == units[7]]), 'refuse')
    # methodes : renommee (retire un couple connu) ; ajoutee sans rien retirer (seul le controle des methodes, M10)
    g.decide('methode_hors_plan_renommee', ppath, lot('methode_renommee', edit({4: dict(method='hdb_bis')})),
             'refuse')
    g.decide('methode_inconnue_ajoutee', ppath, lot('methode_ajoutee', rows + [dict(rows[0], method='inconnue')]),
             'refuse')
    g.decide('metadonnee_famille', ppath, lot('famille', edit({0: dict(family='hierarchical')})), 'refuse')
    noisy = [i for i, r in enumerate(rows) if r['noise'] == '0.1'][0]
    g.decide('metadonnee_bruit_chaine', ppath, lot('bruit', edit({noisy: dict(noise='0.10')})), 'refuse')
    g.decide('metadonnee_graine', ppath, lot('graine', edit({9: dict(seed=str(int(rows[9]['seed']) + 1))})), 'refuse')
    g.decide('score_non_fini', ppath, lot('nan', edit({valid[0]: dict(ari_s='nan')})), 'refuse')
    g.decide('refus_hors_domaine', ppath, lot('refus2', edit({valid[0]: dict(refused='2')})), 'refuse')
    g.decide('colonne_absente', ppath, lot('colonne', rows, fields=tuple(c for c in COLUMNS if c != 'ami_nc')),
             'refuse')
    g.decide('run_declare_incomplet', ppath, lot('declare_incomplet', rows, complete=False), 'refuse')
    g.decide('run_autre_plan', ppath, lot('autre_plan', rows, plan_sha256='0' * 64), 'refuse')
    g.decide('run_autre_nombre_de_scenes', ppath, lot('autre_nombre', rows, scenes=31), 'refuse')
    g.decide('autre_preenregistrement', ppath, lot('autre_prereg', rows, prereg_sha256='1' * 64), 'refuse')
    # plan altere, epingle d'origine gardee : autre nombre de scenes (48), puis MEME nombre avec un lot coherent avec
    # le plan altere, qui declare l'epingle : seule la comparaison du manifeste reconstruit a l'epingle l'arrete (M09)
    altered = dict(prereg, plan=dict(prereg['plan'], replicates=3))
    apath = write_json(os.path.join(g.tmp, 'PREREG_ALTERE.json'), altered)
    g.decide('plan_altere_autre_nombre', apath, lot('plan_altere', rows, prereg_sha256=sha256_file(apath)), 'refuse')
    same_plan = dict(MINI_PLAN, sizes=[8000, 32000])
    same_specs, same_digest = oracle_plan(same_plan)
    same = dict(prereg, plan=dict(same_plan, manifest_sha256=digest))
    spath = write_json(os.path.join(g.tmp, 'PREREG_ALTERE_MEME_NOMBRE.json'), same)
    g.check('plan_altere_meme_nombre_fixture', len(same_specs) == len(specs) and same_digest != digest and
            set(unit_of(s) for s in same_specs) != set(units),
            '%d scenes, manifeste %s au lieu de l epingle %s' % (len(same_specs), same_digest[:12], digest[:12]))
    same_rows = as_text(mini_rows(same_specs))
    g.decide('plan_altere_meme_nombre_sous_l_epingle', spath,
             lot('plan_altere_meme', same_rows, prereg_sha256=sha256_file(spath)), 'refuse')
    # domaine des scores lus par la decision, lignes non refusees (P4) ; tolerance declaree 1e-9
    g.decide('ari_s_1_25_non_refuse', ppath,
             lot('ari_125', [dict(r, ari_s='1.25') if r['method'] == 'tour' else r for r in rows]), 'refuse')
    g.decide('ari_s_au_dela_de_la_tolerance', ppath, lot('ari_1000001', edit({valid[1]: dict(ari_s='1.000001')})),
             'refuse')
    g.decide('ari_s_sous_moins_un_demi', ppath, lot('ari_moins', edit({valid[2]: dict(ari_s='-0.75')})), 'refuse')
    g.decide('ami_au_dessus_de_un', ppath, lot('ami_15', edit({valid[3]: dict(ami_nc='1.5')})), 'refuse')
    g.decide('bornes_exactes_acceptees', ppath,
             lot('bornes', edit({valid[1]: dict(ari_s='1'), valid[2]: dict(ari_s='-0.5'),
                                 valid[3]: dict(ami_nc='1.0')})), 'accepte')
    g.decide('tolerance_declaree_acceptee', ppath,
             lot('tolerance', edit({valid[1]: dict(ari_s='1.0000000005'), valid[2]: dict(ari_s='-0.5000000005'),
                                    valid[3]: dict(ami_nc='1.0000000005')})), 'accepte')
    g.decide('scores_negatifs_valides', ppath,
             lot('negatifs', edit({valid[1]: dict(ari_s='-0.2'), valid[2]: dict(ami_nc='-0.3')})), 'accepte')
    g.decide('temoin_refuse_nan_admis', ppath, lot('temoin_nan', edit({witness[0]: dict(ari_s='nan', ami_nc='nan')})),
             'accepte')
    # fichiers absents ou illisibles (P7) : refus explicite, jamais un plantage
    path = lot('results_absent', rows)
    os.remove(os.path.join(path, 'results.csv'))
    g.decide('results_csv_absent', ppath, path, 'refuse')
    path = lot('run_json_absent', rows)
    os.remove(os.path.join(path, 'run.json'))
    g.decide('run_json_absent', ppath, path, 'refuse')
    path = lot('run_json_illisible', rows)
    with open(os.path.join(path, 'run.json'), 'w') as f:
        f.write('{ pas du JSON')
    g.decide('run_json_illisible', ppath, path, 'refuse')
    path = lot('run_json_non_objet', rows)
    write_json(os.path.join(path, 'run.json'), [info])
    g.decide('run_json_non_objet', ppath, path, 'refuse')
    g.decide('preenregistrement_absent', os.path.join(g.tmp, 'PREREG_ABSENT.json'), lot('prereg_absent', rows),
             'refuse')
    # plan aux specifications dupliquees (P7) : refus explicite, que le lot soit dedoublonne ou ecrit par run_test.py
    dup_plan = dict(MINI_PLAN, sizes=[8000, 8000])
    dup_specs, dup_digest = oracle_plan(dup_plan)
    dpath = write_json(os.path.join(g.tmp, 'PREREG_DUPLIQUE.json'),
                       dict(prereg, plan=dict(dup_plan, manifest_sha256=dup_digest)))
    unique = []
    for s in dup_specs:
        if unit_of(s) not in [unit_of(u) for u in unique]:
            unique.append(s)
    g.check('plan_duplique_fixture', len(dup_specs) == 32 and len(unique) == 16, '%d specifications, %d noms' % (
        len(dup_specs), len(unique)))
    dinfo = dict(prereg_sha256=sha256_file(dpath), plan_sha256=dup_digest, scenes=len(dup_specs))
    g.decide('plan_duplique_lot_dedoublonne', dpath, lot('plan_duplique', as_text(mini_rows(unique)), **dinfo),
             'refuse')
    g.decide('plan_duplique_lot_de_run_test', dpath, lot('plan_duplique_rt', as_text(mini_rows(dup_specs)), **dinfo),
             'refuse')
    return dict(ppath=ppath, psha=psha, digest=digest, specs=specs, rows=rows, units=units, dpath=dpath,
                dinfo=dinfo, dup_rows=as_text(mini_rows(unique)))


def merge_cases(g, m):
    ppath, psha, digest, specs, rows, units = m['ppath'], m['psha'], m['digest'], m['specs'], m['rows'], m['units']
    info = dict(prereg='PREREG_PORTE.json', prereg_sha256=psha, plan_sha256=digest, scenes=len(specs))
    half = set(units[:16])

    def session(name, lot_rows, fields=COLUMNS, **over):
        path = os.path.join(g.tmp, 'sessions', name)
        write_lot(path, lot_rows, dict(info, **over), fields=fields)
        return path

    def merge(name, sessions, prereg=ppath):
        out = os.path.join(g.tmp, 'fusion', name)
        code, stdout, _ = run(MERGE, ['--prereg', prereg, '--out', out] + sessions)
        merged = None
        if os.path.exists(os.path.join(out, 'run.json')):
            with open(os.path.join(out, 'run.json')) as f:
                merged = json.load(f)
        return out, code, stdout, merged

    def refuse(name, sessions, prereg=ppath):
        out, code, stdout, _ = merge(name, sessions, prereg)
        g.check('fusion_' + name, refused(code, stdout) and not os.path.exists(out),
                'code=%d ecrit=%s : %s' % (code, os.path.exists(out), stdout.strip()[:120]))

    s1 = session('s1', [r for r in rows if r['unit'] in half])
    s2 = session('s2', [r for r in rows if r['unit'] not in half])
    outside = unit_of(dict(specs[20], seed=1))
    refuse('scene_hors_plan', [s1, session('hors_plan', [dict(r, unit=outside) if r['unit'] == units[20] else r
                                                          for r in rows if r['unit'] not in half])])
    refuse('deux_plans', [s1, session('autre_plan', [r for r in rows if r['unit'] not in half],
                                      plan_sha256='0' * 64)])
    # une seule session, sous un autre plan : toutes les sessions sont d'accord entre elles (N03)
    refuse('session_unique_autre_plan', [session('autre_plan_seule', rows, plan_sha256='0' * 64)])
    # epingle fausse : le preenregistrement n'epingle pas son plan ; la session declare cette epingle (N04)
    with open(ppath) as f:
        wrong = json.load(f)
    wrong['plan']['manifest_sha256'] = 'f' * 64
    wpath = write_json(os.path.join(g.tmp, 'PREREG_EPINGLE_FAUSSE.json'), wrong)
    refuse('epingle_fausse', [session('epingle_fausse', rows, prereg_sha256=sha256_file(wpath),
                                      plan_sha256='f' * 64)], prereg=wpath)
    refuse('plan_duplique', [session('plan_duplique', m['dup_rows'], **m['dinfo'])], prereg=m['dpath'])
    refuse('scene_deux_fois', [s1, s1])
    refuse('scene_incomplete', [session('scene_incomplete', [r for r in rows if r['unit'] in half and
                                                             (r['unit'], r['method']) != (units[0], 'hdb')])])
    path = session('sans_results', rows)
    os.remove(os.path.join(path, 'results.csv'))
    refuse('results_csv_absent', [path])
    path = session('sans_run_json', rows)
    os.remove(os.path.join(path, 'run.json'))
    refuse('run_json_absent', [path])
    refuse('colonne_methode_absente', [session('sans_methode', rows, fields=tuple(c for c in COLUMNS
                                                                                   if c != 'method'))])
    path, code, out, merged = merge('partielle', [s1])
    g.check('fusion_partielle_ecrite', code == 0 and merged is not None and merged.get('complete') is False and
            merged.get('computed') == 16 and merged.get('scenes') == 32,
            'code=%d complete=%s' % (code, None if merged is None else merged.get('complete')))
    g.decide('fusion_partielle_refusee', ppath, path, 'refuse')
    path, code, out, merged = merge('complete', [s1, s2])
    g.check('fusion_complete_ecrite', code == 0 and merged is not None and merged.get('complete') is True and
            merged.get('computed') == 32, 'code=%d complete=%s' % (code, None if merged is None else merged.get('complete')))
    g.decide('fusion_complete_acceptee', ppath, path, 'accepte')
    pycache_case(g, ppath, [s1, s2])


def pycache_case(g, ppath, sessions):
    """merge_sessions.py lance SANS -B ne doit pas ecrire bench/synthetic/__pycache__ (import de decide). Copie du
    banc dans un dossier jetable, variables de bytecode retirees ; temoin : un import ordinaire de decide y ecrit un
    __pycache__, preuve que l'environnement ecrit le bytecode (controle non vide)."""
    env = {k: v for k, v in ENV.items() if k not in ('PYTHONDONTWRITEBYTECODE', 'PYTHONPYCACHEPREFIX')}
    roots = {}
    for label in ('fusion', 'temoin'):
        root = os.path.join(g.tmp, 'pycache_' + label, 'bench')
        os.makedirs(os.path.join(root, 'g4'))
        os.makedirs(os.path.join(root, 'synthetic'))
        shutil.copy(MERGE, os.path.join(root, 'g4'))
        shutil.copy(DECIDE, os.path.join(root, 'synthetic'))
        shutil.copy(SCENES, os.path.join(root, 'synthetic'))
        roots[label] = root
    out = os.path.join(g.tmp, 'fusion', 'sans_B')
    r = subprocess.run([sys.executable, '-S', os.path.join(roots['fusion'], 'g4', 'merge_sessions.py'), '--prereg',
                        ppath, '--out', out] + sessions, capture_output=True, text=True, timeout=600, env=env)
    t = subprocess.run([sys.executable, '-S', '-c', 'import sys; sys.path.insert(0, sys.argv[1]); import decide',
                        os.path.join(roots['temoin'], 'synthetic')], capture_output=True, text=True, timeout=600,
                       env=env)
    caches = sorted(os.path.relpath(os.path.join(d, name), roots['fusion']) for d, names, _ in os.walk(roots['fusion'])
                    for name in names if name == '__pycache__')
    witness = os.path.isdir(os.path.join(roots['temoin'], 'synthetic', '__pycache__'))
    g.check('fusion_sans_B_aucun_pycache', r.returncode == 0 and t.returncode == 0 and witness and not caches,
            'fusion code=%d caches=%s ; temoin code=%d cache_ecrit=%s' % (r.returncode, caches, t.returncode, witness))


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
        g.decide('%s_archive' % label, ppath, path, 'accepte', full=False)  # decision complete : minutes (recu)
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
        g.decide('lot_C_sonde_audit_decision_nu', ppath, probe, 'refuse', check_only=False)
    g.check('oracle_epingles_A_C', digests == [True, True], 'manifestes reconstruits = epingles : %s' % digests)


def main():
    if len(sys.argv) != 2:
        print('usage : test_decide_completeness.py <dossier de build>')
        return 2
    numpy = numpy_importable()
    print('numpy importable : %s -> %s' % ('oui' if numpy else 'non', 'chaque lot aussi en decision complete' if numpy
                                           else '--check-only et refus en Python nu seulement (VM sans numpy)'),
          flush=True)
    with tempfile.TemporaryDirectory(prefix='mhgp10-decide-porte-') as tmp:
        g = Gate(tmp, numpy)
        for step in (lambda: merge_cases(g, mini_cases(g)), lambda: archived_cases(g)):
            try:
                step()
            except Exception as exc:  # une API absente ou cassee est un echec, pas un crash de la porte
                g.check('exception', False, repr(exc))
        if numpy:
            g.check('plancher_decisions_completes', g.full >= FULL_MIN, '%d jouees, plancher %d' % (g.full, FULL_MIN))
    failures = g.results.count(False)
    print('decide_completeness_ok' if g.results and not failures else 'ECHECS %d' % failures)
    return 1 if failures or not g.results else 0


if __name__ == '__main__':
    sys.exit(main())

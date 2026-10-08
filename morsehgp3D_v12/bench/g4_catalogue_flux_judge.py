#!/usr/bin/env python3
"""Juge de la session G4 de la tranche T2-d-C (transferts et publication du catalogue sur l'appareil), regle ecrite
d'avance (REGLE_T2D_C, docstring de g4_catalogue_flux.py, et RULE ci-dessous). Fonction pure du rapport : trois
verdicts, << adopte >>, << rejete >>, << refuse >>. Le juge relit les lignes NATIVES des sondes (le pilote les garde
telles quelles) et les lie a la commande : sequence des lignes, statuts et raisons, indices consecutifs, voie, profil,
K, feuille, fils et options, entiers u64 hors booleens, grand livre et diagnostics complets ; un champ absent n'est
jamais une mesure nulle (la sonde de la base, sans ligne << sorties >>, est une exception declaree : HISTORICAL).
Auto-test par injections : g4_catalogue_flux_selftest.py. Bibliotheque standard, Python 3.10 nu, aucun assert.
"""
import math
import random
import statistics
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from g4_catalogue_flux_lecteur import (BUDGET_OPEN_LINE, COORD_BITS, COUNTS, DEVICE_OPEN_LINE, LEAF,  # noqa: E402
                                       catalogue_spec, is_hex, is_int, read_catalogue, read_full)

FRAMES = ('ng00', 'ng01', 'ng02')
# Bras : avant (base, sonde historique), avant_bis (le meme binaire : A/A), apres, puis un bras par levier, chacun par
# substitution exacte d'une constante de apres (g4_catalogue_flux.py, SUBSTITUTIONS) ; noms = ce que le bras retire.
ARMS = ('avant', 'avant_bis', 'apres', 'sans_anticipation', 'sans_double_tampon', 'repli_cles_entieres',
        'flux_et_repli_selectif')
HISTORICAL = ('avant', 'avant_bis')  # sonde de la base : ni --sorties ni --digest-complet (exception declaree)
BUILT_ARMS = tuple(a for a in ARMS if a != 'avant_bis')
# Leviers (nom, bras de reference, bras mesure, trames decisives). flux : avant -> flux_et_repli_selectif, decisif sur
# ng00 et ng01 seulement (aucune chaine retriee : le repli selectif n'y agit pas) ; publie sur ng02, ou il porte aussi
# le repli selectif (positions compactees, ordre et cles des seules chaines). fenetres_du_repli : bornes des chaines
# lues par fenetres de 64 cles doublees contre toutes les cles par chaine (8 n octets) ; ng02 seule trame decisive qui
# retrie une chaine. L'ancien repli (verdicts, ordre et cles des n boules, 16 n octets) n'a pas de bras : il ne se
# mesure que dans le lot.
LEVERS = (('lot', 'avant', 'apres', FRAMES),
          ('flux', 'avant', 'flux_et_repli_selectif', ('ng00', 'ng01')),
          ('double_tampon', 'sans_double_tampon', 'apres', FRAMES),
          ('sorties_anticipees', 'sans_anticipation', 'apres', FRAMES),
          ('fenetres_du_repli', 'repli_cles_entieres', 'apres', ('ng02',)))
CONTROL = ('A/A', 'avant', 'avant_bis')
IDENTITY_CASES = (('ng00', 5), ('ng00', 10), ('ng01', 5), ('ng01', 10), ('ng02', 5), ('ng02', 10), ('u8000', 5),
                  ('u16000', 5), ('u32000', 5))
FUL1_CASES = IDENTITY_CASES
RULE = {'k': 5, 'rounds_min': 10, 'passes_min': 10, 'threads': 48, 'leaf': LEAF, 'coord_bits': COORD_BITS,
        'bootstrap': 10000, 'seed': 20261008, 'upper_bound': 1.0, 'aa_window': 0.015,
        'statistic': 'par processus : mediane des passes 2..P du wall_ns de la sonde du catalogue (voie appareil, '
                     'a chaud) ; par tour et par trame : rapport des deux bras ; moyenne geometrique des rapports et '
                     'IC '
                     '95 % par bootstrap sur les tours (random.Random(seed).choices, 10 000 tirages, rangs '
                     'int(0,025 B) et int(0,975 B) - 1)',
        'adoption': 'empreintes identiques partout (catalogue MHGP12DP, niveaux, table, grand livre, comptes, FUL1 ; '
                    'voie CPU = F2 ; FUL1 = session K) ET borne haute NON ARRONDIE de l\'IC du rapport apres/avant '
                    'sous 1 sur chacune de ng00, ng01, ng02 a K5',
        'aa': 'condition de validite de la mesure : moyenne geometrique NON ARRONDIE du rapport avant_bis/avant dans '
              '[1 - aa_window ; 1 + aa_window] sur chacune des trois trames ; sinon refuse (la session ne mesure pas a '
              '1,5 % pres : aucune decision)',
        'mutant': 'flux_sans_attente_appareil, sonde --device --digest --passes=2 : tue si code 0 avec deux '
                  'empreintes dont une differe de F2, ou code 3 dont la cause (statut invariant_violated et raison) '
                  'est lue, ou arret par signal (compte a part) ; survivant (deux empreintes egales a F2) : rejete ; '
                  'comparaison absente ou incomplete, code 2, delai ou sortie hors schema : refuse',
        'levers': [list(lever[:3]) + [list(lever[3])] for lever in LEVERS], 'control': list(CONTROL)}
# Empreintes de la voie CPU (session F2, receipts/g4_t1f_20261007, trame << probe >>) et FUL1 de la session K
# (receipts/g4_fullk_20261008 ; sources du produit inchangees depuis, sorties identiques).
F2_DIGESTS = {
    'ng00:5': '698efd3d46e2d9c24363dca26eb76d293d78ddb46e4594d1fcb16eef2f1dd977',
    'ng00:10': '0f92e4f1187d88a0d5bcd94b5814aad4f40a6dc76ed1e598bfffac441639eaf7',
    'ng01:5': '12e7684173191378e43dcf306b87259111fbea427c5381829a447a563ce4d309',
    'ng01:10': '2c0018b203c65feee260535b6ce6d904a75276c674daa45e27d217a01db445c3',
    'ng02:5': 'd0578a530bd4247b873fbb99efac3038f2b9eb3f9c52da3a2de9e44821046c7c',
    'ng02:10': 'aea71fe1eea83fd15a7d70dd6fd27d7c18bf320ed40e75b00b2d893cecefeca7',
    'u8000:5': 'b38e265772e407cc49d2efc5c51d6f4d5ddb94d3397bddeebac6c6b9da8c2230',
    'u16000:5': '3838851969c0ca2511fd84265127e53b32f89e5fffaa942c3b5402c18ee345cc',
    'u32000:5': '2fe412a5066e83d9ca223bc4ade93f45dcd7eaee6a6a2017d47db750d7e12561',
}
FUL1_SESSION_K = {
    'ng00:5': '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
    'ng01:5': '2d58a72a623ab293a05f30c215cd65fcb455419a0a118f6d301bb17600645e26',
    'ng02:5': '27d7650add157cdd582d55f50305f0b8f9319865c8ffd0737cf12b76ac2197f0',
    'ng00:10': 'e9e5f0bdb3e192c0b9f040576b0871ceb099457d43bdacc95ea5395ab23a6398',
    'ng01:10': 'f07ebf5b7aee58430bc693e41b5cf5fab6d05264e447740c9d5dcf388d578a78',
    'ng02:10': '8c9e3c5945e79694727f42ff629848d138d967a5237986e927fffdc53a166c1c',
}
# Etapes disjointes publiees (CST-0235 et T2-d) : leur somme tient dans le total de la passe. transferts : copies
# chronometrees apres l'attente des noyaux, et pour le flux de sortie le mur du flux moins ses consommateurs de
# publication (une partition du mur, pas une mesure du DMA) ; sorties : reservation anticipee et premier toucher
# (duree murale de l'hote, un noyau pouvant avancer pendant ce temps) ; absente de la sonde historique.
STAGES = {'parcours': ('traversal_ns',), 'feuilles': ('count_ns',), 'emission': ('fill_ns',),
          'fin_etage': ('levels_ns', 'sort_ns', 'assemble_ns', 'table_ns'), 'transferts': ('transfer_ns',),
          'sorties': ('outputs_ns',), 'publication': ('publish_ns',)}
PHYSICAL = ('leaves_narrow', 'leaves_medium', 'leaves_wide', 'leaves_exact', 'leaves_virtual_warp', 'leaves_rewritten',
            'max_leaf_span')


def physical_ok(cpu_row, dev_row):
    """Diagnostics physiques de la voie appareil comptes comme ceux de la voie CPU ; reprises par cause."""
    cd, dd, dv = cpu_row['diagnostics'], dev_row['diagnostics'], dev_row['device']
    if any(cd[f] != dd[f] for f in PHYSICAL):
        return False
    low, high = max(dv['replayed_wide'], dv['replayed_span']), dv['replayed_wide'] + dv['replayed_span']
    return dv['rewritten_device'] + dv['rewritten_host'] == dd['leaves_rewritten'] and \
        dv['replayed_wide'] == cd['leaves_virtual_warp'] and dv['replayed_span'] == cd['leaves_exact'] and \
        low <= dv['replayed_leaves'] <= high


def by_key(entries, *fields):
    """Entrees d'une etape indexees par leurs champs ; None si une entree est en double ou mal formee."""
    seen = {}
    for e in entries if type(entries) is list else []:
        if type(e) is not dict:
            return None
        key = tuple(e.get(f) for f in fields)
        if key in seen:
            return None
        seen[key] = e
    return seen


def check_identity(steps, out, threads):
    """Voie appareil (3 passes) = voie CPU (une passe), liees a leur commande ; voie CPU = F2. Rend les comptes de la
    voie CPU par cas (sites, boules, incidences, niveaux), qui lient ensuite les autres prises a leur entree."""
    refs, entries = {}, by_key(steps.get('identity'), 'case', 'k')
    if entries is None:
        out['refused'].append('identite : etape illisible ou cas en double')
        return refs
    for case, k in IDENTITY_CASES:
        key, e = '%s:%d' % (case, k), entries.get((case, k)) or {}
        cs, cr, cp = read_catalogue(e.get('cpu'), catalogue_spec('cpu', k, threads, 1, True, True))
        ds, dr, dp = read_catalogue(e.get('device'), catalogue_spec('device', k, threads, 3, True, True, True))
        if 'invariant' in (cs, ds):
            out['rejected'].append('identite : invariant viole ' + key)
            continue
        if cs != 'ok' or ds != 'ok':
            out['refused'].append('identite : %s (%s ; %s)' % (key, cr or cs, dr or ds))
            continue
        ref, digest, full = cp['passes'][0], cp['digests'][0]['catalogue_sha256'], cp['complets'][0]
        same = full['table_ecarts'] == 0 and all(d['catalogue_sha256'] == digest for d in dp['digests']) and all(
            c['niveaux_sha256'] == full['niveaux_sha256'] and c['table_sha256'] == full['table_sha256'] and
            c['table_ecarts'] == 0 for c in dp['complets']) and all(
            row[f] == ref[f] for row in dp['passes'] for f in COUNTS) and all(
            row['ledger'] == ref['ledger'] for row in dp['passes'])
        if not same:
            out['rejected'].append('identite en defaut ' + key)
        if not all(physical_ok(ref, row) for row in dp['passes']):
            out['rejected'].append('identite : diagnostics physiques ou reprises mal comptes ' + key)
        if digest != F2_DIGESTS[key]:
            out['rejected'].append('voie CPU differente de F2 ' + key)
        refs[key] = {f: ref[f] for f in COUNTS}
    return refs


def check_arms_identity(steps, out, threads, refs):
    """Chaque bras construit (avant compris) rend l'empreinte MHGP12DP de F2 sur ng00-02 a K5 (voie appareil, deux
    passes), avec les comptes de l'identite."""
    entries = by_key(steps.get('arms_identity'), 'arm', 'case')
    if entries is None:
        out['refused'].append('identite des bras : etape illisible ou prise en double')
        return
    for arm in BUILT_ARMS:
        for frame in FRAMES:
            state, why, parsed = read_catalogue((entries.get((arm, frame)) or {}).get('run'),
                                                catalogue_spec('device', 5, threads, 2, digest=True))
            if state != 'ok':
                out['rejected' if state == 'invariant' else 'refused'].append(
                    'identite des bras : %s %s (%s)' % (arm, frame, why or state))
                continue
            ref = refs.get(frame + ':5')
            if any(d['catalogue_sha256'] != F2_DIGESTS[frame + ':5'] for d in parsed['digests']) or ref is None or \
                    any(row[f] != ref[f] for row in parsed['passes'] for f in COUNTS):
                out['rejected'].append('identite des bras en defaut : %s %s' % (arm, frame))


def check_ful1(steps, out, threads, refs):
    """FUL1 (sonde FULL, voie appareil, deux passes) : stable, avant = apres sur chaque cas ; ng00-02 = session K ;
    sites de chaque passe = ceux de l'identite."""
    entries = by_key(steps.get('ful1'), 'arm', 'case', 'k')
    if entries is None:
        out['refused'].append('FUL1 : etape illisible ou prise en double')
        return
    for case, k in FUL1_CASES:
        key, found = '%s:%d' % (case, k), {}
        for arm in ('avant', 'apres'):
            state, why, digests, _, _ = read_full((entries.get((arm, case, k)) or {}).get('run'), case, k, threads, 2,
                                                  (refs.get(key) or {}).get('sites'))
            if state != 'ok':
                out['refused'].append('FUL1 absente ou hors commande : %s %s (%s)' % (arm, key, why))
                continue
            if len(set(digests)) != 1:
                out['rejected'].append('FUL1 instable : %s %s' % (arm, key))
            found[arm] = digests[0]
        if len(found) == 2 and found['avant'] != found['apres']:
            out['rejected'].append('FUL1 differente de la base : ' + key)
        if key in FUL1_SESSION_K and found.get('apres') not in (None, FUL1_SESSION_K[key]):
            out['rejected'].append('FUL1 differente de la session K : ' + key)


def stage_sum(arm, row, sorties):
    """Somme des etapes d'une passe, champs natifs ; la sonde historique n'a pas d'etape << sorties >>."""
    fields = {'transfer_ns': row['device']['transfer_ns'], 'publish_ns': row['device']['publish_ns']}
    fields.update({k: row['diagnostics'][k] for keys in STAGES.values() for k in keys if k in row['diagnostics']})
    if arm not in HISTORICAL:
        fields['outputs_ns'] = sorties['outputs_ns']
    return sum(fields[k] for name, keys in STAGES.items() for k in keys
               if not (name == 'sorties' and arm in HISTORICAL))


def closed_campaign(steps, rounds):
    """Cohorte exacte partagee par jugement et publication : aucune observation ignoree."""
    entries = steps.get('campaign') if type(steps) is dict else None
    if type(entries) is not list or not is_int(rounds):
        return None
    expected = {(r, f, a) for r in range(rounds) for f in FRAMES for a in ARMS}
    found = {}
    for entry in entries:
        if type(entry) is not dict:
            return None
        r, frame, arm = (entry.get(k) for k in ('round', 'frame', 'arm'))
        if not is_int(r) or type(frame) is not str or type(arm) is not str:
            return None
        key = (r, frame, arm)
        if key not in expected or key in found:
            return None
        found[key] = entry
    return found if len(found) == len(expected) else None


def campaign_table(steps, out, rounds, passes, threads, refs):
    """Medianes chaudes par (tour, trame, bras) ; refus si une prise manque, sort de sa commande ou si des etapes
    depassent le total d'une passe."""
    table, entries = {}, closed_campaign(steps, rounds)
    if entries is None:
        out['refused'].append('campagne illisible, incomplete, hors cohorte ou prise en double')
        return table
    for (r, frame, arm), e in entries.items():
        if arm not in ARMS or frame not in FRAMES or not is_int(r) or r >= rounds:
            continue
        spec = catalogue_spec('device', RULE['k'], threads, passes, sorties=arm not in HISTORICAL)
        state, why, parsed = read_catalogue(e.get('run'), spec)
        ref = refs.get(frame + ':5')
        if state != 'ok' or ref is None or any(row[f] != ref[f] for row in parsed['passes'] for f in COUNTS):
            out['refused'].append('prise hors commande : tour %s %s %s (%s)' % (r, frame, arm, why or state))
            continue
        sorties = parsed['sorties'] if arm not in HISTORICAL else [None] * passes
        if any(stage_sum(arm, row, s) > row['wall_ns'] for row, s in zip(parsed['passes'], sorties)):
            out['refused'].append('etapes non disjointes : tour %s %s %s' % (r, frame, arm))
            continue
        table[(r, frame, arm)] = statistics.median([row['wall_ns'] for row in parsed['passes'][1:]])
    for frame in FRAMES:
        for arm in ARMS:
            got = sum(1 for r in range(rounds) if (r, frame, arm) in table)
            if got < rounds:
                out['refused'].append('prise manquante : %s %s (%d tours sur %d)' % (frame, arm, got, rounds))
    return table


BOOTSTRAP_CACHE = {}


def bootstrap(logs):
    """Moyenne geometrique et IC 95 % (bornes NON arrondies) par bootstrap des tours."""
    key = tuple(logs)
    if key not in BOOTSTRAP_CACHE:
        rng, n, draws = random.Random(RULE['seed']), len(logs), RULE['bootstrap']
        boot = sorted(sum(rng.choices(logs, k=n)) / n for _ in range(draws))
        BOOTSTRAP_CACHE[key] = (math.exp(sum(logs) / n), math.exp(boot[int(0.025 * draws)]),
                                math.exp(boot[int(0.975 * draws) - 1]))
    return BOOTSTRAP_CACHE[key]


def ratios(table, rounds, frame, a, b):
    logs = []
    for r in range(rounds):
        x, y = table.get((r, frame, a)), table.get((r, frame, b))
        if x is None or y is None or x <= 0 or y <= 0:
            return None
        logs.append(math.log(y / x))
    return logs


def judge_levers(table, rounds, out):
    """Leviers et lot (borne haute non arrondie sous 1 sur chaque trame decisive) ; A/A : condition de validite."""
    levers = {}
    for name, a, b, frames in LEVERS + (CONTROL + (FRAMES,),):
        per = {}
        for frame in FRAMES:
            logs = ratios(table, rounds, frame, a, b)
            if logs is not None:
                gm, low, high = bootstrap(logs)
                per[frame] = {'gm': gm, 'low': low, 'high': high, 'decisive': frame in frames,
                              'affiche': '%.4f (%.4f-%.4f)' % (gm, low, high)}
        complete = all(f in per for f in frames)
        if name == 'A/A':
            held = complete and all(abs(per[f]['gm'] - 1.0) <= RULE['aa_window'] for f in frames)
        else:
            held = complete and all(per[f]['high'] < RULE['upper_bound'] for f in frames)
        levers[name] = {'from': a, 'to': b, 'frames': per,
                        'verdict': ('valide' if held else 'hors fenetre' if complete else None) if name == 'A/A' else
                        ('adopte' if held else 'rejete' if complete else None)}
    out['stats']['levers'] = levers
    if levers['A/A']['verdict'] == 'hors fenetre':
        out['refused'].append('A/A hors de la fenetre de 1,5 % : la session ne mesure pas assez finement')
    if levers['lot']['verdict'] == 'rejete':
        out['rejected'].append('lot : borne haute de l\'IC du rapport apres/avant pas sous 1 sur chaque trame')


def check_binaries(steps, out):
    before, after = steps.get('binaries') or {}, steps.get('binaries_after') or {}
    for arm in BUILT_ARMS:
        if not is_hex(before.get(arm)) or before.get(arm) != after.get(arm):
            out['refused'].append('binaire absent ou change pendant la campagne : ' + arm)


def check_mutant(steps, out, threads):
    """Mutant appareil (RULE['mutant']) : la comparaison doit exister ; l'absence de comparaison est un refus."""
    m = steps.get('mutant') or {}
    if m.get('applied') is not True or m.get('built') is not True:
        out['refused'].append('mutant appareil non juge (non applique ou non construit)')
        return
    state, why, parsed = read_catalogue(m.get('run'), catalogue_spec('device', RULE['k'], threads, 2, digest=True))
    if state == 'ok':
        reference = F2_DIGESTS['ng00:5']
        killed = any(d['catalogue_sha256'] != reference for d in parsed['digests'])
        out['stats']['mutant'] = 'tue (empreinte)' if killed else 'survivant'
        if not killed:
            out['rejected'].append('mutant appareil survivant (flux_sans_attente_appareil)')
    elif state in ('invariant', 'signal'):
        out['stats']['mutant'] = 'tue (%s)' % why
    else:
        out['refused'].append('mutant appareil sans comparaison (%s : %s)' % (state, why))


def unit_ok(run, marker):
    """Porte d'unite jouee hors CTest : code 0, une ligne marqueur exacte (texte ou expression)."""
    if type(run) is not dict or run.get('code') != 0 or run.get('timeout') is True or \
            type(run.get('stdout')) is not str:
        return False
    lines = run['stdout'].splitlines()
    return marker in lines if isinstance(marker, str) else any(marker.match(line) for line in lines)


def check_gates(steps, out):
    gates = steps.get('gates') or {}
    if not is_int(gates.get('code')) or gates.get('timeout') is not False:
        out['refused'].append('portes rapides non jouees')
    elif gates['code'] != 0:
        out['rejected'].append('portes rapides en echec (code %s)' % gates['code'])
    for name, marker in (('device_open', DEVICE_OPEN_LINE), ('device_open_budget', BUDGET_OPEN_LINE)):
        run = gates.get(name)
        if type(run) is dict and is_int(run.get('code')) and run['code'] != 0:
            out['rejected'].append('%s en echec (code %s)' % (name, run['code']))
        elif not unit_ok(run, marker):
            out['refused'].append('%s sans voie appareil jouee' % name)


def judge(report):
    out = {'refused': [], 'rejected': [], 'stats': {}}
    steps = report.get('steps') if isinstance(report, dict) else None
    opts = report.get('options') if isinstance(report, dict) else None
    if not isinstance(steps, dict) or not isinstance(opts, dict):
        return {'verdict': 'refuse', 'refused': ['rapport illisible'], 'rejected': [], 'stats': {}}
    env = steps.get('environment') or {}
    if not env.get('nvcc') or not env.get('gpu'):
        out['refused'].append('outil absent (nvcc ou GPU)')
    rounds, passes, threads = opts.get('rounds'), opts.get('passes'), opts.get('threads')
    if not is_int(rounds) or not is_int(passes) or rounds < RULE['rounds_min'] or passes < RULE['passes_min'] or \
            threads != RULE['threads']:
        out['refused'].append('contrat de mesure non respecte')
        rounds, passes, threads = RULE['rounds_min'], RULE['passes_min'], RULE['threads']
    builds = steps.get('builds') or {}
    for arm in BUILT_ARMS:
        if (builds.get(arm) or {}).get('ok') is not True:
            out['refused'].append('construction en echec ou absente : ' + arm)
    check_gates(steps, out)
    if steps.get('gpu_quiet_before') is not True or steps.get('gpu_quiet_after') is not True:
        out['refused'].append('GPU non isole avant ou apres les temps')
    refs = check_identity(steps, out, threads)
    check_arms_identity(steps, out, threads, refs)
    check_ful1(steps, out, threads, refs)
    check_binaries(steps, out)
    check_mutant(steps, out, threads)
    table = campaign_table(steps, out, rounds, passes, threads, refs)
    judge_levers(table, rounds, out)
    out['verdict'] = 'refuse' if out['refused'] else 'rejete' if out['rejected'] else 'adopte'
    return out

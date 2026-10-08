#!/usr/bin/env python3
"""Juge de la session G4 de la tranche T1-d (catalogue en flux), regle ecrite d'avance (REGLE_T1D, docstring de
g4_catalogue_t1d.py, et RULE ci-dessous). Fonction pure du rapport : << adopte >>, << rejete >>, << refuse >>. Relit les
lignes natives des sondes avec le lecteur strict (g4_catalogue_flux_lecteur.py, liaison a la commande) ; reprend du
juge T2-d-C l'identite (voie appareil = voie CPU = F2), FUL1 (= session K) et les portes de l'appareil.
Bibliotheque standard, Python 3.10 nu, aucun assert.
"""
import math
import statistics
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import g4_catalogue_flux_judge as J  # noqa: E402
import g4_catalogue_flux_lecteur as L  # noqa: E402

FRAMES = J.FRAMES
ARMS = ('avant', 'avant_bis', 'apres')
BUILT_ARMS = ('avant', 'apres')
# Budgets de l'appareil de la voie en flux : fractions des octets de l'appareil gardes par la voie complete (ligne
# << tranches >> d'une prise sans budget), pour chaque trame a K5 et K10. Comptes locaux du transit simule (8 octobre) :
# a 1/2, arene rapatriee a la fin et 3 tranches ; a 1/4, a K10, six ou sept lots rapatries un a un et 6 tranches ; a
# 1/8, refus memory_budget (front du parcours et cases d'un lot) ; d'ou des fractions serrees entre 1/2 et 1/8.
FRACTIONS = ((1, 2), (1, 3), (1, 4), (1, 6), (1, 8), (1, 32))
SLICE_CASES = tuple((f, k) for f in FRAMES for k in (5, 10))
RULE = {'k': 5, 'rounds_min': 10, 'passes_min': 10, 'threads': 48, 'bootstrap': 10000, 'seed': 20261008,
        'cost_bound': 1.01, 'aa_window': 0.015,
        'statistic': J.RULE['statistic'],
        'adoption': 'empreintes identiques partout (identite appareil = CPU = F2, voie en flux sous chaque budget de '
                    'l\'appareil joue, FUL1 avant = apres = session K, mutants et portes) ET borne haute NON ARRONDIE '
                    'de l\'IC 95 % du rapport apres/avant au plus 1,01 sur chacune de ng00, ng01, ng02 a K5 (le '
                    'contrat ne paie rien a 1 % pres)',
        'aa': J.RULE['aa'],
        'flux': 'pour chaque (trame, K) de SLICE_CASES : chaque prise sous budget de l\'appareil rend F2 ou le refus '
                'memory_budget ; au moins une prise identique avec au moins 2 tranches et un lot d\'arene rapatrie ; '
                'sur l\'ensemble des cas a K10, au moins une prise identique avec au moins 2 lots rapatries (arene en '
                'flux lot par lot sur l\'appareil reel)'}


def spec_campaign(threads, passes):
    return L.catalogue_spec('device', RULE['k'], threads, passes, sorties=True)


def spec_free(k, threads):
    return L.catalogue_spec('device', k, threads, 2, digest=True, tranches=True)


def spec_budget(k, threads, budget):
    return L.catalogue_spec('device', k, threads, 2, digest=True, tranches=True, device_budget=budget)


def exact_entries(entries, fields, expected=None):
    """Types des cles, unicite et ensemble ferme ; aucune observation ignoree."""
    if type(entries) is not list:
        return None
    for e in entries:
        if type(e) is not dict or any(
                not L.is_int(e.get(f)) if f in ('round', 'k', 'budget') else type(e.get(f)) is not str
                for f in fields):
            return None
    indexed = J.by_key(entries, *fields)
    if indexed is None or (expected is not None and set(indexed) != set(expected)):
        return None
    return indexed


def memory_ok(parsed, budget):
    """Pic appareil propre cumulatif ; sans budget propre, pic publie nul par contrat de la sonde."""
    previous = 0
    for row, t in zip(parsed['passes'], parsed['tranches']):
        if t['device_bytes'] != row['device']['device_bytes']:
            return False
        if budget:
            if not (t['device_bytes'] <= t['device_peak'] <= budget) or t['device_peak'] < previous:
                return False
        elif t['device_peak'] != 0:
            return False
        previous = t['device_peak']
    return True


def check_slices(steps, out, threads, refs):
    """Voie en flux sur l'appareil reel : budgets derives de la prise sans budget, F2 ou refus, tranches."""
    entries = exact_entries(steps.get('slices'), ('case', 'k', 'budget'))
    if entries is None:
        out['refused'].append('flux : etape illisible ou prise en double')
        return
    stats, streamed_k10 = {}, 0
    expected = {(case, k, 0) for case, k in SLICE_CASES}
    for case, k in SLICE_CASES:
        key, reference = '%s:%d' % (case, k), J.F2_DIGESTS['%s:%d' % (case, k)]
        state, why, free = L.read_catalogue((entries.get((case, k, 0)) or {}).get('run'), spec_free(k, threads))
        if state != 'ok' or any(d['catalogue_sha256'] != reference for d in free['digests']):
            out['refused' if state != 'ok' else 'rejected'].append('flux sans budget : %s (%s)' % (key, why or state))
            continue
        held = free['tranches'][-1]['device_bytes']
        ref = refs.get(key)
        if not memory_ok(free, 0) or not held or ref is None or any(
                row[f] != ref[f] for row in free['passes'] for f in L.COUNTS):
            out['refused'].append('flux sans budget : memoire ou comptes incoherents sur ' + key)
            continue
        budgets = [held * num // den for num, den in FRACTIONS]
        if 0 in budgets or len(set(budgets)) != len(budgets):
            out['refused'].append('flux : budgets non distincts ou nuls sur ' + key)
            continue
        expected.update((case, k, budget) for budget in budgets)
        best = 0
        for num, den in FRACTIONS:
            budget = held * num // den
            state, why, got = L.read_catalogue((entries.get((case, k, budget)) or {}).get('run'),
                                               spec_budget(k, threads, budget))
            if not memory_ok(got, budget):
                out['refused'].append('flux : budget appareil ou pic incoherent sur ' + key)
                continue
            # Une passe deja emise ne devient pas neutre si la suivante refuse.
            if state in ('ok', 'refus'):
                if any(row[f] != ref[f] for row in got['passes'] for f in L.COUNTS):
                    out['refused'].append('flux : comptes differents de l identite sur ' + key)
                    continue
                if any(d['catalogue_sha256'] != reference for d in got['digests']):
                    out['rejected'].append('flux en defaut : %s budget %d/%d' % (key, num, den))
                    continue
            if state == 'refus' and why == 'resource_exhausted/memory_budget':
                continue
            if state != 'ok':
                out['refused' if state != 'invariant' else 'rejected'].append(
                    'flux %s budget %d/%d : %s' % (key, num, den, why or state))
                continue
            t = got['tranches'][-1]
            if t['arena_streamed'] >= 1 and t['finish_slices'] >= 2:
                best = max(best, t['finish_slices'])
            if k == 10:
                streamed_k10 = max(streamed_k10, t['arena_streamed'])
        stats[key] = best
        if best < 2:
            out['rejected'].append('flux : tranches insuffisantes sur %s (%d)' % (key, best))
    stats['lots_rapatries_k10'] = streamed_k10
    if streamed_k10 < 2:
        out['rejected'].append('flux : aucune arene rapatriee lot par lot a K10 (%d)' % streamed_k10)
    if set(entries) != expected:
        out['refused'].append('flux : cohorte differente des budgets commandes')
    out['stats']['tranches'] = stats


def stage_total(row, sorties):
    d, dev = row['diagnostics'], row['device']
    return sum(d[k] for k in ('traversal_ns', 'count_ns', 'fill_ns', 'levels_ns', 'sort_ns', 'assemble_ns',
                              'table_ns')) + dev['transfer_ns'] + dev['publish_ns'] + sorties['outputs_ns']


def campaign_table(steps, out, rounds, passes, threads, refs):
    expected = {(r, f, a) for r in range(rounds) for f in FRAMES for a in ARMS}
    table, entries = {}, exact_entries(steps.get('campaign'), ('round', 'frame', 'arm'), expected)
    if entries is None:
        out['refused'].append('campagne illisible ou prise en double')
        return table
    for (r, frame, arm), e in entries.items():
        if arm not in ARMS or frame not in FRAMES or not L.is_int(r) or r >= rounds:
            continue
        state, why, parsed = L.read_catalogue(e.get('run'), spec_campaign(threads, passes))
        ref = refs.get(frame + ':5')
        if state != 'ok' or ref is None or any(row[f] != ref[f] for row in parsed['passes'] for f in L.COUNTS):
            out['refused'].append('prise hors commande : tour %s %s %s (%s)' % (r, frame, arm, why or state))
            continue
        if any(stage_total(row, s) > row['wall_ns'] for row, s in zip(parsed['passes'], parsed['sorties'])):
            out['refused'].append('etapes non disjointes : tour %s %s %s' % (r, frame, arm))
            continue
        table[(r, frame, arm)] = statistics.median([row['wall_ns'] for row in parsed['passes'][1:]])
    for frame in FRAMES:
        for arm in ARMS:
            got = sum(1 for r in range(rounds) if (r, frame, arm) in table)
            if got < rounds:
                out['refused'].append('prise manquante : %s %s (%d tours sur %d)' % (frame, arm, got, rounds))
    return table


def judge_cost(table, rounds, out):
    levers = {}
    for name, a, b in (('lot', 'avant', 'apres'), ('A/A', 'avant', 'avant_bis')):
        per = {}
        for frame in FRAMES:
            logs = J.ratios(table, rounds, frame, a, b)
            if logs is not None:
                gm, low, high = J.bootstrap(logs)
                per[frame] = {'gm': gm, 'low': low, 'high': high, 'affiche': '%.4f (%.4f-%.4f)' % (gm, low, high)}
        complete = all(f in per for f in FRAMES)
        if name == 'A/A':
            held = complete and all(abs(per[f]['gm'] - 1.0) <= RULE['aa_window'] for f in FRAMES)
            verdict = ('valide' if held else 'hors fenetre') if complete else None
        else:
            held = complete and all(per[f]['high'] <= RULE['cost_bound'] for f in FRAMES)
            verdict = ('ne paie rien' if held else 'paie') if complete else None
        levers[name] = {'from': a, 'to': b, 'frames': per, 'verdict': verdict}
    out['stats']['levers'] = levers
    if levers['A/A']['verdict'] == 'hors fenetre':
        out['refused'].append('A/A hors de la fenetre de 1,5 % : la session ne mesure pas assez finement')
    if levers['lot']['verdict'] == 'paie':
        out['rejected'].append('le contrat paie : borne haute du rapport apres/avant au-dessus de 1,01 sur une trame')


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
    if not L.is_int(rounds) or not L.is_int(passes) or rounds < RULE['rounds_min'] or passes < RULE['passes_min'] or \
            not L.is_int(threads) or threads != RULE['threads']:
        out['refused'].append('contrat de mesure non respecte')
        rounds, passes, threads = RULE['rounds_min'], RULE['passes_min'], RULE['threads']
    builds = steps.get('builds') or {}
    for arm in BUILT_ARMS:
        if (builds.get(arm) or {}).get('ok') is not True:
            out['refused'].append('construction en echec ou absente : ' + arm)
    before, after = steps.get('binaries') or {}, steps.get('binaries_after') or {}
    for arm in BUILT_ARMS:
        if not L.is_hex(before.get(arm)) or before.get(arm) != after.get(arm):
            out['refused'].append('binaire absent ou change pendant la campagne : ' + arm)
    J.check_gates(steps, out)
    if steps.get('gpu_quiet_before') is not True or steps.get('gpu_quiet_after') is not True:
        out['refused'].append('GPU non isole avant ou apres les temps')
    identity = exact_entries(steps.get('identity'), ('case', 'k'), J.IDENTITY_CASES)
    ful1 = exact_entries(steps.get('ful1'), ('arm', 'case', 'k'),
                         ((a, c, k) for a in BUILT_ARMS for c, k in J.FUL1_CASES))
    if identity is None or ful1 is None:
        out['refused'].append('identite ou FUL1 : cohorte non fermee')
        refs = {}
    else:
        refs = J.check_identity(steps, out, threads)
        J.check_ful1(steps, out, threads, refs)
    check_slices(steps, out, threads, refs)
    table = campaign_table(steps, out, rounds, passes, threads, refs)
    judge_cost(table, rounds, out)
    out['verdict'] = 'refuse' if out['refused'] else 'rejete' if out['rejected'] else 'adopte'
    return out


def ratio_check():
    """Borne non arrondie : 1,00999 tient, 1,01001 non (regle ecrite)."""
    return 1.00999 <= RULE['cost_bound'] < 1.01001 and math.isfinite(RULE['cost_bound'])

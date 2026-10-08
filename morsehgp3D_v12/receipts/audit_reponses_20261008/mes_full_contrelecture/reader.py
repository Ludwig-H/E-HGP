#!/usr/bin/env python3
"""Contre-lecteur MES-FULL K : aucune commande moteur, aucun lancement de campagne."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess

STAGES = ('P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R')
CAT = ('parcours', 'feuilles', 'emission', 'fin_etage', 'transferts', 'publication')
FRAMES = ('ng00', 'ng01', 'ng02')
BUDGET = 100_000_000
SHA = re.compile(r'[0-9a-f]{64}\Z')
U64 = (1 << 64) - 1


class Refusal(ValueError):
    pass


def require(ok, message):
    if not ok:
        raise Refusal(message)


def unique(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'cle JSON repetee')
        result[key] = value
    return result


def bad_constant(_):
    raise Refusal('constante JSON non finie')


def decode(data):
    try:
        return json.loads(data.decode('utf-8', 'strict'), object_pairs_hook=unique, parse_constant=bad_constant)
    except (UnicodeError, ValueError, RecursionError) as error:
        raise Refusal('JSON invalide : ' + str(error)) from error


def uint(value):
    return type(value) is int and 0 <= value <= U64


def keys(value, expected, label):
    require(type(value) is dict and set(value) == set(expected), label + ' : champs')


def integers(value, names, label):
    keys(value, names, label)
    require(all(uint(value[k]) for k in names), label + ' : u64 requis')


def admit_process(raw, spec, sites, threads=48):
    """Admet le flux seul. Un exit JSON ne fabrique pas de code de processus externe."""
    require(type(raw) is bytes and raw.isascii(), 'flux non ASCII natif')
    lines = raw.splitlines()
    require(lines and all(line.strip() for line in lines), 'ligne vide ou flux vide')
    rows = [decode(line) for line in lines]
    require(all(type(row) is dict for row in rows), 'ligne non objet')
    device, names, kmax = spec['device'], spec['names'], spec['kmax']
    resident_input_bytes = 16 * sum(sites[name] for name in set(names))
    require(len(rows) == 2 * len(names) + 1 + int(device), 'nombre de lignes')
    offset = int(device)
    if device:
        opened = rows[0]
        keys(opened, ('phase', 'status', 'reason', 'wall_ns'), 'open')
        require(opened['phase'] == 'open' and opened['status'] == 'ok' and opened['reason'] == 'none'
                and uint(opened['wall_ns']), 'ouverture appareil')
    require(rows[-1] == {'phase': 'exit', 'status': 'ok', 'reason': 'none'}, 'sortie incomplete/refusee')
    result = []
    for i, name in enumerate(names):
        row, release = rows[offset + 2*i:offset + 2*i + 2]
        keys(row, ('phase', 'pass', 'trame', 'voie', 'status', 'coord_bits', 'kmax', 'threads', 'sites',
                   'wall_ns', 'etapes_ns', 'c_ns', 'g_ns', 'hors_mur_ns', 'pic_octets', 'full_sha256'), 'full')
        require(all(uint(row[k]) for k in ('pass', 'coord_bits', 'kmax', 'threads', 'sites', 'wall_ns', 'pic_octets')),
                'full : entier hors u64 ou bool')
        require(row['phase'] == 'full' and row['pass'] == i and row['status'] == 'ok', 'ordre/statut full')
        require(row['trame'] == name[-23:] and row['voie'] == ('device' if device else 'cpu')
                and row['coord_bits'] == 21 and row['threads'] == threads and row['kmax'] == kmax
                and row['sites'] == sites[name], 'metadonnees de commande/trame')
        require(type(row['full_sha256']) is str and SHA.fullmatch(row['full_sha256']), 'empreinte FUL1')
        integers(row['etapes_ns'], STAGES, 'etages')
        integers(row['c_ns'], CAT, 'diagnostics C')
        integers(row['g_ns'], ('tables', 'resolution'), 'diagnostics G')
        integers(row['hors_mur_ns'], ('validation', 'empreinte'), 'hors mur')
        stages = row['etapes_ns']
        require(sum(stages[k] for k in ('P', 'C', 'G', 'raccord', 'TMVR')) <= row['wall_ns'], 'etages > mur')
        require(sum(stages[k] for k in ('T', 'M', 'V', 'R')) <= stages['TMVR'], 'T/M/V/R > enveloppe')
        require(sum(row['g_ns'].values()) <= stages['G'], 'tables/resolution > G')
        # InputFiles conserve x/y/z u32 et PointId u32 de chaque trame chargee jusqu'a la fin du processus.
        # restart_peak() repart de used(), donc les entrees residentes restent dans chaque pic.
        require(row['pic_octets'] >= resident_input_bytes, 'pic inferieur aux entrees residentes')
        # Les sous-diagnostics C ne sont pas declares disjoints : aucune somme vers C.
        keys(release, ('phase', 'pass', 'liberation_ns'), 'liberation')
        require(release['phase'] == 'liberation' and uint(release['pass']) and release['pass'] == i
                and uint(release['liberation_ns']), 'ordre/duree liberation')
        result.append(row)
    return result


def specs(cohort):
    require(type(cohort) is list and len(cohort) == 37, 'cohorte : 37 trames requises')
    names = []
    for item in cohort:
        keys(item, ('name', 'sites'), 'cohorte')
        name = item['name']
        require(type(name) is str and re.fullmatch(r'[A-Za-z0-9_-]+', name) is not None
                and uint(item['sites']) and item['sites'] > 0, 'cohorte : nom/sites')
        names.append(name)
    require(len(set(names)) == 37 and len({n[-23:] for n in names}) == 37, 'cohorte/etiquettes non uniques')
    require(not set(names).intersection(FRAMES), 'cohortes confondues')
    result = {}
    for tag, group, k, processes, passes, device in (
            ('k5', 'k5_appareil', 5, 5, 10, True), ('k10', 'k10_appareil', 10, 3, 5, True),
            ('cpu', 'k5_cpu', 5, 3, 5, False)):
        for rep in range(processes):
            for name in FRAMES[rep % 3:] + FRAMES[:rep % 3]:
                result[f'{tag}_{name}_r{rep}.jsonl'] = dict(group=group, kmax=k, device=device,
                                                          names=[name]*passes)
    for rep in range(5):
        order = names[rep:] + names[:rep]
        result[f'v12set_r{rep}.jsonl'] = dict(group='v12set_k5_appareil', kmax=5, device=True, names=order*2)
    return result


def frame_stats(runs):
    warm = [row for run in runs for row in run[1:]]
    med = statistics.median
    return dict(mediane_ns=med(row['wall_ns'] for row in warm),
                max_medianes_ns=max(med(row['wall_ns'] for row in run[1:]) for run in runs),
                max_ns=max(row['wall_ns'] for row in warm), premiere_ns=med(run[0]['wall_ns'] for run in runs),
                etapes_ns={k: med(row['etapes_ns'][k] for row in warm) for k in STAGES},
                c_ns={k: med(row['c_ns'][k] for row in warm) for k in CAT},
                pic_octets=max(row['pic_octets'] for row in warm), sites=warm[0]['sites'], valeurs=len(warm))


def contract(stats):
    med = statistics.median(v['mediane_ns'] for v in stats.values())
    maximum = max(v['max_medianes_ns'] for v in stats.values())
    return dict(mediane_ns=med, maximum_ns=maximum, trames=len(stats), tenu=med <= BUDGET and maximum <= BUDGET)


def same(actual, expected):
    """Comparaison recursive : bool ne vaut jamais 0/1, nan/inf exclus."""
    if type(expected) is dict:
        return type(actual) is dict and set(actual) == set(expected) and all(same(actual[k], v) for k, v in expected.items())
    if type(expected) is list:
        return type(actual) is list and len(actual) == len(expected) and all(same(a, e) for a, e in zip(actual, expected))
    if type(expected) in (int, float):
        return type(actual) is type(expected) and (type(actual) is int or math.isfinite(actual)) and actual == expected
    return type(actual) is type(expected) and actual == expected


def reconstruct(raws, cohort, ng_sites):
    expected = specs(cohort)
    require(type(raws) is dict and set(raws) == set(expected), 'processus manquant/supplementaire')
    require(set(ng_sites) == set(FRAMES) and all(uint(v) and v > 0 for v in ng_sites.values()), 'sites ng')
    sites = dict(ng_sites, **{x['name']: x['sites'] for x in cohort})
    groups = {g: {} for g in ('k5_appareil', 'k10_appareil', 'k5_cpu', 'v12set_k5_appareil')}
    for filename, spec in expected.items():
        try:
            rows = admit_process(raws[filename], spec, sites)
        except Refusal as error:
            raise Refusal(filename + ': ' + str(error)) from error
        group = groups[spec['group']]
        if filename.startswith('v12set_'):
            for i, name in enumerate(spec['names'][:37]):
                group.setdefault(name, []).append([rows[i], rows[i+37]])
        else:
            group.setdefault(spec['names'][0], []).append(rows)
    fingerprints = {}
    for label, selected in (('k5', ('k5_appareil', 'k5_cpu')), ('k10', ('k10_appareil',)),
                            ('v12set', ('v12set_k5_appareil',))):
        seen = {}
        for group in selected:
            for frame, runs in groups[group].items():
                seen.setdefault(frame, set()).update(p['full_sha256'] for run in runs for p in run)
        require(all(len(v) == 1 for v in seen.values()), 'empreintes incoherentes : ' + label)
        fingerprints[label] = {name: next(iter(values)) for name, values in seen.items()}
    stats = {group: {frame: frame_stats(runs) for frame, runs in frames.items()} for group, frames in groups.items()}
    contracts = {'ng00_02': contract(stats['k5_appareil']), 'v12set': contract(stats['v12set_k5_appareil'])}
    return dict(statistiques=stats, empreintes=fingerprints, contrat=contracts,
                verdict='tenu' if all(v['tenu'] for v in contracts.values()) else 'non tenu')


def review(report, computed, external_codes=None):
    """Temps admis separement ; conditions de campagne et concordance du rapport explicites."""
    require(type(report) is dict, 'rapport non objet')
    problems = []
    def check(ok, message):
        if not ok:
            problems.append(message)
    check(set(report) == {'mesure', 'budget_ns', 'options', 'refus', 'environnement_avant', 'environnement_apres',
                          'empreintes', 'statistiques', 'contrat', 'verdict', 'duree_s'}, 'champs rapport')
    duration = report.get('duree_s')
    check(type(duration) in (int, float) and (type(duration) is int or math.isfinite(duration))
          and 0 <= duration <= U64, 'duree campagne')
    check(report.get('mesure') == 'MES-FULL' and same(report.get('budget_ns'), BUDGET), 'mesure/budget')
    options = report.get('options')
    check(type(options) is dict, 'options absentes')
    if type(options) is dict:
        check(set(options) == {'fils', 'processus', 'passes', 'jobs', 'delai', 'essai', 'sonde',
                               'src', 'travail', 'donnees', 'sortie', 'archive_v12set'}, 'champs options')
        for key, value in dict(fils=48, processus=5, passes=10, jobs=44, delai=900, essai=False, sonde=None).items():
            check(same(options.get(key), value), 'option ' + key)
        for key in ('src', 'travail', 'donnees', 'sortie', 'archive_v12set'):
            check(type(options.get(key)) is str and bool(options[key]), 'chemin option ' + key)
    refus = report.get('refus')
    no_refusal = type(refus) is list and len(refus) == 0
    check(no_refusal, 'refus du pilote ou liste absente')
    for phase in ('avant', 'apres'):
        env = report.get('environnement_' + phase)
        check(type(env) is dict, 'environnement ' + phase)
        if type(env) is dict:
            check(set(env) == {'gpu_apps', 'gpu', 'nvcc', 'cmake'}, 'champs environnement ' + phase)
            check(type(env.get('gpu_apps')) is str and env['gpu_apps'] == '', 'GPU non connu vide ' + phase)
            for field in ('gpu', 'nvcc', 'cmake'):
                check(type(env.get(field)) is str and bool(env[field].strip()), field + ' indisponible ' + phase)
    for key in ('statistiques', 'empreintes', 'contrat'):
        check(same(report.get(key), computed[key]), 'divergence ' + key)
    if external_codes is None:
        code_evidence = 'zero infere de refus=[] sous le pilote epingle' if no_refusal else 'indetermine'
    else:
        check(type(external_codes) is dict and len(external_codes) == 38
              and all(type(v) is int and v == 0 for v in external_codes.values()), 'codes externes')
        code_evidence = 'codes externes fournis (identite verifiee par appelant)'
    verdict = 'refuse' if problems else computed['verdict']
    check(report.get('verdict') == verdict, 'divergence verdict')
    return dict(bruts_admis=True, statistiques_recalculees=computed, conditions_non_satisfaites=problems,
                verdict_independant='refuse' if problems else verdict, codes_processus=code_evidence,
                limites=['Provenance compilation/binaire et identite/fermeture Session : controle distinct.',
                         'Inference des codes conditionnee a la provenance du rapport sous le pilote epingle.',
                         'Sites v12set declares par manifeste ; ng declares par tailles. Aucun payload relu.',
                         'pic_octets = MemoryBudget::peak commun hote/GPU/epingle ; ni RSS ni pic VRAM separe.',
                         'CPU.s par trame absent. Mur exclut lecture, validation, digest, liberation et initialisation PassState.',
                         'open ne mesure que CatalogueDevice::open ; Pool et Session precedents non chronometres.'])


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--results', type=Path, required=True, help='Dossier contenant rapport_full.json et brut/')
    parser.add_argument('--repo', type=Path, required=True, help='Depot Git contenant les commits epingles')
    parser.add_argument('--plan', type=Path, required=True, help='Plan K immutable')
    parser.add_argument('--codes', type=Path, help='Codes EXTERNES archives par nom JSONL, facultatifs')
    args = parser.parse_args()
    metadata = decode(Path(__file__).with_name('capture.json').read_bytes())
    require(sha(args.plan.read_bytes()) == metadata['plan_sha256'], 'plan different')
    for pin in metadata['sources']:
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', pin['commit'] + ':' + pin['path']])
        require(sha(body) == pin['sha256'], 'source epinglee differente')
    report_bytes = (args.results/'rapport_full.json').read_bytes()
    report = decode(report_bytes)
    paths = sorted((args.results/'brut').glob('*.jsonl'))
    raws = {p.name: p.read_bytes() for p in paths}
    computed = reconstruct(raws, metadata['cohort'], metadata['ng_sites'])
    codes = decode(args.codes.read_bytes()) if args.codes else None
    if codes is not None:
        require(type(codes) is dict and set(codes) == set(raws), 'identite codes externes')
    result = review(report, computed, codes)
    require((args.results/'rapport_full.json').read_bytes() == report_bytes, 'rapport modifie pendant lecture')
    require(sorted(p.name for p in (args.results/'brut').glob('*.jsonl')) == sorted(raws)
            and all(p.read_bytes() == raws[p.name] for p in paths), 'bruts modifies pendant lecture')
    result['fichiers_sha256'] = {'rapport_full.json': sha(report_bytes), **{k: sha(v) for k, v in raws.items()}}
    print(json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (Refusal, OSError, subprocess.CalledProcessError) as error:
        print(json.dumps({'bruts_admis': False, 'erreur': str(error)}, ensure_ascii=False))
        raise SystemExit(1)

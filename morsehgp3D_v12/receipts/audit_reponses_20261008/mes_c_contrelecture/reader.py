"""MES-C : relecture des bruts uniquement ; aucun lancement de sonde ni lecture de données."""
import argparse
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import types

HARD = {'lattice', 'line', 'sphere'}
STAGES = ('P', 'C', 'G', 'raccord', 'TMVR')
SHA = re.compile(r'[0-9a-f]{64}\Z')


class Invalid(ValueError):
    pass


def need(condition, reason):
    if not condition:
        raise Invalid(reason)


def unique(pairs):
    out = {}
    for key, value in pairs:
        need(key not in out, 'cle repetee : ' + key)
        out[key] = value
    return out


def no_constant(value):
    raise Invalid('constante non finie : ' + value)


def loads(text):
    return json.loads(text, object_pairs_hook=unique, parse_constant=no_constant)


def load_sources(repo, capture):
    texts = {}
    for path, expected in capture['sources'].items():
        data = subprocess.check_output(['git', '-C', str(repo), 'show',
                                       capture['commit'] + ':morsehgp3D_v12/' + path])
        need(hashlib.sha256(data).hexdigest() == expected, 'pin source ' + path)
        texts[path] = data.decode()
    lf = types.ModuleType('pinned_lecteur_full')
    exec(compile(texts['microbancs/outils/lecteur_full.py'], 'pinned_lecteur_full', 'exec'), lf.__dict__)
    reasons = dict(re.findall(r'^MHGP12_REASON\((\w+), (\w+), \w+\)',
                              texts['src/core/reasons.def'], re.M))
    return lf, reasons


def cases_of(capture):
    cases = [dict(zip(capture['cases_columns'], row)) for row in capture['cases']]
    need(len(cases) == len({c['nom'] for c in cases}) == len({c['etiquette'] for c in cases}), 'cas repetes')
    for i, c in enumerate(cases):
        need(c['etiquette'] == 'c%03d' % i and type(c['sites']) is int and c['sites'] > 0, 'metadonnees cas')
        need('/' not in c['nom'] and c['groupe'] in HARD | {'reel', 'uniform', 'clusters8', 'slab'}, 'groupe/nom')
    return cases


def specifications(capture):
    cfg = capture['configuration']
    cases = cases_of(capture)
    regular = [c for c in cases if c['groupe'] not in HARD]
    hard = [c for c in cases if c['groupe'] in HARD]
    out = []
    for k in cfg['ks']:
        for voie in cfg['voies']:
            for w in cfg['threads']:
                key = '%s:%d:%d' % (voie, k, w)
                out.append(dict(kind='session', key=key, file='session_' + key.replace(':', '_') + '.jsonl',
                                cases=regular, voie=voie, k=k, fils=w, rounds=cfg['rounds']))
        for voie in cfg['voies']:
            for c in hard:
                key = '%s_k%d_%s' % (c['nom'], k, voie)
                out.append(dict(kind='hard', key=key, file=key + '.jsonl', cases=[c],
                                voie=voie, k=k, fils=cfg['hard_threads'], rounds=cfg['hard_rounds']))
    return out


def expected(spec, capture):
    return dict(voie=spec['voie'], k=spec['k'], fils=spec['fils'],
                passes=len(spec['cases']) * spec['rounds'], empreinte=True,
                trames=[(c['etiquette'], c['sites']) for c in spec['cases']],
                budget_appareil='separe', bits=capture['configuration']['coord_bits'])


def strict_process(code, text, spec, capture, lf, reasons):
    need(type(code) is int or code == 'expire' and type(code) is str, 'type du code')
    state = lf.parse_output(code, text, expected(spec, capture))
    if code != 'expire' and not (type(code) is int and code < 0):
        rows, why = lf.read_rows(text)
        need(rows is not None and rows, 'JSON ' + why)
        end = rows[-1]
        need(end.get('reason') in reasons and reasons[end['reason']] == end.get('status'), 'statut/raison exit')
    floor = 16 * sum(c['sites'] for c in spec['cases'])
    limit = capture['configuration']['budget_octets']
    for row in state['passes']:
        need(floor <= row['pic_octets'] <= limit, 'pic hote/plancher entrees/budget')
        need(row['epinglee_octets'] <= row['pic_octets'], 'epingle/pic hote')
        if spec['voie'] == 'appareil':
            need(row['appareil_octets'] <= row['pic_appareil_octets'] <= limit, 'capacite/pic appareil')
        memory = row['memoire_octets']
        for before, after in zip(STAGES, STAGES[1:]):
            need(memory[after][1] >= memory[before][0], 'continuite du pic memoire')
    return state


def fit(points):
    """OLS indépendante en fractions, sans mélange de groupes/configurations."""
    if len({n for n, _ in points}) < 2:
        return None
    n = len(points)
    mx = sum(Fraction(x) for x, _ in points) / n
    my = sum(Fraction(y) for _, y in points) / n
    b = sum((Fraction(x) - mx) * (Fraction(y) - my) for x, y in points) / sum(
        (Fraction(x) - mx) ** 2 for x, _ in points)
    a = my - b * mx
    residuals = [float(Fraction(y) - a - b * x) for x, y in points]
    return dict(fixe_ns=float(a), par_site_ns=float(b), nuages=n,
                residu_abs_max_ns=max(abs(r) for r in residuals),
                cout_individuel_max_ns_par_site=max(float(Fraction(y) / x) for x, y in points))


def summarize(passes, spec):
    n = len(spec['cases'])
    need(len(passes) == n * spec['rounds'], 'passes completes pour statistiques')
    values, groups = {}, {}
    for index, case in enumerate(spec['cases']):
        mine = passes[index::n]
        hashes = sorted({p['full_sha256'] for p in mine})
        warm = mine[1:]
        med = statistics.median(p['wall_ns'] for p in warm) if warm else None
        values[case['nom']] = dict(sites=case['sites'], groupe=case['groupe'], premiere_ns=mine[0]['wall_ns'],
                                  chaud_ns=med, cpu_ns=statistics.median(p['cpu_ns'] for p in warm) if warm else None,
                                  empreintes=hashes)
        if med is not None:
            groups.setdefault(case['groupe'], []).append((case['sites'], med))
    return values, {group: fit(points) for group, points in sorted(groups.items())}


def equal_number(a, b):
    return type(a) in (int, float) and type(b) in (int, float) and math.isfinite(a) and math.isfinite(b) and \
        math.isclose(a, b, rel_tol=1e-12, abs_tol=1e-6)


def inferred_code(entry):
    """Inférence conditionnelle au pilote épinglé ; aucun faux code externe archivé."""
    if entry['etat'] == 'ok':
        return 0
    if entry['etat'] == 'refus':
        return 2
    reason = entry.get('raison', '')
    if entry['etat'] == 'echec' and reason == 'expire':
        return 'expire'
    match = re.fullmatch(r'signal (\d+)', reason)
    if entry['etat'] == 'echec' and match:
        return -int(match.group(1))
    match = re.fullmatch(r'sortie [^,]+, code (-?\d+), \d+ passes', reason)
    if entry['etat'] == 'echec' and match:
        return int(match.group(1))
    return None


def metadata_controls(report, capture):
    controls = []
    p = report.get('provenance', {})
    for key, pin in [('pilote_sha256', capture['sources']['microbancs/mes_c_petits/pilote_c.py']),
                     ('lecteur_sha256', capture['sources']['microbancs/outils/lecteur_full.py']),
                     ('archive_sha256', capture['data_archive']['sha256'])]:
        if p.get(key) != pin:
            controls.append('provenance ' + key)
    if type(p.get('sonde_sha256')) is not str or not SHA.fullmatch(p['sonde_sha256']):
        controls.append('hash sonde absent/mal forme')
    cache = p.get('cmake')
    if type(cache) is not list or not all(type(s) is str for s in cache):
        controls.append('cache de compilation absent')
    else:
        for line in ['CMAKE_BUILD_TYPE:STRING=Release', 'MHGP12_COORD_BITS:STRING=21',
                     'MHGP12_ENABLE_CUDA:BOOL=ON']:
            if cache.count(line) != 1:
                controls.append('cache ' + line)
    for when in ('avant', 'apres'):
        env = report.get('environnement', {}).get(when, {})
        if env.get('gpu_apps') != '' or any(type(env.get(k)) is not str or not env[k].strip()
                                           for k in ('cmake', 'nvcc', 'gpu')):
            controls.append('environnement/GPU connu vide ' + when)
    params = report.get('parametres', {})
    if type(params.get('tours')) is not int or params['tours'] != capture['configuration']['rounds'] or \
            type(params.get('budget_octets')) is not int or params['budget_octets'] != capture['configuration']['budget_octets']:
        controls.append('parametres tours/budget')
    argv = params.get('argv')
    wanted = capture['command_options']
    options = {}
    if type(argv) is not list or len(argv) % 2 or not all(type(x) is str for x in argv):
        controls.append('arguments mal formes')
    else:
        for option, value in zip(argv[::2], argv[1::2]):
            if option in options:
                controls.append('argument repete ' + option)
            options[option] = value
        if set(options) != set(wanted):
            controls.append('cohorte des arguments')
        for option, value in wanted.items():
            actual = options.get(option)
            if value is None:
                if type(actual) is not str or not actual:
                    controls.append('chemin absent ' + option)
            elif actual != value:
                controls.append('argument different ' + option)
        if options.get('--archive', '').rsplit('/', 1)[-1] != capture['data_archive']['name']:
            controls.append('nom archive different')
    return controls


def review(report, raws, capture, lf, reasons, codes=None):
    need(type(report) is dict and report.get('mesure') == 'MES-C' and report.get('regime') == 'c', 'format rapport')
    need(type(report.get('configurations')) is dict and type(report.get('difficiles')) is list, 'cohortes rapport')
    need(type(report.get('controles')) is list and all(type(s) is str for s in report['controles']), 'controles rapport')
    specs = specifications(capture)
    regular_keys = {s['key'] for s in specs if s['kind'] == 'session'}
    hard_keys = {s['key'] for s in specs if s['kind'] == 'hard'}
    need(set(report['configurations']) <= regular_keys, 'configuration inconnue')
    hard = {}
    for row in report['difficiles']:
        need(type(row) is dict and type(row.get('k')) is int and type(row.get('fils')) is int, 'ligne difficile')
        key = '%s_k%d_%s' % (row.get('nom'), row['k'], row.get('voie'))
        need(key in hard_keys and key not in hard, 'difficile inconnu ou duplique')
        hard[key] = row
    need(set(raws) <= {s['file'] for s in specs}, 'brut inattendu')
    if codes is not None:
        need(set(codes) == set(raws), 'cohorte des codes externes')
    controls = metadata_controls(report, capture)
    differences, states, configs, all_hashes = [], {}, {}, {}
    missing, played, full_count, warm_count = [], 0, 0, 0
    for spec in specs:
        entry = report['configurations'].get(spec['key']) if spec['kind'] == 'session' else hard.get(spec['key'])
        need(entry is None or type(entry) is dict, 'entree de configuration')
        if entry is not None and spec['kind'] == 'hard':
            c = spec['cases'][0]
            need(entry.get('fils') == spec['fils'] and entry.get('sites') == c['sites'] and
                 entry.get('groupe') == c['groupe'], 'metadonnees difficile')
        if entry is None or entry.get('etat') == 'non_joue':
            need(spec['file'] not in raws, 'brut pour cas non joue/absent')
            if entry is None:
                controls.append('ligne planifiee absente ' + spec['key'])
            missing.append(spec['key'])
            states[spec['key']] = 'non_joue'
            continue
        need(type(entry) is dict and entry.get('etat') in {'ok', 'refus', 'echec', 'illisible'}, 'etat inconnu')
        need(spec['file'] in raws, 'brut absent ' + spec['key'])
        played += 1
        code = codes[spec['file']] if codes is not None else inferred_code(entry)
        if code is None:
            controls.append('code non reconstructible ' + spec['key'])
            states[spec['key']] = 'illisible'
            continue
        try:
            state = strict_process(code, raws[spec['file']], spec, capture, lf, reasons)
        except (Invalid, KeyError, TypeError, ValueError) as exc:
            state = dict(etat='illisible', raison=str(exc), passes=[])
        states[spec['key']] = state['etat']
        if state['etat'] != entry['etat'] or state['raison'] != entry.get('raison', ''):
            differences.append(spec['key'] + ' issue differente')
        if state['etat'] == 'illisible' or spec['kind'] == 'session' and state['etat'] != 'ok':
            controls.append(spec['key'] + ' ' + state['etat'] + ' ' + state['raison'])
        full_count += len(state['passes'])
        for i, row in enumerate(state['passes']):
            case = spec['cases'][i % len(spec['cases'])]
            all_hashes.setdefault((spec['k'], case['nom']), set()).add(row['full_sha256'])
            warm_count += i >= len(spec['cases'])
        if spec['kind'] == 'hard':
            got = [dict(mur_ns=p['wall_ns'], full_sha256=p['full_sha256']) for p in state['passes']]
            if entry.get('passes') != got:
                differences.append(spec['key'] + ' passes publiees differentes')
        if spec['kind'] == 'session' and state['etat'] == 'ok':
            values, lines = summarize(state['passes'], spec)
            configs[spec['key']] = dict(valeurs=values, droites=lines)
            if entry.get('valeurs') != values:
                differences.append(spec['key'] + ' valeurs publiees differentes')
            published = entry.get('droites', {})
            if set(published) != set(lines):
                differences.append(spec['key'] + ' groupes differents')
            else:
                for group, line in lines.items():
                    other = published[group]
                    good = other is None if line is None else type(other) is dict and \
                        set(other) == {'fixe_ns', 'par_site_ns', 'nuages'} and \
                        type(other['nuages']) is int and other['nuages'] == line['nuages'] and \
                        all(equal_number(other[k], line[k]) for k in ('fixe_ns', 'par_site_ns'))
                    if not good:
                        differences.append(spec['key'] + ' regression differente ' + group)
    for (k, name), hashes in sorted(all_hashes.items()):
        if len(hashes) != 1:
            controls.append('empreintes instables K%d %s' % (k, name))
    line = configs.get('cpu:5:48', {}).get('droites', {}).get('reel')
    criteria = {'C1': 'non evalue' if line is None else 'tenu' if line['fixe_ns'] <= 2_000_000 else 'non tenu',
                'C2': 'non evalue' if line is None else 'tenu' if line['par_site_ns'] <= 241_300_000 / 64740 else 'non tenu'}
    k5 = [s for s in specs if s['kind'] == 'hard' and s['k'] == 5]
    incomplete = not k5 or any(states[s['key']] in {'non_joue', 'illisible'} for s in k5)
    criteria['C3'] = 'non evalue' if incomplete else 'tenu' if all(states[s['key']] == 'ok' for s in k5) else 'non tenu'
    controls += report['controles']
    verdict = 'refuse' if controls or 'non evalue' in criteria.values() else \
        'tenu' if all(v == 'tenu' for v in criteria.values()) else 'non tenu'
    announced = report.get('criteres', {})
    if any(announced.get(k, {}).get('etat') != v for k, v in criteria.items()):
        differences.append('criteres publies differents')
    if report.get('verdict') != verdict:
        differences.append('verdict publie different')
    return dict(criteres=criteria, verdict=verdict, controles=sorted(set(controls)), differences=sorted(differences),
                processus_prevus=len(specs), processus_joues=played, non_joues=missing,
                cohorte_complete=not missing,
                passes_completes=full_count, passes_chaudes=warm_count,
                codes='externes fournis' if codes is not None else 'inferes du rapport et du pilote epingle',
                configurations=configs, etats=states)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--rapport', required=True)
    parser.add_argument('--bruts', required=True)
    parser.add_argument('--codes', help='codes externes archivés, nom JSONL -> int ou expire ; facultatif')
    args = parser.parse_args()
    capture = loads(Path(__file__).with_name('capture.json').read_text())
    lf, reasons = load_sources(args.repo, capture)
    report = loads(Path(args.rapport).read_text())
    folder = Path(args.bruts)
    need(all(p.is_file() and not p.is_symlink() and p.suffix == '.jsonl' for p in folder.iterdir()), 'inventaire bruts')
    raws = {p.name: p.read_bytes().decode('ascii') for p in folder.iterdir()}
    codes = loads(Path(args.codes).read_text()) if args.codes else None
    print(json.dumps(review(report, raws, capture, lf, reasons, codes), sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()

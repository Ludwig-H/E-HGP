"""Decision du banc v10 : un CALCUL sur les resultats d'une campagne de test preenregistree (EVAL_v2 § 6, D12).

Comparaisons appariees K = min_samples (directive utilisateur du 28 septembre 2026) : chaque paire confirmatoire
oppose la tour a l'ordre K a sklearn HDBSCAN a min_samples = K, chacun avec sa tete choisie sur dev par la meme
regle. Unite statistique : la scene. Poids w_s = 1 / (C * R_c) : chaque cellule (famille, niveau, bruit, taille)
pese autant. Pour une paire (M, A) : Delta_s = ARI_s(M) - ARI_s(A), Delta = sum_s w_s Delta_s.
  - test : retournement de signe stratifie (bootstrap sauvage de Rademacher), bilateral ; Holm sur les paires ;
  - IC 95 % : bootstrap de McCarthy-Snowden (R_c - 1 tirages avec remise par cellule) ;
  - garde : AMI (bruit en classe), une perte significative bloque « bat ».
« M bat A » : p_Holm < alpha, Delta >= delta_min, borne basse de l'IC > 0, Delta > 0 a chaque taille, aucune perte
significative d'AMI, refus de M <= refusal_cap. « A bat M » : p_Holm < alpha et Delta < 0. Sinon : pas de
difference significative, avec la non-inferiorite a la marge delta_min si la borne basse de l'IC >= -delta_min.
Familles secondaires (`decision.secondary`, facultatives) : meme regle, Holm dans chaque famille, libelles propres
(`method_label`, `adversary_label`) ; elles ne changent pas la decision principale.

  python3 decide.py --prereg prereg/PREREG_<id>.json --run <dossier de run_test> [--check-only]
Ecrit <dossier>/DECISION.json et <dossier>/DECISION.md. Codes : 0, 2 (preenregistrement ou run incoherent).

Avant tout calcul, le lot doit etre EXACTEMENT le plan preenregistre (audit du 29 septembre 2026) : manifeste des
scenes reconstruit depuis le preenregistrement et egal a son epingle `manifest_sha256`, meme plan et meme nombre de
scenes dans run.json (et `complete` s'il est declare), puis chaque couple (scene prevue, methode) present une et une
seule fois, aucune scene ni methode hors plan, metadonnees (famille, niveau, bruit, n, graine) egales a la
specification, `refused` dans {0, 1}, ARI_s et AMI finis. Sinon : refus, code 2, rien n'est ecrit. Cette
verification tourne en Python nu (numpy n'est importe qu'apres) ; --check-only s'arrete la (code 0 si conforme).
Complements du 30 septembre 2026 (verificateur des bancs, contre-audits) :
  - domaine des scores que la decision lit, sur les lignes non refusees : ARI_s dans [-1/2, 1] et AMI_nc <= 1, a
    TOL = 1e-9 pres (arrondi du calcul flottant ; run_test.py ecrit 6 decimales). Bornes : ARI de Hubert-Arabie,
    minimum -1/2 (Chacon et Rastrojo 2023, documente par sklearn 1.9.1 que metrics.py appelle) ; AMI de sklearn,
    borne par 1 et sans borne basse (un score negatif est valide). Une ligne refusee vaut 0 (EVAL_v2 D8) : ses
    scores ne sont pas lus, un NaN y est admis. ari_nc, coverage et clusters ne sont pas lus par la decision ;
  - preenregistrement, run.json ou results.csv absent ou illisible : refus, code 2 (plus de plantage, code 1) ;
  - plan aux specifications dupliquees (meme nom d'unite deux fois) : refus explicite ; run_test.py calculerait la
    scene deux fois et aucun lot ne pourrait etre exactement le plan.
"""
import argparse
import ast
import collections
import csv
import hashlib
import json
import math
import os
import sys

np = None  # importe par main() apres la verification du lot : refus et --check-only tournent sans numpy

HERE = os.path.dirname(os.path.abspath(__file__))
REQUIRED = ('unit', 'family', 'level', 'noise', 'n', 'seed', 'method', 'ari_s', 'ami_nc', 'refused')
TOL = 1e-9  # tolerance numerique declaree des bornes des scores
ARI_MIN, ARI_MAX = -0.5, 1.0  # ARI de Hubert-Arabie (sklearn adjusted_rand_score)
AMI_MAX = 1.0  # AMI (sklearn adjusted_mutual_info_score) : aucune borne basse


def sha256_file(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


def plan_axes():
    """FAMILIES et LEVELS de scenes.py, lus dans sa source (l'importer exigerait numpy) : leur ordre fixe celui du
    manifeste, donc son sha256."""
    with open(os.path.join(HERE, 'scenes.py')) as f:
        tree = ast.parse(f.read())
    axes = {}
    for node in tree.body:
        if (isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name) and
                node.targets[0].id in ('FAMILIES', 'LEVELS')):
            axes[node.targets[0].id] = tuple(ast.literal_eval(node.value))
    return axes['FAMILIES'], axes['LEVELS']


def seed_of(split, spec, replicate):
    """run_campaign.seed_of, en Python nu ; le sha256 du manifeste, compare a l'epingle, prouve l'egalite."""
    text = json.dumps(dict(spec, split=split, replicate=replicate), sort_keys=True)
    return int(hashlib.sha256(text.encode()).hexdigest()[:15], 16)


def plan_specs(prereg):
    """Specifications du plan preenregistre et sha256 de leur manifeste canonique, exactement comme
    run_test.plan_manifest (run_campaign.plan, puis filtre des familles et des niveaux), en Python nu."""
    p = prereg['plan']
    families, levels = plan_axes()
    specs = []
    for family in families:
        for level in levels:
            for n in p['sizes']:
                for noise in tuple(p['noises']):
                    base = dict(family=family, n=n, groups=8, level=level, noise_fraction=noise)
                    for r in range(p['replicates']):
                        specs.append(dict(base, seed=seed_of(p['split'], base, r)))
    specs = [s for s in specs if s['family'] in p['families'] and s['level'] in p['levels']]
    text = json.dumps(specs, sort_keys=True, separators=(',', ':'))
    return specs, hashlib.sha256(text.encode()).hexdigest()


def unit_name(spec):
    """run_test.unit_name, en Python nu."""
    return '%s_n%d_%s_nu%g_s%d' % (spec['family'], spec['n'], spec['level'], spec['noise_fraction'], spec['seed'])


def finite_scores(row):
    """Une ligne refusee vaut 0 (EVAL_v2 D8) ; sinon ARI_s et AMI doivent etre des nombres finis."""
    if row.get('refused') == '1':
        return True
    if row.get('refused') != '0':
        return False
    try:
        return math.isfinite(float(row['ari_s'])) and math.isfinite(float(row['ami_nc']))
    except (TypeError, ValueError):
        return False


def scores_in_domain(row):
    """Scores finis d'une ligne (finite_scores vrai) dans le domaine de leur definition, a TOL pres : ARI_s dans
    [-1/2, 1], AMI_nc <= 1. Une ligne refusee vaut 0 : ses scores ne sont pas lus."""
    if row.get('refused') != '0':
        return True
    ari, ami = float(row['ari_s']), float(row['ami_nc'])
    return ARI_MIN - TOL <= ari <= ARI_MAX + TOL and ami <= AMI_MAX + TOL


def check_rows(rows, fields, specs, names, complete):
    """Raisons de refus des lignes d'un lot contre le plan : colonnes, scenes hors plan, methodes inconnues, couples
    (scene, methode) dupliques, metadonnees differentes de la specification (chaine CSV exacte : la cellule en
    depend), refus hors {0, 1}, scores non finis ; couples manquants parmi toutes les scenes prevues (complete) ou
    parmi les scenes presentes (fusion partielle)."""
    absent = [c for c in REQUIRED if c not in (fields or ())]
    if absent:
        return ['colonnes absentes de results.csv : ' + ', '.join(absent)]
    plan = collections.OrderedDict((unit_name(s), s) for s in specs)
    known = set(names)
    seen, outside, unknown, dups, meta, bad, domain = set(), [], [], [], [], [], []
    for r in rows:
        key = (r['unit'], r['method'])
        spec = plan.get(r['unit'])
        if spec is None:
            outside.append(key)
        elif r['method'] not in known:
            unknown.append(key)
        elif key in seen:
            dups.append(key)
        else:
            seen.add(key)
            if (r['family'], r['level'], r['noise'], r['n'], r['seed']) != (
                    spec['family'], spec['level'], str(spec['noise_fraction']), str(spec['n']), str(spec['seed'])):
                meta.append(key)
            if not finite_scores(r):
                bad.append(key)
            elif not scores_in_domain(r):
                domain.append(key)
    units = list(plan) if complete else [u for u in plan if any((u, m) in seen for m in names)]
    missing = [(u, m) for u in units for m in names if (u, m) not in seen]
    errors = []
    for items, label in ((missing, 'couples (scene prevue, methode) manquants'),
                         (outside, 'lignes de scenes hors du plan preenregistre'),
                         (unknown, 'lignes de methodes hors du preenregistrement'),
                         (dups, 'couples (scene, methode) dupliques'),
                         (meta, 'lignes aux metadonnees differentes de la specification'),
                         (bad, 'lignes a refus hors {0, 1} ou a score non fini'),
                         (domain, 'lignes non refusees a score hors domaine (ARI_s hors [-1/2, 1] ou AMI_nc > 1, '
                                  'tolerance %g)' % TOL)):
        if items:
            errors.append('%d %s, par exemple %s' % (len(items), label, ', '.join('%s/%s' % k for k in items[:3])))
    return errors


def check_run(prereg, run_dir, info):
    """Raisons de refus du lot de run_dir : manifeste reconstruit different de l'epingle, run.json d'un autre plan,
    d'un autre nombre de scenes ou declare incomplet, puis lignes (check_rows, lot complet exige)."""
    try:
        pinned = prereg['plan'].get('manifest_sha256')
        specs, digest = plan_specs(prereg)
        names = [m['name'] for m in prereg['methods']]
    except (OSError, SyntaxError, AttributeError, KeyError, TypeError, ValueError) as e:
        return ['plan preenregistre illisible : %r' % e]
    errors = []
    if digest != pinned:
        errors.append('manifeste reconstruit %s different de l epingle %s' % (digest, pinned))
    repeated = len(specs) - len({unit_name(s) for s in specs})
    if repeated:
        errors.append('plan preenregistre aux specifications dupliquees : %d noms d unites repetes' % repeated)
    if info.get('plan_sha256') != pinned:
        errors.append('run.json : plan %s au lieu de l epingle %s' % (info.get('plan_sha256'), pinned))
    if info.get('scenes') != len(specs):
        errors.append('run.json : %s scenes annoncees, %d prevues' % (info.get('scenes'), len(specs)))
    if info.get('complete', True) is not True or info.get('computed', len(specs)) != len(specs):
        errors.append('run.json declare une campagne incomplete (complete=%s, computed=%s)' % (
            info.get('complete'), info.get('computed')))
    try:
        with open(os.path.join(run_dir, 'results.csv'), newline='') as f:
            reader = csv.DictReader(f)
            rows = list(reader)
            fields = reader.fieldnames
    except (OSError, ValueError, csv.Error) as e:
        return errors + ['results.csv absent ou illisible : %r' % e]
    return errors + check_rows(rows, fields, specs, names, complete=True)


def load(run_dir, methods):
    rows = list(csv.DictReader(open(os.path.join(run_dir, 'results.csv'))))
    units = {}
    for r in rows:
        u = units.setdefault(r['unit'], dict(cell=(r['family'], r['level'], r['noise'], r['n']), family=r['family'],
                                             n=int(r['n']), score={}, ami={}, refused={}))
        ok = r['refused'] == '0'
        u['score'][r['method']] = float(r['ari_s']) if ok else 0.0
        u['ami'][r['method']] = float(r['ami_nc']) if ok else 0.0
        u['refused'][r['method']] = int(r['refused'])
    missing = [(k, m) for k, u in units.items() for m in methods if m not in u['score']]
    return units, missing


class Paired:
    """Ecarts apparies ponderes par cellule, avec test de retournement de signe et IC bootstrap."""

    def __init__(self, units, keys, seed, b_perm, b_boot):
        cells = collections.defaultdict(list)
        for i, k in enumerate(keys):
            cells[units[k]['cell']].append(i)
        self.cells = list(cells.values())
        self.w = np.zeros(len(keys))
        for idx in self.cells:
            self.w[idx] = 1.0 / (len(self.cells) * len(idx))
        self.seed, self.b_perm, self.b_boot = seed, b_perm, b_boot

    def stats(self, d):
        d = np.asarray(d, dtype=np.float64)
        mean = float(np.dot(self.w, d))
        rng = np.random.default_rng(self.seed)
        wd = self.w * d
        hits, done = 0, 0
        while done < self.b_perm:
            b = min(20000, self.b_perm - done)
            signs = rng.integers(0, 2, size=(b, len(d)), dtype=np.int8) * 2 - 1
            hits += int(np.count_nonzero(np.abs(signs @ wd) >= abs(mean) - 1e-15))
            done += b
        boots = np.zeros(self.b_boot)
        for idx in self.cells:
            vals = d[idx]
            if len(vals) < 2:
                boots += vals[0] / len(self.cells)
                continue
            draws = rng.integers(0, len(vals), size=(self.b_boot, len(vals) - 1))
            boots += vals[draws].mean(axis=1) / len(self.cells)
        lo, hi = np.percentile(boots, [2.5, 97.5])
        wins, losses = int(np.sum(d > 1e-9)), int(np.sum(d < -1e-9))
        return dict(delta=mean, p=(1 + hits) / (1 + self.b_perm), ci=[float(lo), float(hi)], wins=wins,
                    losses=losses, ties=len(d) - wins - losses, scenes=len(d))


def holm(pvals):
    order = sorted(range(len(pvals)), key=lambda i: pvals[i])
    adj, running = [0.0] * len(pvals), 0.0
    for rank, i in enumerate(order):
        running = max(running, min(1.0, (len(pvals) - rank) * pvals[i]))
        adj[i] = running
    return adj


def fr(x, nd=3):
    return ('%+.*f' % (nd, x)).replace('.', ',')


def fp(p):
    return ('%.2g' % p).replace('.', ',')


def compare(units, keys, seed, dec, method, adversary):
    pair = Paired(units, keys, seed, dec['permutations'], dec['bootstrap'])
    d = np.array([units[k]['score'][method] - units[k]['score'][adversary] for k in keys])
    g = np.array([units[k]['ami'][method] - units[k]['ami'][adversary] for k in keys])
    out = pair.stats(d)
    out['guard_ami'] = pair.stats(g)
    out['by_size'] = {}
    for n in sorted({units[k]['n'] for k in keys}):
        ks = [k for k in keys if units[k]['n'] == n]
        sub = Paired(units, ks, seed + n, dec['permutations'], dec['bootstrap'])
        out['by_size'][str(n)] = sub.stats(np.array([units[k]['score'][method] - units[k]['score'][adversary]
                                                     for k in ks]))
    fams = sorted({units[k]['family'] for k in keys})
    out['by_family'] = {}
    for i, f in enumerate(fams):
        ks = [k for k in keys if units[k]['family'] == f]
        sub = Paired(units, ks, seed + 1000 + i, dec['permutations'], dec['bootstrap'])
        out['by_family'][f] = sub.stats(np.array([units[k]['score'][method] - units[k]['score'][adversary]
                                                  for k in ks]))
    for f, pa in zip(fams, holm([out['by_family'][f]['p'] for f in fams])):
        out['by_family'][f]['p_holm'] = pa
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--prereg', required=True)
    ap.add_argument('--run', required=True)
    ap.add_argument('--check-only', action='store_true', help='verifier le lot contre le plan, sans decider')
    args = ap.parse_args()
    loaded = {}
    for key, path in (('prereg', args.prereg), ('info', os.path.join(args.run, 'run.json'))):
        try:
            with open(path) as f:
                loaded[key] = json.load(f)
        except (OSError, ValueError) as e:  # absent, illisible, JSON invalide ou mal encode
            print('REFUS : %s absent ou illisible : %r' % (path, e), flush=True)
            return 2
        if not isinstance(loaded[key], dict):
            print('REFUS : %s n est pas un objet JSON' % path, flush=True)
            return 2
    prereg, info = loaded['prereg'], loaded['info']
    psha = sha256_file(args.prereg)
    if info.get('prereg_sha256') != psha:
        print('REFUS : la campagne n a pas ete executee sous ce preenregistrement', flush=True)
        return 2
    errors = check_run(prereg, args.run, info)
    if errors:
        for e in errors:
            print('REFUS : ' + e, flush=True)
        return 2
    if args.check_only:
        print('lot_conforme_au_plan : %d scenes x %d methodes' % (info['scenes'], len(prereg['methods'])), flush=True)
        return 0
    global np
    import numpy as np  # noqa: E402  (apres la verification : les refus tournent en Python nu)
    dec = prereg['decision']
    names = [m['name'] for m in prereg['methods']]
    units, missing = load(args.run, names)
    if missing:
        print('REFUS : %d couples (scene, methode) manquants' % len(missing), flush=True)
        return 2
    keys = sorted(units)
    seed = int(psha[:16], 16)
    weights = Paired(units, keys, seed, 1, 1).w
    refusals = {m: int(sum(units[k]['refused'][m] for k in keys)) for m in names}
    out = dict(prereg=os.path.basename(args.prereg), prereg_sha256=psha, scenes=len(keys), refusals=refusals,
               mean_ari_s={m: float(np.dot(weights, [units[k]['score'][m] for k in keys])) for m in names},
               pairs={}, reported={})
    alpha, dmin = dec['alpha'], dec['delta_min']
    for i, p in enumerate(dec['pairs']):
        out['pairs'][p['name']] = dict(p, **compare(units, keys, seed + 17 * i, dec, p['method'], p['adversary']))
    for pname, pa in zip(out['pairs'], holm([v['p'] for v in out['pairs'].values()])):
        out['pairs'][pname]['p_holm'] = pa
    for i, p in enumerate(dec.get('reported', [])):
        out['reported'][p['name']] = dict(p, **compare(units, keys, seed + 7919 + 17 * i, dec, p['method'],
                                                       p['adversary']))
    def judge_pairs(pairs, mlab, alab):
        res, text = {}, []
        for pname, s in pairs.items():
            g = s['guard_ami']
            guard_loss = g['p'] < alpha and g['delta'] < 0
            beats = (s['p_holm'] < alpha and s['delta'] > 0 and s['delta'] >= dmin and s['ci'][0] > 0 and
                     all(v['delta'] > 0 for v in s['by_size'].values()) and not guard_loss and
                     refusals[s['method']] <= dec['refusal_cap'] * len(keys))
            if beats:
                verdict = '%s bat %s' % (mlab, alab)
            elif s['p_holm'] < alpha and s['delta'] < 0:
                verdict = '%s bat %s' % (alab[0].upper() + alab[1:], mlab)
            elif s['p_holm'] < alpha and s['delta'] > 0:
                verdict = 'avantage significatif à %s, sous la marge ou non uniforme en taille' % mlab
            else:
                verdict = 'pas de différence significative'
            res[pname] = 'gagne' if beats else ('perd' if s['p_holm'] < alpha and s['delta'] < 0 else 'autre')
            margin = ('%.2f' % dmin).replace('.', ',')
            ni = ('non-infériorité de %s à la marge %s établie' % (mlab, margin) if s['ci'][0] >= -dmin
                  else 'non-infériorité de %s non établie' % mlab)
            s['verdict'] = verdict
            text.append('- %s (%s contre %s) : %s ; Δ = %s [%s ; %s], p_Holm = %s, %d victoires / %d défaites / '
                        '%d égalités ; %s.' % (pname, s['method'], s['adversary'], verdict, fr(s['delta']),
                                              fr(s['ci'][0]), fr(s['ci'][1]), fp(s['p_holm']), s['wins'], s['losses'],
                                              s['ties'], ni))
        return res, text

    verdicts, lines = judge_pairs(out['pairs'], 'la tour', 'HDBSCAN')
    out['secondary'] = {}
    sec_lines = []
    for fi, fam in enumerate(dec.get('secondary', [])):
        fp_ = {}
        for i, p in enumerate(fam['pairs']):
            fp_[p['name']] = dict(p, **compare(units, keys, seed + 104729 * (fi + 1) + 17 * i, dec, p['method'],
                                               p['adversary']))
        for pname, pa in zip(fp_, holm([v['p'] for v in fp_.values()])):
            fp_[pname]['p_holm'] = pa
        _, text = judge_pairs(fp_, fam.get('method_label', 'la tour'), fam.get('adversary_label', 'HDBSCAN'))
        out['secondary'][fam['name']] = fp_
        sec_lines += ['', 'Famille secondaire « %s » (Holm dans la famille ; sans effet sur la décision principale) :'
                      % fam['name']] + text
    won = [p for p, v in verdicts.items() if v == 'gagne']
    lost = [p for p, v in verdicts.items() if v == 'perd']
    head = ('Banc v10 préenregistré %s, %d scènes de test, comparaison appariée K = min_samples. '
            % (prereg['id'], len(keys)))
    if won and not lost:
        head += 'La tour bat HDBSCAN pour %s, sans perte significative ailleurs.' % ', '.join(won)
    elif won:
        head += 'La tour bat HDBSCAN pour %s et perd pour %s.' % (', '.join(won), ', '.join(lost))
    elif lost:
        head += 'Aucune victoire de la tour ; HDBSCAN la bat pour %s.' % ', '.join(lost)
    else:
        head += 'Aucune revendication de supériorité : pas de différence significative au sens préenregistré.'
    lines.insert(0, head)
    lines += sec_lines
    lines.append(prereg['attribution_statement'])
    out['statement'] = lines
    with open(os.path.join(args.run, 'DECISION.json'), 'w') as f:
        json.dump(out, f, indent=1, ensure_ascii=False, sort_keys=True)
    md = ['# Décision du banc v10 (%s)' % prereg['id'], '', 'Calculée par `decide.py` ; aucun chiffre écrit à la '
          'main.', ''] + lines
    md += ['', '## ARI_s moyen pondéré par cellule', '', '| Méthode | ARI_s | Refus |', '| --- | ---: | ---: |']
    md += ['| %s | %s | %d |' % (m, ('%.4f' % out['mean_ari_s'][m]).replace('.', ','), refusals[m]) for m in names]
    groups = [('pairs', out['pairs'])] + [('secondary', v) for v in out['secondary'].values()]
    groups.append(('reported', out['reported']))
    for group, items in groups:
        for pname, s in items.items():
            md += ['', '## %s : %s contre %s%s' % (pname, s['method'], s['adversary'],
                                                   '' if group == 'pairs' else ' (descriptif)'), '',
                   '| Strate | Δ | IC 95 % | p |', '| --- | ---: | --- | ---: |',
                   '| toutes | %s | [%s ; %s] | %s |' % (fr(s['delta']), fr(s['ci'][0]), fr(s['ci'][1]),
                                                       fp(s.get('p_holm', s['p'])))]
            md += ['| n = %s | %s | [%s ; %s] | %s |' % (n, fr(v['delta']), fr(v['ci'][0]), fr(v['ci'][1]), fp(v['p']))
                   for n, v in s['by_size'].items()]
            md += ['| %s | %s | [%s ; %s] | %s (Holm) |' % (f, fr(v['delta']), fr(v['ci'][0]), fr(v['ci'][1]),
                                                           fp(v['p_holm'])) for f, v in s['by_family'].items()]
    with open(os.path.join(args.run, 'DECISION.md'), 'w') as f:
        f.write('\n'.join(md) + '\n')
    print('\n'.join(lines))
    return 0


if __name__ == '__main__':
    sys.exit(main())

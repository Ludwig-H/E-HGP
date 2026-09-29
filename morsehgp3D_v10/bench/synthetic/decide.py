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

  python3 decide.py --prereg prereg/PREREG_<id>.json --run <dossier de run_test>
Ecrit <dossier>/DECISION.json et <dossier>/DECISION.md. Codes : 0, 2 (preenregistrement ou run incoherent).
"""
import argparse
import collections
import csv
import hashlib
import json
import os
import sys

import numpy as np


def sha256_file(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


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
    args = ap.parse_args()
    prereg = json.load(open(args.prereg))
    info = json.load(open(os.path.join(args.run, 'run.json')))
    psha = sha256_file(args.prereg)
    if info.get('prereg_sha256') != psha:
        print('REFUS : la campagne n a pas ete executee sous ce preenregistrement', flush=True)
        return 2
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

#!/usr/bin/env python3
"""Inventaire des mutants du raccord R2 par campagne, par fichier vise et par classe.
Lit : la relecture terminale (relecture.txt) et les journaux JSONL du rejeu final (lecture seule)."""
import collections, json, os, re, sys
V8 = '/workspaces/E-HGP/build/v10-integration-r2/logs/final5/v8'
rel = open(os.path.join(V8, 'relecture.txt'), encoding='utf-8').read().splitlines()
camp = collections.OrderedDict()
ids = collections.defaultdict(list)
for l in rel:
    m = re.match(r'^(\S+)\s+(\S+)\s+annonce=(\S+)\s+cause=(\S+)', l)
    if m:
        c, i, a, k = m.groups()
        camp.setdefault(c, collections.Counter())[a + '/' + k] += 1
        ids[c].append(i)
tot = collections.Counter()
print('campagne;mutants;classes')
for c, cnt in camp.items():
    print(f'{c};{sum(cnt.values())};' + ' '.join(f'{k}={v}' for k, v in sorted(cnt.items())))
    tot.update(cnt)
print(f'TOTAL;{sum(tot.values())};' + ' '.join(f'{k}={v}' for k, v in sorted(tot.items())))
# recouvrement bancs 3.10 / 3.12
a, b = set(ids.get('bancs_py312', [])), set(ids.get('bancs_py310', []))
print(f'recouvrement bancs: py312={len(a)} py310={len(b)} communs={len(a & b)} propres_py310={len(b - a)}')
distincts = sum(len(set(v)) for k, v in ids.items() if k != 'bancs_py310') + len(b - a)
print(f'mutants distincts (identifiants de bancs_py310 deja dans bancs_py312 comptes une fois) = {distincts}')
# doublons d'identifiant entre campagnes
allid = collections.Counter()
for c, v in ids.items():
    for i in set(v):
        allid[i] += 1
print('identifiants presents dans plusieurs campagnes :', sorted(i for i, n in allid.items() if n > 1)[:60])
# fichiers vises, d'apres les journaux
def fichiers(nom, cle_f):
    p = os.path.join(V8, nom)
    cnt = collections.Counter()
    if not os.path.exists(p):
        return cnt
    for l in open(p, encoding='utf-8'):
        l = l.strip()
        if not l:
            continue
        try:
            d = json.loads(l)
        except Exception:
            continue
        f = d.get('file') or d.get('fichier')
        if f and d.get('id') not in ('temoin_avant', 'temoin_apres'):
            cnt[f] += 1
    return cnt
print()
for nom in ('reparation_journal.jsonl', 'tete_journal.jsonl', 'ecli_journal.jsonl'):
    cnt = fichiers(nom, None)
    print(nom, sum(cnt.values()))
    for f, n in cnt.most_common():
        print(f'   {n:3d}  {f}')

"""Fusion des sessions G4 d'une campagne de test preenregistree executee par lot_runner.py.

Verifie que toutes les sessions ont tourne sous le meme preenregistrement (sha256) et sous le plan qu'il epingle
(manifeste reconstruit par decide.plan_specs, en Python nu), qu'aucune scene n'a ete calculee deux fois, que chaque
scene presente appartient au plan et porte toutes les methodes du preenregistrement. La campagne n'est declaree
complete que si l'ensemble des scenes fusionnees EST celui du plan. Ecrit :
  - OUT/results.csv et OUT/run.json (segments = les sessions), directement lisibles par decide.py ;
  - OUT/done.u32le : conteneur (en-tete u64 = longueur) de la liste des scenes faites, pour la session suivante.

  python3 merge_sessions.py --prereg PREREG.json --out DIR SESSION_DIR [SESSION_DIR ...]
Codes : 0 (fusion ecrite, campagne complete ou non) ; 2 refus (sessions incoherentes, preenregistrement ou fichier
de session absent ou illisible, plan aux specifications dupliquees). Aucun __pycache__ n'est ecrit (import de decide).
Raccord R2 (30 septembre 2026, prealable P6) : le schema du preenregistrement est celui de decide.check_prereg, juge
avant toute lecture de session (cles exactes, domaines, noms de methodes distincts, paires vers des methodes
enregistrees) ; une famille ou un niveau inconnu de scenes.py, un en-tete de session a colonnes dupliquees, une cle
dupliquee dans un objet JSON (preenregistrement ou run.json de session) : refus, code 2, rien d'ecrit. --out qui
designe une session (meme chemin resolu, liens symboliques et « .. » compris, ou meme dossier existant) : refus,
code 2, avant toute lecture ; la fusion remplacerait results.csv et run.json de cette session.
"""
import argparse
import csv
import hashlib
import json
import os
import struct
import sys

sys.dont_write_bytecode = True  # l'import de decide n'ecrit pas bench/synthetic/__pycache__ (lance sans -B)
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'synthetic'))
import decide  # noqa: E402  (plan preenregistre reconstruit sans numpy)


def same_directory(a, b):
    """Vrai si deux chemins designent le meme dossier : meme chemin resolu (liens symboliques suivis, meme vers un
    dossier absent), ou deux dossiers existants de meme peripherique et meme inode."""
    if os.path.realpath(a) == os.path.realpath(b):
        return True
    try:
        return os.path.samefile(a, b)
    except OSError:  # l'un des deux n'existe pas : chemins resolus differents, dossiers differents
        return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--prereg', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('sessions', nargs='+')
    a = ap.parse_args()
    clash = [d for d in a.sessions if same_directory(a.out, d)]
    if clash:  # avant toute lecture et toute ecriture
        print('REFUS --out %s designe la session %s : ses fichiers seraient remplaces' % (a.out, clash[0]))
        return 2
    try:
        with open(a.prereg) as f:
            prereg = json.load(f, object_pairs_hook=decide.unique_keys)
        with open(a.prereg, 'rb') as f:
            psha = hashlib.sha256(f.read()).hexdigest()
    except (OSError, ValueError) as e:  # absent, illisible, JSON invalide ou mal encode, cle dupliquee
        print('REFUS preenregistrement absent ou illisible : %r' % (e,))
        return 2
    schema = decide.check_prereg(prereg)  # P6 : avant toute lecture de session
    if schema:
        for e in schema:
            print('REFUS preenregistrement hors schema : ' + e)
        return 2
    try:
        names = {m['name'] for m in prereg['methods']}
        specs, digest = decide.plan_specs(prereg)  # lit scenes.py ; famille ou niveau inconnu : ValueError
        pinned = prereg['plan']['manifest_sha256']
    except (OSError, SyntaxError, AttributeError, KeyError, TypeError, ValueError) as e:
        print('REFUS plan preenregistre illisible : %r' % (e,))
        return 2
    if digest != pinned:
        print('REFUS manifeste reconstruit %s different de l epingle %s' % (digest, pinned))
        return 2
    units = [decide.unit_name(s) for s in specs]
    plan = set(units)
    if len(plan) != len(units):
        print('REFUS plan preenregistre aux specifications dupliquees : %d noms d unites repetes' % (
            len(units) - len(plan)))
        return 2
    rows, segments, seen, columns = [], [], {}, None
    for d in a.sessions:
        try:
            with open(os.path.join(d, 'run.json')) as f:
                info = json.load(f, object_pairs_hook=decide.unique_keys)
        except (OSError, ValueError) as e:  # absent, illisible, cle dupliquee
            print('REFUS %s : run.json absent ou illisible : %r' % (d, e))
            return 2
        if not isinstance(info, dict):
            print('REFUS %s : run.json n est pas un objet JSON' % d)
            return 2
        if info.get('prereg_sha256') != psha:
            print('REFUS %s : autre preenregistrement' % d)
            return 2
        if info.get('plan_sha256') != pinned:
            print('REFUS %s : plan %s au lieu de l epingle %s' % (d, info.get('plan_sha256'), pinned))
            return 2
        segments.append(info)
        try:
            with open(os.path.join(d, 'results.csv')) as f:
                r = csv.DictReader(f)
                fields = r.fieldnames
                session_rows = list(r)
        except (OSError, ValueError, csv.Error) as e:
            print('REFUS %s : results.csv absent ou illisible : %r' % (d, e))
            return 2
        twice = decide.doubled(fields or ())
        if twice:  # DictReader garderait la derniere colonne d'un meme nom, et la fusion l'ecrirait deux fois
            print('REFUS %s : colonnes dupliquees dans l en-tete de results.csv : %s' % (d, ', '.join(twice)))
            return 2
        if not fields or 'unit' not in fields or 'method' not in fields or fields != (columns or fields):
            print('REFUS %s : colonnes de results.csv absentes ou differentes des sessions precedentes' % d)
            return 2
        columns = columns or fields
        by_unit = {}
        for row in session_rows:
            by_unit.setdefault(row['unit'], []).append(row)
        for unit, rs in by_unit.items():
            if unit not in plan:
                print('REFUS scene hors du plan preenregistre : %s dans %s' % (unit, d))
                return 2
            if unit in seen:
                print('REFUS scene calculee deux fois : %s (%s et %s)' % (unit, seen[unit], d))
                return 2
            if {x['method'] for x in rs} != names or len(rs) != len(names):
                print('REFUS scene incomplete : %s dans %s' % (unit, d))
                return 2
            seen[unit] = d
            rows += rs
    plans = {s['plan_sha256'] for s in segments}
    if len(plans) != 1:
        print('REFUS plans differents')
        return 2
    os.makedirs(a.out, exist_ok=True)
    with open(os.path.join(a.out, 'results.csv'), 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=columns)
        w.writeheader()
        for row in rows:
            w.writerow(row)
    total = len(specs)
    info = dict(prereg=os.path.basename(a.prereg), prereg_sha256=psha, plan_sha256=plans.pop(), scenes=total,
                computed=len(seen), complete=set(seen) == plan, segments=segments)
    with open(os.path.join(a.out, 'run.json'), 'w') as f:
        json.dump(info, f, indent=1, sort_keys=True)
    payload = '\n'.join(sorted(seen)).encode()
    with open(os.path.join(a.out, 'done.u32le'), 'wb') as f:
        f.write(struct.pack('<Q', len(payload)) + payload + b'\0' * ((-(8 + len(payload))) % 4))
    print('%d scenes sur %d fusionnees depuis %d session(s)' % (len(seen), total, len(segments)))
    return 0


if __name__ == '__main__':
    sys.exit(main())

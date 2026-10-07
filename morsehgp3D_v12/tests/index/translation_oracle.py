"""Invariance par translation, jugee a translation pres (docs/CONTRAT_NUMERIQUE.md, paragraphe 7).

Chaque nuage de test de l'index (fixtures.py) et son support sont translates jusqu'aux deux bords du domaine du
profil, [0, 2^B) (au profil 32 : [0, 2^32)), puis recenses par le census generique et par le census garde. Le juge
compare chaque reponse a celle du nuage d'origine A TRANSLATION PRES : meme statut, meme genre de certificat, memes
populations interieures et de coquille rangees par PointId (census complet ; un certificat saturant rend K temoins
quelconques, seul leur nombre est compare), memes sites et multiplicites ramenes par la translation inverse et ranges
par PointId. Les rangs de Morton, la forme de l'arbre et le travail du parcours ne sont pas compares :
la cle de Morton lit les coordonnees absolues et n'entre dans aucun ordre que cette porte juge. Python 3.10 nu.
"""
import hashlib
import json
import subprocess
import sys

from fixtures import fixtures
from fraction_model import population, require
from judge import Request, parse


def answers(probe, queries, flag):
    payload = ''.join(query.encode() for query in queries)
    command = [probe] + ([flag] if flag else [])
    result = subprocess.run(command, input=payload, capture_output=True, text=True, timeout=120)
    require(result.returncode == 0 and not result.stderr, 'pilote natif en echec : ' + result.stderr)
    lines = [parse(line) for line in result.stdout.splitlines()]
    require(len(lines) == len(queries), 'nombre de reponses')
    return lines, hashlib.sha256(payload.encode()).hexdigest()


def shifted(points, shift):
    return tuple(tuple(x + s for x, s in zip(point[:3], shift)) + tuple(point[3:]) for point in points)


def canonical(answer, shift):
    """Reponse ramenee par la translation inverse, sites et populations ranges par PointId."""
    if answer.get('status') == 'uncertified':
        return 'uncertified'
    if answer.get('kind') == 'refused':
        return ('refused', answer['status'], answer['reason'])
    sites = answer['sites']
    owners = answer['site_ids']
    back = sorted((tuple(ids), tuple(x - s for x, s in zip(site, shift))) for site, ids in zip(sites, owners))
    inner = sorted(tuple(owners[i]) for i in answer['inner'])
    shell = sorted(tuple(owners[i]) for i in answer['shell'])
    if answer['kind'] == 'saturated':
        # K temoins interieurs quelconques : lesquels depend de l'ordre de visite (Morton absolu), pas leur nombre.
        # Leur exactitude est jugee par l'oracle Fraction ; ici, le genre, le nombre et la coquille vide.
        return (answer['status'], answer['kind'], tuple(back), len(inner), tuple(shell))
    return (answer['status'], answer['kind'], tuple(back), tuple(inner), tuple(shell))


def matrix(bits):
    """Requetes d'origine et translatees aux deux bords ; une ligne par (fixture, seuil, translation)."""
    top = (1 << bits) - 1
    rows = []
    for fixture in fixtures(bits):
        coordinates = [record[:3] for record in fixture.records] + list(fixture.support)
        low = [min(p[j] for p in coordinates) for j in range(3)]
        high = [max(p[j] for p in coordinates) for j in range(3)]
        count = len(population(fixture.records, fixture.support)['inner'])
        for threshold in sorted({1, 3, max(1, count), count + 1, 2**32 - 1}):
            for name, shift in (('origine', (0, 0, 0)), ('bas', tuple(-x for x in low)),
                                ('haut', tuple(top - x for x in high))):
                rows.append((fixture.name, threshold, name, shift,
                             Request('%s_K%d_%s' % (fixture.name, threshold, name), shifted(fixture.records, shift),
                                     shifted(fixture.support, shift), threshold, 4)))
    return rows


def run(probe):
    info = subprocess.run([probe, '--profile'], capture_output=True, text=True, timeout=15)
    require(info.returncode == 0 and not info.stderr, 'profil natif en echec')
    bits = parse(info.stdout).get('coord_bits')
    require(type(bits) is int and bits in (21, 24, 32), 'profil absent')
    rows = matrix(bits)
    queries = [row[4] for row in rows]
    require(all(0 <= x < 1 << bits for q in queries for p in q.records + q.support for x in p[:3]), 'domaine')
    counts = dict(rows=len(rows), moved=0, low_edge=0, high_edge=0, certified=0, checks=0)
    digests = []
    for flag in ('', '--guarded'):
        replies, digest = answers(probe, queries, flag)
        digests.append(digest)
        reference = {}
        for (name, threshold, where, shift, query), reply in zip(rows, replies):
            value = canonical(reply, shift)
            key = (name, threshold)
            if where == 'origine':
                reference[key] = value
                counts['certified'] += flag == '--guarded' and value != 'uncertified'
                continue
            require(value == reference[key], '%s : reponse differente a translation pres (%s)' % (query.name, flag))
            counts['checks'] += 1
            moved = any(s != 0 for s in shift)
            counts['moved'] += moved
            points = query.records + query.support
            counts['low_edge'] += moved and any(min(p[j] for p in points) == 0 for j in range(3))
            counts['high_edge'] += moved and any(max(p[j] for p in points) == (1 << bits) - 1 for j in range(3))
    # Planchers sous les comptes observes aux trois profils (la plupart des nuages touchent deja 0 : la translation
    # vers le bas n'y deplace rien, celle vers le haut les porte au bord superieur).
    require(counts['checks'] >= 800 and counts['moved'] >= 400 and counts['low_edge'] >= 60 and
            counts['high_edge'] >= 350 and counts['certified'] >= 150, 'plancher : %s' % counts)
    print(json.dumps(dict(bits=bits, verdict='conforme', input_sha256=digests[0], **counts), sort_keys=True))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage : translation_oracle.py sonde', file=sys.stderr)
        sys.exit(2)
    try:
        run(sys.argv[1])
    except (ValueError, OSError, subprocess.TimeoutExpired, KeyError, TypeError) as error:
        print('ECHEC translation : ' + str(error), file=sys.stderr)
        sys.exit(1)

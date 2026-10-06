"""Modèle de durées de vie sur une jonction après refus. Aucun C++ exécuté."""
import argparse
import itertools
import json
from pathlib import Path


def check(condition, message):
    if not condition:
        raise ValueError(message)


def schedules(main_order):
    # ready : le fil a quitté wake.wait ; il ne consulte plus aborted avant gather.
    # R : refus du Pool ; C : fin de vie du tableau claimed ; A : abort ; J : join terminé.
    # Le fil lit un pointeur Q du tableau puis termine D. Join ne finit qu'après D.
    events = ['ready', 'R', 'C', 'A', 'J', 'Q', 'D']
    legal = []
    for row in itertools.permutations(events):
        at = {e:i for i,e in enumerate(row)}
        if not all(at[a] < at[b] for a,b in zip(main_order, main_order[1:])):
            continue
        if not (at['ready'] < at['Q'] < at['D'] < at['J'] and at['ready'] < at['R']):
            continue
        legal.append(dict(events=list(row), read_outside_lifetime=at['C'] < at['Q']))
    return legal


def proof():
    old = schedules(['R', 'C', 'A', 'J'])
    fixed = schedules(['R', 'A', 'J', 'C'])
    bad = [x for x in old if x['read_outside_lifetime']]
    check(old and fixed and bad, 'témoin permis attendu')
    check(not any(x['read_outside_lifetime'] for x in fixed), 'correction par ordre de déclaration')
    witness = ['ready', 'R', 'C', 'A', 'Q', 'D', 'J']
    check(any(x['events'] == witness for x in bad), 'abort ne rejoint pas un chunk déjà commencé')
    return dict(kind='modèle borné des événements ; native0/cloud0',
                limits='ne reproduit aucun diagnostic sanitizer, crash, nuage ou vitesse ; applique les obligations de durée de vie au chemin source',
                source_commit='cf28afb04040eacf1eb92080d75e8a0ebb58a4eb',
                original=dict(main_events=['R','C','A','J'], schedules=len(old),
                              schedules_reading_after_claimed_lifetime=len(bad), witness=witness),
                corrected=dict(main_events=['R','A','J','C'], schedules=len(fixed),
                               schedules_reading_after_claimed_lifetime=0),
                event_meanings=dict(ready='fil sorti de wake.wait pour un sous-lot prêt',
                                    R='Pool terminé avec Outcome de refus, return par MHGP11_TRY',
                                    C='fin de vie des objets du tableau claimed',
                                    A='~OverlapLane appelle abort, pas de cancellation de run_chunk déjà commencé',
                                    Q='gather_leaves charge un pointeur dans le tableau prêté',
                                    D='le fil termine', J='join terminé'),
                fix='déclarer claimed avant OverlapLane lane, donc détruire/join lane avant claimed',
                other_owners='outputs, queues, cloud, params, budget restent vivants pendant la jonction')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--check', type=Path)
    args = p.parse_args()
    value = proof()
    if args.check:
        check(json.loads(args.check.read_text()) == value, 'JSON figé différent')
        print('L4 lifetime conforme : témoin refus hors-vie ; ordre corrigé sans témoin ; native0 cloud0')
    else:
        print(json.dumps(value, sort_keys=True, ensure_ascii=False, indent=2))

#!/usr/bin/env python3
"""Interleavings abstraits W=3 ; ni moteur natif ni simulation du modèle mémoire C++."""
from collections import deque
import argparse
import hashlib
from itertools import product
import json
from pathlib import Path
from typing import NamedTuple


class Invalid(ValueError):
    pass


def need(ok, why):
    if not ok:
        raise Invalid(why)


class State(NamedTuple):
    job: int = 0
    phase: int = 0  # prepare, publish wakes, caller work, await done, retire
    published: int = 0
    current: int = -1
    team: int = 0
    count: int = 0
    next_chunk: int = 0
    hits: int = 0
    written: int = 0
    remaining: int = 0
    done: int = -1
    tokens: tuple = (-1, -1)
    # pc: idle, claim, callback in progress, write outcome, acknowledge, after acknowledge.
    # An actor may be arbitrarily delayed between any two transitions.
    actors: tuple = ((0, -1, -1), (0, -1, -1), (0, -1, -1))


def actor(s, w, value, **changes):
    actors = list(s.actors)
    actors[w] = value
    return s._replace(actors=tuple(actors), **changes)


def transitions(s, sequence, mutant=None):
    out = []
    if s.job < len(sequence):
        if s.phase == 0:
            need(s.current == -1 and s.done == -1 and s.tokens == (-1, -1), 'reutilisation sale')
            need(all(a[0] in (0, 5) for a in s.actors[1:]), 'Job retire avant acquittement')
            team = sequence[s.job]
            remaining = team - 1 if mutant == 'compteur_trop_court' and team else team
            out.append(actor(s, 0, (1, s.job, -1), phase=1, current=s.job, team=team,
                             count=team + 1, next_chunk=0, hits=0, written=0, published=0,
                             remaining=remaining))
        elif s.phase == 1:
            if s.published < s.team:
                w = s.published
                need(s.tokens[w] == -1, 'debordement semaphore go')
                tokens = list(s.tokens)
                tokens[w] = s.job
                out.append(s._replace(tokens=tuple(tokens), published=w + 1))
            else:
                out.append(s._replace(phase=2))
        elif s.phase == 3:
            ready = s.team == 0 or s.done == s.job or mutant == 'retour_sans_attendre'
            if ready:
                need(s.written == (1 << (s.team + 1)) - 1, 'retour avant toutes les issues')
                need(s.hits == (1 << s.count) - 1, 'retour avant tous les callbacks')
                need(s.remaining == 0, 'retour avant acquittements')
                out.append(s._replace(phase=4, done=-1))
        elif s.phase == 4:
            out.append(actor(s, 0, (0, -1, -1), phase=0, current=-1, job=s.job + 1))
    for w, (pc, epoch, pending) in enumerate(s.actors):
        if w == 0 and s.phase != 2:
            continue
        if pc == 0 and w and s.tokens[w - 1] != -1:
            epoch = s.tokens[w - 1]
            need(epoch == s.current and w <= s.team, 'lecture mauvais Job/equipe')
            tokens = list(s.tokens)
            tokens[w - 1] = -1
            out.append(actor(s, w, (1, epoch, -1), tokens=tuple(tokens)))
        elif pc == 1:
            need(epoch == s.current, 'claim sur Job expire')
            if s.next_chunk < s.count:
                out.append(actor(s, w, (2, epoch, s.next_chunk), next_chunk=s.next_chunk + 1))
            else:
                out.append(actor(s, w, (3, epoch, -1)))
        elif pc == 2:
            need(epoch == s.current, 'callback sur Job expire')
            need(not s.hits & (1 << pending), 'callback repete')
            out.append(actor(s, w, (1, epoch, -1), hits=s.hits | (1 << pending)))
        elif pc == 3:
            need(epoch == s.current and not s.written & (1 << w), 'issue repetee/Job expire')
            out.append(actor(s, w, (4, epoch, -1), written=s.written | (1 << w)))
        elif pc == 4:
            need(epoch == s.current, 'acquittement sur Job expire')
            if w:
                need(s.remaining > 0, 'compteur sous zero')
                remaining = s.remaining - 1
                if remaining == 0:
                    need(s.done == -1, 'debordement semaphore done')
                out.append(actor(s, w, (5, epoch, -1), remaining=remaining,
                                 done=epoch if remaining == 0 else s.done))
            else:
                out.append(actor(s, 0, (5, epoch, -1), phase=3))
        elif pc == 5 and w:
            # Crucial: no old Job access here; a next-job token can already be pending.
            out.append(actor(s, w, (0, -1, -1)))
    return out


def explore(sequence, mutant=None):
    first = State()
    queue, seen = deque([first]), {first}
    transitions_count = completed = token_before_wait = 0
    while queue:
        s = queue.popleft()
        token_before_wait += any(s.tokens[w - 1] != -1 and s.actors[w][0] == 5 for w in (1, 2))
        following = transitions(s, sequence, mutant)
        if not following:
            need(s.job == len(sequence) and all(a[0] == 0 for a in s.actors), 'blocage atteignable')
            need(s.current == -1 and s.tokens == (-1, -1) and s.done == -1, 'fermeture sale')
            completed += 1
        for new in following:
            transitions_count += 1
            if new not in seen:
                seen.add(new)
                queue.append(new)
    need(completed > 0, 'aucune terminaison')
    return dict(states=len(seen), transitions=transitions_count, final_states=completed,
                states_token_before_wait=token_before_wait)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, help='copie des sept fichiers source épinglés, facultative')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    if args.snapshot:
        capture = json.loads((here / 'capture.json').read_text())
        for path, pin in capture['files'].items():
            raw = (args.snapshot / path).read_bytes()
            need(len(raw) == pin['bytes'] and hashlib.sha256(raw).hexdigest() == pin['sha256'], 'pin source ' + path)
    results = {}
    for sequence in product(range(3), repeat=3):
        results[''.join(map(str, sequence))] = explore(sequence)
    killed = {}
    for mutant in ('compteur_trop_court', 'retour_sans_attendre'):
        try:
            explore((2, 0, 1), mutant)
        except Invalid as exc:
            killed[mutant] = str(exc)
        else:
            raise Invalid('mutant vivant ' + mutant)
    totals = {k: sum(v[k] for v in results.values()) for k in next(iter(results.values()))}
    need(totals['states_token_before_wait'] > 0, 'ouvrier retarde non couvert')
    result = dict(teams=3, invocations=3, sequences=27, pool_workers=3,
                  totals=totals, by_sequence=results, mutants=killed,
                  memory_model='abstract interleavings, C++ happens-before proved separately',
                  native_execution=False)
    if args.check:
        need(result == json.loads((here / 'results.json').read_text()), 'resultats differents')
        print('Pool: 27 sequences, 22766 etats, deux mutants refuses ; modele abstrait uniquement')
    else:
        print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()

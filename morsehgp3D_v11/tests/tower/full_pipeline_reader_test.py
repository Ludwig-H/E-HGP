#!/usr/bin/env python3
"""Lecteur du diagnostic de pipeline (bench/full_acceleration_diagnostics.py) : bornes par le mur, pas d'ordre impose.

Audit du 4 octobre 2026 (pin 66372e621) : sans barriere de depart, une voie de resolution peut finir avant qu'une autre
demarre ; le lecteur ne doit donc relier ni le dernier depart a la premiere fin, ni refuser ce FULL correct. Chaque
tache garde debut <= fin <= mur des forets, et l'ordre un n'a aucun balayage vertical. Bibliotheque standard seule.
"""
import copy
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'bench'))
import full_acceleration_diagnostics as acceleration

CHECKS = 0


def check(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(reason)


def need(value, reason):
    if not value:
        raise LookupError(reason)


def unsigned(event, keys):
    need(all(type(event[key]) is int and 0 <= event[key] < 2**64 for key in keys), 'unsigned event values')


def concurrent_full():
    """FULL concurrent minimal a K = 2 : la voie 1 finit (40) avant que la derniere voie demarre (55)."""
    orders = [dict(timings=dict(classify_ns=1, births_ns=1, plateaus_ns=5, verticals_ns=0), work=dict(population_hits=0)),
              dict(timings=dict(classify_ns=1, births_ns=1, plateaus_ns=5, verticals_ns=7), work=dict(population_hits=0))]
    tasks = dict(placement_cores=24, lanes_last_start_ns=55, lanes_first_finish_ns=40, lanes_last_finish_ns=90,
                 lanes_cpu_ns=300,
                 orders=[dict(k=1, publish_start_ns=2, publish_end_ns=95, publish_cpu_ns=30, publish_wait_ns=50,
                              vertical_start_ns=0, vertical_end_ns=0, vertical_cpu_ns=0, vertical_wait_ns=0),
                         dict(k=2, publish_start_ns=3, publish_end_ns=97, publish_cpu_ns=31, publish_wait_ns=52,
                              vertical_start_ns=4, vertical_end_ns=110, vertical_cpu_ns=20, vertical_wait_ns=70)])
    return dict(optimizations=8 + 8192, kmax=2, forest_ns=120, population_lookup=False, concurrent_orders=True,
                population_lookup_entries=0, population_lookup_reserved_bytes=0, reserved_after_bytes=10,
                memo_reserved_bytes=0, parallel=dict(lane_memo_reserved_bytes=0), census_workspace_reserved_bytes=0,
                regular_vertical_reserved_bytes=0, peak_reserved_bytes=10,
                phases=dict(classify_ns=2, births_ns=2, regular_ns=90, publish_ns=7, verticals_ns=13),
                orders=orders, pipeline_tasks=tasks)


def accepted(full):
    try:
        acceleration.validate(full, need, unsigned)
        return True
    except LookupError:
        return False


def mutated(change):
    full = concurrent_full()
    change(full)
    return full


def main():
    base = concurrent_full()
    check(base['pipeline_tasks']['lanes_last_start_ns'] > base['pipeline_tasks']['lanes_first_finish_ns'],
          'temoin : une voie finit avant le dernier depart')
    check(accepted(base), 'FULL correct sans barriere de depart refuse')
    # Ancienne relation : dernier depart <= premiere fin ; le meme FULL reste accepte quand elle tient aussi.
    check(accepted(mutated(lambda f: f['pipeline_tasks'].update(lanes_last_start_ns=40))), 'egalite depart/fin')
    tasks = lambda f: f['pipeline_tasks']  # noqa: E731
    refusals = {
        'last_start_after_wall': lambda f: tasks(f).update(lanes_last_start_ns=121),
        'last_finish_after_wall': lambda f: tasks(f).update(lanes_last_finish_ns=121),
        'first_finish_after_last': lambda f: tasks(f).update(lanes_first_finish_ns=91),
        'publish_end_before_start': lambda f: tasks(f)['orders'][1].update(publish_start_ns=98),
        'publish_end_after_wall': lambda f: tasks(f)['orders'][0].update(publish_end_ns=121),
        'vertical_end_before_start': lambda f: tasks(f)['orders'][1].update(vertical_end_ns=3),
        'vertical_end_after_wall': lambda f: tasks(f)['orders'][1].update(vertical_end_ns=121),
        'order_one_vertical_end': lambda f: tasks(f)['orders'][0].update(vertical_end_ns=5),
        'missing_lane_finish': lambda f: tasks(f).pop('lanes_last_finish_ns'),
        'missing_publish_end': lambda f: tasks(f)['orders'][0].pop('publish_end_ns'),
        'missing_vertical_end': lambda f: tasks(f)['orders'][1].pop('vertical_end_ns'),
        'negative_end': lambda f: tasks(f)['orders'][1].update(publish_end_ns=-1),
        'missing_placement': lambda f: tasks(f).pop('placement_cores'),
        'negative_placement': lambda f: tasks(f).update(placement_cores=-1),
    }
    for name, change in refusals.items():
        check(not accepted(mutated(change)), 'refus attendu : ' + name)
    # Voie sequentielle : aucun champ de pipeline non nul, fins comprises.
    sequential = concurrent_full()
    sequential.update(optimizations=8, concurrent_orders=False, phases=dict.fromkeys(sequential['phases'], 0))
    sequential['pipeline_tasks'] = copy.deepcopy(sequential['pipeline_tasks'])
    for key in acceleration.PIPELINE_LANES | {'placement_cores'}:
        sequential['pipeline_tasks'][key] = 0
    for row in sequential['pipeline_tasks']['orders']:
        for key in acceleration.PIPELINE_ORDER:
            row[key] = 0
    check(accepted(sequential), 'voie sequentielle a zeros refusee')
    placed = copy.deepcopy(sequential)
    placed['pipeline_tasks']['placement_cores'] = 24
    check(not accepted(placed), 'voie sequentielle avec un placement de pipeline acceptee')
    sequential['pipeline_tasks']['orders'][1]['publish_end_ns'] = 1
    check(not accepted(sequential), 'voie sequentielle avec une fin de tache acceptee')
    if CHECKS != 20:
        raise ValueError('plancher : %d controles' % CHECKS)
    print('full_pipeline_reader_verdict conforme checks%d' % CHECKS)


if __name__ == '__main__':
    main()

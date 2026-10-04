"""Gram/Fraction indépendant inchangé ; contrôle des arènes à partir des émissions de chaque ordinal.

Tailles x86_64 explicites, trois profils : Emission96/104/112 et Page80.
Les portes natives de stockage comparent également les octets réellement réservés aux sizeof.
"""
import argparse
import copy
import json
import subprocess

from fraction_oracle import requests
from fraction_model import require
from judge import canonical_answer, check_response, parse
from optimization_oracle import CACHE_FIELDS


def validate(req, row, reference, bits):
    checks = check_response(req, row, bits)
    require(row.get('single_pass') is True and type(row.get('workers')) is int and row['workers'] == 4,
            'voie une passe observee')
    require(canonical_answer(row) == canonical_answer(reference), 'catalogue exact divergent')
    require({k:v for k,v in row['ledger'].items() if k not in CACHE_FIELDS} ==
            {k:v for k,v in reference['ledger'].items() if k not in CACHE_FIELDS}, 'travail geometrique divergent')
    if req.refusal:
        require('execution' not in row and 'planning' not in row, 'publication partielle sur refus')
        return checks+4
    e, tasks = row['execution'], row['tasks']
    names = {'geometry_passes','arena_blocks','arena_capacity_bytes','arena_metadata_bytes',
             'compact_records','compact_population'}
    require(type(e) is dict and set(e) == names and all(type(v) is int and 0 <= v < 2**64 for v in e.values()),
            'inventaire execution entier')
    require(e['geometry_passes'] == 1 and type(row['planning']['replay_bytes']) is int and
            row['planning']['replay_bytes'] == 0, 'pas de rejeu geometrique')
    require(type(tasks) is list and len(tasks) <= (1024 if row['adaptive_frontier'] else 256), 'frontiere bornee')
    record_blocks = population_blocks = balls = population = 0
    for task in tasks:
        b, p = task['ledger']['emitted'], task['ledger']['incidences']
        require(type(b) is int and type(p) is int and min(b,p) >= 0, 'sorties par ordinal')
        record_blocks += (b+255)//256
        population_blocks += (p+2047)//2048
        balls += b
        population += p
        require(type(task['count_ns']) is int and type(task['fill_ns']) is int and
                task['count_ns'] == task['fill_ns'] == 0, 'anciennes phases inactives')
    require(balls == len(row['balls']) == e['compact_records'], 'inventaire des emissions')
    require(population == sum(b['p']+b['m'] for b in row['balls']) == e['compact_population'], 'inventaire des incidences')
    blocks = record_blocks + population_blocks
    require(e['arena_blocks'] == blocks and e['arena_metadata_bytes'] == 80*blocks, 'metadonnees budgetees')
    require(e['arena_capacity_bytes'] == record_blocks*256*{18:96,21:104,24:112}[bits]+population_blocks*2048*4,
            'capacites fixes exactes')
    require(row['peak'] >= e['arena_capacity_bytes']+e['arena_metadata_bytes'], 'pages reellement coexistantes')
    return checks+14


def selftest():
    from fixtures import records
    from judge import Request
    from model_test import truthful_response
    positives = corruptions = 0
    for bits in (18,21,24):
        req=Request('single-model',records(((0,0,0),(4,0,0))),1)
        base=truthful_response(req,bits)
        row=copy.deepcopy(base)
        row.update(single_pass=True,workers=4,adaptive_frontier=False)
        size=256*{18:96,21:104,24:112}[bits]+2048*4
        row['peak']=size+160
        row['execution']=dict(geometry_passes=1,arena_blocks=2,arena_capacity_bytes=size,arena_metadata_bytes=160,
                               compact_records=1,compact_population=2)
        row['planning']=dict(replay_bytes=0)
        row['tasks']=[dict(ledger=dict(emitted=1,incidences=2),count_ns=0,fill_ns=0)]
        validate(req,row,base,bits); positives+=1
        for change in (
            lambda r:r.update(single_pass=False), lambda r:r['execution'].update(geometry_passes=2),
            lambda r:r['execution'].update(arena_blocks=1),lambda r:r['execution'].update(arena_metadata_bytes=0),
            lambda r:r['execution'].update(arena_capacity_bytes=size-1),lambda r:r['execution'].update(compact_records=0),
            lambda r:r['execution'].update(compact_population=1),lambda r:r['planning'].update(replay_bytes=8),
            lambda r:r['tasks'][0].update(count_ns=1),lambda r:r['tasks'][0]['ledger'].update(emitted=2),
            lambda r:r.update(peak=size+159),lambda r:r['execution'].update(arena_blocks=True)):
            bad=copy.deepcopy(row); change(bad)
            try: validate(req,bad,base,bits)
            except (ValueError,KeyError,TypeError): corruptions+=1
            else: raise ValueError('corruption une passe survivante')
    require((positives,corruptions)==(3,36),'planchers modele')
    print('single_pass_model_verdict conforme positives3 corruptions36 native0')


def run(probe):
    options=dict(capture_output=True,text=True,encoding='utf-8',errors='backslashreplace')
    profile=subprocess.run([probe,'--profile'],timeout=15,**options)
    require(profile.returncode==0 and not profile.stderr,'profil refuse')
    bits=parse(profile.stdout)['coord_bits']; require(type(bits) is int and bits in (18,21,24),'profil invalide')
    batch,_,_=requests(bits); payload=''.join(req.encode() for req in batch)
    def execute(flags):
        result=subprocess.run([probe]+flags,input=payload,timeout=120,**options)
        require(result.returncode==0 and not result.stderr,'pilote refuse')
        lines=result.stdout.splitlines(); require(len(lines)==len(batch),'inventaire des reponses')
        return [parse(line) for line in lines]
    baseline=execute([])
    checks=0
    for flags in ([],['--adaptive-frontier','--cache-center-lines','--indirect-sort','--parallel-assembly']):
        actual=execute(['--single-pass','--workers','4']+flags)
        for req,row,reference in zip(batch,actual,baseline): checks+=validate(req,row,reference,bits)
    require(checks>30000 and len(batch)==378,'planchers Fraction une passe')
    print(json.dumps(dict(verdict='conforme',bits=bits,requests=1134,paired=756,refusals=60,checks=checks),sort_keys=True))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('probe',nargs='?'); parser.add_argument('--selftest',action='store_true')
    args=parser.parse_args()
    if args.selftest and args.probe is None: selftest()
    elif args.probe and not args.selftest: run(args.probe)
    else: parser.error('pilote ou --selftest')

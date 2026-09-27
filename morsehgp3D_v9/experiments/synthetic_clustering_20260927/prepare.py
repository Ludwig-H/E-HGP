#!/usr/bin/env python3
"""Prepare complete synthetic inputs on an exact common u18 grid per series.

No fit, geometry engine, label-based scale choice or input dropping. Grid
bounds use all planned sizes of each series, once, before measurement.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from fractions import Fraction
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import time
import traceback

import numpy as np

from plan import PLAN
from synthetic_data import generate

HERE = Path(__file__).resolve().parent
QUANTIZER = HERE.parents[1]/'audits/b_point_hierarchy_k_20260927/datasets.py'
QUANTIZER_SHA = '7ff3d93677f51eabe9c1bea0f56a62bbcd53ce531c3a49bdfafeab29ab14f53c'
LIMIT = 2**18-1


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False); stream.write('\n')


def frozen_quantizer():
    need(sha(QUANTIZER) == QUANTIZER_SHA, 'frozen quantizer source')
    spec = importlib.util.spec_from_file_location('synthetic_common_grid_quantizer', QUANTIZER)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


def grid_key(case):
    return tuple(case['spec'][key] for key in ('family', 'groups', 'separation', 'noise_fraction', 'seed'))


def quantize_on_grid(points, grid):
    """Exact rounding of represented binary64; truth is not an argument."""
    points = np.asarray(points, dtype=np.float64)
    need(points.ndim == 2 and points.shape[1] == 3 and len(points) >= 2 and np.isfinite(points).all(),
         'finite n by 3 coordinates required')
    origin = [Fraction(value) for value in grid['origin_exact']]
    step = Fraction(grid['step_exact'])
    need(step > 0 and len(origin) == 3, 'positive isotropic grid')
    values, maximum = [], Fraction(0)
    for row in points:
        exact = [Fraction(float(value)) for value in row]
        q = [round((value-base)/step) for value, base in zip(exact, origin)]
        need(all(0 <= value <= LIMIT for value in q), 'common-grid domain exceeded, never clipping')
        maximum = max(maximum, *(abs(base+value*step-x) for base,value,x in zip(origin,q,exact)))
        values.append(q)
    need(len({tuple(row) for row in values}) == len(values), 'duplicate/colliding sites refused, never merged')
    need(maximum <= step/2, 'exact half-cell error bound')
    return np.asarray(values, dtype='<u4'), dict(origin_exact=list(grid['origin_exact']),
        step_exact=grid['step_exact'], max_abs_error_exact=str(maximum),
        max_abs_error=float(maximum), euclidean_error_bound=float(step)*3**0.5/2,
        n_before=len(points), n_after=len(points), merged_rows=0, grid_duplicate_rows=0,
        limit=LIMIT, scheme='exact_binary64_nearest_ties_even_common_isotropic_u18',
        method_coordinates='same_uint32_sites_exactly_represented_as_float64_for_HDBSCAN')


def prepare(output):
    need(not output.exists(), 'NEW prepared-data directory required')
    output.mkdir(parents=True)
    paths = [HERE/name for name in ('prepare.py','plan.py','synthetic_data.py','PLAN.md')]+[QUANTIZER,Path(sys.executable).resolve()]
    pins = {str(path):sha(path) for path in paths}
    receipt = dict(schema='mhgp9_synthetic_preparation_receipt_v1',status='running',sources_before=pins,
                   python=sys.version,numpy_version=np.__version__,argv=sys.argv,GCP_used=False)
    save(output/'intent.json', receipt)
    started=time.perf_counter()
    try:
        save(output/'plan.json',PLAN)
        generated={}; groups=defaultdict(list)
        for case in PLAN['cases']:
            points,labels,parameters=generate(case['spec'])
            need(points.shape==(case['spec']['n'],3) and labels.shape==(len(points),), 'complete generated case')
            need(set(int(x) for x in labels if x >= 0)==set(range(1,case['spec']['groups']+1)), 'all prescribed components retained')
            generated[case['id']]=(points,labels,parameters)
            groups[grid_key(case)].append(case['id'])
        quantizer=frozen_quantizer(); grids={}
        for key, ids in groups.items():
            lower=np.min([generated[name][0].min(axis=0) for name in ids],axis=0)
            upper=np.max([generated[name][0].max(axis=0) for name in ids],axis=0)
            # Two opposite corners carry exactly the series bounds, without
            # duplicate observations or any access to component labels.
            _,_,grid=quantizer.quantize(np.asarray([lower,upper]),np.asarray([1,1]))
            grids[key]=grid
        manifest=dict(schema='mhgp9_synthetic_cluster_input_manifest_v1',plan=PLAN,
            plan_sha256=sha(output/'plan.json'),generator_source_sha256=sha(HERE/'synthetic_data.py'),
            common_grid_scope='family_groups_separation_noise_seed_across_all_planned_sizes',
            units='synthetic_model_units_not_metres',cases=[])
        for case in PLAN['cases']:
            points,labels,parameters=generated[case['id']]
            q,grid=quantize_on_grid(points,grids[grid_key(case)])
            directory=output/case['id'];directory.mkdir()
            raw_path=directory/'original.npy';float_path=directory/'points.npy';binary=directory/'points.u32le'
            np.save(raw_path,points,allow_pickle=False)
            np.save(float_path,q.astype(np.float64),allow_pickle=False)
            with binary.open('xb') as stream: stream.write(q.tobytes())
            save(directory/'labels.json',labels.tolist());save(directory/'parameters.json',parameters)
            entry=dict(case,n=len(points),groups=case['spec']['groups'],quantization=grid,
                true_counts={str(k):v for k,v in sorted(Counter(map(int,labels)).items())},
                grid_series_case_ids=groups[grid_key(case)],original_npy=str(raw_path),
                points_npy=str(float_path),points_u32le=str(binary),
                labels_json=str(directory/'labels.json'),parameters_json=str(directory/'parameters.json'))
            entry['files']={entry[name]:sha(entry[name]) for name in
                            ('original_npy','points_npy','points_u32le','labels_json','parameters_json')}
            entry['prepared_sha256']={name:entry['files'][entry[name]]
                                      for name in ('points_u32le','points_npy','labels_json')}
            save(directory/'case.json',entry);manifest['cases'].append(entry)
        save(output/'manifest.json',manifest)
        receipt.update(status='completed',manifest=str(output/'manifest.json'),
            manifest_sha256=sha(output/'manifest.json'),cases=len(manifest['cases']),
            quality_cases=sum(c['phase']=='quality' for c in manifest['cases']),
            growth_cases=sum(c['phase']=='growth' for c in manifest['cases']),
            clustering_executed=False,geometry_executed=False)
    except BaseException as error:
        receipt.update(status='failed',error=repr(error),traceback=traceback.format_exc())
        raise
    finally:
        receipt['sources_after']={path:sha(path) for path in pins}
        if receipt['sources_after']!=pins:
            receipt['status']='failed';receipt['closure_error']='source changed'
        receipt['elapsed_seconds']=time.perf_counter()-started
        save(output/'receipt.json',receipt)
    need(receipt['status']=='completed','preparation source closure')
    print(json.dumps({key:receipt[key] for key in ('status','manifest','cases','quality_cases','growth_cases','elapsed_seconds')}))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    prepare(parser.parse_args().output.resolve())

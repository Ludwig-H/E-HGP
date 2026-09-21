#!/usr/bin/env python3
"""Unweighted selected-edge work summary, never a full-front estimate."""
from collections import defaultdict
import json
from pathlib import Path
import sys
from campaign import require
from read import read


def summarize(rows):
    result = dict(calls=len(rows), empty_atlases=sum(r['preparation']['live_leaves']==0 for r in rows),
                  outputs=sum(r['baseline']['output_count'] for r in rows),
                  nonempty_outputs=sum(r['baseline']['output_count']>0 for r in rows))
    for mode in ('baseline','alive','join'):
        def total(category, field):
            return sum(r[mode][category][field] for r in rows)
        result[mode] = dict(seed_point_tests=total('generator','point_tests'),
                           seed_box_tests=total('generator','bound_tests'),
                           families=total('extra','family_preparations'),
                           atlas_visits=total('sweep','query_visits'),
                           line_tests=total('sweep','line_tests'),
                           products=total('extra','product_visits'),
                           block_bounds=total('extra','block_bound_tests'),
                           singleton_bounds=total('extra','singleton_bound_tests'),
                           leaves=total('sweep','leaf_queries'),
                           active_sites=total('sweep','active_sites'),
                           sort_comparisons=total('sweep','sort_comparisons'),
                           cache_hits=total('extra','family_cache_hits'),
                           cache_initialized=total('extra','cache_entries_initialized'),
                           blocks=total('extra','blocks'),
                           max_cache_bytes=max((r[mode]['extra']['cache_bytes_peak'] for r in rows), default=0),
                           max_stack=max((r[mode]['extra']['peak_product_stack'] for r in rows), default=0))
    return result


def main(folders):
    rows=[]
    reports=[]
    for folder in folders:
        value=read(folder)
        require(all(r['case'].startswith('lidar_') for r in value['rows']), 'LiDAR-only analysis')
        reports.append(dict(receipt=str(folder),summary=summarize(value['rows'])))
        rows.extend(value['rows'])
    grain64=[r for r in rows if r['grain']==64]
    groups=defaultdict(list)
    for r in grain64:
        groups[r['n']].append(r)
    return dict(status='passed',scope='unweighted selected edges, shared atlas; no global speed or growth claim',
                receipts=reports,all=summarize(rows),grain64=summarize(grain64),
                by_n_grain64={str(n):summarize(group) for n,group in sorted(groups.items())})


if __name__=='__main__':
    print(json.dumps(main([Path(p).resolve() for p in sys.argv[1:]]),sort_keys=True,indent=2))

#!/usr/bin/env python3
"""Lecture des seuls agregats captures ; aucune execution des sondes ni invention de temps individuels."""
import json
from fractions import Fraction
from pathlib import Path
import re
import statistics

HERE = Path(__file__).resolve().parent


def need(value, message):
    if not value:
        raise ValueError(message)


def main():
    capture = json.loads((HERE / 'capture.json').read_text())
    script = next(x['text'] for x in capture['excerpts'] if x['path'] == 'ab_local.py')
    need('res[b].append(min(walls, key=lambda w: w[0]))' in script and
         'ratios = [a / c for a, c in zip(w, base)]' in script and
         'statistics.median(ratios)' in script, 'formule capturee differente')
    need('walls[1:]' not in script, 'exclusion de passe nouvelle')
    rows=[]
    for item in capture['aggregate']:
        match = re.fullmatch(r'min ([0-9.]+) med ([0-9.]+) ms \(resolve min ([0-9.]+)\) digest ([0-9a-f]{16})(?: ratio med ([0-9.]+))?',item['numeric_excerpt'])
        need(match is not None,'agregat illisible')
        rows.append(dict(arm=item['arm'],min_ms=match[1],median_of_process_minima_ms=match[2],
                         resolve_min_ms=match[3],reported_digest_prefix16=match[4],
                         reported_median_paired_ratio=match[5] or '1'))
    base=Fraction(rows[0]['median_of_process_minima_ms'])
    for row in rows:
        ratio=Fraction(row['median_of_process_minima_ms']) / base
        row['ratio_of_rounded_medians_exact']=str(ratio)
        row['ratio_of_rounded_medians_decimal']=float(ratio)
    # Contre-exemple algebrique SANS UNITE, sans rapport avec les processus du banc.
    a,b=[1,100,101],[1,2,100]
    median_ratio=statistics.median(Fraction(x,y) for x,y in zip(a,b))
    ratio_medians=Fraction(statistics.median(a),statistics.median(b))
    need(median_ratio != ratio_medians,'contre-exemple')
    print(json.dumps(dict(aggregate_rows=rows,paired_ratios_independently_recomputed=False,
        missing_evidence='individual_passes_and_process_minima_not_archived',
        dimensionless_arithmetic_example=dict(a=a,b=b,median_ratios=str(median_ratio),
                                             ratio_medians=str(ratio_medians)),
        native_runs=0,cloud_calls=0),indent=1,sort_keys=True))


if __name__=='__main__':
    main()

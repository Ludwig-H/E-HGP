#!/usr/bin/env python3
"""Pilote local (agent G) : rejoue la sonde mhgp11_full_bench et hache le vidage MHGP11FUL1.

Usage : run_probe.py <bench> <data_dir> <out_jsonl> <case> <K> <leaf> <W> <mode> [passes] [--keep=<chemin>]
Ligne de commande de la sonde identique a bench/gpu_ab.py (tail = K leaf 256 0 4294967295 8589934592 W mode [passes]).
"""
import hashlib
import json
import os
import subprocess
import sys
import time


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 22), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--keep=')]
    keep = [a[len('--keep='):] for a in sys.argv[1:] if a.startswith('--keep=')]
    bench, data, out_jsonl, case, k, leaf, workers, mode = args[:8]
    passes = args[8] if len(args) > 8 else '1'
    dump = out_jsonl + '.%s_k%s_l%s_w%s_m%s_p%s.dump' % (case, k, leaf, workers, mode, passes)
    if os.path.exists(dump):
        os.unlink(dump)
    cmd = [bench, os.path.join(data, case + '.u32le'), os.path.join(data, case + '.ids.u32le'), dump,
           k, leaf, '256', '0', '4294967295', '8589934592', workers, mode]
    if passes != '1':
        cmd.append(passes)
    t0 = time.monotonic()
    done = subprocess.run(['/usr/bin/time', '-f', '%e %U %S %M'] + cmd, capture_output=True, text=True)
    elapsed = time.monotonic() - t0
    rows = {}
    pass_rows = []
    for line in done.stdout.splitlines():
        try:
            j = json.loads(line)
        except ValueError:
            continue
        if isinstance(j, dict) and 'phase' in j:
            if j['phase'] == 'pass':
                pass_rows.append(j)
            else:
                rows[j['phase']] = j
    time_line = done.stderr.strip().splitlines()[-1] if done.stderr.strip() else ''
    digest = sha256(dump) if os.path.exists(dump) else None
    size = os.path.getsize(dump) if os.path.exists(dump) else None
    full = rows.get('full', {})
    dom = rows.get('domain', {})
    rec = dict(case=case, k=int(k), leaf=int(leaf), workers=int(workers), mode=mode, passes=int(passes),
               cmd=' '.join(os.path.basename(c) if i == 0 else c for i, c in enumerate(cmd)), rc=done.returncode,
               status=full.get('status'), exit=rows.get('exit'), process_s=round(elapsed, 3),
               time_e_u_s_maxrsskb=time_line, dump_sha256=digest, dump_bytes=size,
               coord_bits=full.get('coord_bits'), optimizations=full.get('optimizations'),
               wall_ms=full.get('wall_ns', 0) / 1e6, domain_ms=full.get('domain_ns', 0) / 1e6,
               forest_ms=full.get('forest_ns', 0) / 1e6, cpu_s=full.get('cpu_seconds'),
               peak_reserved_bytes=full.get('peak_reserved_bytes'), catalogue_balls=dom.get('catalogue_balls'),
               single_pass_ms=dom.get('single_pass_ns', 0) / 1e6 if dom.get('single_pass_ns') else None,
               passes_wall_ms=[round(p.get('wall_ns', 0) / 1e6, 3) for p in pass_rows],
               passes_domain_ms=[round(p.get('domain_ns', 0) / 1e6, 3) for p in pass_rows],
               passes_forest_ms=[round(p.get('forest_ns', 0) / 1e6, 3) for p in pass_rows],
               stderr_tail=done.stderr[-1500:] if done.returncode != 0 else '')
    if keep and digest is not None:
        os.replace(dump, keep[0])
    elif os.path.exists(dump):
        os.unlink(dump)
    with open(out_jsonl, 'a') as f:
        f.write(json.dumps(rec) + '\n')
    print(json.dumps({x: rec[x] for x in ('case', 'k', 'leaf', 'workers', 'mode', 'passes', 'rc', 'status',
                                          'dump_sha256', 'dump_bytes', 'coord_bits', 'wall_ms', 'domain_ms',
                                          'forest_ms', 'cpu_s', 'peak_reserved_bytes', 'process_s',
                                          'time_e_u_s_maxrsskb', 'passes_wall_ms')}))
    return 0 if done.returncode == 0 else 1


if __name__ == '__main__':
    sys.exit(main())

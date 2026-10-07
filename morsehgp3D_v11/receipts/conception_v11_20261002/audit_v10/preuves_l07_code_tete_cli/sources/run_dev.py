"""Audit L07 : rejoue les scenes dev par la sonde (tete publiee V contre tete a cohortes C et N), 4 processus au
plus, 1 fil chacun. Sorties : un JSONL par scene et les etiquettes des seuls cas ou V et N different.

  python3 -B run_dev.py SCENES_DIR OUT_DIR KLIST ENTRIES MREACH [filtre d'indices pair|tous] [jobs]
"""
import json
import math
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

PROBE = '/tmp/v11-audit/l07_code_tete_cli/probe/l07_probe'
ZS = (1.0, 'zhat', 3.0, 4.0, 5.0, 6.0)


def configs(n, zhat):
    rows = []
    for mcs in (5, 15, int(round(math.sqrt(n)))):
        for z in ZS:
            rows.append((mcs, zhat if z == 'zhat' else z, 'eom', 0, 'zhat' if z == 'zhat' else '%g' % z))
    rows.append((int(round(math.sqrt(n))), 1.0, 'leaf', 0, '1'))
    return rows


def one(scene, src_dir, out_dir, klist, entries, mreach):
    name = scene['name']
    done = os.path.join(out_dir, name + '.jsonl')
    if os.path.exists(done):
        return name, 0.0, 0
    cfg = os.path.join(out_dir, name + '.cfg')
    rows = configs(scene['points'], scene['zhat'])
    with open(cfg, 'w') as f:
        for mcs, z, sel, single, _ in rows:
            f.write('%d %r %s %d\n' % (mcs, float(z), sel, single))
    with open(os.path.join(out_dir, name + '.cfgnames'), 'w') as f:
        json.dump([dict(mcs=r[0], z=r[1], sel=r[2], zname=r[4]) for r in rows], f)
    cmd = ['nice', '-n', '10', PROBE, 'run', os.path.join(src_dir, name + '.u32le'), os.path.join(out_dir, name),
           '--k-list=' + klist, '--entries=' + entries, '--configs=' + cfg, '--threads=1', '--dump=diff']
    if mreach:
        cmd.append('--mreach=' + mreach)
    t0 = time.time()
    r = subprocess.run(cmd, capture_output=True, text=True)
    with open(done + '.tmp', 'w') as f:
        f.write(r.stdout)
    os.replace(done + '.tmp', done)
    return name, time.time() - t0, r.returncode


def main():
    src_dir, out_dir, klist, entries, mreach = sys.argv[1:6]
    which = sys.argv[6] if len(sys.argv) > 6 else 'tous'
    jobs = int(sys.argv[7]) if len(sys.argv) > 7 else 4
    os.makedirs(out_dir, exist_ok=True)
    index = json.load(open(os.path.join(src_dir, 'index.json')))
    if which == 'pair':
        index = index[0::2]
    if which == 'quart':
        index = index[0::4]
    if which == 'huitieme':
        index = index[0::8]
    t0 = time.time()
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        futs = [pool.submit(one, s, src_dir, out_dir, klist, entries, mreach) for s in index]
        bad = 0
        for i, fu in enumerate(futs):
            name, dt, rc = fu.result()
            bad += rc != 0
            if (i + 1) % 16 == 0:
                print('%d/%d %.0f s' % (i + 1, len(index), time.time() - t0), flush=True)
    print('fini %d scenes, %d codes non nuls, %.0f s' % (len(index), bad, time.time() - t0))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Lentille 7 (auditeur C) : table chronologique R1 -> R7b depuis les sorties
BRUTES des recus G4 (vm/probe_N.stdout, vm/probe_N.stderr GNU time,
vm/probe_N.summary.json), jamais depuis les README ni SUMMARY.json.

Lecture seule. Sans assert (tient sous python3 -O)."""
import json
import re
import sys
from pathlib import Path

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else '/workspaces/E-HGP/build/v9-audit-c-worktree')
REC = ROOT / 'morsehgp3D_v9/receipts'
SESSIONS = [('R1', 'g4_tower_r1_20260922'), ('R2', 'g4_tower_r2_20260923'), ('R3', 'g4_tower_r3_20260923'),
            ('R4b', 'g4_tower_r4b_20260923'), ('R5', 'g4_tower_r5_20260923'), ('R6', 'g4_tower_r6_20260923'),
            ('R7b', 'g4_tower_r7b_20260923')]
FRAME = {39885: '000000', 35551: '000100', 45845: '000200'}
STAGES = ('prepare', 'gen_index', 'q2', 'q34', 'merge', 'tower_index', 'census', 'tower')
ALL_ON = None


def gnu_time(text):
    def grab(pattern, conv=float):
        m = re.search(pattern, text, re.M)
        return conv(m.group(1)) if m else None
    return dict(user=grab(r'User time \(seconds\): ([\d.]+)'), sys=grab(r'System time \(seconds\): ([\d.]+)'),
                rss=grab(r'Maximum resident set size \(kbytes\): (\d+)', int),
                minflt=grab(r'Minor \(reclaiming a frame\) page faults: (\d+)', int),
                vcs=grab(r'Voluntary context switches: (\d+)', int))


def load_rows():
    rows = []
    for tag, name in SESSIONS:
        d = REC / name
        pkg = json.loads((d / 'PACKAGE.json').read_text())
        summary = json.loads((d / 'SUMMARY.json').read_text()) if (d / 'SUMMARY.json').exists() else None
        idx = 0
        while (d / 'vm' / ('probe_%d.stdout' % idx)).exists():
            v = json.loads((d / 'vm' / ('probe_%d.stdout' % idx)).read_text())
            cs = json.loads((d / 'vm' / ('probe_%d.summary.json' % idx)).read_text())
            gt = gnu_time((d / 'vm' / ('probe_%d.stderr' % idx)).read_text(errors='replace'))
            t = v['times_ms']
            case = cs['case']
            stages = sum(t[k] for k in STAGES)
            digest_explicit = 'digest' in t
            orders = v['orders']
            row = dict(session=tag, commit=pkg['commit'][:8], schema=v['schema'], idx=idx,
                       frame=FRAME[v['input']['sites']], n=v['input']['sites'], K=v['options']['K'],
                       W=v['options']['workers'], static=v['options']['tower_static_threads'],
                       levers=(v['options'].get('levers') or
                               {k: v['options'][k] for k in ('atlas_saturate_deep', 'q3_leaf_census',
                                                               'q34_dead_lanes', 'q34_witness_cache',
                                                               'q34_dead_core', 'tower_meb_proposal')
                                if k in v['options']}), repeat=case.get('repeat', 0),
                       status=v['status'], digest=v['tower_digest'],
                       read=t['read'] / 1e3, prepare=t['prepare'] / 1e3, gen_index=t['gen_index'] / 1e3,
                       q2=t['q2'] / 1e3, q34=t['q34'] / 1e3, merge=t['merge'] / 1e3,
                       tower_index=t['tower_index'] / 1e3, census=t['census'] / 1e3, tower=t['tower'] / 1e3,
                       chain_total=t['chain_total'] / 1e3, digest_s=(t['digest'] / 1e3 if digest_explicit else None),
                       digest_in_chain=not digest_explicit,
                       residual=(t['chain_total'] - stages) / 1e3,
                       chain_cpu=v['chain_cpu_s'], wall_ext=cs['elapsed_seconds'],
                       gt_user=gt['user'], gt_sys=gt['sys'], gt_rss=gt['rss'], minflt=gt['minflt'],
                       peak_rss_kb=v['peak_rss_kb'], balls=v['catalogue']['balls'],
                       cat_bytes=v['catalogue']['bytes'],
                       presentations=v['catalogue']['q2_presentations'] + v['catalogue']['q3_presentations'] +
                       v['catalogue']['q4_presentations'],
                       nodes=sum(o['nodes'] for o in orders), parents=sum(o['parents'] for o in orders),
                       contributions=sum(o['contributions'] for o in orders),
                       nodes_topK=orders[-1]['nodes'] if orders else 0,
                       expanded_pairs=v['generator']['q34_expanded_pairs'],
                       q3e=v['generator']['q3_emitted'], q4e=v['generator']['q4_emitted'],
                       q2a=v['generator']['q2_accepted_pairs'])
            # chaine au perimetre ancien (digest inclus) et au perimetre R7b (digest exclu)
            if digest_explicit:
                row['chain_old'] = row['chain_total'] + row['digest_s']
                row['chain_new'] = row['chain_total']
            else:
                row['chain_old'] = row['chain_total']
                row['chain_new'] = None  # digest non separe : inconnu
            # recoupement du SUMMARY.json (non genere par un script versionne)
            if summary:
                srow = next((r for r in summary['rows'] if r['case'] == 'probe_%d' % idx), None)
                row['summary_row'] = srow
            rows.append(row)
            idx += 1
    return rows


def default_config(r):
    """Configuration 'produit' de chaque session : celle que le README retient."""
    lv = r['levers']
    if r['session'] == 'R1':
        return r['static'] == 0
    if r['session'] == 'R2':
        return True
    if r['session'] == 'R3':
        return lv.get('q34_dead_lanes', True)
    if r['session'] == 'R4b':
        return lv.get('q34_witness_cache', True)
    if r['session'] == 'R5':
        return True
    if r['session'] == 'R6':
        return lv.get('q34_dead_core', True)
    if r['session'] == 'R7b':
        return lv.get('tower_meb_proposal', True)
    return True


def main():
    rows = load_rows()
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    # 1. controle du SUMMARY.json contre les bruts
    mism = []
    for r in rows:
        s = r.get('summary_row')
        if not s:
            continue
        checks = dict(chain_s=(s.get('chain_s'), round(r['chain_total'], 2)),
                      q34_s=(s.get('q34_s'), round(r['q34'], 2)),
                      tower_s=(s.get('tower_s'), round(r['tower'], 2)),
                      balls=(s.get('balls'), r['balls']), digest=(s.get('digest'), r['digest']))
        if 'cpu_s' in s:
            checks['cpu_s_vs_gnutime'] = (s['cpu_s'], round(r['gt_user'] + r['gt_sys'], 1))
        if 'rss_kb' in s:
            checks['rss_kb'] = (s['rss_kb'], r['gt_rss'])
        if 'wall_s' in s:
            checks['wall_s'] = (s['wall_s'], round(r['wall_ext'], 2))
        for k, (a, b) in checks.items():
            if a is None:
                continue
            if isinstance(a, float) or isinstance(b, float):
                if abs(a - b) > 0.011 + 0.001 * abs(b):
                    mism.append((r['session'], r['idx'], k, a, b))
            elif a != b:
                mism.append((r['session'], r['idx'], k, a, b))
    print('== SUMMARY.json vs bruts : %d ecarts' % len(mism))
    for m in mism[:40]:
        print('   ', m)
    # 2. table complete
    hdr = ('ses', 'pkg', 'i', 'frame', 'K', 'W', 'st', 'rep', 'def', 'q2', 'q34', 'merge', 'census', 'tower',
           'other', 'digest', 'chain', 'chain_old', 'wall', 'cpu', 'gnuCPU', 'rssGiB', 'balls', 'nodes')
    print('\n== table brute (secondes ; other = prepare+gen_index+tower_index+residu hors digest si separe)')
    print(' | '.join(hdr))
    for r in rows:
        if r['digest_in_chain']:
            other = r['prepare'] + r['gen_index'] + r['tower_index'] + r['residual']  # residu contient le digest
            dig = 'in'
        else:
            other = r['prepare'] + r['gen_index'] + r['tower_index'] + r['residual']
            dig = '%.3f' % r['digest_s']
        print(' | '.join(str(x) for x in (
            r['session'], r['commit'], r['idx'], r['frame'], r['K'], r['W'], r['static'], r['repeat'],
            int(default_config(r)), '%.3f' % r['q2'], '%.3f' % r['q34'], '%.3f' % r['merge'], '%.3f' % r['census'],
            '%.3f' % r['tower'], '%.3f' % other, dig, '%.3f' % r['chain_total'], '%.3f' % r['chain_old'],
            '%.3f' % r['wall_ext'], '%.1f' % r['chain_cpu'], '%.1f' % (r['gt_user'] + r['gt_sys']),
            '%.2f' % (r['gt_rss'] / 2**20), r['balls'], r['nodes'])))
    if out:
        out.write_text(json.dumps([{k: v for k, v in r.items() if k != 'summary_row'} for r in rows], indent=1))
    return rows


if __name__ == '__main__':
    main()

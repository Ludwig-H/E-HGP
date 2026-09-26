#!/usr/bin/env python3
"""Lignes PROFA : repartition du temps echantillonne de la phase A, surcout rdtsc soustrait.

Chaque intervalle mesure porte le cout d'une lecture rdtsc (PROFA_CAL). s_rdtsc compte les
lectures ; on retranche s_rdtsc * cout a s_total et une lecture par intervalle a chaque poste.
Postes, d'apres le correctif :
  delim   L0->L1  fin du lot dans level_run, niveau du lot, lot_run, prefetch
  bloc    L1->L2  order_block_lean : compteurs, gardes, prefetches, order_root, push_back
          dont facettes tb0->tb1 (tout sauf le tri) et tri tb1->tb2
  lot     L2->L3  order_lot ; les sous-postes singleton/groupe ne le couvrent pas entierement
"""
import json, sys

def load(path):
    o, cal, tsc = [], None, None
    for line in open(path):
        if line.startswith('PROFA {'): o.append(json.loads(line[6:]))
        elif line.startswith('PROFA_CAL'): cal = json.loads(line[10:])
        elif line.startswith('PROFA_TSC'): tsc = json.loads(line[10:])['tsc_per_ns']
    return sorted(o, key=lambda a: a['K']), cal, tsc

def main(path, label):
    orders, cal, tsc = load(path)
    r = cal['rdtsc_pair_min']            # cout d'une lecture, borne basse
    print(f'== {label}   (1 lecture rdtsc = {r} tics min, {cal["rdtsc_pair_mean"]:.1f} moyen ; tsc/ns {tsc})')
    print(f"{'K':>2} {'cpu ms':>7} {'brut%':>6} | {'delim':>6} {'facet':>6} {'tri':>5} {'bloc-r':>6} "
          f"{'brouil':>6} {'noeud':>6} {'ancre':>6} {'grp-r':>6} {'reste':>6} | {'lots ech':>8} {'ecartes':>7}")
    for o in orders:
        n = {'delim': 1, 'blk_f': o['s_blocks'], 'blk_s': o['s_blocks'],
             's1d': o['s_singleton'], 'sg': o['s_groups']}
        s_net = o['s_total'] - o['s_rdtsc'] * r
        if s_net <= 0: continue
        sub = lambda v, k: max(0.0, v - k * r)
        delim = sub(o['s_disc'], o['s_lots'])
        facet = sub(o['s_blk_facets'], o['s_blocks'])
        srt = sub(o['s_blk_sort'], o['s_blocks'])
        blk_rest = max(0.0, o['s_blk_total'] - o['s_blk_facets'] - o['s_blk_sort'] - o['s_blocks'] * r)
        draft = sub(o['s1_draft'], o['s_singleton']) + sub(o['sg_draft'], o['s_groups'])
        node = sub(o['s1_node'], o['s_singleton']) + sub(o['sg_node'], o['s_groups'])
        anch = sub(o['s1_anchor'], o['s_singleton']) + sub(o['sg_anchor'], o['s_groups'])
        grp_r = max(0.0, o['sg_total'] - o['sg_node'] - o['sg_draft'] - o['sg_anchor'] - o['sg_setup'] - o['sg_union'] - o['sg_action'])
        named = delim + facet + srt + blk_rest + draft + node + anch + grp_r + \
                sub(o['sg_setup'], o['s_grouped']) + sub(o['sg_union'], o['s_grouped']) + sub(o['sg_action'], o['s_groups'])
        rest = max(0.0, s_net - named)
        pc = lambda x: 100.0 * x / s_net
        print(f"{o['K']:>2} {o['cpu_ns']/1e6:>7.1f} {100*o['s_rdtsc']*r/o['s_total']:>6.1f} | "
              f"{pc(delim):>6.1f} {pc(facet):>6.1f} {pc(srt):>5.1f} {pc(blk_rest):>6.1f} "
              f"{pc(draft):>6.1f} {pc(node):>6.1f} {pc(anch):>6.1f} "
              f"{pc(grp_r + sub(o['sg_setup'], o['s_grouped']) + sub(o['sg_union'], o['s_grouped']) + sub(o['sg_action'], o['s_groups'])):>6.1f} "
              f"{pc(rest):>6.1f} | {o['s_lots']:>8} {o['s_discarded']:>3}/{o['s_lots']+o['s_discarded']:<4}")
    print('  brut% = part des lectures rdtsc dans s_total (surcout retranche des colonnes)')

for i in range(1, len(sys.argv), 2):
    main(sys.argv[i], sys.argv[i+1]); print()

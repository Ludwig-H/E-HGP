#!/usr/bin/env python3
"""Lignes PROFA : repartition du temps echantillonne de la phase A, surcout rdtsc retranche.

Convention. s_total est l'intervalle L0->L3 du lot ; toutes les lectures sauf L0 tombent
dedans, soit (s_rdtsc - s_lots) lectures. On les retranche du denominateur. Chaque poste
nomme est corrige d'UNE lecture par intervalle, avec le bon nombre d'intervalles :
  delim, blk_facets, blk_sort, blk_rest : par lot / par bloc / par bloc / par bloc (tb0)
  s1_* : par lot singleton      sg_setup, sg_union, sg_anchor : par lot groupe
  sg_action, sg_node, sg_draft : par groupe
Le reste absorbe les entrees et sorties non couvertes et les lectures non imputees.
Postes, d'apres le correctif :
  delim   L0->L1  fin du lot dans level_run, niveau du lot, lot_run, prefetch
  bloc    L1->L2  order_block_lean, dont facettes tb0->tb1 et tri tb1->tb2
  lot     L2->L3  order_lot ; les sous-postes ne le couvrent pas entierement
"""
import json, sys

def load(path):
    o, cal, tsc = [], None, None
    for line in open(path):
        if line.startswith('PROFA {'): o.append(json.loads(line[6:]))
        elif line.startswith('PROFA_CAL'): cal = json.loads(line[10:])
        elif line.startswith('PROFA_TSC'): tsc = json.loads(line[10:])['tsc_per_ns']
    return sorted(o, key=lambda a: a['K']), cal, tsc

def split(o, r):
    """Postes corriges, en tics, et denominateur net."""
    sub = lambda v, k: max(0.0, v - k * r)
    b, g, s1, gg = o['s_blocks'], o['s_grouped'], o['s_singleton'], o['s_groups']
    p = {}
    p['delim'] = sub(o['s_disc'], o['s_lots'])
    p['facet'] = sub(o['s_blk_facets'], b)
    p['tri'] = sub(o['s_blk_sort'], b)
    p['bloc-r'] = sub(o['s_blk_total'] - o['s_blk_facets'] - o['s_blk_sort'], b)
    p['brouil'] = sub(o['s1_draft'], s1) + sub(o['sg_draft'], gg)
    p['noeud'] = sub(o['s1_node'], s1) + sub(o['sg_node'], gg)
    p['ancre'] = sub(o['s1_anchor'], s1) + sub(o['sg_anchor'], g)
    grp = sub(o['sg_setup'], g) + sub(o['sg_union'], g) + sub(o['sg_action'], gg)
    grp += max(0.0, o['sg_total'] - o['sg_setup'] - o['sg_union'] - o['sg_action']
               - o['sg_node'] - o['sg_draft'] - o['sg_anchor'])
    p['grp'] = grp
    net = o['s_total'] - (o['s_rdtsc'] - o['s_lots']) * r
    p['reste'] = net - sum(p.values())
    return p, net

def main(path, label):
    orders, cal, tsc = load(path)
    r = cal['rdtsc_pair_min']
    print(f'== {label}   (1 lecture rdtsc = {r} tics min, {cal["rdtsc_pair_mean"]:.1f} moyen ; tsc/ns {tsc})')
    print(f"{'K':>2} {'cpu ms':>7} {'brut%':>6} | {'delim':>6} {'facet':>6} {'tri':>5} {'bloc-r':>6} "
          f"{'brouil':>6} {'noeud':>6} {'ancre':>6} {'grp':>5} {'reste':>6} {'somme':>6} | {'lots ech':>8} {'lect/lot':>8} {'ecartes':>7}")
    for o in orders:
        p, net = split(o, r)
        if net <= 0: continue
        pc = lambda k: 100.0 * p[k] / net
        print(f"{o['K']:>2} {o['cpu_ns']/1e6:>7.1f} {100*(o['s_rdtsc']-o['s_lots'])*r/o['s_total']:>6.1f} | "
              f"{pc('delim'):>6.1f} {pc('facet'):>6.1f} {pc('tri'):>5.1f} {pc('bloc-r'):>6.1f} "
              f"{pc('brouil'):>6.1f} {pc('noeud'):>6.1f} {pc('ancre'):>6.1f} {pc('grp'):>5.1f} {pc('reste'):>6.1f} "
              f"{sum(pc(k) for k in p):>6.1f} | {o['s_lots']:>8} {o['s_rdtsc']/o['s_lots']:>8.1f} "
              f"{o['s_discarded']:>3}/{o['s_lots']+o['s_discarded']:<4}")
    print('  brut% = lectures rdtsc internes a s_total, retranchees du denominateur')

for i in range(1, len(sys.argv), 2):
    main(sys.argv[i], sys.argv[i+1]); print()

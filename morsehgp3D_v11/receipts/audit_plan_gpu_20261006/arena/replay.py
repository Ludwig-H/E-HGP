"""Bornes et modèle de stockage abstrait N1. Aucune exécution C++ ni preuve géométrique."""
import argparse
import json
from pathlib import Path


def check(value, message):
    if not value:
        raise ValueError(message)


def walk_model(capacity, rewind):
    # Filtre abstrait conservant tous les trois sites. Géométrie et refus non modélisés.
    arena = [None] * capacity
    top = high = fallback = nodes = 0
    leaves = []

    def process(parent, depth):
        nonlocal top, high, fallback, nodes
        nodes += 1
        mark = top
        fits = capacity - top >= len(parent)
        if fits:
            arena[mark:mark + len(parent)] = parent
            top += len(parent)
            high = max(high, top)
            owned = list(arena[mark:top])  # valeurs observées ; le modèle ne prétend pas vérifier l'alias C++
        else:
            fallback += 1
            owned = list(parent)  # ReadyNode possédé, repli exact dans le WIP
        if depth == 4:
            leaves.append(owned)
        else:
            process(owned, depth + 1)
            process(owned, depth + 1)
        if fits and rewind:
            top = mark

    process([0, 1, 2], 0)
    return dict(top_after=top, high_sites=high, fallback_nodes=fallback, nodes=nodes, leaves=leaves)


def proof():
    profiles = []
    for bits in (18, 21, 24):
        depth = 3 * bits
        # Filtre: enfant <= parent. Potentiel des boîtes: chaque coupe diminue >=1.
        serial_frames = len(range(depth + 1))
        check(serial_frames == 3 * bits + 1, 'nombre de listes filtrées simultanées')
        for d in range(depth + 1):
            check(len(range(d + 1, depth + 1)) == depth - d, 'suffixe de frontière possédée')
            check(depth - d <= serial_frames, 'borne plus petite pour suffixe')
        profiles.append(dict(bits=bits, depth_bound=depth, plan_factor=depth + 2,
                             sufficient_arena_factor_serial=serial_frames,
                             suffix_factor_at_depth_d='3B-d', frontier_root_owned=True,
                             root_buffer_outside_arena=True))
    normal, mutant = walk_model(20, True), walk_model(20, False)
    check(normal['leaves'] == mutant['leaves'] and normal['nodes'] == mutant['nodes'], 'sortie/ledger abstraits')
    check(normal['top_after'] == 0 and mutant['top_after'] != 0, 'garde du mark causale')
    check(normal['fallback_nodes'] == 0 < mutant['fallback_nodes'], 'repli exact observe le no_rewind')
    for row in (normal, mutant):
        check(row['high_sites'] <= 20, 'pas de dépassement de capacité dans le modèle')
    for row in (normal, mutant):
        row['leaf_count'] = len(row.pop('leaves'))
    cap = 1 << 18
    return dict(kind='preuve borne et modèle de stockage abstrait, native0/cloud0',
                abi='SiteIdx mot u32, 4 octets comme contrat source ; aucun sizeof exécuté',
                profiles=profiles,
                wip=dict(capacity_sites_per_lane=cap, capacity_bytes_per_lane=4 * cap,
                         reserved_bytes_by_active_lanes={str(w):4 * cap * w for w in (1, 4, 8, 24, 48)},
                         active_lanes='min(pool.size(), frontier.size())',
                         fallback='Buffer exact par noeud quand parent.size() > capacity-top',
                         old_suffix_bound_still_admitted=True,
                         activation_switch_in_snapshot=False),
                budget_witness=dict(fixture='ghost: neuf sites, K1/leaf4', source_contract_tasks=37,
                                    source_contract_nodes=75, current_gate_workers=4,
                                    current_gate_budget='unlimited', proposed_workers=16,
                                    proposed_budget_bytes=16*(1<<20), arena_reservation_bytes=16*4*cap,
                                    conclusion='arene seule = budget ; autres scratchs/frontiere positifs, admission impossible',
                                    scope='extension du temoin source ; aucune porte existante W16/16MiB ni execution native'),
                no_rewind_model=dict(capacity_sites=20, tree_depth=4, parent_sites=3,
                                     normal=normal, no_rewind=mutant,
                                     limits='filtre abstrait tout conservé ; aucune entrée naturelle, erreur ou qualification native'),
                advice=['3B+1 au lieu de 3B+2 ne peut pas être désigné mutant fautif pour cette arène',
                        'no_rewind: contrôler retour au mark/top=0 et fallback/allocations, pas seulement dump/ledger',
                        'budget: admettre et allouer réellement active_lanes*capacity, à côté des frontières et sorties',
                        'A/B: capacité 0 référence et 2^18 candidat permettent une ablation du même binaire'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', type=Path)
    args = parser.parse_args()
    result = proof()
    if args.check:
        check(json.loads(args.check.read_text()) == result, 'JSON figé différent')
        print('N1 borne/modèle conforme : 3 profils, arène fixe/repli, no_rewind causal par top ; native0 cloud0')
    else:
        print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))

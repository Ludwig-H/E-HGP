"""Plan du banc : un axe varie a la fois autour d'un point central.

Le produit cartesien complet (8 familles x 4 tailles x 5 nombres de groupes x
4 difficultes x 5 graines) ferait 12 800 scenes : la plupart n'apprendraient
rien. Le plan croise donc chaque axe avec le point central, puis ajoute deux
grilles serrees la ou la comparaison se joue : famille x difficulte, et
taille x nombre de groupes.

Point central : spherique, n = 2000, 8 groupes, difficulte moyenne, sans bruit.
Cinq graines partout. Les scenes sont nommees de facon stable et triees, donc
deux appels donnent le meme plan.
"""

import bench_datasets as data

CENTRE = dict(family='spherical', n=2000, groups=8, level='medium', noise_fraction=0.0)
SIZES = (500, 2000, 8000, 32000)
GROUPS = (2, 4, 8, 16, 32)
NOISE = (0.0, 0.1, 0.3)
SEEDS = 5
BASE_SEED = 2026092800


def _name(spec):
    return '%s_n%d_g%d_%s_noise%s' % (spec['family'], spec['n'], spec['groups'], spec['level'],
                                      ('%g' % spec['noise_fraction']).replace('.', 'p'))


def _axis(**changes):
    return dict(CENTRE, **changes)


def specifications(seeds=SEEDS, sizes=SIZES, heavy=True):
    """Rend la liste triee et dedoublonnee des scenes du banc."""
    axes = []
    # Axe 1 : la difficulte, pour chaque famille (32 combinaisons).
    for family in data.FAMILIES:
        for level in data.LEVELS:
            axes.append(_axis(family=family, level=level))
    # Axe 2 : la taille (le nombre de points), au point central.
    for n in sizes:
        if heavy or n <= 8000:
            axes.append(_axis(n=n))
    # Axe 3 : le nombre de groupes.
    for groups in GROUPS:
        axes.append(_axis(groups=groups))
    # Axe 4 : le bruit uniforme.
    for noise in NOISE:
        axes.append(_axis(noise_fraction=noise))
    # Grille serree taille x nombre de groupes, a difficulte moyenne et dure.
    for n in (500, 2000, 8000):
        for groups in (4, 16):
            for level in ('medium', 'hard'):
                axes.append(_axis(n=n, groups=groups, level=level))
    seen, plan = set(), []
    for spec in axes:
        name = _name(spec)
        if name in seen:
            continue
        seen.add(name)
        for replicate in range(seeds):
            full = dict(spec, seed=BASE_SEED + replicate)
            data.validate_spec(full)
            plan.append(dict(full, scene=name, replicate=replicate))
    plan.sort(key=lambda row: (row['scene'], row['replicate']))
    return plan


def summary(plan):
    families = sorted({row['family'] for row in plan})
    return dict(scenes=len({row['scene'] for row in plan}), runs=len(plan), families=families,
                sizes=sorted({row['n'] for row in plan}), groups=sorted({row['groups'] for row in plan}),
                levels=sorted({row['level'] for row in plan}),
                points_total=sum(row['n'] for row in plan))

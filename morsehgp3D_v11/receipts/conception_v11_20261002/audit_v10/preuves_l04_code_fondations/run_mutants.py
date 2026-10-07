#!/usr/bin/env python3
"""L04 audit : mutants causaux des fondations, juges par la seule porte unitaire mhgp10_unit (copies sous /tmp).

Chaque mutant modifie UNE ligne d'une copie de src/ ; on recompile les fichiers des fondations + tests/unit/unit_main.cpp
et on execute. « tue » = code de sortie non nul ou signal ; « survit » = unit_ok.
"""
import os
import shutil
import subprocess
import sys

SRC = sys.argv[1]
WORK = os.path.abspath('work')
MUTANTS = [
    ('M0_temoin', None, None, None),
    ('M1_side_facteur_2', 'src/arith/geometry.hpp',
     [('return c.D * (dx * dx + dy * dy + dz * dz) - 2 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);',
       'return c.D * (dx * dx + dy * dy + dz * dz) - 1 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);'),
      ('const i128 rhs = 2 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);',
       'const i128 rhs = 1 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);')], 'side et side_key : facteur 2 -> 1'),
    ('M2_center3_D', 'src/arith/geometry.hpp',
     [('out.D = 2 * (i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);',
       'out.D = 4 * (i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);')], 'center3 : D double'),
    ('M3_center4_signe', 'src/arith/geometry.hpp',
     [('i128 N0 = uu * vsx + vv * sux + ss * uvx,', 'i128 N0 = uu * vsx - vv * sux + ss * uvx,')], 'center4 : signe d\'un terme de N0'),
    ('M4_level3_den', 'src/arith/geometry.cpp',
     [('l.den = arith::I128w::from_u128(4 * ww);', 'l.den = arith::I128w::from_u128(2 * ww);')], 'level3 : denominateur'),
    ('M5_orient_center_wide', 'src/arith/geometry.cpp',
     [('return acc.sign();', 'return -acc.sign();')], 'orient_center_wide : signe inverse'),
    ('M6_compare', 'src/arith/geometry.cpp',
     [('return arith::cmp(arith::mul(a.num, b.den), arith::mul(b.num, a.den));',
       'return arith::cmp(arith::mul(a.num, a.den), arith::mul(b.num, b.den));')], 'compare : produits non croises'),
    ('M7_wide_mul_retenue', 'src/arith/wide.hpp',
     [('r.w[i + B] = carry;', 'r.w[i + B] = 0;')], 'Wide::mul : retenue finale perdue'),
    ('M8_wide_sub_emprunt', 'src/arith/wide.hpp',
     [('borrow = static_cast<u64>(d >> 64) ? 1 : 0;', 'borrow = 0;')], 'Wide::sub_mag : emprunt perdu'),
    ('M9_tree_marge_nulle', 'src/cloud/site_tree.cpp',
     [('constexpr double kMargin = 0.02;', 'constexpr double kMargin = 0.0;')], 'SiteTree : marge flottante nulle'),
    ('M10_tree_bande_inversee', 'src/cloud/site_tree.cpp',
     [('const double r2 = r2a + kMargin, inner = r2a - kMargin;', 'const double r2 = r2a + kMargin, inner = r2a + kMargin;')],
     'closed_ball : bande exacte supprimee cote coquille'),
    ('M11_tree_sans_tri_exact', 'src/cloud/site_tree.cpp',
     [('  std::sort(out.begin(), out.end());\n  if (out.size() > count) out.resize(count);',
       '  if (out.size() > count) out.resize(count);\n  std::sort(out.begin(), out.end());')], 'nearest : troncature avant le tri exact'),
    ('M12_kth_poids', 'src/cloud/site_tree.cpp',
     [('held += cloud_.w[s];', 'held += 1;')], 'kth_distance : multiplicites ignorees'),
    ('M13_cloud_tri_sans_pid', 'src/cloud/cloud.cpp',
     [('    return point_ids[a] < point_ids[b];', '    return false;')], 'prepare_cloud : departage par PointId supprime'),
    ('M14_morton_axes', 'src/cloud/cloud.cpp',
     [('return spread21(x) | (spread21(y) << 1) | (spread21(z) << 2);', 'return spread21(x) | (spread21(y) << 2) | (spread21(z) << 1);')],
     'morton3 : axes y et z echanges'),
    ('M15_pool_grain', 'src/sched/pool.cpp',
     [('(*job.body)(begin, std::min(job.n, begin + job.grain), id);', '(*job.body)(begin, std::min(job.n, begin + job.grain + 1), id);')],
     'Pool : tranche debordant d\'un indice'),
    ('M16_budget_release', 'src/core/buffer.hpp',
     [('      budget_->release(size_ * sizeof(T));', '      budget_->release(size_);')], 'Buffer : liberation en elements au lieu d\'octets'),
    ('M17_precedes', 'src/core/status.hpp',
     [('if (order != o.order) return order < o.order;', 'if (order != o.order) return order > o.order;')], 'Outcome::precedes : ordre inverse'),
]
FILES = ['tests/unit/unit_main.cpp', 'src/core/buffer.cpp', 'src/sched/pool.cpp', 'src/cloud/cloud.cpp', 'src/cloud/site_tree.cpp',
         'src/arith/geometry.cpp']
results = []
for name, path, edits, what in MUTANTS:
    if os.path.exists(WORK):
        shutil.rmtree(WORK)
    os.makedirs(WORK)
    shutil.copytree(os.path.join(SRC, 'src'), os.path.join(WORK, 'src'))
    shutil.copytree(os.path.join(SRC, 'tests'), os.path.join(WORK, 'tests'))
    if path:
        p = os.path.join(WORK, path)
        s = open(p).read()
        for old, new in edits:
            if old not in s:
                print('MUTANT INAPPLICABLE', name, old)
                sys.exit(2)
            s = s.replace(old, new)
        open(p, 'w').write(s)
    exe = os.path.join(WORK, 'unit')
    cmd = ['g++', '-std=c++20', '-O2', '-I' + os.path.join(WORK, 'src')] + [os.path.join(WORK, f) for f in FILES] + ['-lpthread', '-o', exe]
    b = subprocess.run(cmd, capture_output=True, text=True)
    if b.returncode != 0:
        results.append((name, what, 'ne compile pas'))
        continue
    try:
        r = subprocess.run([exe], capture_output=True, text=True, timeout=300)
        verdict = 'SURVIT' if (r.returncode == 0 and 'unit_ok' in r.stdout) else 'tue (code %d)' % r.returncode
    except subprocess.TimeoutExpired:
        verdict = 'tue (delai)'
    results.append((name, what or 'aucune mutation', verdict))
    print('%-28s %-52s %s' % results[-1], flush=True)
shutil.rmtree(WORK)

"""Petite campagne de mutants causaux contre la suite HEAD (13 portes) de morsehgp3D_v10 (afb081774).
Chaque mutant = remplacements exacts (motif present exactement une fois, sinon erreur du harnais) appliques a une
COPIE des sources sous /tmp. Jamais applique au depot. Usage :
  python3 mutants.py prepare NOM   -> cree mut/NOM/src (copie + mutation)
  python3 mutants.py list
"""
import os
import shutil
import sys

BASE = '/tmp/v11-audit/l08_tests_portes'
SRC = os.path.join(BASE, 'src', 'morsehgp3D_v10')

MUTANTS = {
    # AT1 (audit geant) : attache core a l'egalite, niveau <= e devient niveau < e
    'M01_core_strict': [('src/tower/tower.cpp',
                         'return arith::cmp(L.num, ed) <= 0;',
                         'return arith::cmp(L.num, ed) < 0;')],
    # AT1 cover : ancetre vivant au rang de la boule couvrante, sans le +1
    'M02_cover_rank': [('src/tower/tower.cpp',
                        'return v == kNone ? kNone : ancestor(out, o.jumps, v, cat.rank[b] + 1, sc.walk[k]);',
                        'return v == kNone ? kNone : ancestor(out, o.jumps, v, cat.rank[b], sc.walk[k]);')],
    # verticale d'une naissance : image prise un rang trop bas (un descendant de la bonne image)
    'M03_vertical_birth_low': [('src/tower/tower.cpp',
                                'up.lower[v] = ancestor(down, od.jumps, ball_vnode[ball], up.rank[v], sc.walk[k]);',
                                'up.lower[v] = ancestor(down, od.jumps, ball_vnode[ball], up.rank[v] - 1, sc.walk[k]);'),
                               ('src/tower/tower.cpp',
                                'up.lower[v] = ancestor(down, od.jumps, m, up.rank[v], sc.walk[k]);',
                                'up.lower[v] = ancestor(down, od.jumps, m, up.rank[v] - 1, sc.walk[k]);')],
    # contrat K = 10 : le catalogue refuse tout K >= 10
    'M04_refuse_k10': [('src/catalogue/generator.cpp',
                        'if (params.kmax < 1 || params.kmax > kMaxCatalogueOrder) return fail(Reason::kmax_out_of_range);',
                        'if (params.kmax < 1 || params.kmax >= 10) return fail(Reason::kmax_out_of_range);')],
    # temoin : seuil de masse de la condensation, >= devient >
    'M05_head_mcs_strict': [('src/head/head.cpp',
                             'if (mass[d.child_val[j]] >= p.min_cluster_size) big.push_back(d.child_val[j]);',
                             'if (mass[d.child_val[j]] > p.min_cluster_size) big.push_back(d.child_val[j]);'),
                            ('src/head/head.cpp',
                             'if (mass[u] >= p.min_cluster_size) {',
                             'if (mass[u] > p.min_cluster_size) {')],
    # exposant z ignore par la tete (lambda = r^-1 toujours)
    'M06_head_z_ignored': [('src/head/head.cpp',
                            'return std::pow(level, -0.5 * z);',
                            'return std::pow(level, -0.5 * (z > 0 ? 1.0 : 1.0));')],
    # temoin : le catalogue perd les boules q = 4 d'interieur non vide (admission p + q <= K + 1 -> p + q <= K pour q = 4)
    'M07_catalogue_admission': [('src/catalogue/generator.cpp',
                                 'const bool admit = (flags & kWeightedShell) ? p + 1 <= static_cast<u32>(C.K) : p + q <= static_cast<u32>(C.K) + 1;',
                                 'const bool admit = (flags & kWeightedShell) ? p + 1 <= static_cast<u32>(C.K) : p + q <= static_cast<u32>(C.K) + (q == 4 ? 0u : 1u);')],
    # regression du 29 sept. (niveaux exacts distincts, doubles egaux) : correctif retire
    'M08_level_collision_unfixed': [('src/tower/tower.cpp',
                                     'if (first || (distinct && x > d.level.back())) d.level.push_back(x);',
                                     'if (first || distinct) d.level.push_back(x);')],
    # defaut rare ET dependant de la magnitude (invariant par translation et par renumerotation) : une boule q2 de la
    # DERNIERE couche (p + q = K + 1) n'est pas emise si son diametre carre d2 depasse 2^24 et d2 % 2048 == 5.
    # Aucune naissance d'ordre <= K n'est perdue (ces boules sont des jonctions a l'ordre K) : fusions retardees.
    # Invisible aux oracles (coordonnees <= 998 : d2 <= 3 * 998^2 < 2^24).
    'M09_last_layer_rare_drop': [('src/catalogue/generator.cpp',
                                  '  if (!admit) return;\n  Rec r;',
                                  '  if (!admit) return;\n  if (q == 2 && p + q == static_cast<u32>(C.K) + 1) {\n'
                                  '    const P3 dd = geom::sub(C.P[sup[0]], C.P[sup[1]]);\n'
                                  '    const i64 d2 = geom::dot(dd, dd);\n'
                                  '    if (d2 > (i64{1} << 24) && d2 % 2048 == 5) return;\n  }\n  Rec r;')],
    # meme defaut sur TOUTES les couches : des naissances d'ordre <= K disparaissent aussi.
    'M10_any_layer_rare_drop': [('src/catalogue/generator.cpp',
                                 '  if (!admit) return;\n  Rec r;',
                                 '  if (!admit) return;\n  if (q == 2) {\n'
                                 '    const P3 dd = geom::sub(C.P[sup[0]], C.P[sup[1]]);\n'
                                 '    const i64 d2 = geom::dot(dd, dd);\n'
                                 '    if (d2 > (i64{1} << 24) && d2 % 2048 == 5) return;\n  }\n  Rec r;')],
}


def prepare(name):
    dst = os.path.join(BASE, 'mut', name, 'src', 'morsehgp3D_v10')
    if os.path.exists(os.path.join(BASE, 'mut', name)):
        shutil.rmtree(os.path.join(BASE, 'mut', name))
    shutil.copytree(SRC, dst)
    for rel, old, new in MUTANTS[name]:
        path = os.path.join(dst, rel)
        text = open(path).read()
        if text.count(old) != 1:
            print('HARNAIS : motif present %d fois dans %s : %r' % (text.count(old), rel, old))
            return 3
        open(path, 'w').write(text.replace(old, new))
    print('prepare', name, 'ok')
    return 0


if __name__ == '__main__':
    if sys.argv[1] == 'list':
        print(' '.join(MUTANTS))
        sys.exit(0)
    sys.exit(prepare(sys.argv[2]))

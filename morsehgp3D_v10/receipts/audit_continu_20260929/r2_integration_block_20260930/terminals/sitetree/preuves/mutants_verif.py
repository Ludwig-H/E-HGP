"""Mutants du verificateur r2 (groupe sitetree). Chaque mutant : remplacements textuels exacts dans
src/cloud/site_tree.cpp de r2 ; recompilation du seul objet avec les drapeaux du build r2 ; remplacement dans une copie
de libmhgp10_core.a ; edition de liens des portes (r2 et r1) ; execution. Un mutant est TUE si le code differe de 0."""
import os, shutil, subprocess, sys

V = '/tmp/mhgp10-r2/sitetree-verif'
SRC = V + '/r2/morsehgp3D_v10/src'
BUILD = V + '/build-r2'
FLAGS = ['-O3', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror']

ROUND = "  if (std::fegetround() != FE_TONEAREST) return false;"
FILT_DEF = "bool SiteTree::filtered(const geom::P3& a, const geom::Center& c) const {\n"
NEAR_CALL = "  if (!filtered(anchor, c)) {  // repli exact : les count"
BALL_CALL = "  if (!filtered(anchor, c)) {  // repli exact : cle de chaque site"
SKIP_DECL = "namespace { thread_local bool mut_skip_round = false; }\n"
SKIP_ROUND = "  if (!mut_skip_round && std::fegetround() != FE_TONEAREST) return false;"
FLAG = "    sites_u18_ = r.bmax[0] <= lim && r.bmax[1] <= lim && r.bmax[2] <= lim;"
LIM = "    const double lim = static_cast<double>(kCoordinateLimit);"
AXIS = "    if (av[i] < 0 || av[i] > kCoordinateLimit || c.N[i] < -kMaxN || c.N[i] > kMaxN) return false;"
GUARD = "  if (!sites_u18_ || c.D <= 0 || c.D > kMaxD) return false;"
ERR = '#if defined(__FAST_MATH__)'

def path_skip(call):
    return (call, "  mut_skip_round = true;\n  const bool mut_f = filtered(anchor, c);\n  mut_skip_round = false;\n"
            + call.replace("!filtered(anchor, c)", "!mut_f"))

MUTANTS = [
    # S-b : chemin reellement pris, decouple du predicat public filtered()
    ('chemin_nearest_sans_arrondi', 'S-b chemin', [(FILT_DEF, SKIP_DECL + FILT_DEF), (ROUND, SKIP_ROUND), path_skip(NEAR_CALL)]),
    ('chemin_closed_ball_sans_arrondi', 'S-b chemin', [(FILT_DEF, SKIP_DECL + FILT_DEF), (ROUND, SKIP_ROUND), path_skip(BALL_CALL)]),
    ('chemin_deux_sans_arrondi', 'S-b chemin', [(FILT_DEF, SKIP_DECL + FILT_DEF), (ROUND, SKIP_ROUND), path_skip(NEAR_CALL),
                                                path_skip(BALL_CALL)]),
    # S-b : variantes du controle
    ('arrondi_cache_thread_local', 'S-b', [(ROUND, "  thread_local const int mut_mode = std::fegetround();\n"
                                                   "  if (mut_mode != FE_TONEAREST) return false;")]),
    ('arrondi_upward_seul', 'S-b', [(ROUND, "  if (std::fegetround() == FE_UPWARD) return false;")]),
    ('arrondi_sans_towardzero', 'S-b', [(ROUND, "  if (std::fegetround() == FE_UPWARD || std::fegetround() == FE_DOWNWARD) return false;")]),
    ('arrondi_dans_nearest_seul', 'S-b', [(ROUND + "\n", ""), (NEAR_CALL, NEAR_CALL.replace("!filtered(anchor, c)",
                                          "std::fegetround() != FE_TONEAREST || !filtered(anchor, c)"))]),
    # S-a : drapeau des sites
    ('drapeau_axe_x_seul', 'S-a drapeau', [(FLAG, "    sites_u18_ = r.bmax[0] <= lim;")]),
    ('drapeau_sans_axe_z', 'S-a drapeau', [(FLAG, "    sites_u18_ = r.bmax[0] <= lim && r.bmax[1] <= lim;")]),
    ('drapeau_seuil_L_plus_1', 'S-a drapeau', [(LIM, "    const double lim = static_cast<double>(kCoordinateLimit + 1);")]),
    ('drapeau_seuil_19_bits', 'S-a drapeau', [(LIM, "    const double lim = static_cast<double>((i64{1} << 19) - 1);")]),
    ('drapeau_seuil_20_bits', 'S-a drapeau', [(LIM, "    const double lim = static_cast<double>((i64{1} << 20) - 1);")]),
    ('drapeau_bmin', 'S-a drapeau', [(FLAG, "    sites_u18_ = r.bmin[0] <= lim && r.bmin[1] <= lim && r.bmin[2] <= lim;")]),
    ('drapeau_strict', 'S-a drapeau', [(FLAG, "    sites_u18_ = r.bmax[0] < lim && r.bmax[1] < lim && r.bmax[2] < lim;")]),
    # S-a : ancre
    ('ancre_sans_borne_basse', 'S-a ancre', [(AXIS, AXIS.replace("av[i] < 0 || ", ""))]),
    ('ancre_sans_borne_haute', 'S-a ancre', [(AXIS, AXIS.replace("av[i] > kCoordinateLimit || ", ""))]),
    ('ancre_axe_x_seul', 'S-a ancre', [(AXIS, AXIS.replace("av[i] < 0 || av[i] > kCoordinateLimit ||",
                                                          "(i == 0 && (av[i] < 0 || av[i] > kCoordinateLimit)) ||"))]),
    ('ancre_bord_plus_un', 'S-a ancre', [(AXIS, AXIS.replace("av[i] > kCoordinateLimit", "av[i] > kCoordinateLimit + 1"))]),
    ('ancre_bord_moins_un', 'S-a ancre', [(AXIS, AXIS.replace("av[i] < 0", "av[i] < -1"))]),
    # S-c : bornes
    ('borne_D_stricte', 'S-c', [(GUARD, GUARD.replace("c.D > kMaxD", "c.D >= kMaxD"))]),
    ('borne_D_zero_admis', 'S-c', [(GUARD, GUARD.replace("c.D <= 0", "c.D < 0"))]),
    # Rejeu des mutants du correcteur (S-a, S-b), textes du verificateur
    ('R_garde_sans_nuage_u18', 'rejeu S-a', [(GUARD, "  if (c.D <= 0 || c.D > kMaxD) return false;")]),
    ('R_garde_sans_ancre', 'rejeu S-a', [(AXIS, "    if (c.N[i] < -kMaxN || c.N[i] > kMaxN) return false;")]),
    ('R_arrondi_absent', 'rejeu S-b', [(ROUND + "\n", "")]),
    ('R_arrondi_force_nearest', 'rejeu S-b', [(ROUND, "  if (std::fegetround() != FE_TONEAREST) std::fesetround(FE_TONEAREST);")]),
    ('R_arrondi_cache_statique', 'rejeu S-b', [(ROUND, "  static const int mut_m0 = std::fegetround();\n  if (mut_m0 != FE_TONEAREST) return false;")]),
    ('R_arrondi_seulement_closed_ball', 'rejeu S-b', [(ROUND + "\n", ""), (BALL_CALL, BALL_CALL.replace("!filtered(anchor, c)", "std::fegetround() != FE_TONEAREST || !filtered(anchor, c)"))]),
]

def sh(argv, cwd=None, timeout=900):
    try:
        r = subprocess.run(argv, cwd=cwd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return 'delai', ''
    return (r.returncode if r.returncode >= 0 else 'signal%d' % -r.returncode), r.stdout + r.stderr

def main():
    only = sys.argv[1].split(',') if len(sys.argv) > 1 else None
    gates = {'porte_r2': V + '/xgate/gate_r2.o', 'porte_r1': V + '/xgate/gate_r1.o'}
    base = open(SRC + '/cloud/site_tree.cpp').read()
    rows = []
    for name, fam, edits in [('temoin', '-', [])] + MUTANTS:
        if only and name not in only and name != 'temoin':
            continue
        text = base
        ok = True
        for old, new in edits:
            if text.count(old) != 1:
                ok = False
                print(name, 'MOTIF_ABSENT', repr(old[:60]))
                break
            text = text.replace(old, new)
        if not ok:
            rows.append((name, fam, 'motif_absent', '', '', ''))
            continue
        w = V + '/mut/work/' + name
        shutil.rmtree(w, ignore_errors=True)
        os.makedirs(w)
        cpp = w + '/site_tree.cpp'
        open(cpp, 'w').write(text)
        obj = w + '/site_tree.cpp.o'
        code, out = sh(['nice', '-n', '5', '/usr/bin/c++'] + FLAGS + ['-I' + SRC, '-c', cpp, '-o', obj])
        if code != 0:
            print(name, 'COMPILATION', out[:300])
            rows.append((name, fam, 'compilation', '', '', out.splitlines()[0][:100] if out else ''))
            continue
        lib = w + '/libmhgp10_core.a'
        shutil.copy(BUILD + '/libmhgp10_core.a', lib)
        sh(['ar', 'r', lib, obj])
        res = {}
        detail = ''
        for g, gobj in gates.items():
            exe = w + '/' + g
            c, o = sh(['nice', '-n', '5', '/usr/bin/c++', '-O3', '-DNDEBUG', gobj, '-o', exe, lib, '-pthread'])
            if c != 0:
                res[g] = 'lien'
                continue
            c, o = sh(['nice', '-n', '5', exe])
            res[g] = c
            open(w + '/' + g + '.stdout', 'w').write(o)
            first = [l for l in o.splitlines() if l.startswith('ECHEC') or 'plancher' in l]
            if first and not detail:
                detail = g + ': ' + first[0][:140]
        verdict = 'TUE' if res.get('porte_r2') != 0 else 'SURVIT'
        if name == 'temoin':
            verdict = 'TEMOIN_OK' if all(v == 0 for v in res.values()) else 'TEMOIN_ECHEC'
        rows.append((name, fam, verdict, str(res.get('porte_r2')), str(res.get('porte_r1')), detail))
        print('%-34s %-12s r2=%-4s r1=%-4s %-12s %s' % (name, fam, res.get('porte_r2'), res.get('porte_r1'), verdict, detail), flush=True)
    with open(V + '/mut/resultats' + ('_' + '_'.join(only) if only else '') + '.tsv', 'w') as f:
        f.write('mutant\tfamille\tverdict\tporte_r2\tporte_r1\tdetail\n')
        for r in rows:
            f.write('\t'.join(r) + '\n')

main()

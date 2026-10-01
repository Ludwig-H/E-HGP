"""Campagne de mutants de l'etape entrees_cli du raccord R2 (30 septembre 2026), jouee sur l'arbre integre.

Chaque mutant est un petit remplacement de source (ou le deplacement d'un bloc) applique a une COPIE de travail de
morsehgp3D_v10, reconstruit (build incremental), puis juge par les portes permanentes de l'arbre integre :
  unit     : mhgp10_unit (toutes les portes unitaires, dont les sorties test_output_set) ;
  fault    : mhgp10_fault output_set (chaque allocation d'une sequence de sorties echoue a son tour) ;
  produit  : tests/regression/test_cli_input_frontiers.py <build> produit ;
  temoin   : tests/regression/test_cli_input_frontiers.py <build> temoin ;
  fils     : tests/regression/test_thread_creation_refusal.py <build> (sorties presentes intactes) ;
  fast     : tests/regression/test_fast_targets.py (seulement pour les mutants de CMake et des portes fast).
Tue = au moins un juge rend un code non nul (code exact ; un signal ou un delai depasse n'est jamais un succes).
Motif absent ou multiple : erreur du harnais (code 3), jamais un mutant tue. Le temoin (copie non mutee) est joue
avant et apres la campagne : tous les juges doivent y rendre 0. La copie est remise a l'identique apres chaque mutant.
Python nu, sans assert (meme comportement sous python3 -O).

  python3 mutants_entrees_cli.py <copie de morsehgp3D_v10> <dossier de build de la copie> <journal.jsonl> [id...]
Codes : 0 campagne conforme (tous tues ou equivalents annonces survivants), 1 survivant non annonce ou equivalent tue,
3 harnais (motif, temoin, build).
"""
import json
import os
import shutil
import subprocess
import sys
import time

OPT = 'src/core/cli_options.hpp'
OUT = 'src/core/cli_output.hpp'
GEN = 'src/catalogue/generator.cpp'
RDR = 'src/cloud/u32le_input.hpp'
CAT = 'cli/mhgp10_catalogue.cpp'
TOW = 'cli/mhgp10_tower.cpp'
CLU = 'cli/mhgp10_cluster.cpp'
MRC = 'tests/head/mreach_cluster.cpp'
CMK = 'CMakeLists.txt'
FST = 'tests/regression/test_fast_targets.py'

BLOCK_DUMP = ("  cli::OutputSet outputs;\n  if (!dump.empty()) {\n"
              "    if (const Outcome o = outputs.add_input(argv[1]); !o.ok()) return print_refusal(o);\n"
              "    if (const Outcome o = outputs.declare(dump); !o.ok()) return print_refusal(o);\n"
              "    if (const Outcome o = outputs.reserve(); !o.ok()) return print_refusal(o);\n  }\n")


def rep(old, new):
    return ('rep', old, new)


def move(start, end, anchor):
    """Deplace le bloc [start ... end] (bornes comprises, chacune unique) juste apres la ligne anchor (unique)."""
    return ('move', start, end, anchor)


# (id, groupe, fichier, operations, description, equivalent annonce, juge fast)
MUTANTS = [
    # --- parseur strict (copie r2, MA*) ---
    ('MA1', 'r2', OPT, [rep("    if (!is_digit(c)) return fail(Reason::parameter_out_of_range);",
                            "    if (!is_digit(c)) break;")], 'prefixe numerique accepte', False, False),
    ('MA2', 'r2', OPT, [rep("  if (value < static_cast<i128>(lo) || value > static_cast<i128>(hi)) return fail(",
                            "  if (false && (value < static_cast<i128>(lo) || value > static_cast<i128>(hi))) return fail(")],
     'borne [lo, hi] non verifiee avant conversion', False, False),
    ('MA3', 'r2', OPT, [rep("    if constexpr (std::is_unsigned_v<T>) return fail(Reason::parameter_out_of_range);\n"
                            "    else if (lo >= 0) return fail(Reason::parameter_out_of_range);\n", "")],
     'signe - admis partout', False, False),
    ('MA4', 'r2', OPT, [rep("    if (magnitude > (std::numeric_limits<u64>::max() - d) / 10) return fail("
                            "Reason::parameter_out_of_range);\n", "")], 'depassement u64 non detecte', False, False),
    ('MA5', 'r2', OPT, [rep("  if (i != s.size()) return fail(Reason::parameter_out_of_range);\n  const std::string text(s);",
                            "  i = s.size();\n  const std::string text(s);")], 'grammaire des flottants retiree', False,
     False),
    ('MA5e', 'r2', OPT, [rep("  if (end != text.c_str() + text.size() || !std::isfinite(v) || v < lo || v > hi)",
                             "  if (end != text.c_str() + text.size() || v < lo || v > hi)")],
     'isfinite retire (equivalent : bornes finies de tous les appels)', True, False),
    ('MA6', 'r2', OPT, [rep("inline constexpr unsigned kMaxThreads = 1024;",
                            "inline constexpr unsigned kMaxThreads = 4294967295u;")], 'plafond de --threads retire',
     False, False),
    ('MA7', 'r2', OPT, [rep("    const Result<T> v = parse_integer<T>(item, lo, hi);\n",
                            "    if (item.empty()) { if (comma == std::string_view::npos) return out; pos = comma + 1; "
                            "continue; }\n    const Result<T> v = parse_integer<T>(item, lo, hi);\n")],
     'element vide de liste ignore', False, False),
    ('MA8', 'r2', OPT, [rep('(tok[2] != "eom" && tok[2] != "leaf") || ', '')], 'selection de --configs non jugee',
     False, False),
    # --- feuille, budget, garde des boules (copie r2, MB*, MC*) ---
    ('MB1', 'r2', GEN, [rep("!(params.allow_small_leaf && params.max_nodes > 0)", "!params.allow_small_leaf")],
     'petite feuille admise sans budget (equivalent a l\'integration : le diagnostic exige un budget plus haut)', True,
     False),
    ('MB2', 'r2', GEN, [rep("  if (params.leaf_size != 0 && params.leaf_size < K) return fail(Reason::parameter_out_of_range);\n",
                            "")], 'M < K admis avec l\'option', False, False),
    ('MB3', 'r2', GEN, [rep("C.nodes_seen.fetch_add(1, std::memory_order_relaxed) >= C.max_nodes) return;",
                            "C.nodes_seen.fetch_add(1, std::memory_order_relaxed) >= C.max_nodes && false) return;")],
     'arret anticipe retire', False, False),
    ('MB4', 'r2', GEN, [rep("  if (C.max_nodes != 0 && C.nodes_seen.load(std::memory_order_relaxed) > C.max_nodes) return "
                            "fail(Reason::node_budget);\n", "")], 'refus final node_budget retire', False, False),
    ('MB5', 'r2', GEN, [rep("C.nodes_seen.load(std::memory_order_relaxed) > C.max_nodes) return fail",
                            "C.nodes_seen.load(std::memory_order_relaxed) >= C.max_nodes) return fail")],
     'seuil final decale d\'un noeud', False, False),
    ('MB6e', 'r2', GEN, [rep("C.nodes_seen.fetch_add(1, std::memory_order_relaxed) >= C.max_nodes) return;",
                             "C.nodes_seen.fetch_add(1, std::memory_order_relaxed) > C.max_nodes) return;")],
     'seuil d\'arret anticipe decale (equivalent : meme issue)', True, False),
    ('MB7', 'r2', GEN, [rep("  if (effective_leaf_size(params) > params.max_leaf) return fail(Reason::parameter_out_of_range);\n",
                            "")], 'feuille > max_leaf admise avant calcul', False, False),
    ('MB8', 'r2', GEN, [rep("params.max_leaf < 1 || params.max_leaf > kMaxLeafBound", "params.max_leaf < 1")],
     'max_leaf non borne', False, False),
    ('MB9', 'r2', GEN, [rep("  C.max_nodes = params.max_nodes;",
                            "  C.max_nodes = params.allow_small_leaf ? params.max_nodes : 0;")],
     'budget ignore en mode produit', False, False),
    ('MC1', 'r2', GEN, [rep("  if (const Outcome guard = check_ball_count(first.back(), params.ball_limit); !guard.ok()) "
                            "return guard;\n", "")], 'garde des boules retiree', False, False),
    ('MC2', 'r2', GEN, [rep("return balls >= limit ? fail", "return balls > limit ? fail")], 'garde des boules (>)',
     False, False),
    ('MC3', 'r2', GEN, [rep("check_ball_count(first.back(), params.ball_limit)", "check_ball_count(first.back())")],
     'ball_limit ignore', False, False),
    # --- ordres de cluster (copie r2, MD*) ---
    ('MD1', 'r2', CLU, [rep("    if (u32(kk) > cloud.sites()) return print_refusal(fail(Reason::k_out_of_range));",
                            "    if (false && u32(kk) > cloud.sites()) return print_refusal(fail(Reason::k_out_of_range));")],
     'ordre K > sites admis', False, False),
    ('MD2', 'r2', CLU, [rep("    if (klist[i] < 1) return print_refusal(fail(Reason::k_out_of_range));\n", "")],
     'ordre K = 0 admis', False, False),
    ('MD3', 'r2', CLU, [rep("      if (klist[j] == klist[i]) return print_refusal(fail(Reason::parameter_out_of_range));",
                            "      if (false && klist[j] == klist[i]) return print_refusal(fail("
                            "Reason::parameter_out_of_range));")], 'doublon de --k-list admis', False, False),
    # --- sorties (copie r2, ME*, adaptes a cli::OutputSet integre) ---
    ('ME1', 'r2', OUT, [rep("    if (bad || !closed) return fail(Reason::output_unwritable);",
                            "    (void)bad;\n    (void)closed;")], 'ecriture non controlee', False, False),
    ('ME1b', 'r2', OUT, [rep("    const bool bad = std::fflush(f.get()) != 0 || std::ferror(f.get()) != 0;",
                             "    const bool bad = false;")], 'fflush et ferror non controles (fclose seul)', False,
     False),
    ('ME2', 'r2', CAT, [rep("    if (const Outcome o = outputs.declare(dump); !o.ok()) return print_refusal(o);\n"
                            "    if (const Outcome o = outputs.reserve(); !o.ok()) return print_refusal(o);\n", "")],
     'dump non declare ni reserve', False, False),
    ('ME3', 'r2', CAT, [rep("  if (!dump.empty()) {\n    const Outcome written",
                            "  std::printf(\"{\\\"status\\\":\\\"ok\\\"}\\n\");\n  if (!dump.empty()) {\n    const Outcome written")],
     'ligne status=ok avant le dump', False, False),
    ('ME4', 'r2', OUT, [rep("      if (e.temp_live) ::unlink(e.temp.c_str());", "      (void)e;")],
     'temporaires jamais retires sur refus', False, False),
    ('ME5', 'r2', OUT, [rep("  if (bad && code == 0) {", "  if (bad && code == 0 && false) {")],
     'sortie standard non controlee', False, False),
    ('ME6', 'r2', CLU, [rep("    if (!text.ok()) return print_refusal(text.outcome());", "    if (!text.ok()) return 2;")],
     '--configs illisible refuse sans ligne de statut', False, False),
    ('MF1', 'r2', CMK, [rep('set_tests_properties(mhgp10_regression_cli_input_frontiers_mreach PROPERTIES LABELS '
                            '"gate;regression" TIMEOUT 600)',
                            'set_tests_properties(mhgp10_regression_cli_input_frontiers_mreach PROPERTIES LABELS '
                            '"gate;regression;fast" TIMEOUT 600)')], 'partie temoin etiquetee fast', False, True),
    ('MG1', 'r2', RDR, [rep("  const Result<u64> points = check_u32le_size(bytes.size());\n  if (!points.ok()) return "
                            "points.outcome();\n  const u64 n = points.value();", "  const u64 n = bytes.size() / 12;")],
     'taille lue non jugee apres lecture (tube)', False, False),
    # --- prealable P2 : cli::OutputSet ---
    ('MP1', 'P2', OUT, [rep("      e.temp_live = true;\n",
                            "      e.temp_live = true;\n      if (e.exists) ::close(::open(e.dest.c_str(), O_WRONLY | O_TRUNC"
                            " | O_CLOEXEC));\n")], 'destination tronquee a la reservation (defaut r2)', False, False),
    ('MP2', 'P2', OUT, [rep("      if (e.temp_live) ::unlink(e.temp.c_str());",
                            "      if (e.temp_live) ::unlink(e.temp.c_str()), ::unlink(e.dest.c_str());")],
     'destination retiree sans commit (defaut r2)', False, False),
    ('MP3', 'P2', OUT, [rep("        if (same_file(e, other)) return fail(Reason::output_conflict);",
                            "        if (false && same_file(e, other)) return fail(Reason::output_conflict);")],
     'sorties concurrentes admises', False, False),
    ('MP4', 'P2', OUT, [rep("    return (a.exists && b.exists && a.dev == b.dev && a.ino == b.ino) || (!a.dest.empty() && "
                            "a.dest == b.dest);", "    return !a.dest.empty() && a.dest == b.dest;")],
     'identite par chemin seul (liens physiques ignores)', False, False),
    ('MP5', 'P2', OUT, [rep("      if (!S_ISLNK(ls.st_mode)) break;", "      if (true) break;")],
     'liens symboliques non resolus', False, False),
    ('MP6', 'P2', OUT, [rep("        if (e.exists && e.dev == k.dev && e.ino == k.ino) return fail(Reason::output_conflict);",
                            "        (void)k;")], 'sortie = entree admise', False, False),
    ('MP7', 'P2', OUT, [rep("      if (!e.optional && !e.written) return fail(Reason::output_unwritable);", "      (void)e;")],
     'commit sans exiger toutes les sorties', False, False),
    ('MP8', 'P2', OUT, [rep("  void operator()(std::FILE* f) const noexcept { std::fclose(f); }",
                            "  void operator()(std::FILE*) const noexcept {}")],
     'FILE* sans proprietaire (ecrivain qui leve : descripteur perdu)', False, False),
    ('MP9', 'P2', OUT, [rep("    } catch (const std::bad_alloc&) {\n      return fail(Reason::memory_budget);  // fichier ferme",
                            "    } catch (const std::length_error&) {\n      return fail(Reason::memory_budget);  // fichier ferme")],
     'bad_alloc de l\'ecrivain non converti', False, False),
    ('MP10', 'P2', OUT, [rep("      e.temp = std::move(t);  // noexcept : aucune operation qui leve entre la creation et "
                             "l'enregistrement\n",
                             "      const std::string copy = t + \"\";\n      e.temp = copy;\n")],
     'temporaire enregistre apres une allocation', False, False),
    ('MP11', 'P2', OUT, [rep("      if (!S_ISREG(st.st_mode)) {  // peripherique, tube, socket : ecrit directement",
                             "      if (false) {  // peripherique, tube, socket : ecrit directement")],
     'sortie speciale traitee comme reguliere', False, False),
    ('MP12', 'P2', OUT, [rep("      if (::access(path.c_str(), W_OK) != 0) return fail(Reason::output_unwritable);\n"
                             "      if (!S_ISREG(st.st_mode)) {", "      if (!S_ISREG(st.st_mode)) {")],
     'fichier sans droit d\'ecriture remplace', False, False),
    ('MP13', 'P2', OUT, [rep("      const bool mode_ok = !e.exists || ::fchmod(fd, e.mode) == 0;",
                             "      const bool mode_ok = true;")], 'droits de la destination non repris', False, False),
    ('MP14', 'P2', CLU, [rep("        if (vote && es.entry == PointEntry::cover)\n", "        if (false)\n")],
     'etiquettes .vote non declarees', False, False),
    ('MP15', 'P2', CLU, [rep("    if (const Outcome o = outputs.add_input(configs); !o.ok()) return print_refusal(o);\n",
                             "    {}\n")], '--configs non enregistre comme entree', False, False),
    ('MP16', 'P2', CAT, [rep("    if (const Outcome o = outputs.add_input(argv[1]); !o.ok()) return print_refusal(o);\n",
                             "")], 'entree du catalogue non enregistree', False, False),
    ('MP17', 'P2', TOW, [rep("      if (v.empty()) bad = fail(Reason::parameter_out_of_range);  // --dump= vide", "      if (false) bad = fail(Reason::parameter_out_of_range);  // --dump= vide")],
     '--dump= vide admis (tour)', False, False),
    ('MP18', 'P2', CLU, [rep("      if (v.empty()) bad = fail(Reason::parameter_out_of_range);  // --tree= vide",
                             "      if (false) bad = fail(Reason::parameter_out_of_range);  // --tree= vide")],
     '--tree= vide admis', False, False),
    ('MP19', 'P2', MRC, [rep("      if (v.empty()) bad = fail(Reason::parameter_out_of_range);  // --configs= vide",
                             "      if (false) bad = fail(Reason::parameter_out_of_range);  // --configs= vide")],
     '--configs= vide admis (temoin)', False, False),
    ('MP20', 'P2', GEN, [rep("  if (params.allow_small_leaf && params.max_nodes == 0) return fail(Reason::parameter_out_of_range);\n",
                             "")], '--allow-small-leaf sans budget admis', False, False),
    # --- prealable P4 ---
    ('MQ1', 'P4', CMK, [rep('    set_property(TEST ${mhgp10_test} APPEND PROPERTY ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")',
                            '    # retire')], 'portes Python sans PYTHONDONTWRITEBYTECODE', False, True),
    ('MQ2', 'P4', CMK, [rep('           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}\n'
                            '                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/points/test_cover_band.py"\n'
                            '                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)',
                            '           COMMAND ${Python3_EXECUTABLE} -B ${PROJECT_SOURCE_DIR}/tests/points/test_cover_band.py)')],
     'porte fast de la tete remise dans sa forme d\'avant P4 (python3 -B, sans run_expect)', False, True),
    ('MQ3', 'P4', FST, [rep("            env.update(environment(props))  # environnement de la porte, comme sous ctest\n", "")],
     'rejeu fast sans l\'environnement de la porte (equivalent : aucune porte fast ne depend de son environnement)',
     True, True),
    # --- cas isolants du verificateur interrompu (X*) ---
    ('XA3', 'X', OPT, [rep("    if (exponent == 0) return fail(Reason::parameter_out_of_range);\n", ""),
                       rep("  if (end != text.c_str() + text.size() || !std::isfinite(v) || v < lo || v > hi)",
                           "  if (end == text.c_str() || !std::isfinite(v) || v < lo || v > hi)")],
     'exposant sans chiffre (1e) admis : grammaire et fin de lecture de strtod relachees', False, False),
    ('XA3e', 'X', OPT, [rep("    if (exponent == 0) return fail(Reason::parameter_out_of_range);\n", "")],
     'grammaire seule relachee (equivalent : strtod s\'arrete avant e, la fin de lecture refuse 1e)', True, False),
    ('XA5', 'X', OPT, [rep("    io_error = std::ferror(f) != 0 || !std::feof(f);", "    io_error = false;")],
     '--configs dossier lu comme vide', False, False),
    ('XA8', 'X', CLU, [rep("          if (entries[j].tag == entries.back().tag) bad = fail(",
                           "          if (false) bad = fail(")], '--entry en double admis', False, False),
    ('XB2', 'X', CAT, [move("  if (const Outcome o = check_catalogue_params(params); !o.ok()) return print_refusal(o);\n",
                            "  if (const Outcome o = check_catalogue_params(params); !o.ok()) return print_refusal(o);\n",
                            "  if (!input.ok()) return print_refusal(input.outcome());\n")],
     'parametres du catalogue juges apres la lecture', False, False),
    ('XB2t', 'X', TOW, [rep("  if (const Outcome o = check_catalogue_params(cp); !o.ok()) return print_refusal(o);\n", "")],
     'K de la tour juge apres la reservation (par build_catalogue)', False, False),
    ('XB2c', 'X', CLU, [rep("  if (const Outcome o = check_catalogue_params(catp); !o.ok()) return print_refusal(o);\n", "")],
     'ordre du catalogue de cluster juge apres la lecture', False, False),
    ('XD1', 'X', CLU, [rep("    if (u32(kk) > cloud.sites()) return print_refusal(fail(Reason::k_out_of_range));",
                           "    if (u32(kk) > n) return print_refusal(fail(Reason::k_out_of_range));")],
     'borne K <= points au lieu de K <= sites', False, False),
    ('XD1m', 'X', MRC, [rep("  if (k > n) return refuse(fail(Reason::k_out_of_range));",
                            "  if (k > cloud.sites()) return refuse(fail(Reason::k_out_of_range));")],
     'temoin : borne K <= sites au lieu de K <= points', False, False),
    ('XE1', 'X', CAT, [move("  cli::OutputSet outputs;\n", "  }\n  // Fils du pool",
                            "  if (!built.ok()) return print_refusal(built.outcome());\n")],
     'catalogue : sorties declarees et reservees apres le calcul', False, False),
    ('XE2', 'X', TOW, [move("  cli::OutputSet outputs;\n", "  }\n  // Fils du pool", "  const Tower& tower = tw.value();\n")],
     'tour : sorties declarees et reservees apres le calcul', False, False),
    ('XE3', 'X', CLU, [move("  cli::OutputSet outputs;\n", "  if (const Outcome o = outputs.reserve(); !o.ok()) return "
                            "print_refusal(o);\n", "  if (!cat.ok()) return print_refusal(cat.outcome());\n")],
     'cluster : sorties declarees et reservees apres le catalogue', False, False),
    ('XE4', 'X', MRC, [move("  cli::OutputSet outputs;\n", "  if (const Outcome o = outputs.reserve(); !o.ok()) return "
                            "refuse(o);\n", "  sched::Pool& pool = *made.value();\n")],
     'temoin : sorties declarees et reservees apres la creation du pool', False, False),
    ('XE6', 'X', CLU, [rep("        if (!written.ok()) return print_refusal(written);\n      }\n    }\n    if (!tree_out.empty())",
                           "        if (!written.ok() && variant == 0) return print_refusal(written);\n      }\n    }\n"
                           "    if (!tree_out.empty())")], 'echec d\'ecriture des .vote ignore', False, False),
]


def apply_ops(text, ops):
    """Texte mute, ou (None, motif) si un motif est absent ou multiple."""
    for op in ops:
        if op[0] == 'rep':
            _, old, new = op
            if text.count(old) != 1:
                return None, 'motif %d fois : %r' % (text.count(old), old[:60])
            text = text.replace(old, new, 1)
        else:
            _, start, end, anchor = op
            i = text.find(start)
            if i < 0 or text.count(start) != 1:
                return None, 'debut de bloc %d fois' % text.count(start)
            j = text.find(end, i)
            if j < 0:
                return None, 'fin de bloc absente'
            j += len(end)
            if end.endswith('// Fils du pool'):  # la fin est le debut du bloc suivant : garder ce commentaire
                j -= len('  // Fils du pool')
            block = text[i:j]
            text = text[:i] + text[j:]
            if text.count(anchor) != 1:
                return None, 'ancre %d fois' % text.count(anchor)
            k = text.find(anchor) + len(anchor)
            text = text[:k] + block + text[k:]
    return text, None


def run(argv, timeout, env):
    start = time.monotonic()
    try:
        r = subprocess.run(argv, capture_output=True, text=True, timeout=timeout, env=env)
        code, out = r.returncode, (r.stdout + r.stderr)
    except subprocess.TimeoutExpired:
        code, out = 'delai_%ds' % timeout, ''
    return code, out, time.monotonic() - start


def judges(src, build, with_fast, env):
    ctest = shutil.which('ctest') or 'ctest'
    gate = os.path.join(src, 'tests/regression/test_cli_input_frontiers.py')
    fils = os.path.join(src, 'tests/regression/test_thread_creation_refusal.py')
    fast = os.path.join(src, FST)
    v = {}
    v['unit'] = run([os.path.join(build, 'mhgp10_unit')], 600, env)
    v['fault'] = run([os.path.join(build, 'mhgp10_fault'), 'output_set'], 600, env)
    v['produit'] = run([sys.executable, gate, build, 'produit'], 1500, env)
    v['temoin'] = run([sys.executable, gate, build, 'temoin'], 900, env)
    v['fils'] = run([sys.executable, fils, build], 900, env)
    if with_fast:
        v['fast'] = run([sys.executable, fast, build, ctest, 'mhgp10_fast_targets', 'mhgp10_catalogue', 'mhgp10_tower',
                         'mhgp10_cluster', 'mhgp10_unit'], 1500, env)
    return v


def build_copy(build):
    return run(['nice', '-n', '5', 'cmake', '--build', build, '--parallel', '3'], 3600, dict(os.environ))


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 3
    src, build, journal = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]
    only = set(sys.argv[4:])
    env = dict(os.environ)
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    log = open(journal, 'a', encoding='utf-8')

    def record(row):
        log.write(json.dumps(row, ensure_ascii=False, sort_keys=True) + '\n')
        log.flush()

    bcode, bout, _ = build_copy(build)
    if bcode != 0:
        print('HARNAIS build de la copie : code %s' % bcode)
        return 3
    ref = judges(src, build, True, env)
    record(dict(id='temoin_avant', verdicts={k: str(c) for k, (c, _, _) in ref.items()}))
    print('temoin avant : %s' % {k: c for k, (c, _, _) in ref.items()}, flush=True)
    if any(c != 0 for c, _, _ in ref.values()):
        print('HARNAIS temoin non conforme')
        return 3
    killed = survived = equivalents = unexpected = harness = 0
    for mid, group, rel, ops, desc, equivalent, with_fast in MUTANTS:
        if only and mid not in only:
            continue
        path = os.path.join(src, rel)
        with open(path, encoding='utf-8') as f:
            original = f.read()
        mutated, why = apply_ops(original, ops)
        if mutated is None:
            harness += 1
            record(dict(id=mid, group=group, error=why))
            print('%-5s HARNAIS %s' % (mid, why), flush=True)
            continue
        try:
            with open(path, 'w', encoding='utf-8') as f:
                f.write(mutated)
            bcode, bout, bwall = build_copy(build)
            if bcode != 0:
                verdicts = {'build': (bcode, bout[-2000:], bwall)}
            else:
                verdicts = judges(src, build, with_fast, env)
        finally:
            with open(path, 'w', encoding='utf-8') as f:
                f.write(original)
        killers = sorted(k for k, (c, _, _) in verdicts.items() if c != 0)
        is_killed = bool(killers) and 'build' not in killers
        if 'build' in killers:
            harness += 1
        elif is_killed:
            killed += 1
            unexpected += equivalent  # un equivalent annonce ne doit pas etre tue
        elif equivalent:
            equivalents += 1
        else:
            survived += 1
        record(dict(id=mid, group=group, file=rel, description=desc, equivalent_claimed=equivalent,
                    verdicts={k: str(c) for k, (c, _, _) in verdicts.items()}, killed_by=killers,
                    seconds={k: round(w, 1) for k, (_, _, w) in verdicts.items()}))
        print('%-5s %-3s %-9s tue_par=%-34s %s' % (mid, group, 'NON_COMPILE' if 'build' in killers else
                                                   ('TUE' if is_killed else ('EQUIVALENT' if equivalent else 'SURVIT')),
                                                   ','.join(killers) or '-', desc), flush=True)
    bcode, _, _ = build_copy(build)
    ref = judges(src, build, True, env) if bcode == 0 else {}
    record(dict(id='temoin_apres', build=bcode, verdicts={k: str(c) for k, (c, _, _) in ref.items()}))
    ref_ok = bcode == 0 and all(c == 0 for c, _, _ in ref.values())
    print('temoin apres : %s' % ({k: c for k, (c, _, _) in ref.items()} if ref else 'build %s' % bcode))
    line = ('mutants_entrees_cli tues=%d equivalents=%d survivants=%d equivalents_tues=%d harnais=%d temoin_apres=%s'
            % (killed, equivalents, survived, unexpected, harness, 'ok' if ref_ok else 'ECHEC'))
    record(dict(id='bilan', line=line))
    print(line)
    if harness or not ref_ok:
        return 3
    return 1 if survived or unexpected else 0


if __name__ == '__main__':
    sys.exit(main())

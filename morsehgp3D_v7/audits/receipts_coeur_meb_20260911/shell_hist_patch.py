#!/usr/bin/env python3
"""Instrumente `anchor_meb.hpp` pour histogrammer (support_size, coquille).

Mesure la frequence du cas U=S, c'est-a-dire `selected_shell_count ==
support_size`, sur le flux reel. Refuse tout arbre versionne : l'instrumentation
ne doit jamais toucher le worktree partage.
Code 0 applique, 2 refus de garde, 3 cible inattendue.
"""
from __future__ import annotations

import sys
from pathlib import Path

MARK = "MHGP7_SHELL_HIST"

PREAMBLE = """// ===== MHGP7_SHELL_HIST : instrumentation d'audit, arbre isole uniquement =====
#include <array>
#include <cstdio>
namespace mhgp7 {
namespace shell_hist {
// Variable inline de portee espace de noms : trivialement destructible, donc
// lisible depuis le destructeur de vidage quel que soit l'ordre de destruction.
inline std::array<std::array<unsigned long long, 12>, 5> g_table{};
inline void note(unsigned q, unsigned shell) noexcept {
  if (q < 5 && shell < 12) ++g_table[q][shell];
}
struct Dump {
  ~Dump() {
    unsigned long long total = 0, equal = 0;
    std::fprintf(stderr, "MHGP7_SHELL_HIST_BEGIN\\n");
    for (unsigned q = 1; q < 5; ++q)
      for (unsigned s = 0; s < 12; ++s) {
        const unsigned long long v = g_table[q][s];
        if (v == 0) continue;
        total += v;
        if (s == q) equal += v;
        std::fprintf(stderr, "q=%u shell=%u count=%llu\\n", q, s, v);
      }
    std::fprintf(stderr, "total_accepte=%llu coquille_egale_support=%llu\\n", total, equal);
    std::fprintf(stderr, "MHGP7_SHELL_HIST_END\\n");
  }
};
inline Dump g_dump;
}  // namespace shell_hist
}  // namespace mhgp7

"""

ANCHOR_NS = "namespace mhgp7 {"
SITE_MAIN = "    result.selected_shell_count = shell;"
CALL_MAIN = "    result.selected_shell_count = shell;\n    ::mhgp7::shell_hist::note(q, shell);"
SITE_ONE = "    result.support_size = result.selected_shell_count = 1;"
CALL_ONE = "    result.support_size = result.selected_shell_count = 1;\n    ::mhgp7::shell_hist::note(1, 1);"


def refuse(message: str, code: int) -> int:
    print(f"REFUS : {message}", file=sys.stderr)
    return code


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        return refuse("usage : shell_hist_patch.py <chemin/anchor_meb.hpp>", 2)
    target = Path(argv[1]).resolve()
    for parent in [target] + list(target.parents):
        if (parent / ".git").exists():
            return refuse(f"arbre versionne detecte en {parent} ; extraire hors du depot", 2)
    if not target.is_file():
        return refuse(f"cible absente : {target}", 2)

    text = target.read_text(encoding="utf-8")
    if MARK in text:
        return refuse("cible deja instrumentee", 3)
    if text.count(SITE_MAIN) != 1:
        return refuse(f"site principal attendu une fois, vu {text.count(SITE_MAIN)}", 3)
    if text.count(SITE_ONE) != 1:
        return refuse(f"site singleton attendu une fois, vu {text.count(SITE_ONE)}", 3)
    if ANCHOR_NS not in text:
        return refuse("ouverture d'espace de noms introuvable", 3)

    text = text.replace(ANCHOR_NS, PREAMBLE + ANCHOR_NS, 1)
    text = text.replace(SITE_MAIN, CALL_MAIN, 1)
    text = text.replace(SITE_ONE, CALL_ONE, 1)
    target.write_text(text, encoding="utf-8")
    print(f"instrumente : {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

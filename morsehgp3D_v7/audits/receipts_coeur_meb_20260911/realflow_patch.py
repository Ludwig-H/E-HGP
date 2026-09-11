#!/usr/bin/env python3
"""Applique le Welzl reparé a une COPIE isolee de l'arbre, jamais au depot.

Renomme `anchor_meb` en `anchor_meb_brute`, puis reinstalle un `anchor_meb` qui
tente le chemin repare et **delegue a l'original** tout cas invalide, degenere
ou d'echec : l'exactitude est donc garantie par construction, seul le chemin
nominal change. Le cas de base construit la boule passant PAR R via `q3_form` et
`q4_form`, en ne gardant que `g > 0` et `det > 0`, sans les filtres de minimalite
de `form()` (acuite q3, centre interieur q4) qui etaient l'erreur d'origine.

  mkdir -p /tmp/iso && git archive <commit> morsehgp3D_v7 | tar -x -C /tmp/iso
  python3 realflow_patch.py /tmp/iso/morsehgp3D_v7/src/forest/anchor_meb.hpp
  g++ -std=c++20 -O2 -pthread -isystem <boost> -I /tmp/iso/morsehgp3D_v7 \
      /tmp/iso/morsehgp3D_v7/bench/full_ball_tower_probe.cpp -o probe_var

Juge : `payload_digest` identique entre sonde de base et sonde patchee.
Mesure a n=8000, K1..10, --static-threads=1 : digest identique, `tower_s`
61,23 s -> 45,13 s, supports testes 340 615 272 -> 23 092 967 (`flux_reel.out`).
Sans assert ; identique sous python3 et python3 -O.
"""
from __future__ import annotations

import sys
from pathlib import Path

ORIGINAL = "inline AnchorMebResult anchor_meb(std::span<const P3> sites, AnchorMebWork& work) noexcept {"
RENAMED = "inline AnchorMebResult anchor_meb_brute(std::span<const P3> sites, AnchorMebWork& work) noexcept {"

INJECT = r'''
namespace anchor_meb_detail {
struct Bag { u8 v[12]; u8 n = 0; };
inline bool boundary_ball(std::span<const P3> s, const Bag& R, Candidate& out, AnchorMebWork& w) noexcept {
  if (R.n >= 2 && !charge(w.supports_by_size[R.n < 4 ? R.n : 4])) return false;
  if (R.n == 0) return false;
  if (R.n <= 2) { const u8 i0=R.v[0], i1=R.v[R.n-1];
    out.q=2; out.slots={i0,i1,0,0}; out.a=s[i0]; out.b=s[i1]; return true; }
  if (R.n == 3) { out.q=3; out.slots={R.v[0],R.v[1],R.v[2],0}; out.a=s[R.v[0]]; out.b=s[R.v[1]];
    out.three=q3_form(s[R.v[0]],s[R.v[1]],s[R.v[2]]); return out.three.g>0; }
  out.q=4; out.slots={R.v[0],R.v[1],R.v[2],R.v[3]}; out.a=s[R.v[0]]; out.b=s[R.v[1]];
  out.four=q4_form(s[R.v[0]],s[R.v[1]],s[R.v[2]],s[R.v[3]]); return out.four.det>0;
}
inline bool welzl_repaired(std::span<const P3> s, const u8* P, u8 pn, Bag& R, Candidate& out, AnchorMebWork& w) noexcept {
  if (pn == 0 || R.n == 4) return boundary_ball(s, R, out, w);
  const u8 p = P[pn-1];
  bool ok = welzl_repaired(s, P, (u8)(pn-1), R, out, w);
  if (ok) { if (!charge(w.power_tests)) return false;
            if (out.power(s[p]) <= 0) return true; }
  R.v[R.n++] = p;
  ok = welzl_repaired(s, P, (u8)(pn-1), R, out, w);
  --R.n;
  return ok;
}
}  // namespace anchor_meb_detail

inline AnchorMebResult anchor_meb(std::span<const P3> sites, AnchorMebWork& work) noexcept {
  // Valider la taille AVANT toute conversion : un grand span tronquerait son
  // cardinal. Borne a dix facettes, conformement au contrat actif.
  if (sites.size() < 2 || sites.size() > 10) return anchor_meb_brute(sites, work);
  const u8 n = static_cast<u8>(sites.size());
  for (u8 i = 0; i < n; ++i) {
    if (!p3_in_profile(sites[i])) return anchor_meb_brute(sites, work);
    for (u8 j = 0; j < i; ++j) if (sites[i] == sites[j]) return anchor_meb_brute(sites, work);
  }
  using namespace anchor_meb_detail;
  u8 P[12]; for (u8 i = 0; i < n; ++i) P[i] = static_cast<u8>(n - 1 - i);
  Bag R{}; Candidate meb;
  if (!welzl_repaired(sites, P, n, R, meb, work)) return anchor_meb_brute(sites, work);
  u8 shell[12]; u8 sn = 0;
  for (u8 i = 0; i < n; ++i) { if (!charge(work.power_tests)) return anchor_meb_brute(sites, work);
    const i128 pw = meb.power(sites[i]);
    if (pw > 0) return anchor_meb_brute(sites, work);
    if (pw == 0) shell[sn++] = i; }
  AnchorMebResult r;
  const auto emit = [&](std::array<u8,4> sl, u8 q) noexcept {
    if (!charge(work.supports_by_size[q])) return false;
    Candidate cd; if (!form(sites, sl, q, cd)) return false;
    u8 sh = 0;
    for (u8 i = 0; i < n; ++i) { if (!charge(work.power_tests)) return false;
      const i128 pw = cd.power(sites[i]); if (pw > 0) return false; if (pw == 0) ++sh; }
    if (!charge(work.materializations)) return false;
    if (!charge(work.calls)) return false;
    r.support_size = q; r.support_slots = sl; r.selected_shell_count = sh;
    if (q == 2) { r.key = q2_ball_key(cd.a, cd.b);
      r.level = promote_level(q2_exact_level(p3_norm2(p3_sub(cd.a, cd.b)))); }
    else if (q == 3) { r.key = q3_ball_key(cd.three);
      r.level = promote_level(q3_exact_level(cd.a, cd.b, sites[sl[2]])); }
    else { r.key = ball_key_reduce(q4_ball_form(cd.four)); r.level = q4_level_raw(cd.four); }
    r.status = AnchorMebStatus::kOk; r.reason = "anchor_meb_exact_local"; return true; };
  for (u8 a=0;a<sn;++a) for (u8 b=a+1;b<sn;++b) if (emit({shell[a],shell[b],0,0},2)) return r;
  for (u8 a=0;a<sn;++a) for (u8 b=a+1;b<sn;++b) for (u8 c=b+1;c<sn;++c)
    if (emit({shell[a],shell[b],shell[c],0},3)) return r;
  for (u8 a=0;a<sn;++a) for (u8 b=a+1;b<sn;++b) for (u8 c=b+1;c<sn;++c) for (u8 d=c+1;d<sn;++d)
    if (emit({shell[a],shell[b],shell[c],shell[d]},4)) return r;
  return anchor_meb_brute(sites, work);
}
'''


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: realflow_patch.py <copie isolee>/src/forest/anchor_meb.hpp", file=sys.stderr)
        return 2
    target = Path(sys.argv[1])
    resolved = target.resolve()
    for parent in [resolved, *resolved.parents]:
        if (parent / ".git").exists():
            print(f"refus : {parent} est un arbre versionne ; patcher une copie isolee",
                  file=sys.stderr)
            return 2
    try:
        text = target.read_text(encoding="utf-8")
    except OSError as error:
        print(f"lecture impossible : {error}", file=sys.stderr)
        return 2
    if "anchor_meb_brute" in text:
        print("deja patche ; rien a faire")
        return 0
    if text.count(ORIGINAL) != 1:
        print("ancre introuvable ou multiple : arbre inattendu", file=sys.stderr)
        return 1
    closing = "}  // namespace mhgp7"
    if closing not in text:
        print("fin de namespace introuvable", file=sys.stderr)
        return 1
    text = text.replace(ORIGINAL, RENAMED, 1)
    cut = text.rindex(closing)
    text = text[:cut] + INJECT + "\n" + text[cut:]
    target.write_text(text, encoding="utf-8")
    print(f"patche : {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

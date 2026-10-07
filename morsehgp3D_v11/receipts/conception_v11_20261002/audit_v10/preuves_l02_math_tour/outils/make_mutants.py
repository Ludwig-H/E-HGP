#!/usr/bin/env python3
"""Copies mutees de morsehgp3D_v10/src/tower/tower.cpp (audit L02, hors depot).

    python3 make_mutants.py SRC_V10 DEST
SRC_V10 : copie des sources de morsehgp3D_v10 (HEAD afb081774) ; DEST : dossier ou ecrire mutants/<nom>/ et
variants/<nom>/. Chaque edition exige une occurrence unique du texte remplace (sinon arret).
Mutants (fautes simulees) : m_bin, m_seq, m_vopen, m_vnoclimb, m_onepiece, m_strict.
Variantes neutres (memes sorties attendues) : v_noseed, v_alt.
"""
import os
import shutil
import sys

EDITS = {}

# --- mutants
EDITS['mutants/m_bin'] = [(  # multifusion binarisee : chaine de fusions binaires de meme rang
"""      if (b - a >= 2) {
        const u32 node = static_cast<u32>(out.rank.size());
        out.rank.push_back(rk);
        out.birth.push_back(kNone);
        out.parent.push_back(kNone);
        kids.clear();
        for (size_t t = a; t < b; ++t) kids.push_back(top[members[t].second]);
        std::sort(kids.begin(), kids.end());
        for (u32 kid : kids) {
          out.child_val.push_back(kid);
          out.parent[kid] = node;
        }
        out.child_off.push_back(static_cast<u32>(out.child_val.size()));
        top[members[a].first] = node;
        ++out.merges;
      }""",
"""      if (b - a >= 2) {
        kids.clear();
        for (size_t t = a; t < b; ++t) kids.push_back(top[members[t].second]);
        std::sort(kids.begin(), kids.end());
        u32 acc = kids[0];
        for (size_t t = 1; t < kids.size(); ++t) {  // MUTANT : chaine binaire
          const u32 node = static_cast<u32>(out.rank.size());
          out.rank.push_back(rk);
          out.birth.push_back(kNone);
          out.parent.push_back(kNone);
          out.child_val.push_back(acc);
          out.parent[acc] = node;
          out.child_val.push_back(kids[t]);
          out.parent[kids[t]] = node;
          out.child_off.push_back(static_cast<u32>(out.child_val.size()));
          acc = node;
          ++out.merges;
        }
        top[members[a].first] = acc;
      }""")]

EDITS['mutants/m_seq'] = [(  # plateau non atomique : chaque jonction est son propre lot
"""    while (j < nj && cat.rank[o.join_ball[j]] + 1 == rk) ++j;""",
"""    j = i + 1;  // MUTANT : chaque jonction est son propre lot""")]

EDITS['mutants/m_vopen'] = [(  # image verticale d'une naissance prise a la coupe ouverte
"""            up.lower[v] = ancestor(down, od.jumps, ball_vnode[ball], up.rank[v], sc.walk[k]);
            return true;""",
"""            up.lower[v] = ancestor(down, od.jumps, ball_vnode[ball], up.rank[v] - 1, sc.walk[k]);  // MUTANT
            return true;"""), (
"""        up.lower[v] = ancestor(down, od.jumps, m, up.rank[v], sc.walk[k]);
        return true;""",
"""        up.lower[v] = ancestor(down, od.jumps, m, up.rank[v] - 1, sc.walk[k]);  // MUTANT
        return true;""")]

EDITS['mutants/m_vnoclimb'] = [(  # image d'une fusion = image brute du premier enfant, naturalite non verifiee
"""            const u32 c = ancestor(down, dj, up.lower[up.child_val[j]], up.rank[v], steps);
            if (image == kNone) image = c;""",
"""            const u32 c = j == up.child_off[v] ? up.lower[up.child_val[j]] : image;  // MUTANT : pas de remontee
            (void)dj; (void)steps; (void)down;
            if (image == kNone) image = c;""")]

EDITS['mutants/m_onepiece'] = [(  # coquille etendue jamais jointe
"""  return firsts.size() >= 2 ? Local::join : Local::inert;
}""",
"""  if (firsts.size() >= 2) reps.resize(1);  // MUTANT
  return Local::inert;
}""")]

EDITS['mutants/m_strict'] = [(  # niveau <= e remplace par < (constat AT1 de l'audit geant)
"""  return arith::cmp(L.num, ed) <= 0;""",
"""  return arith::cmp(L.num, ed) < 0;  // MUTANT""")]

# --- variantes neutres
EDITS['variants/v_noseed'] = [(
"""      if (seed != kNone) {  // semis : sommet de naissance""",
"""      if (false && seed != kNone) {  // VARIANTE : semis ignore""")]

EDITS['variants/v_alt'] = [(
"""      sc.nearD.clear();
      for (u32 z : I) sc.nearD.push_back({approx_d2(S, g.P[z]), z});
      std::nth_element(sc.nearD.begin(), sc.nearD.begin() + k, sc.nearD.end());
      double dk = sc.nearD[0].first;
      for (u32 i = 1; i < k; ++i) dk = std::max(dk, sc.nearD[i].first);
      if (sc.nearD[k].first - dk > kApproxMargin) {
        for (u32 i = 0; i < k; ++i) F.s[i] = sc.nearD[i].second;
      } else {
        ++cnt.jump_exact;
        sc.near.clear();
        for (u32 z : I) sc.near.push_back({geom::side_key(S.c, a, g.P[z]), z});
        std::partial_sort(sc.near.begin(), sc.near.begin() + k, sc.near.end());
        for (u32 i = 0; i < k; ++i) F.s[i] = sc.near[i].second;
      }
      F.sort();
      continue;""",
"""      {  // VARIANTE : les k interieurs les plus eloignes du centre (cle exacte decroissante, puis indice)
        ++cnt.jump_exact;
        sc.near.clear();
        for (u32 z : I) sc.near.push_back({-geom::side_key(S.c, a, g.P[z]), z});
        std::partial_sort(sc.near.begin(), sc.near.begin() + k, sc.near.end());
        for (u32 i = 0; i < k; ++i) F.s[i] = sc.near[i].second;
      }
      F.sort();
      continue;"""), (
"""    if (t + 1 == m) merge_into(rep, I.data(), static_cast<u32>(I.size()), U.data() + 1, m - 1);
    else merge_into(rep, I.data(), static_cast<u32>(I.size()), U.data(), t);
    return Local::inert;""",
"""    if (t + 1 == m) merge_into(rep, I.data(), static_cast<u32>(I.size()), U.data(), m - 1);  // VARIANTE
    else merge_into(rep, I.data(), static_cast<u32>(I.size()), U.data() + (m - t), t);        // VARIANTE
    return Local::inert;""")]


def main():
    src, dest = sys.argv[1], sys.argv[2]
    base = open(os.path.join(src, 'src/tower/tower.cpp')).read()
    for name, edits in EDITS.items():
        s = base
        for old, new in edits:
            if s.count(old) != 1:
                print('ARRET', name, ': texte remplace present', s.count(old), 'fois')
                return 1
            s = s.replace(old, new)
        d = os.path.join(dest, name)
        if os.path.exists(d):
            shutil.rmtree(d)
        shutil.copytree(src, d, ignore=shutil.ignore_patterns('docs', 'bench', 'reference', 'receipts', 'audits'))
        open(os.path.join(d, 'src/tower/tower.cpp'), 'w').write(s)
        print(name, 'ok')
    return 0


if __name__ == '__main__':
    sys.exit(main())

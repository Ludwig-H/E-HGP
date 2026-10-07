#!/usr/bin/env python3
"""Audit L06 : mutants causaux de tower.cpp (copie sous /tmp), selectionnes par macro, pour eprouver la sensibilite de
la porte mhgp10_tower_oracle. Un mutant par build (-DL06_MUT_x)."""
p = '/tmp/v11-audit/l06_code_tour/mutants/src_mut/src/tower/tower.cpp'
s = open(p).read()
def rep(old, new):
    global s
    assert s.count(old) == 1, (old[:60], s.count(old))
    s = s.replace(old, new)

# M1 : multifusion binarisee (chaine de noeuds binaires au meme rang)
rep('''      if (b - a >= 2) {
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
      }''', '''      if (b - a >= 2) {
#ifdef L06_MUT_BINARIZE
        kids.clear();
        for (size_t t = a; t < b; ++t) kids.push_back(top[members[t].second]);
        std::sort(kids.begin(), kids.end());
        u32 acc = kids[0];
        for (size_t t = 1; t < kids.size(); ++t) {
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
#else
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
#endif
      }''')
# M4 : attache core a la coupe ouverte d'un rang plus bas
rep('''            const u32 r = ranks.at_most(cat, lev);
            v = ancestor(out, o.jumps, v, r, sc.walk[k]);''', '''#ifdef L06_MUT_POINT_OPEN
            const u32 r0 = ranks.at_most(cat, lev);
            const u32 r = r0 > out.rank[v] ? r0 - 1 : r0;
#else
            const u32 r = ranks.at_most(cat, lev);
#endif
            v = ancestor(out, o.jumps, v, r, sc.walk[k]);''')
# M7 : image verticale d'une naissance a la coupe ouverte
rep('''            up.lower[v] = ancestor(down, od.jumps, ball_vnode[ball], up.rank[v], sc.walk[k]);
            return true;''', '''#ifdef L06_MUT_VERT_OPEN
            up.lower[v] = ball_vnode[ball];  // naissance d'arrivee de la descente, sans remontee au niveau de la boule
#else
            up.lower[v] = ancestor(down, od.jumps, ball_vnode[ball], up.rank[v], sc.walk[k]);
#endif
            return true;''')
# M9 : la derniere union de chaque jonction est omise
rep('''      for (u32 r = r0 + 1; r < o.join_off[t + 1] - o.join_off[i]; ++r) {
        const u32 x = find(pre[r0]), y = find(pre[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }''', '''#ifdef L06_MUT_DROP_LAST_REP
      const u32 l06_end = o.join_off[t + 1] - o.join_off[i] > r0 + 2 ? o.join_off[t + 1] - o.join_off[i] - 1
                                                                 : o.join_off[t + 1] - o.join_off[i];
#else
      const u32 l06_end = o.join_off[t + 1] - o.join_off[i];
#endif
      for (u32 r = r0 + 1; r < l06_end; ++r) {
        const u32 x = find(pre[r0]), y = find(pre[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }''')
# M12 : saut vers k sites interieurs quelconques (les k premiers indices) au lieu des k plus proches du centre
rep('''      if (p == k) {  // exactement k interieurs : ce sont les k plus proches (I trie par indice)
        for (u32 i = 0; i < k; ++i) F.s[i] = I[i];
        continue;
      }''', '''      if (p == k) {  // exactement k interieurs : ce sont les k plus proches (I trie par indice)
        for (u32 i = 0; i < k; ++i) F.s[i] = I[i];
        continue;
      }
#ifdef L06_MUT_JUMP_ANY
      for (u32 i = 0; i < k; ++i) F.s[i] = I[i];  // k sites strictement interieurs quelconques (plus petits indices)
      continue;
#endif''')
# M13 : fenetre basse p + q au lieu de p + q - 1
rep('''      const u32 lo = std::max(p + cat.qmin[b] - 1, atlas.kfirst);''', '''#ifdef L06_MUT_WINDOW_LO
      const u32 lo = std::max(p + cat.qmin[b], atlas.kfirst);
#else
      const u32 lo = std::max(p + cat.qmin[b] - 1, atlas.kfirst);
#endif''')
open(p, 'w').write(s)
print('ok')

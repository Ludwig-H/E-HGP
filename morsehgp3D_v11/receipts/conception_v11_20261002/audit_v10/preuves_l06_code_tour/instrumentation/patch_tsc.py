#!/usr/bin/env python3
"""Second correctif d'instrumentation (copie d'audit) : repartition en cycles (rdtsc) des segments de resolve()."""
p = '/tmp/v11-audit/l06_code_tour/instr/src/tower/tower.cpp'
s = open(p).read()
def rep(old, new, count=1):
    global s
    assert s.count(old) == count, (old[:80], s.count(old))
    s = s.replace(old, new)

rep('#include <cstdio>\n#include <cstdlib>\n#include <mutex>\n', '#include <cstdio>\n#include <cstdlib>\n#include <mutex>\n#include <x86intrin.h>\n')
rep('''  u64 meb_fallbacks = 0;
  u64 walk[kOrders] = {};''', '''  u64 meb_fallbacks = 0;
  u64 tsc[12] = {};  // L06 : cycles par segment de resolve
  u64 walk[kOrders] = {};''')
# segments
rep('''    {
      const u64 h = have_h0 ? h0 : hash_sites(F.s.data(), F.n);
      have_h0 = false;''', '''    u64 l06_t = __rdtsc();
    auto l06_lap = [&](int seg) {
      const u64 now = __rdtsc();
      sc.tsc[seg] += now - l06_t;
      l06_t = now;
    };
    {
      const u64 h = have_h0 ? h0 : hash_sites(F.s.data(), F.n);
      have_h0 = false;''')
rep('''      if (seed != kNone) {  // semis : sommet de naissance
        ++cnt.seed_hits;
        node = seed;
        break;
      }
    }
    Sphere S = meb(g, F, sc.meb_fallbacks);
    ++cnt.meb;''', '''      l06_lap(0);
      if (seed != kNone) {  // semis : sommet de naissance
        ++cnt.seed_hits;
        node = seed;
        break;
      }
    }
    Sphere S = meb(g, F, sc.meb_fallbacks);
    ++cnt.meb;
    l06_lap(1);''')
rep('''    if (!from_cat) {
      g.tree.closed_ball(a, S.c, sc.I, sc.U);
      ++cnt.closed_balls;
      I = sc.I;
      U = sc.U;
      p = static_cast<u32>(I.size());
      m = static_cast<u32>(U.size());''', '''    l06_lap(2);
    if (!from_cat) {
      g.tree.closed_ball(a, S.c, sc.I, sc.U);
      ++cnt.closed_balls;
      l06_lap(3);
      I = sc.I;
      U = sc.U;
      p = static_cast<u32>(I.size());
      m = static_cast<u32>(U.size());''')
rep('''      if (p == k) {  // exactement k interieurs : ce sont les k plus proches (I trie par indice)
        for (u32 i = 0; i < k; ++i) F.s[i] = I[i];
        continue;
      }''', '''      if (p == k) {  // exactement k interieurs : ce sont les k plus proches (I trie par indice)
        for (u32 i = 0; i < k; ++i) F.s[i] = I[i];
        l06_lap(4);
        continue;
      }''')
rep('''      F.sort();
      continue;
    }
    if (!from_cat) {
      std::array<u32, 4> sup = {kNone, kNone, kNone, kNone};''', '''      F.sort();
      l06_lap(4);
      continue;
    }
    if (!from_cat) {
      std::array<u32, 4> sup = {kNone, kNone, kNone, kNone};''')
rep('''    Facet rep;
    const Local L = first_rep(g, I, U, S.anchor, S.c, k - p, m == q, rep);
    ++cnt.local_calls;''', '''    l06_lap(5);
    Facet rep;
    const Local L = first_rep(g, I, U, S.anchor, S.c, k - p, m == q, rep);
    ++cnt.local_calls;
    l06_lap(6);''')
# cumul et impression apres l'etage G
rep('''    // histogrammes des descentes des jonctions (avant attaches et verticales)''', '''    {
      u64 tsc[12] = {};
      for (const Scratch& sc2 : scratch)
        for (int i = 0; i < 12; ++i) tsc[i] += sc2.tsc[i];
      u64 tot = 0;
      for (int i = 0; i < 12; ++i) tot += tsc[i];
      std::fprintf(stderr, "L06_TSC_G t_resolve_s=%.4f cycles_total=%llu seeds=%llu meb=%llu lookup_census_cat=%llu closed_ball=%llu "
                           "jump_select=%llu support_cell=%llu first_rep=%llu\\n",
                   ts.t_resolve, (unsigned long long)tot, (unsigned long long)tsc[0], (unsigned long long)tsc[1],
                   (unsigned long long)tsc[2], (unsigned long long)tsc[3], (unsigned long long)tsc[4],
                   (unsigned long long)tsc[5], (unsigned long long)tsc[6]);
    }
    // histogrammes des descentes des jonctions (avant attaches et verticales)''')
open(p, 'w').write(s)
print('ok')

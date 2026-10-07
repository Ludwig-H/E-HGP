# Sonde de compteurs (conception v11, pistes de rupture) : applique a une COPIE de generator.cpp de la v10
#   MHGP_X_NOLINE=1 : un triplet de poids <= theta4 est vivant (H) meme si sa droite manque la boite ;
#   MHGP_X_ORDER=1  : filtre "un seul ordre K" : une partie T de support n'est gardee que si le nombre de sites
#                     de la liste certainement exterieurs (domines sur la boite par un site de T) est <= m - K.
import sys
p = sys.argv[1]
s = open(p).read()
def rep(old, new, count=1):
    global s
    if s.count(old) != count:
        raise SystemExit("occurrences inattendues (%d) pour : %s" % (s.count(old), old[:60]))
    s = s.replace(old, new)
rep('#include "sched/sort.hpp"', '#include "sched/sort.hpp"\n#include <cstdlib>')
# drapeaux et masques transposes
rep('''  L.led.leaf_dominance_tests += u64(m) * (m - 1) / 2;
  geom::Center ctr;''', '''  L.led.leaf_dominance_tests += u64(m) * (m - 1) / 2;
  static const bool x_noline = std::getenv("MHGP_X_NOLINE") != nullptr;
  static const bool x_order = std::getenv("MHGP_X_ORDER") != nullptr;
  const bool xo = x_order && nw == 1 && tight;
  u64 DB[64] = {0};  // DB[s] : sites que s domine sur la boite (certainement exterieurs si s est sur la coquille)
  if (xo)
    for (u32 i = 0; i < m; ++i)
      for (u64 x = Dm[i]; x; x &= x - 1) DB[std::countr_zero(x)] |= u64(1) << i;
  const i64 omax = static_cast<i64>(m) - K;
  geom::Center ctr;''')
# paires
rep('''      if (static_cast<i64>(d) > th2) continue;
      if (static_cast<i64>(d) <= th3) {''', '''      if (static_cast<i64>(d) > th2) continue;
      if (xo && static_cast<i64>(popcount64((DB[i] | DB[j]) & ~((u64(1) << i) | (u64(1) << j)))) > omax) continue;
      if (static_cast<i64>(d) <= th3) {''')
# triplets
rep('''            if (static_cast<i64>(d) > th3) continue;
            const int line = center_line_meets(lx[i], lx2[i], lx[j], lx2[j], lx[k], lx2[k], Q);
            if (line < 0) continue;  // alignes
            ++L.led.triple_tests;
            if (line == 0) continue;''', '''            if (static_cast<i64>(d) > th3) continue;
            if (xo && static_cast<i64>(popcount64((DB[i] | DB[j] | DB[k]) &
                                                   ~((u64(1) << i) | (u64(1) << j) | (u64(1) << k)))) > omax)
              continue;
            const int line = center_line_meets(lx[i], lx2[i], lx[j], lx2[j], lx[k], lx2[k], Q);
            if (line < 0) continue;  // alignes
            ++L.led.triple_tests;
            if (line == 0) {
              if (x_noline && static_cast<i64>(d) <= th4) {
                setbit(hrow(i, j), k);
                setbit(hrow(i, k), j);
                setbit(hrow(j, k), i);
              }
              continue;
            }''')
# quadruplets
rep('''                if (static_cast<i64>(d) > th4) continue;
                ++L.led.quad_tests;''', '''                if (static_cast<i64>(d) > th4) continue;
                if (xo && static_cast<i64>(popcount64((DB[i] | DB[j] | DB[k] | DB[l]) &
                                                       ~((u64(1) << i) | (u64(1) << j) | (u64(1) << k) |
                                                         (u64(1) << l)))) > omax)
                  continue;
                ++L.led.quad_tests;''')
open(p, 'w').write(s)
print("ok")

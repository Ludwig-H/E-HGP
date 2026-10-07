# Second jeu de compteurs (copie deja sondee) : entonnoir q3 sans test de droite.
#   aigus parmi tous les triplets de poids admissible ; aigus dont l'enveloppe du triangle median rencontre la
#   boite fermee ; (le compte "centre dans la boite" est celui des juges q3, inchange).
import sys
p = sys.argv[1]
s = open(p).read()
def rep(old, new, count=1):
    global s
    if s.count(old) != count:
        raise SystemExit("occurrences inattendues (%d) pour : %s" % (s.count(old), old[:60]))
    s = s.replace(old, new)
rep('#include <cstdlib>', '''#include <cstdlib>
#include <cstdio>
namespace { unsigned long long g_x_trip = 0, g_x_acute = 0, g_x_env = 0, g_x_acute_hit = 0;
struct XReport { ~XReport() { if (std::getenv("MHGP_X_COUNT")) std::fprintf(stderr,
  "X_COUNT triplets %llu aigus %llu aigus_et_enveloppe_mediane %llu aigus_et_droite %llu\\n",
  g_x_trip, g_x_acute, g_x_env, g_x_acute_hit); } } g_x_report; }''')
rep('''            ++L.led.triple_tests;
            if (line == 0) {''', '''            ++L.led.triple_tests;
            {
              ++g_x_trip;
              if (geom::acute(lp[i], lp[j], lp[k])) {
                ++g_x_acute;
                if (line == 1) ++g_x_acute_hit;
                bool meet = true;  // enveloppe du triangle median, coordonnees doublees, boite fermee
                const i64 xi[3] = {lx[i].x, lx[i].y, lx[i].z}, xj[3] = {lx[j].x, lx[j].y, lx[j].z},
                          xk[3] = {lx[k].x, lx[k].y, lx[k].z};
                for (int ax = 0; ax < 3; ++ax) {
                  const i64 m1 = xi[ax] + xj[ax], m2 = xj[ax] + xk[ax], m3 = xi[ax] + xk[ax];
                  const i64 mn = std::min(m1, std::min(m2, m3)), mx = std::max(m1, std::max(m2, m3));
                  if (mn > 2 * Q.hi[ax] || mx < 2 * Q.lo[ax]) meet = false;
                }
                if (meet) ++g_x_env;
              }
            }
            if (line == 0) {''')
open(p, 'w').write(s)
print("ok")

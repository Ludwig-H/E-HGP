// Auto-test du juge JUG-EMST (borne, jamais a l'echelle) : SHA-256 sur les vecteurs de la norme, entiers naturels
// contre u128, comparateur de Morton contre la cle formee explicitement, et arbre couvrant de Boruvka contre Kruskal
// sur toutes les paires (ordre strict : arbre unique) puis arbre a plateaux contre un lien simple calcule sur TOUTES
// les paires, sur de petits nuages tires a graine fixe (grilles, droites, coins u32 : egalites nombreuses).
// Code 0 conforme, 1 sinon.
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "emst.hpp"
#include "entier.hpp"
#include "nuage.hpp"
#include "plateaux.hpp"
#include "sha256.hpp"

using namespace mhgp12_emst;

namespace {

int echecs = 0;

void verifier(bool ok, const std::string& quoi) {
  if (!ok) {
    ++echecs;
    std::cerr << "ECHEC : " << quoi << "\n";
  }
}

struct Graine {
  u64 etat;
  u64 suivant() {
    etat += 0x9E3779B97F4A7C15ull;
    u64 z = etat;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
};

std::string sha(const std::string& texte) {
  Sha256 h;
  h.ajouter(texte.data(), texte.size());
  return h.hex();
}

void test_sha256() {
  verifier(sha("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "sha256 vide");
  verifier(sha("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 abc");
  verifier(sha("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
               "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
           "sha256 448 bits");
  Sha256 h;
  const std::string bloc(1000, 'a');
  for (int i = 0; i < 1000; ++i) h.ajouter(bloc.data(), bloc.size());
  verifier(h.hex() == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", "sha256 million de a");
}

void test_naturels() {
  Graine g{7};
  for (int i = 0; i < 20000; ++i) {
    const u64 a = g.suivant() >> (g.suivant() % 64), b = g.suivant() >> (g.suivant() % 64);
    const Naturel p = produit(naturel(a), naturel(b));
    verifier(comparer(p, naturel(static_cast<u128>(a) * b)) == 0, "produit naturel");
    verifier(comparer(naturel(a), naturel(b)) == (a < b ? -1 : a > b ? 1 : 0), "comparaison naturelle");
  }
  const u128 max = ~static_cast<u128>(0);
  verifier(decimal128(max) == "340282366920938463463374607431768211455", "decimal 2^128 - 1");
  verifier(decimal128(static_cast<u128>(1) << 64) == "18446744073709551616", "decimal 2^64");
  verifier(decimal(produit(naturel(max), naturel(max))) ==
               "115792089237316195423570985008687907852589419931798687112530834793049593217025",
           "decimal (2^128 - 1)^2");
  u128 num, den;
  niveau_reduit(6, num, den);
  verifier(num == 3 && den == 2, "niveau 6/4 = 3/2");
  niveau_reduit(8, num, den);
  verifier(num == 2 && den == 1, "niveau 8/4 = 2");
}

u128 cle_morton(u32 x, u32 y, u32 z) {
  u128 cle = 0;
  const u32 v[3] = {x, y, z};
  for (int bit = 0; bit < 32; ++bit) {
    for (int a = 0; a < 3; ++a) cle |= static_cast<u128>((v[a] >> bit) & 1u) << (3 * bit + a);
  }
  return cle;
}

void test_morton() {
  Graine g{11};
  for (int i = 0; i < 200000; ++i) {
    const int forme = static_cast<int>(g.suivant() % 3);
    auto tirer = [&]() {
      const u64 r = g.suivant();
      return forme == 0 ? static_cast<u32>(r) : forme == 1 ? static_cast<u32>(r % 8) : static_cast<u32>(r >> 43);
    };
    const std::array<u32, 3> p{tirer(), tirer(), tirer()}, q{tirer(), tirer(), tirer()};
    const bool attendu = cle_morton(p[0], p[1], p[2]) < cle_morton(q[0], q[1], q[2]);
    verifier(morton_inferieur(p, q) == attendu, "comparateur de Morton");
  }
}

// Kruskal sur toutes les paires, ordre strict (d2, a, b).
std::vector<Arete> kruskal_complet(const Sites& s) {
  std::vector<Arete> paires;
  for (u32 a = 0; a < s.taille(); ++a) {
    for (u32 b = a + 1; b < s.taille(); ++b) {
      u128 d2 = 0;
      const u32 pa[3] = {s.x[a], s.y[a], s.z[a]}, pb[3] = {s.x[b], s.y[b], s.z[b]};
      for (int k = 0; k < 3; ++k) {
        const u64 e = pa[k] > pb[k] ? u64{pa[k] - pb[k]} : u64{pb[k] - pa[k]};
        d2 += static_cast<u128>(e) * e;
      }
      paires.push_back({d2, a, b});
    }
  }
  std::sort(paires.begin(), paires.end(), arete_inferieure);
  UnionPlateau uf(s.taille());
  std::vector<Arete> arbre;
  for (const Arete& e : paires) {
    if (uf.trouver(e.a) != uf.trouver(e.b)) {
      uf.unir(e.a, e.b);
      arbre.push_back(e);
    }
  }
  return arbre;
}

// Lien simple avec plateaux sur toutes les paires (aucune hypothese de foret) : forme texte canonique.
std::string lien_simple_complet(const Sites& s) {
  std::vector<Arete> paires;
  for (u32 a = 0; a < s.taille(); ++a) {
    for (u32 b = a + 1; b < s.taille(); ++b) {
      u128 d2 = 0;
      const u32 pa[3] = {s.x[a], s.y[a], s.z[a]}, pb[3] = {s.x[b], s.y[b], s.z[b]};
      for (int k = 0; k < 3; ++k) {
        const u64 e = pa[k] > pb[k] ? u64{pa[k] - pb[k]} : u64{pb[k] - pa[k]};
        d2 += static_cast<u128>(e) * e;
      }
      paires.push_back({d2, a, b});
    }
  }
  std::sort(paires.begin(), paires.end(), arete_inferieure);
  const u32 n = s.taille();
  UnionPlateau uf(n);
  std::vector<u32> noeud_de(n), petite;
  for (u32 i = 0; i < n; ++i) noeud_de[i] = i;
  for (u32 i = 0; i < n; ++i) petite.push_back(i);
  std::string texte;
  u32 prochain = n;
  for (std::size_t i = 0; i < paires.size();) {
    std::size_t j = i;
    while (j < paires.size() && paires[j].d2 == paires[i].d2) ++j;
    std::vector<std::pair<u32, u32>> avant;
    for (std::size_t e = i; e < j; ++e) avant.emplace_back(uf.trouver(paires[e].a), uf.trouver(paires[e].b));
    for (const auto& r : avant) uf.unir(r.first, r.second);
    std::vector<std::pair<u32, u32>> anciens;
    for (const auto& r : avant) {
      anciens.emplace_back(uf.trouver(r.first), noeud_de[r.first]);
      anciens.emplace_back(uf.trouver(r.first), noeud_de[r.second]);
    }
    std::sort(anciens.begin(), anciens.end());
    anciens.erase(std::unique(anciens.begin(), anciens.end()), anciens.end());
    std::vector<std::pair<u32, std::vector<u32>>> fusions;  // (plus petite naissance, enfants)
    std::vector<u32> racines;
    for (std::size_t a = 0; a < anciens.size();) {
      std::size_t b = a;
      std::vector<u32> enfants;
      u32 p = kAucun;
      while (b < anciens.size() && anciens[b].first == anciens[a].first) {
        enfants.push_back(anciens[b].second);
        p = std::min(p, petite[anciens[b].second]);
        ++b;
      }
      if (enfants.size() >= 2) {
        fusions.emplace_back(p, enfants);
        racines.push_back(anciens[a].first);
      }
      a = b;
    }
    std::vector<std::size_t> ordre(fusions.size());
    for (std::size_t k = 0; k < ordre.size(); ++k) ordre[k] = k;
    std::sort(ordre.begin(), ordre.end(), [&](std::size_t p, std::size_t q) { return fusions[p].first < fusions[q].first; });
    for (std::size_t k : ordre) {
      u128 num, den;
      niveau_reduit(paires[i].d2, num, den);
      texte += "F " + decimal128(num) + " " + decimal128(den);
      for (u32 c : fusions[k].second) texte += " " + std::to_string(c);
      texte += "\n";
      petite.push_back(fusions[k].first);
      noeud_de[racines[k]] = prochain++;
    }
    i = j;
  }
  return texte;
}

std::string texte_arbre(const Arbre& a) {
  std::string texte;
  for (u32 k = a.naissances; k < a.noeuds(); ++k) {
    u128 num, den;
    niveau_reduit(a.d2[k], num, den);
    texte += "F " + decimal128(num) + " " + decimal128(den);
    for (u32 c = 0; c < a.cardinal[k]; ++c) texte += " " + std::to_string(a.enfants[a.debut[k] + c]);
    texte += "\n";
  }
  return texte;
}

bool memes_aretes(const std::vector<Arete>& p, const std::vector<Arete>& q) {
  if (p.size() != q.size()) return false;
  for (std::size_t i = 0; i < p.size(); ++i) {
    if (p[i].d2 != q[i].d2 || p[i].a != q[i].a || p[i].b != q[i].b) return false;
  }
  return true;
}

Sites nuage_tire(Graine& g, int famille, u32 n) {
  struct P {
    u32 x, y, z;
    bool operator<(const P& o) const { return x != o.x ? x < o.x : y != o.y ? y < o.y : z < o.z; }
    bool operator==(const P& o) const { return x == o.x && y == o.y && z == o.z; }
  };
  std::vector<P> pts;
  for (u32 i = 0; i < 4 * n && pts.size() < n; ++i) {
    const u64 r = g.suivant();
    P p{};
    if (famille == 0) p = {static_cast<u32>(r % 6), static_cast<u32>((r >> 8) % 6), static_cast<u32>((r >> 16) % 6)};
    if (famille == 1) p = {static_cast<u32>(r % 40), 0, 0};
    if (famille == 2) p = {static_cast<u32>(r % 1000), static_cast<u32>((r >> 20) % 1000), static_cast<u32>(r >> 50)};
    if (famille == 3) {
      const u32 m = 0xFFFFFFFFu;
      p = {(r & 1) ? m - static_cast<u32>((r >> 8) % 3) : static_cast<u32>((r >> 8) % 3),
           (r & 2) ? m - static_cast<u32>((r >> 16) % 3) : static_cast<u32>((r >> 16) % 3),
           (r & 4) ? m - static_cast<u32>((r >> 24) % 3) : static_cast<u32>((r >> 24) % 3)};
    }
    if (famille == 4) p = {static_cast<u32>(r % 7) * 0x10000000u, static_cast<u32>((r >> 8) % 7) * 0x20000000u, 5};
    if (std::find(pts.begin(), pts.end(), p) == pts.end()) pts.push_back(p);
  }
  std::sort(pts.begin(), pts.end());
  Sites s;
  for (u32 i = 0; i < pts.size(); ++i) {
    s.x.push_back(pts[i].x);
    s.y.push_back(pts[i].y);
    s.z.push_back(pts[i].z);
    s.entree.push_back(i);
    s.coordonnee_max = std::max({s.coordonnee_max, pts[i].x, pts[i].y, pts[i].z});
  }
  return s;
}

void test_emst() {
  Graine g{2026};
  u64 nuages = 0, egalites = 0;
  for (int famille = 0; famille < 5; ++famille) {
    for (int essai = 0; essai < 60; ++essai) {
      const u32 n = 2 + static_cast<u32>(g.suivant() % 150);
      const Sites s = nuage_tire(g, famille, n);
      const std::vector<Arete> attendu = kruskal_complet(s);
      for (int force = 0; force < 2; ++force) {
        if (force == 0 && s.coordonnee_max >= 0x80000000u) continue;
        StatsEmst st;
        std::string voie;
        const std::vector<Arete> obtenu = emst_exact(s, force == 1, st, voie);
        verifier(memes_aretes(obtenu, attendu), "Boruvka contre Kruskal, famille " + std::to_string(famille) +
                                                    ", voie " + voie + ", n = " + std::to_string(s.taille()));
        StatsArbre sa;
        const Arbre arbre = arbre_plateaux(s.taille(), obtenu, sa);
        verifier(texte_arbre(arbre) == lien_simple_complet(s), "plateaux contre lien simple complet, famille " +
                                                                    std::to_string(famille));
        egalites += sa.aretes_egalite;
      }
      ++nuages;
    }
  }
  std::cout << "autotest_emst nuages=" << nuages << " aretes_a_egalite=" << egalites << "\n";
  verifier(nuages == 300 && egalites > 1000, "planchers de l'auto-test (nuages, egalites)");
}

// Prim dense en O(n^2) sous l'ordre strict : arbre unique, independant de l'arbre k-d et des memoires de Boruvka.
std::vector<Arete> prim_dense(const Sites& s) {
  const u32 n = s.taille();
  std::vector<Arete> cle(n, Arete{~static_cast<u128>(0), kAucun, kAucun});
  std::vector<char> dans(n, 0);
  std::vector<Arete> arbre;
  u32 courant = 0;
  for (u32 pas = 1; pas < n; ++pas) {
    dans[courant] = 1;
    u32 suivant = kAucun;
    for (u32 v = 0; v < n; ++v) {
      if (dans[v]) continue;
      u128 d2 = 0;
      const u32 pa[3] = {s.x[courant], s.y[courant], s.z[courant]}, pb[3] = {s.x[v], s.y[v], s.z[v]};
      for (int k = 0; k < 3; ++k) {
        const u64 e = pa[k] > pb[k] ? u64{pa[k] - pb[k]} : u64{pb[k] - pa[k]};
        d2 += static_cast<u128>(e) * e;
      }
      const Arete candidat{d2, std::min(courant, v), std::max(courant, v)};
      if (arete_inferieure(candidat, cle[v])) cle[v] = candidat;
      if (suivant == kAucun || arete_inferieure(cle[v], cle[suivant])) suivant = v;
    }
    arbre.push_back(cle[suivant]);
    courant = suivant;
  }
  std::sort(arbre.begin(), arbre.end(), arete_inferieure);
  return arbre;
}

// Echelle moyenne (arbre k-d profond, une dizaine de tours, memoires exercees) : Boruvka contre Prim dense.
void test_prim() {
  Graine g{31};
  for (int forme = 0; forme < 3; ++forme) {
    Sites s;
    std::vector<std::array<u32, 3>> pts;
    for (u32 x = 0; x < 20; ++x) {
      for (u32 y = 0; y < 20; ++y) {
        for (u32 z = 0; z < 20; ++z) {
          if (forme == 0 && g.suivant() % 2 == 0) pts.push_back({x, y, z});
          if (forme == 1 && g.suivant() % 2 == 0) pts.push_back({x * 3 + (y % 2), y * 2, z * 5});
        }
      }
    }
    if (forme == 2) {
      for (u32 i = 0; i < 4000; ++i) {
        const u64 r = g.suivant();
        pts.push_back({static_cast<u32>(r) & 0x1FFFFFu, static_cast<u32>(r >> 21) & 0x1FFFFFu,
                       static_cast<u32>(r >> 42) & 0x1FFFFFu});
      }
    }
    std::sort(pts.begin(), pts.end());
    pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
    for (u32 i = 0; i < pts.size(); ++i) {
      s.x.push_back(pts[i][0]);
      s.y.push_back(pts[i][1]);
      s.z.push_back(pts[i][2]);
      s.entree.push_back(i);
      s.coordonnee_max = std::max({s.coordonnee_max, pts[i][0], pts[i][1], pts[i][2]});
    }
    StatsEmst st;
    std::string voie;
    const std::vector<Arete> obtenu = emst_exact(s, false, st, voie);
    verifier(memes_aretes(obtenu, prim_dense(s)), "Boruvka contre Prim dense, forme " + std::to_string(forme));
    std::cout << "autotest_prim forme=" << forme << " sites=" << s.taille() << " tours=" << st.tours
              << " requetes=" << st.requetes << " voisins_gardes=" << st.voisins_gardes << " minorants=" << st.minorants
              << "\n";
    verifier(s.taille() > 3000 && st.tours >= 3 && st.voisins_gardes > 0 && st.minorants > 0,
             "planchers de l'auto-test de Prim, forme " + std::to_string(forme));
  }
}

}  // namespace

int main() {
  test_sha256();
  test_naturels();
  test_morton();
  test_emst();
  test_prim();
  std::cout << (echecs == 0 ? "autotest_conforme" : "autotest_echec") << " echecs=" << echecs << "\n";
  return echecs == 0 ? 0 : 1;
}

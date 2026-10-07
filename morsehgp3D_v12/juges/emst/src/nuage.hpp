// Nuage du juge JUG-EMST : lecture d'un fichier .u32le (12 octets par point : x, y, z en u32 petit-boutiste, le
// format des trames de la v11 et de la v12), identifiants .ids.u32le facultatifs, ordre canonique des sites et refus
// des doublons (decision D8 de la v12), comparateur de Morton exact pour l'ordre des sites d'un vidage.
#pragma once
#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <vector>

#include "entier.hpp"

namespace mhgp12_emst {

// Sites dans l'ordre canonique des naissances d'ordre un : lexicographique sur (x, y, z). Le numero d'un site dans
// cet ordre est son numero de naissance ; c'est aussi l'indice du departage des aretes de meme longueur.
struct Sites {
  std::vector<u32> x, y, z;
  std::vector<u32> entree;  // indice d'entree (ligne du fichier .u32le) de chaque site canonique
  u32 coordonnee_max = 0;
  u32 taille() const { return static_cast<u32>(x.size()); }
};

inline u32 petit_boutiste(const unsigned char* p) {
  return u32{p[0]} | (u32{p[1]} << 8) | (u32{p[2]} << 16) | (u32{p[3]} << 24);
}

// Lit un fichier de mots u32 petit-boutistes ; refuse un fichier vide ou de taille non multiple de `par_point` mots.
inline bool lire_mots(const std::string& chemin, u64 par_point, std::vector<u32>& mots, std::string& erreur) {
  std::ifstream entree(chemin, std::ios::binary | std::ios::ate);
  if (!entree) {
    erreur = "fichier illisible : " + chemin;
    return false;
  }
  const std::streamoff octets = entree.tellg();
  if (octets <= 0 || static_cast<u64>(octets) % (4 * par_point) != 0) {
    erreur = "taille de " + chemin + " : " + std::to_string(octets) + " octets, multiple non nul de " +
             std::to_string(4 * par_point) + " attendu";
    return false;
  }
  const u64 compte = static_cast<u64>(octets) / 4;
  mots.resize(compte);
  entree.seekg(0);
  std::vector<unsigned char> bloc(1 << 20);
  for (u64 debut = 0; debut < compte; debut += bloc.size() / 4) {
    const u64 n = std::min<u64>(bloc.size() / 4, compte - debut);
    entree.read(reinterpret_cast<char*>(bloc.data()), static_cast<std::streamsize>(4 * n));
    if (!entree) {
      erreur = "lecture interrompue : " + chemin;
      return false;
    }
    for (u64 j = 0; j < n; ++j) mots[debut + j] = petit_boutiste(bloc.data() + 4 * j);
  }
  return true;
}

// Lit le nuage, le trie dans l'ordre canonique et refuse les doublons (code 3 de l'appelant). Au plus 2^31 - 1
// sites : l'arbre en a au plus 2 n - 1 noeuds, numerotes en u32 sous la sentinelle 2^32 - 1.
inline bool lire_sites(const std::string& chemin, Sites& sites, std::string& erreur) {
  std::vector<u32> mots;
  if (!lire_mots(chemin, 3, mots, erreur)) return false;
  const u64 n = mots.size() / 3;
  if (n > 0x7FFFFFFFull) {
    erreur = "nuage de " + std::to_string(n) + " points : au plus 2^31 - 1";
    return false;
  }
  struct Point {
    u32 x, y, z, i;
  };
  std::vector<Point> points(n);
  for (u64 i = 0; i < n; ++i) {
    points[i] = {mots[3 * i], mots[3 * i + 1], mots[3 * i + 2], static_cast<u32>(i)};
  }
  std::vector<u32>().swap(mots);
  std::sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    if (a.z != b.z) return a.z < b.z;
    return a.i < b.i;
  });
  for (u64 i = 1; i < n; ++i) {
    const Point& a = points[i - 1];
    const Point& b = points[i];
    if (a.x == b.x && a.y == b.y && a.z == b.z) {
      erreur = "doublon (decision D8 : refus) : position (" + std::to_string(a.x) + ", " + std::to_string(a.y) +
               ", " + std::to_string(a.z) + ") aux lignes " + std::to_string(a.i) + " et " + std::to_string(b.i);
      return false;
    }
  }
  sites.x.resize(n);
  sites.y.resize(n);
  sites.z.resize(n);
  sites.entree.resize(n);
  sites.coordonnee_max = 0;
  for (u64 i = 0; i < n; ++i) {
    sites.x[i] = points[i].x;
    sites.y[i] = points[i].y;
    sites.z[i] = points[i].z;
    sites.entree[i] = points[i].i;
    sites.coordonnee_max = std::max({sites.coordonnee_max, points[i].x, points[i].y, points[i].z});
  }
  return true;
}

// Ordre de Morton exact des vidages de la v11 : le bit `b` de l'axe `a` (x = 0, y = 1, z = 2) occupe la position
// 3 b + a de la cle. On compare sans former la cle : l'axe decisif est celui dont le ou exclusif a le bit de poids
// fort le plus haut, l'axe le plus grand l'emportant a egalite de position (il est plus significatif).
inline bool msb_inferieur(u32 a, u32 b) { return a < b && a < (a ^ b); }

inline bool morton_inferieur(const std::array<u32, 3>& p, const std::array<u32, 3>& q) {
  int axe = 0;
  u32 meilleur = p[0] ^ q[0];
  for (int a = 1; a < 3; ++a) {
    const u32 x = p[a] ^ q[a];
    if (x != 0 && !msb_inferieur(x, meilleur)) {
      axe = a;
      meilleur = x;
    }
  }
  return p[axe] < q[axe];
}

// Numeros canoniques des sites dans l'ordre de Morton exact.
inline std::vector<u32> ordre_morton(const Sites& sites) {
  std::vector<u32> ordre(sites.taille());
  for (u32 i = 0; i < sites.taille(); ++i) ordre[i] = i;
  std::sort(ordre.begin(), ordre.end(), [&](u32 a, u32 b) {
    return morton_inferieur({sites.x[a], sites.y[a], sites.z[a]}, {sites.x[b], sites.y[b], sites.z[b]});
  });
  return ordre;
}

}  // namespace mhgp12_emst

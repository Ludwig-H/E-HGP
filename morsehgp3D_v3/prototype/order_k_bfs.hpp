// MorseHGP3D v3 — M3 : le catalogue par NAVIGATION dans l'arrangement releve.
//
// ---------------------------------------------------------------------------
// Pourquoi ce fichier remplace la source d'ancres
// ---------------------------------------------------------------------------
//
// Le chemin ancre enumere les C(n,2) paires. A 50 000 points cela fait 1,25
// milliard d'ancres AVANT tout travail utile, et la mesure du clipping de JUNG
// l'a confirme : la lentille retire environ deux tiers des points mais m_e
// reste proportionnel a n. Le clipping gagne un grand facteur constant, pas un
// ordre. Le verrou n'est donc pas le constructeur local : c'est l'enumeration.
//
// On change d'objet. Par le relevement de LIFTING
//
//     phi(p) = (p_x, p_y, p_z, |p|^2),
//
// une sphere de centre c et de rayon r devient l'hyperplan
//
//     x_4 = 2 c . x - (|c|^2 - r^2),
//
// et « z est strictement interieur a la sphere » devient « phi(z) est
// strictement SOUS l'hyperplan ». Un sommet de l'arrangement de ces n
// hyperplans dans R^4 est l'intersection de quatre d'entre eux, c'est-a-dire
// une sphere passant par quatre points ; son NIVEAU est le nombre de points
// strictement interieurs. Le catalogue des spheres critiques de rang ferme au
// plus s_max est donc exactement le <=k-niveau de cet arrangement, avec
//
//     rang ferme = 4 + niveau.
//
// On ne l'enumere pas : on le PARCOURT. Deux sommets voisins partagent trois
// hyperplans, donc trois points : ils sont relies par une arete de
// l'arrangement, qui est le pinceau des spheres passant par un triangle fixe.
// Le long de ce pinceau un seul point change d'etat, donc
//
//     niveau(voisin) = niveau(courant) +- 1.
//
// Le niveau ne se RECALCULE jamais : il se transporte. C'est ce qui rend le
// parcours output-sensitive — le cout suit la taille du catalogue, pas n^2.
//
// ---------------------------------------------------------------------------
// Le pinceau, exactement
// ---------------------------------------------------------------------------
//
// Pour un triangle (a,b,c) de normale u = (b-a) x (c-a), les centres des
// spheres passant par a, b, c decrivent la droite c_0 + t u. En posant
//
//     o(z) = signe de orient3d(a,b,c,z) = signe de u.(z-a),
//
// la sphere de parametre t contient z strictement des que t > t_z lorsque
// o(z) > 0, et des que t < t_z lorsque o(z) < 0 : quand t -> +oo la boule tend
// vers le demi-espace {u.(x-a) > 0}. On en tire l'identite qui remplace tout
// calcul de centre rationnel :
//
//     signe(t_z - t_w) = o(w) * [w interieur a sphere(a,b,c,z) ? +1 : -1].
//
// Tout l'ordre du pinceau se lit donc en predicats InSphere entiers. Aucun
// rationnel, aucun centre, aucune division.
//
// ---------------------------------------------------------------------------
// Le germe
// ---------------------------------------------------------------------------
//
// Le parcours doit demarrer a un sommet de niveau 0. On prend une FACE DE
// L'ENVELOPPE CONVEXE : tous les points sont d'un meme cote, donc en t = -oo la
// boule est un demi-espace VIDE. Le premier point rencontre en augmentant t
// donne un tetraedre de DELAUNAY, de niveau 0 par construction. La face est
// obtenue par emballage : lex-min, arete du contour 2D, puis rotation autour
// de cette arete.
//
// ---------------------------------------------------------------------------
// Les degenerescences sont TRAITEES, pas exclues
// ---------------------------------------------------------------------------
//
// Sur un nuage LIDAR quantifie de 500 points, le parcours rencontre deja une
// cospherie a cinq points DANS le <=k-niveau. Le germe est propre et compare_t
// ne rend zero que si un cinquieme point est exactement sur la sphere : c'est
// une vraie degenerescence, pas un defaut de predicat. A 50 000 points sur une
// grille u16 elle est certaine. L'exactitude etant contractuelle, on ne peut
// pas s'en debarrasser par un rejet de domaine.
//
// Un sommet ne porte donc pas quatre points mais sa COQUILLE entiere : quand m
// hyperplans se rencontrent, le sommet est degenere et ses aretes incidentes
// sont indexees par les 3-sous-ensembles de la coquille. Le cas non degenere,
// m = 4, redonne exactement les quatre faces d'un tetraedre. Les points a
// parametre egal le long d'un pinceau entrent donc ENSEMBLE, en un seul lot.
//
// Le critere de criticite devient uniforme et se lit sans cas particulier :
// une boule est critique si et seulement si elle EST la miniboule de sa
// coquille. Le support HGP est alors celui de cette miniboule, d'au plus quatre
// points, et le rang ferme compte tous les membres, coquille surnumeraire
// comprise.
//
// ---------------------------------------------------------------------------
// Ce que ce fichier ne pretend PAS
// ---------------------------------------------------------------------------
//
// La connexite du <=k-niveau sous cette adjacence n'est pas demontree ici. Elle
// est PLAUSIBLE — le niveau croit le long de toute verticale montante, donc la
// region est connexe et etoilee depuis la cellule de niveau 0 — mais la
// connexite de la region ne donne pas gratuitement celle du graphe sommets-
// aretes restreint. C'est l'oracle exhaustif qui tranche, nuage par nuage, et
// un manque se lira comme un support absent. Tant que ce juge n'a pas parle,
// aucune completude n'est revendiquee.
#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/anchored_catalogue.hpp"

namespace mhgp3v {

struct OrderKStatistics {
  long long seed_scans = 0;            // O(n) payes pour le germe
  long long vertices_visited = 0;      // sommets du <=k-niveau atteints
  long long vertices_beyond = 0;       // atteints puis coupes sur le rang
  long long pencil_queries = 0;        // requetes « point suivant »
  long long pencil_candidates = 0;     // travail reel de ces requetes
  long long unbounded_stops = 0;       // pinceau sans successeur : face d'enveloppe
  long long level_recomputed = 0;      // DOIT rester a 1 : le germe seul
  long long degenerate_shells = 0;     // cospheries : hors domaine declare
  long long cocircular_pencil = 0;     // deux points au meme t : hors domaine
  long long emitted_arity[5] = {};
  long long harvest_faces = 0;
  long long harvest_edges = 0;

  // Un compteur oublie au site de sommation reste a zero dans le recu, et un
  // lecteur y lit « branche morte ». Le static_assert casse le build si un
  // champ est ajoute sans etre somme ici.
  void absorb(const OrderKStatistics& other) {
    static_assert(sizeof(OrderKStatistics) == 16 * sizeof(long long),
                  "champ ajoute a OrderKStatistics : le sommer dans absorb()");
    seed_scans += other.seed_scans;
    vertices_visited += other.vertices_visited;
    vertices_beyond += other.vertices_beyond;
    pencil_queries += other.pencil_queries;
    pencil_candidates += other.pencil_candidates;
    unbounded_stops += other.unbounded_stops;
    level_recomputed += other.level_recomputed;
    degenerate_shells += other.degenerate_shells;
    cocircular_pencil += other.cocircular_pencil;
    for (int i = 0; i < 5; ++i) emitted_arity[i] += other.emitted_arity[i];
    harvest_faces += other.harvest_faces;
    harvest_edges += other.harvest_edges;
  }
};

namespace bfs {

// --------------------------------------------------------------------------
// Predicats entiers exacts.
//
// Les coordonnees tiennent sur la grille declaree u16, donc |p| < 2^16 et les
// differences sous 2^17. Un mineur 3x3 de differences reste sous 6*2^51 <
// 2^53,6 ; la colonne relevee |x-a|^2 reste sous 3*2^34 < 2^35,6. Le
// determinant 4x4, developpe le long de la colonne relevee, reste donc sous
// 4 * 2^35,6 * 2^53,6 = 2^91,2 : tres largement dans i128.
// --------------------------------------------------------------------------

inline mhgp::i128 orient3d_exact(const mhgp::P3& a, const mhgp::P3& b,
                           const mhgp::P3& c, const mhgp::P3& d) {
  const mhgp::i128 bx = b.x - a.x, by = b.y - a.y, bz = b.z - a.z;
  const mhgp::i128 cx = c.x - a.x, cy = c.y - a.y, cz = c.z - a.z;
  const mhgp::i128 dx = d.x - a.x, dy = d.y - a.y, dz = d.z - a.z;
  return bx * (cy * dz - cz * dy) - by * (cx * dz - cz * dx) + bz * (cx * dy - cy * dx);
}

inline int sign_of(mhgp::i128 v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// Signe du determinant 4x4 releve. Combine avec l'orientation du tetraedre il
// donne : -1 interieur strict, 0 sur la coquille, +1 exterieur strict.
inline int in_sphere_side(const mhgp::P3& a, const mhgp::P3& b, const mhgp::P3& c,
                          const mhgp::P3& d, const mhgp::P3& e, int orient_sign) {
  auto row = [&a](const mhgp::P3& p, mhgp::i128* x, mhgp::i128* y, mhgp::i128* z,
                  mhgp::i128* w) {
    *x = p.x - a.x; *y = p.y - a.y; *z = p.z - a.z;
    *w = (*x) * (*x) + (*y) * (*y) + (*z) * (*z);
  };
  mhgp::i128 bx, by, bz, bw, cx, cy, cz, cw, dx, dy, dz, dw, ex, ey, ez, ew;
  row(b, &bx, &by, &bz, &bw);
  row(c, &cx, &cy, &cz, &cw);
  row(d, &dx, &dy, &dz, &dw);
  row(e, &ex, &ey, &ez, &ew);
  // Developpement le long de la colonne relevee : det = -bw M_b + cw M_c
  // - dw M_d + ew M_e, ou M_* est le mineur 3x3 des trois autres lignes.
  auto minor = [](mhgp::i128 x1, mhgp::i128 y1, mhgp::i128 z1,
                  mhgp::i128 x2, mhgp::i128 y2, mhgp::i128 z2,
                  mhgp::i128 x3, mhgp::i128 y3, mhgp::i128 z3) {
    return x1 * (y2 * z3 - z2 * y3) - y1 * (x2 * z3 - z2 * x3) + z1 * (x2 * y3 - y2 * x3);
  };
  const mhgp::i128 det = -bw * minor(cx, cy, cz, dx, dy, dz, ex, ey, ez)
                       + cw * minor(bx, by, bz, dx, dy, dz, ex, ey, ez)
                       - dw * minor(bx, by, bz, cx, cy, cz, ex, ey, ez)
                       + ew * minor(bx, by, bz, cx, cy, cz, dx, dy, dz);
  const int s = sign_of(det);
  if (s == 0) return 0;
  // Convention CALIBREE sur un cas explicite, pas supposee : pour le tetraedre
  // regulier de centre (2,2,2) et de rayon carre 3, le centre doit sortir
  // interieur et (100,100,100) exterieur. Le signe global etait inverse, et
  // l'accord force-brute / parcours ne pouvait pas le voir : les deux partagent
  // ce predicat. C'est le nombre de tetraedres de DELAUNAY — 57 pour 120 points
  // au lieu de plusieurs centaines, soit exactement la DELAUNAY DU POINT LE PLUS
  // ELOIGNE — qui a trahi l'inversion.
  return (s * orient_sign < 0) ? -1 : 1;
}

// Clef canonique d'un support de quatre indices tries.
inline unsigned long long key4(mhgp::i32 a, mhgp::i32 b, mhgp::i32 c, mhgp::i32 d) {
  mhgp::i32 v[4] = {a, b, c, d};
  std::sort(v, v + 4);
  unsigned long long k = 0;
  for (int i = 0; i < 4; ++i) k = (k << 16) | static_cast<unsigned long long>(v[i] & 0xFFFF);
  return k;
}

struct Vertex {
  std::vector<mhgp::i32> shell;   // TOUS les points sur la sphère, tries
  int level;                      // points strictement interieurs
};

// --------------------------------------------------------------------------
// Ordre du pinceau. Toutes les comparaisons passent par l'identite
//     signe(t_z - t_w) = o(w) * [w interieur a sphere(a,b,c,z) ? +1 : -1].
// --------------------------------------------------------------------------
struct Pencil {
  const std::vector<mhgp::P3>* points;
  mhgp::i32 a, b, c;

  int orient(mhgp::i32 z) const {
    return sign_of(orient3d_exact((*points)[static_cast<std::size_t>(a)],
                            (*points)[static_cast<std::size_t>(b)],
                            (*points)[static_cast<std::size_t>(c)],
                            (*points)[static_cast<std::size_t>(z)]));
  }

  // Position de w relativement a la sphere passant par (a,b,c,z).
  int side(mhgp::i32 z, mhgp::i32 w, int orient_z) const {
    return in_sphere_side((*points)[static_cast<std::size_t>(a)],
                          (*points)[static_cast<std::size_t>(b)],
                          (*points)[static_cast<std::size_t>(c)],
                          (*points)[static_cast<std::size_t>(z)],
                          (*points)[static_cast<std::size_t>(w)], orient_z);
  }

  // signe(t_z - t_w), avec o_w = orient(w) suppose non nul.
  int compare_t(mhgp::i32 z, int orient_z, mhgp::i32 w, int orient_w) const {
    const int inside = side(z, w, orient_z);      // -1 interieur, +1 exterieur
    if (inside == 0) return 0;                    // t_z = t_w : cocircularite
    return orient_w * (inside < 0 ? 1 : -1);
  }
};

}  // namespace bfs

// ---------------------------------------------------------------------------
// Le parcours. Renvoie les sommets visites de rang ferme au plus rank_ceiling.
// Un sommet porte sa COQUILLE entiere : le cas degenere n'est pas un cas
// particulier, c'est le cas general avec |coquille| > 4.
// ---------------------------------------------------------------------------

namespace bfs {

struct ShellHash {
  std::size_t operator()(const std::vector<mhgp::i32>& v) const {
    std::size_t h = 1469598103934665603ULL;
    for (mhgp::i32 x : v) { h ^= static_cast<std::size_t>(x) + 0x9e3779b9; h *= 1099511628211ULL; }
    return h;
  }
};

}  // namespace bfs

inline std::vector<bfs::Vertex> order_k_vertices(const std::vector<mhgp::P3>& points,
                                                 int rank_ceiling,
                                                 OrderKStatistics* statistics,
                                                 bool* out_of_domain) {
  const int n = static_cast<int>(points.size());
  std::vector<bfs::Vertex> visited;
  *out_of_domain = false;
  if (n < 4) return visited;

  // ---- GERME : une face de l'enveloppe convexe -----------------------------
  mhgp::i32 p0 = 0;
  for (mhgp::i32 i = 1; i < n; ++i) {
    const mhgp::P3& u = points[static_cast<std::size_t>(i)];
    const mhgp::P3& v = points[static_cast<std::size_t>(p0)];
    if (u.x < v.x || (u.x == v.x && (u.y < v.y || (u.y == v.y && u.z < v.z)))) p0 = i;
  }
  mhgp::i32 p1 = -1;
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0) continue;
    if (p1 < 0) { p1 = z; continue; }
    const mhgp::i128 ax = points[static_cast<std::size_t>(p1)].x - points[static_cast<std::size_t>(p0)].x;
    const mhgp::i128 ay = points[static_cast<std::size_t>(p1)].y - points[static_cast<std::size_t>(p0)].y;
    const mhgp::i128 bx = points[static_cast<std::size_t>(z)].x - points[static_cast<std::size_t>(p0)].x;
    const mhgp::i128 by = points[static_cast<std::size_t>(z)].y - points[static_cast<std::size_t>(p0)].y;
    const mhgp::i128 cross = ax * by - ay * bx;
    if (cross < 0) p1 = z;
    else if (cross == 0 && (bx * bx + by * by) > (ax * ax + ay * ay)) p1 = z;
  }
  mhgp::i32 p2 = -1;
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0 || z == p1) continue;
    if (p2 < 0) { p2 = z; continue; }
    if (bfs::orient3d_exact(points[static_cast<std::size_t>(p0)],
                            points[static_cast<std::size_t>(p1)],
                            points[static_cast<std::size_t>(p2)],
                            points[static_cast<std::size_t>(z)]) < 0) p2 = z;
  }
  statistics->seed_scans += 3;
  if (p2 < 0) { *out_of_domain = true; return visited; }

  bfs::Pencil face{&points, p0, p1, p2};
  int supporting = 0;
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0 || z == p1 || z == p2) continue;
    const int o = face.orient(z);
    if (o == 0) continue;
    if (supporting == 0) supporting = o;
    else if (supporting != o) { *out_of_domain = true; return visited; }
  }
  ++statistics->seed_scans;
  if (supporting == 0) { *out_of_domain = true; return visited; }

  // En t = -oo la boule est le demi-espace VIDE. Le premier LOT rencontre — un
  // lot, car plusieurs points peuvent partager le meme parametre — donne un
  // sommet de niveau zero.
  mhgp::i32 seed = -1;
  int seed_orient = 0;
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0 || z == p1 || z == p2) continue;
    const int oz = face.orient(z);
    if (oz == 0) continue;
    if (seed < 0) { seed = z; seed_orient = oz; continue; }
    const int cmp = face.compare_t(seed, seed_orient, z, oz);
    if (cmp > 0) { seed = z; seed_orient = oz; }
  }
  ++statistics->seed_scans;
  if (seed < 0) { *out_of_domain = true; return visited; }

  std::vector<mhgp::i32> root_shell{p0, p1, p2, seed};
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0 || z == p1 || z == p2 || z == seed) continue;
    const int oz = face.orient(z);
    if (oz == 0) {
      // Coplanaire avec la face : son etat ne depend pas du parametre, car
      // toute sphère du pinceau coupe ce plan selon le MEME cercle. S'il est
      // dessus, il appartient a toutes les coquilles du pinceau.
      if (face.side(seed, z, seed_orient) == 0) {
        root_shell.push_back(z);
        ++statistics->cocircular_pencil;
      }
      continue;
    }
    if (face.compare_t(seed, seed_orient, z, oz) == 0) {   // meme parametre : meme sphère
      root_shell.push_back(z);
      ++statistics->cocircular_pencil;
    }
  }
  std::sort(root_shell.begin(), root_shell.end());
  ++statistics->level_recomputed;

  // ---- PARCOURS -----------------------------------------------------------
  std::unordered_set<std::vector<mhgp::i32>, bfs::ShellHash> seen;
  std::vector<bfs::Vertex> frontier;
  seen.insert(root_shell);
  frontier.push_back(bfs::Vertex{root_shell, 0});

  std::vector<mhgp::i32> tied;
  while (!frontier.empty()) {
    const bfs::Vertex v = frontier.back();
    frontier.pop_back();
    const int shell_size = static_cast<int>(v.shell.size());
    if (shell_size + v.level > rank_ceiling) { ++statistics->vertices_beyond; continue; }
    visited.push_back(v);
    ++statistics->vertices_visited;

    // Les aretes incidentes sont indexees par les 3-sous-ensembles de la
    // coquille : quatre pour un sommet simple, C(m,3) pour un sommet degenere.
    for (int i = 0; i < shell_size; ++i)
    for (int j = i + 1; j < shell_size; ++j)
    for (int k = j + 1; k < shell_size; ++k) {
      const mhgp::i32 tri[3] = {v.shell[static_cast<std::size_t>(i)],
                                v.shell[static_cast<std::size_t>(j)],
                                v.shell[static_cast<std::size_t>(k)]};
      bfs::Pencil pencil{&points, tri[0], tri[1], tri[2]};
      mhgp::i32 apex = -1;
      int orient_apex = 0;
      for (int t = 0; t < shell_size; ++t) {
        if (t == i || t == j || t == k) continue;
        const mhgp::i32 z = v.shell[static_cast<std::size_t>(t)];
        const int oz = pencil.orient(z);
        if (oz != 0) { apex = z; orient_apex = oz; break; }
      }
      if (apex < 0) continue;                        // triangle degenere

      for (int direction = -1; direction <= 1; direction += 2) {
        ++statistics->pencil_queries;
        mhgp::i32 best = -1;
        int best_orient = 0;
        for (mhgp::i32 z = 0; z < n; ++z) {
          if (std::binary_search(v.shell.begin(), v.shell.end(), z)) continue;
          const int oz = pencil.orient(z);
          if (oz == 0) continue;     // coplanaire : etat constant le long du pinceau
          ++statistics->pencil_candidates;
          if (pencil.compare_t(z, oz, apex, orient_apex) != direction) continue;
          if (best < 0) { best = z; best_orient = oz; continue; }
          if (pencil.compare_t(z, oz, best, best_orient) == -direction) { best = z; best_orient = oz; }
        }
        if (best < 0) { ++statistics->unbounded_stops; continue; }

        // Le LOT : tout ce qui partage le parametre du meilleur candidat entre
        // en meme temps. C'est ici que la degenerescence est absorbee.
        tied.assign(1, best);
        for (mhgp::i32 z = 0; z < n; ++z) {
          if (z == best) continue;
          if (std::binary_search(v.shell.begin(), v.shell.end(), z)) continue;
          const int oz = pencil.orient(z);
          if (oz == 0) {
            // Sur le cercle du triangle : present dans TOUTE coquille du
            // pinceau, donc aussi dans la nouvelle.
            if (pencil.side(best, z, best_orient) == 0) {
              tied.push_back(z);
              ++statistics->cocircular_pencil;
            }
            continue;
          }
          if (pencil.compare_t(z, oz, best, best_orient) == 0) {
            tied.push_back(z);
            ++statistics->cocircular_pencil;
          }
        }

        // Transport du niveau : ce qui entre, ce qui sort. Rien d'autre ne
        // change d'etat entre les deux parametres.
        int level = v.level;
        for (mhgp::i32 z : tied)
          if (pencil.side(apex, z, orient_apex) < 0) --level;      // etait dedans
        for (int t = 0; t < shell_size; ++t) {
          if (t == i || t == j || t == k) continue;
          const mhgp::i32 z = v.shell[static_cast<std::size_t>(t)];
          if (pencil.side(best, z, best_orient) < 0) ++level;      // entre dedans
        }
        if (level < 0) { *out_of_domain = true; return visited; }

        std::vector<mhgp::i32> shell(tri, tri + 3);
        shell.insert(shell.end(), tied.begin(), tied.end());
        std::sort(shell.begin(), shell.end());
        if (static_cast<int>(shell.size()) + level > rank_ceiling) continue;
        if (seen.insert(shell).second) frontier.push_back(bfs::Vertex{shell, level});
      }
    }
  }
  return visited;
}

}  // namespace mhgp3v

namespace mhgp3v {

// ---------------------------------------------------------------------------
// Du parcours au CATALOGUE.
//
// Le critere est unifie et sans cas particulier : une boule est CRITIQUE si et
// seulement si elle est la MINIBOULE de sa coquille. Cela couvre d'un coup le
// bien-centrage des arites deux, trois et quatre, et les coquilles
// surnumeraires — une sphère portant cinq points reste UNE boule, dont le
// support HGP est celui de sa miniboule et dont le rang compte les cinq.
//
// Completude des arites basses. Une arete (a,b) de rang r se relie a un sommet
// en deux pas : grossir la sphère diametrale jusqu'au premier troisieme point
// donne le rang r+1, puis suivre le pinceau jusqu'au premier quatrieme donne
// r+2. Un triangle ne demande qu'un pas. Il faut donc parcourir jusqu'au rang
// s_max + 2 pour ne rien perdre en arite deux.
// ---------------------------------------------------------------------------

inline mhgp::Catalogue order_k_catalogue(const std::vector<mhgp::P3>& points, int s_max,
                                         OrderKStatistics* statistics,
                                         bool* out_of_domain) {
  *statistics = OrderKStatistics{};
  mhgp::Catalogue catalogue;
  const int n = static_cast<int>(points.size());
  *out_of_domain = false;

  std::vector<mhgp::CriticalSphere> kept;
  std::vector<mhgp::i32> members_pool;
  std::unordered_set<std::vector<mhgp::i32>, bfs::ShellHash> emitted;

  // Emet la boule portee par un support candidat, si elle est critique.
  auto try_emit = [&](const mhgp::i32* candidate, int m) {
    const mhgp::MiniballResult mb = mhgp::miniball_of(points, candidate, m);
    if (!mb.ok) return;
    // La miniboule doit passer par TOUT le candidat : sinon la sphère de ce
    // support n'est pas minimale, donc n'est pas une sphère critique. Sa
    // sous-sphère, elle, sera recoltee ailleurs.
    for (int i = 0; i < m; ++i)
      if (mhgp::sphere_side(mb.sph, points[static_cast<std::size_t>(candidate[i])]) != 0) return;

    std::vector<mhgp::i32> members, shell;
    for (mhgp::i32 z = 0; z < n; ++z) {
      const int side = mhgp::sphere_side(mb.sph, points[static_cast<std::size_t>(z)]);
      if (side > 0) continue;
      if (side == 0) shell.push_back(z);
      members.push_back(z);
    }
    if (static_cast<int>(members.size()) > s_max) return;
    if (!emitted.insert(shell).second) return;      // clef canonique : la coquille

    mhgp::CriticalSphere critical;
    for (int i = 0; i < mhgp::kMaxSupport; ++i)
      critical.support[i] = i < mb.n_support ? mb.support[i] : -1;
    critical.n_support = mb.n_support;
    critical.rank = static_cast<int>(members.size());
    critical.sph = mb.sph;
    critical.beta = mhgp::sphere_beta(mb.sph);
    critical.members_begin = static_cast<mhgp::i32>(members_pool.size());
    members_pool.insert(members_pool.end(), members.begin(), members.end());
    kept.push_back(critical);
    ++statistics->emitted_arity[mb.n_support];
  };

  for (mhgp::i32 p = 0; p < n; ++p) try_emit(&p, 1);

  const auto vertices = order_k_vertices(points, s_max + 2, statistics, out_of_domain);
  if (*out_of_domain) return catalogue;

  std::unordered_set<std::vector<mhgp::i32>, bfs::ShellHash> seen_sub;
  for (const bfs::Vertex& v : vertices) {
    try_emit(v.shell.data(), static_cast<int>(v.shell.size()));
    const int m = static_cast<int>(v.shell.size());
    for (int i = 0; i < m; ++i)
      for (int j = i + 1; j < m; ++j) {
        const mhgp::i32 e[2] = {v.shell[static_cast<std::size_t>(i)],
                                v.shell[static_cast<std::size_t>(j)]};
        if (seen_sub.insert(std::vector<mhgp::i32>(e, e + 2)).second) {
          ++statistics->harvest_edges;
          try_emit(e, 2);
        }
        for (int k = j + 1; k < m; ++k) {
          const mhgp::i32 f[3] = {e[0], e[1], v.shell[static_cast<std::size_t>(k)]};
          if (!seen_sub.insert(std::vector<mhgp::i32>(f, f + 3)).second) continue;
          ++statistics->harvest_faces;
          try_emit(f, 3);
        }
      }
  }

  std::vector<int> order(kept.size());
  for (std::size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
  std::sort(order.begin(), order.end(), [&](int x, int y) {
    const mhgp::CriticalSphere& u = kept[static_cast<std::size_t>(x)];
    const mhgp::CriticalSphere& w = kept[static_cast<std::size_t>(y)];
    if (u.n_support != w.n_support) return u.n_support < w.n_support;
    for (int i = 0; i < u.n_support; ++i)
      if (u.support[i] != w.support[i]) return u.support[i] < w.support[i];
    return false;
  });
  for (int idx : order) {
    mhgp::CriticalSphere critical = kept[static_cast<std::size_t>(idx)];
    const int begin = critical.members_begin;
    critical.members_begin = static_cast<mhgp::i32>(catalogue.members.size());
    for (int i = 0; i < critical.rank; ++i)
      catalogue.members.push_back(members_pool[static_cast<std::size_t>(begin + i)]);
    catalogue.spheres.push_back(critical);
  }
  return catalogue;
}

}  // namespace mhgp3v

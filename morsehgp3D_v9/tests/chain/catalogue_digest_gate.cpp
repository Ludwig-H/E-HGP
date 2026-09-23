// MorseHGP3D v9 — porte du condense canonique du catalogue (proposition de l'auditeur C, 23 septembre 2026,
// pour le condense v18 du developpeur).
//
// Le condense FULL (tower_digest) ne voit pas une boule redondante omise : la tour n'en depend pas. Le
// condense du catalogue (ChainOptions::catalogue_digest) la voit. Cette porte juge :
//   - recalcul : le condense publie par la chaine egale un recalcul local, sur le catalogue garde, selon
//     l'encodage documente (tower_chain.hpp : boules triees par cle, puis cle, niveau exact, arite,
//     identifiants tries des interieurs puis de la coquille, FNV-1a 64 ; identifiants = indices de l'index) ;
//   - egalites : moteur un fil = moteur quatre fils = lots CPU = certificats S3 CPU (quatre fils),
//     catalogues canoniques egaux ; option coupee : condense nul, temps nul, tour FULL identique ; ordre de
//     la liste indifferent ;
//   - sensibilite : boule retiree, identifiant change, site passe de l'interieur a la coquille, niveau,
//     arite, cle : chaque alteration change le condense ;
//   - coquilles etendues (fixture gravee, jugee en premier) : huit coins d'un cube (arite 2, coquille 8), un
//     triangle aigu et un point hors plan sur sa sphere (arite 3, coquille 4), cinq points entiers de la
//     sphere x^2+y^2+z^2 = 9 sans paire antipodale ni triplet coplanaire avec le centre (arite 4, coquille
//     5), mis a l'echelle 100 et eloignes d'un fond de 400 points ; ces trois boules doivent etre au
//     catalogue, et le condense recalcule, egal entre moteur 1/4 fils et lots CPU ;
//   - motivation (plancher par arite) : des boules de la zone aveugle (q2 a p = Kmax-1, q3 a p = Kmax-2),
//     vingt par arite prises a pas regulier dans le catalogue, retirees une a une : au moins une par arite
//     laisse la tour FULL et son condense inchanges, et le condense du catalogue les voit toutes ;
//   - mutants (cibles recompilees) : condense omettant la derniere boule (SKIP_LAST) ; coquille etendue
//     hachee sur ses seuls `arite` premiers sites (SHELL_ARITY_ONLY). Le recalcul les refuse.
//
//   mhgp9_catalogue_digest_gate [--n=1500]
//
// Code 0 conforme, 1 desaccord (`cause=`), 2 argument, 3 plancher ou chaine incomplete.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../gen/front_fixtures.hpp"

namespace {

using mhgp9::tower::BallData;

int fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return 1;
}

// Encodage documente, reecrit ici : FNV-1a 64 octet par octet, mots de 64 bits petit-boutistes.
struct Fnv {
  std::uint64_t h = 14695981039346656037ull;
  void word(std::uint64_t w) {
    for (int i = 0; i < 8; ++i) {
      h ^= (w >> (8 * i)) & 0xffu;
      h *= 1099511628211ull;
    }
  }
  void wide(mhgp9::tower::i128 v) {
    const auto u = static_cast<mhgp9::tower::u128>(v);
    word(static_cast<std::uint64_t>(u));
    word(static_cast<std::uint64_t>(u >> 64));
  }
};

// Recalcul local de l'encodage documente, a un etage.
std::uint64_t local_digest(std::vector<BallData> balls) {
  std::sort(balls.begin(), balls.end(), [](const BallData& l, const BallData& r) { return l.key < r.key; });
  Fnv f;
  f.word(balls.size());
  for (const auto& b : balls) {
    f.wide(b.key.a);
    for (const auto x : b.key.b) f.wide(x);
    f.wide(b.key.c);
    for (const auto w : b.level.num) f.word(w);
    f.wide(static_cast<mhgp9::tower::i128>(b.level.den));
    f.word(b.arity);
    for (const auto part : {b.interior(), b.shell()}) {
      std::vector<std::uint64_t> ids;
      for (const auto u : part) ids.push_back(static_cast<std::uint32_t>(u));
      std::sort(ids.begin(), ids.end());
      f.word(ids.size());
      for (const auto id : ids) f.word(id);
    }
  }
  return f.h;
}

using Row = std::tuple<std::array<mhgp9::tower::i128, 5>, std::array<std::uint64_t, 3>, mhgp9::tower::i128, unsigned,
                       std::vector<std::int32_t>, std::vector<std::int32_t>>;

std::vector<Row> canonical(const std::vector<BallData>& cat) {
  std::vector<Row> rows;
  for (const auto& b : cat) {
    Row r{{b.key.a, b.key.b[0], b.key.b[1], b.key.b[2], b.key.c},
          {b.level.num[0], b.level.num[1], b.level.num[2]},
          b.level.den,
          b.arity,
          {b.interior().begin(), b.interior().end()},
          {b.shell().begin(), b.shell().end()}};
    std::sort(std::get<4>(r).begin(), std::get<4>(r).end());
    std::sort(std::get<5>(r).begin(), std::get<5>(r).end());
    rows.push_back(std::move(r));
  }
  std::sort(rows.begin(), rows.end());
  return rows;
}

mhgp9::ChainResult run(const std::vector<mhgp9::gen::Point3>& pts, unsigned kmax, std::size_t workers, bool batch,
                       bool digest, bool certificates = false) {
  mhgp9::ChainOptions o;
  o.kmax = kmax;
  o.workers = workers;
  o.tower_static_threads = static_cast<int>(workers);
  o.keep_catalogue = true;
  o.run_tower = true;
  o.q34_batch_filter = batch || certificates;
  o.q34_batch_certificates = certificates;
  o.catalogue_digest = digest;
  return mhgp9::run_tower_chain(pts, o);
}

mhgp9::tower::CloudIndex rebuild_index(const std::vector<mhgp9::gen::Point3>& pts) {
  std::vector<mhgp9::tower::P3> p(pts.size());
  for (std::size_t i = 0; i < pts.size(); ++i) p[i] = mhgp9::tower::P3{pts[i].x, pts[i].y, pts[i].z};
  return mhgp9::tower::build_cloud_index(p);
}

// Fixture gravee de coquilles etendues (coordonnees exactes, echelle 100) et fond de 400 points dans un
// autre cube. Les configurations sont a plus de 40 000 unites l'une de l'autre et du fond.
std::vector<mhgp9::gen::Point3> cospheric_fixture() {
  using C = mhgp9::gen::Coordinate;
  std::vector<mhgp9::gen::Point3> pts;
  const auto add = [&](std::int64_t ox, std::int64_t x, std::int64_t y, std::int64_t z) {
    pts.push_back({static_cast<C>(ox + 100 * x), static_cast<C>(100000 + 100 * y), static_cast<C>(100000 + 100 * z)});
  };
  for (const int x : {-1, 1})
    for (const int y : {-1, 1})
      for (const int z : {-1, 1}) add(100000, x, y, z);  // arite 2, coquille 8
  for (const auto& p : {std::array<int, 3>{5, 0, 0}, {-3, 4, 0}, {-3, -4, 0}, {0, 0, 5}})
    add(140000, p[0], p[1], p[2]);  // arite 3, coquille 4 (triangle aigu du plan z = 0, point (0,0,5))
  for (const auto& p : {std::array<int, 3>{-3, 0, 0}, {-2, -2, -1}, {-2, -2, 1}, {-1, 2, -2}, {2, -1, 2}})
    add(180000, p[0], p[1], p[2]);  // arite 4, coquille 5
  std::uint64_t state = 0x9e3779b97f4a7c15ull;
  for (int i = 0; i < 400; ++i) {
    std::array<C, 3> c{};
    for (auto& v : c) {
      state = state * 6364136223846793005ull + 1442695040888963407ull;
      v = static_cast<C>((state >> 33) % 30000);
    }
    pts.push_back({c[0], c[1], c[2]});
  }
  return pts;
}

}  // namespace

int main(int argc, char** argv) {
  std::size_t n = 1500;
  for (int i = 1; i < argc; ++i) {
    const std::string_view a(argv[i]);
    if (a.starts_with("--n=")) {
      const auto v = a.substr(4);
      const auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), n);
      if (ec != std::errc{} || ptr != v.data() + v.size() || n < 200 || n > 100000) {
        std::fprintf(stderr, "argument refusal: --n must be in 200..100000\n");
        return 2;
      }
    } else {
      std::fprintf(stderr, "usage: mhgp9_catalogue_digest_gate [--n=1500]\n");
      return 2;
    }
  }

  std::uint64_t runs = 0, balls_seen = 0, extra_shell = 0;
  double digest_ms_max = 0;
  std::array<std::uint64_t, 5> extended_by_arity{};
  {
    const auto pts = cospheric_fixture();
    const std::string where = "cospheric K5";
    const auto e1 = run(pts, 5, 1, false, true);
    const auto e4 = run(pts, 5, 4, false, true);
    const auto b4 = run(pts, 5, 4, true, true);
    const auto c4 = run(pts, 5, 4, true, true, true);
    for (const auto* r : {&e1, &e4, &b4, &c4})
      if (r->status != mhgp9::ChainStatus::kComplete) {
        std::printf("cause=chain.incomplete %s reason=%s\n", where.c_str(), r->reason.c_str());
        return 3;
      }
    runs += 4;
    for (const auto* r : {&e1, &e4, &b4, &c4})
      if (r->catalogue_digest != local_digest(r->catalogue_balls)) return fail("catalogue_digest.recompute " + where);
    if (e1.catalogue_digest != e4.catalogue_digest || e4.catalogue_digest != b4.catalogue_digest ||
        b4.catalogue_digest != c4.catalogue_digest)
      return fail("catalogue_digest.paths " + where);
    const auto ce = canonical(e4.catalogue_balls);
    if (ce != canonical(e1.catalogue_balls) || ce != canonical(b4.catalogue_balls) || ce != canonical(c4.catalogue_balls))
      return fail("catalogue.differs " + where);
    bool engraved[5] = {};
    for (const auto& b : e4.catalogue_balls) {
      if (b.n_shell > b.arity && b.arity <= 4) ++extended_by_arity[b.arity];
      if (b.n_interior == 0 && ((b.arity == 2 && b.n_shell == 8) || (b.arity == 3 && b.n_shell == 4) ||
                                (b.arity == 4 && b.n_shell == 5)))
        engraved[b.arity] = true;
    }
    if (!engraved[2] || !engraved[3] || !engraved[4]) {
      std::printf("cause=floor.engraved_extended_shells q2=%d q3=%d q4=%d\n", engraved[2], engraved[3], engraved[4]);
      return 3;
    }
    balls_seen += e4.catalogue_balls.size();
  }
  std::vector<BallData> probe_catalogue;  // uniforme K5, pour la sensibilite et la motivation
  std::vector<mhgp9::gen::Point3> probe_points;
  std::uint64_t probe_tower_digest = 0;
  for (const char* family : {"uniform", "terrain", "clusters"}) {
    const auto fx = mhgp9::gen::bench::make_front_fixture(n, family, 3);
    for (const unsigned kmax : {5u, 10u}) {
      const std::string where = std::string(family) + " K" + std::to_string(kmax);
      const auto e1 = run(fx.points, kmax, 1, false, true);
      const auto e4 = run(fx.points, kmax, 4, false, true);
      const auto b4 = run(fx.points, kmax, 4, true, true);
      const auto c4 = run(fx.points, kmax, 4, true, true, true);
      const auto off = run(fx.points, kmax, 4, false, false);
      for (const auto* r : {&e1, &e4, &b4, &c4, &off})
        if (r->status != mhgp9::ChainStatus::kComplete) {
          std::printf("cause=chain.incomplete %s reason=%s\n", where.c_str(), r->reason.c_str());
          return 3;
        }
      if (!b4.q34_batch.used || !c4.q34_batch.used || c4.q34_batch.certificate_backend.empty()) {
        std::printf("cause=floor.batch_unused %s\n", where.c_str());
        return 3;
      }
      runs += 5;
      for (const auto* r : {&e1, &e4, &b4, &c4}) {
        if (r->catalogue_digest != local_digest(r->catalogue_balls)) return fail("catalogue_digest.recompute " + where);
        digest_ms_max = std::max(digest_ms_max, r->times.catalogue_digest_ms);
      }
      if (e1.catalogue_digest != e4.catalogue_digest) return fail("catalogue_digest.workers " + where);
      if (e4.catalogue_digest != b4.catalogue_digest) return fail("catalogue_digest.batch " + where);
      if (e4.catalogue_digest != c4.catalogue_digest) return fail("catalogue_digest.certificates " + where);
      const auto ce = canonical(e4.catalogue_balls);
      if (ce != canonical(e1.catalogue_balls) || ce != canonical(b4.catalogue_balls) || ce != canonical(c4.catalogue_balls))
        return fail("catalogue.differs " + where);
      if (off.catalogue_digest != 0 || off.times.catalogue_digest_ms != 0)
        return fail("catalogue_digest.off_not_null " + where);
      if (off.tower_digest != e4.tower_digest || off.tower_digest != e1.tower_digest)
        return fail("catalogue_digest.option_changes_tower " + where);
      auto reversed = e4.catalogue_balls;
      std::reverse(reversed.begin(), reversed.end());
      if (mhgp9::catalogue_digest(reversed) != e4.catalogue_digest) return fail("catalogue_digest.order " + where);
      balls_seen += e4.catalogue_balls.size();
      extra_shell += e4.catalogue.extra_shell_balls;
      if (std::string_view(family) == "uniform" && kmax == 5) {
        probe_catalogue = e4.catalogue_balls;
        probe_points = fx.points;
        probe_tower_digest = e4.tower_digest;
      }
    }
  }
  if (balls_seen < 6 * n || probe_catalogue.size() < 100) {
    std::printf("cause=floor.balls balls=%llu\n", (unsigned long long)balls_seen);
    return 3;
  }

  // Sensibilite : chaque alteration d'une boule change le condense (local et public).
  const auto ix = rebuild_index(probe_points);
  const auto base = mhgp9::catalogue_digest(probe_catalogue);
  const std::size_t mid = probe_catalogue.size() / 2;
  std::size_t with_interior = mid;
  while (with_interior < probe_catalogue.size() && (probe_catalogue[with_interior].n_interior == 0 ||
                                                    probe_catalogue[with_interior].n_shell >= mhgp9::tower::kBallShellMax))
    ++with_interior;
  if (with_interior == probe_catalogue.size()) {
    std::printf("cause=floor.no_interior_ball\n");
    return 3;
  }
  struct Alteration {
    const char* name;
    std::vector<BallData> cat;
  };
  std::vector<Alteration> alterations;
  {
    auto c = probe_catalogue;
    c.erase(c.begin() + static_cast<std::ptrdiff_t>(mid));
    alterations.push_back({"drop_ball", std::move(c)});
  }
  {
    auto c = probe_catalogue;
    auto& b = c[with_interior];
    std::int32_t other = 0;
    const auto in_ball = [&](std::int32_t s) {
      return std::find(b.interior().begin(), b.interior().end(), s) != b.interior().end() ||
             std::find(b.shell().begin(), b.shell().end(), s) != b.shell().end();
    };
    while (in_ball(other)) ++other;
    b.interior_ids[0] = other;
    alterations.push_back({"interior_id", std::move(c)});
  }
  {
    auto c = probe_catalogue;
    auto& b = c[with_interior];
    b.shell_ids[b.n_shell] = b.interior_ids[b.n_interior - 1];
    ++b.n_shell;
    --b.n_interior;
    alterations.push_back({"interior_to_shell", std::move(c)});
  }
  {
    auto c = probe_catalogue;
    c[mid].level.num[0] += 1;
    alterations.push_back({"level", std::move(c)});
  }
  {
    auto c = probe_catalogue;
    c[mid].arity = static_cast<mhgp9::tower::u8>(c[mid].arity == 2 ? 3 : 2);
    alterations.push_back({"arity", std::move(c)});
  }
  {
    auto c = probe_catalogue;
    c[mid].key.c += 1;
    alterations.push_back({"key", std::move(c)});
  }
  for (const auto& alt : alterations) {
    if (mhgp9::catalogue_digest(alt.cat) == base) return fail(std::string("catalogue_digest.insensitive_") + alt.name);
    if (local_digest(alt.cat) == base) return fail(std::string("local_digest.insensitive_") + alt.name);
  }

  // Motivation : des boules de la zone aveugle retirees laissent la tour FULL inchangee ; le condense du
  // catalogue les voit. Vingt candidats par arite (2 et 3), a pas regulier dans le catalogue (les cles q2
  // precedent les cles q3 : les premiers candidats seraient tous q2).
  const unsigned kmax = 5;
  std::array<std::uint64_t, 4> tried{}, full_unchanged{}, refused{};
  for (const unsigned q : {2u, 3u}) {
    std::vector<std::size_t> zone;
    for (std::size_t i = 0; i < probe_catalogue.size(); ++i) {
      const auto& b = probe_catalogue[i];
      if (b.arity == q && b.n_shell == q && b.n_interior + q == kmax + 1) zone.push_back(i);
    }
    const std::size_t stride = std::max<std::size_t>(1, zone.size() / 20);
    for (std::size_t z = 0; z < zone.size() && tried[q] < 20; z += stride) {
      ++tried[q];
      auto reduced = probe_catalogue;
      reduced.erase(reduced.begin() + static_cast<std::ptrdiff_t>(zone[z]));
      const auto tw = mhgp9::tower::build_full_ball_tower(ix, reduced, kmax, 4, {}, false, false);
      if (tw.status != mhgp9::tower::FullBallStatus::kCompleteRelative) {
        ++refused[q];
        continue;
      }
      if (mhgp9::tower_digest(tw) != probe_tower_digest) continue;
      ++full_unchanged[q];
      if (mhgp9::catalogue_digest(reduced) == base) return fail("catalogue_digest.blind_omission_unseen");
    }
    if (full_unchanged[q] == 0) {
      std::printf("cause=floor.no_redundant_blind_ball q=%u tried=%llu refused=%llu\n", q, (unsigned long long)tried[q],
                  (unsigned long long)refused[q]);
      return 3;
    }
  }
  std::printf("catalogue_digest_gate n=%zu runs=%llu balls=%llu extra_shell=%llu extended_q2=%llu extended_q3=%llu "
              "extended_q4=%llu alterations=%zu blind_q2=%llu/%llu blind_q3=%llu/%llu blind_refused=%llu "
              "digest_ms_max=%.3f\n",
              n, (unsigned long long)runs, (unsigned long long)balls_seen, (unsigned long long)extra_shell,
              (unsigned long long)extended_by_arity[2], (unsigned long long)extended_by_arity[3],
              (unsigned long long)extended_by_arity[4], alterations.size(), (unsigned long long)full_unchanged[2],
              (unsigned long long)tried[2], (unsigned long long)full_unchanged[3], (unsigned long long)tried[3],
              (unsigned long long)(refused[2] + refused[3]), digest_ms_max);
  return 0;
}

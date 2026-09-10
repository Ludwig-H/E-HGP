// Sous-auditeur — pont stdin/stdout vers build_full_ball_tower (overlay WIP 13:04 UTC).
// Aucune geometrie n'est decidee ici : le nuage, le catalogue de boules (cles
// primitives, niveaux exacts, arite = q_min, I/U complets) et les coupes sont
// fournis par le juge rationnel Python. Le pont imprime, par ordre K, coupe et
// cote, les couvertures des racines vivantes et la couverture de leur image
// verticale a K-1 a la meme coupe. Les identifiants de noeuds ne sont pas
// exportes : seules les couvertures sont comparables.
//
// Grammaire stdin (une commande par ligne) :
//   point <id:u32> <x> <y> <z>
//   ball <A> <Bx> <By> <Bz> <C> <levelnum> <levelden> <arity> | <interior ids> | <shell ids>
//   cut <num> <den>
//   kmax <k>
//   run
// Codes de sortie : 0 = tour executee (statut produit dans le JSON, y compris
// un refus produit) ; 2 = entree du pont invalide ; 3 = construction du pont
// impossible (index invalide, bornes BallData, identite inconnue).
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "src/forest/full_ball_tower.hpp"

#ifdef MHGP7_TESTING
#error Le pont doit utiliser le chemin produit non modifie
#endif

namespace {
using namespace mhgp7;

struct BridgeFailure { int code; std::string why; };
[[noreturn]] void fail(int code, const std::string& why) { throw BridgeFailure{code, why}; }

bool parse_i128(const std::string& s, i128& out) {
  if (s.empty()) return false;
  size_t i = 0; bool neg = false;
  if (s[0] == '-') { neg = true; i = 1; } else if (s[0] == '+') { i = 1; }
  if (i >= s.size()) return false;
  u128 mag = 0;
  const u128 limit = (u128{1} << 127);  // |value| <= 2^127 ; -2^127 accepte, +2^127 refuse
  for (; i < s.size(); ++i) {
    if (s[i] < '0' || s[i] > '9') return false;
    const unsigned d = static_cast<unsigned>(s[i] - '0');
    if (mag > (limit - d) / 10) return false;
    mag = mag * 10 + d;
  }
  if (neg) { if (mag > limit) return false; out = mag == limit ? std::numeric_limits<i128>::min() : -static_cast<i128>(mag); }
  else { if (mag >= limit) return false; out = static_cast<i128>(mag); }
  return true;
}

// Entier non negatif decimal -> U192 (trois limbs u64), refus si >= 2^192.
bool parse_u192(const std::string& s, u64 limbs[3]) {
  if (s.empty()) return false;
  limbs[0] = limbs[1] = limbs[2] = 0;
  for (char ch : s) {
    if (ch < '0' || ch > '9') return false;
    u128 carry = static_cast<u128>(ch - '0');
    for (int j = 0; j < 3; ++j) {
      const u128 v = static_cast<u128>(limbs[j]) * 10 + carry;
      limbs[j] = static_cast<u64>(v);
      carry = v >> 64;
    }
    if (carry != 0) return false;
  }
  return true;
}

bool parse_u64(const std::string& s, u64& out) {
  if (s.empty()) return false;
  u64 v = 0;
  for (char ch : s) {
    if (ch < '0' || ch > '9') return false;
    const u64 d = static_cast<u64>(ch - '0');
    if (v > (std::numeric_limits<u64>::max() - d) / 10) return false;
    v = v * 10 + d;
  }
  out = v; return true;
}

std::string status_name(FullBallStatus s) {
  switch (s) {
    case FullBallStatus::kCompleteRelative: return "complete_relative";
    case FullBallStatus::kInvalidInput: return "invalid_input";
    case FullBallStatus::kResourceExhausted: return "resource_exhausted";
    case FullBallStatus::kInvariantViolated: return "invariant_violated";
  }
  return "unknown";
}

std::string json_ids(const std::vector<PointId>& ids) {
  std::string out = "[";
  for (size_t j = 0; j < ids.size(); ++j) {
    if (j) out += ",";
    out += std::to_string(ids[j]);
  }
  return out + "]";
}

struct Cut { std::string num, den; ExactLevel level; };

int run() {
  std::vector<InputPoint> points;
  std::vector<BallData> balls;
  std::vector<Cut> cuts;
  u64 kmax = 0; bool have_kmax = false, ran = false;
  std::map<PointId, i32> geometry;  // rempli apres build_cloud_index
  std::string line;
  size_t line_no = 0;
  // Les boules sont d'abord conservees en identifiants PointId, converties
  // en indices de geometrie seulement apres construction de l'index.
  struct RawBall { BallKey key; ExactLevel level; unsigned arity; std::vector<PointId> interior, shell; };
  std::vector<RawBall> raw;
  while (std::getline(std::cin, line)) {
    ++line_no;
    if (line.empty()) continue;
    std::istringstream in(line);
    std::string cmd; in >> cmd;
    const auto ctx = [&](const char* why) { return std::string(why) + " at line " + std::to_string(line_no); };
    if (cmd == "point") {
      std::string sid, sx, sy, sz; in >> sid >> sx >> sy >> sz;
      u64 id, x, y, z;
      if (!parse_u64(sid, id) || !parse_u64(sx, x) || !parse_u64(sy, y) || !parse_u64(sz, z) ||
          id > std::numeric_limits<PointId>::max() || x > 65535 || y > 65535 || z > 65535)
        fail(2, ctx("bridge_point_syntax"));
      points.push_back({static_cast<PointId>(id), P3{static_cast<i64>(x), static_cast<i64>(y), static_cast<i64>(z)}});
    } else if (cmd == "ball") {
      std::string sa, sb0, sb1, sb2, sc, snum, sden, sarity, bar;
      in >> sa >> sb0 >> sb1 >> sb2 >> sc >> snum >> sden >> sarity >> bar;
      RawBall b{};
      u64 arity, den_dummy = 0; (void)den_dummy;
      if (!parse_i128(sa, b.key.a) || !parse_i128(sb0, b.key.b[0]) || !parse_i128(sb1, b.key.b[1]) ||
          !parse_i128(sb2, b.key.b[2]) || !parse_i128(sc, b.key.c) || !parse_u192(snum, b.level.num) ||
          !parse_i128(sden, b.level.den) || !parse_u64(sarity, arity) || bar != "|")
        fail(2, ctx("bridge_ball_syntax"));
      b.arity = static_cast<unsigned>(arity);
      std::string tok; bool in_shell = false;
      while (in >> tok) {
        if (tok == "|") { if (in_shell) fail(2, ctx("bridge_ball_bars")); in_shell = true; continue; }
        u64 id; if (!parse_u64(tok, id) || id > std::numeric_limits<PointId>::max()) fail(2, ctx("bridge_ball_id"));
        (in_shell ? b.shell : b.interior).push_back(static_cast<PointId>(id));
      }
      if (!in_shell) fail(2, ctx("bridge_ball_missing_shell_bar"));
      raw.push_back(std::move(b));
    } else if (cmd == "cut") {
      std::string snum, sden; in >> snum >> sden;
      Cut c{snum, sden, {}};
      if (!parse_u192(snum, c.level.num) || !parse_i128(sden, c.level.den) || c.level.den <= 0)
        fail(2, ctx("bridge_cut_syntax"));
      cuts.push_back(c);
    } else if (cmd == "kmax") {
      std::string sk; in >> sk;
      if (!parse_u64(sk, kmax)) fail(2, ctx("bridge_kmax_syntax"));
      have_kmax = true;
    } else if (cmd == "run") {
      ran = true; break;
    } else {
      fail(2, ctx("bridge_unknown_command"));
    }
  }
  if (!ran || !have_kmax || points.empty()) fail(2, "bridge_incomplete_input");
  if (kmax > static_cast<u64>(kFacetMaxK)) fail(2, "bridge_kmax_domain");

  const CloudIndex ix = build_cloud_index(points);
  if (!ix.valid) fail(3, "bridge_cloud_index_invalid");
  if (ix.has_duplicate_positions()) fail(3, "bridge_duplicate_positions");
  for (i32 u = 0; u < ix.unique_count(); ++u) geometry.emplace(ix.point_id(u), u);
  for (const auto& b : raw) {
    BallData row{};
    row.key = b.key; row.level = b.level;
    if (b.arity > 255) fail(3, "bridge_arity_domain");
    row.arity = static_cast<u8>(b.arity);
    if (b.interior.size() > kBallInteriorMax || b.shell.size() > kBallShellMax) fail(3, "bridge_ball_bounds");
    for (PointId id : b.interior) {
      const auto found = geometry.find(id);
      if (found == geometry.end()) fail(3, "bridge_unknown_interior_id");
      row.interior_ids[row.n_interior++] = found->second;
    }
    for (PointId id : b.shell) {
      const auto found = geometry.find(id);
      if (found == geometry.end()) fail(3, "bridge_unknown_shell_id");
      row.shell_ids[row.n_shell++] = found->second;
    }
    std::sort(row.interior_ids, row.interior_ids + row.n_interior);
    std::sort(row.shell_ids, row.shell_ids + row.n_shell);
    balls.push_back(row);
  }

  const FullBallTowerResult tower = build_full_ball_tower(ix, balls, static_cast<unsigned>(kmax));
  const auto& st = tower.stats;
  std::printf("{\"type\":\"tower\",\"status\":\"%s\",\"reason\":\"%s\",\"orders\":%zu,\"points\":%zu,\"balls\":%zu,"
      "\"stats\":{\"records\":%llu,\"extra_records\":%llu,\"anchor_blocks\":%llu,\"regular_blocks\":%llu,"
      "\"extra_blocks\":%llu,\"representatives\":%llu,\"anchor_hits\":%llu,\"key_lookups\":%llu,"
      "\"intruder_queries\":%llu,\"same_radius_steps\":%llu,\"descending_steps\":%llu,\"max_chain_steps\":%llu,"
      "\"births\":%llu,\"merges\":%llu,\"contributions\":%llu,\"inert_blocks\":%llu}}\n",
      status_name(tower.status).c_str(), tower.reason, tower.orders.size(), points.size(), balls.size(),
      (unsigned long long)st.records, (unsigned long long)st.extra_records, (unsigned long long)st.anchor_blocks,
      (unsigned long long)st.regular_blocks, (unsigned long long)st.extra_blocks,
      (unsigned long long)st.representatives, (unsigned long long)st.anchor_hits, (unsigned long long)st.key_lookups,
      (unsigned long long)st.intruder_queries, (unsigned long long)st.same_radius_steps,
      (unsigned long long)st.descending_steps, (unsigned long long)st.max_chain_steps,
      (unsigned long long)st.births, (unsigned long long)st.merges, (unsigned long long)st.contributions,
      (unsigned long long)st.inert_blocks);
  if (tower.status != FullBallStatus::kCompleteRelative) return 0;

  for (unsigned k = 1; k <= tower.orders.size(); ++k) {
    const auto& forest = tower.orders[k - 1].forest;
    if (forest.order() != k) fail(3, "bridge_order_identity");
    for (const Cut& cut : cuts) for (int closed = 0; closed < 2; ++closed) {
      struct Root { std::vector<PointId> cover; std::string image; };
      std::vector<Root> roots;
      for (FullNodeId node = 0; node < forest.nodes().size(); ++node) {
        if (full_coverage_root_at(forest, node, cut.level, closed != 0) != node) continue;
        Root r;
        const auto read = full_coverage_at(forest, node, cut.level, closed != 0);
        if (read.status != FullCertificateStatus::kOk) r.cover = {}; else r.cover = read.values;
        if (read.status != FullCertificateStatus::kOk) r.image = "\"cover_read_failed\"";
        const FullNodeId image = full_ball_vertical_root_at(tower, k, node, cut.level, closed != 0);
        if (k == 1) {
          r.image = image == kFullCoverageAbsent ? "null" : "\"unexpected_image_at_K1\"";
        } else if (image == kFullCoverageAbsent) {
          r.image = "\"absent\"";
        } else {
          const auto lower = full_coverage_at(tower.orders[k - 2].forest, image, cut.level, closed != 0);
          r.image = lower.status == FullCertificateStatus::kOk ? json_ids(lower.values) : "\"image_read_failed\"";
        }
        roots.push_back(std::move(r));
      }
      std::sort(roots.begin(), roots.end(), [](const Root& a, const Root& b) {
        return a.cover != b.cover ? a.cover < b.cover : a.image < b.image;
      });
      std::string out = "{\"type\":\"cut\",\"k\":" + std::to_string(k) + ",\"num\":\"" + cut.num + "\",\"den\":\"" +
          cut.den + "\",\"closed\":" + std::to_string(closed) + ",\"roots\":[";
      for (size_t j = 0; j < roots.size(); ++j) {
        if (j) out += ",";
        out += "{\"cover\":" + json_ids(roots[j].cover) + ",\"image\":" + roots[j].image + "}";
      }
      out += "]}\n";
      std::fputs(out.c_str(), stdout);
    }
  }
  return 0;
}
}  // namespace

int main() {
  try {
    return run();
  } catch (const BridgeFailure& f) {
    std::printf("{\"type\":\"bridge_failure\",\"code\":%d,\"reason\":\"%s\"}\n", f.code, f.why.c_str());
    return f.code;
  } catch (const std::exception& e) {
    std::printf("{\"type\":\"bridge_failure\",\"code\":3,\"reason\":\"exception: %s\"}\n", e.what());
    return 3;
  }
}

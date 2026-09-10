// Pont differentiel INDEPENDANT : lit un journal JSON minimal, appelle le
// header PRODUIT (full_coverage_certificate.hpp) sans le modifier, et imprime
// pour chaque noeud et chaque coupe (ouverte/fermee) le statut, la racine et
// la couverture triee. Aucune semantique n'est reimplementee ici : ce pont
// n'est qu'un adaptateur JSON <-> API produit.
//
// Compilation (identique aux flags du developpeur, sans Boost) :
//   g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I<repo>/morsehgp3D_v7 bridge.cpp -o bridge
// Usage : bridge <journal.json>  -> JSON sur stdout, code 0 ; code 2 si le
// journal est irrepresentable (JSON malforme, entier hors representation).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "src/forest/full_coverage_certificate.hpp"

#ifdef MHGP7_TESTING
#error bridge must use the unmodified product path
#endif

namespace {
using namespace mhgp7;

// ---------- JSON minimal (objets, tableaux, chaines, nombres bruts, bool, null)
struct Json;
using JsonArray = std::vector<Json>;
using JsonObject = std::vector<std::pair<std::string, Json>>;
struct Json {
  std::variant<std::nullptr_t, bool, std::string, JsonArray, JsonObject> v;
  bool is_number = false;  // nombres conserves comme texte brut dans v (string)
  const Json* get(std::string_view key) const {
    if (const auto* obj = std::get_if<JsonObject>(&v))
      for (const auto& [k, val] : *obj) if (k == key) return &val;
    return nullptr;
  }
};
struct ParseError { std::string what; };
struct Parser {
  std::string_view s; size_t i = 0;
  void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\t' || s[i] == '\r')) ++i; }
  bool eat(char c) { ws(); if (i < s.size() && s[i] == c) { ++i; return true; } return false; }
  [[noreturn]] void fail(const char* m) { throw ParseError{std::string(m) + " at offset " + std::to_string(i)}; }
  Json parse() {
    ws();
    if (i >= s.size()) fail("unexpected end");
    Json out;
    if (s[i] == '{') {
      ++i; JsonObject obj;
      if (!eat('}')) {
        do {
          ws(); if (i >= s.size() || s[i] != '"') fail("expected key");
          std::string key = string();
          if (!eat(':')) fail("expected colon");
          obj.emplace_back(std::move(key), parse());
        } while (eat(','));
        if (!eat('}')) fail("expected }");
      }
      out.v = std::move(obj);
    } else if (s[i] == '[') {
      ++i; JsonArray arr;
      if (!eat(']')) {
        do arr.push_back(parse()); while (eat(','));
        if (!eat(']')) fail("expected ]");
      }
      out.v = std::move(arr);
    } else if (s[i] == '"') {
      out.v = string();
    } else if (s.compare(i, 4, "true") == 0) { i += 4; out.v = true; }
    else if (s.compare(i, 5, "false") == 0) { i += 5; out.v = false; }
    else if (s.compare(i, 4, "null") == 0) { i += 4; out.v = nullptr; }
    else {
      size_t b = i;
      while (i < s.size() && (s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E' ||
             (s[i] >= '0' && s[i] <= '9'))) ++i;
      if (b == i) fail("unexpected character");
      out.v = std::string(s.substr(b, i - b)); out.is_number = true;
    }
    return out;
  }
  std::string string() {
    if (s[i] != '"') fail("expected string");
    ++i; std::string r;
    while (i < s.size() && s[i] != '"') {
      if (s[i] == '\\') { ++i; if (i >= s.size()) fail("bad escape"); }
      r.push_back(s[i++]);
    }
    if (i >= s.size()) fail("unterminated string");
    ++i; return r;
  }
};

// ---------- conversions entieres (refus explicite hors representation)
struct Unrepresentable { std::string what; };
std::string_view text(const Json& j, const char* what) {
  if (const auto* s = std::get_if<std::string>(&j.v)) return *s;
  throw Unrepresentable{std::string("expected number/string for ") + what};
}
// Decimal -> u64 ; refuse signe, vide, non-chiffre, depassement.
u64 to_u64(const Json& j, const char* what) {
  const auto t = text(j, what);
  if (t.empty()) throw Unrepresentable{std::string("empty integer for ") + what};
  u64 r = 0;
  for (char c : t) {
    if (c < '0' || c > '9') throw Unrepresentable{std::string("non-digit in ") + what + ": " + std::string(t)};
    const u64 d = static_cast<u64>(c - '0');
    if (r > (std::numeric_limits<u64>::max() - d) / 10) throw Unrepresentable{std::string("u64 overflow in ") + what};
    r = r * 10 + d;
  }
  return r;
}
template <typename T> T to_unsigned(const Json& j, const char* what) {
  const u64 r = to_u64(j, what);
  if (r > std::numeric_limits<T>::max()) throw Unrepresentable{std::string("overflow in ") + what};
  return static_cast<T>(r);
}
// Decimal -> U192 (num) ; decimal signe -> i128 (den).
void to_u192(const Json& j, u64 out[3], const char* what) {
  const auto t = text(j, what);
  if (t.empty()) throw Unrepresentable{std::string("empty numerator for ") + what};
  out[0] = out[1] = out[2] = 0;
  for (char c : t) {
    if (c < '0' || c > '9') throw Unrepresentable{std::string("non-digit numerator in ") + what + ": " + std::string(t)};
    u128 carry = static_cast<u128>(c - '0');
    for (int k = 0; k < 3; ++k) {
      const u128 acc = static_cast<u128>(out[k]) * 10 + carry;
      out[k] = static_cast<u64>(acc); carry = acc >> 64;
    }
    if (carry) throw Unrepresentable{std::string("U192 overflow in ") + what};
  }
}
i128 to_i128(const Json& j, const char* what) {
  auto t = text(j, what);
  bool neg = false;
  if (!t.empty() && t[0] == '-') { neg = true; t.remove_prefix(1); }
  if (t.empty()) throw Unrepresentable{std::string("empty denominator for ") + what};
  u128 r = 0;
  const u128 limit = static_cast<u128>(std::numeric_limits<i128>::max());
  for (char c : t) {
    if (c < '0' || c > '9') throw Unrepresentable{std::string("non-digit denominator in ") + what};
    const u128 d = static_cast<u128>(c - '0');
    if (r > (limit - d) / 10) throw Unrepresentable{std::string("i128 overflow in ") + what};
    r = r * 10 + d;
  }
  return neg ? -static_cast<i128>(r) : static_cast<i128>(r);
}
ExactLevel to_level(const Json& j, const char* what) {
  const auto* arr = std::get_if<JsonArray>(&j.v);
  if (!arr || arr->size() != 2) throw Unrepresentable{std::string("level must be [num, den] for ") + what};
  ExactLevel l{{0, 0, 0}, 1};
  to_u192((*arr)[0], l.num, what);
  l.den = to_i128((*arr)[1], what);
  return l;
}
std::string u192_text(const u64 w[3]) {
  u64 limbs[3] = {w[0], w[1], w[2]};
  std::string digits;
  while (limbs[0] || limbs[1] || limbs[2]) {
    u128 rem = 0;
    for (int k = 2; k >= 0; --k) {
      const u128 cur = (rem << 64) | limbs[k];
      limbs[k] = static_cast<u64>(cur / 10); rem = cur % 10;
    }
    digits.push_back(static_cast<char>('0' + static_cast<int>(rem)));
  }
  if (digits.empty()) digits = "0";
  return std::string(digits.rbegin(), digits.rend());
}
std::string i128_text(i128 v) {
  const bool neg = v < 0;
  u128 m = neg ? static_cast<u128>(-(v + 1)) + 1 : static_cast<u128>(v);
  std::string digits;
  while (m) { digits.push_back(static_cast<char>('0' + static_cast<int>(m % 10))); m /= 10; }
  if (digits.empty()) digits = "0";
  if (neg) digits.push_back('-');
  return std::string(digits.rbegin(), digits.rend());
}
std::string level_text(const ExactLevel& l) {
  return "[\"" + u192_text(l.num) + "\",\"" + i128_text(l.den) + "\"]";
}
const char* status_text(FullCertificateStatus s) {
  switch (s) {
    case FullCertificateStatus::kOk: return "ok";
    case FullCertificateStatus::kInvalidInput: return "invalid_input";
    case FullCertificateStatus::kResourceExhausted: return "resource_exhausted";
  }
  return "unknown";
}
const JsonArray& array(const Json* j, const char* what) {
  if (!j) throw Unrepresentable{std::string("missing ") + what};
  if (const auto* a = std::get_if<JsonArray>(&j->v)) return *a;
  throw Unrepresentable{std::string("expected array for ") + what};
}
bool boolean(const Json* j, const char* what) {
  if (!j) throw Unrepresentable{std::string("missing ") + what};
  if (const auto* b = std::get_if<bool>(&j->v)) return *b;
  throw Unrepresentable{std::string("expected bool for ") + what};
}
std::string json_escape(std::string_view s) {
  std::string r;
  for (char c : s) { if (c == '"' || c == '\\') r.push_back('\\'); r.push_back(c); }
  return r;
}

template <typename T> std::string list_text(const std::vector<T>& v) {
  std::string r = "[";
  for (size_t i = 0; i < v.size(); ++i) { if (i) r += ","; r += std::to_string(v[i]); }
  return r + "]";
}
std::string id_text(FullNodeId id) {
  return id == kFullCoverageAbsent ? std::string("null") : std::to_string(id);
}

// ---------- une coupe : chaque noeud existant + deux sondes hors domaine
void emit_cut(std::string& out, const FullCoverageCertificate& forest, const ExactLevel& cut,
              bool closed, const char* origin) {
  out += "{\"level\":" + level_text(cut) + ",\"closed\":" + (closed ? "true" : "false") +
         ",\"origin\":\"" + origin + "\",\"nodes\":[";
  const size_t n = forest.nodes().size();
  std::vector<FullNodeId> probes;
  for (size_t id = 0; id < n; ++id) probes.push_back(static_cast<FullNodeId>(id));
  probes.push_back(static_cast<FullNodeId>(n));  // un cran au-dela
  probes.push_back(kFullCoverageAbsent);
  for (size_t k = 0; k < probes.size(); ++k) {
    const FullNodeId id = probes[k];
    const FullNodeId root = full_coverage_root_at(forest, id, cut, closed);
    const auto read = full_coverage_at(forest, id, cut, closed);
    if (k) out += ",";
    out += "{\"id\":" + id_text(id) + ",\"root\":" + id_text(root) +
           ",\"live\":" + (root != kFullCoverageAbsent && root == id ? "true" : "false") +
           ",\"read\":{\"status\":\"" + status_text(read.status) + "\",\"reason\":\"" +
           json_escape(read.reason) + "\",\"values\":" + list_text(read.values) + "}}";
  }
  out += "]}";
}

int run(const std::string& path) {
  std::ifstream in(path);
  if (!in) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); return 2; }
  std::stringstream buffer; buffer << in.rdbuf();
  const std::string content = buffer.str();
  Parser parser{content};
  Json doc = parser.parse();
  parser.ws();
  if (parser.i != content.size()) throw ParseError{"trailing characters"};

  std::string name;
  if (const auto* nm = doc.get("name"))
    if (const auto* s = std::get_if<std::string>(&nm->v)) name = *s;
  const unsigned order = to_unsigned<unsigned>(*[&] {
    const auto* o = doc.get("order"); if (!o) throw Unrepresentable{"missing order"}; return o; }(), "order");

  std::vector<PointId> domain;
  for (const auto& p : array(doc.get("domain"), "domain")) domain.push_back(to_unsigned<PointId>(p, "domain point"));
  std::vector<FullCoveragePopulation> rows;
  for (const auto& r : array(doc.get("populations"), "populations")) {
    FullCoveragePopulation row;
    for (const auto& p : array(r.get("interior"), "interior")) row.interior.push_back(to_unsigned<PointId>(p, "interior point"));
    for (const auto& p : array(r.get("shell"), "shell")) row.shell.push_back(to_unsigned<PointId>(p, "shell point"));
    rows.push_back(std::move(row));
  }
  std::vector<FullCoverageBatch> batches;
  for (const auto& b : array(doc.get("batches"), "batches")) {
    FullCoverageBatch batch;
    const auto* lv = b.get("level"); if (!lv) throw Unrepresentable{"missing batch level"};
    batch.level = to_level(*lv, "batch level");
    for (const auto& a : array(b.get("actions"), "actions")) {
      FullCoverageAction action;
      for (const auto& p : array(a.get("parents"), "parents")) action.parents.push_back(to_u64(p, "parent"));
      for (const auto& c : array(a.get("contributions"), "contributions")) {
        FullCoverageRef ref;
        const auto* pop = c.get("population"); if (!pop) throw Unrepresentable{"missing population"};
        ref.population = to_u64(*pop, "population");
        const auto* mask = c.get("shell_mask"); if (!mask) throw Unrepresentable{"missing shell_mask"};
        ref.shell_mask = to_unsigned<u16>(*mask, "shell_mask");
        ref.include_interior = boolean(c.get("include_interior"), "include_interior");
        action.contributions.push_back(ref);
      }
      batch.actions.push_back(std::move(action));
    }
    batches.push_back(std::move(batch));
  }
  std::vector<ExactLevel> extra_cuts;
  if (const auto* cuts = doc.get("cuts"))
    for (const auto& c : array(cuts, "cuts")) extra_cuts.push_back(to_level(c, "cut"));

  std::string out = "{\"name\":\"" + json_escape(name) + "\",\"order\":" + std::to_string(order);
  auto bank = build_full_coverage_populations(domain, rows);
  out += ",\"bank\":{\"status\":\"" + std::string(status_text(bank.status)) + "\",\"reason\":\"" +
         json_escape(bank.reason) + "\",\"value\":" + (bank.value ? "true" : "false") + "}";
  // Le certificat est demande meme si la banque a refuse : la banque nulle
  // doit produire coverage_invalid_domain, jamais un acces a un pointeur nul.
  auto built = build_full_coverage_certificate(order, bank.value, batches);
  const auto& forest = built.value;
  const bool empty_forest = forest.order() == 0 && !forest.populations() && forest.nodes().empty() &&
      forest.parents().empty() && forest.successors().empty() && forest.contributions().empty();
  out += ",\"build\":{\"status\":\"" + std::string(status_text(built.status)) + "\",\"reason\":\"" +
         json_escape(built.reason) + "\",\"order\":" + std::to_string(forest.order()) +
         ",\"empty\":" + (empty_forest ? "true" : "false") + "}";
  if (built.status == FullCertificateStatus::kOk) {
    out += ",\"nodes\":[";
    for (size_t id = 0; id < forest.nodes().size(); ++id) {
      const auto& node = forest.nodes()[id];
      std::vector<FullNodeId> parents;
      for (u64 j = 0; j < node.parent_count; ++j) parents.push_back(forest.parents()[static_cast<size_t>(node.first + j)]);
      if (id) out += ",";
      out += "{\"id\":" + std::to_string(id) + ",\"level\":" + level_text(node.level) +
             ",\"parents\":" + list_text(parents) + ",\"successor\":" + id_text(forest.successors()[id]) + "}";
    }
    out += "],\"contributions\":[";
    for (size_t i = 0; i < forest.contributions().size(); ++i) {
      const auto& c = forest.contributions()[i];
      if (i) out += ",";
      out += "{\"level\":" + level_text(c.level) + ",\"segment\":" + std::to_string(c.segment) +
             ",\"population\":" + std::to_string(c.ref.population) + ",\"shell_mask\":" +
             std::to_string(c.ref.shell_mask) + ",\"include_interior\":" + (c.ref.include_interior ? "true" : "false") + "}";
    }
    out += "],\"cuts\":[";
    bool first = true;
    bool batch_cuts = true;
    if (const auto* flag = doc.get("batch_cuts")) batch_cuts = boolean(flag, "batch_cuts");
    for (const auto& batch : batches)
      for (bool closed : {false, true}) {
        if (!batch_cuts) break;
        if (!first) out += ",";
        first = false;
        emit_cut(out, forest, batch.level, closed, "batch");
      }
    for (const auto& cut : extra_cuts)
      for (bool closed : {false, true}) {
        if (!first) out += ",";
        first = false;
        emit_cut(out, forest, cut, closed, "extra");
      }
    out += "]";
  }
  out += "}\n";
  std::fputs(out.c_str(), stdout);
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) { std::fprintf(stderr, "usage: bridge <journal.json>\n"); return 2; }
  try {
    return run(argv[1]);
  } catch (const ParseError& e) {
    std::printf("{\"bridge_error\":\"json_parse\",\"detail\":\"%s\"}\n", json_escape(e.what).c_str());
    return 2;
  } catch (const Unrepresentable& e) {
    std::printf("{\"bridge_error\":\"unrepresentable\",\"detail\":\"%s\"}\n", json_escape(e.what).c_str());
    return 2;
  } catch (const std::exception& e) {
    std::printf("{\"bridge_error\":\"exception\",\"detail\":\"%s\"}\n", json_escape(e.what()).c_str());
    return 3;
  }
}

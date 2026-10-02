// Registre de compteurs et de durees par etage (ledger.hpp).
#include "core/ledger.hpp"

#include <limits>

namespace mhgp11 {

namespace {

// Somme saturee : a + b si elle tient dans u64, sinon 2^64 - 1. Le test a > max - b ne deborde pas (b <= max).
u64 saturating_add(u64 a, u64 b) noexcept {
  const u64 max = std::numeric_limits<u64>::max();
  return a > max - b ? max : a + b;
}

void add_to(LedgerTable& table, std::string_view key, u64 delta) {
  auto it = table.find(key);
  if (it == table.end()) it = table.emplace(std::string(key), u64{0}).first;
  it->second = saturating_add(it->second, delta);
}

u64 value_of(const LedgerTable& table, std::string_view key) {
  const auto it = table.find(key);
  return it == table.end() ? 0 : it->second;
}

// Chaine JSON (RFC 8259) : guillemet, barre oblique inversee et caracteres de controle echappes.
void append_json_string(std::string& out, std::string_view text) {
  static constexpr char kHex[] = "0123456789abcdef";
  out.push_back('"');
  for (const char c : text) {
    const unsigned char byte = static_cast<unsigned char>(c);
    if (c == '"' || c == '\\') {
      out.push_back('\\');
      out.push_back(c);
    } else if (byte < 0x20) {
      out += "\\u00";
      out.push_back(kHex[byte >> 4]);
      out.push_back(kHex[byte & 0x0F]);
    } else {
      out.push_back(c);
    }
  }
  out.push_back('"');
}

void append_json_table(std::string& out, std::string_view title, const LedgerTable& table) {
  append_json_string(out, title);
  out += ":{";
  bool first = true;
  for (const auto& [key, value] : table) {
    if (!first) out.push_back(',');
    first = false;
    append_json_string(out, key);
    out.push_back(':');
    out += std::to_string(value);
  }
  out.push_back('}');
}

}  // namespace

void Ledger::count(std::string_view name, u64 delta) { add_to(counters_, name, delta); }

void Ledger::time(std::string_view stage, u64 nanoseconds) { add_to(times_, stage, nanoseconds); }

void Ledger::merge(const Ledger& other) {
  for (const auto& [key, value] : other.counters_) add_to(counters_, key, value);
  for (const auto& [key, value] : other.times_) add_to(times_, key, value);
}

u64 Ledger::counter(std::string_view name) const { return value_of(counters_, name); }

u64 Ledger::nanoseconds(std::string_view stage) const { return value_of(times_, stage); }

std::string Ledger::to_json() const {
  std::string out = "{";
  append_json_table(out, "counters", counters_);
  out.push_back(',');
  append_json_table(out, "nanoseconds", times_);
  out.push_back('}');
  return out;
}

StageTimer::StageTimer(Ledger& ledger, std::string_view stage)
    : ledger_(ledger), stage_(stage), start_(std::chrono::steady_clock::now()) {
  ledger_.time(stage_, 0);
}

StageTimer::~StageTimer() {
  const auto elapsed = std::chrono::steady_clock::now() - start_;
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
  // L'etage existe depuis la construction : cet ajout ne cree aucune cle. Une duree negative (horloge monotone :
  // impossible) compterait pour 0.
  ledger_.time(stage_, ns > 0 ? static_cast<u64>(ns) : u64{0});
}

}  // namespace mhgp11

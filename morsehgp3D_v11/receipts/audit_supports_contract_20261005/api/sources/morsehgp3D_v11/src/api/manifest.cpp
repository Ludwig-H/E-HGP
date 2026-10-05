// Manifeste deterministe et publication (api.hpp ; paragraphes 6.6 et 6.7 de la specification de la sortie
// parametree). Ecrit a neuf ; le JSON vit ici et non dans io, tant qu'un seul document JSON existe (compte rendu S4).
//
// Le manifeste est un objet JSON a cles en ordre FIXE, sans espace, termine par un saut de ligne : entiers decimaux
// (std::to_chars, independant de la locale), litteraux ASCII, empreintes hexadecimales minuscules et decimaux controles
// (valid_grid_step, valid_origin_coordinate) ; aucun caractere n'y demande d'echappement, et json.dumps de Python avec
// separators=(',', ':') le reecrit a l'identique (lecteur bench/mhgp11_formats.py). Il ne contient ni temps, ni nombre
// de fils, ni chemin : il est identique quel que soit le nombre de fils et le dossier de sortie.
// Portes : mhgp11_api_session_tree_digest, mhgp11_cli_full_determinism, mhgp11_cli_full_relabel,
// mhgp11_cli_full_identity ; mutants tree_k_sans_enfants, manifeste_taille_fausse.
#include <charconv>
#include <span>

#include "api/internal.hpp"

namespace mhgp11 {

namespace {

using api::Provenance;

// u32 et u64 petit-boutistes vers une empreinte, par paquets sur la pile.
class LittleEndianDigest {
 public:
  explicit LittleEndianDigest(io::Sha256& sha) noexcept : sha_(sha) {}
  void u32le(u32 value) noexcept {
    if (count_ + 4 > buffer_.size()) flush();
    for (int b = 0; b < 4; ++b) buffer_[count_++] = static_cast<u8>(value >> (8 * b));
  }
  void u64le(u64 value) noexcept {
    if (count_ + 8 > buffer_.size()) flush();
    for (int b = 0; b < 8; ++b) buffer_[count_++] = static_cast<u8>(value >> (8 * b));
  }
  void flush() noexcept {
    sha_.update(std::span<const u8>(buffer_.data(), count_));
    count_ = 0;
  }

 private:
  io::Sha256& sha_;
  std::array<u8, 4096> buffer_{};
  std::size_t count_ = 0;
};

bool digits(std::string_view text) noexcept {
  if (text.empty()) return false;
  for (const char c : text)
    if (c < '0' || c > '9') return false;
  return true;
}

// Decimal sans signe : chiffres, puis au plus un point suivi d'au moins un chiffre.
bool unsigned_decimal(std::string_view text) noexcept {
  if (text.empty() || text.size() > api::kMaxDecimal) return false;
  const std::size_t point = text.find('.');
  if (point == std::string_view::npos) return digits(text);
  return digits(text.substr(0, point)) && digits(text.substr(point + 1));
}

void number(std::string& out, u64 value) {
  std::array<char, 20> buffer{};
  const auto done = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  out.append(buffer.data(), static_cast<std::size_t>(done.ptr - buffer.data()));
}

void quoted(std::string& out, std::string_view text) {
  out.push_back('"');
  out.append(text);
  out.push_back('"');
}

void hex(std::string& out, const io::Digest& digest) {
  const auto text = io::to_hex(digest);
  quoted(out, std::string_view(text.data(), text.size()));
}

void parameters(std::string& out, const Provenance& provenance) {
  out.append(",\"parameters\":{\"budget_bytes\":");
  if (provenance.budget_bytes) number(out, *provenance.budget_bytes);
  else out.append("null");
  out.append(",\"grid_step\":");
  if (provenance.grid_step.empty()) out.append("null");
  else quoted(out, provenance.grid_step);
  out.append(",\"origin\":");
  if (provenance.origin[0].empty()) {
    out.append("null");
  } else {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      out.push_back(axis == 0 ? '[' : ',');
      quoted(out, provenance.origin[axis]);
    }
    out.push_back(']');
  }
  out.push_back('}');
}

void inputs(std::string& out, const Provenance& provenance) {
  out.append(",\"inputs\":[{\"name\":\"points\",\"bytes\":");
  number(out, provenance.points_bytes);
  out.append(",\"sha256\":");
  hex(out, provenance.points_sha256);
  out.append("},{\"name\":\"ids\",\"bytes\":");
  number(out, provenance.ids_bytes);
  out.append(",\"sha256\":");
  hex(out, provenance.ids_sha256);
  out.append("}]");
}

// Comptes de la tour : sites et points, puis par ordre naissances, noeuds, aretes et racine (en-tetes de MHGP11FUL1).
void full_counts(std::string& out, const FullTower& tower) {
  const auto& cloud = tower.domain().index().cloud();
  out.append(",\"counts\":{\"sites\":");
  number(out, cloud.sites());
  out.append(",\"points\":");
  number(out, cloud.weight());
  out.append(",\"orders\":[");
  for (u32 k = 1; k <= tower.kmax(); ++k) {
    const OrderForest& forest = tower.order(static_cast<Order>(k));
    out.append(k == 1 ? "{\"k\":" : ",{\"k\":");
    number(out, k);
    out.append(",\"births\":");
    number(out, forest.births());
    out.append(",\"nodes\":");
    number(out, forest.nodes().size());
    out.append(",\"edges\":");
    number(out, forest.edges().size());
    out.append(",\"root\":");
    number(out, idx(forest.root()));
    out.push_back('}');
  }
  out.append("]}");
}

}  // namespace

namespace api {

bool valid_grid_step(std::string_view text) noexcept {
  return unsigned_decimal(text) && text.find_first_not_of("0.") != std::string_view::npos;
}

bool valid_origin_coordinate(std::string_view text) noexcept {
  if (text.size() > kMaxDecimal) return false;
  if (!text.empty() && text.front() == '-') text.remove_prefix(1);
  return unsigned_decimal(text);
}

io::Digest tree_k_sha256(const OrderForest& forest) noexcept {
  io::Sha256 sha;
  sha.update(std::string_view("MHGP11TK"));
  LittleEndianDigest out(sha);
  out.u64le(forest.order());
  out.u64le(forest.nodes().size());
  for (const ForestNode& node : forest.nodes()) {
    out.u32le(idx(node.parent));
    out.u32le(idx(node.rank));
    out.u32le(node.birth_key);
    out.u32le(node.child_count);
  }
  for (const NodeIdx child : forest.edges()) out.u32le(idx(child));
  out.flush();
  return sha.finish();
}

Result<io::Digest> publish(Session& session, const Product& product, io::OutputDirectory& directory,
                           const Provenance& provenance, RunReport* report) noexcept {
  MHGP11_TRY(api_detail::check_provenance(provenance));
  MemoryBudget& budget = session.budget();
  RunReport local;
  return guarded([&]() -> Result<io::Digest> {
    // Sortie full : le produit est la tour elle-meme, l'etage output est vide ; write couvre le fichier, l'empreinte
    // de l'arbre, le manifeste et la publication (synchronisations et renommage compris).
    local.at(Stage::output) = {0, budget.restart_peak()};
    Stopwatch write_clock;
    Result<io::FileWriter*> file = directory.create(kFullFileName);
    if (!file.ok()) return file.outcome();
    MHGP11_TRY(api_detail::write_full(*file.value(), product.full()));
    const std::string manifest =
        api_detail::full_manifest(product, provenance, file.value()->size(), file.value()->digest());
    MHGP11_TRY(directory.commit(manifest));
    local.at(Stage::write) = {write_clock.nanoseconds(), budget.restart_peak()};
    if (report != nullptr) {
      report->at(Stage::output) = local.at(Stage::output);
      report->at(Stage::write) = local.at(Stage::write);
    }
    return directory.manifest_sha256();
  });
}

}  // namespace api

namespace api_detail {

Outcome check_provenance(const Provenance& provenance) noexcept {
  if (!provenance.grid_step.empty() && !api::valid_grid_step(provenance.grid_step))
    return fail(Reason::parameter_out_of_range);
  const bool declared = !provenance.origin[0].empty();
  for (const std::string_view axis : provenance.origin)
    if (declared ? !api::valid_origin_coordinate(axis) : !axis.empty()) return fail(Reason::parameter_out_of_range);
  return {};
}

std::string full_manifest(const api::Product& product, const Provenance& provenance, u64 file_bytes,
                          const io::Digest& file_sha256) {
  const FullTower& tower = product.full();
  std::string out;
  out.reserve(2048);
  out.append("{\"schema\":");
  quoted(out, api::kManifestSchema);
  out.append(",\"output\":");
  quoted(out, api::output_name(product.kind()));
  out.append(",\"status\":\"complete\",\"public_status\":\"not_claimed\",\"coord_bits\":");
  number(out, static_cast<u64>(kCoordBits));
  out.append(",\"k\":");
  number(out, product.k());
  parameters(out, provenance);
  inputs(out, provenance);
  out.append(",\"files\":[{\"name\":");
  quoted(out, api::kFullFileName);
  out.append(",\"format\":\"MHGP11FUL1\",\"version\":1,\"bytes\":");
  number(out, file_bytes);
  out.append(",\"sha256\":");
  hex(out, file_sha256);
  out.append("}],\"tree_k_sha256\":");
  hex(out, api::tree_k_sha256(tower.order(product.k())));
  full_counts(out, tower);
  out.append("}\n");
  return out;
}

}  // namespace api_detail

}  // namespace mhgp11

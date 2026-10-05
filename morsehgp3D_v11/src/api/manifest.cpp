// Manifeste deterministe, signature tree_k_sha256 et publication (api.hpp ; docs/SORTIES.md, paragraphes 3, 8 et 9).
// Ecrit a neuf ; le JSON vit ici et non dans io, tant qu'un seul document JSON existe (compte rendu S4).
//
// Le manifeste est un objet JSON a cles en ordre FIXE, sans espace, termine par un saut de ligne : entiers decimaux
// (std::to_chars, independant de la locale), litteraux ASCII, empreintes hexadecimales minuscules et decimaux controles
// (valid_grid_step, valid_origin_coordinate) ; aucun caractere n'y demande d'echappement, et json.dumps de Python avec
// separators=(',', ':') le reecrit a l'identique (lecteur bench/mhgp11_formats.py). Il ne contient ni temps, ni nombre
// de fils, ni chemin : il est identique quel que soit le nombre de fils et le dossier de sortie.
// tree_k_sha256 est la signature version 2 (reponse D.2 de l'auditeur, aef7182b3) : geometrie des sites, puis par
// noeud parent, rang, genre et naissance en SiteIdx (le site a K = 1, S* du catalogue sinon), enfants ; ni PointId ni
// BallIdx. La version 1 (BallIdx des naissances) n'est jamais publiee.
// Portes : mhgp11_api_session_tree_digest (signature recalculee a la main, valeurs gravees de l'auditeur, champ publie
// du manifeste), mhgp11_api_session_after_publish (etat publie, retrait), mhgp11_cli_full_determinism,
// mhgp11_cli_full_relabel, mhgp11_cli_full_identity, mhgp11_cli_contract ; mutants de tests/mutants/api.json
// (tree_k_*, retrait_omis_fin_de_session) et de tests/mutants/cli.json (manifeste_taille_fausse,
// retrait_omis_double_echec).
#include <algorithm>
#include <charconv>
#include <span>

#include "api/internal.hpp"

namespace mhgp11 {

namespace {

using api::Provenance;

// Octets, u32 et u64 petit-boutistes vers une empreinte, par paquets sur la pile.
class LittleEndianDigest {
 public:
  explicit LittleEndianDigest(io::Sha256& sha) noexcept : sha_(sha) {}
  void u8le(u8 value) noexcept {
    if (count_ + 1 > buffer_.size()) flush();
    buffer_[count_++] = value;
  }
  void u32le(u32 value) noexcept {
    if (count_ + 4 > buffer_.size()) flush();
    for (int b = 0; b < 4; ++b) buffer_[count_++] = static_cast<u8>(value >> (8 * b));
  }
  void u64le(u64 value) noexcept {
    if (count_ + 8 > buffer_.size()) flush();
    for (int b = 0; b < 8; ++b) buffer_[count_++] = static_cast<u8>(value >> (8 * b));
  }
  void digest(const io::Digest& value) noexcept {
    for (const u8 byte : value) u8le(byte);
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

// Parametres de l'appel ; pour plat, s'y ajoutent mcs, z et selection (docs/SORTIES.md, paragraphe 8).
void parameters(std::string& out, const Provenance& provenance, const api::Product& product) {
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
  if (const api::FlatRequest* flat = std::get_if<api::FlatRequest>(&product.request())) {
    out.append(",\"mcs\":");
    number(out, flat->mcs);
    out.append(",\"z\":");
    number(out, flat->z);
    out.append(flat->selection == head::Selection::eom ? ",\"selection\":\"eom\"" : ",\"selection\":\"feuilles\"");
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

// Genre d'un noeud dans la signature (colonne kind de MHGP11SP) : feuille de site a K = 1, naissance de boule, fusion.
enum class NodeKind : u8 { site_leaf = 0, ball_birth = 1, merge = 2 };

// SHA-256 de la geometrie : "MHGP11GX", u64 coord_bits, u64 n, puis u32 x, y, z de chaque site, ordre des SiteIdx.
io::Digest geometry_sha256(const Cloud& cloud) noexcept {
  io::Sha256 sha;
  sha.update(std::string_view("MHGP11GX"));
  LittleEndianDigest out(sha);
  out.u64le(static_cast<u64>(kCoordBits));
  out.u64le(cloud.sites());
  for (u32 s = 0; s < cloud.sites(); ++s) {
    out.u32le(cloud.x()[s]);
    out.u32le(cloud.y()[s]);
    out.u32le(cloud.z()[s]);
  }
  out.flush();
  return sha.finish();
}

// Etat de `directory` apres une issue : D publie (committed) ou non, et l'empreinte du manifeste de D publie.
api::Publication state_of(const Outcome& outcome, const io::OutputDirectory& directory) noexcept {
  if (!directory.committed()) return {outcome, api::PublicationState::none, {}};
  return {outcome, api::PublicationState::published_complete, directory.manifest_sha256()};
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

io::Digest tree_k_sha256(const FullDomain& domain, const OrderForest& forest) noexcept {
  const Cloud& cloud = domain.index().cloud();
  const io::Digest geometry = geometry_sha256(cloud);
  io::Sha256 sha;
  sha.update(std::string_view("MHGP11TK"));
  LittleEndianDigest out(sha);
  out.u64le(kTreeSignatureVersion);
  out.u64le(static_cast<u64>(kCoordBits));
  out.u64le(forest.order());
  out.u64le(cloud.sites());
  out.u64le(forest.nodes().size());
  out.digest(geometry);
  const auto balls = domain.catalogue().balls_data();
  for (u32 i = 0; i < forest.nodes().size(); ++i) {
    const ForestNode& node = forest.nodes()[i];
    out.u32le(idx(node.parent));
    out.u32le(idx(node.rank));
    if (node.birth_key == kNone) {  // fusion : aucune naissance
      out.u8le(static_cast<u8>(NodeKind::merge));
      out.u8le(0);
    } else if (forest.order() == 1) {  // feuille : la cle de naissance est le SiteIdx
      out.u8le(static_cast<u8>(NodeKind::site_leaf));
      out.u8le(1);
      out.u32le(node.birth_key);
    } else {  // naissance de boule : S*, support minimal canonique, croissant
      const CatalogueBall& ball = balls[node.birth_key];
      out.u8le(static_cast<u8>(NodeKind::ball_birth));
      out.u8le(ball.qmin);
      for (u32 j = 0; j < ball.qmin; ++j) out.u32le(idx(ball.support[j]));
    }
    const std::span<const NodeIdx> children = forest.children(NodeIdx{i});  // croissants (numerotation canonique)
    out.u32le(static_cast<u32>(children.size()));
    for (const NodeIdx child : children) out.u32le(idx(child));
  }
  out.flush();
  return sha.finish();
}

Publication publish(Session& session, const Product& product, io::OutputDirectory& directory,
                    const Provenance& provenance, RunReport* report) noexcept {
  // Produit d'une autre Session (audit general a65903a7b, P1) : ses tampons sont comptes dans un autre budget ; refus
  // avant toute creation de fichier et toute ecriture du rapport. Porte mhgp11_api_session_session_identity.
  if (!product.computed_by(session)) return {fail(Reason::parameter_out_of_range), PublicationState::none, {}};
  const Outcome checked = api_detail::check_provenance(provenance, product.domain().index().cloud().weight());
  if (!checked.ok()) return {checked, PublicationState::none, {}};
  MemoryBudget& budget = session.budget();
  RunReport local;
  const bool supports = product.kind() == OutputKind::supports, points = product.kind() == OutputKind::points;
  const bool flat = product.kind() == OutputKind::flat;
  const Outcome written = guarded([&]() -> Outcome {
    // Sortie full : le produit est la tour elle-meme, l'etage output est vide. Sorties supports, points et plat :
    // l'etage output est l'assemblage, la pendaison, ou la pendaison et la tete plate, mesure par compute. write couvre le fichier, l'empreinte de l'arbre, le
    // manifeste et la publication (synchronisations et renommage compris).
    if (!supports && !points && !flat) local.at(Stage::output) = {0, budget.restart_peak()};
    else budget.restart_peak();
    Stopwatch write_clock;
    Result<io::FileWriter*> file =
        directory.create(supports ? kSupportsFileName
                                  : (points ? kPointsFileName : (flat ? kFlatFileName : kFullFileName)));
    if (!file.ok()) return file.outcome();
    std::string manifest;
    if (supports) {
      MHGP11_TRY(api_detail::write_supports(*file.value(), product.order_tree(), product.hierarchy()));
      manifest = api_detail::supports_manifest(product, provenance, file.value()->size(), file.value()->digest());
    } else if (points) {
      MHGP11_TRY(api_detail::write_points(*file.value(), product.order_tree(), product.points()));
      manifest = api_detail::points_manifest(product, provenance, file.value()->size(), file.value()->digest());
    } else if (flat) {
      MHGP11_TRY(api_detail::write_flat(*file.value(), std::get<FlatRequest>(product.request()), product.flat()));
      manifest = api_detail::flat_manifest(product, provenance, file.value()->size(), file.value()->digest());
    } else {
      MHGP11_TRY(api_detail::write_full(*file.value(), product.full()));
      manifest = api_detail::full_manifest(product, provenance, file.value()->size(), file.value()->digest());
    }
    MHGP11_TRY(directory.commit(manifest));
    local.at(Stage::write) = {write_clock.nanoseconds(), budget.restart_peak()};
    return {};
  });
  // Un refus ne laisse D publie qu'apres un commit en double echec (synchronisation du parent puis retour en echec) :
  // withdraw le retire, ou declare published_complete.
  if (!written.ok()) return withdraw(written, directory);
  if (report != nullptr) {
    if (!supports && !points && !flat) report->at(Stage::output) = local.at(Stage::output);
    report->at(Stage::write) = local.at(Stage::write);
  }
  return state_of(written, directory);
}

Publication withdraw(const Outcome& refusal, io::OutputDirectory& directory) noexcept {
  // Le refus du retrait (renommage impossible, D.pending apparu entre-temps) ne change pas la raison de l'appel : il
  // laisse D publie et complet, committed() reste vrai, et l'etat rendu le dit.
  if (directory.committed()) static_cast<void>(directory.retract());
  return state_of(refusal, directory);
}

Publication finish(Session& session, io::OutputDirectory& directory) noexcept {
  const Outcome closed = session.close();
  if (!closed.ok()) return withdraw(closed, directory);
  return state_of(closed, directory);
}

std::string_view publication_state_name(PublicationState state) noexcept {
  switch (state) {
    case PublicationState::none: return "none";
    case PublicationState::published_complete: return "published_complete";
  }
  return "unknown";
}

}  // namespace api

namespace api_detail {

Outcome check_provenance(const Provenance& provenance, u64 points) noexcept {
  // Tailles des deux entrees du nuage du produit (12 et 4 octets par point) et budget declare strictement positif :
  // le lecteur officiel (bench/mhgp11_formats.py) refuse tout autre manifeste (audit abc30ed06). Porte
  // mhgp11_api_publish_reader (aller-retour vers le lecteur) et groupe provenance de mhgp11_api_session.
  if (provenance.points_bytes != 12 * points || provenance.ids_bytes != 4 * points)
    return fail(Reason::parameter_out_of_range);
  if (provenance.budget_bytes && *provenance.budget_bytes == 0) return fail(Reason::parameter_out_of_range);
  if (!provenance.grid_step.empty() && !api::valid_grid_step(provenance.grid_step))
    return fail(Reason::parameter_out_of_range);
  const bool declared = !provenance.origin[0].empty();
  for (const std::string_view axis : provenance.origin)
    if (declared ? !api::valid_origin_coordinate(axis) : !axis.empty()) return fail(Reason::parameter_out_of_range);
  return {};
}

std::string manifest_head(const api::Product& product, const Provenance& provenance, std::string_view name,
                          std::string_view format, u64 bytes, const io::Digest& sha256, const io::Digest& tree) {
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
  parameters(out, provenance, product);
  inputs(out, provenance);
  out.append(",\"files\":[{\"name\":");
  quoted(out, name);
  out.append(",\"format\":");
  quoted(out, format);
  out.append(",\"version\":1,\"bytes\":");
  number(out, bytes);
  out.append(",\"sha256\":");
  hex(out, sha256);
  out.append("}],\"tree_k_sha256\":");
  hex(out, tree);
  return out;
}

void manifest_number(std::string& out, u64 value) { number(out, value); }

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
  parameters(out, provenance, product);
  inputs(out, provenance);
  out.append(",\"files\":[{\"name\":");
  quoted(out, api::kFullFileName);
  out.append(",\"format\":\"MHGP11FUL1\",\"version\":1,\"bytes\":");
  number(out, file_bytes);
  out.append(",\"sha256\":");
  hex(out, file_sha256);
  out.append("}],\"tree_k_sha256\":");
  hex(out, api::tree_k_sha256(tower.domain(), tower.order(product.k())));
  full_counts(out, tower);
  out.append("}\n");
  return out;
}

namespace {

// Comptes de la sortie supports (docs/SORTIES.md, paragraphe 8), dans l'ordre fixe du contrat.
void supports_counts(std::string& out, const OrderTree& tree, const supports::SupportHierarchy& h) {
  const OrderForest& forest = tree.forest();
  std::array<u64, 3> roles{}, arities{};
  u64 extended = 0, multiple = 0, widest = 0, kparties_sum = 0, kparties_max = 0, cofaces_sum = 0, cofaces_max = 0;
  const auto offsets = h.support_offsets();
  for (u64 b = 0; b < h.balls().size(); ++b) {
    const supports::Ball& ball = h.balls()[b];
    ++roles[static_cast<std::size_t>(ball.role)];
    const u64 count = offsets[b + 1] - offsets[b];
    extended += ball.m > ball.qmin;
    multiple += count >= 2;
    widest = std::max(widest, count);
    kparties_sum += ball.kparties_reliees;
    kparties_max = std::max<u64>(kparties_max, ball.kparties_reliees);
    cofaces_sum += ball.cofaces;
    cofaces_max = std::max<u64>(cofaces_max, ball.cofaces);
  }
  for (const supports::Support& support : h.supports()) ++arities[support.arity - 2];
  out.append(",\"counts\":{\"sites\":");
  number(out, tree.domain().index().cloud().sites());
  out.append(",\"nodes\":");
  number(out, forest.nodes().size());
  out.append(",\"births\":");
  number(out, forest.births());
  out.append(",\"merges\":");
  number(out, forest.nodes().size() - forest.births());
  out.append(",\"balls\":");
  number(out, h.balls().size());
  out.append(",\"roles\":{\"birth\":");
  number(out, roles[0]);
  out.append(",\"merge\":");
  number(out, roles[1]);
  out.append(",\"internal\":");
  number(out, roles[2]);
  out.append("},\"supports\":");
  number(out, h.supports().size());
  out.append(",\"arities\":{\"2\":");
  number(out, arities[0]);
  out.append(",\"3\":");
  number(out, arities[1]);
  out.append(",\"4\":");
  number(out, arities[2]);
  out.append("},\"extended_shells\":");
  number(out, extended);
  out.append(",\"multi_support_balls\":");
  number(out, multiple);
  out.append(",\"max_supports_per_ball\":");
  number(out, widest);
  out.append(",\"prior\":");
  number(out, h.prior().size());
  out.append(",\"kparties_reliees\":{\"sum\":");
  number(out, kparties_sum);
  out.append(",\"max\":");
  number(out, kparties_max);
  out.append("},\"cofaces\":{\"sum\":");
  number(out, cofaces_sum);
  out.append(",\"max\":");
  number(out, cofaces_max);
  out.append("}}");
}

}  // namespace

std::string supports_manifest(const api::Product& product, const Provenance& provenance, u64 bytes,
                              const io::Digest& sha256) {
  const OrderTree& tree = product.order_tree();
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
  parameters(out, provenance, product);
  inputs(out, provenance);
  out.append(",\"files\":[{\"name\":");
  quoted(out, api::kSupportsFileName);
  out.append(",\"format\":\"MHGP11SP\",\"version\":1,\"bytes\":");
  number(out, bytes);
  out.append(",\"sha256\":");
  hex(out, sha256);
  out.append("}],\"tree_k_sha256\":");
  hex(out, api::tree_k_sha256(tree.domain(), tree.forest()));
  supports_counts(out, tree, product.hierarchy());
  out.append("}\n");
  return out;
}

}  // namespace api_detail

}  // namespace mhgp11

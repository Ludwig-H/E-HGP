// Executable mhgp11 : sortie parametree de la v11 (docs/SORTIES.md), au-dessus de la facade api (seul en-tete
// inclus). Tranche S5 : --sortie=full seulement ; supports, points et plat sont refuses (parameter_out_of_range)
// jusqu'a la livraison de leur tranche.
//
//   mhgp11 --sortie=full --points=<x.u32le> --ids=<ids.u32le> --dossier=<D> --k=<K>
//          [--fils=<W>] [--budget=<octets>] [--pas=<decimal>] [--origine=<x,y,z>]
//
// Ordre deterministe des refus, les neuf etapes du paragraphe 3 de docs/SORTIES.md ; la cle stage de la ligne de
// refus nomme l'etape (options, plan, session, read, compute, publish, close, report) :
//   1. options (stage options) : forme, inconnue, repetee, sortie absente, inconnue ou non livree, option propre a
//      une autre sortie (--mcs, --z, --selection : plat), option obligatoire absente, chemin vide, valeur hors
//      domaine -> parameter_out_of_range, avant tout effet ;
//   2. plan (stage plan), avant que le processus n'ouvre quoi que ce soit : sortie standard fermee ou en lecture
//      seule (output_unwritable), puis designant l'une des entrees (output_conflict) ; puis le dossier
//      (io::OutputDirectory::plan : parameter_out_of_range, output_unwritable, output_conflict, output_unwritable) ;
//   3. session (stage session) : session_overhead, memory_budget, environment_selftest (code 3) ;
//   4. lecture (stage read, io::read_u32le) : input_unreadable (ouverture, type, tailles), index_overflow_u32,
//      memory_budget (16 octets par point), input_unreadable (lecture incomplete, octet de trop) ;
//   5. a 7. (stage compute, api::compute) : nuage dans l'ordre de prepare_cloud (empty_input,
//      coordinate_out_of_domain, memory_budget, duplicate_point_id, memory_budget), positions repetees
//      (multiplicity_unsupported), K > n (parameter_out_of_range, connu seulement apres la preparation du nuage),
//      calcul ;
//   8. ecriture et publication (stage publish, api::publish) : output_unwritable, output_conflict ;
//   9. apres la publication : fin de session (stage close, api::finish : budget_not_released, code 3), puis ligne
//      d'etat sur la sortie standard (stage report : output_unwritable, code 2). Chacun retire le dossier publie
//      (api::withdraw ; frontiere R2 de la v10, finish de src/core/cli_output.hpp).
// Sortie standard : exactement une ligne JSON a cles anglaises ; temps et nombre de fils n'apparaissent que la, jamais
// dans le manifeste. La ligne de refus va sur la sortie standard si elle peut etre ecrite, sinon sur la sortie
// d'erreur (sortie standard fermee, en lecture seule, egale a une entree, ou en echec). Elle finit toujours par l'etat
// du dossier : "publication":"none" et "manifest_sha256":null, ou, si un retrait a echoue apres un double echec,
// "publication":"published_complete" et l'empreinte du manifeste de D publie et complet (docs/SORTIES.md,
// paragraphe 9). SIGPIPE et SIGXFSZ sont ignores : un tube sans lecteur, une sortie pleine ou une limite de taille de
// fichier rendent une erreur d'ecriture (EPIPE, ENOSPC, EFBIG) et un refus, jamais un arret par signal qui laisserait
// un D.pending orphelin. Codes de sortie : 0 conforme, 2 refus, 3 invariant viole.
// Portes : mhgp11_cli_contract, mhgp11_cli_full_identity, mhgp11_cli_full_determinism, mhgp11_cli_full_relabel ;
// mutants de tests/mutants/cli.json.
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <charconv>
#include <cstdio>
#include <optional>
#include <string_view>

#include "api/api.hpp"

namespace {

using namespace mhgp11;

// Etape du refus, publiee par la ligne de refus (ordre du paragraphe 3 de docs/SORTIES.md).
enum class Step : u8 { options, plan, session, read, compute, publish, close, report };

constexpr std::string_view step_name(Step step) noexcept {
  constexpr std::array<std::string_view, 8> kNames = {"options", "plan",    "session", "read",
                                                       "compute", "publish", "close",   "report"};
  return kNames[static_cast<std::size_t>(step)];
}

// Options connues ; plat_only : propre a la sortie plat, non livree, donc jamais admise ici.
struct Known {
  std::string_view name;
  bool plat_only;
};
constexpr std::array<Known, 12> kKnown = {{{"sortie", false},
                                           {"points", false},
                                           {"ids", false},
                                           {"dossier", false},
                                           {"k", false},
                                           {"fils", false},
                                           {"budget", false},
                                           {"pas", false},
                                           {"origine", false},
                                           {"mcs", true},
                                           {"z", true},
                                           {"selection", true}}};
enum Slot : std::size_t { kSortie, kPoints, kIds, kDossier, kK, kFils, kBudget, kPas, kOrigine };

struct Options {
  const char* points = nullptr;
  const char* ids = nullptr;
  const char* directory = nullptr;
  Order k = 0;
  u32 workers = 1;
  std::optional<u64> budget;
  std::string_view grid_step;
  std::array<std::string_view, 3> origin{};
};

// Entier decimal sans signe, chiffres ASCII seulement, sans debordement de u64.
bool parse_u64(std::string_view text, u64& value) noexcept {
  if (text.empty() || text.size() > 20) return false;
  for (const char c : text)
    if (c < '0' || c > '9') return false;
  const auto done = std::from_chars(text.data(), text.data() + text.size(), value);
  return done.ec == std::errc{} && done.ptr == text.data() + text.size();
}

// --origine=x,y,z : trois decimaux controles par api::valid_origin_coordinate.
bool parse_origin(std::string_view text, std::array<std::string_view, 3>& origin) noexcept {
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::size_t comma = text.find(',');
    if ((axis < 2) != (comma != std::string_view::npos)) return false;
    origin[axis] = text.substr(0, comma);
    if (!api::valid_origin_coordinate(origin[axis])) return false;
    text = axis < 2 ? text.substr(comma + 1) : std::string_view{};
  }
  return true;
}

// Valeurs des options apres la lecture de argv ; une valeur va jusqu'a la fin de son argument (chaine C).
struct Raw {
  std::array<std::string_view, kKnown.size()> value{};
  std::array<bool, kKnown.size()> seen{};
};

const char* read_arguments(int argc, char** argv, Raw& raw) noexcept {
  for (int i = 1; i < argc; ++i) {
    const std::string_view argument(argv[i]);
    const std::size_t equal = argument.find('=');
    if (argument.substr(0, 2) != "--" || equal == std::string_view::npos)
      return "argument hors de la forme --nom=valeur";
    const std::string_view name = argument.substr(2, equal - 2);
    std::size_t slot = 0;
    while (slot < kKnown.size() && kKnown[slot].name != name) ++slot;
    if (slot == kKnown.size()) return "option inconnue";
    if (raw.seen[slot]) return "option repetee";
    raw.seen[slot] = true;
    raw.value[slot] = argument.substr(equal + 1);
  }
  return nullptr;
}

// Etape 1 : rend nullptr si les options sont conformes, sinon la cause du refus parameter_out_of_range.
const char* parse(int argc, char** argv, Options& o) noexcept {
  Raw raw;
  if (const char* why = read_arguments(argc, argv, raw)) return why;
  if (!raw.seen[kSortie]) return "option --sortie absente";
  const std::string_view output = raw.value[kSortie];
  if (output == "supports" || output == "points" || output == "plat") return "sortie non livree dans cette tranche";
  if (output != api::output_name(api::OutputKind::full)) return "sortie inconnue";
  for (std::size_t slot = 0; slot < kKnown.size(); ++slot)
    if (raw.seen[slot] && kKnown[slot].plat_only) return "option propre a la sortie plat";
  for (const Slot slot : {kPoints, kIds, kDossier, kK})
    if (!raw.seen[slot]) return "option obligatoire absente (--points, --ids, --dossier, --k)";
  for (const Slot slot : {kPoints, kIds, kDossier})
    if (raw.value[slot].empty()) return "chemin vide";
  o.points = raw.value[kPoints].data();
  o.ids = raw.value[kIds].data();
  o.directory = raw.value[kDossier].data();
  u64 number = 0;
  if (!parse_u64(raw.value[kK], number) || number < 1 || number > api::kMaxOrder) return "--k hors de 1..12";
  o.k = static_cast<Order>(number);
  if (raw.seen[kFils]) {
    if (!parse_u64(raw.value[kFils], number) || number < 1 || number > sched::kMaxWorkers)
      return "--fils hors de 1..256";
    o.workers = static_cast<u32>(number);
  }
  if (raw.seen[kBudget]) {
    if (!parse_u64(raw.value[kBudget], number) || number == 0) return "--budget hors de 1..2^64-1 octets";
    o.budget = number;
  }
  if (raw.seen[kPas]) {
    if (!api::valid_grid_step(raw.value[kPas])) return "--pas n'est pas un decimal strictement positif";
    o.grid_step = raw.value[kPas];
  }
  if (raw.seen[kOrigine] && !parse_origin(raw.value[kOrigine], o.origin)) return "--origine n'est pas x,y,z decimaux";
  return nullptr;
}

// Etat de la sortie standard, lu des le debut sans rien ouvrir ni ecrire : ouverte en ecriture, et fichier regulier
// qui est aussi l'une des entrees nommees par --points= ou --ids= (meme peripherique et meme inode), lues dans argv
// avant toute validation des options. Une ligne de refus n'est jamais ecrite dans une entree, meme a l'etape 1.
struct StandardOutput {
  bool writable = false;
  bool input = false;
};

StandardOutput inspect_stdout(int argc, char** argv) noexcept {
  StandardOutput standard;
  const int flags = ::fcntl(STDOUT_FILENO, F_GETFL);
  struct stat out {};
  standard.writable = flags >= 0 && (flags & O_ACCMODE) != O_RDONLY && ::fstat(STDOUT_FILENO, &out) == 0;
  if (!standard.writable || !S_ISREG(out.st_mode)) return standard;
  for (int i = 1; i < argc; ++i) {
    const std::string_view argument(argv[i]);
    for (const std::string_view prefix : {std::string_view("--points="), std::string_view("--ids=")}) {
      struct stat in {};
      if (argument.substr(0, prefix.size()) == prefix && ::stat(argv[i] + prefix.size(), &in) == 0 &&
          in.st_dev == out.st_dev && in.st_ino == out.st_ino)
        standard.input = true;
    }
  }
  return standard;
}

// Etat d'un appel, pour la ligne de la sortie standard.
struct Run {
  Step step = Step::options;
  bool stdout_usable = true;
  api::RunReport report;
  u64 read_ns = 0;
  u32 workers = 0, sites = 0;
  u64 nodes = 0, births = 0, edges = 0;
  api::Publication publication;  // issue, etat du dossier et empreinte du manifeste de D publie
};

void tally(const api::Product& product, Run& run) noexcept {
  const FullTower& tower = product.full();
  run.sites = tower.domain().index().cloud().sites();
  for (u32 k = 1; k <= tower.kmax(); ++k) {
    const OrderForest& forest = tower.order(static_cast<Order>(k));
    run.nodes += forest.nodes().size();
    run.births += forest.births();
    run.edges += forest.edges().size();
  }
}

// Etapes 3 a 9 (fin de session) sur un dossier planifie. Entrees et produit sont rendus au budget avant la fin de
// session ; un refus de publication ou de fin de session porte l'etat du dossier (api::withdraw).
api::Publication execute(const Options& o, io::OutputDirectory& directory, Run& run) noexcept {
  run.step = Step::session;
  Result<api::Session> made = api::Session::make({o.budget.value_or(MemoryBudget::kUnlimited), o.workers});
  if (!made.ok()) return {made.outcome()};
  api::Session& session = made.value();
  run.workers = session.workers();
  {
    run.step = Step::read;
    const Stopwatch read_clock;
    Result<io::InputFiles> input = io::read_u32le(o.points, o.ids, session.budget());
    if (!input.ok()) return {input.outcome()};
    run.read_ns = read_clock.nanoseconds();
    const io::InputFiles& in = input.value();
    run.step = Step::compute;
    const api::CloudView view{in.x.span(), in.y.span(), in.z.span(), in.ids.span()};
    Result<api::Product> product = api::compute(session, view, api::FullRequest{o.k}, &run.report);
    if (!product.ok()) return {product.outcome()};
    tally(product.value(), run);
    run.step = Step::publish;
    const api::Provenance provenance{in.points_sha256, in.ids_sha256, in.points_bytes, in.ids_bytes,
                                     o.budget,         o.grid_step,   o.origin};
    const api::Publication published = api::publish(session, product.value(), directory, provenance, &run.report);
    if (!published.ok()) return published;
  }
  run.step = Step::close;
  return api::finish(session, directory);
}

// Ligne de refus ; output vaut null tant que les options ne sont pas lues ; publication et manifest_sha256 disent ce
// qui reste publie (docs/SORTIES.md, paragraphes 3 et 9) : none et null, ou published_complete et l'empreinte du
// manifeste de D.
void refusal_line(std::FILE* stream, const Outcome& outcome, const Run& run) noexcept {
  const std::string_view status = status_name(outcome.status()), reason = reason_name(outcome.reason);
  const std::string_view stage = step_name(run.step);
  const api::PublicationState state = run.publication.state;
  const std::string_view publication = api::publication_state_name(state);
  std::fprintf(stream,
               "{\"phase\":\"mhgp11\",\"output\":%s,\"status\":\"%.*s\",\"reason\":\"%.*s\",\"stage\":\"%.*s\","
               "\"coord_bits\":%d,\"publication\":\"%.*s\",\"manifest_sha256\":",
               run.step != Step::options ? "\"full\"" : "null", static_cast<int>(status.size()), status.data(),
               static_cast<int>(reason.size()), reason.data(), static_cast<int>(stage.size()), stage.data(),
               kCoordBits, static_cast<int>(publication.size()), publication.data());
  if (state == api::PublicationState::published_complete) {
    const auto manifest = io::to_hex(run.publication.manifest_sha256);
    std::fprintf(stream, "\"%.*s\"}\n", static_cast<int>(manifest.size()), manifest.data());
  } else {
    std::fputs("null}\n", stream);
  }
}

// Refus : la ligne va sur la sortie standard si elle est utilisable et accepte la ligne entiere, sinon sur la sortie
// d'erreur ; puis un message en francais sur la sortie d'erreur.
int refuse(const Outcome& outcome, const Run& run, const char* why) noexcept {
  bool written = false;
  if (run.stdout_usable) {
    refusal_line(stdout, outcome, run);
    written = std::fflush(stdout) == 0 && std::ferror(stdout) == 0;
  }
  if (!written) refusal_line(stderr, outcome, run);
  const std::string_view reason = reason_name(outcome.reason), stage = step_name(run.step);
  const bool published = run.publication.state == api::PublicationState::published_complete;
  std::fprintf(stderr, "mhgp11 : refus %.*s a l'etape %.*s%s%s%s\n", static_cast<int>(reason.size()), reason.data(),
               static_cast<int>(stage.size()), stage.data(), why != nullptr ? " : " : "", why != nullptr ? why : "",
               published ? " ; retrait impossible : le dossier reste publie et complet (published_complete)" : "");
  return exit_code(outcome);
}

unsigned long long ull(u64 value) noexcept { return static_cast<unsigned long long>(value); }

// Ligne de succes ; rend faux si la sortie standard ne l'a pas acceptee en entier. Cles des etages : api::stage_name.
bool success_line(const Options& o, const Run& run) noexcept {
  using api::Stage;
  const auto& r = run.report;
  std::printf("{\"phase\":\"mhgp11\",\"output\":\"full\",\"status\":\"ok\",\"reason\":\"none\",\"coord_bits\":%d,"
              "\"k\":%u,\"workers\":%u,\"sites\":%u,\"stages_ns\":{",
              kCoordBits, unsigned{o.k}, run.workers, run.sites);
  constexpr std::array<Stage, 8> kTimed = {Stage::cloud,  Stage::index,  Stage::domain, Stage::tree,
                                           Stage::attach, Stage::output, Stage::write,  Stage::total};
  for (std::size_t i = 0; i < kTimed.size(); ++i) {
    const std::string_view name = api::stage_name(kTimed[i]);
    // cloud : lecture des fichiers et preparation du nuage (docs/SORTIES.md, paragraphe 3).
    const u64 ns = r.at(kTimed[i]).nanoseconds + (kTimed[i] == Stage::cloud ? run.read_ns : 0);
    std::printf("%s\"%.*s\":%llu", i == 0 ? "" : ",", static_cast<int>(name.size()), name.data(), ull(ns));
  }
  std::printf("},\"peaks_bytes\":{");
  constexpr std::array<Stage, 6> kPeaks = {Stage::cloud, Stage::index,  Stage::domain,
                                           Stage::tree,  Stage::output, Stage::write};
  for (std::size_t i = 0; i < kPeaks.size(); ++i) {
    const std::string_view name = api::stage_name(kPeaks[i]);
    std::printf("%s\"%.*s\":%llu", i == 0 ? "" : ",", static_cast<int>(name.size()), name.data(),
                ull(r.at(kPeaks[i]).peak_bytes));
  }
  const auto manifest = io::to_hex(run.publication.manifest_sha256);
  std::printf("},\"counts\":{\"nodes\":%llu,\"births\":%llu,\"edges\":%llu},\"publication\":\"published_complete\","
              "\"manifest_sha256\":\"%.*s\"}\n",
              ull(run.nodes), ull(run.births), ull(run.edges), static_cast<int>(manifest.size()), manifest.data());
  const bool flushed = std::fflush(stdout) == 0;
  return flushed && std::ferror(stdout) == 0;
}

}  // namespace

int main(int argc, char** argv) {
  const Stopwatch total;
  ::signal(SIGPIPE, SIG_IGN);
  ::signal(SIGXFSZ, SIG_IGN);
  const StandardOutput standard = inspect_stdout(argc, argv);
  Options o;
  Run run;
  run.stdout_usable = standard.writable && !standard.input;
  if (const char* why = parse(argc, argv, o)) return refuse(fail(Reason::parameter_out_of_range), run, why);
  // Etape 2, avant toute ouverture de fichier par le processus : sortie standard, puis dossier.
  run.step = Step::plan;
  if (!standard.writable) return refuse(fail(Reason::output_unwritable), run, "sortie standard fermee ou en lecture");
  if (standard.input) return refuse(fail(Reason::output_conflict), run, "sortie standard egale a une entree");
  const std::array<const char*, 2> inputs{o.points, o.ids};
  Result<io::OutputDirectory> planned = io::OutputDirectory::plan(o.directory, inputs);
  if (!planned.ok()) return refuse(planned.outcome(), run, "dossier de sortie");
  io::OutputDirectory& directory = planned.value();
  run.publication = execute(o, directory, run);
  if (!run.publication.ok()) return refuse(run.publication.outcome, run, nullptr);
  run.report.at(api::Stage::total) = {total.nanoseconds(), 0};
  run.step = Step::report;
  if (!success_line(o, run)) {
    run.publication = api::withdraw(fail(Reason::output_unwritable), directory);
    run.stdout_usable = false;  // la ligne de refus va sur la sortie d'erreur
    return refuse(run.publication.outcome, run, "sortie standard en echec apres la publication");
  }
  return 0;
}

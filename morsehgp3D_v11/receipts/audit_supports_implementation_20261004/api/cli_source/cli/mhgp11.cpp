// Executable mhgp11 : sortie parametree de la v11 (specification de la sortie parametree, paragraphe 5), au-dessus de
// la facade api (seul en-tete inclus). Tranche S5 : --sortie=full seulement ; supports, points et plat sont refuses
// (parameter_out_of_range) jusqu'a la livraison de leur tranche.
//
//   mhgp11 --sortie=full --points=<x.u32le> --ids=<ids.u32le> --dossier=<D> --k=<K>
//          [--fils=<W>] [--budget=<octets>] [--pas=<decimal>] [--origine=<x,y,z>]
//
// Ordre deterministe des refus ; aucun ne laisse de dossier publie :
//   1. options : forme, inconnue, repetee, sortie absente, inconnue ou non livree, option propre a une autre sortie
//      (--mcs, --z, --selection : plat), option obligatoire absente, valeur hors domaine -> parameter_out_of_range,
//      avant tout effet ;
//   2. plan de sortie, avant toute lecture : sortie standard fermee ou non inscriptible (output_unwritable) ou
//      designant une entree (output_conflict), puis le dossier (io::OutputDirectory::plan : forme, parent, conflits) ;
//   3. session : session_overhead, memory_budget, environment_selftest ;
//   4. lecture : input_unreadable, index_overflow_u32, memory_budget ;
//   5-7. nuage, positions repetees, K > n, calcul (api::compute) ;
//   8. ecriture et publication (api::publish), fin de session (budget_not_released), ligne de la sortie standard.
// Sortie standard : exactement une ligne JSON a cles anglaises ; temps et nombre de fils n'apparaissent que la, jamais
// dans le manifeste. Si la sortie standard est fermee ou designe une entree, la ligne de refus va sur la sortie
// d'erreur. Si la ligne ne peut pas etre ecrite APRES la publication (sortie pleine, tube sans lecteur : SIGPIPE est
// ignore pour que l'ecriture rende EPIPE au lieu de tuer le processus), le dossier publie est retire
// (io::OutputDirectory::retract ; frontiere R2 de la v10, finish de src/core/cli_output.hpp). Meme retrait si la fin
// de session echoue apres la publication. Codes de sortie : 0 conforme, 2 refus, 3 invariant viole.
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

// Etape du refus, publiee par la ligne de refus (ordre du paragraphe 5).
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
    if (argument.substr(0, 2) != "--" || equal == std::string_view::npos) return "argument hors de la forme --nom=valeur";
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
    if (!parse_u64(raw.value[kFils], number) || number < 1 || number > sched::kMaxWorkers) return "--fils hors de 1..256";
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

// Etape 2, avant toute ouverture de fichier par le processus : la sortie standard est ouverte en ecriture et ne
// designe aucune entree (meme peripherique et meme inode). `usable` dit si la ligne de refus peut y aller.
Outcome check_stdout(const Options& o, bool& usable) noexcept {
  usable = false;
  const int flags = ::fcntl(STDOUT_FILENO, F_GETFL);
  struct stat out {};
  if (flags < 0 || (flags & O_ACCMODE) == O_RDONLY || ::fstat(STDOUT_FILENO, &out) != 0)
    return fail(Reason::output_unwritable);
  if (S_ISREG(out.st_mode)) {
    for (const char* input : {o.points, o.ids}) {
      struct stat in {};
      if (::stat(input, &in) == 0 && in.st_dev == out.st_dev && in.st_ino == out.st_ino)
        return fail(Reason::output_conflict);
    }
  }
  usable = true;
  return {};
}

// Etat d'un appel, pour la ligne de la sortie standard.
struct Run {
  Step step = Step::options;
  bool stdout_usable = true;
  api::RunReport report;
  u64 read_ns = 0;
  u32 workers = 0, sites = 0;
  u64 nodes = 0, births = 0, edges = 0;
  io::Digest manifest{};
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

// Etapes 3 a 8 sur un dossier planifie. Entrees et produit sont rendus au budget avant la fin de session.
Outcome execute(const Options& o, io::OutputDirectory& directory, Run& run) noexcept {
  run.step = Step::session;
  Result<api::Session> made = api::Session::make({o.budget.value_or(MemoryBudget::kUnlimited), o.workers});
  if (!made.ok()) return made.outcome();
  api::Session& session = made.value();
  run.workers = session.workers();
  {
    run.step = Step::read;
    const Stopwatch read_clock;
    Result<io::InputFiles> input = io::read_u32le(o.points, o.ids, session.budget());
    if (!input.ok()) return input.outcome();
    run.read_ns = read_clock.nanoseconds();
    const io::InputFiles& in = input.value();
    run.step = Step::compute;
    const api::CloudView view{in.x.span(), in.y.span(), in.z.span(), in.ids.span()};
    Result<api::Product> product = api::compute(session, view, api::FullRequest{o.k}, &run.report);
    if (!product.ok()) return product.outcome();
    tally(product.value(), run);
    run.step = Step::publish;
    const api::Provenance provenance{in.points_sha256, in.ids_sha256, in.points_bytes, in.ids_bytes,
                                     o.budget,         o.grid_step,   o.origin};
    Result<io::Digest> published = api::publish(session, product.value(), directory, provenance, &run.report);
    if (!published.ok()) return published.outcome();
    run.manifest = published.value();
  }
  run.step = Step::close;
  return session.close();
}

void refusal_line(std::FILE* stream, const Outcome& outcome, Step step, bool output_known) noexcept {
  const std::string_view status = status_name(outcome.status()), reason = reason_name(outcome.reason);
  const std::string_view stage = step_name(step);
  std::fprintf(stream,
               "{\"phase\":\"mhgp11\",\"output\":%s,\"status\":\"%.*s\",\"reason\":\"%.*s\",\"stage\":\"%.*s\","
               "\"coord_bits\":%d}\n",
               output_known ? "\"full\"" : "null", static_cast<int>(status.size()), status.data(),
               static_cast<int>(reason.size()), reason.data(), static_cast<int>(stage.size()), stage.data(),
               kCoordBits);
}

int refuse(const Outcome& outcome, const Run& run, const char* why) noexcept {
  refusal_line(run.stdout_usable ? stdout : stderr, outcome, run.step, run.step != Step::options);
  std::fflush(stdout);
  const std::string_view reason = reason_name(outcome.reason), stage = step_name(run.step);
  std::fprintf(stderr, "mhgp11 : refus %.*s a l'etape %.*s%s%s\n", static_cast<int>(reason.size()), reason.data(),
               static_cast<int>(stage.size()), stage.data(), why != nullptr ? " : " : "", why != nullptr ? why : "");
  return exit_code(outcome);
}

unsigned long long ull(u64 value) noexcept { return static_cast<unsigned long long>(value); }

// Ligne de succes ; rend faux si la sortie standard ne l'a pas acceptee en entier.
bool success_line(const Options& o, const Run& run) noexcept {
  const auto& r = run.report;
  const auto ns = [&](api::Stage s) { return ull(r.at(s).nanoseconds); };
  const auto peak = [&](api::Stage s) { return ull(r.at(s).peak_bytes); };
  using api::Stage;
  const auto manifest = io::to_hex(run.manifest);
  std::printf("{\"phase\":\"mhgp11\",\"output\":\"full\",\"status\":\"ok\",\"reason\":\"none\",\"coord_bits\":%d,"
              "\"k\":%u,\"workers\":%u,\"sites\":%u,",
              kCoordBits, unsigned{o.k}, run.workers, run.sites);
  std::printf("\"stages_ns\":{\"read\":%llu,\"cloud\":%llu,\"index\":%llu,\"domain\":%llu,\"tree\":%llu,"
              "\"attach\":%llu,\"output\":%llu,\"write\":%llu,\"total\":%llu},",
              ull(run.read_ns), ns(Stage::cloud), ns(Stage::index), ns(Stage::domain), ns(Stage::tree),
              ns(Stage::attach), ns(Stage::output), ns(Stage::write), ns(Stage::total));
  std::printf("\"peaks_bytes\":{\"cloud\":%llu,\"index\":%llu,\"domain\":%llu,\"tree\":%llu,\"output\":%llu,"
              "\"write\":%llu},\"counts\":{\"nodes\":%llu,\"births\":%llu,\"edges\":%llu},"
              "\"manifest_sha256\":\"%.*s\"}\n",
              peak(Stage::cloud), peak(Stage::index), peak(Stage::domain), peak(Stage::tree), peak(Stage::output),
              peak(Stage::write), ull(run.nodes), ull(run.births), ull(run.edges), static_cast<int>(manifest.size()),
              manifest.data());
  const bool flushed = std::fflush(stdout) == 0;
  return flushed && std::ferror(stdout) == 0;
}

}  // namespace

int main(int argc, char** argv) {
  const Stopwatch total;
  ::signal(SIGPIPE, SIG_IGN);
  Options o;
  Run run;
  if (const char* why = parse(argc, argv, o)) return refuse(fail(Reason::parameter_out_of_range), run, why);
  run.step = Step::plan;
  const Outcome standard = check_stdout(o, run.stdout_usable);
  if (!standard.ok()) return refuse(standard, run, "sortie standard fermee, non inscriptible ou egale a une entree");
  const std::array<const char*, 2> inputs{o.points, o.ids};
  Result<io::OutputDirectory> planned = io::OutputDirectory::plan(o.directory, inputs);
  if (!planned.ok()) return refuse(planned.outcome(), run, "dossier de sortie");
  io::OutputDirectory& directory = planned.value();
  const Outcome executed = execute(o, directory, run);
  if (!executed.ok()) {
    if (directory.committed() && !directory.retract().ok())
      std::fprintf(stderr, "mhgp11 : retrait du dossier publie impossible\n");
    return refuse(executed, run, nullptr);
  }
  run.report.at(api::Stage::total) = {total.nanoseconds(), 0};
  run.step = Step::report;
  if (!success_line(o, run)) {
    const bool retracted = directory.retract().ok();
    std::fprintf(stderr, "mhgp11 : refus output_unwritable a l'etape report : sortie standard en echec apres la "
                         "publication, dossier %s\n", retracted ? "retire" : "NON retire");
    return exit_code(fail(Reason::output_unwritable));
  }
  return 0;
}

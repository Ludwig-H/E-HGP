// Portes du plan de la transaction de dossier (OutputDirectory::plan, create) :
//   plan_syntax        chemin vide, "/", ".", "..", nom final trop long, chemin de PATH_MAX octets :
//                      parameter_out_of_range ; barres finales admises ; le plan ne cree rien ;
//   plan_parent        parent absent ou fichier regulier : output_unwritable ;
//   plan_conflicts     D existe (dossier, fichier, lien symbolique pendant), D.pending orphelin : output_conflict,
//                      et l'orphelin n'est jamais retire ;
//   plan_inputs        entree sous D (par lien symbolique, par lien physique, D.pending), entree egale a D :
//                      output_conflict ; entrees voisines ou absentes admises ;
//   create_names       noms hors de [a-z0-9_.]+, manifeste reserve, trop de fichiers : parameter_out_of_range sans
//                      rien creer ; nom double : output_conflict, sans empecher la publication ;
//   parent_unwritable  parent sans droit d'ecriture : output_unwritable (sous root, ou les droits ne refusent rien,
//                      le refus est attendu au plus tard a la creation de D.pending dans /proc).
#include <sys/stat.h>
#include <unistd.h>

#include <climits>
#include <string>
#include <vector>

#include "io_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::io_test;

namespace {

Reason plan_reason(const std::string& directory, std::vector<const char*> inputs = {}) {
  const Result<io::OutputDirectory> r = io::OutputDirectory::plan(directory.c_str(), inputs);
  return r.ok() ? Reason::none : r.outcome().reason;
}

}  // namespace

MHGP11_TEST(plan_syntax, 17) {
  Scratch s;
  REQUIRE(s.ok());
  CHECK_EQ(io::OutputDirectory::plan(nullptr, {}).outcome().reason, Reason::parameter_out_of_range);
  for (const char* bad : {"", "/", ".", "..", "./..", "a/."})
    CHECK_EQ(plan_reason(bad), Reason::parameter_out_of_range);
  CHECK_EQ(plan_reason(s.path(std::string(248, 'd'))), Reason::parameter_out_of_range);  // 248 + 8 > NAME_MAX
  CHECK_EQ(plan_reason(s.path(std::string(247, 'd'))), Reason::none);
  CHECK_EQ(plan_reason(std::string(PATH_MAX, 'p')), Reason::parameter_out_of_range);
  CHECK_EQ(plan_reason(s.path("D//")), Reason::none);
  // le plan ne cree rien, en succes comme en refus
  CHECK(entries(s.root()).empty());
  {
    Result<io::OutputDirectory> r = io::OutputDirectory::plan(s.path("D/").c_str(), {});
    REQUIRE(r.ok());
    io::OutputDirectory out = std::move(r).take();
    CHECK(entries(s.root()).empty());
    CHECK(out.commit("{}").ok());  // "D/" designe D
  }
  CHECK(entries(s.root()) == std::vector<std::string>{"D"});
}

MHGP11_TEST(plan_parent, 5) {
  Scratch s;
  REQUIRE(s.ok());
  CHECK_EQ(plan_reason(s.path("absent/D")), Reason::output_unwritable);
  REQUIRE(write_file(s.path("file"), {1}));
  CHECK_EQ(plan_reason(s.path("file/D")), Reason::output_unwritable);
  CHECK(entries(s.root()) == std::vector<std::string>{"file"});
}

MHGP11_TEST(plan_conflicts, 16) {
  Scratch s;
  REQUIRE(s.ok());
  REQUIRE(::mkdir(s.path("D").c_str(), 0700) == 0);
  CHECK_EQ(plan_reason(s.path("D")), Reason::output_conflict);
  CHECK_EQ(plan_reason(s.path("D/")), Reason::output_conflict);
  REQUIRE(write_file(s.path("F"), {1}));
  CHECK_EQ(plan_reason(s.path("F")), Reason::output_conflict);
  REQUIRE(::symlink(s.path("nowhere").c_str(), s.path("L").c_str()) == 0);
  CHECK_EQ(plan_reason(s.path("L")), Reason::output_conflict);  // lien pendant : existe
  // D.pending orphelin : refus, et l'orphelin et son contenu restent intacts apres destruction
  REQUIRE(::mkdir(s.path("E.pending").c_str(), 0700) == 0);
  REQUIRE(write_file(s.path("E.pending/partial.bin"), {7, 7}));
  CHECK_EQ(plan_reason(s.path("E")), Reason::output_conflict);
  CHECK(exists(s.path("E.pending/partial.bin")));
  CHECK(!exists(s.path("E")));
  // un lien symbolique nomme D.pending compte aussi
  REQUIRE(::symlink(s.path("nowhere").c_str(), s.path("G.pending").c_str()) == 0);
  CHECK_EQ(plan_reason(s.path("G")), Reason::output_conflict);
  CHECK(exists(s.path("G.pending")));
}

MHGP11_TEST(plan_inputs, 17) {
  Scratch s;
  REQUIRE(s.ok());
  REQUIRE(::mkdir(s.path("real").c_str(), 0700) == 0);
  REQUIRE(write_file(s.path("real/points.u32le"), {1, 2, 3}));
  REQUIRE(write_file(s.path("ids.u32le"), {4}));
  const std::string points = s.path("real/points.u32le"), ids = s.path("ids.u32le");
  // entrees voisines, entree absente : admises
  CHECK_EQ(plan_reason(s.path("D"), {points.c_str(), ids.c_str()}), Reason::none);
  CHECK_EQ(plan_reason(s.path("D"), {s.path("absent").c_str(), nullptr}), Reason::none);
  // la sortie est un dossier d'entree : D = real
  CHECK_EQ(plan_reason(s.path("real"), {points.c_str()}), Reason::output_conflict);
  // entree designee sous D par un lien symbolique : D -> real
  REQUIRE(::symlink(s.path("real").c_str(), s.path("D").c_str()) == 0);
  const std::string through = s.path("D/points.u32le");
  CHECK_EQ(plan_reason(s.path("D"), {through.c_str()}), Reason::output_conflict);
  // entree dont un lien physique se trouve sous D
  REQUIRE(::mkdir(s.path("H").c_str(), 0700) == 0);
  REQUIRE(::link(ids.c_str(), s.path("H/ids.u32le").c_str()) == 0);
  CHECK_EQ(plan_reason(s.path("H"), {ids.c_str()}), Reason::output_conflict);
  // entree sous un D.pending orphelin, entree egale a D
  REQUIRE(::mkdir(s.path("P.pending").c_str(), 0700) == 0);
  REQUIRE(write_file(s.path("P.pending/x"), {9}));
  const std::string orphan_input = s.path("P.pending/x");
  CHECK_EQ(plan_reason(s.path("P"), {orphan_input.c_str()}), Reason::output_conflict);
  CHECK_EQ(plan_reason(ids, {ids.c_str()}), Reason::output_conflict);
  CHECK(exists(s.path("P.pending/x")));
}

MHGP11_TEST(create_names, 28) {
  Scratch s;
  REQUIRE(s.ok());
  Result<io::OutputDirectory> r = io::OutputDirectory::plan(s.path("D").c_str(), {});
  REQUIRE(r.ok());
  io::OutputDirectory out = std::move(r).take();
  const std::string long_name(io::kMaxFileName + 1, 'a');
  for (const std::string& bad : {std::string(""), std::string("A"), std::string("a b"), std::string(".x"),
                                std::string("../x"), std::string("a/b"), std::string("x-y"), long_name,
                                std::string(io::kManifestName)})
    CHECK_EQ(out.create(bad).outcome().reason, Reason::parameter_out_of_range);
  CHECK(entries(s.root()).empty());  // aucun refus de nom ne cree D.pending
  const Result<io::FileWriter*> first = out.create("a.bin");
  REQUIRE(first.ok());
  CHECK(first.value()->bytes(std::vector<u8>{1, 2}).ok());
  CHECK(exists(s.path("D.pending/a.bin")));
  CHECK_EQ(out.create("a.bin").outcome().reason, Reason::output_conflict);
  for (u32 i = 1; i < io::kMaxOutputFiles; ++i)
    CHECK(out.create("f" + std::to_string(i)).ok());
  CHECK_EQ(out.create("one_too_many").outcome().reason, Reason::parameter_out_of_range);
  CHECK(out.create(std::string(io::kMaxFileName, 'z')).outcome().reason == Reason::parameter_out_of_range);
  // le nom double refuse n'empeche pas la publication : il est refuse avant toute entree-sortie
  CHECK(out.commit("{\"ok\":1}").ok());
  CHECK_EQ(entries(s.path("D")).size(), io::kMaxOutputFiles + 1u);
  CHECK(read_file(s.path("D/a.bin")) == std::vector<u8>({1, 2}));
}

MHGP11_TEST(parent_unwritable, 3) {
  Scratch s;
  REQUIRE(s.ok());
  if (::geteuid() != 0) {
    REQUIRE(::mkdir(s.path("ro").c_str(), 0500) == 0);
    CHECK_EQ(plan_reason(s.path("ro/D")), Reason::output_unwritable);
    return;
  }
  // root : les droits ne refusent rien ; /proc ne recoit aucun dossier
  Result<io::OutputDirectory> r = io::OutputDirectory::plan("/proc/mhgp11_io_D", {});
  Reason reason = r.ok() ? Reason::none : r.outcome().reason;
  if (r.ok()) reason = r.value().create("x").outcome().reason;
  CHECK_EQ(reason, Reason::output_unwritable);
  CHECK(::access("/proc/mhgp11_io_D", F_OK) != 0);  // rien n'a ete cree : meme plancher que la branche ordinaire
}

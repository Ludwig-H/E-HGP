// Portes de la publication du dossier (OutputDirectory::commit et destructeur, FileWriter) :
//   commit         octets petit-boutistes exacts, bourrage, tailles et empreintes tenues au fil de l'ecriture,
//                  manifeste ecrit en dernier, D.pending disparu, aucun commit ni create apres publication, le
//                  destructeur ne retire jamais un dossier publie ;
//   noreplace      D apparu entre le plan et le commit (dossier vide, fichier) : output_conflict, D intact,
//                  D.pending retire a la destruction (jamais un renommage qui ecrase) ; l'empreinte du manifeste,
//                  ferme avant le renommage refuse, est gardee, et committed() reste faux ;
//   orphan         D.pending apparu entre le plan et la creation : output_conflict, jamais retire ;
//   discard        destruction sans commit : ni D ni D.pending ;
//   retract        retrait apres publication : D rendu a D.pending puis retire, refus ensuite ;
//   write_failure  ecriture refusee par le systeme (RLIMIT_FSIZE, SIGXFSZ ignore, a la place de /dev/full qui ne
//                  peut pas etre un fichier de D) au vidage final, pendant l'ecriture, ou sur le manifeste :
//                  output_unwritable, aucun manifeste dans D.pending, rien de publie, aucune empreinte de
//                  manifeste.
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "io_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::io_test;

namespace {

io::OutputDirectory planned(const Scratch& s, const char* name) {
  Result<io::OutputDirectory> r = io::OutputDirectory::plan(s.path(name).c_str(), {});
  if (!r.ok()) std::abort();  // precondition des portes : le plan d'un dossier neuf reussit
  return std::move(r).take();
}

// Limite de taille des fichiers ecrits par ce processus, retablie a la destruction.
class FileSizeLimit {
 public:
  explicit FileSizeLimit(rlim_t bytes) {
    ::signal(SIGXFSZ, SIG_IGN);
    ok_ = ::getrlimit(RLIMIT_FSIZE, &saved_) == 0;
    rlimit limit = saved_;
    limit.rlim_cur = bytes;
    ok_ = ok_ && ::setrlimit(RLIMIT_FSIZE, &limit) == 0;
  }
  FileSizeLimit(const FileSizeLimit&) = delete;
  FileSizeLimit& operator=(const FileSizeLimit&) = delete;
  ~FileSizeLimit() { ::setrlimit(RLIMIT_FSIZE, &saved_); }
  bool ok() const { return ok_; }

 private:
  rlimit saved_{};
  bool ok_ = false;
};

}  // namespace

MHGP12_TEST(commit, 27) {
  Scratch s;
  REQUIRE(s.ok());
  const std::string manifest = "{\"schema\":\"essai\"}";
  {
    io::OutputDirectory out = planned(s, "D");
    Result<io::FileWriter*> data = out.create("data.bin");
    Result<io::FileWriter*> empty = out.create("empty.bin");
    REQUIRE(data.ok() && empty.ok());
    io::FileWriter& w = *data.value();
    CHECK(w.bytes(std::vector<u8>{1, 2, 3}).ok());
    CHECK(w.pad8().ok());
    CHECK_EQ(w.size(), 8u);
    CHECK(w.pad8().ok());  // deja aligne : rien
    CHECK(w.u32s(std::vector<u32>{0x04030201u, 0xFFFFFFFFu}).ok());
    CHECK(w.u64s(std::vector<u64>{0x0807060504030201u}).ok());
    CHECK(w.u32s(std::vector<u32>{}).ok());
    CHECK_EQ(w.size(), 24u);
    CHECK(exists(s.path("D.pending")));
    CHECK(!exists(s.path("D")));
    const Digest digest = w.digest();
    CHECK(out.commit(manifest).ok());
    CHECK(out.committed());
    CHECK(!exists(s.path("D.pending")));
    CHECK(entries(s.path("D")) == std::vector<std::string>({"data.bin", "empty.bin", "manifeste.json"}));
    const std::vector<u8> bytes = read_file(s.path("D/data.bin"));
    CHECK(bytes == std::vector<u8>({1, 2, 3, 0, 0, 0, 0, 0, 1, 2, 3, 4, 255, 255, 255, 255, 1, 2, 3, 4, 5, 6, 7, 8}));
    CHECK(digest_of(bytes) == digest);
    CHECK(read_file(s.path("D/empty.bin")).empty());
    CHECK(read_file(s.path("D/manifeste.json")) == std::vector<u8>(manifest.begin(), manifest.end()));
    CHECK(out.manifest_sha256() == digest_of(manifest));
    // publication definitive : ni second commit, ni fichier de plus, ni ecriture dans un fichier ferme
    CHECK_EQ(out.commit(manifest).reason, Reason::output_unwritable);
    CHECK_EQ(out.create("late.bin").outcome().reason, Reason::output_unwritable);
    CHECK_EQ(w.bytes(std::vector<u8>{9}).reason, Reason::output_unwritable);
  }
  // le destructeur d'un dossier publie ne retire rien
  CHECK_EQ(entries(s.path("D")).size(), 3u);
  // un dossier sans fichier de donnees : le manifeste seul
  {
    io::OutputDirectory out = planned(s, "M");
    CHECK(out.commit("{}").ok());
  }
  CHECK(entries(s.path("M")) == std::vector<std::string>({"manifeste.json"}));
}

// Le renommage est refuse apres la fermeture du manifeste : l'empreinte du manifeste est gardee quand meme (elle ne
// dit pas que D est publie : committed() reste faux).
MHGP12_TEST(noreplace, 23) {
  Scratch s;
  REQUIRE(s.ok());
  const Digest closed = digest_of(std::string_view("{}"));
  {
    io::OutputDirectory out = planned(s, "D");
    REQUIRE(out.create("a.bin").ok());
    REQUIRE(::mkdir(s.path("D").c_str(), 0700) == 0);  // D apparait vide apres le plan
    CHECK_EQ(out.commit("{}").reason, Reason::output_conflict);
    CHECK(!out.committed());
    CHECK(out.manifest_sha256() == closed);
    CHECK(entries(s.path("D")).empty());
    CHECK(exists(s.path("D.pending")));
    CHECK_EQ(out.commit("{}").reason, Reason::output_unwritable);  // un commit refuse est definitif
  }
  CHECK(entries(s.path("D")).empty());
  CHECK(!exists(s.path("D.pending")));
  {
    io::OutputDirectory out = planned(s, "E");
    REQUIRE(out.create("a.bin").ok());
    REQUIRE(write_file(s.path("E"), {5}));  // E apparait comme fichier
    CHECK_EQ(out.commit("{}").reason, Reason::output_conflict);
    CHECK(!out.committed());
    CHECK(out.manifest_sha256() == closed);
  }
  CHECK(read_file(s.path("E")) == std::vector<u8>({5}));
  CHECK(!exists(s.path("E.pending")));
  {
    io::OutputDirectory out = planned(s, "F");  // sans fichier : D.pending est cree au commit
    REQUIRE(::mkdir(s.path("F").c_str(), 0700) == 0);
    CHECK_EQ(out.commit("{}").reason, Reason::output_conflict);
    CHECK(!out.committed());
    CHECK(out.manifest_sha256() == closed);
  }
  CHECK(!exists(s.path("F.pending")));
}

MHGP12_TEST(orphan, 6) {
  Scratch s;
  REQUIRE(s.ok());
  {
    io::OutputDirectory out = planned(s, "D");
    REQUIRE(::mkdir(s.path("D.pending").c_str(), 0700) == 0);  // un autre appel prend D.pending
    CHECK_EQ(out.create("a.bin").outcome().reason, Reason::output_conflict);
    CHECK_EQ(out.commit("{}").reason, Reason::output_unwritable);
  }
  CHECK(exists(s.path("D.pending")));  // jamais retire : cet objet ne l'a pas cree
  CHECK(!exists(s.path("D")));
}

MHGP12_TEST(discard, 7) {
  Scratch s;
  REQUIRE(s.ok());
  {
    io::OutputDirectory out = planned(s, "D");
    Result<io::FileWriter*> a = out.create("a.bin");
    REQUIRE(a.ok());
    CHECK(a.value()->bytes(std::vector<u8>(100000, 3)).ok());  // ecrit au-dela du tampon
    REQUIRE(out.create("b.bin").ok());
    CHECK(entries(s.path("D.pending")) == std::vector<std::string>({"a.bin", "b.bin"}));
  }
  CHECK(!exists(s.path("D.pending")));
  CHECK(entries(s.root()).empty());
}

// Retrait apres publication (frontiere R2) : D renomme en D.pending puis retire a la destruction ; aucun dossier ne
// reste, ni rien a son nom ; un second retrait, un commit ou un create ulterieurs sont refuses.
MHGP12_TEST(retract, 12) {
  Scratch s;
  REQUIRE(s.ok());
  {
    io::OutputDirectory out = planned(s, "D");
    CHECK_EQ(out.retract().reason, Reason::output_unwritable);  // rien de publie : refus
    Result<io::FileWriter*> a = out.create("a.bin");
    REQUIRE(a.ok());
    CHECK(a.value()->bytes(std::vector<u8>{7, 8}).ok());
    REQUIRE(out.commit("{}").ok());
    CHECK(exists(s.path("D")));
    CHECK(out.retract().ok());
    CHECK(!out.committed());
    CHECK(!exists(s.path("D")));
    CHECK_EQ(out.retract().reason, Reason::output_unwritable);
    CHECK_EQ(out.commit("{}").reason, Reason::output_unwritable);
  }
  CHECK(!exists(s.path("D.pending")));
  CHECK(entries(s.root()).empty());
}

MHGP12_TEST(write_failure, 21) {
  Scratch s;
  REQUIRE(s.ok());
  {  // echec au vidage final : 1000 octets dans le tampon, 512 permis
    io::OutputDirectory out = planned(s, "D");
    Result<io::FileWriter*> a = out.create("a.bin");
    REQUIRE(a.ok());
    CHECK(a.value()->bytes(std::vector<u8>(1000, 1)).ok());
    Outcome done;
    {
      FileSizeLimit limit(512);
      REQUIRE(limit.ok());
      done = out.commit("{}");
    }
    CHECK_EQ(done.reason, Reason::output_unwritable);
    CHECK(!exists(s.path("D.pending/manifeste.json")));  // jamais de temoin d'achevement sur des donnees en echec
    CHECK(!exists(s.path("D")));
    CHECK(out.manifest_sha256() == Digest{});  // aucun manifeste ferme : aucune empreinte
  }
  CHECK(!exists(s.path("D.pending")));
  {  // echec pendant l'ecriture : erreur definitive
    io::OutputDirectory out = planned(s, "E");
    Result<io::FileWriter*> a = out.create("a.bin");
    REQUIRE(a.ok());
    Outcome first, second;
    {
      FileSizeLimit limit(512);
      REQUIRE(limit.ok());
      first = a.value()->bytes(std::vector<u8>(200000, 2));
      second = a.value()->bytes(std::vector<u8>{1});
    }
    CHECK_EQ(first.reason, Reason::output_unwritable);
    CHECK_EQ(second.reason, Reason::output_unwritable);
    CHECK_EQ(out.commit("{}").reason, Reason::output_unwritable);
  }
  CHECK(entries(s.root()).empty());
  {  // echec sur le manifeste : donnees courtes, manifeste de 1000 octets
    io::OutputDirectory out = planned(s, "G");
    Result<io::FileWriter*> a = out.create("a.bin");
    REQUIRE(a.ok());
    CHECK(a.value()->bytes(std::vector<u8>(100, 3)).ok());
    Outcome done;
    {
      FileSizeLimit limit(512);
      REQUIRE(limit.ok());
      done = out.commit(std::string(1000, ' '));
    }
    CHECK_EQ(done.reason, Reason::output_unwritable);
    CHECK(out.manifest_sha256() == Digest{});  // manifeste en echec, jamais ferme : aucune empreinte
  }
  CHECK(entries(s.root()).empty());
}

MHGP12_TEST_MAIN()

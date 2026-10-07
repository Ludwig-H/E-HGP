// Porte de l'admission du lecteur MHGP12DP (common/format.hpp), constat CST-0225 de l'auditeur Codex : chaque avancee
// dans le fichier (en-tete de section, donnees, remplissage) doit etre controlee contre la place restante, sans
// addition qui deborde. Fichiers synthetiques ecrits dans le dossier donne, lus par le vrai Reader ; aucun element de
// payload n'est lu au-dela des sections admises.
//
// Usage : mhgp12_format_test <dossier de travail>
// Codes : 0 conforme ; 1 une admission ou un refus differe de l'attendu ; 2 usage ; 3 dossier inutilisable.
#include "format.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace {

using namespace mhgp12::dump;

struct Case {
  std::string name;
  std::vector<unsigned char> bytes;
  bool admitted;    // attendu
  u64 popval_count; // attendu si admis
};

std::vector<unsigned char> header_bytes(u32 sections) {
  Header h{};
  std::memcpy(h.magic, kMagic, 8);
  h.version = kVersion;
  h.kind = kCatalogue;
  h.coord_bits = 21;
  h.kmax = 5;
  h.order = 0;
  h.sections = sections;
  h.sites = 1;
  const auto* p = reinterpret_cast<const unsigned char*>(&h);
  return std::vector<unsigned char>(p, p + sizeof(h));
}

void append_section(std::vector<unsigned char>& out, u32 elem_bytes, u64 count) {
  SectionHeader s{};
  std::memcpy(s.tag, "POPVAL", 6);
  s.elem_bytes = elem_bytes;
  s.count = count;
  const auto* p = reinterpret_cast<const unsigned char*>(&s);
  out.insert(out.end(), p, p + sizeof(s));
}

std::vector<Case> cases() {
  std::vector<Case> all;
  {
    auto b = header_bytes(1);
    append_section(b, 4, 0);
    all.push_back({"section_vide", b, true, 0});
  }
  {
    auto b = header_bytes(1);
    append_section(b, 4, 1);
    b.insert(b.end(), {7, 0, 0, 0, 0, 0, 0, 0});  // un element et son remplissage a 8 octets
    all.push_back({"un_element", b, true, 1});
  }
  {
    // Temoin de l'auditeur : 88 octets, 2^62 - 1 elements de 4 octets annonces, aucun present ; le produit
    // (2^64 - 4) ne deborde pas, l'ancienne addition a l'offset 88 debordait.
    auto b = header_bytes(1);
    append_section(b, 4, (u64{1} << 62) - 1);
    all.push_back({"addition_debordante_cst0225", b, false, 0});
  }
  {
    auto b = header_bytes(1);
    append_section(b, 8, (u64{1} << 61) + 1);  // produit qui deborde
    all.push_back({"produit_debordant", b, false, 0});
  }
  {
    auto b = header_bytes(1);
    append_section(b, 4, 2);
    b.insert(b.end(), {1, 0, 0, 0});  // un seul element present sur deux
    all.push_back({"donnees_tronquees", b, false, 0});
  }
  {
    auto b = header_bytes(1);
    append_section(b, 4, 1);
    b.insert(b.end(), {1, 0, 0, 0});  // element present, remplissage absent
    all.push_back({"remplissage_tronque", b, false, 0});
  }
  {
    auto b = header_bytes(1);
    append_section(b, 4, 1);
    b.insert(b.end(), {1, 0, 0, 0, 0, 0, 0, 0});
    b.insert(b.end(), 8, 0);  // octets en trop
    all.push_back({"octets_en_trop", b, false, 0});
  }
  {
    auto b = header_bytes(1);
    b.insert(b.end(), 10, 0);  // en-tete de section tronque
    all.push_back({"entete_de_section_tronque", b, false, 0});
  }
  {
    auto b = header_bytes(2);  // deux sections annoncees, une seule presente
    append_section(b, 4, 0);
    all.push_back({"section_manquante", b, false, 0});
  }
  return all;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage : mhgp12_format_test <dossier de travail>\n");
    return 2;
  }
  const std::string folder = argv[1];
  int failures = 0, admitted = 0, refused = 0;
  for (const auto& c : cases()) {
    const std::string path = folder + "/" + c.name + ".bin";
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (f == nullptr || std::fwrite(c.bytes.data(), 1, c.bytes.size(), f) != c.bytes.size() || std::fclose(f) != 0) {
      std::fprintf(stderr, "format_test : ecriture impossible de %s\n", path.c_str());
      return 3;
    }
    bool ok = false;
    u64 count = 0;
    try {
      Reader reader(path);
      count = reader.get<u32>("POPVAL").second;
      ok = true;
    } catch (const std::runtime_error&) {
      ok = false;
    }
    std::remove(path.c_str());
    const bool good = ok == c.admitted && (!ok || count == c.popval_count);
    std::printf("%s %s %s\n", c.name.c_str(), ok ? "admis" : "refuse", good ? "conforme" : "ECART");
    failures += !good;
    admitted += ok;
    refused += !ok;
  }
  if (failures != 0) return 1;
  std::printf("format_test_ok admis=%d refus=%d\n", admitted, refused);
  return 0;
}

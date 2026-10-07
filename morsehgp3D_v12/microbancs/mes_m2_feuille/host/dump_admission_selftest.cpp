// Porte d'admission des vidages MHGP12LF (MES-M2, hors produit ; constat CST-0215 de l'auditeur Codex).
//
// Fabrique un vidage synthetique VALIDE (sphere de 32 sites x^2 + y^2 + z^2 = 50 translatee, temoin de l'auditeur, en
// deux feuilles ; emissions et compteurs de reference tires de la forme j3 sur l'hote), puis des MUTANTS ecrits par le
// Writer original (empreinte FNV-1a juste) : chacun viole un seul invariant d'admission. dump::read doit accepter le
// vidage valide et refuser chaque mutant. Les six injections du reçu audit_socle_microbancs_20261007/feuille
// (site_out_of_cloud, wrapped_job_begin, zero_k, profile_mismatch, coordinate_out_of_profile,
// short_record_population) sont rejouees a l'identique sur ce vidage valide ; les autres couvrent le reste du domaine.
// Aucune donnee reelle. Les fichiers restent dans <dossier> pour que la porte Python rejoue aussi les outils
// (mhgp12_leaf_identity, mhgp12_mes_s, mhgp12_arena_selftest) : refus de code 2 avant tout noyau.
//
// Usage : mhgp12_dump_admission_selftest <dossier>
// Sortie : une ligne JSON par cas. Codes : 0 conforme (valide admis, tous les mutants refuses), 1 vidage valide
// refuse, 2 usage ou ecriture, 3 mutant admis (vivant).
#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "mhgp12/leaf/dump_format.hpp"
#include "mhgp12/leaf/leaf_j3.hpp"

namespace {

namespace dump = mhgp12::dump;
namespace leaf = mhgp12::leaf;
using dump::i64;
using dump::u32;
using dump::u64;
using dump::u8;

struct RecordSink {
  std::vector<dump::Record> records;
  std::vector<u8> population;
  void emit(const leaf::Emission& e) {
    dump::Record r{};
    for (int k = 0; k < 4; ++k) r.support[k] = e.support[k];
    r.p = e.p;
    r.m = e.m;
    r.qmin = e.qmin;
    r.pad = 0;
    records.push_back(r);
    for (u32 rest = e.interior; rest != 0; rest &= rest - 1) population.push_back(static_cast<u8>(__builtin_ctz(rest)));
    for (u32 rest = e.shell; rest != 0; rest &= rest - 1) population.push_back(static_cast<u8>(__builtin_ctz(rest)));
  }
};

// Vidage valide : deux feuilles sur la meme sphere de 32 sites (toute la sphere, puis ses 8 premiers sites).
dump::LeafDump valid_dump() {
  std::vector<std::array<u32, 3>> sphere;
  for (int x = -7; x <= 7 && sphere.size() < 32; ++x)
    for (int y = -7; y <= 7 && sphere.size() < 32; ++y)
      for (int z = -7; z <= 7 && sphere.size() < 32; ++z)
        if (x * x + y * y + z * z == 50 && (x > 0 || (x == 0 && (y > 0 || (y == 0 && z > 0))))) {
          sphere.push_back({u32(x + 8), u32(y + 8), u32(z + 8)});
          sphere.push_back({u32(8 - x), u32(8 - y), u32(8 - z)});
        }
  std::sort(sphere.begin(), sphere.end());
  dump::LeafDump d;
  auto& h = d.header;
  h.coord_bits = dump::kProfileBits;
  h.kmax = 5;
  h.leaf_size = 16;
  h.max_leaf = 256;
  h.flags = dump::kFlagCache | dump::kFlagPairGraph;
  for (const auto& p : sphere) {
    d.x.push_back(p[0]);
    d.y.push_back(p[1]);
    d.z.push_back(p[2]);
  }
  const std::array<u32, 2> sizes = {32, 8};
  const std::array<i64, 2> highs = {17, 9};
  d.record_begin.push_back(0);
  d.population_begin.push_back(0);
  auto shared = std::make_unique<leaf::j3::SharedJ3>();
  for (int j = 0; j < 2; ++j) {
    dump::Job job;
    job.begin = d.sites.size();
    job.m = sizes[j];
    for (int a = 0; a < 3; ++a) {
      job.lo[a] = 0;
      job.hi[a] = highs[j];
    }
    for (u32 i = 0; i < job.m; ++i) d.sites.push_back(i);
    d.jobs.push_back(job);
    leaf::Input in;
    in.x = d.x.data();
    in.y = d.y.data();
    in.z = d.z.data();
    in.sites = d.sites.data() + job.begin;
    in.m = job.m;
    for (int a = 0; a < 3; ++a) {
      in.lo[a] = job.lo[a];
      in.hi[a] = job.hi[a];
    }
    in.kmax = static_cast<int>(h.kmax);
    in.cache = true;
    leaf::Counts c;
    RecordSink sink;
    if (leaf::j3::run_leaf(in, *shared, c, sink) != leaf::kStatusOk) throw std::runtime_error("feuille non resolue");
    for (u32 f = 0; f < dump::kCounters; ++f) d.counts.push_back(c.c[f]);
    d.status.push_back(dump::kStatusReference);
    d.records.insert(d.records.end(), sink.records.begin(), sink.records.end());
    d.population.insert(d.population.end(), sink.population.begin(), sink.population.end());
    d.record_begin.push_back(d.records.size());
    d.population_begin.push_back(d.population.size());
  }
  h.n_sites = d.x.size();
  h.n_leaves = d.jobs.size();
  h.n_leaf_sites = d.sites.size();
  h.n_records = d.records.size();
  h.n_population = d.population.size();
  h.walk_leaves = h.n_leaves;
  h.walk_inline_leaves = 0;
  return d;
}

// Premier enregistrement de la feuille 0 ayant au moins une incidence interieure (p > 0).
u64 record_with_interior(const dump::LeafDump& d) {
  for (u64 b = 0; b < d.record_begin[1]; ++b)
    if (d.records[b].p > 0) return b;
  throw std::runtime_error("aucun enregistrement avec interieur");
}
u64 population_of(const dump::LeafDump& d, u64 record) {
  u64 at = 0;
  for (u64 b = 0; b < record; ++b) at += u64(d.records[b].p) + d.records[b].m;
  return at;
}

void retotal(dump::LeafDump& d) {  // en-tete aux tailles des tableaux (un mutant ne casse qu'un invariant)
  d.header.n_sites = d.x.size();
  d.header.n_leaves = d.jobs.size();
  d.header.n_leaf_sites = d.sites.size();
  d.header.n_records = d.records.size();
  d.header.n_population = d.population.size();
}

int run(const std::string& dir) {
  const dump::LeafDump base = valid_dump();
  int code = 0;
  std::string error;
  const std::string base_path = dir + "/valide.bin";
  if (!dump::write(base_path, base, error)) {
    std::cerr << error << '\n';
    return 2;
  }
  {
    dump::LeafDump got;
    const bool ok = dump::read(base_path, got, error);
    std::cout << "{\"cas\":\"valide\",\"admis\":" << (ok ? "true" : "false") << ",\"feuilles\":" << base.header.n_leaves
              << ",\"enregistrements\":" << base.header.n_records << ",\"incidences\":" << base.header.n_population
              << ",\"dump_fnv1a\":\"" << dump::digest_hex(got.digest) << "\"}\n";
    if (!ok) {
      std::cerr << error << '\n';
      code = 1;
    }
  }
  const u64 side = u64{1} << dump::kProfileBits;
  const u64 rec = record_with_interior(base);
  const u64 pop = population_of(base, rec);
  using Change = std::function<void(dump::LeafDump&)>;
  const std::vector<std::pair<const char*, Change>> mutants = {
      // Les six injections de l'auditeur (feuille/REPORT.md), sur un vidage valide.
      {"site_out_of_cloud", [](dump::LeafDump& d) { d.sites[0] = static_cast<u32>(d.header.n_sites); }},
      {"wrapped_job_begin", [](dump::LeafDump& d) { d.jobs[0].begin = ~u64{0}; }},
      {"zero_k", [](dump::LeafDump& d) { d.header.kmax = 0; }},
      {"profile_mismatch", [](dump::LeafDump& d) { d.header.coord_bits = dump::kProfileBits == 21 ? 24 : 21; }},
      {"coordinate_out_of_profile",
       [](dump::LeafDump& d) {
         d.x[0] = 0xFFFFFFFFu;
         d.jobs[0].lo[0] = 0xFFFFFFFFll;
         d.jobs[0].hi[0] = 0x100000000ll;
       }},
      {"short_record_population",
       [](dump::LeafDump& d) {  // un enregistrement exige deux incidences, la plage de la feuille n'en a qu'une
         d.records.assign(1, dump::Record{{0, 1, 0xFF, 0xFF}, 0, 2, 2, 0});
         d.population.assign(1, 0);
         d.record_begin = {0, 1, 1};
         d.population_begin = {0, 1, 1};
         d.counts[dump::kCounters * 0 + 4] = 1;
         d.counts[dump::kCounters * 0 + 5] = 1;
         d.counts[dump::kCounters * 1 + 4] = 0;
         d.counts[dump::kCounters * 1 + 5] = 0;
         retotal(d);
       }},
      // En-tete.
      {"k_13", [](dump::LeafDump& d) { d.header.kmax = 13; }},
      {"feuille_sous_k_plus_3", [](dump::LeafDump& d) { d.header.leaf_size = d.header.kmax + 2; }},
      {"feuille_257", [](dump::LeafDump& d) { d.header.leaf_size = 257; }},
      {"max_leaf_sous_feuille", [](dump::LeafDump& d) { d.header.max_leaf = d.header.leaf_size - 1; }},
      {"drapeau_inconnu", [](dump::LeafDump& d) { d.header.flags |= 4; }},
      {"sans_graphe_de_paires", [](dump::LeafDump& d) { d.header.flags = dump::kFlagCache; }},
      {"parcours_incoherent", [](dump::LeafDump& d) { d.header.walk_inline_leaves = 1; }},
      {"reserve_non_nulle", [](dump::LeafDump& d) { d.header.reserved = 1; }},
      {"taille_en_tete_fausse", [](dump::LeafDump& d) { d.header.n_population += 8; }},
      {"comptes_geants", [](dump::LeafDump& d) { d.header.n_records = u64{1} << 60; }},
      // Nuage, feuilles et boites.
      {"coordonnee_2_puissance_b", [side](dump::LeafDump& d) { d.z[3] = static_cast<u32>(side); }},
      {"feuille_vide", [](dump::LeafDump& d) {
         d.jobs[1].m = 0;
         d.jobs[1].begin = 32;
       }},
      {"feuille_33_sites", [](dump::LeafDump& d) { d.jobs[0].m = 33; }},
      {"pavage_trou", [](dump::LeafDump& d) { d.jobs[1].begin = 31; }},
      {"remplissage_feuille", [](dump::LeafDump& d) { d.jobs[0].pad = 1; }},
      {"boite_vide", [](dump::LeafDump& d) { d.jobs[1].hi[2] = d.jobs[1].lo[2]; }},
      {"boite_negative", [](dump::LeafDump& d) { d.jobs[0].lo[1] = -1; }},
      {"boite_au_dela_de_2_puissance_b", [side](dump::LeafDump& d) { d.jobs[0].hi[0] = static_cast<i64>(side) + 1; }},
      {"sites_non_croissants", [](dump::LeafDump& d) { std::swap(d.sites[4], d.sites[5]); }},
      {"site_repete", [](dump::LeafDump& d) { d.sites[7] = d.sites[6]; }},
      // Statuts et compteurs.
      {"statut_sans_reference", [](dump::LeafDump& d) { d.status[1] = dump::kStatusV11DeviceUnresolved; }},
      {"statut_ecart_temoin_v11", [](dump::LeafDump& d) { d.status[0] |= dump::kStatusV11DeviceMismatch; }},
      {"statut_bit_inconnu", [](dump::LeafDump& d) { d.status[0] |= 8; }},
      {"compteur_emitted", [](dump::LeafDump& d) { d.counts[4] += 1; }},
      {"compteur_incidences", [](dump::LeafDump& d) { d.counts[5] -= 1; }},
      // Debuts.
      {"debut_enregistrements_non_nul", [](dump::LeafDump& d) {
         d.record_begin[0] = 1;
       }},
      {"debuts_non_monotones", [](dump::LeafDump& d) { d.record_begin[1] = d.record_begin[2] + 1; }},
      // Enregistrements et populations.
      {"qmin_5", [rec](dump::LeafDump& d) { d.records[rec].qmin = 5; }},
      {"support_hors_feuille", [rec](dump::LeafDump& d) {  // dernier site de S* : rang 32 dans une feuille de 32
         auto& r = d.records[rec];
         r.support[r.qmin - 1] = 32;
       }},
      {"support_non_croissant",
       [rec](dump::LeafDump& d) { std::swap(d.records[rec].support[0], d.records[rec].support[1]); }},
      {"support_mal_termine", [rec](dump::LeafDump& d) {
         auto& r = d.records[rec];
         r.support[3] = r.qmin < 4 ? 0 : r.support[3];
         if (r.qmin == 4) r.qmin = 3;
       }},
      {"p_plus_qmin_au_dela_de_k_plus_1", [rec](dump::LeafDump& d) {
         const auto& r = d.records[rec];
         d.header.kmax = r.p + r.qmin - 2;  // p + qmin = K + 2 ; K reste dans 1..12 et la feuille >= K + 3
       }},
      {"population_non_croissante",
       [rec, pop](dump::LeafDump& d) {
         const auto& r = d.records[rec];
         if (r.m >= 2) std::swap(d.population[pop + r.p], d.population[pop + r.p + 1]);
       }},
      {"population_hors_feuille", [pop](dump::LeafDump& d) { d.population[pop] = 40; }},
      {"interieur_dans_coquille", [rec, pop](dump::LeafDump& d) {
         const auto& r = d.records[rec];
         d.population[pop + r.p - 1] = d.population[pop + r.p + r.m - 1];  // dernier interieur = dernier de U
       }},
      {"coquille_sans_s_etoile", [rec](dump::LeafDump& d) {
         auto& r = d.records[rec];
         // S* remplace par un site interieur : S* n'est plus dans U (I et U restent disjoints et croissants).
         r.support[0] = d.population[population_of(d, rec)];
         std::sort(r.support, r.support + r.qmin);
       }},
      {"enregistrement_remplissage", [rec](dump::LeafDump& d) { d.records[rec].pad = 1; }},
      {"incidences_en_trop", [](dump::LeafDump& d) {  // feuille 1 : une incidence de plus, comptes ajustes
         d.population.push_back(0);
         d.population_begin[2] += 1;
         d.counts[dump::kCounters + 5] += 1;
         retotal(d);
       }},
  };
  for (const auto& [name, change] : mutants) {
    dump::LeafDump m = base;
    change(m);
    const std::string path = dir + "/" + name + ".bin";
    if (!dump::write(path, m, error)) {  // le Writer original accepte tout vidage aux tailles de son en-tete
      if (std::string(name) == "taille_en_tete_fausse" || std::string(name) == "comptes_geants") {
        // en-tete et tableaux discordants : le Writer refuse ; on ecrit l'en-tete altere a la main.
        dump::LeafDump same = base;
        if (!dump::write(path, same, error)) return 2;
        std::FILE* f = std::fopen(path.c_str(), "r+b");
        if (f == nullptr) return 2;
        const bool ok = std::fwrite(&m.header, 1, sizeof(m.header), f) == sizeof(m.header);
        if (std::fclose(f) != 0 || !ok) return 2;
      } else {
        std::cerr << error << '\n';
        return 2;
      }
    }
    dump::LeafDump got;
    std::string why;
    const bool admitted = dump::read(path, got, why);
    std::string reason;
    for (char c : why) reason += (c == '"' || c == '\\') ? '\'' : c;
    const std::size_t at = reason.rfind(" : ");
    if (at != std::string::npos) reason.resize(at);  // sans le chemin
    std::cout << "{\"cas\":\"" << name << "\",\"admis\":" << (admitted ? "true" : "false") << ",\"raison\":\""
              << reason << "\",\"verdict\":\"" << (admitted ? "VIVANT" : "refuse") << "\"}\n";
    if (admitted && code == 0) code = 3;
  }
  // Fichier tronque et octets en trop : empreinte ou taille, refus avant toute allocation dependante des comptes.
  for (const char* name : {"tronque", "octet_en_trop"}) {
    const std::string path = dir + "/" + name + ".bin";
    if (!dump::write(path, base, error)) return 2;
    std::FILE* f = std::fopen(path.c_str(), "r+b");
    if (f == nullptr) return 2;
    bool ok = true;
    if (std::string(name) == "octet_en_trop") {
      ok = std::fseek(f, 0, SEEK_END) == 0 && std::fputc(0, f) != EOF;
    }
    if (std::fclose(f) != 0 || !ok) return 2;
    if (std::string(name) == "tronque") {
      std::FILE* g = std::fopen(path.c_str(), "rb");
      if (g == nullptr) return 2;
      std::vector<char> bytes;
      for (int c = std::fgetc(g); c != EOF; c = std::fgetc(g)) bytes.push_back(static_cast<char>(c));
      std::fclose(g);
      bytes.resize(bytes.size() - 9);
      g = std::fopen(path.c_str(), "wb");
      if (g == nullptr || std::fwrite(bytes.data(), 1, bytes.size(), g) != bytes.size() || std::fclose(g) != 0)
        return 2;
    }
    dump::LeafDump got;
    std::string why;
    const bool admitted = dump::read(path, got, why);
    std::cout << "{\"cas\":\"" << name << "\",\"admis\":" << (admitted ? "true" : "false") << ",\"verdict\":\""
              << (admitted ? "VIVANT" : "refuse") << "\"}\n";
    if (admitted && code == 0) code = 3;
  }
  return code;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  try {
    return run(argv[1]);
  } catch (const std::exception& e) {
    std::cerr << "refus : " << e.what() << '\n';
    return 2;
  }
}

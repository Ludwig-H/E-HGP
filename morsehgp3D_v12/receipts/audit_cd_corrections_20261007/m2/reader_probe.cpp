// Une seule TU Release : vrai lecteur et vrai point d'entree de leaf_identity.cpp.
// Les fichiers rejetes sont relus par l'outil seulement APRES verification de leur refus par read().
#define main audit_original_identity_main
#include "leaf_identity.cpp"
#undef main

namespace audit_dump = mhgp12::dump;

void audit_check(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

audit_dump::LeafDump audit_base(bool historical = false) {
  audit_dump::LeafDump d;
  auto& h = d.header;
  h.coord_bits = 21; h.kmax = 1; h.leaf_size = historical ? 1 : 4; h.max_leaf = 256; h.flags = 3;
  h.n_sites = h.n_leaves = h.n_leaf_sites = h.walk_leaves = 1;
  d.x = d.y = d.z = {1}; d.sites = {0}; d.jobs.resize(1); d.jobs[0].m = 1;
  for (int a = 0; a < 3; ++a) d.jobs[0].hi[a] = 2;
  d.counts.resize(15); d.counts[1] = 1; d.status = {1}; d.record_begin = {0, 0}; d.population_begin = {0, 0};
  return d;
}

audit_dump::LeafDump audit_pair() {
  auto d = audit_base();
  d.header.n_sites = d.header.n_leaf_sites = 2;
  d.header.n_records = 1; d.header.n_population = 2;
  d.x = {0, 2}; d.y = d.z = {0, 0}; d.sites = {0, 1}; d.jobs[0].m = 2; d.jobs[0].hi[0] = 3;
  d.records = {{{0, 1, 255, 255}, 0, 2, 2, 0}};
  d.record_begin = {0, 1}; d.population_begin = {0, 2}; d.population = {0, 1};
  d.counts[4] = 1; d.counts[5] = 2;
  return d;
}

int audit_identity(const std::vector<std::string>& paths, std::string& output) {
  std::vector<std::string> args = {"leaf_identity", "--threads", "1"};
  args.insert(args.end(), paths.begin(), paths.end());
  std::vector<char*> argv;
  for (auto& s : args) argv.push_back(s.data());
  std::ostringstream stdout_capture, stderr_capture;
  auto* out = std::cout.rdbuf(stdout_capture.rdbuf());
  auto* err = std::cerr.rdbuf(stderr_capture.rdbuf());
  const int code = audit_original_identity_main(static_cast<int>(argv.size()), argv.data());
  std::cout.rdbuf(out); std::cerr.rdbuf(err);
  output = stdout_capture.str();
  return code;
}

int main(int argc, char** argv) {
  audit_check(argc == 2, "temporary directory required");
  const std::string directory = argv[1];
  std::string error;
  const std::string valid_path = directory + "/valid.bin";
  audit_check(audit_dump::write(valid_path, audit_base(), error), "write valid");
  audit_dump::LeafDump valid;
  audit_check(audit_dump::read(valid_path, valid, error), "valid refused: " + error);
  std::string output;
  const int valid_code = audit_identity({valid_path}, output);
  audit_check(valid_code == 0 && output.find("\"form\":\"j3\"") != std::string::npos &&
              output.find("\"form\":\"coherent\"") != std::string::npos &&
              output.find("\"identity\":false") == std::string::npos, "valid native identity");
  std::cout << "valid native_code=0 forms=j3,coherent identity=true\n";
  audit_check(audit_dump::write(directory + "/pair.bin", audit_pair(), error), "write pair");
  audit_dump::LeafDump pair;
  audit_check(audit_dump::read(directory + "/pair.bin", pair, error), "pair domain refused: " + error);
  std::cout << "valid_pair reader=1\n";
  const char* names[] = {"site_out_of_cloud", "wrapped_job_begin", "zero_k", "profile_mismatch",
                         "coordinate_out_of_profile", "short_record_population"};
  const char* reasons[] = {"site hors du nuage", "pavage", "K = 0", "profil 24", "coordonnee",
                           "incidences de l'enregistrement"};
  for (int historical = 1; historical >= 0; --historical) {
    for (int mutation = 0; mutation < 6; ++mutation) {
      auto d = audit_base(historical != 0);
      if (mutation == 0) d.sites[0] = 1;
      if (mutation == 1) d.jobs[0].begin = ~audit_dump::u64{0};
      if (mutation == 2) d.header.kmax = 0;
      if (mutation == 3) d.header.coord_bits = 24;
      if (mutation == 4) {
        d.x[0] = 0xFFFFFFFFu; d.jobs[0].lo[0] = 0xFFFFFFFFll; d.jobs[0].hi[0] = 0x100000000ll;
      }
      if (mutation == 5) {
        if (historical) {
          d.header.n_records = d.header.n_population = 1;
          d.records = {{{0, 0, 255, 255}, 0, 2, 2, 0}};
          d.record_begin = d.population_begin = {0, 1}; d.population = {0};
        } else {
          d = audit_pair(); d.header.n_population = 1; d.population = {0};
          d.population_begin = {0, 1}; d.counts[5] = 1;
        }
      }
      const std::string name = std::string(historical ? "historical_" : "causal_") + names[mutation];
      const std::string path = directory + "/" + name + ".bin";
      audit_check(audit_dump::write(path, d, error), "write " + name);
      audit_dump::LeafDump got;
      audit_check(!audit_dump::read(path, got, error), "reader admitted " + name);
      if (!historical) audit_check(error.find(reasons[mutation]) != std::string::npos, "wrong refusal cause: " + name + error);
      const int native = audit_identity({path}, output);
      audit_check(native == 2 && output.find("\"form\"") == std::string::npos, "kernel reached on " + name);
      const int mixed = audit_identity({valid_path, path}, output);
      audit_check(mixed == 2 && output.find("\"form\"") == std::string::npos, "partial calculation before " + name);
      std::cout << name << " reader=0 native_code=2 mixed_code=2 result_lines=0\n";
    }
  }
  // Nouveau controle de taille avant allocation et refus de domaines adjacents.
  for (int i = 0; i < 4; ++i) {
    auto d = audit_base();
    const char* name = "huge_header";
    if (i == 0) d.header.n_records = audit_dump::u64{1} << 60;
    if (i == 1) { name = "status_without_reference"; d.status[0] = 0; }
    if (i == 2) { name = "counter_mismatch"; d.counts[4] = 1; }
    if (i == 3) { name = "duplicate_population"; d = audit_pair(); d.population[1] = 0; }
    const std::string path = directory + "/" + name + ".bin";
    if (i == 0) {
      // En-tete geant, fichier physique minuscule, FNV des octets presents valide.
      auto* file = std::fopen(path.c_str(), "wb");
      audit_check(file != nullptr, "open huge-header fixture");
      const auto hash = audit_dump::fnv1a(audit_dump::kFnvBasis, &d.header, sizeof(d.header));
      audit_check(std::fwrite(&d.header, sizeof(d.header), 1, file) == 1 &&
                  std::fwrite(&hash, sizeof(hash), 1, file) == 1 && std::fclose(file) == 0, "write huge header");
    } else {
      audit_check(audit_dump::write(path, d, error), "write adjacent");
    }
    audit_dump::LeafDump got;
    audit_check(!audit_dump::read(path, got, error), std::string("adjacent accepted: ") + name);
    if (i == 0) audit_check(got.x.empty() && got.jobs.empty() && got.records.empty(), "allocation before size check");
    std::cout << name << " reader=0\n";
  }
}

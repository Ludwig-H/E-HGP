// MorseHGP3D v5 — fixture des familles : digests SHA-256 GRAVES des nuages
// (famille, n, coord par defaut, graine 3). Ils ont ete calcules par le
// generateur v4 (`morsehgp3D_v4/src/cloud/families.hpp`) avec la meme
// serialisation : c'est la preuve du port bit a bit. Codes : 0 conforme,
// 3 digest different, 4 mutant tue (cardinalite).
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/core/sha256.hpp"

using namespace mhgp5;

static std::string cloud_digest(const std::vector<P3>& pts) {
  Sha256 h;
  h.tag("cloud");
  h.u64le(pts.size());
  for (const P3& p : pts) {
    h.i64le(p.x);
    h.i64le(p.y);
    h.i64le(p.z);
  }
  return h.hex();
}

struct Expected {
  const char* family;
  int n;
  const char* digest;
};

// Graves depuis la v4 (receipts/conformite_v4/familles_v4.txt).
static const Expected kExpected[] = {
    {"uniform", 8000, "29b92e845eb453fb719dc4d881d28b330e372c1d9fd81cdb88dfb57df5e2712a"},
    {"terrain", 8000, "fb72ea8ac7b754a166ea3940caab7c9459cddeb5d5debac10006234c6c83e3fc"},
    {"eight_clusters", 8000, "db3b1e4578cb321d537be319102863cf41a49682a5bb8bf057fe391a4cb5c362"},
    {"scanline_single_pass", 8000, "9cae906ee9a10a2b4c1b9d4529af850b52d36546ebba0f02b78394d0102088ad"},
    {"scanline_overlap_multiecho", 8000, "a73402e2c0fd00423eaef770022159b698e3141554a13d26e50984a6a9f20c63"},
    {"uniform", 32000, "fbadf10d3de34343c24913faacfdcb878a0dd2860bb18d782c1bff496e7ec8be"},
    {"terrain", 32000, "c52f85211a2063fccb7bc0c8beb7aeddfd8dd2abe40947d53fa965201a3ab939"},
    {"eight_clusters", 32000, "62cda521131f2faa7dc5012a6669bfd35de8d62b5596670699e70dbda92935c3"},
    {"scanline_single_pass", 32000, "e56db25cb02b3436731f143b79724934d42a102c71e4c5204542e0e5cb4c9351"},
    {"scanline_overlap_multiecho", 32000, "2ef7254fc6df732c3996f5c30b007a78c1bc2c04be0a9685a6aeb88c5fb5b4c1"},
    {"two_lines", 2000, "8aabdf091aa948d890d4e3884e10e6a1ef30abb83d35c0211f390c9cd31fa6e9"},
    {"collinear_seven", 9, "191b0983bb7eb99d873510fb45b248dfffaf0f8337e7c85afc770e146c54af75"},
};

int main(int argc, char** argv) {
  bool print_only = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--print") == 0) print_only = true;
    else if (std::strncmp(argv[i], "--inject=", 9) == 0) {
      if (!mutants_enable(argv[i] + 9)) return 2;
    } else return 2;
  }
  int bad = 0;
  for (const Expected& e : kExpected) {
    CloudFamily f;
    if (!parse_cloud_family(e.family, &f)) return 2;
    const int coord = cloud_family_default_coord(f, e.n);
    const std::vector<P3> pts = make_family_cloud(f, e.n, coord, 3);
    const std::string d = cloud_digest(pts);
    if (print_only) {
      std::printf("%s n=%d coord=%d points=%zu digest=%s\n", e.family, e.n, coord, pts.size(), d.c_str());
      continue;
    }
    // Contrat de cardinalite : jamais plus de n points (mutant overshoot).
    if (pts.size() > (size_t)e.n) {
      std::fprintf(stderr, "MUTANT TUE : %s n=%d a produit %zu points\n", e.family, e.n, pts.size());
      return 4;
    }
    if (d != e.digest) {
      std::fprintf(stderr, "DIGEST DIFFERENT : %s n=%d attendu %s lu %s\n", e.family, e.n, e.digest, d.c_str());
      ++bad;
    }
  }
  if (print_only) return 0;
  if (bad) return 3;
  std::printf("families_fixture OK (%zu nuages)\n", sizeof(kExpected) / sizeof(kExpected[0]));
  return 0;
}

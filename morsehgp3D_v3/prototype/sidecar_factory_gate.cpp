// MorseHGP3D v3 — LA PORTE DE LA FACTORY `ValidatedHybridSidecar`.
//
// Les fixtures du contrat du 10 aout, chacune avec son refus EXACT :
//
//   1. deux handles de la boule singleton -> refus « deux handles » ;
//   2. colineaires 0,1,2, boule portee par {0,2}, membres declares {0,2} ->
//      saturation incomplete refusee (le point du milieu est interieur) ;
//   3. deux boules CONCENTRIQUES de rayons distincts -> toutes deux
//      acceptees (la cle exacte porte centre ET rayon) ;
//   4. carre cocirculaire -> support alternatif, NON-PRINCIPAL certifie,
//      temoin d'egalite indexe par le PointId supprime ;
//   5. paire diametrale + point de coquille -> PRINCIPAL certifie, trois
//      temoins stricts ;
//   6. carrier supprime mais recu conserve -> la fermeture reste kUnknown
//      (digest desynchronise), jamais un certificat invente ;
//   7. den=0, support hors coquille, pool chevauche -> refus avant index.
//
// Mutants de la factory (code 4) : oubli du dernier u (skip-last-removal),
// « < » change en « <= » (strict-leq), temoin indexe par POSITION au lieu du
// PointId (witness-by-position). Codes : 0 OK ; 1 ecart ; 2 CLI ; 4 mutant
// tue.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"
#include "prototype/sealed_source.hpp"
#include "prototype/validated_hybrid_sidecar.hpp"

namespace {

// Une sphere exacte depuis un point de coquille `base` et le vecteur entier
// vers le centre : centre = base + n/den, rayon^2 = |n|^2/den^2. Le champ
// interne `support` (taille 1..4) doit etre COHERENT avec le n_support
// declare — la factory le verifie (audit S3).
mhgp::Sphere make_sphere(mhgp::P3 base, mhgp::i32 nx, mhgp::i32 ny, mhgp::i32 nz,
                         mhgp::i32 den, int support_size) {
  mhgp::Sphere sphere{};
  sphere.base = base;
  sphere.nx = nx;
  sphere.ny = ny;
  sphere.nz = nz;
  sphere.den = den;
  sphere.support = support_size;
  return sphere;
}

struct SyntheticGenerator {
  mhgp::Sphere sphere;
  std::vector<mhgp::i32> members;
  std::vector<mhgp::i32> support;
};

mhgp::Catalogue make_catalogue(const std::vector<SyntheticGenerator>& generators) {
  mhgp::Catalogue catalogue;
  for (const SyntheticGenerator& g : generators) {
    mhgp::CriticalSphere sphere{};
    sphere.sph = g.sphere;
    sphere.rank = (mhgp::i32)g.members.size();
    sphere.members_begin = (mhgp::i32)catalogue.members.size();
    sphere.n_support = (mhgp::i32)g.support.size();
    for (std::size_t u = 0; u < g.support.size() && u < 4; ++u)
      sphere.support[u] = g.support[u];
    catalogue.members.insert(catalogue.members.end(), g.members.begin(), g.members.end());
    catalogue.spheres.push_back(sphere);
  }
  return catalogue;
}

}  // namespace

int main(int argc, char** argv) {
  const char* mutant_name = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--mutant") && i + 1 < argc) {
      mutant_name = argv[++i];
      continue;
    }
    std::printf("ECHEC : argument inconnu %s\n", argv[i]);
    return 2;
  }
  mhgp3v::SidecarMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "skip-last-removal")) mutants.skip_last_removal = true;
    else if (!strcmp(mutant_name, "strict-leq")) mutants.strict_leq = true;
    else if (!strcmp(mutant_name, "witness-by-position")) mutants.witness_by_position = true;
    else { std::printf("ECHEC : mutant inconnu %s\n", mutant_name); return 2; }
  }
  const auto kill = [&](const std::string& where, const std::string& why) {
    if (mutant_name != nullptr) {
      std::printf("mutant tue par %s : %s\n", where.c_str(), why.c_str());
      return 4;
    }
    std::printf("ECHEC %s : %s\n", where.c_str(), why.c_str());
    return 1;
  };
  std::string refusal;

  // FIXTURE 1 : deux handles de la meme boule singleton.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{5, 5, 5}};
    const mhgp::Sphere singleton = make_sphere(cloud[0], 0, 0, 0, 1, 1);
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{singleton, {0}, {0}}, {singleton, {0}, {0}}}), nullptr, 3,
        &refusal, mutants);
    if (sidecar.ok() || refusal.find("deux handles") == std::string::npos)
      return kill("la fixture des deux handles",
                  sidecar.ok() ? "acceptee a tort" : refusal);
  }
  // FIXTURE 2 : colineaires, saturation incomplete (le milieu est interieur).
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{0, 0, 0}, mhgp::P3{1, 0, 0},
                                         mhgp::P3{2, 0, 0}};
    const mhgp::Sphere diametral = make_sphere(cloud[0], 1, 0, 0, 1, 2);
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{diametral, {0, 2}, {0, 2}}}), nullptr, 3, &refusal, mutants);
    if (sidecar.ok() || refusal.find("saturation incomplete") == std::string::npos)
      return kill("la fixture de la saturation incomplete",
                  sidecar.ok() ? "acceptee a tort" : refusal);
  }
  // FIXTURE 3 : deux boules concentriques de rayons distincts, acceptees.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{1, 2, 0}, mhgp::P3{3, 2, 0},
                                         mhgp::P3{0, 2, 0}, mhgp::P3{4, 2, 0}};
    const mhgp::Sphere inner = make_sphere(cloud[0], 1, 0, 0, 1, 2);
    const mhgp::Sphere outer = make_sphere(cloud[2], 2, 0, 0, 1, 2);
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{inner, {0, 1}, {0, 1}}, {outer, {0, 1, 2, 3}, {2, 3}}}),
        nullptr, 3, &refusal, mutants);
    if (!sidecar.ok())
      return kill("la fixture des concentriques", "refusees a tort : " + refusal);
    // Le PRINCIPAL de la boule externe (support {2,3} != positions {0,1}) :
    // les temoins doivent etre indexes par le PointId SUPPRIME — le mutant
    // par position rend 0/1 et meurt ici.
    const mhgp3v::GeneratorCertificate& outer_certificate = sidecar.generators()[1];
    if (outer_certificate.principal != mhgp3v::PrincipalState::kPrincipalCertified)
      return kill("la fixture des concentriques", "le principal externe n'est pas certifie");
    for (int e = 0; e < (int)outer_certificate.evidence_count; ++e) {
      const mhgp::i32 removed = outer_certificate.evidence[(std::size_t)e].removed;
      if (removed != 2 && removed != 3)
        return kill("la fixture des concentriques",
                    "temoin indexe par position au lieu du PointId supprime");
    }
  }
  // FIXTURE 4 : carre cocirculaire — NON-PRINCIPAL certifie, temoin d'egalite
  // indexe par le PointId supprime.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0},
                                         mhgp::P3{1, 2, 0}, mhgp::P3{1, 0, 0}};
    const mhgp::Sphere circle = make_sphere(cloud[0], 1, 0, 0, 1, 2);
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{circle, {0, 1, 2, 3}, {0, 1}}}), nullptr, 3, &refusal,
        mutants);
    if (!sidecar.ok())
      return kill("la fixture du carre cocirculaire", "refusee a tort : " + refusal);
    const mhgp3v::GeneratorCertificate& certificate = sidecar.generators()[0];
    if (certificate.principal != mhgp3v::PrincipalState::kNonPrincipalCertified)
      return kill("la fixture du carre cocirculaire",
                  "le support alternatif n'a pas ete certifie non-principal");
    bool witness_ok = certificate.evidence_count >= 1;
    for (int e = 0; e < (int)certificate.evidence_count; ++e) {
      const mhgp3v::RemovalEvidence& evidence = certificate.evidence[(std::size_t)e];
      if (evidence.removed != 0 && evidence.removed != 1) witness_ok = false;
      if (evidence.relation == mhgp3v::RemovalEvidence::Relation::kEqual) {
        // Le support alternatif doit EXCLURE le point supprime.
        for (int w = 0; w < (int)evidence.witness.size; ++w)
          if (evidence.witness.ids[(std::size_t)w] == evidence.removed) witness_ok = false;
      }
    }
    if (!witness_ok)
      return kill("la fixture du carre cocirculaire",
                  "temoin d'egalite mal indexe (PointId supprime exige)");
  }
  // FIXTURE 5 : diametre + point de coquille — PRINCIPAL certifie.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0},
                                         mhgp::P3{1, 2, 0}};
    const mhgp::Sphere circle = make_sphere(cloud[0], 1, 0, 0, 1, 2);
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{circle, {0, 1, 2}, {0, 1}}}), nullptr, 3, &refusal, mutants);
    if (!sidecar.ok())
      return kill("la fixture du diametre", "refusee a tort : " + refusal);
    if (sidecar.generators()[0].principal != mhgp3v::PrincipalState::kPrincipalCertified)
      return kill("la fixture du diametre", "le principal n'a pas ete certifie");
  }
  // FIXTURE 6 : recu du PRODUCTEUR SCELLE (audit S1 : la forge fraiche est
  // refusee a la compilation, seul le producteur terminal detient le jeton) ;
  // reemploye sur la table amputee, la fermeture reste kUnknown.
  {
    const std::vector<mhgp::P3> two_triangles = {
        mhgp::P3{6, 10, 0}, mhgp::P3{14, 10, 0}, mhgp::P3{10, 18, 0}, mhgp::P3{10, 2, 0},
        mhgp::P3{0, 0, 20}};
    const mhgp3v::SealedSourceProducer::Result sealed =
        mhgp3v::SealedSourceProducer::run(two_triangles, 5);
    if (!sealed.ok || sealed.receipt.empty()) {
      std::printf("ECHEC : producteur scelle refuse sur les deux triangles\n");
      return 1;
    }
    // Table complete + recu scelle : fermeture certifiee.
    const auto certified = mhgp3v::ValidatedHybridSidecar::build(
        two_triangles, sealed.catalogue, &sealed.receipt[0], 2, &refusal, mutants);
    if (!certified.ok() || !certified.closure_certified_all_orders())
      return kill("la fixture du recu", "la fermeture scellee n'a pas ete certifiee : " +
                                            refusal);
    // Table AMPUTEE du carrier AB + le MEME recu : digest desynchronise, la
    // fermeture reste kUnknown — jamais inventee.
    mhgp::Catalogue doctored;
    for (const mhgp::CriticalSphere& sphere : sealed.catalogue.spheres) {
      const bool is_ab =
          sphere.rank == 2 &&
          sealed.catalogue.members[(std::size_t)sphere.members_begin] == 0 &&
          sealed.catalogue.members[(std::size_t)sphere.members_begin + 1] == 1;
      if (is_ab) continue;
      mhgp::CriticalSphere copy = sphere;
      copy.members_begin = (mhgp::i32)doctored.members.size();
      doctored.members.insert(
          doctored.members.end(), sealed.catalogue.members.begin() + sphere.members_begin,
          sealed.catalogue.members.begin() + sphere.members_begin + sphere.rank);
      doctored.spheres.push_back(copy);
    }
    const auto amputated = mhgp3v::ValidatedHybridSidecar::build(
        two_triangles, doctored, &sealed.receipt[0], 2, &refusal, mutants);
    if (!amputated.ok())
      return kill("la fixture du recu", "table amputee refusee pour une autre raison : " +
                                            refusal);
    if (amputated.closure_certified_all_orders())
      return kill("la fixture du recu",
                  "la fermeture a ete certifiee sur une table desynchronisee");
  }
  // FIXTURE 8 (audit S3) : support REDONDANT cospherique — les quatre points
  // du carre declares support q=4 : le support canonique recalcule est la
  // paire diametrale, le cardinal differe, refus.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0},
                                         mhgp::P3{1, 2, 0}, mhgp::P3{1, 0, 0}};
    const mhgp::Sphere circle = make_sphere(cloud[0], 1, 0, 0, 1, 4);
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{circle, {0, 1, 2, 3}, {0, 1, 2, 3}}}), nullptr, 3, &refusal,
        mutants);
    if (sidecar.ok() || refusal.find("support") == std::string::npos)
      return kill("la fixture du support redondant",
                  sidecar.ok() ? "certifiee a tort" : refusal);
  }
  // FIXTURE 9 (audit S3) : champ `Sphere.support` incoherent avec n_support.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0},
                                         mhgp::P3{1, 2, 0}};
    const mhgp::Sphere circle = make_sphere(cloud[0], 1, 0, 0, 1, 3);   // ment : 3
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{circle, {0, 1, 2}, {0, 1}}}), nullptr, 3, &refusal, mutants);
    if (sidecar.ok() || refusal.find("incoherent") == std::string::npos)
      return kill("la fixture du champ support",
                  sidecar.ok() ? "acceptee a tort" : refusal);
  }
  // FIXTURE 10 (audit S3) : support NON CANONIQUE {0,2} la ou le recalcul
  // independant rend la paire diametrale {0,1} — refus, jamais un certificat.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0},
                                         mhgp::P3{1, 2, 0}};
    const mhgp::Sphere circle = make_sphere(cloud[0], 1, 0, 0, 1, 2);
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{circle, {0, 1, 2}, {0, 2}}}), nullptr, 3, &refusal, mutants);
    if (sidecar.ok() || refusal.find("support") == std::string::npos)
      return kill("la fixture du support non canonique",
                  sidecar.ok() ? "acceptee a tort" : refusal);
  }
  // FIXTURE 11 (audit S2) : la refutation u16 aux numerateurs geants — la cle
  // de centre seule (sans carre) doit accepter SANS deborder ; la campagne
  // sanitizers juge l'absence d'UB.
  {
    const std::vector<mhgp::P3> wide = {mhgp::P3{32767, 32767, 0}, mhgp::P3{57863, 57862, 0},
                                        mhgp::P3{7672, 7673, 0}, mhgp::P3{60104, 30135, 1}};
    const mhgp3v::SealedSourceProducer::Result sealed =
        mhgp3v::SealedSourceProducer::run(wide, 4);
    if (!sealed.ok || sealed.receipt.empty()) {
      std::printf("ECHEC : producteur scelle refuse la refutation u16\n");
      return 1;
    }
    const auto sidecar = mhgp3v::ValidatedHybridSidecar::build(
        wide, sealed.catalogue, &sealed.receipt[0], 3, &refusal, mutants);
    if (!sidecar.ok())
      return kill("la fixture des numerateurs geants",
                  "la refutation u16 a ete refusee : " + refusal);
  }
  // FIXTURE 7 : den=0, support hors coquille, pool chevauche — refus.
  {
    const std::vector<mhgp::P3> cloud = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0}};
    mhgp::Sphere bad = make_sphere(cloud[0], 1, 0, 0, 1, 2);
    bad.den = 0;
    const auto zero_den = mhgp3v::ValidatedHybridSidecar::build(
        cloud, make_catalogue({{bad, {0, 1}, {0, 1}}}), nullptr, 2, &refusal, mutants);
    if (zero_den.ok() || refusal.find("den") == std::string::npos)
      return kill("la fixture den=0", zero_den.ok() ? "acceptee a tort" : refusal);
    const std::vector<mhgp::P3> cloud3 = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0},
                                          mhgp::P3{1, 2, 0}};
    const mhgp::Sphere circle = make_sphere(cloud3[0], 1, 0, 0, 1, 2);
    // SUPPORT HORS COQUILLE : declarer un support contenant un point
    // STRICTEMENT interieur — cloud4 ajoute (1,1,0), centre de la boule.
    const std::vector<mhgp::P3> cloud4 = {mhgp::P3{0, 1, 0}, mhgp::P3{2, 1, 0},
                                          mhgp::P3{1, 2, 0}, mhgp::P3{1, 1, 0}};
    const auto off_shell = mhgp3v::ValidatedHybridSidecar::build(
        cloud4, make_catalogue({{circle, {0, 1, 2, 3}, {0, 3}}}), nullptr, 2, &refusal,
        mutants);
    if (off_shell.ok() || refusal.find("coquille") == std::string::npos)
      return kill("la fixture du support hors coquille",
                  off_shell.ok() ? "acceptee a tort" : refusal);
    mhgp::Catalogue overlapped = make_catalogue({{circle, {0, 1, 2}, {0, 1}}});
    overlapped.spheres[0].members_begin = 1;   // tranche decalee : trou au debut
    const auto bad_pool = mhgp3v::ValidatedHybridSidecar::build(
        cloud3, overlapped, nullptr, 2, &refusal, mutants);
    if (bad_pool.ok() || refusal.find("pool") == std::string::npos)
      return kill("la fixture du pool", bad_pool.ok() ? "acceptee a tort" : refusal);
  }

  if (mutant_name != nullptr) {
    std::printf("MUTANT SURVIVANT %s : la porte ne mord pas\n", mutant_name);
    return 0;
  }
  std::printf("OK : factory ValidatedHybridSidecar — sept fixtures du contrat recues,"
              " fermeture liee par digests, certificats principaux par temoins\n");
  return 0;
}

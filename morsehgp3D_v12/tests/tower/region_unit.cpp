// Porte native et deterministe de terminaison de la region de la Session recouverte (CST-0241 ; protocole de
// l'auditeur Codex du 8 octobre, a_terminaison_porte), hors produit. Cette cible compile src/tower/pipeline_run.cpp
// avec MHGP12_REGION_HOOKS : trois points d'observation dans le VRAI corps de run_region (annonce, juste apres
// l'annonce ; retrait sans travail, juste apres le retrait et la capture de last ; attente, juste avant la boucle
// d'attente), fournis ici. Le marqueur region_hooks_build prouve que ce corps instrumente est lie ; la porte de la carte
// de lien (region_map_check.py) que le membre pipeline_run.cpp.o de l'archive mhgp12 ne l'est pas et que l'archive ne
// porte aucun crochet.
//   terminaison  Session d'un site, K1, Pool de 2, preparee comme dans pipeline_unit.cpp ; run_region joue une fois
//                seul, crochets desarmes, jusqu'a la coupe finale (toute etape terminee, g_next >= g_total, in_flight
//                nul). Puis deux fils du harnais, A (fil 0) et B (fil 1), appellent run_region, ordonnes par
//                semaphores : A suspendu apres son retrait (in_flight 1 -> 0), B suspendu apres son annonce (0 -> 1),
//                A relache ; regle corrigee : A sort (son retrait etait le dernier) ; ancienne regle : A relit 1 et
//                attend. B relache sort ; A, s'il attendait, relache, voit le compte nul et sort. Assertion finale :
//                attente_A == 0, retours_A == 1, retours_B == 1, in_flight nul. L'ancienne regle echoue par cette
//                assertion precise (attente_A = 1), apres la liberation et la jointure des deux fils ; aucune attente
//                du harnais n'est bornee : seul le delai CTest garde un defaut inattendu, et il n'est pas le verdict.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <semaphore>
#include <system_error>
#include <thread>
#include <utility>

#include "catalogue/catalogue.hpp"
#include "forest_support.hpp"
#include "index/index.hpp"
#include "tower/pipeline.hpp"

namespace {

using Sem = std::binary_semaphore;

// Etat du harnais, prepare avant les fils. Les crochets ne bloquent que leur PREMIERE visite pour le fil indique ;
// toute autre visite est transparente (l'attente de A est seulement comptee).
std::atomic<bool> armed{false}, first_withdrawn_a{true}, first_announce_b{true}, first_wait_a{true};
Sem withdrawn_a{0}, announced_b{0}, event_a{0}, returned_b{0};  // evenements vers le harnais
Sem go_withdrawn_a{0}, go_announce_b{0}, go_wait_a{0};          // liberations vers les fils
// event_a porte deux evenements distincts de A, dans l'ordre ou ils surviennent : kind_a les dit (1 retour, 2 attente).
std::atomic<int> waits_a{0}, returns_a{0}, returns_b{0}, kind_a{0};

}  // namespace

namespace mhgp12::tower::detail {

void region_hook(int point, u32 worker) noexcept {
  if (!armed.load()) return;
  if (point == kHookWithdrawn && worker == 0 && first_withdrawn_a.exchange(false)) {
    withdrawn_a.release();
    go_withdrawn_a.acquire();
  } else if (point == kHookAnnounce && worker == 1 && first_announce_b.exchange(false)) {
    announced_b.release();
    go_announce_b.acquire();
  } else if (point == kHookWait && worker == 0) {
    waits_a.fetch_add(1);
    if (first_wait_a.exchange(false)) {
      kind_a.store(2);
      event_a.release();
      go_wait_a.acquire();
    }
  }
}

}  // namespace mhgp12::tower::detail

namespace mhgp12::tower_test {
namespace {

using tower::detail::Pipeline;

// Index et catalogue d'un nuage d'un site (PointId 0), comme make_chain de pipeline_unit.cpp.
struct Chain {
  std::optional<GlobalIndex> index;
  std::optional<Catalogue> catalogue;
};
Chain one_site_chain(MemoryBudget& budget, sched::Pool& pool) {
  const std::vector<u32> x{1000}, y{2000}, z{3000};
  const std::vector<PointId> ids{make_id<PointId>(0)};
  Chain c;
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  if (!cloud.ok()) return c;
  auto index = build_index(std::move(cloud).take(), IndexParams{}, budget);
  if (!index.ok()) return c;
  c.index.emplace(std::move(index).take());
  CatalogueParams params;
  params.kmax = 1;
  auto catalogue = build_catalogue(c.index->cloud(), params, budget, pool);
  if (catalogue.ok()) c.catalogue.emplace(std::move(catalogue).take());
  return c;
}

// Coupe finale : toute etape terminee (les absentes le sont par prepare_graph), plus de tranche de G, aucun
// participant annonce.
bool final_cut(const Pipeline& p) {
  for (u32 i = 0; i < p.orders; ++i)
    for (u32 s = 0; s < tower::detail::kStepCount; ++s)
      if (!tower::detail::step_done(p, i, static_cast<tower::detail::Step>(s))) return false;
  return p.g_next.load() >= p.g_total && p.in_flight.load() == 0;
}

// Lance un fil du harnais ; faux si sa creation echoue, sans rien laisser de joignable.
template <class Body>
bool start(std::thread& t, Body&& body) {
  try {
    t = std::thread(std::forward<Body>(body));
    return true;
  } catch (const std::system_error&) {
    return false;
  }
}

MHGP12_TEST(terminaison, 11) {
  CHECK_EQ(tower::detail::region_hooks_build, 1);  // le corps instrumente est celui qui est lie
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(2);
  const Chain c = one_site_chain(budget, *pool);
  REQUIRE(c.catalogue.has_value());
  auto run = std::make_unique<tower::detail::SessionRun>(*c.index, *c.catalogue, budget, *pool);
  REQUIRE(tower::detail::open_session(*run).ok());
  REQUIRE(tower::detail::admit_session(*run).ok());
  Pipeline& p = *run->pipeline;
  REQUIRE(p.threads == 2 && p.orders == 1);
  p.start = std::chrono::steady_clock::now();
  REQUIRE(tower::detail::run_region(&p, 0, 1, 0).ok());  // seul, crochets desarmes
  REQUIRE(final_cut(p));
  // Deux fils du harnais sur la coupe finale, ordonnes par la table du protocole ; aucun controle avant les jointures.
  armed.store(true);
  std::thread a, b;
  const bool a_started = start(a, [&p] {
    (void)tower::detail::run_region(&p, 0, 1, 0);
    returns_a.fetch_add(1);
    kind_a.store(1);
    event_a.release();
  });
  bool b_started = false;
  int first_kind = 0;
  if (a_started) {
    withdrawn_a.acquire();  // A a retire 1 -> 0 et attend sa liberation ; aucune autre annonce
    b_started = start(b, [&p] {
      (void)tower::detail::run_region(&p, 1, 2, 1);
      returns_b.fetch_add(1);
      returned_b.release();
    });
    if (b_started) announced_b.acquire();  // B a annonce 0 -> 1 et n'a pas encore scanne
    go_withdrawn_a.release();              // A reprend tandis que B reste compte (ou seul si B n'a pas demarre)
    event_a.acquire();                     // corrige : retour de A ; ancien : attente de A, avant la boucle
    first_kind = kind_a.load();
    if (b_started) {  // B fait son scan vide, retire 1 -> 0 et sort
      go_announce_b.release();
      returned_b.acquire();
    }
    if (first_kind == 2) {  // A voit desormais 0 dans l'attente, recommence seul, retire 1 -> 0 et sort
      go_wait_a.release();
      event_a.acquire();
    }
    a.join();
    if (b_started) b.join();
  }
  armed.store(false);
  REQUIRE(a_started && b_started);  // echec de preparation, signale apres la jointure des fils lances
  std::printf("terminaison premier_evenement_A=%s attente_A=%d retours_A=%d retours_B=%d in_flight=%u\n",
              first_kind == 1 ? "retour" : "attente", waits_a.load(), returns_a.load(), returns_b.load(),
              p.in_flight.load());
  CHECK_EQ(p.in_flight.load(), 0u);
  CHECK(waits_a.load() == 0 && returns_a.load() == 1 && returns_b.load() == 1);
  CHECK(final_cut(p));
}

}  // namespace
}  // namespace mhgp12::tower_test

MHGP12_TEST_MAIN()

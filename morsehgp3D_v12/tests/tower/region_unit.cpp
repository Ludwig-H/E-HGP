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
//   fermeture    (A6b, residu « test force aide/fermeture » de CST-0242) cloture du noyau face aux taches d'indices,
//                quatre points de plus du meme corps (debut de la cloture, avant la levee de hint_closed ; chaque tour
//                de l'attente de hint_active nul ; fin de l'attente, avant close_kernel ; garde de run_hint franchie,
//                avant hint_leaves). Session d'un nuage aleatoire de 800 sites, K1 (au moins deux tranches de
//                cellules), Pool de 2. Le fil K joue run_region seul (fil 0) ; arrete au debut de la cloture, le fil H
//                appelle run_hint sur la tranche 1 et s'arrete garde franchie (hint_active = 1) ; K relache doit
//                ATTENDRE : premier evenement de K = tour d'attente, noyau non termine et union-find vivant pendant
//                l'arret. H relache indice toute la tranche sur des tampons vivants et rend. K sort de l'attente et
//                s'arrete avant close_kernel : une tache d'indices reclamee avant la cloture mais jouee maintenant doit
//                lire hint_closed vrai et ne rien indicer (tampons encore vivants : un defaut se lit au compte, pas a un
//                signal). K relache clot et finit seul la Session : coupe finale, foret de l'ordre 1 identique a celle
//                de build_tower sur le meme nuage. Mutants : sans attente, le premier evenement de K est la fin de
//                l'attente (H toujours arrete) ; sans garde, la tache tardive indice la tranche. Memes regles : arrets
//                a la premiere visite seulement, aucune attente bornee, controles apres les jointures.
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <random>
#include <semaphore>
#include <set>
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

// Fermeture : crochets actifs si armed_close ; K = fil 0 de run_region, H = fil du harnais marque par harness_hint.
// event_k porte jusqu'a deux evenements de K : tour d'attente (1, une seule fois) et fin d'attente (2) ; first_k dit le
// premier (1 attendu ; 2 si la cloture n'attend pas la tache en cours).
std::atomic<bool> armed_close{false}, first_close_begin{true}, first_hint_work{true}, first_close_wait{true},
    first_closed{true};
Sem close_begin{0}, hint_working{0}, event_k{0}, hint_returned{0}, k_returned{0};
Sem go_close{0}, go_hint{0}, go_k_wait{0}, go_k_closed{0};
std::atomic<int> close_waits{0}, first_k{0};
thread_local bool harness_hint = false;

void close_hook(int point, mhgp12::u32 worker) {
  namespace d = mhgp12::tower::detail;
  if (point == d::kHookCloseBegin && worker == 0 && first_close_begin.exchange(false)) {
    close_begin.release();
    go_close.acquire();
  } else if (point == d::kHookHintWork && harness_hint && first_hint_work.exchange(false)) {
    hint_working.release();
    go_hint.acquire();
  } else if (point == d::kHookCloseWait && worker == 0) {
    close_waits.fetch_add(1);
    if (first_close_wait.exchange(false)) {
      int none = 0;
      first_k.compare_exchange_strong(none, 1);
      event_k.release();
      go_k_wait.acquire();
    }
  } else if (point == d::kHookClosed && worker == 0 && first_closed.exchange(false)) {
    int none = 0;
    first_k.compare_exchange_strong(none, 2);
    event_k.release();
    go_k_closed.acquire();
  }
}

}  // namespace

namespace mhgp12::tower::detail {

void region_hook(int point, u32 worker) noexcept {
  if (armed_close.load()) {
    close_hook(point, worker);
    return;
  }
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

// Index et catalogue K1 d'un nuage (PointId = rang d'entree), comme make_chain de pipeline_unit.cpp.
struct Chain {
  std::optional<GlobalIndex> index;
  std::optional<Catalogue> catalogue;
};
Chain chain_of(const std::vector<u32>& x, const std::vector<u32>& y, const std::vector<u32>& z, MemoryBudget& budget,
               sched::Pool& pool) {
  std::vector<PointId> ids(x.size());
  for (u32 i = 0; i < ids.size(); ++i) ids[i] = make_id<PointId>(i);
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

// Un site (PointId 0) : terminaison.
Chain one_site_chain(MemoryBudget& budget, sched::Pool& pool) { return chain_of({1000}, {2000}, {3000}, budget, pool); }

// n sites distincts tires dans un cube de cote 4096 (graine fixe) : fermeture.
Chain random_chain(u32 n, MemoryBudget& budget, sched::Pool& pool) {
  std::mt19937_64 rng(20261008);
  std::vector<u32> x, y, z;
  std::set<std::array<u32, 3>> seen;
  while (x.size() < n) {
    const std::array<u32, 3> c{1000 + static_cast<u32>(rng() % 4096), 1000 + static_cast<u32>(rng() % 4096),
                               1000 + static_cast<u32>(rng() % 4096)};
    if (!seen.insert(c).second) continue;
    x.push_back(c[0]);
    y.push_back(c[1]);
    z.push_back(c[2]);
  }
  return chain_of(x, y, z, budget, pool);
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

MHGP12_TEST(fermeture, 14) {
  CHECK_EQ(tower::detail::region_hooks_build, 1);  // le corps instrumente est celui qui est lie
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(2);
  const Chain c = random_chain(800, budget, *pool);
  REQUIRE(c.catalogue.has_value());
  auto ref = build_tower(*c.index, *c.catalogue, budget, *pool);  // reference : la Session, crochets desarmes
  REQUIRE(ref.ok());
  auto run = std::make_unique<tower::detail::SessionRun>(*c.index, *c.catalogue, budget, *pool);
  REQUIRE(tower::detail::open_session(*run).ok());
  REQUIRE(tower::detail::admit_session(*run).ok());
  Pipeline& p = *run->pipeline;
  REQUIRE(p.threads == 2 && p.orders == 1 && p.g_items[0] >= 2);
  const ForestInput& in = p.forest.inputs[0];
  const u64 grain = tower_detail::kCellGrain;
  const u64 slice_reps = in.rep_offsets[std::min<u64>(in.cell_ball.size(), 2 * grain)] - in.rep_offsets[grain];
  p.start = std::chrono::steady_clock::now();
  // K joue seul la Session (fil 0) ; H et la tache tardive appellent run_hint sur la tranche 1 de l'ordre 1.
  armed_close.store(true);
  std::thread k, h;
  bool k_ok = false, k_early = false, h_started = false, h_skipped = false, kernel_done = true, cells_alive = false;
  u64 hinted_h = 0, late = ~u64{0};
  const bool k_started = start(k, [&p, &k_ok, &k_early] {
    k_ok = tower::detail::run_region(&p, 0, 1, 0).ok();
    if (first_close_begin.exchange(false)) {  // region finie sans cloture du noyau : le harnais ne doit pas attendre
      k_early = true;
      close_begin.release();
    }
    k_returned.release();
  });
  if (k_started) {
    close_begin.acquire();  // K au debut de la cloture : hint_closed encore faux
    if (!k_early) {
      h_started = start(h, [&p, &hinted_h, &h_skipped] {
        harness_hint = true;
        hinted_h = tower::detail::run_hint(p, 1, 0, 1);
        if (first_hint_work.exchange(false)) {  // garde jamais franchie : ne pas laisser le harnais attendre
          h_skipped = true;
          hint_working.release();
        }
        hint_returned.release();
      });
      if (h_started) hint_working.acquire();  // H garde franchie, hint_active = 1, arrete avant hint_leaves
      go_close.release();
      event_k.acquire();  // premier evenement de K : tour d'attente (1) ou fin d'attente sans attente (2)
      kernel_done = tower::detail::step_done(p, 0, tower::detail::kKernel);
      cells_alive = p.forest.work[0].cells.size() == p.forest.forests.orders[0].births;
      if (h_started) {
        go_hint.release();
        hint_returned.acquire();
      }
      if (first_k.load() == 1) {  // K sort de l'attente et s'arrete avant close_kernel
        go_k_wait.release();
        event_k.acquire();
      }
      // K arrete avant close_kernel : hint_closed vrai, aucune tache en cours, tampons vivants. Tache tardive.
      harness_hint = true;
      late = tower::detail::run_hint(p, 1, 0, 1);
      harness_hint = false;
      go_k_closed.release();
    }
    k_returned.acquire();
    k.join();
    if (h_started) h.join();
  }
  armed_close.store(false);
  REQUIRE(k_started && !k_early && h_started && !h_skipped);  // signale apres la jointure des fils lances
  std::printf("fermeture premier_evenement_K=%s tours_attente=%d noyau_termine_a_l_arret=%d indicees_H=%llu/%llu "
              "tardive=%llu\n",
              first_k.load() == 1 ? "attente" : "fin_sans_attente", close_waits.load(), kernel_done ? 1 : 0,
              static_cast<unsigned long long>(hinted_h), static_cast<unsigned long long>(slice_reps),
              static_cast<unsigned long long>(late));
  CHECK(k_ok);
  CHECK_EQ(first_k.load(), 1);  // la cloture attend la tache en cours
  CHECK(close_waits.load() >= 1);
  CHECK(!kernel_done && cells_alive);  // pendant l'arret : noyau non termine, union-find vivant
  CHECK_EQ(hinted_h, slice_reps);      // H a indice toute sa tranche
  CHECK_EQ(late, 0u);                  // une tache jouee apres la levee de hint_closed ne touche plus aux tampons
  CHECK(final_cut(p));
  CHECK(same_registry(p.forest.forests.orders[0], ref.value().forests.orders[0]));
}

}  // namespace
}  // namespace mhgp12::tower_test

MHGP12_TEST_MAIN()

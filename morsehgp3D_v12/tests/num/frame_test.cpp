// Repere local certifie (NUM-REPERE) et paliers : etendues exactes, boites fermees a 2^32 (CST-0204), site exterieur a
// la boite (NUM-COUVERTURE), garde prouvee par requete et non par repere commun, coupes des paliers.
// Temoins de l'auditeur Codex rejoues : receipts/audit_contrats_20261007/repere_et_profondeur/check.py.
#include <array>
#include <cstdio>

#include "num/num.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;

namespace {
Frame frame_of(std::initializer_list<std::array<u64, 3>> points) {
  Frame frame;
  for (const auto& p : points)
    if (!frame.add(p).ok()) throw std::runtime_error("repere de test refuse");
  return frame;
}
}  // namespace

MHGP12_TEST(frame_extents, 33) {
  // Etendue d'une largeur : plus petit s tel que largeur < 2^s.
  CHECK_EQ(span_bits(0), 0);
  CHECK_EQ(span_bits(1), 1);
  CHECK_EQ(span_bits(2), 2);
  CHECK_EQ(span_bits((u64{1} << 20) - 1), 20);
  CHECK_EQ(span_bits(u64{1} << 20), 21);
  CHECK_EQ(span_bits((u64{1} << 32) - 1), 32);
  CHECK_EQ(span_bits(u64{1} << 32), 33);
  // Etendue nulle : un point, ou des points confondus.
  CHECK_EQ(frame_of({{7, 8, 9}}).span(), 0);
  CHECK_EQ(frame_of({{7, 8, 9}, {7, 8, 9}}).span(), 0);
  CHECK(Frame().empty());
  CHECK_EQ(Frame().span(), 0);
  // Extremites des trois profils : sites (0,0,0) et (2^b-1,0,0), etendue b ; fermeture de leur boite de centres
  // (hi = max(site)+1 = 2^b, comme boxes.cpp de la v11), etendue b+1 : 22, 25 et 33.
  for (const int bits : {21, 24, 32}) {
    const u64 top = u64{1} << bits;
    Frame sites = frame_of({{0, 0, 0}, {top - 1, 0, 0}});
    CHECK_EQ(sites.span(), bits);
    Frame closure = sites;
    REQUIRE(closure.add_box({0, 0, 0}, {top, 1, 1}).ok());
    CHECK_EQ(closure.span(), bits + 1);
    CHECK_EQ(closure.upper()[0], top);
  }
  // Un repere de 33 bits est le palier large.
  CHECK(frame_of({{0, 0, 0}, {u64{1} << 32, 0, 0}}).tier() == Tier::wide);
  // Site exterieur a la boite : la liste d'une feuille entre dans son repere (NUM-COUVERTURE).
  Frame leaf;
  REQUIRE(leaf.add_box({0, 0, 0}, {1, 1, 1}).ok());
  CHECK_EQ(leaf.span(), 1);
  leaf.add_site(1u << 20, 0, 0);
  CHECK_EQ(leaf.span(), 21);
  // Garde : chaque paire (support, requete gardee) tient sur s+2 bits, l'union du pave sur s+3 (temoin {0,3},
  // requetes -7 et 11 de l'auditeur, translate de 8 pour rester dans des entiers positifs).
  const Frame support = frame_of({{8, 0, 0}, {11, 0, 0}});
  CHECK_EQ(support.span(), 2);
  Frame low = support, high = support, both = support;
  low.add_site(1, 0, 0);
  high.add_site(19, 0, 0);
  both.add_site(1, 0, 0);
  both.add_site(19, 0, 0);
  CHECK_EQ(low.span(), 4);
  CHECK_EQ(high.span(), 4);
  CHECK_EQ(both.span(), 5);
  // Reunion : le repere de l'union.
  Frame united = low;
  united.unite(high);
  CHECK_EQ(united.span(), 5);
  CHECK(united.origin() == (std::array<u64, 3>{1, 0, 0}));
}

MHGP12_TEST(frame_refusals, 14) {
  Frame frame = frame_of({{5, 5, 5}});
  // Au-dela de 2^32 : refus, repere inchange.
  CHECK_EQ(frame.add({(u64{1} << 32) + 1, 0, 0}).reason, Reason::parameter_out_of_range);
  CHECK_EQ(frame.span(), 0);
  CHECK(frame.add({u64{1} << 32, 0, 0}).ok());
  CHECK_EQ(frame.span(), 32);  // largeur 2^32 - 5
  CHECK(frame.add({0, 0, 0}).ok());
  CHECK_EQ(frame.span(), 33);  // largeur 2^32
  // Boite renversee ou trop haute : refus, repere inchange.
  Frame box;
  CHECK_EQ(box.add_box({2, 0, 0}, {1, 0, 0}).reason, Reason::parameter_out_of_range);
  CHECK(box.empty());
  CHECK_EQ(box.add_box({0, 0, 0}, {(u64{1} << 32) + 1, 0, 0}).reason, Reason::parameter_out_of_range);
  CHECK(box.empty());
  CHECK(box.add_box({0, 0, 0}, {0, 0, 0}).ok());
  CHECK_EQ(box.span(), 0);
  CHECK(!box.empty());
  CHECK(box.covers({0, 0, 0}) && !box.covers({1, 0, 0}));
}

MHGP12_TEST(frame_tiers, 24) {
  // Coupes des paliers : 16 etroit, 17 moyen, 24 moyen, 25 large, 33 large.
  CHECK(tier_of(0) == Tier::narrow);
  CHECK(tier_of(16) == Tier::narrow);
  CHECK(tier_of(17) == Tier::medium);
  CHECK(tier_of(24) == Tier::medium);
  CHECK(tier_of(25) == Tier::wide);
  CHECK(tier_of(33) == Tier::wide);
  CHECK_EQ(tier_span(Tier::narrow), 16);
  CHECK_EQ(tier_span(Tier::medium), 24);
  CHECK_EQ(tier_span(Tier::wide), 33);
  // Table du paragraphe 3 du contrat, aux plafonds des paliers et aux seuils natifs.
  CHECK_EQ(SpanBudgets<16>::center_orientation, 121);
  CHECK_EQ(SpanBudgets<17>::center_orientation, 128);
  CHECK_EQ(SpanBudgets<19>::side, 122);
  CHECK_EQ(SpanBudgets<20>::side, 128);
  CHECK_EQ(SpanBudgets<19>::guarded_side, 125);
  CHECK_EQ(SpanBudgets<20>::guarded_side, 131);
  CHECK_EQ(SpanBudgets<24>::numerator3, 125);
  CHECK_EQ(SpanBudgets<25>::numerator3, 130);
  CHECK_EQ(SpanBudgets<29>::reservoir, 62);
  CHECK_EQ(SpanBudgets<30>::reservoir, 64);
  CHECK_EQ(SpanBudgets<33>::level_comparison, 482);
  CHECK_EQ(SpanBudgets<24>::midpoint_frame, 126);
  CHECK_EQ(SpanBudgets<25>::midpoint_frame, 131);
  CHECK_EQ(SpanBudgets<14>::center_fraction, 122);
  CHECK_EQ(words_for(SpanBudgets<33>::level_comparison), 8);
}

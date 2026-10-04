// Portes de core : registre de compteurs et de durees (fusion entre fils, saturation, JSON canonique).
#include <string>

#include "core/core.hpp"
#include "test.hpp"

using namespace mhgp11;

MHGP11_TEST(ledger, 31) {
  const u64 max = ~u64{0};
  Ledger empty;
  CHECK_EQ(empty.to_json(), "{\"counters\":{},\"nanoseconds\":{}}");
  CHECK_EQ(empty.counter("absent"), 0u);
  CHECK_EQ(empty.nanoseconds("absent"), 0u);

  Ledger a;
  a.count("boules");  // delta par defaut : 1
  a.count("boules", 41);
  a.count("aretes", 7);
  a.time("catalogue", 1500);
  a.time("catalogue", 500);
  CHECK_EQ(a.counter("boules"), 42u);
  CHECK_EQ(a.counter("aretes"), 7u);
  CHECK_EQ(a.nanoseconds("catalogue"), 2000u);
  CHECK_EQ(a.counter("catalogue"), 0u);      // compteurs et durees sont deux tables
  CHECK_EQ(a.nanoseconds("boules"), 0u);
  // cles triees quel que soit l'ordre d'insertion
  CHECK_EQ(a.to_json(), "{\"counters\":{\"aretes\":7,\"boules\":42},\"nanoseconds\":{\"catalogue\":2000}}");

  // fusion : somme cle par cle, commutative et associative
  Ledger b;
  b.count("boules", 8);
  b.count("cellules", 3);
  b.time("tour", 9);
  Ledger c;
  c.count("aretes", 1);
  c.time("catalogue", 1);
  Ledger ab = a;
  ab.merge(b);
  Ledger ba = b;
  ba.merge(a);
  CHECK_EQ(ab.counter("boules"), 50u);
  CHECK_EQ(ab.counter("cellules"), 3u);
  CHECK_EQ(ab.counter("aretes"), 7u);
  CHECK_EQ(ab.nanoseconds("tour"), 9u);
  CHECK_EQ(ab.nanoseconds("catalogue"), 2000u);
  CHECK_EQ(ab.to_json(), ba.to_json());
  Ledger ab_c = ab;
  ab_c.merge(c);
  Ledger bc = b;
  bc.merge(c);
  Ledger a_bc = a;
  a_bc.merge(bc);
  CHECK_EQ(ab_c.to_json(), a_bc.to_json());
  CHECK_EQ(ab_c.to_json(), "{\"counters\":{\"aretes\":8,\"boules\":50,\"cellules\":3},"
                           "\"nanoseconds\":{\"catalogue\":2001,\"tour\":9}}");
  Ledger with_empty = a;
  with_empty.merge(empty);
  CHECK_EQ(with_empty.to_json(), a.to_json());  // le registre vide est neutre
  CHECK_EQ(b.to_json(), "{\"counters\":{\"boules\":8,\"cellules\":3},\"nanoseconds\":{\"tour\":9}}");  // source intacte

  // saturation : un compteur ne revient jamais a une petite valeur
  Ledger s;
  s.count("x", max - 1);
  s.count("x", 1);
  CHECK_EQ(s.counter("x"), max);
  s.count("x", 5);
  CHECK_EQ(s.counter("x"), max);
  s.time("t", max);
  s.time("t", max);
  CHECK_EQ(s.nanoseconds("t"), max);
  Ledger s2;
  s2.count("x", 2);
  s2.merge(s);
  CHECK_EQ(s2.counter("x"), max);
  CHECK_EQ(s.to_json(), "{\"counters\":{\"x\":18446744073709551615},\"nanoseconds\":{\"t\":18446744073709551615}}");

  // noms quelconques : le JSON reste de l'ASCII pur. Guillemet et barre oblique inversee echappes ; tout octet hors
  // de l'ASCII imprimable (controle, 0x7F, 0x80 et au-dela) ecrit \u00XX : la correspondance est injective.
  Ledger e;
  e.count("a\"b\\c", 1);
  e.count(std::string("ligne\nsuivante\x01"), 2);
  e.count(std::string("\x7f\xc3\xa9\xff"), 3);
  CHECK_EQ(e.to_json(), "{\"counters\":{\"a\\\"b\\\\c\":1,\"ligne\\u000asuivante\\u0001\":2,"
                        "\"\\u007f\\u00c3\\u00a9\\u00ff\":3},\"nanoseconds\":{}}");
  CHECK_EQ(e.counter("a\"b\\c"), 1u);
  bool ascii = true;
  for (const char c : e.to_json()) ascii = ascii && c >= 0x20 && c <= 0x7E;
  CHECK(ascii);

  // chronometre d'etage : il ne connait aucun registre ; l'etage ecrit lui-meme sa duree. La valeur n'est pas jugee
  // (aucune porte ne depend de l'heure), seulement sa monotonie.
  Ledger t;
  const Stopwatch watch;
  const u64 first = watch.nanoseconds();
  const u64 second = watch.nanoseconds();
  CHECK(second >= first);
  t.time("etage", second);
  CHECK(t.to_json().rfind("{\"counters\":{},\"nanoseconds\":{\"etage\":", 0) == 0);
  t.time("etage", max);  // sature : la valeur lue ne depend plus de l'horloge
  CHECK_EQ(t.nanoseconds("etage"), max);
  CHECK_EQ(t.counter("etage"), 0u);
}

// Registre de compteurs et de durees par etage : compteurs nommes u64 et nanosecondes par etage, fusionnables entre
// fils, serialisables en JSON. Ecrit a neuf pour la v11 (la v10 n'avait pas de registre commun).
//
// Un Ledger n'est pas partage entre fils : chaque fil remplit le sien, le proprietaire les fusionne (merge). La
// fusion additionne cle par cle ; elle est commutative et associative, donc le registre fusionne ne depend pas de
// l'ordre d'arrivee des fils. Les additions saturent a 2^64 - 1 : un compteur ne revient jamais a une petite valeur.
// Les compteurs sont deterministes si les etages le sont ; les durees ne le sont jamais et n'entrent dans aucune
// sortie canonique. Petit etat hors boucle chaude : une boucle chaude compte dans une variable locale et ajoute une
// fois.
//
// Exceptions : count, time et merge allouent (std::map) et peuvent lever std::bad_alloc (frontiere : guarded,
// status.hpp). Chacune est tout ou rien : si elle leve, le registre est inchange (merge prepare la fusion a part et
// l'echange a la fin).
//
// Noms : une suite d'octets quelconque. Le JSON reste de l'ASCII pur : tout octet hors de l'ASCII imprimable est
// ecrit \u00XX, c'est-a-dire le point de code de meme valeur (U+0000 a U+00FF) ; la correspondance entre noms et
// cles JSON est injective. Les noms du produit sont des litteraux ASCII.
#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>

#include "core/types.hpp"

namespace mhgp11 {

// Table triee nom -> valeur ; la recherche accepte un std::string_view sans copie (std::less<>).
using LedgerTable = std::map<std::string, u64, std::less<>>;

class Ledger {
 public:
  // Ajoute delta au compteur `name` (cree a 0 s'il est absent).
  void count(std::string_view name, u64 delta = 1);
  // Ajoute une duree a l'etage `stage` (cree a 0 s'il est absent).
  void time(std::string_view stage, u64 nanoseconds);
  // Additionne cle par cle le registre d'un autre fil.
  void merge(const Ledger& other);

  // Valeur d'un compteur ou d'un etage ; 0 s'il est absent.
  u64 counter(std::string_view name) const;
  u64 nanoseconds(std::string_view stage) const;

  // {"counters":{...},"nanoseconds":{...}} : cles triees octet par octet, entiers decimaux, sans espace, ASCII pur.
  std::string to_json() const;

 private:
  LedgerTable counters_;
  LedgerTable times_;
};

// Chronometre d'un etage (horloge monotone). Il ne connait aucun registre et sa destruction ne fait rien : l'etage
// ecrit lui-meme sa duree, ledger.time("etage", watch.nanoseconds()), la ou une allocation peut etre refusee
// proprement. Un minuteur qui ecrirait dans le registre a sa destruction devrait y retrouver sa cle : si le nom ou
// le registre changent entre-temps, il alloue dans un destructeur, et une penurie termine le processus (audits du
// 2 octobre 2026).
class Stopwatch {
 public:
  Stopwatch() noexcept : start_(std::chrono::steady_clock::now()) {}
  // Nanosecondes ecoulees depuis la construction ; jamais decroissant d'une lecture a la suivante.
  u64 nanoseconds() const noexcept;

 private:
  std::chrono::steady_clock::time_point start_;
};
static_assert(std::is_trivially_destructible_v<Stopwatch>, "Stopwatch : aucune action a la destruction");

}  // namespace mhgp11

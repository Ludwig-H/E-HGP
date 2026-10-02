// Registre de compteurs et de durees par etage : compteurs nommes u64 et nanosecondes par etage, fusionnables entre
// fils, serialisables en JSON. Ecrit a neuf pour la v11 (la v10 n'avait pas de registre commun).
//
// Un Ledger n'est pas partage entre fils : chaque fil remplit le sien, le proprietaire les fusionne (merge). La
// fusion additionne cle par cle ; elle est commutative et associative, donc le registre fusionne ne depend pas de
// l'ordre d'arrivee des fils. Les additions saturent a 2^64 - 1 : un compteur ne revient jamais a une petite valeur.
// Les compteurs sont deterministes si les etages le sont ; les durees ne le sont jamais et n'entrent dans aucune
// sortie canonique. Petit etat hors boucle chaude : une boucle chaude compte dans une variable locale et ajoute une
// fois. Les ajouts allouent (std::map) et peuvent lever std::bad_alloc (frontiere : guarded, status.hpp).
#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <string_view>

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

  // {"counters":{...},"nanoseconds":{...}} : cles triees octet par octet, entiers decimaux, sans espace.
  std::string to_json() const;

 private:
  LedgerTable counters_;
  LedgerTable times_;
};

// Mesure un etage : ajoute a sa destruction la duree ecoulee (horloge monotone) a l'etage `stage` du registre.
// L'etage est cree a la construction : la destruction n'alloue pas et ne leve pas. `stage` et le registre doivent
// survivre au minuteur.
class StageTimer {
 public:
  StageTimer(Ledger& ledger, std::string_view stage);
  ~StageTimer();
  StageTimer(const StageTimer&) = delete;
  StageTimer& operator=(const StageTimer&) = delete;

 private:
  Ledger& ledger_;
  std::string_view stage_;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace mhgp11

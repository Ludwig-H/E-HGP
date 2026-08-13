// MorseHGP3D v3 — INTERFACE DU JUGE INDEPENDANT DU CONE CIBLE.
//
// Ce header ne declare que des types du langage. Il n'inclut ni
// `spindle_cone.hpp`, ni `anchor_envelope.hpp`, ni `mhgp/mhgp.hpp`, et il
// n'expose aucun type de production. L'unite de traduction qui l'implemente
// (`spindle_cone_oracle.cpp`) reecrit ses seuils ET son arithmetique.
//
// Raison : le contre-audit du 13 aout 2026 a montre qu'un juge partageant
// `anchor::lane_death_threshold` avec le sujet ferme les memes paires que lui
// lorsque cette fonction est fausse. Un `smax` hors largeur `int` rendait des
// seuils negatifs des deux cotes, et l'accord etait total sur zero temoin.
// Un juge n'est independant que si un defaut du sujet ne peut pas s'y produire
// a l'identique.
#pragma once

#include <cstdint>
#include <vector>

namespace mhgp3v {
namespace cone_oracle {

// Verites par lane sur les paires ORDONNEES `(a,b)`, `a != b`, a l'indice
// `a*n+b`. Une lane est morte lorsque le nombre de temoins universels de cette
// lane atteint son seuil. Les trois lanes sont INDEPENDANTES : la conjonction
// est publiee separement et ne remplace aucune des trois.
struct LaneTruth {
  std::vector<unsigned char> dead_q2;
  std::vector<unsigned char> dead_q3;
  std::vector<unsigned char> dead_q4;
  long long closable_q2 = 0;
  long long closable_q3 = 0;
  long long closable_q4 = 0;
  long long closable_all = 0;
  bool refused = false;         // domaine `smax` ou cardinalite invalide
  const char* refusal = "";
};

// `xyz` porte exactement `3*n` coordonnees entieres. Le juge refuse plutot que
// de convertir : c'est precisement le defaut qu'il doit detecter.
LaneTruth judge(const std::vector<int>& xyz, int n, long long smax);

// JUGER LE JUGE. Compare l'arithmetique deux limbes ecrite ici a `mhgp3v::BigInt`
// (signe et magnitude, chiffres de 32 bits) sur `draws` tirages dans
// `[0, span]`. Rend le nombre de desaccords ; zero est la seule valeur admise.
long long selftest_against_bigint(long long draws, long long span, unsigned long long seed);

// Nombre de temoins universels reellement compares par le dernier `judge`.
// Sert aux planchers d'anti-vacuite.
long long last_witness_evaluations();

}  // namespace cone_oracle
}  // namespace mhgp3v

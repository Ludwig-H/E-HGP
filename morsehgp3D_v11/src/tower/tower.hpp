// En-tete public de tower : parapluie du domaine FULL, de la MEB bornee et des forets FULL.
// Un autre module n'inclut que ce fichier ; les fichiers internes de tower n'incluent jamais ce parapluie (cycle).
// Aucun code ne quitte tower_detail : les using ci-dessous nomment les types et fonctions publics existants.
#pragma once

#include "tower/full_domain.hpp"
#include "tower/meb.hpp"
#include "tower/forest.hpp"

namespace mhgp11 {

using tower_detail::ForestNode;
using tower_detail::ForestLedger;
using tower_detail::OrderForest;
using tower_detail::FullTower;
using tower_detail::FullParams;
using tower_detail::FullTimings;
using tower_detail::OrderTimings;
using tower_detail::build_full;

}  // namespace mhgp11

// En-tete public de tower : parapluie du domaine FULL, de la MEB bornee, des forets FULL, de l'arbre d'ordre K seul et
// de l'index d'ancetres (pointeurs de saut, tranche S9).
// Un autre module n'inclut que ce fichier ; les fichiers internes de tower n'incluent jamais ce parapluie (cycle).
// Aucun code ne quitte tower_detail : les using ci-dessous nomment les types et fonctions publics existants.
#pragma once

#include "tower/full_domain.hpp"
#include "tower/meb.hpp"
#include "tower/forest.hpp"
#include "tower/order_tree.hpp"
#include "tower/ancestor_index.hpp"

namespace mhgp11 {

using tower_detail::ForestNode;
using tower_detail::ForestLedger;
using tower_detail::OrderForest;
using tower_detail::FullTower;
using tower_detail::FullParams;
using tower_detail::FullTimings;
using tower_detail::OrderTimings;
using tower_detail::build_full;
using tower_detail::OrderTree;
using tower_detail::WindowAttachment;
using tower_detail::BallRole;
using tower_detail::build_order;
using tower_detail::AncestorIndex;

}  // namespace mhgp11

#pragma once
#include "points/dendrogram.hpp"
inline mhgp10::PointDendrogram historical_cover() {
  mhgp10::PointDendrogram d;
  d.level = {0.0/1.0, 1.0/2.0, 4.0/1.0, 5.0/1.0, 13.0/2.0, 13.0/1.0, 16.0/1.0, 33.0/2.0, 169.0/9.0, 20.0/1.0, 25.0/1.0};
  d.node_rank = {4, 5, 5, 6, 7, 8};
  d.child_off = {0, 0, 0, 0, 0, 3, 5};
  d.child_val = {0, 1, 2, 3, 4};
  d.parent = {4, 4, 4, 5, 5, 4294967295};
  d.point_node = {1, 0, 0, 0, 5, 2};
  d.point_rank = {5, 4, 4, 4, 10, 5};
  d.point_weight = {1, 1, 1, 1, 1, 1};
  return d;
}

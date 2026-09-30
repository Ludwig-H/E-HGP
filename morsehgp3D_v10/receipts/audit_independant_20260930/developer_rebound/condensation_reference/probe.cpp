#include <iomanip>
#include <iostream>
#include <vector>
#include "head/head.hpp"

template <class T> void read_vector(std::vector<T>& v, std::size_t n) {
  v.resize(n);
  for (auto& value : v) std::cin >> value;
}
template <class T> void array(const std::vector<T>& v) {
  std::cout << '[';
  for (std::size_t i=0; i<v.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << v[i];
  }
  std::cout << ']';
}
int main() {
  using namespace mhgp10;
  std::cout << std::setprecision(17);
  std::size_t count=0;
  std::cin >> count;
  for (std::size_t i=0; i<count; ++i) {
    std::size_t nl=0, nn=0, ne=0, np=0;
    ClusterParams p;
    unsigned single=0, leaf=0;
    std::cin >> nl >> nn >> ne >> np >> p.min_cluster_size >> p.z >> single >> leaf;
    p.allow_single_cluster=single!=0;
    p.selection=leaf ? Selection::leaf : Selection::eom;
    PointDendrogram d;
    read_vector(d.level,nl); read_vector(d.node_rank,nn);
    read_vector(d.child_off,nn+1); read_vector(d.child_val,ne); read_vector(d.parent,nn);
    read_vector(d.point_node,np); read_vector(d.point_rank,np); read_vector(d.point_weight,np);
    if (!std::cin || !validate(d).ok()) return 3;
    const auto out=cluster(d,p);
    const auto& t=out.tree;
    std::cout << "{\"parent\":"; array(t.parent);
    std::cout << ",\"birth\":"; array(t.birth);
    std::cout << ",\"stability\":"; array(t.stability);
    std::cout << ",\"mass\":"; array(t.mass);
    std::cout << ",\"point_cluster\":"; array(t.point_cluster);
    std::cout << ",\"point_lambda\":"; array(t.point_lambda);
    std::cout << ",\"label\":"; array(out.label);
    std::cout << "}\n";
  }
}

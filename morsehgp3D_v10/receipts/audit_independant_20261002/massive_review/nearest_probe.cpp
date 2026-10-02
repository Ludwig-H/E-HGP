#include <iostream>
#include "cloud/site_tree.cpp"

int main() {
  using namespace mhgp10;
  Cloud c;
  std::vector<geom::P3> points;
  for (i64 x=-12;x<=12;++x) for(i64 y=-12;y<=12;++y) for(i64 z=-12;z<=12;++z)
    if(x*x+y*y+z*z==125) points.push_back({64+x,64+y,64+z});
  const auto n=points.size();
  if (!c.x.allocate(n)||!c.y.allocate(n)||!c.z.allocate(n)||!c.w.allocate(n)) return 2;
  c.bits=18;c.weight=n;
  for (size_t i=0;i<n;++i) {c.x[i]=u32(points[i].x);c.y[i]=u32(points[i].y);c.z[i]=u32(points[i].z);c.w[i]=1;}
  SiteTree tree(c);
  // Center of a valid diameter MEB: first point and its antipode are both in the cloud.
  const auto a=points.front();
  const geom::Center center{{64-a.x,64-a.y,64-a.z},1};
  std::vector<std::pair<i128,u32>> out;
  tree.nearest(a,center,1,out);
  if(out.size()!=1||out.front().first!=0||tls.cand.size()!=n||out.capacity()<n) return 3;
  const auto high_cand=tls.cand.capacity(),high_out=out.capacity();
  tree.nearest(a,geom::Center{{0,0,0},1},1,out);
  if(out.size()!=1||tls.cand.capacity()!=high_cand||out.capacity()!=high_out) return 4;
  std::cout<<"{\"status\":\"PASS\",\"sites\":"<<n<<",\"requested_k\":1,"
           <<"\"sphere_candidates\":"<<n<<",\"cand_capacity\":"<<high_cand
           <<",\"out_capacity\":"<<high_out<<",\"after_site_query_cand_size\":"<<tls.cand.size()
           <<",\"after_site_query_out_size\":"<<out.size()
           <<",\"pair_double_u32_bytes\":"<<sizeof(std::pair<double,u32>)
           <<",\"pair_i128_u32_bytes\":"<<sizeof(std::pair<i128,u32>)
           <<",\"Level_bytes\":"<<sizeof(geom::Level)
           <<",\"vector_u32_bytes\":"<<sizeof(std::vector<u32>)<<"}\n";
}

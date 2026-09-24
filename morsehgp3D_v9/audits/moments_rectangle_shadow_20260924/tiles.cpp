// Audit-only: bounded, disjoint one/two-level tiling of heavy WSPD products.
// Depends on the sibling shadow.cpp source.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main mhgp9_frozen_shadow_main
#include "shadow.cpp"
#undef main
#pragma GCC diagnostic pop

struct TileStats {
  std::uint64_t tiles=0, mass=0, corner_calls=0, corner_pairs=0, cheap_calls=0;
  std::uint64_t exact_q3_only=0, exact_q4_only=0, exact_both=0, exact_all_open=0;
  std::uint64_t fast_q3_only=0, fast_q4_only=0, fast_both=0, fast_all_open=0;
  std::uint64_t exact_q3_only_mass=0, exact_q4_only_mass=0, exact_both_mass=0, exact_all_open_mass=0;
  std::uint64_t fast_q3_only_mass=0, fast_q4_only_mass=0, fast_both_mass=0, fast_all_open_mass=0;
  std::uint64_t cheap_ns=0, corner_ns=0, split_ns=0;
};
struct Tile { std::size_t a=0,b=0; std::uint8_t inherited_fast=0,inherited_exact=0; };

static std::uint8_t corners_counted(Box3 a,Box3 b,Moments m,unsigned k,std::uint8_t mask,
                                    std::uint64_t& evaluations) {
  std::uint8_t yes=mask;
  for(int ai=0;ai<8 && yes;++ai)
    for(int bi=0;bi<8 && yes;++bi) {
      ++evaluations;
      yes &= point_test(corner(a,ai),corner(b,bi),m,k,yes);
    }
  return yes;
}
static void record_bits(TileStats& s,std::uint8_t mask,std::uint8_t bits,std::uint64_t mass,bool fast) {
  auto& q3=fast?s.fast_q3_only:s.exact_q3_only;
  auto& q4=fast?s.fast_q4_only:s.exact_q4_only;
  auto& both=fast?s.fast_both:s.exact_both;
  auto& all=fast?s.fast_all_open:s.exact_all_open;
  auto& m3=fast?s.fast_q3_only_mass:s.exact_q3_only_mass;
  auto& m4=fast?s.fast_q4_only_mass:s.exact_q4_only_mass;
  auto& mboth=fast?s.fast_both_mass:s.exact_both_mass;
  auto& mall=fast?s.fast_all_open_mass:s.exact_all_open_mass;
  if(bits==2) {++q3;m3+=mass;}
  if(bits==4) {++q4;m4+=mass;}
  if(bits==6) {++both;mboth+=mass;}
  if(bits==mask) {++all;mall+=mass;}
}
static std::array<Tile,2> split_tile(const Q2CensusIndex& idx,Tile tile) {
  const auto n=idx.spatial_nodes();
  const auto an=n[tile.a],bn=n[tile.b];
  const bool split_a=an.range.size()>=bn.range.size();
  if(split_a) {
    if(an.left==Q2SpatialNode::absent) throw std::runtime_error("cannot split A");
    return {Tile{an.left,tile.b},Tile{an.right,tile.b}};
  }
  if(bn.left==Q2SpatialNode::absent) throw std::runtime_error("cannot split B");
  return {Tile{tile.a,bn.left},Tile{tile.a,bn.right}};
}

int main(int argc,char** argv) try {
  if(argc!=4) throw std::runtime_error("usage: tiles points.u32le K min_product");
  const unsigned k=std::stoul(argv[2]);
  const std::uint64_t min_product=std::stoull(argv[3]);
  if(k<4||k>10) throw std::runtime_error("K outside [4,10]");
  std::ifstream file(argv[1],std::ios::binary);
  if(!file) throw std::runtime_error("cannot open input");
  const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file),{}};
  if(raw.size()%12) throw std::runtime_error("bad input length");
  std::vector<Point3> points(raw.size()/12);
  for(std::size_t j=0;j<points.size();++j)
    points[j]={Coordinate(load32(raw.data()+12*j)),Coordinate(load32(raw.data()+12*j+4)),
               Coordinate(load32(raw.data()+12*j+8))};
  const auto idx=make_q2_cloud_index(prepare_cloud(points));
  const auto nodes=idx->spatial_nodes();
  std::array<TileStats,3> stats{};
  std::uint64_t front=0,open=0,roots=0,root_mass=0,selection_ns=0,group_small=0;
  const auto start=std::chrono::steady_clock::now();
  const auto result=run_wspd_front(*idx,k,8,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r) {
    ++front;
    const auto a=nodes[r.a_node],b=nodes[r.b_node];
    Q34WitnessSearchWork work{};Q34WitnessBoundsWork bounds{};
    const std::uint8_t mask=filter_q34_witnesses(*idx,a.box,b.box,k,r.lane_mask,work,
                                                 Q34WitnessBoundsMode::Affine,bounds);
    if(!mask)return;
    ++open;
    const auto mass=std::uint64_t(a.range.size())*b.range.size();
    if(mass<min_product)return;
    ++roots;root_mass+=mass;
    if(roots>10000) throw std::runtime_error("root safety cap");
    auto t=std::chrono::steady_clock::now();
    const auto group=choose_group(*idx,a.box,b.box);
    const auto moment=make_moments(*idx,group);
    if(moment.n<unsigned(k-1))++group_small;
    selection_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
    std::array<std::vector<Tile>,3> levels;
    levels[0].push_back(Tile{r.a_node,r.b_node});
    for(int depth=0;depth<3;++depth) {
      auto& s=stats[depth];
      for(const auto& tile:levels[depth]) {
        const auto aa=nodes[tile.a],bb=nodes[tile.b];
        const auto pmass=std::uint64_t(aa.range.size())*bb.range.size();
        ++s.tiles;s.mass+=pmass;
        if(s.tiles>10000) throw std::runtime_error("tile safety cap");
        t=std::chrono::steady_clock::now();
        const auto fast=cheap(aa.box,bb.box,moment,k,mask);
        s.cheap_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
        ++s.cheap_calls;
        t=std::chrono::steady_clock::now();
        const auto exact=corners_counted(aa.box,bb.box,moment,k,mask,s.corner_pairs);
        s.corner_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
        ++s.corner_calls;
        if((fast&~exact)!=0 || (tile.inherited_fast&~fast)!=0 || (tile.inherited_exact&~exact)!=0)
          throw std::runtime_error("unsound bound or non-monotone tile");
        record_bits(s,mask,fast,pmass,true);
        record_bits(s,mask,exact,pmass,false);
        if(depth<2) {
          t=std::chrono::steady_clock::now();
          const auto children=split_tile(*idx,tile);
          const auto left=nodes[children[0].a].range.size()*nodes[children[0].b].range.size();
          const auto right=nodes[children[1].a].range.size()*nodes[children[1].b].range.size();
          if(left+right!=pmass) throw std::runtime_error("non-disjoint split mass");
          for(auto child:children) {
            child.inherited_fast=fast;child.inherited_exact=exact;
            levels[depth+1].push_back(child);
          }
          s.split_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
        }
      }
    }
  },6);
  const auto wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  if(result.work.emitted_rectangles!=front) throw std::runtime_error("front counter mismatch");
  std::cout<<"META sites "<<points.size()<<" K "<<k<<" min_product "<<min_product
      <<" front "<<front<<" open "<<open<<" roots "<<roots<<" root_mass "<<root_mass
      <<" group_small "<<group_small<<" selection_ns "<<selection_ns<<" wall_s "<<wall<<'\n';
  for(int depth=0;depth<3;++depth) {
    const auto& s=stats[depth];
    if(s.tiles!=roots*(std::uint64_t{1}<<depth) || s.mass!=root_mass)
      throw std::runtime_error("non-conserving level");
    std::cout<<"DEPTH "<<depth<<" tiles "<<s.tiles<<" mass "<<s.mass
        <<" exact_q3_only "<<s.exact_q3_only<<" exact_q4_only "<<s.exact_q4_only
        <<" exact_both "<<s.exact_both<<" exact_all_open "<<s.exact_all_open
        <<" exact_q3_only_mass "<<s.exact_q3_only_mass<<" exact_q4_only_mass "<<s.exact_q4_only_mass
        <<" exact_both_mass "<<s.exact_both_mass<<" exact_all_open_mass "<<s.exact_all_open_mass
        <<" fast_q3_only "<<s.fast_q3_only<<" fast_q4_only "<<s.fast_q4_only
        <<" fast_both "<<s.fast_both<<" fast_all_open "<<s.fast_all_open
        <<" fast_q3_only_mass "<<s.fast_q3_only_mass<<" fast_q4_only_mass "<<s.fast_q4_only_mass
        <<" fast_both_mass "<<s.fast_both_mass<<" fast_all_open_mass "<<s.fast_all_open_mass
        <<" corner_calls "<<s.corner_calls<<" corner_pairs "<<s.corner_pairs
        <<" cheap_calls "<<s.cheap_calls<<" cheap_ns "<<s.cheap_ns<<" corner_ns "<<s.corner_ns
        <<" split_ns "<<s.split_ns<<'\n';
  }
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}

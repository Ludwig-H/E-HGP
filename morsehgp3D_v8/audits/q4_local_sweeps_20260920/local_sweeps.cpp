// Independent audit prototype. Explicit reuse of audit helpers at 2920b8b5;
// no product code or qualification is imported. The earlier map remains intact.
#define main audit_old_unused_main
#include "../q4_center_blocks_20260920/center_blocks.cpp"
#undef main

#include <span>

namespace {

struct LocalWork {
  Stats spatial;
  U64 I{}, W{}, A{}, max_leaf_active_ids{};
  U64 retained_active_capacity_slots{};
  U64 temporary_ids{}, temporary_capacity_slots{};
  U64 peak_temporary_ids{}, peak_temporary_capacity_slots{};
  U64 peak_live_active_ids{}, peak_live_active_capacity_slots{};
  U64 events{}, constant_inside{}, constant_shell_ids{}, constant_outside{};
  U64 sort_comparisons{}, group_comparisons{}, event_side_tests{}, groups{};
  U64 groups_outside_cell{}, groups_not_owned{}, owned_groups{}, lowdepthgroups{};
  U64 root_location_visits{}, shell_ids{}, shell_sort_comparisons{};
  U64 scratch_peak_capacity_bytes{}, retained_record_shell_capacity_slots{};
  U64 retained_record_string_capacity_bytes{};
  double sweep_ms{};
};

struct LocalNode {
  Cell cell;
  std::array<std::size_t,4> children{absent,absent,absent,absent};
  std::vector<std::size_t> active;
  U64 inside{};  // Exact on UNKNOWN cells; never a clipped initial census.
  Kind kind{Kind::Unknown};
  unsigned char depth{};
};

struct LocalRoot {
  I xi{}, eta{}, denominator{};  // Physical xi,eta: t=A*xi+B*eta; denominator>0.
};

int local_sign(I value) { return (value>0)-(value<0); }
I local_abs(I value) { return value<0?-value:value; }
I local_gcd(I a,I b) {
  while(b!=0) { const I remainder=a%b; a=b; b=remainder; }
  return a;
}
std::string local_decimal(I value) {
  // Every value is strictly smaller than 2^120 in magnitude under u16/depth10.
  return value<0?"-"+decimal(static_cast<U>(-value)):decimal(static_cast<U>(value));
}

LocalRoot local_root(const Form& x,const Form& z) {
  // Solve c_x+a_x*xi+b_x*eta=c_z+a_z*xi+b_z*eta=0.
  // |a|,|b|<=8M^2, |c|<=12M^2: determinant and numerators are O(M^4),
  // not products of two family P/B numerators. Promotions precede products.
  LocalRoot result{I{x.beta}*z.constant-I{z.beta}*x.constant,
                   I{z.alpha}*x.constant-I{x.alpha}*z.constant,
                   I{x.alpha}*z.beta-I{z.alpha}*x.beta};
  require(result.denominator!=0,"parallel forms cannot define a nonconstant event");
  if(result.denominator<0) {
    result.xi=-result.xi; result.eta=-result.eta; result.denominator=-result.denominator;
  }
  const I divisor=local_gcd(local_gcd(local_abs(result.xi),local_abs(result.eta)),result.denominator);
  require(divisor>0,"invalid center-coordinate gcd");
  result.xi/=divisor; result.eta/=divisor; result.denominator/=divisor;
  return result;
}

bool local_contains(const Cell& cell,const LocalRoot& root,I64 Q) {
  // Node bounds are alpha,beta=Q*(xi,eta). Closed comparisons keep contacts.
  const I xi=I{Q}*root.xi,eta=I{Q}*root.eta;
  return I{cell.al}*root.denominator<=xi && xi<=I{cell.ah}*root.denominator &&
         I{cell.bl}*root.denominator<=eta && eta<=I{cell.bh}*root.denominator;
}

class LocalMap {
 public:
  LocalMap(const Preparation& preparation,U64 threshold,unsigned depth,std::size_t budget,LocalWork& work)
      : p_(preparation),threshold_(threshold),depth_(depth),budget_(budget),work_(work) {
    nodes_.reserve(std::min<std::size_t>(budget,4096));
    nodes_.push_back(LocalNode{{-2*p_.Q,2*p_.Q,-2*p_.Q,2*p_.Q},
                              {absent,absent,absent,absent},{},0,Kind::Unknown,0});
  }
  void build() {
    require(p_.cover.size()>=2 && p_.cover[0]==0 && p_.cover[1]==1,
            "initial cover must retain the original endpoints first");
    observe_storage();
    build_node(0,std::span<const std::size_t>(p_.cover).subspan(2),0);
    require(work_.temporary_ids==0 && work_.temporary_capacity_slots==0,
            "temporary active-list ownership ledger did not close");
  }
  template<class Consumer> bool query(std::size_t seed,const Consumer& consumer) {
    bool touched=false;
    visit(0,p_.forms[seed],consumer,touched);
    return touched;
  }
  const LocalNode& node(std::size_t id) const { return nodes_[id]; }
  std::size_t size() const { return nodes_.size(); }
  std::size_t capacity() const { return nodes_.capacity(); }
  std::size_t owner(const LocalRoot& root) {
    if(!local_contains(nodes_[0].cell,root,p_.Q)) return absent;
    std::size_t id=0;
    for(;;) {
      add(work_.root_location_visits);
      const auto& current=nodes_[id];
      if(current.kind!=Kind::Internal) return id;
      const I64 middle_a=current.cell.al+(current.cell.ah-current.cell.al)/2;
      const I64 middle_b=current.cell.bl+(current.cell.bh-current.cell.bl)/2;
      const bool right=I{p_.Q}*root.xi>=I{middle_a}*root.denominator;
      const bool upper=I{p_.Q}*root.eta>=I{middle_b}*root.denominator;
      id=current.children[static_cast<std::size_t>(right)+2*static_cast<std::size_t>(upper)];
    }
  }
 private:
  void observe_storage() {
    work_.peak_temporary_ids=std::max(work_.peak_temporary_ids,work_.temporary_ids);
    work_.peak_temporary_capacity_slots=std::max(work_.peak_temporary_capacity_slots,work_.temporary_capacity_slots);
    U64 ids=work_.A,capacity=work_.retained_active_capacity_slots;
    add(ids,work_.temporary_ids); add(ids,as_u64(p_.cover.size()));
    add(capacity,work_.temporary_capacity_slots); add(capacity,as_u64(p_.cover.capacity()));
    work_.peak_live_active_ids=std::max(work_.peak_live_active_ids,ids);
    work_.peak_live_active_capacity_slots=std::max(work_.peak_live_active_capacity_slots,capacity);
  }
  struct Temporary {
    LocalMap& owner;
    std::vector<std::size_t> ids;
    U64 tracked_size{},tracked_capacity{};
    explicit Temporary(LocalMap& map):owner(map) {}
    void push(std::size_t id) {
      ids.push_back(id);
      add(owner.work_.temporary_ids);
      add(owner.work_.temporary_capacity_slots,as_u64(ids.capacity())-tracked_capacity);
      tracked_size=as_u64(ids.size()); tracked_capacity=as_u64(ids.capacity());
      owner.observe_storage();
    }
    void retain(LocalNode& leaf) {
      leaf.active=std::move(ids);
      require(leaf.active.size()==tracked_size && leaf.active.capacity()==tracked_capacity,
              "leaf ownership transfer changed active-list storage");
      owner.work_.temporary_ids-=tracked_size;
      owner.work_.temporary_capacity_slots-=tracked_capacity;
      add(owner.work_.A,tracked_size);
      add(owner.work_.retained_active_capacity_slots,tracked_capacity);
      owner.work_.max_leaf_active_ids=std::max(owner.work_.max_leaf_active_ids,tracked_size);
      tracked_size=0; tracked_capacity=0;
      owner.observe_storage();
    }
    ~Temporary() {
      owner.work_.temporary_ids-=tracked_size;
      owner.work_.temporary_capacity_slots-=tracked_capacity;
    }
  };
  bool outside(const Cell& cell) {
    auto& stats=work_.spatial;
    add(stats.disk_box_tests);
    std::array<I,3> low{},high{};
    bool first=true;
    for(const auto alpha:{cell.al,cell.ah}) for(const auto beta:{cell.bl,cell.bh}) {
      for(std::size_t axis=0;axis<3;++axis) {
        const I value=I{p_.A[axis]}*alpha+I{p_.B[axis]}*beta;
        if(first) low[axis]=high[axis]=value;
        else { low[axis]=std::min(low[axis],value); high[axis]=std::max(high[axis],value); }
      }
      first=false;
    }
    I norm=0;
    for(std::size_t axis=0;axis<3;++axis) {
      const I nearest=low[axis]>0?low[axis]:(high[axis]<0?high[axis]:0);
      norm+=nearest*nearest;
    }
    if(2*norm>I{p_.Q}*p_.Q*p_.D) { add(stats.out_disk); return true; }
    for(const auto& side:p_.sides) {
      add(stats.hull_side_tests);
      I ca=0,cb=0;
      for(std::size_t axis=0;axis<3;++axis) {
        ca+=I{side.normal[axis]}*p_.A[axis]; cb+=I{side.normal[axis]}*p_.B[axis];
      }
      const I minimum=ca*(ca<0?cell.ah:cell.al)+cb*(cb<0?cell.bh:cell.bl);
      if(minimum>side.height*p_.Q) { add(stats.out_hull); return true; }
    }
    return false;
  }
  void build_node(std::size_t id,std::span<const std::size_t> candidates,U64 inside) {
    auto& stats=work_.spatial;
    add(stats.cell_visits);
    const auto cell=nodes_[id].cell;
    const auto depth=nodes_[id].depth;
    stats.max_depth=std::max(stats.max_depth,static_cast<U64>(depth));
    if(outside(cell)) { nodes_[id].kind=Kind::Out; return; }
    Temporary active(*this);
    for(const auto point:candidates) {
      add(stats.plane_cell_tests);
      const auto [minimum,maximum]=bounds(p_.forms[point],cell,p_.Q);
      if(maximum<0) {
        add(inside);
        if(inside>=threshold_) { nodes_[id].kind=Kind::Deep; add(stats.deep); return; }
      } else if(minimum<=0) active.push(point);
      // STRICT outside only. min==0 must remain available for the full shell,
      // including when a family line touches only a cell side or corner.
    }
    if(active.ids.empty() || depth>=depth_ || budget_-nodes_.size()<4) {
      nodes_[id].inside=inside;
      active.retain(nodes_[id]);
      add(stats.unknown);
      return;
    }
    const I64 am=cell.al+(cell.ah-cell.al)/2,bm=cell.bl+(cell.bh-cell.bl)/2;
    const std::array<Cell,4> cells{{{cell.al,am,cell.bl,bm},{am,cell.ah,cell.bl,bm},
                                    {cell.al,am,bm,cell.bh},{am,cell.ah,bm,cell.bh}}};
    std::array<std::size_t,4> children{};
    for(std::size_t child=0;child<4;++child) {
      children[child]=nodes_.size();
      nodes_.push_back(LocalNode{cells[child],{absent,absent,absent,absent},{},0,Kind::Unknown,
                                static_cast<unsigned char>(depth+1)});
    }
    nodes_[id].children=children; nodes_[id].kind=Kind::Internal;
    add(stats.internal);
    // All children read the same immutable parent list. It stays alive until
    // all four return; leaves own their new vectors instead of borrowing it.
    for(const auto child:children) build_node(child,active.ids,inside);
  }
  template<class Consumer> void visit(std::size_t id,const Form& line,const Consumer& consumer,bool& touched) {
    auto& stats=work_.spatial;
    add(stats.query_visits); add(stats.query_line_tests);
    const auto& current=nodes_[id];
    const auto [minimum,maximum]=bounds(line,current.cell,p_.Q);
    if(minimum>0 || maximum<0) { add(stats.query_no_intersection); return; }
    if(current.kind==Kind::Out) { add(stats.query_out); return; }
    if(current.kind==Kind::Deep) { add(stats.query_deep); return; }
    if(current.kind==Kind::Unknown) {
      touched=true; add(stats.query_unknown); add(work_.I); add(work_.W,as_u64(current.active.size()));
      consumer(id); return;
    }
    for(const auto child:current.children) visit(child,line,consumer,touched);
  }
  const Preparation& p_;
  U64 threshold_;
  unsigned depth_;
  std::size_t budget_;
  LocalWork& work_;
  std::vector<LocalNode> nodes_;
};

struct LocalOrderedForm { I c{},a{},b{}; };
LocalOrderedForm local_ordered(const Form& form,bool swap) {
  return {form.constant,swap?form.beta:form.alpha,swap?form.alpha:form.beta};
}
I local_determinant(const LocalOrderedForm& x,const LocalOrderedForm& z,const LocalOrderedForm& w) {
  // Six terms bounded by 12*8*8*M^6 apiece: total <4608*M^6<2^109.
  // Do not replace this by cross products of the O(M^4) reduced p/s values.
  return x.c*(z.a*w.b-z.b*w.a)-x.a*(z.c*w.b-z.b*w.c)+x.b*(z.c*w.a-z.a*w.c);
}
I local_s(const LocalOrderedForm& x,const LocalOrderedForm& z) { return z.a*x.b-z.b*x.a; }
I local_p(const LocalOrderedForm& x,const LocalOrderedForm& z) { return z.c*x.b-z.b*x.c; }

struct LocalRecord {
  std::size_t seed{};
  std::string xi,eta,den;
  U64 depth{};
  std::vector<std::size_t> shell;
};

class LocalDigest {
 public:
  void accept(std::size_t seed,const LocalRoot& root,U64 depth,std::span<const std::size_t> shell) {
    std::ostringstream canonical;
    canonical<<seed<<'|'<<local_decimal(root.xi)<<'|'<<local_decimal(root.eta)<<'|'
             <<local_decimal(root.denominator)<<'|'<<depth<<'|';
    for(const auto id:shell) canonical<<id<<',';
    const auto hash=sha256(canonical.str());
    std::array<std::uint32_t,8> words{};
    for(std::size_t i=0;i<8;++i) {
      const auto start=hash.data()+8*i;
      const auto parsed=std::from_chars(start,start+8,words[i],16);
      require(parsed.ec==std::errc{} && parsed.ptr==start+8,"invalid record SHA-256 encoding");
      xor_[i]^=words[i];
    }
    U64 carry=0;
    for(std::size_t i=8;i>0;--i) {
      const U64 value=U64{sum_[i-1]}+words[i-1]+carry;
      sum_[i-1]=static_cast<std::uint32_t>(value&0xffffffffULL); carry=value>>32U;
    }
    add(count_);
  }
  void json() const {
    std::cout<<"{\"count\":"<<count_<<",\"sum_sha256_mod_2_256\":\""<<hex(sum_)
             <<"\",\"xor_sha256\":\""<<hex(xor_)<<"\"}";
  }
 private:
  static std::string hex(const std::array<std::uint32_t,8>& words) {
    std::ostringstream out; out<<std::hex<<std::setfill('0');
    for(const auto word:words) out<<std::setw(8)<<word;
    return out.str();
  }
  std::array<std::uint32_t,8> sum_{},xor_{};
  U64 count_{};
};

class LocalSweep {
 public:
  LocalSweep(const Preparation& preparation,LocalMap& map,U64 threshold,unsigned mode,LocalWork& work)
      :p_(preparation),map_(map),threshold_(threshold),mode_(mode),work_(work) {}
  void run(std::size_t seed,std::size_t leaf_id) {
    const auto started=Clock::now();
    const auto& leaf=map_.node(leaf_id);
    require(leaf.kind==Kind::Unknown,"local sweep requires a materialized UNKNOWN leaf");
    events_.clear(); constants_.clear(); shell_.clear();
    constants_.push_back(0); constants_.push_back(1);  // Global endpoint shell, never witnesses.
    const auto& seed_form=p_.forms[seed];
    const bool swap=seed_form.beta==0;
    const auto x=local_ordered(seed_form,swap);
    require(x.b!=0,"acute seed has no nonzero center-plane line coefficient");
    U64 population=leaf.inside;
    for(const auto id:leaf.active) {
      require(id>=2,"endpoint duplicated in leaf-active list");
      const auto z=local_ordered(p_.forms[id],swap);
      const I side=local_s(x,z);
      if(side!=0) {
        events_.push_back(id); add(work_.events);
        if(local_sign(side)==local_sign(x.b)) add(population);  // Exit: interior at -infinity.
      } else {
        const int sign=local_sign(local_p(x,z))*local_sign(x.b);
        if(sign<0) { add(population); add(work_.constant_inside); }
        else if(sign==0) { constants_.push_back(id); add(work_.constant_shell_ids); }
        else add(work_.constant_outside);
      }
    }
    require(population<=p_.cover.size(),"local initial population exceeded the cover");
    const auto compare=[&](std::size_t left,std::size_t right) {
      const auto z=local_ordered(p_.forms[left],swap),w=local_ordered(p_.forms[right],swap);
      const I sz=local_s(x,z),sw=local_s(x,w);
      require(sz!=0 && sw!=0,"constant form reached the local event comparator");
      return -local_sign(x.b)*local_sign(local_determinant(x,z,w))*local_sign(sz)*local_sign(sw);
    };
    std::sort(events_.begin(),events_.end(),[&](std::size_t left,std::size_t right) {
      add(work_.sort_comparisons);
      const int order=compare(left,right);
      return order<0 || (order==0 && left<right);
    });
    for(std::size_t first=0;first<events_.size();) {
      std::size_t last=first+1;
      while(last<events_.size()) {
        add(work_.group_comparisons);
        if(compare(events_[first],events_[last])!=0) break;
        ++last;
      }
      U64 entries=0,exits=0;
      for(auto position=first;position<last;++position) {
        add(work_.event_side_tests);
        const I side=local_s(x,local_ordered(p_.forms[events_[position]],swap));
        if(local_sign(side)==local_sign(x.b)) add(exits); else add(entries);
      }
      require(exits<=population,"local sweep exit underflow");
      population-=exits;
      add(work_.groups);
      const auto root=local_root(seed_form,p_.forms[events_[first]]);
      if(!local_contains(leaf.cell,root,p_.Q)) add(work_.groups_outside_cell);
      else if(map_.owner(root)!=leaf_id) add(work_.groups_not_owned);
      else {
        add(work_.owned_groups);
        if(population<threshold_) {
          add(work_.lowdepthgroups);
          shell_.assign(constants_.begin(),constants_.end());
          shell_.insert(shell_.end(),events_.begin()+static_cast<std::ptrdiff_t>(first),
                        events_.begin()+static_cast<std::ptrdiff_t>(last));
          std::sort(shell_.begin(),shell_.end(),[&](std::size_t a,std::size_t b) {
            add(work_.shell_sort_comparisons); return a<b;
          });
          require(std::adjacent_find(shell_.begin(),shell_.end())==shell_.end(),"duplicate local shell ID");
          require(shell_.size()<=p_.cover.size() && population<=p_.cover.size()-shell_.size(),
                  "local depth and shell do not form disjoint cover classes");
          add(work_.shell_ids,as_u64(shell_.size()));
          digest_.accept(seed,root,population,shell_);
          if(mode_==2) {
            records_.push_back({seed,local_decimal(root.xi),local_decimal(root.eta),
                                local_decimal(root.denominator),population,shell_});
            const auto& record=records_.back();
            add(work_.retained_record_shell_capacity_slots,as_u64(record.shell.capacity()));
            add(work_.retained_record_string_capacity_bytes,as_u64(record.xi.capacity()));
            add(work_.retained_record_string_capacity_bytes,as_u64(record.eta.capacity()));
            add(work_.retained_record_string_capacity_bytes,as_u64(record.den.capacity()));
          }
        }
      }
      require(entries<=p_.cover.size()-population,"local sweep entry overflow");
      population+=entries;
      first=last;
    }
    U64 slots=as_u64(events_.capacity()); add(slots,as_u64(constants_.capacity())); add(slots,as_u64(shell_.capacity()));
    require(slots<=std::numeric_limits<U64>::max()/sizeof(std::size_t),"local scratch byte count overflow");
    work_.scratch_peak_capacity_bytes=std::max(work_.scratch_peak_capacity_bytes,slots*sizeof(std::size_t));
    work_.sweep_ms+=ms(started,Clock::now());
  }
  void digest_json() const { digest_.json(); }
  std::size_t record_capacity() const { return records_.capacity(); }
  void records_json() const {
    std::cout<<'[';
    for(std::size_t pos=0;pos<records_.size();++pos) {
      if(pos!=0) std::cout<<',';
      const auto& record=records_[pos];
      std::cout<<"{\"seed\":"<<record.seed<<",\"xi_num\":\""<<record.xi<<"\",\"eta_num\":\""
               <<record.eta<<"\",\"den\":\""<<record.den<<"\",\"depth\":"<<record.depth<<",\"shell\":[";
      for(std::size_t i=0;i<record.shell.size();++i) { if(i!=0)std::cout<<','; std::cout<<record.shell[i]; }
      std::cout<<"]}";
    }
    std::cout<<']';
  }
 private:
  const Preparation& p_;
  LocalMap& map_;
  U64 threshold_;
  unsigned mode_;
  LocalWork& work_;
  std::vector<std::size_t> events_,constants_,shell_;
  std::vector<LocalRecord> records_;
  LocalDigest digest_;
};

int local_run(int argc,char** argv) {
  require(argc==7,"usage: local_sweeps input.txt K depth node_budget domain(0|1) mode(0|1|2)");
  const auto started=Clock::now();
  const U64 K=argument(argv[2]),depth_arg=argument(argv[3]),budget_arg=argument(argv[4]);
  const U64 domain=argument(argv[5]),mode_arg=argument(argv[6]);
  require(K>=3 && depth_arg<=10 && budget_arg>=1 && budget_arg<=std::numeric_limits<std::size_t>::max() &&
          domain<=1 && mode_arg<=2,"invalid local sweep CLI domain");
  const auto depth=static_cast<unsigned>(depth_arg),mode=static_cast<unsigned>(mode_arg);
  const auto budget=static_cast<std::size_t>(budget_arg);
  auto input=read_input(argv[1]);
  const auto input_done=Clock::now();
  LocalWork work;
  const auto prep=prepare(input.points,depth,domain==1,work.spatial);
  const auto prep_done=Clock::now();
  LocalMap map(prep,K-2,depth,budget,work);
  map.build();
  const auto map_done=Clock::now();
  LocalSweep sweep(prep,map,K-2,mode,work);
  U64 survivors=0;
  std::vector<std::size_t> rejected;
  for(const auto seed:prep.seeds) {
    const bool touched=map.query(seed,[&](std::size_t leaf) { if(mode!=0) sweep.run(seed,leaf); });
    if(touched) add(survivors); else rejected.push_back(seed);
  }
  const auto query_done=Clock::now();
  const auto& stats=work.spatial;
  require(map.size()<=budget && stats.cell_visits==map.size(),"local tree budget or visit ledger failed");
  require(stats.out_disk+stats.out_hull+stats.deep+stats.unknown+stats.internal==map.size(),"local tree-kind ledger failed");
  require(work.I==stats.query_unknown,"local incidence ledger failed");
  require(work.groups==work.groups_outside_cell+work.groups_not_owned+work.owned_groups,"local group ownership ledger failed");
  require(work.lowdepthgroups<=work.owned_groups,"local shallow group ledger failed");
  require(mode!=0 || (work.events==0 && work.groups==0 && work.lowdepthgroups==0),"count-only mode executed a sweep");
  require(survivors+rejected.size()==prep.seeds.size(),"local surviving-seed ledger failed");
  std::cout<<std::fixed<<std::setprecision(6)
    <<"{\"schema\":\"mhgp8_audit_q4_local_sweeps_v1\",\"status\":\"passed\","
      "\"phase\":\"independent_local_sweep_prototype\",\"public_status\":\"not_claimed\","
      "\"product_code_used\":false,\"positive_support_filter_executed\":false,"
      "\"helper_source_commit\":\"2920b8b5\",\"coordinate_system\":\"t=A*xi+B*eta\","
      "\"output_contract\":\"shallow_covered_roots; depth and shell concern the edge cover, not arbitrary global spheres\","
    <<"\"input_sha256\":\""<<input.hash<<"\",\"input_bytes\":"<<input.input_bytes
    <<",\"n\":"<<input.points.size()<<",\"kmax\":"<<K<<",\"depth\":"<<depth
    <<",\"node_budget\":"<<budget<<",\"domain\":"<<domain<<",\"mode\":"<<mode
    <<",\"Q\":"<<prep.Q<<",\"D\":"<<prep.D<<",\"basis_axis\":"<<prep.axis<<",\"basis_A\":";
  vector_json(prep.A); std::cout<<",\"basis_B\":"; vector_json(prep.B);
  std::cout<<",\"hull_degenerate\":"<<(prep.hull_degenerate?"true":"false")
    <<",\"cover_sites\":"<<prep.cover.size()<<",\"seeds\":"<<prep.seeds.size()
    <<",\"rejected\":"<<rejected.size()<<",\"survivors\":"<<survivors
    <<",\"forecast_scan_reads\":"<<decimal(U{prep.cover.size()}*survivors)
    <<",\"shallow_covered_roots\":"<<work.lowdepthgroups<<",\"rejected_seed_ids\":[";
  for(std::size_t pos=0;pos<rejected.size();++pos) { if(pos!=0)std::cout<<','; std::cout<<rejected[pos]; }
  std::cout<<"],\"work\":{";
#define LOCAL_FIELD(name) std::cout<<"\"" #name "\":"<<work.name<<','
  LOCAL_FIELD(I); LOCAL_FIELD(W); LOCAL_FIELD(A); LOCAL_FIELD(max_leaf_active_ids);
  LOCAL_FIELD(retained_active_capacity_slots); LOCAL_FIELD(peak_temporary_ids); LOCAL_FIELD(peak_temporary_capacity_slots);
  LOCAL_FIELD(peak_live_active_ids); LOCAL_FIELD(peak_live_active_capacity_slots);
  LOCAL_FIELD(events); LOCAL_FIELD(constant_inside); LOCAL_FIELD(constant_shell_ids); LOCAL_FIELD(constant_outside);
  LOCAL_FIELD(sort_comparisons); LOCAL_FIELD(group_comparisons); LOCAL_FIELD(event_side_tests); LOCAL_FIELD(groups);
  LOCAL_FIELD(groups_outside_cell); LOCAL_FIELD(groups_not_owned); LOCAL_FIELD(owned_groups); LOCAL_FIELD(lowdepthgroups);
  LOCAL_FIELD(root_location_visits); LOCAL_FIELD(shell_ids); LOCAL_FIELD(shell_sort_comparisons);
#undef LOCAL_FIELD
#define LOCAL_SPATIAL(name) std::cout<<"\"" #name "\":"<<stats.name<<','
  LOCAL_SPATIAL(prep_cover_tests); LOCAL_SPATIAL(prep_lens_tests); LOCAL_SPATIAL(prep_form_sites); LOCAL_SPATIAL(prep_seed_tests);
  LOCAL_SPATIAL(eligible_lens_sites); LOCAL_SPATIAL(hull_projection_points); LOCAL_SPATIAL(hull_turn_tests); LOCAL_SPATIAL(hull_sides);
  LOCAL_SPATIAL(cell_visits); LOCAL_SPATIAL(plane_cell_tests); LOCAL_SPATIAL(disk_box_tests); LOCAL_SPATIAL(hull_side_tests);
  LOCAL_SPATIAL(out_disk); LOCAL_SPATIAL(out_hull); LOCAL_SPATIAL(deep); LOCAL_SPATIAL(unknown); LOCAL_SPATIAL(internal); LOCAL_SPATIAL(max_depth);
  LOCAL_SPATIAL(query_visits); LOCAL_SPATIAL(query_line_tests); LOCAL_SPATIAL(query_no_intersection); LOCAL_SPATIAL(query_out); LOCAL_SPATIAL(query_deep);
#undef LOCAL_SPATIAL
  std::cout<<"\"query_unknown\":"<<stats.query_unknown<<"},\"memory\":{"
    <<"\"tree_nodes\":"<<map.size()<<",\"tree_capacity\":"<<map.capacity()<<",\"node_bytes\":"<<sizeof(LocalNode)
    <<",\"tree_capacity_bytes\":"<<decimal(U{map.capacity()}*sizeof(LocalNode))
    <<",\"active_leaf_capacity_bytes\":"<<decimal(U{work.retained_active_capacity_slots}*sizeof(std::size_t))
    <<",\"active_peak_capacity_bytes\":"<<decimal(U{work.peak_live_active_capacity_slots}*sizeof(std::size_t))
    <<",\"form_capacity_bytes\":"<<decimal(U{prep.forms.capacity()}*sizeof(Form))
    <<",\"cover_id_capacity_bytes\":"<<decimal(U{prep.cover.capacity()}*sizeof(std::size_t))
    <<",\"seed_id_capacity_bytes\":"<<decimal(U{prep.seeds.capacity()}*sizeof(std::size_t))
    <<",\"rejected_id_capacity_bytes\":"<<decimal(U{rejected.capacity()}*sizeof(std::size_t))
    <<",\"point_capacity_bytes\":"<<decimal(U{input.points.capacity()}*sizeof(Point))
    <<",\"hull_side_capacity_bytes\":"<<decimal(U{prep.sides.capacity()}*sizeof(Side))
    <<",\"sweep_scratch_capacity_bytes\":"<<work.scratch_peak_capacity_bytes
    <<",\"detail_record_capacity_bytes\":"<<decimal(U{sweep.record_capacity()}*sizeof(LocalRecord))
    <<",\"detail_shell_capacity_bytes\":"<<decimal(U{work.retained_record_shell_capacity_slots}*sizeof(std::size_t))
    <<",\"detail_string_capacity_sum\":"<<work.retained_record_string_capacity_bytes
    <<",\"scope\":\"capacities, not RSS; active peak sums retained leaves, simultaneous parent temporaries and original cover including endpoints; cover is already included in that peak; excludes allocator metadata, transient reallocation, stacks, hashing/JSON temporaries and input parsing; detail string capacities may use inline storage already counted in record slots\"}"
    <<",\"root_digest\":";
  sweep.digest_json();
  std::cout<<",\"roots\":"; sweep.records_json();
  std::cout<<",\"timings_ms\":{\"input_read_hash_validate\":"<<ms(started,input_done)
    <<",\"preparation\":"<<ms(input_done,prep_done)<<",\"map_build\":"<<ms(prep_done,map_done)
    <<",\"queries_and_sweeps\":"<<ms(map_done,query_done)<<",\"sweep_included\":"<<work.sweep_ms
    <<",\"total_before_json\":"<<ms(started,query_done)
    <<"},\"timing_scope\":\"sweep_included is included in queries_and_sweeps; retained-output JSON and object destruction are excluded; no full-cover fallback or positive support cascade executed\"}\n";
  return 0;
}
}  // namespace

int main(int argc,char** argv) {
  try { return local_run(argc,argv); }
  catch(const std::exception& error) { std::cerr<<"local_sweeps: "<<error.what()<<'\n'; return 1; }
}

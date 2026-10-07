#include "index/index.hpp"
#include "num/guard.hpp"
#include "num/local.hpp"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <type_traits>
using namespace mhgp12;
using namespace mhgp12::num;
static_assert(!std::is_constructible_v<CertifiedBall,Sphere>);
static_assert(!std::is_constructible_v<GuardedSphere,Sphere>);
std::string decimal(u128 n){std::string s;do{s.push_back(char('0'+n%10));n/=10;}while(n);return std::string(s.rbegin(),s.rend());}
template<class T>T take(Result<T>&& r){if(!r.ok())throw std::runtime_error(std::string(reason_name(r.outcome().reason)));return std::move(r.value());}
Point readpoint(){i64 x,y,z;std::cin>>x>>y>>z;return take(Point::make(x,y,z));}
std::vector<Point> support(){int q;std::cin>>q;std::vector<Point> s;for(int j=0;j<q;++j)s.push_back(readpoint());return s;}
Sphere sphere(const std::vector<Point>& s){if(s.size()==1)return Sphere::point(s[0]);std::optional<Sphere> p;if(s.size()==2)p=take(Sphere::through(s[0],s[1]));if(s.size()==3)p=take(Sphere::through(s[0],s[1],s[2]));if(s.size()==4)p=take(Sphere::through(s[0],s[1],s[2],s[3]));if(!p)throw std::runtime_error("degenerate");return *p;}
void counts(const LaneCount& c){std::cout<<' '<<c.native<<' '<<c.certified<<' '<<c.checked<<' '<<c.wide;}
struct Capture{std::vector<SiteIdx> i,u;CensusKind kind;CensusLedger ledger;static Outcome call(void* p,const BorrowedCensus& c){auto& x=*static_cast<Capture*>(p);x.i.assign(c.interior().begin(),c.interior().end());x.u.assign(c.shell().begin(),c.shell().end());x.kind=c.kind();x.ledger=c.ledger();return {};}};
int main(){try{char op;while(std::cin>>op){if(op=='D'){auto a=readpoint(),b=readpoint();LaneCount l;auto d=squared_distance(a,b,&l);std::cout<<"D "<<decimal(d);counts(l);}
else if(op=='M'){auto a=readpoint();std::cout<<"M "<<decimal(morton_key(a.x(),a.y(),a.z()));}
else if(op=='R'){std::array<u64,3> l,h,b,e;for(auto& v:l)std::cin>>v;for(auto& v:h)std::cin>>v;auto p=readpoint();for(auto& v:b)std::cin>>v;for(auto& v:e)std::cin>>v;Frame f;auto ok=f.add_box(l,h);if(!ok.ok())throw std::runtime_error("frame");LaneCount lane;auto r=reservoir_distance(f,p.coordinates(),b,e,&lane);std::cout<<"R "<<f.span()<<' '<<(r.ok()?decimal(r.value()):reason_name(r.outcome().reason));counts(lane);}
else if(op=='Q'){auto s=support();auto p=readpoint(),lo=readpoint(),hi=readpoint();auto box=take(Box::make(lo,hi));auto ball=take(CertifiedBall::certify(s));auto raw=sphere(s);auto plain=take(side(raw,p));std::cout<<"Q "<<bool(ball)<<' '<<plain;if(ball){GuardedSphere g(*ball);GuardLedger l;auto sideval=take(g.side(p,&l));auto bounds=take(g.bound_signs(box,&l));std::cout<<' '<<sideval<<' '<<bounds.lower<<' '<<bounds.upper<<' '<<ball->span()<<' '<<l.disjoint_boxes<<' '<<l.partial_boxes<<' '<<l.outside_sites;counts(l.lanes);}}
else if(op=='H'||op=='C'){std::vector<Point> s;if(op=='C')s=support();int n;u32 leaf=1,threshold=kNone;std::cin>>n;if(op=='C')std::cin>>leaf>>threshold;std::vector<u32>x,y,z;std::vector<PointId>ids;for(int j=0;j<n;++j){auto p=readpoint();u32 id;std::cin>>id;x.push_back(p.x());y.push_back(p.y());z.push_back(p.z());ids.push_back(PointId{id});}MemoryBudget budget(MemoryBudget::kUnlimited);auto cloud=take(prepare_cloud(x,y,z,ids,CoordWidth{},budget));std::cout<<op<<' '<<cloud.sites();if(op=='H'){for(u32 j=0;j<cloud.sites();++j){std::cout<<' '<<cloud.x()[j]<<' '<<cloud.y()[j]<<' '<<cloud.z()[j]<<' '<<cloud.w()[j];for(auto id:cloud.points(SiteIdx{j}))std::cout<<' '<<idx(id);}}
else{auto ball=take(CertifiedBall::certify(s));auto index=take(build_index(std::move(cloud),{leaf},budget));auto raw=sphere(s);auto gen=take(census(index,raw,threshold,budget));auto result=ball?take(census(index,*ball,threshold,budget)):take(census(index,raw,threshold,budget));auto ws=take(CensusWorkspace::make(index,budget));Capture c;auto status=ball?ws->query(index,*ball,threshold,&c,Capture::call):ws->query(index,raw,threshold,&c,Capture::call);if(!status.ok())throw std::runtime_error("workspace");bool equal=gen.kind()==result.kind()&&std::equal(gen.interior().begin(),gen.interior().end(),result.interior().begin(),result.interior().end())&&std::equal(gen.shell().begin(),gen.shell().end(),result.shell().begin(),result.shell().end())&&c.kind==result.kind()&&std::equal(c.i.begin(),c.i.end(),result.interior().begin(),result.interior().end())&&std::equal(c.u.begin(),c.u.end(),result.shell().begin(),result.shell().end());std::cout<<' '<<bool(ball)<<' '<<equal<<' '<<(result.kind()==CensusKind::saturated)<<' '<<result.interior().size()<<' '<<result.shell().size();for(auto i:result.interior())std::cout<<' '<<idx(index.cloud().points(i)[0]);for(auto i:result.shell())std::cout<<' '<<idx(index.cloud().points(i)[0]);const auto& l=result.ledger();std::cout<<' '<<l.guard_disjoint<<' '<<l.guard_partial<<' '<<l.guard_outside;counts(l.lanes);}}
else {throw std::runtime_error("operation");}
std::cout<<'\n';if(!std::cin)throw std::runtime_error("input");}return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}

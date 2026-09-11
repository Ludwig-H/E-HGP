#define main inherited_main
#include "meb_hybrid.cpp"
#undef main
#include <random>
int main() {
 std::mt19937_64 rng(20260911); unsigned long long checked=0;
 for(unsigned kind=0;kind<8;++kind) for(unsigned t=0;t<150000;++t){
   std::vector<P3> s; unsigned n=7+rng()%4;
   while(s.size()<n){
     int m=kind<4?4:kind<6?9:65536;
     P3 p{int(rng()%m),int(rng()%m),int(rng()%m)};
     if(kind==0||kind==4)p.z=0;
     if(kind==1)p.z=p.x+p.y;
     if(kind==2){p.x=2*p.x;p.y=2*p.y;p.z=2*p.z;}
     if(kind==3)p.z=0,p.y=p.x*p.x;
     if(std::find(s.begin(),s.end(),p)==s.end())s.push_back(p);
     if(kind==3&&s.size()==4){for(int j=4;j<10;++j)s.push_back(P3{j,j*j,0});s.resize(n);}
   }
   AnchorMebWork w;Counters c;auto a=anchor_meb(s,w);auto b=hybrid(s,c);++checked;
   if(!same(a,b)){
     printf("DIVERGENCE kind=%u iteration=%u checked=%llu reference=%d %s hybrid=%d %s\n",kind,t,checked,int(a.status),a.reason,int(b.status),b.reason);
     for(auto p:s)printf("{%d,%d,%d},",p.x,p.y,p.z);puts("");
     Candidate out;Counters c2;bool ok=welzl(s,out,c2);printf("welzl_ok=%d powers=",ok);if(ok)for(auto p:s)printf("%lld,",(long long)out.power(p));puts("");return 1;
   }
 }
 printf("PASS checked=%llu\n",checked);return 0;
}

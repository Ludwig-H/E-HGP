#define main inherited_main
#include "meb_hybrid.cpp"
#undef main
int main() {
 const std::vector<P3> original{{2,3,2},{2,0,0},{0,2,2},{1,0,0},{2,2,0},{3,0,1},{0,2,3},{2,2,1},{2,2,3},{1,2,1}};
 unsigned bad=0;
 for(unsigned n=7;n<=10;++n)for(unsigned mask=0;mask<1024;++mask){
   if(__builtin_popcount(mask)!=int(n))continue;
   std::vector<P3>s;for(unsigned i=0;i<10;++i)if(mask&(1u<<i))s.push_back(original[i]);
   AnchorMebWork w;Counters c;auto a=anchor_meb(s,w);auto b=hybrid(s,c);
   if(!same(a,b)) {
     ++bad;printf("FAIL n=%u mask=%u reference=%d:%s hybrid=%d:%s\n",n,mask,int(a.status),a.reason,int(b.status),b.reason);
     for(auto p:s)printf("{%d,%d,%d},",p.x,p.y,p.z);puts("");
     printf("reference support=%u slots=%u,%u,%u,%u shell=%u\n",a.support_size,a.support_slots[0],a.support_slots[1],a.support_slots[2],a.support_slots[3],a.selected_shell_count);
     Candidate out;Counters c2;bool ok=welzl(s,out,c2);printf("welzl_ok=%d powers=",ok);if(ok)for(auto p:s)printf("%lld,",(long long)out.power(p));puts("");
     return 1;
   }
 }
 printf("NO FAILURE bad=%u\n",bad);return 0;
}

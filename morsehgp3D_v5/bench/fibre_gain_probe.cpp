// Le dernier maillon : combien d'APPELS DE PUISSANCE une mort de bloc evite-t-elle
// vraiment ? Un bloc mort n'evite PAS la construction du cover, ni W_3, ni la
// grille — tout cela est par ANCRE et l'ancre survit. Il evite seulement
// l'enumeration et le RESCAN des seeds de ce handle.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "/workspaces/E-HGP/morsehgp3D_v5/src/cloud/families.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/edge_cover.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/q3.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/sector_kill.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/pipeline/generate.hpp"
using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform; int n = 8000, coord = 0; size_t cible = 1500;
  for (int i = 1; i < argc; ++i) { const std::string a = argv[i];
    if (a.rfind("--family=",0)==0) { if(!parse_cloud_family(a.c_str()+9,&family)) return 2; }
    else if (a.rfind("--n=",0)==0) n=std::atoi(a.c_str()+4);
    else if (a.rfind("--blocs=",0)==0) cible=(size_t)std::atoll(a.c_str()+8); else return 2; }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 h3 = lane_h(Lane::kQ3, 11);
  const u64 h_of[3] = {lane_h(Lane::kQ2,11), h3, lane_h(Lane::kQ4,11)};
  std::vector<AliveRect> alive; u64 vis=0, wk=0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &vis, &wk);
  generate_detail::AnchorScratch sc;
  u64 blocs=0;
  for (const AliveRect& ar : alive) { rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes); blocs += sc.handles.size(); }
  if (!blocs) return 3;
  const u64 pas = std::max<u64>(1, blocs / std::max<u64>(1,(u64)cible));

  u64 vus=0, n_blocs=0, tue_auj=0, tue_fibre=0, gagnes=0;
  u64 pw_total=0, pw_evite=0, seeds_total=0, seeds_evites=0;
  for (const AliveRect& ar : alive) {
    const NodeRange ra=ix.range_of(ar.r.a), rb=ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    for (const NodeRef h : sc.handles) {
      if (vus++ % pas) continue;
      const NodeRange rc=ix.range_of(h); const AxisBox bC=ix.box_of(h);
      for (i32 ua=ra.first; ua<=ra.last; ++ua) for (i32 ub=rb.first; ub<=rb.last; ++ub) {
        const P3& pa=ix.upos[(size_t)ua]; const P3& pb=ix.upos[(size_t)ub];
        const i64 D2=p3_norm2(p3_sub(pb,pa)); if (!D2) continue;
        i64 u[3],v[3]; if (!bisector_basis(pa,pb,D2,12,u,v)) continue;
        i64 P[8][3];
        for (int i=0;i<3;++i){P[0][i]=u[i];P[1][i]=u[i]+v[i];P[2][i]=v[i];P[3][i]=-u[i]+v[i];
                             P[4][i]=-u[i];P[5][i]=-u[i]-v[i];P[6][i]=-v[i];P[7][i]=u[i]-v[i];}
        const i64 d[3]={pb.x-pa.x,pb.y-pa.y,pb.z-pa.z}, s2[3]={pa.x+pb.x,pa.y+pb.y,pa.z+pb.z};
        // Intervalle exact de Wperp = D2*w - (w.d)*d sur Box(C), w = 2x - (a+b).
        i64 wlo[3],whi[3];
        for (int i=0;i<3;++i){ wlo[i]=2*bC.lo[i]-s2[i]; whi[i]=2*bC.hi[i]-s2[i]; }
        i64 Wlo[3]={0,0,0},Whi[3]={0,0,0};
        for (int i=0;i<3;++i){
          i64 lo=D2*wlo[i], hi=D2*whi[i];
          for (int j=0;j<3;++j){ const i64 c=-d[i]*d[j];
            lo += std::min(c*wlo[j],c*whi[j]); hi += std::max(c*wlo[j],c*whi[j]); }
          Wlo[i]=lo; Whi[i]=hi; }
        // Secteur k atteignable (test CONSERVATEUR : chaque contrainte separement).
        u8 mask=0;
        for (int k=0;k<8;++k){ const int k2=(k+1)&7; bool a1=false,a2=false;
          // max sur la boite de (P[k] x W).d et de (W x P[k2]).d
          i128 m1=0,m2=0;
          for (int i=0;i<3;++i){ const int j=(i+1)%3, l=(i+2)%3;
            const i128 c1=(i128)P[k][j]*d[l]-(i128)P[k][l]*d[j];      // coefficient de W[i] dans (P[k] x W).d
            const i128 c2=(i128)P[k2][l]*d[j]-(i128)P[k2][j]*d[l];    // coefficient de W[i] dans (W x P[k2]).d
            m1 += std::max(c1*Wlo[i], c1*Whi[i]); m2 += std::max(c2*Wlo[i], c2*Whi[i]); }
          a1 = m1>=0; a2 = m2>=0; if (a1&&a2) mask |= (u8)(1u<<k); }
        anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
        u32 cnt[8]={}; u64 wmin=0;
        anchor_sector_kill(sc.cover, ix.upos, ua, ub, pa, pb, D2, 12, h3, &wmin, cnt);
        u64 m8=~0ull, mr=~0ull;
        for (int k=0;k<8;++k){ m8=std::min(m8,(u64)cnt[k]); if (mask>>k&1) mr=std::min(mr,(u64)cnt[k]); }
        if (mr==~0ull) mr=0;
        const bool ka=m8>=h3, kf=mr>=h3;
        // Cout REEL du bloc : ses seeds, et pour chacun le rescan de profondeur
        // du cover avec sortie anticipee a h3 — exactement ce que la lane fait.
        u64 pw=0, sd=0;
        for (i32 ux=rc.first; ux<=rc.last; ++ux){
          if (ux==ua||ux==ub) continue;
          const P3& px=ix.upos[(size_t)ux];
          if (!is_acute_seed(pa,pb,px,D2,ix.point_id(ua),ix.point_id(ub),ix.point_id(ux))) continue;
          ++sd;
          const Q3Form f=q3_form(pa,pb,px);
          if (f.g<=0) continue;
          u64 prof=0;
          for (const CoverPoint& cz : sc.cover){ ++pw;
            if (q3_power(f, ix.upos[(size_t)cz.u])<0 && ++prof>=h3) break; } }
        ++n_blocs; pw_total+=pw; seeds_total+=sd;
        if (ka) ++tue_auj;
        if (kf) ++tue_fibre;
        if (kf&&!ka){ ++gagnes; pw_evite+=pw; seeds_evites+=sd; }
      }
    }
  }
  std::printf("gain famille=%s n=%d : blocs=%llu ; tues aujourd hui=%llu (%.1f %%) ; avec la fibre=%llu (%.1f %%) ; gagnes=%llu\n",
    cloud_family_name(family), n, (unsigned long long)n_blocs, (unsigned long long)tue_auj,
    n_blocs?100.0*tue_auj/n_blocs:0.0, (unsigned long long)tue_fibre, n_blocs?100.0*tue_fibre/n_blocs:0.0,
    (unsigned long long)gagnes);
  std::printf("  APPELS DE PUISSANCE : total=%llu ; evites par les blocs gagnes=%llu (%.1f %%)\n",
    (unsigned long long)pw_total,(unsigned long long)pw_evite, pw_total?100.0*pw_evite/pw_total:0.0);
  std::printf("  seeds : total=%llu ; evites=%llu (%.1f %%)\n",
    (unsigned long long)seeds_total,(unsigned long long)seeds_evites, seeds_total?100.0*seeds_evites/seeds_total:0.0);
  return 0;
}

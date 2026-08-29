// PLAFOND DE L'ARRANGEMENT SHALLOW, dans l'unite de decision : appels de
// puissance REELLEMENT executes par ancre (rescan de profondeur avec sortie
// anticipee a h_3), contre le cout modele de l'arrangement des droites du plan
// bissecteur, m*log2(m) + kappa*m avec kappa = h_3 - 1 = 8.
//
// L'arrangement remplace le produit seeds x cover par une construction de
// niveaux peu profonds. C'est le SEUL mecanisme identifie qui change la PENTE
// et non la constante. Cette sonde en mesure le plafond avant tout chantier.
// Le facteur constant de l'arrangement est inconnu : on publie donc le bilan
// pour plusieurs constantes C, jamais pour une seule.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "/workspaces/E-HGP/morsehgp3D_v5/src/cloud/families.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/edge_cover.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/q3.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/pipeline/generate.hpp"
using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform; int n = 8000, coord = 0; size_t cible = 20000;
  for (int i = 1; i < argc; ++i) { const std::string a = argv[i];
    if (a.rfind("--family=",0)==0) { if(!parse_cloud_family(a.c_str()+9,&family)) return 2; }
    else if (a.rfind("--n=",0)==0) n=std::atoi(a.c_str()+4);
    else if (a.rfind("--ancres=",0)==0) cible=(size_t)std::atoll(a.c_str()+9); else return 2; }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 h3 = lane_h(Lane::kQ3, 11); const double kappa = (double)h3 - 1.0;
  const u64 h_of[3] = {lane_h(Lane::kQ2,11), h3, lane_h(Lane::kQ4,11)};
  std::vector<AliveRect> alive; u64 vis=0, wk=0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &vis, &wk);
  generate_detail::AnchorScratch sc;
  u64 tot_anc=0;
  for (const AliveRect& ar : alive) { const NodeRange A=ix.range_of(ar.r.a), B=ix.range_of(ar.r.b);
    tot_anc += (u64)(A.last-A.first+1)*(u64)(B.last-B.first+1); }
  const u64 pas = std::max<u64>(1, tot_anc/std::max<u64>(1,(u64)cible));

  u64 vus=0, n_anc=0;
  double reel=0, modele[4]={0,0,0,0};  // C = 1, 2, 5, 10
  const double Cs[4]={1,2,5,10};
  // Repartition par taille de cover : ou est le travail, et ou l'arrangement gagne.
  const int NB=6; const size_t bornes[NB]={16,32,64,128,512,1u<<30};
  double reel_b[NB]={}, mod_b[NB]={}; u64 anc_b[NB]={};
  for (const AliveRect& ar : alive) {
    const NodeRange ra=ix.range_of(ar.r.a), rb=ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    for (i32 ua=ra.first; ua<=ra.last; ++ua) for (i32 ub=rb.first; ub<=rb.last; ++ub) {
      if (vus++ % pas) continue;
      const P3& pa=ix.upos[(size_t)ua]; const P3& pb=ix.upos[(size_t)ub];
      const i64 D2=p3_norm2(p3_sub(pb,pa)); if (!D2) continue;
      anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
      const size_t m = sc.cover.size(); if (m==0) continue;
      u64 pw=0;
      for (const CoverPoint& cx : sc.cover) {
        if (cx.u==ua||cx.u==ub) continue;
        const P3& px=ix.upos[(size_t)cx.u];
        if (!is_acute_seed(pa,pb,px,D2,ix.point_id(ua),ix.point_id(ub),ix.point_id(cx.u))) continue;
        const Q3Form f=q3_form(pa,pb,px);
        if (f.g<=0) continue;
        u64 prof=0;
        for (const CoverPoint& cz : sc.cover) { ++pw;
          if (q3_power(f, ix.upos[(size_t)cz.u])<0 && ++prof>=h3) break; }
      }
      const double mm=(double)m, base=mm*std::log2(std::max(2.0,mm))+kappa*mm;
      ++n_anc; reel+=(double)pw;
      for (int c=0;c<4;++c) modele[c]+=Cs[c]*base;
      int b=0; while (b<NB-1 && m>bornes[b]) ++b;
      reel_b[b]+=(double)pw; mod_b[b]+=base; ++anc_b[b];
    }
  }
  std::printf("shallow famille=%s n=%d : ancres echantillonnees=%llu (pas=%llu sur %llu)\n",
    cloud_family_name(family), n, (unsigned long long)n_anc,(unsigned long long)pas,(unsigned long long)tot_anc);
  std::printf("  appels de puissance REELS = %.0f\n", reel);
  for (int c=0;c<4;++c)
    std::printf("  arrangement a C=%.0f : cout modele %.0f  -> %s (%.2f x)\n", Cs[c], modele[c],
                modele[c]<reel?"GAGNE":"perd", reel/std::max(1.0,modele[c]));
  std::printf("  par taille de cover m :\n");
  const char* nm[NB]={"m<=16","17-32","33-64","65-128","129-512","m>512"};
  for (int b=0;b<NB;++b) if (anc_b[b])
    std::printf("    %-8s ancres=%6llu  reel=%12.0f (%5.1f %% du reel)  modele C=1 : %10.0f  rapport %6.2f x\n",
      nm[b],(unsigned long long)anc_b[b],reel_b[b],100.0*reel_b[b]/std::max(1.0,reel),mod_b[b],
      reel_b[b]/std::max(1.0,mod_b[b]));
  return 0;
}

// Positivite du tetraedre de PRESENTATION : jamais un certificat pour un autre quadruplet.
#pragma once
#include <algorithm>
#include "num/geometry_internal.hpp"

namespace mhgp11::num::detail {

inline bool q4_weights_i128(const std::array<Point,4>& points) noexcept {
  // Le cube commun a ces quatre Point a cote L ; ce n'est pas une borne sur des Vec arbitraires.
  for (u32 j=0;j<3;++j) {
    u32 lo=points[0].coordinates()[j], hi=lo;
    for (u32 i=1;i<4;++i) {
      lo=std::min(lo,points[i].coordinates()[j]); hi=std::max(hi,points[i].coordinates()[j]);
    }
    if (hi-lo>(u32{1}<<20)) return false;
  }
  return true;
}

// N est le numerateur BRUT avant normalisation du signe de det. Avec H=2det^2>0,
// (w0,w1,w2,w3)/H sont les poids barycentriques : wi=N.cross_i et w0=H-somme(wi).
// cross_i communs <ou= L^2 (extrema multiaffines aux coins du carre), |det|<=3L^3,
// |N_j|<=9L^4. face=vs+su+uv est aussi un cross a meme ancrage, donc |face_j|<=L^2.
// H<=18L^6, |N.face| et |N.cross_i|<=27L^6. Tous produits/sommes, y compris H-w0-w1-w2,
// sont <=117L^6. L<=2^20 implique <2^127 AVANT toute multiplication i128 de degre six.
inline bool q4_weights_native(const std::array<CenterInt,3>& n,i128 det,
                              const Vec& face,const Vec& vs,const Vec& su) noexcept {
  const i128 h=2*(det*det);
  const auto scalar=[&](const Vec& normal) noexcept {
    return n[0]*normal[0]+n[1]*normal[1]+n[2]*normal[2];
  };
  const i128 w0=h-scalar(face);
  if (w0<=0) return false;
  const i128 w1=scalar(vs);
  if (w1<=0) return false;
  const i128 w2=scalar(su);
  if (w2<=0) return false;
  return h-w0-w1-w2>0;
}

inline Result<bool> q4_weights_wide(const std::array<CenterInt,3>& n,i128 det,
                                   const Vec& face,const Vec& vs,const Vec& su) noexcept {
  constexpr int bits=6*kCoordBits+7, words=(bits+63)/64;
  static_assert(bits<=151);
  auto square=product<words>(det,det);
  if (!square.ok()) return square.outcome();
  auto h=require_add(square.value(),square.value());
  if (!h.ok()) return h.outcome();
  const auto scalar=[&](const Vec& normal) noexcept -> Result<Wide<words>> {
    Wide<words> sum;
    for (u32 j=0;j<3;++j) {
      auto term=product<words>(n[j],normal[j]);
      if (!term.ok()) return term.outcome();
      auto next=require_add(sum,term.value());
      if (!next.ok()) return next.outcome();
      sum=next.value();
    }
    return sum;
  };
  auto f=scalar(face);
  if (!f.ok()) return f.outcome();
  auto w0=require_add(h.value(),f.value().negated());
  if (!w0.ok()) return w0.outcome();
  if (w0.value().sign()<=0) return false;
  auto w1=scalar(vs);
  if (!w1.ok()) return w1.outcome();
  if (w1.value().sign()<=0) return false;
  auto w2=scalar(su);
  if (!w2.ok()) return w2.outcome();
  if (w2.value().sign()<=0) return false;
  auto rest=require_add(h.value(),w0.value().negated());
  if (!rest.ok()) return rest.outcome();
  rest=require_add(rest.value(),w1.value().negated());
  if (!rest.ok()) return rest.outcome();
  rest=require_add(rest.value(),w2.value().negated());
  if (!rest.ok()) return rest.outcome();
  if (rest.value().bit_length()>bits) return fail(Reason::arithmetic_invariant);
  return rest.value().sign()>0;
}

inline Result<bool> q4_presentation_inside(const std::array<Point,4>& points,
                                          const std::array<CenterInt,3>& n,i128 det,
                                          const Vec& vs,const Vec& su,const Vec& uv) noexcept {
  // <=3L^2 par somme partielle, <=3*2^48 aux trois profils : i64 exact sans certificat.
  const Vec face{vs[0]+su[0]+uv[0],vs[1]+su[1]+uv[1],vs[2]+su[2]+uv[2]};
  if (q4_weights_i128(points)) return q4_weights_native(n,det,face,vs,su);
  return q4_weights_wide(n,det,face,vs,su);
}

}  // namespace mhgp11::num::detail

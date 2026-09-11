// Audit guard only, extracted from frozen run(); proposal and fallback work remain separate.
static AnchorMebResult guarded_run(std::span<const P3> sites, Counters& c, AnchorMebWork& fallback_work, bool& fell_back) {
  AnchorMebResult r;
  const u8 n = (u8)sites.size();
  if (n == 1) { AnchorMebWork w{}; return anchor_meb(sites, w); }
  Candidate meb;
  if (!welzl(sites, meb, c)) { fell_back = true; return anchor_meb(sites, fallback_work); }
  // Coquille = sites de puissance nulle pour la MEB.
  std::vector<u8> shell;
  for (u8 i = 0; i < n; ++i) {
    ++c.pow; const i128 power = meb.power(sites[i]);
    if (power > 0) { fell_back = true; return anchor_meb(sites, fallback_work); }
    if (power == 0) shell.push_back(i);
  }
  // Canonicalisation : meme ordre lexicographique, plus petit q d'abord, sur la COQUILLE.
  const u8 m = (u8)shell.size();
  auto emit = [&](std::array<u8,4> sl, u8 q) {
    ++c.cand; Candidate cand;
    if (!form(sites, sl, q, cand)) return false;
    u8 sh = 0;
    for (u8 i = 0; i < n; ++i) { ++c.pow; const i128 p = cand.power(sites[i]);
      if (p > 0) return false; if (p == 0) ++sh; }
    r.support_size = q; r.support_slots = sl; r.selected_shell_count = sh;
    if (q == 2) { r.key = q2_ball_key(cand.a, cand.b);
      r.level = promote_level(q2_exact_level(p3_norm2(p3_sub(cand.a, cand.b)))); }
    else if (q == 3) { r.key = q3_ball_key(cand.three);
      r.level = promote_level(q3_exact_level(cand.a, cand.b, sites[sl[2]])); }
    else { r.key = ball_key_reduce(q4_ball_form(cand.four)); r.level = q4_level_raw(cand.four); }
    r.status = AnchorMebStatus::kOk; r.reason = "anchor_meb_exact_local"; return true;
  };
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) if (emit({shell[a],shell[b],0,0},2)) return r;
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) for (u8 d=b+1;d<m;++d)
    if (emit({shell[a],shell[b],shell[d],0},3)) return r;
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) for (u8 d=b+1;d<m;++d) for (u8 e=d+1;e<m;++e)
    if (emit({shell[a],shell[b],shell[d],shell[e]},4)) return r;
  fell_back = true; return anchor_meb(sites, fallback_work);
}

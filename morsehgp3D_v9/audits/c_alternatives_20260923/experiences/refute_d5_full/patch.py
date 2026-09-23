import re
src = open("probe_refute.cpp").read()
def rep(old, new, count=1):
    global src
    n = src.count(old)
    if n != count:
        raise SystemExit(f"pattern count {n} != {count}: {old[:80]!r}")
    src = src.replace(old, new)

rep('    if (d5_mismatch) fail("d5_terminal_mismatch");',
    '    if (d5_mismatch) std::fprintf(stderr, "WARN K%u d5_terminal_mismatch %llu\\n", k, (unsigned long long)d5_mismatch);')

# compteurs du coup d'ancre sur D dans la boucle "saut"
rep('''            if (k >= lo && k <= (unsigned)b.n_interior + b.n_shell) { t = found->second; break; }
            catalogued = true; dball = found->second;''',
'''            if (k >= lo && k <= (unsigned)b.n_interior + b.n_shell) {
              ++r_dterm;
              if (b.n_shell != b.arity) {
                ++r_dterm_ext;
                if (k > lo && k < (unsigned)b.n_interior + b.n_shell) ++r_dterm_ext_mid;
              }
              if (steps == 1) ++r_dterm_depth0;
              t = found->second; break;
            }
            catalogued = true; dball = found->second;''')
rep('    std::uint64_t j_cat_census = 0, j_hash_after_jump = 0;',
    '    std::uint64_t j_cat_census = 0, j_hash_after_jump = 0;\n    std::uint64_t r_dterm = 0, r_dterm_ext = 0, r_dterm_ext_mid = 0, r_dterm_depth0 = 0;')

# variante "prose" : regle 1-4 telle qu'ecrite, SANS coup d'ancre sur D
prose = r'''
    std::uint64_t pr_fail = 0, pr_fallback = 0, pr_steps_max = 0, pr_ok = 0;
    std::vector<std::uint32_t> prose_target(uniq_list.size(), 0xffffffffu);
    std::vector<std::uint32_t> prose_fail_ball;  // D de l'echec
    {
      std::unordered_map<KSet, std::uint32_t, KSetHash> pmemo;
      for (std::size_t uid = 0; uid < uniq_list.size() && k >= 2; ++uid) {
        const KSet& f = uniq_list[uid].first;
        std::uint32_t t = 0xffffffffu;
        if (auto it = seeds.find(f); it != seeds.end()) t = it->second;
        else if (auto ih = hits.find(f); ih != hits.end()) t = ih->second;
        else {
          std::vector<KSet> ppath; ppath.push_back(f);
          KSet s = f;
          std::uint64_t steps = 0;
          bool failed = false;
          for (;;) {
            if (++steps > 100000) { failed = true; break; }
            std::array<P3, kFacetMaxK> pos{};
            for (std::size_t j = 0; j < k; ++j) pos[j] = ix.upos[s[j]];
            const auto local = anchor_meb_proposed(std::span<const P3>(pos.data(), k), work5);
            if (local.status != AnchorMebStatus::kOk) fail("mebp");
            // census exact de D par l'arbre (puissance <= 0)
            inside.clear();
            const census_detail::AxisBounds bounds(local.key);
            stack.clear(); stack.push_back(ix.root());
            while (!stack.empty()) {
              const auto node = stack.back(); stack.pop_back();
              i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
              if (lo > 0) continue;
              if (is_leaf(node)) {
                const i32 u = leaf_index(node);
                const i128 pw = local.key.power(ix.upos[u]);
                if (pw <= 0) inside.emplace_back(pw, u);
              } else if (hi <= 0) {
                const auto range = ix.range_of(node);
                for (i32 u = range.first; u <= range.last; ++u) inside.emplace_back(local.key.power(ix.upos[u]), u);
              } else { stack.push_back(ix.nodes[node].right); stack.push_back(ix.nodes[node].left); }
            }
            if (inside.size() < k) fail("prose_census");
            std::partial_sort(inside.begin(), inside.begin() + k, inside.end());
            KSet g{};
            for (std::size_t j = 0; j < k; ++j) g[j] = inside[j].second;
            std::sort(g.begin(), g.begin() + k);
            if (auto a = seeds.find(g); a != seeds.end()) { t = a->second; break; }
            if (auto a = hits.find(g); a != hits.end()) { t = a->second; break; }
            if (auto a = pmemo.find(g); a != pmemo.end()) { t = a->second; break; }
            std::array<P3, kFacetMaxK> gp{};
            for (std::size_t j = 0; j < k; ++j) gp[j] = ix.upos[g[j]];
            const auto dg = anchor_meb_proposed(std::span<const P3>(gp.data(), k), work5);
            if (compare_exact_level(dg.level, local.level) < 0) s = g;
            else {
              ++pr_fallback;
              i32 z = -1;
              for (const auto& [pw, u] : inside) if (pw < 0 && !std::binary_search(s.begin(), s.begin() + k, u) && (z < 0 || u < z)) z = u;
              if (z < 0) {
                failed = true;
                if (const auto found = by_key.find(local.key); found != by_key.end()) prose_fail_ball.push_back(found->second);
                else prose_fail_ball.push_back(0xffffffffu);
                break;
              }
              s[local.support_slots[0]] = z;
              std::sort(s.begin(), s.begin() + k);
            }
            if (auto a = seeds.find(s); a != seeds.end()) { t = a->second; break; }
            if (auto a = hits.find(s); a != hits.end()) { t = a->second; break; }
            if (auto a = pmemo.find(s); a != pmemo.end()) { t = a->second; break; }
            ppath.push_back(s);
          }
          pr_steps_max = std::max(pr_steps_max, steps);
          if (failed) { ++pr_fail; t = 0xffffffffu; }
          else for (const auto& st : ppath) pmemo.emplace(st, t);
        }
        if (t != 0xffffffffu) ++pr_ok;
        prose_target[uid] = t;
      }
    }
    std::vector<std::uint32_t> facet_target_prose(facet_uid.size(), 0xffffffffu);
    for (std::size_t f = 0; f < facet_uid.size(); ++f) facet_target_prose[f] = prose_target[facet_uid[f]];
    std::printf("%s{\"refute\":{\"dterm\":%llu,\"dterm_ext\":%llu,\"dterm_ext_mid\":%llu,\"dterm_depth0\":%llu,"
                "\"prose_ok\":%llu,\"prose_fail\":%llu,\"prose_fallback\":%llu,\"prose_steps_max\":%llu,\"prose_fail_balls\":[",
                "", (unsigned long long)r_dterm, (unsigned long long)r_dterm_ext, (unsigned long long)r_dterm_ext_mid,
                (unsigned long long)r_dterm_depth0, (unsigned long long)pr_ok, (unsigned long long)pr_fail,
                (unsigned long long)pr_fallback, (unsigned long long)pr_steps_max);
    for (std::size_t i = 0; i < prose_fail_ball.size() && i < 8; ++i) {
      const std::uint32_t b = prose_fail_ball[i];
      if (b == 0xffffffffu) std::printf("%s{\"uncatalogued\":1}", i ? "," : "");
      else std::printf("%s{\"ball\":%u,\"p\":%u,\"u\":%u,\"q\":%u}", i ? "," : "", b, (unsigned)balls[b].n_interior,
                       (unsigned)balls[b].n_shell, (unsigned)balls[b].arity);
    }
    std::printf("]}},");
'''
rep('    for (std::size_t f = 0; f < facet_uid.size(); ++f) facet_target_jump[f] = jump_target[facet_uid[f]];',
    '    for (std::size_t f = 0; f < facet_uid.size(); ++f) facet_target_jump[f] = jump_target[facet_uid[f]];' + prose)

rep('    std::uint64_t jump_root_equal = 0, jump_root_mismatch = 0, jump_anchor_missing = 0;',
    '    std::uint64_t jump_root_equal = 0, jump_root_mismatch = 0, jump_anchor_missing = 0;\n    std::uint64_t prose_equal = 0, prose_mismatch = 0, prose_unresolved = 0;')
rep('''            else ++jump_root_equal;''',
'''            else ++jump_root_equal;
            const std::uint32_t tp = facet_target_prose[f];
            if (tp == NONE) ++prose_unresolved;
            else if (anchor[tp] == NONE || find(anchor[tp]) != lot_roots.back()) ++prose_mismatch;
            else ++prose_equal;''')
rep('''    std::printf("\\"lean_cpu_ms\\":%.2f,\\"jump_roots\\":{\\"equal\\":%llu,\\"mismatch\\":%llu,\\"anchor_missing\\":%llu},",''',
    '''    std::printf("\\"prose_roots\\":{\\"equal\\":%llu,\\"mismatch\\":%llu,\\"unresolved\\":%llu},", (unsigned long long)prose_equal,
                (unsigned long long)prose_mismatch, (unsigned long long)prose_unresolved);
    std::printf("\\"lean_cpu_ms\\":%.2f,\\"jump_roots\\":{\\"equal\\":%llu,\\"mismatch\\":%llu,\\"anchor_missing\\":%llu},",''')
open("probe_refute.cpp", "w").write(src)
print("ok")

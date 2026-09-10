import sys, os, shutil, json
S=sys.argv[1]
base=f"{S}/base/morsehgp3D_v7"
H="src/forest/full_coverage_certificate.hpp"
hdr=open(f"{base}/{H}").read()
M = {
 # 1 admission: open cut admits equality
 "admit_le": ("return cmp < 0 || (closed && cmp == 0);", "return cmp <= 0;"),
 # 2 roots[] without admission test of the node
 "roots_no_admission": ("      if (!full_coverage_detail::admitted(forest.nodes()[i].level, cut, closed)) continue;\n", ""),
 # 3 no live[parent]=0
 "no_live_reset": ("          live[parent] = 0;\n", ""),
 # 4 unsorted parents accepted (dup still caught by live)
 "unsorted_parent_ok": ("if ((j && action.parents[j - 1] >= parent) || parent >= prior_count || !live[parent])", "if (parent >= prior_count || !live[parent])"),
 # 5 birth cardinal < order accepted
 "birth_small_ok": (" ||\n              row.interior.size() + row.shell.size() < order) return invalid(\"coverage_birth_population\");", ") return invalid(\"coverage_birth_population\");"),
 # 6 shell_mask not masked by all_shell
 "shell_mask_unmasked": ("if ((ref.shell_mask & ~full_coverage_detail::all_shell(row)) ||\n              (ref.include_interior", "if ((ref.include_interior"),
 # 7 dedup omitted in reader
 "no_dedup": ("    out.values.erase(std::unique(out.values.begin(), out.values.end()), out.values.end());\n", ""),
 # 8 continuation creates a node
 "continuation_creates_node": ("        if (action.parents.size() == 1) {\n          segment = action.parents.front();", "        if (false) {\n          segment = action.parents.front();"),
 # 9 successors_[parent] not updated
 "no_successor_update": ("            out.successors_[parent] = segment;\n", ""),
 # 10 k1 without domain identity check
 "k1_no_domain_check": ("            if (id != bank->domain()[a]) return invalid(\"coverage_k1_roots\");\n", "            (void)id;\n"),
 # 11 bad_alloc leaves prefix
 "bad_alloc_prefix": None,  # special
 # 12 reader accepts a dead root
 "read_dead_root_ok": ("  if (root == kFullCoverageAbsent || full_coverage_root_at(forest, root, cut, closed) != root)\n    return out;", "  if (root == kFullCoverageAbsent || root >= forest.nodes().size())\n    return out;"),
 # 13 root_at ignores the birth date of the queried segment
 "root_at_no_birth_check": (" ||\n      !full_coverage_detail::admitted(forest.nodes()[segment].level, cut, closed))\n    return kFullCoverageAbsent;", ")\n    return kFullCoverageAbsent;"),
 # 14 k1 positive first level accepted
 "k1_zero_level_not_required": ("(!full_certificate_detail::zero(batch.level) ||\n          batch.actions.size() != bank->domain().size())", "(batch.actions.size() != bank->domain().size())"),
 # 15 empty continuation accepted
 "empty_continuation_ok": ("        if (action.parents.size() == 1 && action.contributions.empty())\n          return invalid(\"coverage_empty_continuation\");\n", ""),
 # 17 reader always includes interior
 "reader_interior_always": ("      if (record.ref.include_interior)\n        out.values.insert", "      if (true)\n        out.values.insert"),
 # 18 reader ignores shell mask
 "reader_shell_mask_ignored": ("        if (record.ref.shell_mask & (u16{1} << j)) out.values.push_back(row.shell[j]);", "        out.values.push_back(row.shell[j]);"),
 # 19 birth mask check dropped
 "birth_mask_not_checked": ("              ref.shell_mask != full_coverage_detail::all_shell(row) ||\n", ""),
 # 20 nonincreasing accepted at equality
 "equal_level_batch_ok": ("compare_exact_level(batches[b - 1].level, batch.level) >= 0", "compare_exact_level(batches[b - 1].level, batch.level) > 0"),
 # 21 k1 action count not checked
 "k1_count_not_checked": ("(!full_certificate_detail::zero(batch.level) ||\n          batch.actions.size() != bank->domain().size())", "(!full_certificate_detail::zero(batch.level))"),
 # 22 population index off-by-one
 "population_index_gt": ("if (ref.population >= bank->rows().size())", "if (ref.population > bank->rows().size())"),
 # 23 roots[] one-step instead of transitive
 "roots_one_step": ("roots[i] = next == kFullCoverageAbsent || roots[next] == kFullCoverageAbsent ? i : roots[next];", "roots[i] = next == kFullCoverageAbsent || roots[next] == kFullCoverageAbsent ? i : next;"),
 # 24 domain size < order accepted
 "domain_smaller_than_order_ok": (" ||\n      bank->domain().size() < order || batches.empty())", " || batches.empty())"),
 # 25 order > kFacetMaxK accepted
 "order_above_max_ok": ("if (order < 1 || order > kFacetMaxK || !bank", "if (order < 1 || !bank"),
 # 27 roots loop forward
 "roots_forward": ("    for (size_t i = roots.size(); i-- > 0;) {", "    for (size_t i = 0; i < roots.size(); ++i) {"),
 # 29 bank overlap check removed
 "bank_overlap_ok": ("    for (PointId p : row.shell)\n      if (std::binary_search(row.interior.begin(), row.interior.end(), p)) return result;\n", ""),
 # 30 bank shell width check removed
 "bank_width_ok": ("if (row.shell.size() > std::numeric_limits<u16>::digits ||\n        (row.interior.empty()", "if ((row.interior.empty()"),
 # 31 birth include_interior consistency dropped
 "birth_interior_flag_not_checked": ("          if (ref.include_interior != !row.interior.empty() ||\n", "          if (\n"),
 # 36 bad cut den accepted in root_at
 "root_at_bad_cut_ok": ("  if (!forest.order() || cut.den <= 0 || segment >= forest.nodes().size() ||", "  if (!forest.order() || segment >= forest.nodes().size() ||"),
 # 37 absent root accepted by reader
 "read_absent_root_ok": ("  if (root == kFullCoverageAbsent || full_coverage_root_at(forest, root, cut, closed) != root)\n    return out;", "  if (full_coverage_root_at(forest, root, cut, closed) != root)\n    return out;"),
 # 38 empty batch accepted
 "empty_batch_ok": ("      if (batch.actions.empty()) return invalid(\"coverage_empty_batch\");\n", ""),
 # 39 contributions of a continuation dated at the node level instead of batch level
 "contribution_dated_at_node": ("          out.contributions_.push_back({batch.level, segment, ref});", "          out.contributions_.push_back({out.nodes_[segment].level, segment, ref});"),
 # 40 reader stops at contributions loop without break (uses continue) -- equivalent? contributions sorted; test
 "reader_continue_not_break": ("      if (!full_coverage_detail::admitted(record.level, cut, closed)) break;", "      if (!full_coverage_detail::admitted(record.level, cut, closed)) continue;"),
 # 41 order of ids: multifusion id assigned before checking -- n/a. successor of a node overwritten twice? no.
 # 42 full_coverage_root_at follows successor even if not admitted but stops one late
 "root_at_returns_next": ("    if (!full_coverage_detail::admitted(forest.nodes()[next].level, cut, closed)) break;\n    segment = next;", "    segment = next;\n    if (!full_coverage_detail::admitted(forest.nodes()[next].level, cut, closed)) break;"),
 # 43 mask zero with include_interior false accepted
 "empty_mask_ok": (" ||\n              (!ref.include_interior && ref.shell_mask == 0)) return invalid(\"coverage_empty_or_invalid_mask\");", ") return invalid(\"coverage_empty_or_invalid_mask\");"),
 # 44 populations bank rows check on empty bank
 "empty_bank_ok": ("if (order < 1 || order > kFacetMaxK || !bank || bank->rows().empty() ||", "if (order < 1 || order > kFacetMaxK || !bank ||"),
 # 45 birth requires exactly one contribution -> allow several
 "birth_multi_contrib_ok": ("          if (action.contributions.size() != 1) return invalid(\"coverage_birth_population\");", "          if (action.contributions.empty()) return invalid(\"coverage_birth_population\");"),
 # 46 length_error path: never thrown (identifier exhaustion check removed) -- declared unexercised; test survives expected
 "no_id_exhaustion_check": ("          if (out.nodes_.size() == kFullCoverageAbsent)\n            throw std::length_error(\"coverage node identifiers exhausted\");\n", ""),
 # 47 reader: parents-inherited contributions only from direct parents (roots via successor for one hop) -- same as one_step. skip
 # 48 k>1 zero level accepted
 "zero_level_ok_k2": ("      if (order > 1 && full_certificate_detail::zero(batch.level))\n        return invalid(\"coverage_positive_level_required\");\n", ""),
 # 49 den<=0 accepted
 "den_zero_ok": ("      if (batch.level.den <= 0) return invalid(\"coverage_invalid_level\");\n", ""),
 # 50 order_ published even on failure? (set order before loop) -> failure leaves order set
 "order_set_early": None,
}
special_bad_alloc = hdr.replace(
"  try {\n    FullCoverageBuildResult result;\n    auto& out = result.value;",
"  FullCoverageBuildResult result;\n  try {\n    auto& out = result.value;")
special_bad_alloc = special_bad_alloc.replace(
"  } catch (const std::bad_alloc&) {\n    auto result = invalid(\"coverage_allocation_failed\");\n    result.status = FullCertificateStatus::kResourceExhausted; return result;\n  } catch (const std::length_error&) {\n    auto result = invalid(\"coverage_size_overflow\");\n    result.status = FullCertificateStatus::kResourceExhausted; return result;\n  }\n}",
"  } catch (const std::bad_alloc&) {\n    result.reason = \"coverage_allocation_failed\";\n    result.status = FullCertificateStatus::kResourceExhausted; return result;\n  } catch (const std::length_error&) {\n    result.reason = \"coverage_size_overflow\";\n    result.status = FullCertificateStatus::kResourceExhausted; return result;\n  }\n}")
assert special_bad_alloc != hdr and special_bad_alloc.count("FullCoverageBuildResult result;\n  try {")==1
special_order_early = hdr.replace("    FullCoverageBuildResult result;\n    auto& out = result.value;\n", "    FullCoverageBuildResult result;\n    auto& out = result.value;\n    out.order_ = order;\n")
assert special_order_early != hdr
out = {}
for name, rep in M.items():
    if rep is None:
        text = {"bad_alloc_prefix": special_bad_alloc, "order_set_early": special_order_early}[name]
    else:
        old, new = rep
        n = hdr.count(old)
        if n != 1:
            print("PATTERN COUNT", name, n); sys.exit(1)
        text = hdr.replace(old, new)
    d = f"{S}/mut/{name}/morsehgp3D_v7"
    if os.path.exists(d): shutil.rmtree(d)
    shutil.copytree(base, d)
    open(f"{d}/{H}", "w").write(text)
    out[name] = True
print(len(out), "mutants written")
json.dump(list(out), open(f"{S}/mutants.json","w"))

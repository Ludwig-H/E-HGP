// PRIVATE dispatcher/exception qualification for a recorded product overlay.
// The ONLY overlay edit removes noexcept from NoObserver::before_form and
// calls ::mhgp7_test_before_form(work.meb_proposal_supports, work.certified, q).
// Compile with -include fault_hook.hpp and -I<overlay>/morsehgp3D_v7.
// This does not claim exceptions in nominal NoObserver, or inside F fallback.
#include <array>
#include <cstring>
#include <exception>

#if !defined(MHGP7_PRIVATE_MEB_FORM_FAULT_HOOK)
#error "force-include the private fault_hook.hpp before reading the overlay"
#endif
#if defined(MHGP7_TESTING)
#error "private overlay must retain the product code without MHGP7_TESTING"
#endif
#include "tests/full_gabriel_singleton_fixtures.hpp"

using namespace mhgp7_singleton_test;
namespace {
namespace fault = mhgp7_private_fault;
using Kind = fault::Kind;
constexpr u64 kProposalCap = 1000000;
constexpr unsigned kOrder = 2;
constexpr const char* kAccounting = "reference_ordinal_plus_native_z_q3_q4_proposal_v2";
u64 cases = 0, wrapper_cases = 0, builder_cases = 0, public_refusals = 0;
u64 runtime_propagations = 0, builder_propagations = 0, mirrors = 0;
u64 baselines = 0, retries = 0, compared_mirrors = 0, paid_at_throw = 0;

bool text_equal(const char* a, const char* b) {
  return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}
auto proposal(const FullGabrielStats& s) {
  const auto& p = s.meb_proposal;
  return std::tie(p.meb_proposal_supports, p.pivots, p.certified, p.fallback, p.reference_supports);
}
bool same_stats(const FullGabrielStats& a, const FullGabrielStats& b) {
  return work(a) == work(b) && proposal(a) == proposal(b);
}
FullGabrielLimits limits(bool lazy) {
  auto caps = roomy(lazy); caps.max_meb_proposal_supports = kProposalCap; return caps;
}
FullGabrielResult public_call(const Cloud& c, bool lazy) {
  const auto caps = limits(lazy);
  return lazy ? build_full_gabriel_order_lazy(c.ix, kOrder, c.catalogue[2], c.catalogue[3], caps, {1000000})
              : build_full_gabriel_order(c.ix, kOrder, c.catalogue[2], c.catalogue[3], caps);
}
void metadata(const FullGabrielResult& r, bool lazy) {
  check(text_equal(r.meb_accounting, kAccounting) && text_equal(kFullGabrielMebAccounting, kAccounting),
        "explicit filtered-v2 accounting survives the injected exception");
  check(text_equal(r.alias_policy, lazy ? kFullGabrielLazyAliases : kFullGabrielEagerAliases) &&
        text_equal(r.successor_accounting, kFullGabrielSuccessorAccounting), "other policies unchanged");
}
FullGabrielResult unfaulted(const Cloud& c, bool lazy, bool retry) {
  fault::reset(Kind::kNone);
  auto r = public_call(c, lazy);
  const auto observed = fault::state;
  check(r.status == FullGabrielStatus::kCompleteRelative && text_equal(r.reason, kFullGabrielAuthority),
        "unfaulted overlay closes the supplied-catalogue FULL construction");
  metadata(r, lazy);
  const auto& s = r.stats; const auto& p = s.meb_proposal;
  check(s.meb_calls >= 3 && s.chain_steps >= 2 && s.max_chain_length >= 2,
        "named regular chain performs at least three MEB calls");
  check(p.certified == s.meb_calls && s.geometry.meb_calls == s.meb_calls &&
        p.fallback == 0 && p.reference_supports == 0 && p.meb_proposal_supports < kProposalCap,
        "roomy K2 scene certifies every MEB; this test does not exercise an F exception");
  check(observed.callbacks == p.meb_proposal_supports && observed.callbacks > 0 &&
        observed.prospective && observed.hits == 0 && !observed.armed,
        "real overlay form callbacks see every prospective charge, without a nominal throw");
  if (retry) ++retries; else { ++baselines; compare_oracle(c, kOrder, r.forest); }
  return r;
}
void mirror(const FullGabrielResult& r, const fault::State& observed, bool lazy) {
  ++mirrors; metadata(r, lazy);
  const auto& s = r.stats; const auto& p = s.meb_proposal;
  check(p.meb_proposal_supports == observed.throw_paid && p.certified == observed.throw_certified &&
        p.certified >= 2 && p.meb_proposal_supports >= p.certified + 1 &&
        p.meb_proposal_supports <= kProposalCap, "paid form and earlier certificates survive Builder destruction");
  check(s.geometry.meb_calls == p.certified && s.meb_calls == s.geometry.meb_calls + 1 &&
        s.geometry.meb_supports >= p.certified,
        "outer call charged; interrupted form has not incremented the accepted geometry MEB count");
  check(p.fallback == 0 && p.reference_supports == 0 && p.reference_supports <= s.geometry.meb_supports,
        "no fabricated physical F work during this pre-form exception");
}
FullGabrielResult injected(const Cloud& c, bool lazy, Kind kind, bool direct_builder) {
  ++cases;
  if (direct_builder) ++builder_cases; else ++wrapper_cases;
  context = std::string(lazy ? "lazy" : "eager") + (direct_builder ? "/Builder/" : "/wrapper/") +
            std::to_string(static_cast<int>(kind));
  // out MUST outlive the Builder and its destructor; no counters are inspected
  // inside the exception-producing scope. Direct Builder is not a transaction.
  FullGabrielResult out;
  if (lazy) out.alias_policy = kFullGabrielLazyAliases;
  const auto caps = limits(lazy);
  const FullGabrielCacheLimits cache{1000000};
  bool returned = false;
  Kind caught = Kind::kNone;
  fault::reset(kind);
  try {
    if (direct_builder) {
      full_gabriel_detail::Builder builder(c.ix, kOrder, c.catalogue[2], c.catalogue[3],
                                          caps, out, lazy ? &cache : nullptr);
      (void)builder.run();
    } else out = public_call(c, lazy);
    returned = true;
  } catch (const std::bad_alloc&) {
    caught = Kind::kBadAlloc;
  } catch (const std::length_error& e) {
    caught = Kind::kLengthError;
    check(text_equal(e.what(), fault::kLengthMessage), "original injected length_error propagated");
  } catch (const std::runtime_error& e) {
    caught = Kind::kRuntimeError;
    check(text_equal(e.what(), fault::kRuntimeMessage), "original injected runtime_error propagated");
  } catch (...) {
    check(false, "unexpected injected exception type");
  }
  const auto observed = fault::state;
  fault::state.armed = false;
  check(observed.hits == 1 && !observed.armed && observed.prospective && observed.throw_certified >= 2 &&
        observed.throw_paid == observed.callbacks && observed.throw_q >= 2 && observed.throw_q <= 4,
        "one actual prospective form boundary throws only after at least two completed native MEBs");
  paid_at_throw += observed.throw_paid;
  if (direct_builder || kind == Kind::kRuntimeError) {
    check(!returned && caught == kind, "uncaught exception propagates with its exact type");
    if (direct_builder) { ++builder_propagations; mirror(out, observed, lazy); }
    else ++runtime_propagations;  // The wrapper's private result is not observable here.
  } else {
    check(returned && caught == Kind::kNone && out.status == FullGabrielStatus::kResourceExhausted,
          "public wrapper converts only the two documented resource exceptions");
    check(text_equal(out.reason, kind == Kind::kBadAlloc ? "full_gabriel_allocation_failed" : "full_gabriel_size_overflow"),
          "precise public resource reason");
    ++public_refusals; mirror(out, observed, lazy);
  }
  check(empty(out.forest), "no partial public forest escapes this pre-finalization interruption");
  return out;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::strcmp(argv[1], "--selftest") != 0) return 2;
  try {
    Cloud c("two_step_private_form_fault", input({{622,745,858},{839,341,867},{111,242,715},{827,10,537},
                {437,578,984},{396,213,30},{693,305,961},{814,71,415}}));
    check(c.valid, "independent global regularity and every supplied catalogue admitted before injection");
    if (c.valid) for (bool lazy : {false, true}) {
      const auto baseline = unfaulted(c, lazy, false);
      FullGabrielStats previous;
      bool have_previous = false;
      for (Kind kind : {Kind::kBadAlloc, Kind::kLengthError, Kind::kRuntimeError}) {
        const auto public_result = injected(c, lazy, kind, false);
        const auto direct_result = injected(c, lazy, kind, true);
        if (kind != Kind::kRuntimeError) {
          check(same_stats(public_result.stats, direct_result.stats), "wrapper catch preserves the exact external-Builder mirrors");
          ++compared_mirrors;
        }
        if (have_previous) {
          check(same_stats(previous, direct_result.stats), "all exception types retain the identical real work prefix");
          ++compared_mirrors;
        }
        previous = direct_result.stats; have_previous = true;
        const auto retry = unfaulted(c, lazy, true);
        check(same_forest(baseline.forest, retry.forest) && same_stats(baseline.stats, retry.stats),
              "fresh retry resets neither prior evidence nor the semantics of the unfaulted order");
      }
    }
  } catch (const std::exception& e) {
    fault::state.armed = false;
    ++failures; std::fprintf(stderr, "FAIL exception [%s]: %s\n", context.c_str(), e.what());
  }
  check(!mhgp7_oracle::overflow_seen(), "sticky independent oracle overflow remains clear");
  check(admitted_clouds == 1 && baselines == 2 && retries == 6 && cases == 12 && wrapper_cases == 6 &&
        builder_cases == 6 && public_refusals == 4 && runtime_propagations == 2 && builder_propagations == 6 &&
        mirrors == 10 && compared_mirrors == 8 && paid_at_throw >= 36 && cuts > 0,
        "twelve non-vacuous exception cases and ten externally observable mirrors");
  std::printf("{\"schema\":\"mhgp7-private-full-meb-form-fault-v1\",\"status\":\"%s\",\"public_status\":\"not_claimed\","
              "\"nominal_noobserver_exception_claim\":false,\"F_exception_coverage\":\"not_exercised\","
              "\"cases\":%llu,\"public_refusals\":%llu,\"runtime_propagations\":%llu,\"builder_propagations\":%llu,"
              "\"mirrors\":%llu,\"compared_mirrors\":%llu,\"baselines\":%llu,\"retries\":%llu,\"paid_at_throw\":%llu,"
              "\"checks\":%llu,\"failures\":%llu}\n", failures == 0 ? "passed" : "failed",
      (unsigned long long)cases, (unsigned long long)public_refusals, (unsigned long long)runtime_propagations,
      (unsigned long long)builder_propagations, (unsigned long long)mirrors, (unsigned long long)compared_mirrors,
      (unsigned long long)baselines, (unsigned long long)retries, (unsigned long long)paid_at_throw,
      (unsigned long long)checks, (unsigned long long)failures);
  return failures == 0 ? 0 : 1;
}

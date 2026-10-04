// Option une passe : emissions retenues en blocs fixes, compactees seulement apres join complet.
#pragma once
#include "catalogue/internal.hpp"

namespace mhgp11::catalogue_detail {
Result<Catalogue> build_single_pass(const Cloud&, const CatalogueParams&, MemoryBudget&, sched::Pool&,
                                    CatalogueTimings*, CatalogueDiagnostics*) noexcept;
}  // namespace mhgp11::catalogue_detail

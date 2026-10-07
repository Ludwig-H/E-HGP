// Voie appareil absente : construction sans MHGP12_ENABLE_CUDA. Le contexte ne s'ouvre pas et la voie appareil rend
// device_unavailable apres la validation des parametres, jamais un calcul de repli discret sur l'hote (la voie CPU
// reste build_catalogue, appelee explicitement).
#include "catalogue/device_pipeline.hpp"

namespace mhgp12 {

struct CatalogueDevice::Impl {};

CatalogueDevice::CatalogueDevice(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
CatalogueDevice::CatalogueDevice(CatalogueDevice&& other) noexcept = default;
CatalogueDevice::~CatalogueDevice() = default;

Result<CatalogueDevice> CatalogueDevice::open(MemoryBudget&) noexcept {
  return catalogue_detail::dev::device_refusal(false);
}

Result<Catalogue> build_catalogue_device(const Cloud&, const CatalogueParams& params, CatalogueDevice&, sched::Pool&,
                                         CatalogueDiagnostics*) noexcept {
  MHGP12_TRY(check_catalogue_params(params));
  return catalogue_detail::dev::device_refusal(false);
}

}  // namespace mhgp12

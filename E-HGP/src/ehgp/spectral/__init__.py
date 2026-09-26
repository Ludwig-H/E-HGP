"""Couche statistique de E-HGP : log-densite spectrale et sa tour.

Ce paquet remplace le COMPTAGE EMPIRIQUE DE BOULES, inutilisable en grande
dimension, par un modele de log-densite regularise par f-divergence, et
calcule la tour de ses ensembles de sur-niveau.

Deux modules, deux roles nettement separes :

* `log_density` : l'estimateur. Le log-rapport `log(dp/dq)` est
  l'INTEGRALE EN rho des potentiels optimaux de la famille de chi-deux
  ponderes `f_rho` de Bach, et cette integrale se calcule en forme close
  par une seule decomposition en valeurs propres generalisees du couple
  `(Sigma_p, Sigma_q)`, avec le filtre spectral
  `G(lambda) = log(lambda) / (lambda - 1)`. Cout `O(m^2 n + m^3)`, sans
  aucun terme en `C(n, k)`, et gradient analytique partout dans `R^d` ;

* `tower` : l'objet. Maxima locaux par ascension quasi-Newton multi-departs,
  niveaux de col par chemins explicites (minorants monotones), liaison
  simple sur ces niveaux, bassins des observations, enregistrement canonique
  et digest sha256.

Calibration des deux axes : le comptage de boules est un estimateur a noyau
indicatrice de boule, donc le seuil d'ordre `k` au rayon `sqrt(a)` est le
niveau `k / (n V_d a^{d/2})` sur la densite. La fonction
`log_density.order_to_log_density_level` et la methode
`tower.SpectralTower.equivalent_order` sont les deux sens de cette
correspondance.

Statut : `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
`profile=any_dimension_rational_exact` (ce paquet est la partie FLOTTANTE du
profil : il propose, il ne certifie pas), `public_status=not_claimed`.
"""

from .log_density import (
    DegenerateSample,
    FourierFeatures,
    GaussianReference,
    NystromFeatures,
    SingularReference,
    SpectralLogDensity,
    UniformBoxReference,
    kullback_leibler_integrand,
    optimal_potential,
    order_to_log_density_level,
    ratio_from_potential,
    spectral_filter,
    weighted_chi_square,
)
from .tower import AscentFailure, SpectralTower, from_model, gradient_ascent, joint_ascent

__all__ = [
    "AscentFailure",
    "DegenerateSample",
    "FourierFeatures",
    "GaussianReference",
    "NystromFeatures",
    "SingularReference",
    "SpectralLogDensity",
    "SpectralTower",
    "UniformBoxReference",
    "from_model",
    "gradient_ascent",
    "joint_ascent",
    "kullback_leibler_integrand",
    "optimal_potential",
    "order_to_log_density_level",
    "ratio_from_potential",
    "spectral_filter",
    "weighted_chi_square",
]

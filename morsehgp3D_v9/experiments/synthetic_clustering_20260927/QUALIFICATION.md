# Qualification de la campagne synthétique

27 septembre2026. Huit commandes closes, toutes code0 : mêmes quatre
suites en Python normal puis `-O`. Quarante tests par mode, sans additionner
les deux passages pour grossir le bilan.

| Suite | Tests | Portée |
|---|---:|---|
| Générateur | 11 | 150petites scènes, allocations rationnelles, covariance, rang et préfixes appariés |
| Préparateur | 11 | 267contrôles rationnels,25refus, grille commune et indépendance des labels |
| Pipeline | 5 | Deux petites intégrations K5/n8, prédictions réellement recalculées, transport natif rejoué |
| Lanceur | 13 | 34workers simulés/612lignes, interruptions, fermetures, erreurs et refus de résultats mal liés |

Les tests de pipeline exécutent les consommateurs Python et HDBSCAN sur
deux exports antérieurement qualifiés ; ils **ne relancent pas** le binaire
natif. Les tests du lanceur n'exécutent aucun vrai worker. Les calculs
natifs des34nouvelles scènes appartiennent à la campagne, pas à ces gates.

Capture locale : `/tmp/mhgp9-synthetic-qualification-20260927-r1/receipt.json`.
SHA256 : `90dacf3e77d76c1dc992fd68a5cb7a081d5407ab2ecb89c930bd73468efc9eae`.
Durée observée12,579s, sans signification de benchmark. Les86sources et
fixtures de cette qualification sont fermées avant/après ;744empreintes
héritées sont revérifiées. Ces deux inventaires peuvent se recouvrir.
Les25artefacts gardent l'intention et les huit commandes/stdout/stderr.

Les sources du moteur, ses builds et ses anciennes preuves restent figés.
Les captures sont LIVE et nécessitent les dépendances privées référencées ;
ce document n'est pas une archive autonome de tous les binaires.

## Préparation entière

Les43nuages ont été préparés :34scènes de qualité, neuf entrées8k/16k/32k.
Zéro fusion, collision ou écrêtage. Le manifeste complet est
`/tmp/mhgp9-synthetic-inputs-20260927-r1/manifest.json`, SHA256
`922a4ac65ae1498a59f7981a5aa1a0e2bdccde542322e12e4c68f7f0b79f506d`.
Les graines et les paramètres de chaque composante restent disponibles.

Le plus grand écart d'une coordonnée à sa reconstruction de grille est
4,620×10^-5 unité du modèle. Ce n'est pas une précision en mètres ; chaque
série garde sa propre grille isotrope, commune entre ses tailles. Les deux
méthodes reçoivent exactement les mêmes sites. Préparer les neuf gros nuages
n'exécute ni leur FULL ni leur projection.

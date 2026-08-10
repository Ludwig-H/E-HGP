# Delta live `face-owner` — suppression du coût quadratique

> Historique : le refactor ordre par ordre demandé ici est maintenant livré à
> `56e76c6` et audité dans
> [`AUDIT_FACEOWNER_ORDRE_PAR_ORDRE_56E76C6.md`](AUDIT_FACEOWNER_ORDRE_PAR_ORDRE_56E76C6.md).

Date : 10 août 2026 UTC.

Pins : `HEAD=21d85c85c1bd6553606a0081eeb283f58699d173`, header
`saturated_fold_faceowner.hpp=2eb2877a71f07970d641a99915de176dca28b422853201e2eb580ab703ee47fb`.
Les autres pins restent ceux de
[`AUDIT_LIVE_FACEOWNER_8D6516C.md`](AUDIT_LIVE_FACEOWNER_8D6516C.md).

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=bounded_differential_and_performance_regression`,
`mode=audit_independant`. Le correctif appartient au commit `21d85c8` de Claude.

## Correction reçue

Claude a supprimé l'appel `incidences.reserve(incidences.size()+1)` placé dans
la boucle d'émission. `std::vector` retrouve sa croissance géométrique amortie;
le coût accidentel quadratique en `I_k` est fermé.

Build Release ciblé avec `-Werror` : succès. Sélection permanente : 11/11
CTest verts en `6,91 s` — campagnes générique/saturée, sept mutants, refus du
mutant inconnu et pipeline différentiel `faceowner/G2`.

Mesures directes sur le même hôte partagé :

| entrée | incidences | temps fold `face-owner` | résultat différentiel |
| --- | ---: | ---: | --- |
| `n=20,smax=11,K=3` | 66 348 | 0,146 s | même digest et mêmes compteurs que G2 |
| `n=32,smax=11,K=3` | 247 854 | 0,305 s | même digest et mêmes compteurs que G2 |

Avant la correction, le premier cas demandait `19,188 s` et le CTest `n=32`
avait demandé `264,92 s` sous contention. Les ratios ne sont pas des benchmarks
produit, mais ils identifient sans ambiguïté l'allocation comme cause. Le run de
Claude `n=64,K=5` termine désormais en moins de 35 secondes et son `n=200,K=5`
environ 80 secondes sous forte contention, là où l'ancien `n=64` dépassait neuf
minutes.

## Ce qui reste avant un claim mémoire

La correction ne réserve pas encore la masse exacte `I_k`, pourtant connue au
préflight. Elle reste correcte et linéaire amortie, mais le high-water dépend de
la capacité choisie par la bibliothèque. Faire un unique `reserve(I_k)` après
conversion vérifiée vers `size_t` donne une allocation reproductible et permet
de comparer prévu/réel.

Le refactor ordre-par-ordre n'est pas présent : tous les `star_edges[k]` sont
conservés, puis tous les `OrderState[k]` sont alloués avant le rejeu. Le modèle
continue à compter 24 octets par incidence au lieu des 32 mesurés sur cette ABI
et omet les arêtes persistantes, états, sorties et capacités. Le terme « pic
conservateur » reste donc faux et aucun budget dur `faceowner` n'est reçu.

Refactor exact recommandé : boucle externe `k`; réserver et émettre `I_k`,
construire les branches, libérer réellement les incidences, créer un unique
état d'ordre, rejouer tous les lots dans `fold.orders[k-1]`, puis libérer
branches et état avant `k+1`. Les ordres sont indépendants. Les sorties déjà
publiées restent toutefois persistantes et doivent entrer dans le pic.

Identités à publier par ordre : `I_k=I_k_predicted`, branches brutes
`=I_k-S_k` où `S_k` est le nombre de signatures uniques, unions tentées
`=branches_deduplicated`, puis cohérence des sommes globales.

## Prochains verrous déjà résolus sur le papier

- L'API du fast path doit recevoir un sidecar lié au digest : points, `BallKey`,
  certificats `q_min/principal`, index `BallKey -> generator` et complétude par
  ordre. Le `Catalogue` seul ne contient pas les coordonnées nécessaires aux
  miniboules des carriers.
- Le fallback exact intersecte les postings en identifiants de générateurs et
  ne projette vers les racines qu'après l'intersection. Sa coupure est locale à
  la composante staging courante. Les fixtures tueuses sont désormais écrites
  dans
  [`NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md`](NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md).
- Une alternative de pont robuste est de produire des runs bornés de signatures
  un ordre à la fois, puis de les fusionner et rejouer chronologiquement. Elle
  scanne tout `I_k`, mais borne la RAM et conserve exactement le théorème.

GCP non utilisé.

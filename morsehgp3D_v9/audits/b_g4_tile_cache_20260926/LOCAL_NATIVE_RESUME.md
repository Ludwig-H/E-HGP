# Revalidation native après reprise — 26 septembre 2026

État produit `d0e711e23617a7fa2838cac9b370ff125d4be5fb` ; audit sans
modification du moteur. Cadre `exploration_v9_hors_registre`,
`cpu_reference`, `quantized_u18_input_only`, `audit_portable_tile_cache`,
`not_claimed`. GCP non utilisé par ce lot. Les éventuelles mesures G4 sont
un reçu distinct ; aucun résultat CUDA n'est déduit de cette compilation.

Construction neuve Release, GCC 13.3, Boost extrait au préfixe local
historique ; `MHGP9_ENABLE_CUDA=OFF`. Aucun build épinglé n'a été écrasé.
Le [runner](run_local_native_resume.py) ferme les sources suivies qu'il
recense, les six binaires, commandes, codes et sorties dans
[`local_native_receipts/manifest.json`](local_native_receipts/manifest.json).
Les durées locales sont des temps de qualification, pas des benchmarks.
Pas de nouveau sanitizer ou TSan dans ce lot.

## Résultats

- Cinq gates CTest filtre/cache passent : filtre historique, son refus
  d'argument, cache, son refus et mutant sans retest des endpoints.
- Filtre historique : 1 708 741 rectangles, 1 195 074 paires et treize
  contrôles de frontière publique.
- Cache : 98 784 requêtes et 3 176 traces, confrontées au produit et au
  comptage ponctuel exact ; 5 776 rejets complets, 3 298 partiels, 86 534
  non-rejets. Les contacts et extrêmes u18 sont exercés. Le mutant sans
  retest échoue avec le code 1 et `cause=witness_cache.endpoint_retest`.
- Deux refus de la sonde CPU passent. Ils ne constituent ni une exécution
  du GPU ni un oracle de chaîne FULL.
- Mapping : 10 083 additions vérifiées, 182 configurations, 336 196 paires,
  12 012 représentants et 10 486 paires contournant les petites tuiles.
  Le mutant qui laisse une tuile franchir une fin de ligne échoue avec
  code 1 et `cause=tile_mapping.row_representative`.

Le mapping est un modèle hôte indépendant comparé aux formules relues du
CUDA ; seul le test sur appareil pourra vérifier l'exécution des noyaux.

## Conclusion mathématique et limite de coût

La preuve de sûreté du cache tient : nœuds disjoints **par voie**, retest
strict pour chaque nouvelle paire, repli depuis zéro pour chaque voie
encore ouverte. Le seuil saturé limite la trace à `2K−3` nœuds, donc 17
à K10. Une antichaîne globale q3/q4 serait trop restrictive ; un parent
dans une voie et un enfant dans l'autre restent permis. Un échec de
pile ou de trace reste une erreur de l'appel, jamais une réponse partielle.

Ce n'est pas une amélioration d'exposant : il demeure un passage sur chaque
paire résiduelle P, jusqu'à O(KP) retests de cache, puis les recherches
globales restantes. Sur les petites fixtures de cette gate, les visites
descendent de 2 343 212 à 2 289 970, mais **264 924 retests s'ajoutent** :
le total est 2 554 894, environ 9 % de plus. Ne pas présenter la baisse
des seules visites comme un gain. Ce constat est borné aux fixtures,
pas une prédiction de la campagne LiDAR ni du temps GPU.

Relecture LIVE (sources et binaires locaux requis, pas archive autonome) :

```sh
python3 -B morsehgp3D_v9/audits/b_g4_tile_cache_20260926/read_local_native_resume.py
python3 -B -O morsehgp3D_v9/audits/b_g4_tile_cache_20260926/read_local_native_resume.py
```

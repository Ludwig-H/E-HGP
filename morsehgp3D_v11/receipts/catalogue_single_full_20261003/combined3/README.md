# Combined3 — FULL et options du catalogue, 3 octobre 2026

Capture LIVE close, source exécutée `90dd48bd284a960ed687328e1fe2468947039069`.
Session originale `/workspaces/.ehgp-sessions/v11.20261003.combined3`.
`DONE=0`, `completed`, worker0, génération identique à l'arrêt `TERMINATED` ;
arrêt ciblé, résultats vérifiés, suppression de la clé privée, retrait de la clé
OS Login et libération du verrou tous attestés. Aucune erreur ni alerte.

**Qualification : 2 799/2 799 portes de matrice, 209/209 du complément
ASan/UBSan18 ; 27/27 essais FULL réussis, aucune omission, aucun écart parmi
les six groupes de comparaison.** Clang absent facultatif. Les journaux
causaux attestent 265 mutations : 263 verdicts `code`/`ligne` et deux échecs de
construction attendus. Ce total ne signifie pas 265 réponses géométriques.

## Protocole réellement exécuté

Plan `bench/plans/full_combined_g4.json` à la source épinglée. Trois commandes
séquentielles : matrice850s (budget800), ASan18 180s (budget160), banc530s
(budget450), enfant60s. Matrice : Release18=525, mutants=21, ASan24=450,
TSan21=450, bits21=450, bits24=450, poison21=451, style=2. Le complément
sélectionne num/index/tower, profil18 avec ASan/UBSan. Les sélections, JUnit,
planchers de matrice, caches CMake, flags et empreintes des exécutables sont
recoupés ; 32 slots sont déclarés pour la campagne de mutations.

Neuf couples entrée/profil, trois modes, W48, K1..5, une exécution neuve par
unité, sans répétition : les trois nuages sans sol complets en u21 et u24,
puis uniformes8k/16k/32k en u21. Coordonnées entières et PointIds identiques
au manifeste, domaine commun18 bits ; les trois LiDAR proviennent des trames
08/000000, 08/000100 et 08/000200, pas de plusieurs séquences.

- Mode15 : catalogue fixe, cache J2, tri indirect ; descentes régulières
  parallèles et mémos privés, verticales sérielles.
- Mode63 : mode15 avec frontière adaptative et assemblage parallèle.
- Mode127 : mode63 avec génération catalogue une passe et compactage.

Les sorties sémantiques sont égales entre profils/modes ; les SHA256 bruts
sont égaux à profil constant. Tout le travail FULL est égal entre ces modes
qui gardent le même routage des descentes. Le nombre de passes catalogue,
les capacités d'arène et les copies de compactage sont contrôlés séparément.
La référence géométrique indépendante appartient aux petites portes natives ;
le décodage des gros résultats vérifie structure et identité, pas une nouvelle
enumération Python de toutes les parties.

## Temps mesurés

Tous les tableaux sont en millisecondes, CPU sur G4. FULL inclut index,
domaine catalogue/lookup, forêts et verticales. Cloud, Pool, lecture et
écriture du fichier restent hors de ce temps moteur ; `metrics.json` conserve
ces durées et CPU/processus/décodage, les quatre phases de chaque ordre, toutes
les phases du catalogue et les réservations de chacun des 27 essais.

FULL K1..5 :

| Entrée | Bits | Mode 15 | Mode 63 | Mode 127 |
|---|---:|---:|---:|---:|
| lidar_ng00 | 21 | 5206.347 | 3710.175 | 3142.537 |
| lidar_ng00 | 24 | 5255.263 | 3733.806 | 3116.748 |
| lidar_ng01 | 21 | 3635.244 | 2979.421 | 2481.717 |
| lidar_ng01 | 24 | 3716.205 | 3040.179 | 2540.907 |
| lidar_ng02 | 21 | 4867.356 | 3846.666 | 3127.103 |
| lidar_ng02 | 24 | 4962.273 | 3896.612 | 3243.097 |
| uniform_u18_n8000 | 21 | 1248.057 | 1082.611 | 943.309 |
| uniform_u18_n16000 | 21 | 2777.950 | 2324.947 | 2150.255 |
| uniform_u18_n32000 | 21 | 6184.576 | 5186.706 | 4863.845 |

Domaine catalogue/lookup seul :

| Entrée | Bits | Mode 15 | Mode 63 | Mode 127 |
|---|---:|---:|---:|---:|
| lidar_ng00 | 21 | 3030.840 | 1520.637 | 844.518 |
| lidar_ng00 | 24 | 3028.428 | 1512.884 | 880.047 |
| lidar_ng01 | 21 | 1888.273 | 1226.642 | 729.823 |
| lidar_ng01 | 24 | 1946.520 | 1265.498 | 769.025 |
| lidar_ng02 | 21 | 2589.419 | 1569.933 | 871.904 |
| lidar_ng02 | 24 | 2665.189 | 1598.583 | 949.488 |
| uniform_u18_n8000 | 21 | 427.667 | 255.167 | 126.087 |
| uniform_u18_n16000 | 21 | 907.016 | 450.768 | 275.563 |
| uniform_u18_n32000 | 21 | 1978.977 | 901.459 | 595.493 |

Forêts/verticales seules :

| Entrée | Bits | Mode 15 | Mode 63 | Mode 127 |
|---|---:|---:|---:|---:|
| lidar_ng00 | 21 | 2175.089 | 2189.113 | 2297.587 |
| lidar_ng00 | 24 | 2226.409 | 2220.489 | 2236.277 |
| lidar_ng01 | 21 | 1746.597 | 1752.406 | 1751.535 |
| lidar_ng01 | 24 | 1769.322 | 1774.318 | 1771.515 |
| lidar_ng02 | 21 | 2277.507 | 2276.298 | 2254.764 |
| lidar_ng02 | 24 | 2296.652 | 2297.593 | 2293.179 |
| uniform_u18_n8000 | 21 | 820.326 | 827.374 | 817.161 |
| uniform_u18_n16000 | 21 | 1870.828 | 1874.073 | 1874.576 |
| uniform_u18_n32000 | 21 | 4205.407 | 4285.052 | 4268.157 |

Sur LiDAR, mode63→127 : domaine ×1,646–×1,801 et FULL ×1,181–×1,230.
C'est l'ablation une passe, mesurée avec les autres options identiques.
Mode15→63 combine deux options ; il n'isole pas leurs effets individuels.
Les variations de forêt ne sont pas attribuées au catalogue ; un seul essai
par unité ne donne pas une incertitude de chronométrage.

Mur campagne335,475951s : processus natifs108,567168s, inspection/hachage
sémantique225,609073s, autres opérations1,299710s. Ne pas additionner CPU
cumulé et temps mur ni attribuer l'inspection Python au moteur.
Pic Buffer maximal, toutes entrées/modes :649 988 028 octets (uniforme32k,
u21, mode127). Ce n'est pas un pic LiDAR ni RSS ; piles, Python et petites
métadonnées ne sont pas inclus. Les pics par essai et octets retenus sont
dans `metrics.json`. La génération une passe peut augmenter le pic car ses
blocs et le compactage coexistent.

**FULL≤200ms n'est pas acquis.** Aucun temps K10, GPU, préparation de grille,
segmentation du sol, hiérarchie de points ou clustering n'est acquis ici.
Cette capture n'attribue aucune qualification au code ajouté après90dd,
notamment verticales parallèles ou census emprunté.

## Relecture et limites de preuve

```sh
python3 receipts/catalogue_single_full_20261003/combined3/check.py
python3 -O receipts/catalogue_single_full_20261003/combined3/check.py
python3 receipts/catalogue_single_full_20261003/combined3/check_selftest.py
python3 -O receipts/catalogue_single_full_20261003/combined3/check_selftest.py
```

Code0 signifie cohérence des preuves ; le champ `conforming` tranche la
campagne. Les branches d'échec, d'omission budgétaire et de checkpoint ouvert
restent non conformes et sont exercées par l'auto-test.

Une seule archive originale est conservée :654 516 octets,
SHA256 `0adf332b88f474f23d70b963eba3d0604b6becd4783aa572b9bcc8a77f321e71`.
Lecture tar par `extractfile`, sans extraction. `receipt.json` reste comparé
au reçu brut local obligatoire et à son hash ; aucun mode archive autonome.
`matrix.json`, `asan18.json` et `full_parallel.json` sont des copies exactes
des membres archivés. `inputs.json` est repris du manifeste antérieur seulement
après égalité de hash et taille avec le manifeste uploadé attesté dans le brut.
Aucun payload KITTI, paquet source complet ni binaire n'est copié.

**Les 27 payloads FULL ont été supprimés par le collecteur distant.** Le lecteur
ne prétend pas les rehacher :27 empreintes enregistrées, zéro payload gros
rehaché. Neuf inspections décodées et18 réemplois ont leurs chaînes recoupées :
source antérieure réussie, contexte complet, source du décodeur, SHA256/tailles
et copie du résumé. Un petit payload synthétique archivé est réellement
rehaché et décodé dans l'auto-test, avec corruption et doublon refusés.

Les neuf helpers benchmark de `source_90dd/` proviennent de Git90dd, vérifiés
avant import. Les28 petits helpers historiques nécessaires sont figés sous
`legacy/` (333345 octets), également recoupés avec Git90dd et hachés dans
`source_contract.json`. Ils conservent leur arborescence pour les chemins
`__file__` ; aucune dépendance aux capsules retirées du sparse checkout.
Leur source historique interne reste distinguée de la campagne90dd.
Les incidents de préparation sont conservés dans `reader_development.json`.

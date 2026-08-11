# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans matérialiser de mosaïque de Delaunay d'ordre
supérieur. Le profil traité est le nuage quantifié u16; il n'affirme rien sur
le nuage réel antérieur à la quantification.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict courant

Le contrat n'est pas rempli. La spécification fixe comme cible principale un
p95 `warm_e2e < 100 ms` à 50 000 points et `K=10`; la seconde est une cible
secondaire et le jalon immédiat demandé. Aucun chemin actuel n'atteint l'une ou
l'autre, et aucun backend public exact n'est qualifié.

Snapshot committé audité :
`cbac109a09c2575cdf875b19de1570265bd5bf08`. Les livraisons stables utiles
sont :

| commit | contenu | verdict |
| --- | --- | --- |
| `84ba459` | fast principal multi-lot sous garde `q<=k+1` | reçu relativement à la table fournie |
| `3c13cbd`, `4b9d9a1` | session et sorties brutes G4 mass-only | reçus de diagnostic, aucune lane admise |
| `9483b1c` | factory `ValidatedHybridSidecar` v0 | livrée, non reçue comme frontière de confiance |
| `cbac109` | reçu scellé, clé de centre, cardinal minimal recalculé, support déclaré validé et fold typé | corrections réelles, réception encore bloquée |

Le worktree concurrent ajoute une sonde q2 dual-tree sur arbre AABB médian;
elle reste non reçue jusqu'à stabilisation de son empreinte et nouveau
différentiel. Le détail exact est tenu dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) et
[`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](audits/AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md).

## Deux sorties à ne pas confondre

| contrat | contenu | état v3 |
| --- | --- | --- |
| Gamma exhaustif | facettes, cofaces, incidences silencieuses, lots, `coverage_delta` et verticales | spécifié, non scalable et non qualifié à 50 k |
| `hgp_reduced_normalized_h0_v3` candidat | composantes horizontales exactes, niveaux exacts et union des `PointId`, avec quotient certifié des blocs H0 silencieux | pont mathématique disponible; resolver, source et verticales non reçus |

Une boule H0-inerte peut encore porter de vraies incidences Gamma. Le quotient
horizontal doit donc avoir son propre schéma et ne peut jamais revendiquer une
sortie Gamma/v2 byte-identique.

## Ce qui est établi

- À `k=1`, les partitions strictes et fermées sont celles du single-linkage et
  peuvent être produites par un EMST exact. Le Prim quadratique actuel est un
  juge borné, pas la lane 50 k.
- Pour une boule avec `p` points strictement intérieurs et un support propre
  positif de taille `q`, les ordres `1<=k<=p+q-2` sont des continuations H0
  sans fusion ni nouveau `PointId`.
- À `K=10`, une preuve `p+q_cert>=12` autorise une tombstone pour le seul
  quotient horizontal, avec resolver latent. Les seuils témoins des lanes
  q2/q3/q4 sont respectivement `10/9/8`.
- `q_min` décrit la provenance Morse; `q_cert` est la plus grande arité d'un
  support propre positif effectivement certifié. Ils n'ont ni le même champ ni
  la même sémantique.
- Le fast principal d'un lot multiple exige `q<=k+1`, une vraie
  `CarrierClosure` et des carriers stricts résolus dans le snapshot pré-lot.
  `q>k+1` reste au fallback.
- `prefix-all` est exact relativement à la `GeneratorTable` reçue; il ne prouve
  jamais la complétude géométrique de cette table.

La preuve et ses contre-fixtures sont consolidées dans
[`REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`](audits/REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md).

## Ce que la G4 a réellement mesuré

La session 50 k a utilisé les 48 vCPU de la machine G4; aucun kernel GPU n'a
tourné. La sonde a construit les banques, dilations et comptes, sans former un
seul tuple, sans census terminal, sans fold et sans payload.

| lane après prune d'axe | minimum observé | maximum observé | verdict |
| --- | ---: | ---: | --- |
| q2 | 465 371 500 | 2 862 879 000 | moins massive, non admise |
| q3 | 14 667 530 000 | 131 762 100 000 | rouge |
| q4 | 330 437 400 000 | 9 968 861 000 000 | rouge |

Les temps count-only vont de `0,174 s` à `29,153 s`. Ils ne sont pas des temps
de source ni des temps `warm_e2e`. Le catalogue historique demande déjà
`60,931 s` à n=2 400 sur `terrain`, `77,119 s` à n=2 400 sur scanline et
`675,407 s` à n=6 250 sur `terrain`.

Le pinceau q4 par triples n'évite pas le verrou actuel. Au pas 6, même le choix
canonique des trois plus petits identifiants impose avant toute requête plus de
`2,74e9` triples sur `terrain`, `1,063e10` sur scanline simple et `1,020e9`
sur multi-écho. Il reste un oracle/fallback borné, pas la prochaine source
produit.

Les reçus bruts sont
[`cell_50k_raw.txt`](receipts/g4_massonly_20260811/cell_50k_raw.txt) et
[`mask_scale_raw.txt`](receipts/g4_massonly_20260811/mask_scale_raw.txt), de
SHA-256 respectifs
`6b355d0d9c7bf01dbdeb1d14dc442cab75570e6be044dcd50f314d79b9010afe`
et `d82e43c7f4b32a5731cfdb2bbb9edf22cd7cecef0fdc73e84d1457277d61c740`.

## Sidecar : statut exact

Le commit `cbac109` retire le carré de niveau `i128`, vérifie
substantiellement le support minimal et transmet le sidecar au wrapper de fold.
Ces corrections sont effectives, mais la réception reste bloquée :

- `SourceProducerToken` est vide et trivialement copiable; un
  `std::bit_cast` C++20 fabrique le token exigé par le constructeur public du
  reçu. Une table amputée, ses digests recalculés et ce token obtiennent encore
  `closure_certified_all_orders()==true`;
- l'index trie les boules de même centre par indice de catalogue et ne compare
  que les voisines. Un catalogue `[rayon 1, rayon 2, rayon 1]` fait accepter
  deux handles pour la même boule exacte;
- une représentation hostile `nx=INT128_MIN, den=1` déclenche
  `-INT128_MIN` dans `sidecar_gcd` sous UBSan avant tout refus;
- le support déclaré est validé comme support minimal, mais le tie-break
  canonique coordonné du contrat n'est pas reconstruit;
- la sérialisation FNV64 reste endian native, sans schéma ni framing
  contractuel. Le digest du catalogue contient le support déclaré, mais le
  digest final ne lie pas séparément les preuves de suppression calculées,
  `maximum_order` ou les fermetures.

Le pipeline hybride est en outre structurellement borné à `n<=32` : la CLI
impose `smax<=32` tandis que la fermeture exige `smax>=n`. Sur un run accepté,
il construit le catalogue puis le reconstruit séquentiellement dans le
producteur scellé. Ce chemin est un oracle CPU borné à double énumération,
jamais la source chaude 50 k.

## Architecture candidate

```text
k=1 : EMST exact distinct

k>=2 : points u16 + LBVH résidents
  |-> q2 : self-produit de paires, prune par 10 témoins communs
  `-> q3/q4 : source sparse d'ancres diamètre encore à prouver
        -> centres q3 et niveaux shallow q4 dans le disque de Jung
  -> census terminal et BallActivation streamées
  -> tombstones H0 + resolver latent
  -> fast principal / fallback préfixe
  -> lots atomiques et sortie horizontale normalisée
```

Dans un arrangement en position générale, une borne de niveaux peu profonds
peut retirer le carré local seulement si le constructeur ne forme jamais
d'abord toutes les intersections de cordes. Les parallèles, concurrences et
coquilles multiples exigent une porte exacte distincte; aucune borne linéaire
n'est actuellement reçue dans ces cas.

La parcimonie globale du nombre d'ancres et de la somme des formes par ancre
n'est pas prouvée.
L'ancien `center-cover` a dépassé 600 secondes à 50 k; il est rejeté comme
implémentation. Le prochain prototype doit être une nouvelle sonde par blocs,
sans boucle ancre--nuage, et non une réactivation de ce chemin.

La source uniforme par cellules, son plan séparateur et l'anisotropie restent
des falsificateurs mass-only. Ils ne redeviennent candidats que si leurs
préflights de tuples et de triples passent une enveloppe mesurée.

## Prochaines portes, dans l'ordre

1. Tuer la forge fraîche, le doublon concentrique `[r1,r2,r1]` et
   `INT128_MIN`; fermer le tie-break du support et remplacer FNV par un
   engagement canonique complet avant de recevoir le sidecar comme oracle
   borné `n<=32`.
2. Fermer la porte locale du plan séparateur, y compris la contre-fixture où un
   support `beta>=Q` subsiste; publier les masses de triples, pas seulement le
   nombre de cellules.
3. Corriger puis recevoir la sonde q2 self-join avec
   `pruned + microtile = C(n,2)`, rejeu non compensable de chaque bloc pruné,
   multiplicité un de chaque paire, CTests et compteurs d'états, visites,
   paires, files et octets. Le run local mono-thread
   `n=50 000, terrain, leaf=64` n'a pas terminé dans sa fenêtre de 60 s
   (censuré, hors G4, sans reçu de sortie); il n'établit donc aucun chemin sous
   la seconde.
4. Convertir ces masses en enveloppe de temps; refuser la route si les
   microtuiles ou les visites restent quadratiques. En parallèle, prouver une
   source sparse complète des ancres q3/q4 avant tout sweep CUDA.
5. Recevoir `BallActivation`, le resolver décroissant et le quotient local des
   grandes coquilles contre Gamma exhaustif à petit `n`.
6. Porter sur CUDA seulement les primitives dont les masses terminales sont
   admises, puis mesurer `count-only`, source+census, fold et enfin
   `warm_e2e` complet.

Une insuffisance de ressource refuse ou reprend exactement; elle ne tronque
jamais une sortie. Aucun tableau global de tuples, de faces, de cofaces ou
d'incidences n'est autorisé dans le chemin produit.

## Construire les juges locaux

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel
ctest --test-dir build/v3 --output-on-failure
```

Ces commandes valident des portes locales. Elles ne promeuvent ni la source,
ni la performance, ni le statut public.

## Autorités documentaires

- [`PROPOSITION.md`](PROPOSITION.md) : architecture candidate et gates.
- [`audits/README.md`](audits/README.md) : index courant et archives.
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : verdict du
  snapshot et du worktree.
- [`../docs/SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) :
  contrat.
- [`../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) :
  statut des preuves.

GCP non utilisé pour cette consolidation.

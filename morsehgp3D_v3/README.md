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
`232470cbaf2449e5e68c92f2c42c532c4df20458`. Les livraisons stables utiles
sont :

| commit | contenu | verdict |
| --- | --- | --- |
| `84ba459` | fast principal multi-lot sous garde `q<=k+1` | reçu relativement à la table fournie |
| `3c13cbd`, `4b9d9a1` | session et sorties brutes G4 mass-only | reçus de diagnostic, aucune lane admise |
| `9483b1c` | factory `ValidatedHybridSidecar` v0 | livrée, non reçue comme frontière de confiance |
| `232470c` | index documentaire du sidecar | snapshot documentaire courant |

Un worktree concurrent corrige le sidecar et ajoute un plan séparateur pour la
sonde par cellules. Ces changements restent non reçus jusqu'à stabilisation de
leurs empreintes et nouveau différentiel. Le détail exact est tenu dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) et
[`AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md`](audits/AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md).

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

La v0 committée est utile comme harnais CPU borné, mais ne porte pas encore la
confiance annoncée :

- le reçu du commit est fabricable avec les digests d'une table amputée;
- le pipeline le reconstruit depuis `smax`, `n` et le statut de l'énumération;
- le sidecar est détruit avant le fold, qui reçoit encore le catalogue brut;
- la clé de rayon forme des carrés pouvant dépasser 128 bits sur u16;
- `q_min` et le support propre positif ne sont pas reconstruits;
- le digest FNV de l'image mémoire n'est pas canonique;
- le census `O(G*n)` en fait un juge borné, jamais un chemin chaud 50 k.

Le correctif concurrent traite une partie de ces points, mais n'est pas encore
reçu. La frontière finale doit être issue d'un producteur terminal rejouable,
être consommée par le fold et porter une sérialisation canonique exacte.

## Architecture candidate unique

```text
k=1 : EMST exact distinct

k>=2 : points u16 + LBVH résidents
  -> self-join canonique de blocs de paires
  -> center-cover fail-open, seuils 10/9/8
  -> ancres diamètre résiduelles
  -> cordes dans le disque de Jung
  -> niveaux de profondeur faible construits directement
  -> census terminal et BallActivation streamées
  -> tombstones H0 + resolver latent
  -> fast principal / fallback préfixe
  -> lots atomiques et sortie horizontale normalisée
```

Pour une ancre q4 ayant `m_e` cordes et `c_e` intérieurs constants, le rang
vaut `4+c_e+profondeur_stricte` et le nombre de sommets utiles est au plus
`m_e*(8-c_e)`. Cette borne retire le carré local seulement si le constructeur
ne forme jamais d'abord toutes les intersections de cordes.

La parcimonie globale du nombre d'ancres et de `M=sum_e m_e` n'est pas prouvée.
L'ancien `center-cover` a dépassé 600 secondes à 50 k; il est rejeté comme
implémentation. Le prochain prototype doit être une nouvelle sonde par blocs,
sans boucle ancre--nuage, et non une réactivation de ce chemin.

La source uniforme par cellules, son plan séparateur et l'anisotropie restent
des falsificateurs mass-only. Ils ne redeviennent candidats que si leurs
préflights de tuples et de triples passent une enveloppe mesurée.

## Prochaines portes, dans l'ordre

1. Stabiliser puis contre-auditer les correctifs sidecar : forge fraîche,
   multiprécision, support redondant/dépendant, digest canonique et consommation
   réelle par le fold.
2. Fermer la porte locale du plan séparateur, y compris la contre-fixture où un
   support `beta>=Q` subsiste; publier les masses de triples, pas seulement le
   nombre de cellules.
3. Construire P1a mass-only `self-join -> center-cover -> cordes`, avec
   `pruned + microtile = C(n,2)`, transcript rationnel à petit `n`, compteurs
   `a`, `M`, `c_e`, `sum Z_e`, files, ambiguïtés et octets.
4. Refuser P1a si `source-cover + cordes > 400 ms` chaud sur G4, si la majorité
   des paires atteint les microtuiles ou si le travail contient
   `sum_e m_e^2`.
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

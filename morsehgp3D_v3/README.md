# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans matérialiser la mosaïque de Delaunay d'ordre
supérieur. La cible d'échelle est `50 000 points`, `K=10`, sur une machine G4,
avec un objectif secondaire `warm_e2e < 1 s`.

Ce README décrit uniquement l'état courant. Les chronologies, contre-exemples
et anciens snapshots restent dans [`audits/`](audits/README.md).

## Statut courant

Snapshot committé audité :
`ab5a3c86f032bb793b868a9162c3eb299a1f100c`.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`.

Le contrat 50 k/G4/seconde n'est pas rempli. Aucun backend public exact n'est
qualifié. Un chantier non committé ajoute actuellement le fast path principal
dans les lots ex æquo; ses empreintes et ses obligations de réception sont dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

Le pipeline `hybrid`/`hybrid-prefix` committé exige encore `smax>=n`, tandis que
le type historique borne le rang à 32. Il ne peut donc pas être le pipeline
50 k. Le mode `prefix-all` contourne cette garde pour juger exactement un fold
relatif à une table partielle; il ne transforme pas cette table en source
géométriquement complète.

## Deux contrats à ne pas confondre

| contrat | contenu | état |
| --- | --- | --- |
| Gamma/v2 exhaustif | cofaces, incidences, lots silencieux, identifiants et verticales | spécifié, non scalable et non qualifié à 50 k |
| `hgp_reduced_normalized_h0_v3` candidat | composantes horizontales exactes et union des `PointId`, avec quotient des blocs H0 silencieux | pont mathématique disponible, contrat produit et verticales non reçus |

Une boule peut être silencieuse pour H0 tout en portant de vraies facettes ou
cofaces Gamma. Toute sortie qui applique le nouveau quotient doit donc être
versionnée séparément; elle ne peut pas revendiquer l'identité du payload v2.

## État du pipeline

```text
points u16 exacts
  -> source géométrique / catalogue borné CPU
  -> sidecar de certificats
  -> lots de niveau exact
  -> fast support-principal + fallback préfixe exact
  -> DSU et transcript borné
```

Cette chaîne est une référence CPU et un ensemble de falsificateurs. Le chemin
produit 50 k proposé remplace la première flèche et les payloads bornés par :

```text
points résidents + index spatial
  -> BallActivation à coquille variable, streamées par cellule owner
  -> tombstones H0 certifiés + activations pertinentes
  -> carriers stricts / resolver latent
  -> fold sparse atomique par lot
  -> sortie horizontale normalisée
```

La proposition complète est dans [`PROPOSITION.md`](PROPOSITION.md).

## Portes CPU reçues

- Index préfixe : possession temporelle canonique des paires par lots,
  terminaison hostile, longueur indépendante `L`, préflight
  `predicted_hits==actual_hits`, recertification et ledger pré-DSU.
- `prefix-all` : différentiel permanent contre le fold quadratique sur la même
  `GeneratorTable`, y compris lorsqu'elle est partielle.
- Lots ex æquo : factorisation exacte par carriers stricts sous hypothèse de
  fermeture et handle unique par boule.
- `k=1` : comparaison de la partition canonique des `PointId` au replay EMST,
  avant et après chaque niveau exact.
- Catalogue parallèle : résultats séquentiels/parallèles comparés en
  multiensemble, scratch et métriques par worker; campagne TSan déclarée verte
  sur la livraison courante.
- Source cellules : préflight count-only exact de ses listes et masses; cette
  porte diagnostique un refus de coût, elle ne cappe jamais une énumération.

Les commandes, compteurs et fixtures de la livraison sont dans
[`NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md`](audits/NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md).

## Pont mathématique actuellement retenu

Pour une boule fermée `B`, soit `p` le nombre de points strictement intérieurs
et soit `q` l'arité d'un support propre positif. Le théorème 4.2 de
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) prouve :

$$1\leq k\leq p+q-2\Longrightarrow B\text{ ne produit ni fusion }H_0\text{ ni nouveau PointId à l'ordre }k.$$

Conséquences pour `K=10` :

- une preuve positive `p+q>=12` permet de tombstoner la boule pour tout le
  quotient horizontal demandé ;
- les banques exactes de la source par cellules ont les tailles `10/9/8` pour
  les supports `q=2/3/4` ;
- `q_min` reste la provenance Morse, tandis que la plus grande arité positive
  effectivement certifiée, `q_cert`, fournit la meilleure preuve d'inertie ;
- un ensemble cosphérique redondant n'est jamais un support propre positif ;
- l'omission exige un resolver de carriers silencieux et ne dispense pas des
  preuves verticales.

La preuve constructive, le resolver, le quotient local par `Omega`, les
fixtures et la portée exacte sont consolidés dans
[`REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`](audits/REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md).

## Fast path et fallback

Le fast path d'un générateur principal dans un lot multiple est sûr seulement
si toutes les conditions suivantes sont vraies :

- `q<=k+1` ;
- support principal certifié ;
- `CarrierClosure` liée aux digests et à la complétude de l'ordre ;
- chaque face `S_u` résolue dans le snapshot pré-lot à un niveau strictement
  inférieur.

Le cas `q>k+1` reste au fallback dans un lot multiple. Un lookup égal ou absent
sous prétention complète est un refus atomique, jamais une chaîne de carriers
du niveau courant. La capability cible est `ValidatedHybridSidecar`; un simple
booléen CLI n'est pas une frontière de confiance.

Le fallback préfixe utilise un même ordre global, des préfixes de longueur
`rank-k+1`, un préflight exact des hits et une recertification réelle de
`|M intersection N|>=k`. Sur les familles scanline de la livraison, 79--85 %
des générateurs sont encore interrogés à cause des lots ex æquo. Le gain du
fast multi-lot doit être remesuré par catégorie, en conservant séparément le
fallback `q>k+1`.

## Verrou source à 50 k

La sonde uniforme par cellules de centres est rouge en lane q4. Sur `terrain`,
`cell-side=4`, seed `20260810`, elle prédit sans former les tuples :

| n | cellules | `R_2` | `R_3` | `R_4` |
| ---: | ---: | ---: | ---: | ---: |
| 400 | 4 375 | 2,16 M | 21,6 M | 159 M |
| 1 600 | 32 500 | 32,9 M | 560 M | 7,82 G |
| 2 400 | 66 978 | 94,4 M | 2,03 G | 36,8 G |

Ces nombres interdisent de porter l'énumération aveugle des quadruplets sur
GPU. Ils ne sont ni une extrapolation 50 k ni un débit CUDA.

La prochaine réduction exacte est la condition nécessaire locale
`closure(C) intersect conv(A_C) != empty`. Un séparateur flottant peut proposer
un prune, mais seule sa revérification entière ou rationnelle sur tous les
points de `A_C` et les coins fermés de `C` l'autorise. Les masses après prune,
pas le nombre de cellules, décident l'admission.

Si cette réduction ne suffit pas, la baseline q4 est un pinceau par triple
canonique avec reporter terminal des zéros le long de sa droite de centres.
Scanner tous les points par triple recrée `4*R_4+3*R_3` et reste un NO-GO.

## Ce que la G4 a réellement montré

Le flux device déjà mesuré trie et réduit un catalogue préconstruit. Il valide
des primitives GPU et un débit de join borné; il n'inclut pas la construction
de la source, la certification, les verticales ni un run 50 k bout en bout. Il
ne faut donc pas citer son temps comme latence produit.

Le prochain passage G4 n'est utile qu'après admission CPU des masses et
réception des prédicats exacts. L'ordre de mesure est :

1. count-only et arènes ;
2. source streamée ;
3. fold et lots ;
4. `warm_e2e` complet.

## Priorités d'implémentation

1. Recevoir le fast principal multi-lot avec la fixture `q=k+2`, les lookups
   égal/manquant, les permutations et les records complets.
2. Construire `ValidatedHybridSidecar` et rendre impossible une fausse
   `CarrierClosure`.
3. Ajouter le prune convexe à la sonde, puis un dispatcher exact par lane.
4. Introduire `BallActivation`, `q_min/q_cert`, tombstones, handles latents et
   census de coquille variable.
5. Recevoir le resolver décroissant et `Omega` contre des oracles exhaustifs à
   petit `n`.
6. Porter seulement les routes admises sur CUDA et mesurer la G4 avec reçus
   séparés.

## Construire et lancer les portes

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 -j
ctest --test-dir build/v3 --output-on-failure
```

Pour les audits ciblés, préférer les exécutables et CTests nommés
`prefix_index`, `postings_join`, `saturated_pipeline_prefix_all`,
`structural_scale`, `parallel_catalogue` et `cell_source_mass`. Une campagne
locale ne promeut aucun statut public; les reçus doivent rester liés au commit,
au binaire, au profil, à la graine et aux digests d'entrée.

## Documentation courante

- [`PROPOSITION.md`](PROPOSITION.md) : architecture candidate unique.
- [`audits/README.md`](audits/README.md) : index courant et règle d'archive.
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : verdict de
  snapshot.
- [`../docs/SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) :
  autorité du contrat existant.
- [`../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) : registre des preuves.

Les audits datés sont conservés comme preuves historiques. Leurs phrases au
présent ne décrivent pas l'état live lorsqu'elles précèdent le snapshot ci-dessus.

GCP non utilisé pour cette consolidation.

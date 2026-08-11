# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=exact_bounded_and_scale_design`,
`public_status=not_claimed`.

## Snapshot et règle de lecture

La dernière base committée auditée est
`ab5a3c86f032bb793b868a9162c3eb299a1f100c`. Un chantier concurrent du fast
path ex æquo modifie encore les prototypes et CMake; il n'est pas reçu par ce
document. Les SHA-256 de ce chantier au dernier contrôle étaient :

- `saturated_fold_hybrid.hpp` :
  `fdf65fcc070954ce876f1b62e7cd680e78ea8f32b30a26e5222eabf7db8eb8a2` ;
- `postings_join_gate.cpp` :
  `5da5908e1e1b7cbca4301a5465d53a732c0ccc8bbce34447679ade13a2ab02b3` ;
- `CMakeLists.txt` :
  `ed35abed3113452cebeb400ef9b36f97acf36a667d23eb3ee414bd2c70f17ea7`.

Toute modification ultérieure impose un nouveau reçu. Les audits datés plus
anciens décrivent leur propre snapshot; ils ne remplacent pas ce verdict.

## Verdict produit

MorseHGP3D v3 ne satisfait pas encore le contrat `50 000 points / K=10 / G4 /
warm_e2e < 1 s`. Aucun backend public exact n'est qualifié.

Le verrou principal n'est plus la seule union DSU. Il est la production exacte
et admise de la source : le pipeline `hybrid`/`hybrid-prefix` committé refuse
encore `smax<n`, alors que le type historique borne le rang à 32. Le diagnostic
G4 positif porte sur un join d'un catalogue déjà construit, pas sur la source,
les verticales ni le bout en bout à 50 k.

Deux contrats doivent rester séparés :

1. le transcript Gamma/facetté v2, ses incidences et ses verticales ;
2. le candidat `hgp_reduced_normalized_h0_v3`, qui conserve exactement les
   composantes horizontales et l'union des `PointId`, mais quotient les blocs
   silencieux.

Le pont de haut rang reçoit le second contrat seulement. Il ne rend pas le
premier byte-identique.

## Ce qui est reçu sur la base committée

- L'index préfixe CPU a une possession temporelle canonique par lots, une masse
  indépendante `L`, un préflight `predicted_hits==actual_hits`, un ledger
  pré-DSU et des fixtures hostiles de calendrier.
- `prefix-all` est une vérité exacte relative à toute `GeneratorTable` fournie
  et son CTest permanent compare le fold de référence. Il ne certifie pas la
  complétude géométrique de cette table.
- La factorisation des lots ex æquo par carriers stricts est mathématiquement
  correcte sous fermeture de source et handle unique par boule.
- La porte structurelle `k=1` compare les partitions canoniques de `PointId` à
  l'EMST aux coupes stricte et fermée.
- Le catalogue parallèle emploie métriques et scratch par worker; la campagne
  TSan déclarée par Claude est verte sur le snapshot livré.
- La sonde count-only par cellules de centres est exacte pour ses masses; elle
  montre que l'énumération aveugle des quadruplets est rouge, et non qu'une
  source 50 k est disponible.

Les commandes, campagnes et nombres correspondants sont consignés dans
[`NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md`](NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md).

## Pont mathématique décisif

Pour une boule `B`, noter `p` son nombre de points strictement intérieurs et
`q` l'arité d'un support propre positif. Le théorème 4.2 déjà `proved_here`
donne :

$$1\leq k\leq p+q-2\Longrightarrow B\text{ est une continuation }H_0\text{ sans fusion ni nouveau PointId à l'ordre }k.$$

Il faut distinguer `q_min`, provenance Morse, de la plus grande arité positive
effectivement certifiée, `q_cert`, qui fournit le meilleur certificat
d'inertie. Une preuve `p+q_cert>=K+2` permet de tombstoner la `BallKey` pour le
quotient horizontal à tous les ordres demandés. Une combinaison positive
affinement dépendante ne constitue jamais un tel support.

Composé à la source par cellules, ce théorème donne les banques exactes
`t_q=K+2-q`, donc `10/9/8` pour `K=10` et `q=2/3/4`. Tous les témoins stricts
prouvent l'inertie; sinon un témoin non intérieur prouve que le census local
contient support, intérieur et coquille complète.

La preuve, le resolver décroissant, le quotient local par `Omega` et les
fixtures sont dans
[`REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`](REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md).

## Fast path ex æquo : décision exacte

Le fast path d'un support principal est licite dans un lot multiple uniquement
si `q<=k+1`, si le support principal est certifié, si la fermeture des carriers
est une capability validée et si chaque `S_u` se résout dans le snapshot
pré-lot à un niveau strictement inférieur.

Le code concurrent a ajouté la garde `q<=k+1` après qu'un premier delta aurait
pu traiter `q=k+2` avec zéro attache. Ce défaut n'est donc plus présent dans le
SHA courant ci-dessus, mais la correction et ses nouvelles gates restent à
recevoir. `q>k+1` demeure au fallback dans un lot multiple.

Un booléen `fast_exaequo` fourni à un `Catalogue` brut ne constitue pas encore
la capability `CarrierClosure`. La cible reste une factory
`ValidatedHybridSidecar` liée aux digests, à l'unicité des `BallKey`, aux
saturés, à `q_min` et à la complétude par ordre.

## Verrou source et route constructive

La première réduction à sonder est la condition nécessaire locale
`closure(C) intersect conv(A_C) != empty`. Une cellule strictement séparée de
`conv(A_C)` ne peut posséder le centre d'aucun support propre. Le prune n'est
autorisé qu'avec un séparateur entier ou rationnel revérifié sur tous les points
de `A_C` et les huit coins fermés de `C`; le simple résultat flottant propose,
mais ne décide jamais.

Si q4 reste rouge après ce prune, la baseline exacte est un pinceau par triple
non collinéaire de `A_{4,C}` avec range reporter terminal le long de la droite
des centres. Scanner tous les points pour chaque triple recrée exactement
`4*R_4+3*R_3` et est interdit comme architecture d'échelle. Les extrémités
rationnelles doivent utiliser un type `PencilInterval` à largeur prouvée, pas
être forcées dans le type `Sphere` issu de supports.

## Portes suivantes, dans l'ordre

1. Recevoir le fast ex æquo courant : fixture `q=k+2`, lookup égal/manquant,
   refus atomique, permutation, records et couverture comparés au juge.
2. Construire `ValidatedHybridSidecar`; aucun booléen CLI ne doit pouvoir
   fabriquer la fermeture.
3. Ajouter le prune convexe count-only et publier les masses non linéaires
   après prune sur `terrain` et les deux familles scanline.
4. Introduire `BallActivation` à coquille variable, les certificats
   `q_min/q_cert`, le resolver et le différentiel `Omega`.
5. Rejouer le quotient lot par lot contre Gamma exhaustif à petit `n`, coupes
   stricte et fermée, tout en séparant explicitement le payload v2.
6. Porter seulement les primitives admises sur CUDA, puis mesurer dans l'ordre
   `mass-only`, source, fold et `warm_e2e`.

GCP non utilisé.

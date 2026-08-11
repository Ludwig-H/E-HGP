# Note de solution — source q2 par Morton/LBVH, Yao48 strict et census fermé (profil u16)

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note spécifie la **route produit candidate** de la lane q2 décrite par
l'audit courant et par
[`../PROPOSITION.md`](../PROPOSITION.md) §6.2. L'architecture mathématique est
celle de
[`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md)
(théorème de coupe directionnelle, classification terminale, ledger),
respécialisée au profil u16 de v3 où **toute l'arithmétique décisive tient en
`i64`/`i128` sans cascade dyadique**. Le statut logiciel appartient à
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## 1. Objet calculé

Pour le contrat `hgp_reduced_normalized_h0_v3` à `K=10`, la lane q2 doit
partitionner exactement les `C(n,2)` paires :

- **tombstone** : la paire possède au moins dix `PointId` distincts, hors
  extrémités, strictement intérieurs à sa boule diamétrale — `p>=10`, donc
  `p+q>=12`, bloc H0-inerte avec resolver latent ;
- **record fermé** : sinon, le census fermé complet
  `C(u,v)={x : Phi_{u,v}(x)<=0}`, sa profondeur stricte, sa coquille, son rang
  et son niveau `beta=D^2/4` sont publiés en une passe.

Le prédicat est `Phi_{u,v}(x)=(x-u) dot (x-v)` ; sur u16, `|Phi|<3*2^34` tient
en `i64` avec marge. Aucune position générale n'est supposée : les égalités
`Phi=0` sont des contacts de coquille, comptés fermés et jamais intérieurs.

## 2. Structures résidentes

1. **Ordre Morton** : clé 48 bits (trois axes u16 entrelacés), paires
   `(clé, PointId)` triées ; l'ordre canonique des ex æquo est le `PointId`.
2. **LBVH radix** : arbre binaire sur l'ordre trié, coupé au bit dominant de
   la première différence de clés (construction de type Karras, portable
   device) ; chaque nœud porte sa boîte AABB u16 exacte et sa plage
   `[begin,end)` de positions.
3. **Ownership exact une fois** : la paire `(i,j)` avec `pos(i)<pos(j)` est
   possédée par l'ancre de position haute `j`, qui ne parcourt que le préfixe
   `[0,pos(j))`. La masse totale possédée est `somme_j pos(j) = C(n,2)` :
   c'est l'univers comptable du ledger, jamais une énumération.

## 3. Coupe Yao48 stricte fail-open

Par ancre `p`, 48 chambres semi-ouvertes (8 octants × 6 permutations par
magnitudes décroissantes ; les égalités de magnitudes sont routées par une
règle totale documentée — la chambre est un choix de TRAVAIL, seule la preuve
compte). Une banque par chambre retient `K=10` témoins de `PointId` distincts
de `p`, avec `D` = maximum de leurs distances carrées à `p` (les plus proches
donnent le meilleur `D`, l'optimalité n'est pas requise). Une banque
sous-pleine n'autorise **aucun** cutoff : fail-open.

Pour une cible `q` de coordonnées canoniques `(x,y,z)` dans la chambre, les
trois inégalités **strictes**

$$x^{2}>D,\qquad (x+y)^{2}>2D,\qquad (x+y+z)^{2}>3D$$

certifient dix intérieurs **stricts** distincts (théorème de coupe
directionnelle, variante stricte) : la paire `(p,q)` est tombstonée sans
visite. Sur une boîte entière du LBVH contenue dans la chambre, les minima par
axe donnent le même certificat pour toutes ses feuilles : le nœud est remplacé
par un **reçu de masse**. Toute égalité descend ; l'échec du certificat ne
classe rien (non-converse gravé en fixture).

Le témoin égal à la cible est impossible par le même argument que la variante
fermée (`x^2>D` exclut `q` de la banque) ; le mutant qui l'admettrait est
gravé.

## 4. Classification terminale et census fermé

Chaque paire survivante `(u,v)` est classifiée par un parcours LBVH avec les
bornes exactes par boîte déjà reçues dans la lane self-join : l'infimum
séparable `L4` et le supremum `U4` de `4*Phi` sur la boîte.

- `L4>=0` : aucun point strict dans la boîte — retirée de la recherche
  d'intérieurs, **rescannée** obligatoirement pour le census fermé si
  `L4=0` peut porter des contacts ;
- `U4<0` : toute la boîte est strictement intérieure — crédit en bloc ;
- sinon descente, feuilles au prédicat exact.

L'arrêt anticipé à dix intérieurs stricts émet la tombstone ; sinon le census
fermé complet est publié (liste `C(u,v)`, profondeur stricte `p`, coquille,
rang fermé, niveau). Le rescan de census ne peut pas être évité par la
recherche stricte : contacts et intérieurs sont deux comptes.

## 5. Ledger et refus

Le ledger ferme simultanément, par lane et par run :

1. `candidate + certified_pruned + unresolved = C(n,2)` avec résidu nul pour
   une publication exhaustive (`unresolved>0` = refus atomique, jamais une
   sortie partielle) ;
2. la partition `tombstone + census` des candidates classifiées ;
3. multiplicité canonique un par paire (ownership rejoué) ;
4. l'identité du nuage (digest), du `leaf_size`, de l'ordre Morton et du
   seuil.

Aucun tableau global de paires n'est matérialisé : les survivantes du mode
mesure sont comptées et hashées, pas stockées ; le mode oracle borné
(`n<=256`) tient les sorts par paire pour le juge.

## 6. Juge indépendant et différentiel

Le juge borné réécrit sa propre arithmétique (audit : « le juge de couverture
ne partage pas les prédicats décisifs du sujet ») :

- prédicat recalculé sous la forme distincte
  `4*Phi = ||2x-u-v||^2 - ||u-v||^2` en `i128`, jamais la forme produit du
  sujet ;
- scan quadratique complet paires × points, sans Morton, sans LBVH, sans
  chambres ;
- comparaison de **tous** les sorts (tombstone/census), de toutes les
  profondeurs strictes, de tous les rangs fermés et de tous les census.

Le différentiel bi-mode du sujet (baseline sans coupe Yao48 ni prunes de
boîtes, classification terminale seule) doit rendre des sorts et masses
identiques ; il mesure le gain, il ne juge pas la vérité.

## 7. Portes exigées (planchers, fixtures, mutants)

- planchers : `--min-tombstones`, `--min-census-records`,
  `--min-region-prunes`, `--min-underfull-chambers` (le fail-open doit être
  EXERCÉ, pas absent), chacun à code 3 en cas de violation ;
- fixtures gravées : le non-converse `p=(0,0,0)`, `q=(2,0,0)`, `w=(1,1,0)` ;
  une égalité directionnelle exacte `x^2=D` qui doit descendre ; une chambre
  sous-pleine qui doit rester fail-open ; un contact de coquille compté fermé
  et jamais strict ; les extrêmes u16 ;
- mutants à code 4 : `strict-to-large` (l'égalité prune à tort),
  `d-understated` (banque de `K-1` témoins), `witness-is-target`,
  `ownership-doubled` (multiplicité deux), `last-region-omitted`,
  `census-skips-l4-zero` (le rescan des contacts sauté),
  `threshold-minus-one` ;
- équivariance : une permutation des `PointId` d'entrée rend le même ensemble
  canonique de sorts.

## 8. Exposants avant toute latence

Publier par famille (`uniform`, `terrain`, `scanline_single_pass`,
`scanline_overlap_multiecho`) à `12 500/25 000/50 000` : visites de nœuds,
tests `Phi` ponctuels, masses prunées par région, banques sous-pleines,
survivantes, tailles de census, octets et high-water. Deux exposants
consécutifs au-dessus de `1,35` classent la route `NO-GO` avant tout port ou
toute campagne de latence ; la comparaison de référence est le self-join q2
historique (53 à 724 millions de visites `L4`, 86 millions à 1,365 milliard de
tests ponctuels à 50 k).

Le pire cas de SORTIE reste quadratique (graphe de Gabriel dense) ; la gate
d'exposant juge le régime des familles G4, pas un théorème universel. Une
insuffisance de ressource refuse atomiquement.

GCP non utilisé pour cette note.

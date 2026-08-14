# Plan — du noyau ponctuel candidat au contrat 50 000, puis aux dizaines de millions

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce plan ne certifie rien et n'ouvre aucune phase. Il ordonne le travail restant
et attache à chaque jalon une **porte falsifiable** : ce qui doit être vrai pour
passer, et ce qui tue le jalon. Les autorités restent
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) et
[`PROPOSITION.md`](../PROPOSITION.md).

## 0. Où l'on est exactement

Le contrat `50 000` demande trente nuages frais, `K_max=10`, un GPU G4, profil
`hgp_reduced`, sur **deux** familles obligatoires — Poisson uniforme volumique
et mélange équilibré de huit amas. La sortie est
`BenchmarkOutputContract-v1` : dix forêts, applications verticales, lots,
certificat minimal, **copiés en mémoire hôte épinglée avant l'arrêt du
chronomètre**. Le SLO principal est `p95 < 100 ms`, le secondaire `p95 < 1 s`.

L'écart, dit sans adoucissement : la chaîne CPU complète met `78,8 s` sur
`uniform` à `50 000` et **ne produit aucun objet** — ni `BallKey`, ni
`SupportKey` avec `I_B/U_B`, ni census, ni fold, ni payload. Ce n'est donc pas
un facteur `790` à combler : c'est un facteur `790` **plus** un aval entier qui
n'existe pas.

Ce qui est acquis et le rend abordable : Q14 est fermée — aucune Delaunay,
ordre un inclus — et les trois lanes sont indépendantes. Le noyau
`Q4SeedAxisTopR4` reste un candidat ponctuel : la sélection extrémale et la mort
par gaps sont reçues dans leur domaine, et `a369452` refuse désormais les
replays `MORT_GAP` ou profonds. Il n'impose pas encore les préconditions
d'identités. L'exact-once courant ne confronte pas owner et primary à
une provenance indépendante.

## 1. Le seul chiffre qui décide de tout

À `50 000` points sur `uniform`, la chaîne a produit `21 413 140` `SupportKey`.
C'est `428` supports par point. Ce nombre, et non la vitesse, gouverne la
faisabilité :

| cible | supports/s exigés | octets/s à 32 o/support |
|---|---:|---:|
| `p95 < 1 s` | `21,4 M/s` | `0,69 Go/s` |
| `p95 < 100 ms` | `214 M/s` | `6,9 Go/s` |

La bande passante n'est pas le mur — une Blackwell en a plus de vingt fois
assez. Le mur est le **travail par candidat** : chaque support demande des
prédicats exacts en `i128`/`i256`. À `214 M/s` et deux cents opérations larges
par support, il faut `4,3·10^10` opérations entières larges par seconde. C'est
dans l'ordre du possible sur ce GPU, mais **seulement si le travail est
proportionnel à la sortie**. Aujourd'hui il ne l'est pas : la masse candidate
vaut `6 914` fois la population retenue.

**Conséquence de plan.** Tout le travail utile consiste à rendre le producteur
*output-sensitive*. Les optimisations constantes sont secondaires tant que le
rapport candidats/retenus reste à quatre chiffres.

## 2. J0 — mesurer l'objet avant de l'optimiser

**Objectif.** Connaître, sur les deux familles du contrat, à `12 500`, `25 000`
et `50 000` points : le nombre de supports par arité, le nombre de `Q4Seed3`
survivant au filtre de permanents `p < r4`, le nombre de paires survivant au
fuseau, et la taille des groupes d'égalité.

**Pourquoi d'abord.** Personne ne connaît la taille de l'objet à produire au
delà de `n=260`. Sans elle, aucune borne de coût n'est vérifiable et aucun choix
d'architecture n'est argumenté. C'est la mesure la moins chère et la plus
décisive du plan.

**Précondition.** Cette mesure ne devient une mesure de l'objet que lorsque la
source autonome de chaque lane est complète contre son oracle borné. Avant
cela, elle ne publie que des ledgers de candidats, jamais un nombre de supports
produit.

**Porte de sortie.** Trois exposants successifs publiés par arité, avec la
règle du plan de test : deux exposants successifs `> 1,35` suspendent tout.
Publier aussi `candidats/retenus` par lane.

**Ce qui tue le jalon.** Si `supports/point` croît encore au-delà de `50 000`,
la cible `100 ms` est arithmétiquement hors d'atteinte et le contrat doit être
renégocié en `p95 < 1 s`, ou `K_max` abaissé. **Il faut le dire à ce moment-là,
pas après six jalons.**

**Coût proposé, non reçu.** Une session G4 en zone IA, CPU 48 cœurs, moins
d'une heure. Les
oracles exhaustifs plafonnent à `n≈400` ; au-delà, la mesure emploie
l'énumération par ancre d'arête diamétrale, validée exacte contre le brute force
`C(n,4)`.

## 3. J1 — `Lane4` producteur autonome, sans certificat de bloc

**Objectif.** Remplacer la boucle exhaustive du probe par le vrai producteur :
split-tree Morton, génération des `Q4Seed3` owner/aigus, puis
`Q4SeedAxisTopR4` alimenté par une **recherche best-first sur l'octree**.

**Pourquoi cela peut s'ouvrir après les P0 du replay, alors que cinq certificats de bloc ont
échoué.** Parce que `Lane4` n'a plus besoin d'un certificat de profondeur
uniforme sur un rectangle. À `Q4Seed3` fixé, `A_z` est une quadratique convexe
séparable en `z` et `B_z` est linéaire : un nœud témoin se borne par clamp
rationnel et coins, et une descente best-first sélectionne les `k` extrêmes de
chaque signe sans jamais former de produit. Le verrou combinatoire est déplacé
là où il se résout, du bloc vers le seed.

**Porte de sortie.**
1. égalité de multiensemble `SupportKey` avec l'oracle borné, sur cinq familles,
   zéro manque, zéro doublon, mêmes owners, mêmes provenances primaires ;
2. `apex_pair_records = 0` et aucune allocation indexée par un univers de
   cofaces — la porte structurelle du contrat ;
3. `candidate_root_groups <= somme_f 2(r4 - p_f)` vérifiée sur le tape ;
4. compteurs publiés : `Q4Seed3Block`, visites de nœuds, comparaisons larges,
   groupes d'égalité, morts par cause, racines retenues, octets, HWM ;
5. mutant `corners_order_implies_all` tué : sur un `FaceBlock` où `a,b,x`
   varient, `G,W,n,T2` et l'ordre croisé des racines ne sont ni multiaffines ni
   convexes ; les coins seuls publieraient un faux `ALL`. La contre-fixture u16
   existe déjà dans l'audit.

**Ce qui tue le jalon.** Un nombre de `Q4Seed3` visités super-linéaire. C'est le
risque principal et J0 le mesure avant qu'on écrive la descente.

## 4. J2 — `Lane2` et `Lane3`, chacune autonome

`Lane2` est la plus facile : `MidballBlockDepth` est un prédicat de bloc
**exact** — `H(a,b,z)=(z-a).(b-z)` se sépare par axe, minimum aux sommets — et
`CKPairTape` donne déjà toutes les paires exactement une fois. Reste à publier
un bloc factorisé avec son census, `MIXED` remplaçant atomiquement le parent par
ses enfants, et `incomplete_continuation` au cap, jamais une omission.

`Lane3` demande `Q3MiniballDepth` sur la miniboule **ambiante** du triangle, pas
sur son circumdisque planaire, avec l'ordre imposé : `distinct-ID3`, owner parmi
trois arêtes, indépendance, acuité stricte, profondeur, puis `BallForm/BallKey`.

**L'interdit qui structure ce jalon.** Aucune lane ne lit la sortie d'une autre.
La mesure `seeds_rang_q3_mort` — `27 / 43 / 1` sur `uniform`, amas, terrain à
`n=50` — chiffre ce que coûterait la faute : des préfixes déjà morts pour q3
produisent des q4 pertinents. Le plancher `--min-q3-morts` garde cette porte des
deux côtés.

**Porte de sortie.** Les trois lanes produisent séparément, chacune comparée à
son oracle borné, et l'union des trois est comparée au brute force complet.

## 5. J3 — l'aval, qui n'existe pas

C'est le poste que le rapport de session chiffre à `1,2·10^6 ×` et que personne
n'a commencé : `BallKey` canonique, `RLE` avant census, `BallEvent`, fermeture
de descente de facette, forêts horizontales, applications verticales, payload
`BenchmarkOutputContract-v1`.

**Deux exigences qui doivent être prises tôt, pas après.** D'abord `0A` est
**rétractée** : ses numérateurs q3/q4 de 67 à 81 bits étaient rabattus en
`int64`, comportement indéfini sous UBSan. Le microkernel Gram unifié la
remplace, avec `|Phi| < 432·65535^8 < 2^137` et intermédiaires sous `2^139`,
donc `i192`. Ensuite le profil : la v3 est `quantized_u16_input_only` et le plan
de test est `binary64`. **Ce décalage doit être tranché avant l'aval**, sinon le
payload produit ne sera pas celui que le contrat évalue.

**Porte de sortie.** Le payload contractuel complet est produit sur les deux
familles, à `12 500` d'abord, et rejoué à l'identique bit à bit sur deux
machines.

## 6. J4 — la rampe CPU, avant tout GPU

`12 500 / 25 000 / 50 000` sur les deux familles obligatoires, avec la règle
d'arrêt du plan de test : deux exposants successifs `> 1,35` suspendent les
micro-optimisations et rouvrent l'architecture. Publier séparément la pente de
chaque lane et celle de l'aval : une pente globale masque laquelle des quatre
est rouge.

**Ce qui tue le jalon.** Un exposant supérieur à `1,35` sur `eight_clusters`.
C'est la famille qui a toujours résisté — résiduel q4 `89,9 %` là où `uniform`
tombait à `22,8 %` — et c'est elle qui décide, pas `uniform`.

## 7. J5 — le port CUDA et le contrat

Ordre imposé par ce qui précède : on ne porte que ce qui est déjà
output-sensitive et exact sur CPU. Le range-report LBVH `count--scan--emit` et
le CSR résident existent déjà en `P15-HOCUDA-P0` et restent `proposal_only`.

**Trois pièges nommés.** Le chronomètre `warm_e2e` inclut le **nouveau**
transfert du nuage et la sortie matérialisée en mémoire hôte épinglée, GPU
synchronisé ; toute ambiguïté `binary64` reste `fail_open` et non un rejet
silencieux ; et la porte structurelle — zéro allocation indexée par les univers
de facettes ou cofaces, zéro `Γ` — se vérifie **avant** le premier chronomètre.

**Porte de sortie.** Trente nuages frais, deux familles, p50/p95/max/MAD **et
chaque valeur brute** publiés. Le secondaire `1 s` d'abord ; le principal
`100 ms` seulement si J0 l'a montré arithmétiquement atteignable.

## 8. J6 — les dizaines de millions, et ce qui change vraiment

À `30 000 000` points, deux choses cassent, et aucune n'est une question de
vitesse.

**La quantification.** Le profil `u16` donne une grille `65 536^3`. À trente
millions de points sur un nuage LiDAR aérien, l'espacement quantifié devient
comparable au bruit capteur : les coplanarités et les positions dupliquées
cessent d'être des cas limites et deviennent le régime dominant. La politique de
dégénérescence — `RLE` par `BallKey`, plateau déclaré, ou
`unsupported_degeneracy` — n'est plus un garde-fou, c'est le chemin principal.
Le fate typé du census est donc une brique de J6 autant que du présent ; son API
reste à recevoir sur les identités invalides.

**La matérialisation.** À `428` supports par point, trente millions de points
donnent `1,3·10^10` supports. Aucune mémoire ne les tient. Deux issues, et il
faut choisir explicitement : soit **abaisser `K_max`** — la masse décroît comme
`somme_{j<=k}(j+1)(j+2)`, donc passer de `K=10` à `K=4` divise par environ
douze — soit **streamer par tuiles** avec recollement certifié aux bords, ce qui
exige une preuve que les supports traversant une frontière de tuile sont tous
capturés. Le rayon certifié par calottes donne cette preuve : il borne le
diamètre de tout support contenant une ancre, donc la largeur de recouvrement
nécessaire. C'est la seule voie de tuilage qui ne soit pas une heuristique.

**Porte de sortie.** Une tuile isolée produit exactement les mêmes supports que
la chaîne globale restreinte à son noyau, sur une fixture où un support traverse
la frontière.

## 9. Décisions déjà fixées pour cette tranche

Décidé et sans ambiguïté : aucune Delaunay ; trois lanes indépendantes ; pas de
produit `carrier x apex` ; `RelevantGP` rend `unsupported_degeneracy` plutôt
qu'un cardinal.

Les instructions courantes ferment les trois choix immédiats. La tranche vise
d'abord le seuil secondaire `p95 warm_e2e < 1 s` demandé par l'utilisateur,
sans rétracter ni renégocier silencieusement la cible produit primaire de
`100 ms`. Le profil v3 reste `quantized_u16_input_only` ; `binary64` ouvrirait
un profil séparé. Enfin, toute future session emploie la zone IA et le runner
gardé seulement après fermeture des portes CPU et du contre-audit du runner.
J0 peut motiver une demande ultérieure de changement de contrat, jamais la
décider seul.

GCP non utilisé pour l'écriture de cette note.

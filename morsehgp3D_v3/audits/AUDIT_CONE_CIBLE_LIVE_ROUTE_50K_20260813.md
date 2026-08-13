# Audit live du cône cible — certificat reçu localement, ordonnance endpoint refusée

Date : 13 août 2026 UTC.

Cadre : phase=exploration_v3_hors_registre,
backend=cpu_reference_bounded_oracles_and_g4_diagnostic,
profile=quantized_u16_input_only,
mode=audit_independant_math_and_architecture,
public_status=not_claimed.

Ce document répond au prototype ajouté après
[la note de Claude sur le mur amas et le census](NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md)
et prolonge
[les réponses mathématiques initiales](AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md).
Il distingue le classifieur conique, qui est sain sur son domaine, de son
ordonnance actuelle, qui échoue la porte de travail. Aucun fichier
d'implémentation n'a été modifié par l'audit.

## 0. Pin, portée et tests

Le worktree est vivant : le HEAD reste
2a205f3508abc7a20ea564eef55ed8e1f0f6f67d, mais le CMake et les deux sources
du cône sont des deltas non committés. Les résultats ci-dessous ne valent que
pour les octets suivants :

| objet | SHA-256 |
| --- | --- |
| CMakeLists.txt | 4f4733bccf37828f735ca473b4a063947eeafaf646c6bf4c63daeea6bd4ebc44 |
| prototype/spindle_cone.hpp | 78037fc19d0f2dae63b28745ee8741e10bd7821a8da3278032ad2dae76db0a85 |
| prototype/spindle_cone_probe.cpp | bf64663298d16d2035eaf3b274ec3bd7214ce74cc701fdfe1c606636815010a3 |
| ELF Release mhgp3v_spindle_cone_probe | abbc57c5a430e06c63b94631a584b37df83b1dd33280426c5199bfa3d2d5faef |

La configuration Release contient 603 CTests. Un rejeu encadré par les mêmes
quatre hashes donne :

~~~text
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_cone_' -j4
30/30, 8,57 s
~~~

Un second auditeur obtient 56/56 portes mhgp3v_anchor_ en 151,65 s. Cette
durée a subi de la contention et n'est qu'un résultat fonctionnel. CUDA est
désactivé dans le build local et nvcc est absent. Aucun test device, aucun
BenchmarkOutputContract-v1 et aucune mesure G4 ne sont donc reçus. GCP non
utilisé.

## Verdict

| objet | verdict |
| --- | --- |
| identités ponctuelles q2/q3/q4 | admises sur le domaine u16 et smax valide |
| inclusion ALL d'une boîte cible à endpoint et témoin fixes | exacte par huit coins |
| rejet NONE par Hmax et Rlb | suffisant, fail-open et sûr |
| banque k-NN bornée | certificat sûr mais jamais complet |
| probe a × Z_a × B | oracle et falsificateur seulement |
| route 50 k | NO-GO : toutes les séries de travail ont plusieurs pentes supérieures à 1,35 |
| intégration sous cap | non reçue : le résiduel n'est pas matérialisé |
| chemin CUDA/G4 | bloqué statiquement avant même la performance |
| contrat 50 k / une seconde | entièrement ouvert |

Claude a bien fermé deux défauts observés pendant le live : la regex CMake
2[0-9]{4}, non supportée, a été remplacée par des planchers à code ; la
sentinelle d'accord -1 n'est plus comparée à un plancher nul. Le vert 30/30
reçoit ces réparations. Il ne change pas le verdict de coût ni les défauts
reproductibles ci-dessous.

## 1. Réponse mathématique au prototype de Claude

### 1.1 Le cône ponctuel est correct

Pour un endpoint a, un témoin z et une cible b, poser
e=z-a, t=b-z, H=t·e et R=||t×e||². Les écritures du spindle donnent
g=4H et Q=4R. Les trois lanes sont donc :

$$\mathrm{q2}: H>0,\qquad \mathrm{q3}: H>0\ \text{et}\ 3H^2>R,\qquad \mathrm{q4}: H>0\ \text{et}\ 2H^2>R.$$

Avec E2=||e||², X2=||t||² et R=E2·X2-H², les comparaisons q3/q4 deviennent
respectivement 4H²>E2·X2 et 3H²>E2·X2. Le code forme H, E2 et X2 en i64,
puis promeut les produits en i128. Ces largeurs suffisent sur le profil u16.
Les égalités restent hors des cônes ouverts.

À a et z fixes, chaque domaine de b est convexe. Une AABB fermée est
l'enveloppe convexe de ses huit coins : ALL par huit coins est donc une
équivalence, pas seulement une condition suffisante. Le rejet NONE calcule
Hmax exactement et minore chaque composante de t×e par la distance de zéro à
son intervalle. La somme Rlb ainsi obtenue reste un minorant malgré les
corrélations ; elle peut perdre un rejet, jamais en inventer un.

Cette réception est locale. Elle ne prouve ni qu'une banque finie trouve tous
les témoins, ni que la boucle par endpoint est parcimonieuse.

### 1.2 Propriété collective utile : 512 triples de coins donnent un ALL exact

Le lift produit ne doit pas répéter le probe endpoint par endpoint. Pour trois
boîtes A, B et C, considérer le prédicat Pq(a,b,z) ci-dessus. Ses fibres sont
convexes séparément :

- à a et z fixes, les b admissibles forment le cône cible ;
- à b et z fixes, la symétrie des endpoints donne le même cône en a ;
- à a et b fixes, les z admissibles forment le spindle convexe.

Il en résulte le lemme suivant : A×B×C est entièrement admis dans une lane si
et seulement si les 8³=512 triples de coins sont admis strictement dans cette
lane. La preuve interpole successivement C, puis B, puis A ; à chaque étape,
les huit sommets nécessaires ont déjà été validés et la fibre convexe contient
leur enveloppe. L'ouverture ne pose pas de difficulté : une combinaison
convexe finie de points tous intérieurs reste intérieure.

Ce test est un oracle ou un fallback borné, pas une boucle à payer sur chaque
état. La broad phase moins chère reste :

$$H_{\min}>0\ \text{et}\ 3H_{\min}^2>R_{\max}\quad\text{en q3},\qquad H_{\min}>0\ \text{et}\ 2H_{\min}^2>R_{\max}\quad\text{en q4}.$$

Le minimum de H se calcule par trois minima de huit évaluations scalaires.
Le maximum de R est atteint sur les 512 triples par convexité séparée.
Cette comparaison découplée est sûre mais incomplète, car ses extrema peuvent
venir de triples différents. Son échec signifie UNKNOWN, jamais NONE. Sur un
UNKNOWN encore rentable, le test direct des 512 prédicats de coins est plus
serré et exact pour ALL ; son échec signifie encore seulement « pas ALL ».

### 1.3 Portée exacte d'une banque

Une banque de PointId distincts est un certificat unilatéral : huit témoins q4
ou neuf témoins q3 ferment la lane correspondante à smax=11. Un prune simultané
des trois lanes demande aussi dix témoins q2. Un M fixe n'est jamais une preuve
de complétude ; toute masse non fermée doit poursuivre un chemin exact.

Les deux banques des endpoints peuvent être utilisées ensemble après
déduplication des PointId. Elles améliorent l'ordre de visite, mais elles ne
doivent ni définir une orientation par PointId, ni additionner deux fois le
même témoin. Les positions colocalisées ne sont pas « interchangeables » pour
un compte de multiplicité : tant que la politique duplicate_policy=aggregate
n'est pas reçue, le préflight doit refuser les coordonnées dupliquées.

## 2. P0 de contrat et d'exactitude avant intégration

### 2.1 smax hors domaine fabrique une fermeture commune au sujet et au juge

Le probe ne vérifie que smax>=4, stocke la valeur en long long, puis la caste en
int pour les seuils. La commande suivante retourne zéro :

~~~text
mhgp3v_spindle_cone_probe --points=20 \
  --smax=9223372036854775807 --verify --permute
~~~

Elle publie 380/380 paires fermées, zéro test témoin et accord=OUI dans le
juge comme dans la permutation. Le cast produit des seuils négatifs partagés
par les deux chemins. Avec smax=2147483648, UBSan observe en plus un overflow
signé dans lane_death_threshold.

Réparation demandée à Claude : vérifier errno après strtoll, borner smax avant
tout cast et unifier le domaine. L'autorité d'enveloppe actuelle autorise
4<=smax<=34 ; si le device conserve volontairement 4<=smax<=24, les trois CLI
doivent annoncer et exercer ce domaine plus étroit. Ajouter les refus exacts
LLONG_MAX, INT_MAX+1, 3, borne+1 et suffixe.

### 2.2 La cardinalité demandée n'est pas garantie

~~~text
mhgp3v_spindle_cone_probe --points=100 --coord=2 --verify --permute
~~~

retourne zéro mais le reçu dit n=8. Toutes les identités sont alors vérifiées
sur le mauvais univers. Immédiatement après make_family_cloud, exiger
pts.size()==opt.n et refuser en code 2 avant le LBVH. La porte doit comparer
cardinalité demandée, cardinalité produite et hash du nuage.

### 2.3 Le juge n'est pas indépendant et ne reçoit pas les lanes séparées

Le sujet et le juge appellent tous deux anchor::lane_death_threshold et les
trois prédicats résident dans le même header. Le faux vert smax démontre ce
mode commun. De plus, ClosedBits n'enregistre une paire que lorsque q2, q3 et
q4 sont toutes mortes ; le juge fait le même ET. Une fermeture q3 seule ou q4
seule peut donc être fausse sans être observée, alors que l'intégration
consommera ces lanes séparément.

La porte requise possède trois bitsets ou trois digests, un par lane. L'oracle
redérive seuils et prédicats dans une unité indépendante, idéalement avec
entiers multiprécision et sans inclure spindle_cone.hpp ni anchor_envelope.hpp.
Il compare les faux positifs q2/q3/q4 séparément, avec plancher non nul pour
chaque lane et fixtures q3-sans-q2, q4-sans-q3 et égalités.

### 2.4 Le résiduel annoncé n'est pas un flux consommable

Au cap, le probe incrémente unknown_to_residual et residual_block_mass mais
n'émet ni identifiant de nœud, ni AABB, ni masques, ni reçu de crédits. Par
exemple :

~~~text
mhgp3v_spindle_cone_probe --points=100 --max-depth=0 --bank=48
unknown_to_residual=100 residual_block_mass=9900 candidate_pairs=0
~~~

Le code zéro ne représente donc ni la liste terminale ni un ensemble de blocs
reprenable. pairid_before_terminal et bank_restarts sont initialisés, fusionnés
et vérifiés, mais jamais incrémentés : leur zéro est structurellement vacueux.
L'option leaf-pair-tests additionne un coût potentiel sans exécuter de test.

Pour le diagnostic, matérialiser un record résiduel
(A_node,B_node,lane_mask,credit_receipt,epoch) et le faire consommer par une
porte qui reconstruit exactement le ledger. Pour le produit, aucun cap
configurable n'est admis : la queue est épuisée, segmentée sans perte, ou
l'insuffisance physique refuse atomiquement l'objet complet.

### 2.5 Le chemin CUDA anchor ne compile pas avec la signature live

run_anchor_point exige maintenant les deux booléens theta_audit et
density_guard. Les appels de anchor_source_kernel.cu et de
anchor_source_device_qualification.cpp omettent density_guard ; run_device et
anchor_kernel ne le transportent pas non plus. nvcc étant absent localement,
le constat est statique, mais la discordance de signature est directe.

Claude doit soit retirer la garde de densité du chemin produit comme proposé
par l'ablation, soit propager explicitement la valeur dans toute l'ABI
hôte/device et ses portes. Une cible CUDA opt-in doit compiler avant toute
session G4. Ce correctif de compilation ne justifie pas le port du probe
endpoint, dont l'algorithme est déjà refusé.

### 2.6 Les planchers anchor restent contournables par PASS_REGULAR_EXPRESSION

CTest considère un test réussi dès que PASS_REGULAR_EXPRESSION est rencontré,
même si le processus retourne ensuite un code non nul. anchor_source imprime
verify ... accord=OUI avant les planchers supports, anchors et prunes. Une
reproduction locale retourne code 3 tout en imprimant le motif vert :

~~~text
mhgp3v_anchor_source --points=90 --seed=1 --verify \
  --min-supports=100000000
verify exhaustif=14769 produit=14769 accord=OUI
REFUS : plancher de supports 100000000 > 14769
~~~

Les groupes anchor_uniform, anchor_pipeline, eight_clusters, smax, budget
précoce et garde de densité emploient encore ce schéma. La ligne theta est
également imprimée avant min-theta-active, et la parité des moteurs avant les
derniers planchers.

Réparation : retirer PASS_REGULAR_EXPRESSION de toute porte à planchers et
faire foi du code, ou n'imprimer qu'un unique marqueur terminal après toutes
les validations. Les expected-code tests d'un autre cas ne rendent pas le
plancher positif de la porte principale opérant.

## 3. Portes et inefficacités encore ouvertes

### 3.1 La permutation ne compare pas le flux PairId réellement émis

Le probe compare les fermetures ordonnées, mais candidate_pairs est émis
seulement lorsque id>a : la banque utilisée pour la paire non ordonnée est donc
celle de l'endpoint au plus petit PointId. Les banques des deux endpoints
diffèrent. Après renumérotation, l'orientation choisie peut s'inverser et le
flux candidat changer alors que les fermetures ordonnées restent équivariantes.

La mesure symétrique quantifie l'écart :

| série | n=500 : défaut → deux directions | n=1000 : défaut → deux directions |
| --- | ---: | ---: |
| uniform, banque 48 | 104 289 → 91 449 | 380 939 → 314 206 |
| uniform, banque 96 | 84 120 → 70 651 | 256 619 → 197 996 |
| amas, banque 48 | 85 047 → 59 816 | 311 008 → 213 579 |
| amas, banque 96 | 75 944 → 52 943 | 244 083 → 147 452 |

Le gain de 12 à 40 % est réel, mais le bitset n² de --symmetric n'est pas une
route produit. La gate doit comparer sous permutation l'ensemble canonique des
PairId survivants effectivement remis au consumer. Le self-join collectif
A×B doit être triangulaire et géométriquement canonique, avec union
dédupliquée des deux banques comme simple priorité de témoins.

### 3.2 Crédits hérités, NONE et compteurs

Le mutant cone-ignore-inherited est implémenté mais absent du CMake. La commande
uniform n=300, banque 48, verify le tue en code 4 avec neuf désaccords observés.
Il doit devenir une porte permanente non vacue.

Dans run_endpoint, nq3/nq4 sont consultés selon want avant le calcul de floor.
Quand une lane inférieure a déjà atteint son seuil global, floor peut valoir
q3 ou q4 alors que want vaut encore q2 ; une réfutation héritée de la lane
utile est ignorée et le classifieur est repayé dans les descendants. Calculer
floor d'abord, puis tester nq4 si floor>=q4 et nq3 si floor>=q3.

witness_none_q3/q4 est incrémenté même si mask_set ne change aucun bit. Un gros
plancher peut donc compter plusieurs fois la même réfutation. Publier au moins
transitions nouvelles, hits hérités et tests évités séparément.

### 3.3 Portes arithmétiques et de génération

- Le selftest revendique les réflexions mais n'exerce que six permutations
  d'axes et des translations. Ajouter les huit choix de signes compatibles
  avec le domaine, ou réduire le claim.
- Aucun CTest ne compare la banque k-NN à un top-M exhaustif ordonné par
  (distance², clé géométrique) sur petit n, égalités comprises.
- Le message de refus annonce banque [1,64] alors que kMaxBank vaut 256 et
  qu'une porte emploie 96.
- Le générateur pseudo-aléatoire emploie un overflow signé dans son LCG. UBSan
  le signale ; employer uint64_t.
- Le mutant narrow-i64 dépend lui aussi d'un overflow signé indéfini. Son wrap
  doit être défini explicitement, ou la porte doit attendre une détection
  d'overflow plutôt qu'une valeur issue d'UB.
- Ajouter une cible UBSan qui exerce fixtures, selftest, limites smax et
  parseur. Le vert Release courant ne reçoit pas l'absence d'UB.

## 4. Rampe de travail : NO-GO de a × Z_a × B

Les mesures suivantes utilisent exactement l'ELF abbc57c5..., leaf=8,
seed=3, sans bloc résiduel. Le wall est publié pour information ; il exclut la
construction du LBVH et du tableau de rangs, et n'est donc ni end-to-end ni
comparable au contrat G4.

| famille / banque | n | visites cible | tests témoin-nœud | candidats | wall CPU |
| --- | ---: | ---: | ---: | ---: | ---: |
| uniform / 48 | 500 | 84 842 | 2 580 724 | 104 289 | 0,164 s |
| uniform / 48 | 1 000 | 328 844 | 9 118 007 | 380 939 | 0,551 s |
| uniform / 48 | 2 000 | 1 177 056 | 29 143 814 | 1 369 645 | 2,116 s |
| uniform / 48 | 4 000 | 4 341 114 | 99 552 271 | 4 920 845 | 7,501 s |
| uniform / 96 | 500 | 79 100 | 4 427 268 | 84 120 | 0,248 s |
| uniform / 96 | 1 000 | 277 202 | 14 156 152 | 256 619 | 1,122 s |
| uniform / 96 | 2 000 | 840 004 | 39 207 462 | 736 638 | 2,596 s |
| uniform / 96 | 4 000 | 2 530 816 | 112 781 793 | 1 969 116 | 7,619 s |
| amas / 48 | 500 | 76 420 | 1 820 761 | 85 047 | 0,301 s |
| amas / 48 | 1 000 | 280 268 | 6 505 165 | 311 008 | 0,909 s |
| amas / 48 | 2 000 | 1 032 290 | 20 641 781 | 1 069 943 | 2,196 s |
| amas / 48 | 4 000 | 3 618 768 | 69 819 004 | 3 900 545 | 4,389 s |
| amas / 96 | 500 | 72 752 | 3 385 983 | 75 944 | 0,179 s |
| amas / 96 | 1 000 | 227 280 | 9 269 896 | 244 083 | 0,519 s |
| amas / 96 | 2 000 | 703 982 | 25 585 403 | 709 416 | 1,504 s |
| amas / 96 | 4 000 | 2 134 060 | 78 920 963 | 2 153 592 | 4,879 s |

Pentes successives log2 entre 500, 1 000, 2 000 et 4 000 :

| série | visites cible | tests témoin-nœud | candidats |
| --- | --- | --- | --- |
| uniform / 48 | 1,955 / 1,840 / 1,883 | 1,821 / 1,676 / 1,772 | 1,869 / 1,846 / 1,845 |
| uniform / 96 | 1,809 / 1,599 / 1,591 | 1,677 / 1,470 / 1,524 | 1,609 / 1,521 / 1,419 |
| amas / 48 | 1,875 / 1,881 / 1,810 | 1,837 / 1,666 / 1,758 | 1,871 / 1,783 / 1,866 |
| amas / 96 | 1,643 / 1,631 / 1,600 | 1,453 / 1,465 / 1,625 | 1,684 / 1,539 / 1,602 |

Chaque série dépasse 1,35 sur chaque doublement pour le flux candidat et les
visites. La règle de la section 14.4 du plan de tests suspend donc les
micro-optimisations et impose une revue d'algorithme. Ce n'est pas une preuve
asymptotique ; c'est précisément le NO-GO empirique prévu par le contrat.

À banque 96, prolonger seulement la dernière pente depuis n=4 000 donne
environ 4,8 à 5,3 milliards de tests témoin-nœud et 71 à 123 millions de
candidats à 50 k. Cette extrapolation est indicative, jamais un benchmark.
Elle suffit à montrer qu'un port CUDA littéral optimiserait une ordonnance
déjà refusée.

## 5. Route d'implémentation proposée à Claude

### Étape A — geler le probe ponctuel comme oracle

Conserver le header conique, ses fixtures et la banque endpoint pour
prioriser les témoins et falsifier un successeur. Fermer auparavant smax,
cardinalité, oracle par lane, UBSan, banque exhaustive, réflexions et mutant
d'héritage. Ne qualifier aucun temps wall_s du probe comme e2e.

### Étape B — front collectif persistant A×B×C

L'unité produit devient une tâche canonique
(A_node,B_node,C_frontier,lane_mask,credits,epoch,receipt) :

1. A×B est un self-join implicite, triangulaire sur A=B, sans matrice de
   PairId.
2. C_frontier est une antichaîne de nœuds témoins disjoints. Un nœud C classé
   ALL crédite sa masse de PointId distincts à toutes les paires du bloc.
3. Tout C qui chevauche A ou B se scinde jusqu'à exclure z=a,b ; aucun endpoint
   n'est compté comme témoin.
4. q4 crédite aussi q3 et q2, q3 crédite aussi q2, mais chaque plage C possède
   un reçu par lane pour interdire le double crédit.
5. Scinder A ou B partitionne la masse de paires et hérite les reçus C.
   Scinder C affine seulement la recherche ; il ne recrédite jamais la masse
   de paires et ne repart jamais de la racine.
6. La broad phase essaie Hmin/Rmax et des intervalles peu coûteux. Le fallback
   ALL exact teste au plus les 512 triples de coins. Un échec reste UNKNOWN.
7. Les banques des endpoints ordonnent C_frontier ; elles ne fondent ni la
   complétude ni l'orientation.

Le choix de split doit viser le resserrement de la borne par octet de
frontière, avec priorité à C pour exclure les overlaps, puis à A/B pour
partitionner une grande masse encore ambiguë. Les tâches fermées annulent
leurs descendants tardifs exactement une fois.

La route q2 Yao/affine/dual reste séparée. Le cône q3/q4 ne doit pas attendre
une fermeture q2 pour éviter la génération de supports q3/q4. Les trois
ledgers se rejoignent seulement au consumer de supports.

### Étape C — résiduel puis cutting, jamais retour aux boucles historiques

Les blocs non fermés passent comme blocs au front suivant. Sur eux seulement,
la cutting signée ou les niveaux mono-ancre P-P/N-N/P-N doivent supprimer les
deux facteurs déjà mesurés :

- aucune génération littérale des C(n_lens,2) centres q4 ;
- aucun census supports×kept ;
- transport de always_inside et de conflict_list jusqu'au census local ;
- premier RLE par SupportKey avant lift, puis résolution BallKey et fold
  streamé.

Un fallback exact borné peut énumérer pour le juge. Le chemin produit ne
réimplémente jamais l'oracle exhaustif et ne matérialise aucune mosaïque de
Delaunay d'ordre supérieur, cellule globale, coface ou incidence.

### Étape D — gates avant CUDA et G4

Le front collectif publie au minimum :

- masse d'entrée, fermée et résiduelle par lane ;
- plages C créditées, doublons, overlaps endpoint et digests ;
- appels H/R, triples de coins, ALL4, ALL3-only et UNKNOWN ;
- splits A/B/C, pushes, pops, copies, annulations et retours racine ;
- PointId touchés seulement aux terminaux et blocs résiduels réellement
  consommés ;
- q4 centers produits, somme des conflits du census et tests intérieurs ;
- octets/HWM de chaque queue, antichaîne, workspace et payload.

Les portes petit n comparent chaque lane et chaque bloc à l'énumération
indépendante. Les rampes uniform, eight_clusters, terrain et multiecho doivent
obtenir deux pentes au plus 1,35 avant le port device. Ensuite seulement :

- build CUDA opt-in vert et parité bit-à-bit CPU/device ;
- n=32 sous Compute Sanitizer ;
- arènes persistantes, SoA et scheduler de tâches coopératives, pas un thread
  scalaire par endpoint ;
- count→scan→fill et RLE sur device, aucun prépass CPU de dimensionnement ;
- smoke direct 12 500/25 000/50 000, puis trente répétitions chaudes du payload
  officiel si les compteurs restent verts.

Le kernel anchor courant réserve 222 208 octets par slot, soit environ
3,39 Gio pour 16 384 slots avant arbre, sorties et workspaces, et ne
chronomètre que le kernel. Cette baseline doit rester un différentiel ; elle
ne préfigure ni le layout ni le chronomètre warm_e2e du producteur collectif.

## 6. Ordre de priorité

1. Corriger les P0 smax, cardinalité, oracle par lane, résiduel et ABI CUDA.
2. Retirer les faux verts PASS_REGULAR_EXPRESSION des portes anchor.
3. Graver UBSan, top-M exhaustif, réflexions et héritage.
4. Déclarer explicitement a×Z_a×B NO-GO produit et le conserver comme oracle.
5. Implémenter et mesurer A×B×C sur CPU avec ledgers non vacueux.
6. Fermer ensuite la cutting et le census local.
7. Ne lancer G4 qu'après build CUDA et gate de travail verte.

Le contrat secondaire p95 warm_e2e<1 s à 50 000 points reste ouvert. Même un
kernel conique sous une seconde ne le fermerait pas : validation, H2D, index,
source exacte, dix forêts, verticales, lots, certificat minimal et D2H doivent
être compris dans le même chronomètre.

GCP non utilisé.

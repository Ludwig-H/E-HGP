# Audit mathématique courant — supports, hiérarchies et clustering plat

6 octobre 2026. Relecture du contrat S0 et de l'oracle S1 au commit
**5adf6a59f**, puis de la demande du développeur **9290cf3bf** et des WIP
S3/S5/S6 et de S7, publiées jusqu'à **966a351be**, puis des primitives
numériques S8 en **53c027fe8**. Le socle FULL et les travaux sur les points/tête restent épinglés
aux reçus liés ci-dessous. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Note maintenue en place ; détails et échanges clos dans les reçus.

## Feuille coherente : reponse a la section Q du 6 octobre

**Avis favorable au principe**, avec un parcours de préfixes commun au warp
et une réduction ordonnée des résultats. Pin examiné : **ee3eabe5e**.
[Preuves, helpers et traces discriminantes](../receipts/audit_coherent_leaf_design_20261006/README.md).

**Q1 — Le rejet arrive au (θ+1)-ième intérieur, avec θ=K+1−q.**
Le site qui fait dépasser le seuil est compté dans `census_tests`, puis
le préfixe est rejeté. Atteindre seulement θ intérieurs ne termine pas
la boucle. Respecter d'abord la priorité locale : site du générateur
(contact), masque intérieur, masque extérieur, puis `side` certifié.
Un refus spéculatif sur un site déjà protégé n'est pas un refus atteint.

Après ces gardes, noter I le masque des intérieurs connus et U celui des
sites non résolus, en excluant les lanes i≥m. Poser `t=select(θ+1,I)` et
`u=first(U)`, ou m en l'absence de bit correspondant. Le premier arrêt est
`e=min(t,u,m)` :

| Cas | Décision | `census_tests` ajouté |
| --- | --- | --- |
| u<t | Feuille non résolue | u+1 |
| t<u | Préfixe rejeté | t+1 |
| t=u=m | Census complet ; poursuivre | m |

On ignore donc les refus **après** l'arrêt, jamais ceux qui le précèdent.
`judged` vaut **1 dès l'entrée**, sans dépendre du premier succès. Le conflit
`inside & outside` reste un refus préalable, avec zéro test de site ajouté.
Après census complet, compacter intérieur et coquille par popcount des bits
de rang inférieur, pour conserver l'ordre des sites. Les émissions et
incidences attendent support canonique, identité au générateur et admission.

**Q2 — Choisir le premier événement, succès ou refus, dans l'ordre du code.**
L'ordre est : toutes les paires, puis tous les triplets, puis tous les
quadruplets ; ordre lexicographique dans chaque phase. Un succès n'efface
pas un refus antérieur. Un refus après le succès est ignoré. Des tranches
consécutives de 32 candidats, arrêtées à la première contenant un événement,
permettent de conserver cet ordre sans parcourir nécessairement toute la
coquille.

Ne pas réutiliser le rang combinatoire du cache J2 comme priorité canonique :
`(0,1,4)` précède `(0,2,3)` dans les boucles, mais leurs rangs J2 valent4 et2.
Conserver aussi les gardes internes : triangle non aigu écarté avant son
orientation ; faces du tétraèdre dans leur ordre, avec arrêt dès dégénérescence
ou signe incompatible. Un OR de tous les refus spéculatifs serait faux.
Il n'existe actuellement aucun compteur propre à la boucle canonique.

**J2 est une étape scalaire avec effets.** La géométrie pure peut être
recalculée par toutes les lanes ; le test-set du cache et les compteurs
logiques sont exécutés **une seule fois**, puis diffusés. Si les 32 lanes
consultent le bit, la première visite logique peut devenir un hit pour la
lane0. Même règle pour `prefixes`, `judged`, q4 et émissions : ne pas
additionner 32 copies du même événement.

**Q3 — Mutants utiles et limite d'atteignabilité.** Tester le seuil décalé,
le dernier succès choisi, l'ordre canonique lex remplacé par le rang J2,
le bit31 omis, les listes dans l'ordre d'arrivée et le cache consulté par
toutes les lanes. Les refus avant/après arrêt se testent aussi dans un petit
helper de contrôle à statuts injectés. **Ne pas exiger un nuage impossible :**
dans le moteur actuel, le certificat `side` est global au centre et G3 borne
les intérieurs déjà certifiés par les masques à θ. Obtenir θ+1 intérieurs
requiert donc un `side` certifié, qui certifie tous les autres ; un refus
`side` réel après ce dépassement est inaccessible avec ces invariants.
Les traces injectées prouvent le contrôle, pas une détection native causale.

Toutes les opérations spéculatives restent certifiées **avant** leur
arithmétique i128 ; masquer un résultat ne répare aucun débordement déjà
exécuté. Garder les 32 lanes aux votes, les lanes sans site neutres et les
barrières mémoire nécessaires aux listes partagées. Le
[complément CUDA](../receipts/audit_coherent_leaf_design_20261006/cuda_contract/README.md)
détaille ces gardes. Modèles normal/−O conformes ; aucun nouveau noyau
cohérent, test natif ou gain qualifié par cette réponse.

## Noyau compact et deux témoins de qualification — suivi du 6 octobre

Le WIP CUDA relu vers 08:30 conserve le raccord démontré ci-dessous : paire
u16 injective, suffixe `next[i]` après j, masque `dom[i]`, préfixes dans
l'ordre des paires. La représentation partagée est compacte ; l'ancienne
alerte de 22 Kio concernait la copie directe des structures hôtes, pas ce
nouveau layout. Le point de synchronisation restant est dans la
[note moteur](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#premier-noyau-coopératif--corrections-avant-g4).

**La porte du masque doit avoir une dominance réelle.** Toute boîte
contenant les deux sites i,j contient deux points où leur différence de
distances a des signes opposés : aucun des deux ne domine strictement
l'autre sur toute cette boîte. Les fixtures actuelles contiennent tous
leurs sites, donc ne discriminent pas `masks[1]=0`.

Le témoin fourni, en ordre Morton, est
`[(0,3,0),(1,3,0),(3,1,4),(3,0,6),(5,6,6)]`, boîte `[2,5)^3`, K3.
Il donne `dom=[2,0,0,4,0]`. Sous paire `(0,3)`, le masque correct de poids2
coupe la descente ; le mutant de poids1 ajoute le test J2 `(0,3,4)`.
`region_line_tests` passe de **3 à 4**, tandis que les préfixes restent17.
Le patch proposé ajoute ce témoin et exige ses masques dans `sizes`.

**L'attente de refus de `near_max` est impossible en u18 sur ces fixtures.**
Puissance et poids q4 restent certifiés ; les 21 triangles strictement aigus
des trois fixtures ont tous une coquille de taille3, sans appel d'orientation
q3 étendu susceptible de refuser. Réserver `partial>0` à u21/u24, conserver
la comparaison complète et exiger zéro refus en u18.
[Calculs rationnels et 40 contrôles bornés normal/−O](../receipts/audit_coop_wip_followup_20261006/mathematics/REPORT.md).
Aucun CTest ni diagnostic CUDA exécuté par les auditeurs.

## Feuille cooperative : reponse a la section O du 6 octobre

**Réponse initiale : avis favorable au découpage proposé**, sous les invariants ci-dessous.
Réponse à la conception publiée en **3b76a3fcf** ; la relecture du WIP hôte
de `v11-impl-l3` n'est pas une qualification du futur noyau CUDA.
[Preuves bornées, témoin exact et plan de qualification](../receipts/audit_leaf_cooperative_20261006/README.md).

**Q1 — Permuter les sous-arbres de paires conserve les compteurs d'une
feuille résolue.** Garder dans chaque sous-arbre l'ordre des faces J2 et
l'arrêt au premier rejet, l'ordre des sites du census et son arrêt au seuil,
puis la recherche ordonnée du support canonique. Chaque tâche reprend
`prefix[0]=i`, `masks[1]=dom[i]`, le suffixe de `next(i)` **après retrait de j**
et `next_logical(i)`. Le WIP `run_pair`/`depth0_pairs` lu applique ce raccord.
Préfixes, masques, listes intérieur/coquille, puits et compteurs de travail
sont privés par fil. La profondeur zéro et les préfixes coupés y sont comptés
une seule fois.

Le cache appareil actuel recalcule `center_line_meets` même sur un hit :
son bit ne transporte aucune décision. Avec `atomicOr` et un cache initialisé
une seule fois par passe, `evaluations` compte les rangs distincts réellement
interrogés et `hits=tests−evaluations`. Le premier fil gagnant change, la somme
reste identique. Cache désactivé : `evaluations=tests`, `hits=0`. Un futur cache
qui publierait un résultat géométrique demanderait un autre protocole.
Au comptage, `unresolved` annule **toute** la feuille, émissions et compteurs
compris, avant le repli CPU ; aucune égalité des comptes partiels n'est
attendue. Un refus inattendu au remplissage d'une feuille comptée résolue
fait refuser le lot avant matérialisation, comme dans l'exécuteur actuel.

**Q2 — L'ordre lexicographique des paires, puis le DFS interne de chaque
paire, est l'ordre complet des émissions.** La profondeur un site n'émet
rien ; le sous-arbre `(i,j)` émet d'abord q2 s'il est admis, puis tous ses
descendants q3/q4. Les deux préfixes exclusifs, boules et populations,
doivent être calculés dans cet ordre, même si count et fill prennent les
tâches dans deux ordres différents. Le tri final du catalogue ne dispense
pas de vérifier les records et leurs populations avant tri.
Attention : une q2/q3 sans émission ne coupe pas nécessairement ses
descendants ; une face q3 obtuse peut appartenir à un q4 valide.
Le [témoin exact fourni](../receipts/audit_leaf_cooperative_20261006/mathematics/README.md)
a quatre sites, K3, un premier triangle obtus et une boule q4 de rayon carré
25 ; ses quatre poids rationnels sont strictement positifs.

**Q3 — L'émulation valide la décomposition ; G4 valide la concurrence.**
Une émulation séquentielle, même permutée, ne peut révéler un `atomicOr`
manquant ni un retour divergent avant barrière. Les fixtures et mutants
utiles sont dans la [note moteur](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#feuille-cooperative--qualification-ciblee-sur-g4).
Toutes les lanes, y compris sans site ou sans paire, participent aux
barrières entre chargement, tables, liste de paires, count, préfixes et fill.
Le cache est réinitialisé entre count et fill après terminaison de tous
les fils ; les compteurs de fill sont jetés. Le statut final est commun et
contrôlé avant publication.

**Q4 — 496 paires et 155 mots de cache sont exacts ; 9 Kio exigent un
layout compact explicite.** `C(32,2)=496`, `C(32,3)=4960=155×32`.
Le WIP hôte utilise deux u32 et deux u64 par `PairTask` : 24 octets sous
l'ABI usuelle, soit **11 904 octets pour les tâches seules**. Avec les tables
et deux tableaux de comptes u64, on atteint environ **22 392 octets**, avant
les compteurs par lane et les métadonnées. Ce n'est pas un défaut du modèle
hôte ; ne pas recopier cette représentation en prétendant tenir dans 9 Kio.

Une possibilité : paire encodée dans un u32, `next/next_logical` stockés
par i, deux tableaux u32 de comptes remplacés en place par leurs préfixes.
Cela donne environ **9 016 octets avant métadonnées** avec les tables actuelles.
La borne existante `kCountBound=3 979 008<2^22` couvre les comptes et sommes
internes à une feuille ; offsets du lot et registre restent u64. Publier le
`sizeof` réel du layout et un `static_assert` dans la construction G4.
La mémoire privée par lane et les éventuels débordements de registres
restent distincts de cette borne de mémoire partagée.

## Forêt GPU proposée le 6 octobre : équivalence et information à garder

Le [complément exact au plan O7/O8](../receipts/audit_plan_gpu_20261006/mathematics/REPORT.md)
donne le lemme utilisable : un MST conserve les composantes à chaque
niveau ; contracter les fusions Kruskal de même rang rend les plateaux
atomiques, en gardant les naissances et la numérotation canonique.
Le triangle `(0,0), (3,0), (1,2)` démontre la limite : sa dernière arête
Gabriel disparaît du MST alors qu’elle porte une continuation et un
plateau comptés par le moteur. Conserver les cellules ou une métadonnée
suffisante pour reproduire ces comptes.

`ancestor_activations` et `ancestor_unions` restent reconstructibles depuis
les fusions basses et leurs arités jusqu’au dernier rang demandé ; seul
`ancestor_find_steps` dépend du parcours DSU. Ne pas remplacer silencieusement
ces définitions. Les 47 contrôles bornés passent normal/−O, avec six sources
Git épinglées ; cela prépare le port et ne le qualifie pas.

**Qualification finale au pin 38b76701b.** Les portes prioritaires S9/S10
passent dans les quatre profils ordinaires de
[fina2](../receipts/audit_g4_fina2_20261005/README.md). Les mutants head,
points et num sont tous détectés dans
[finm](../receipts/audit_g4_finm_20261005/README.md). La reprise
[R3](../receipts/audit_g4_repriser3_20261005/README.md) au pin corrigé
98a009550 termine les 485 mutants, dont les 23 API et les 28 CLI.
Sa base u18 et ses options locales u21/u24/poison restent distinctes
des six campagnes u21 sans résultat de l’ancien lot L. Aucun nouveau
défaut mathématique n’est déduit des anciens refus du juge API/CLI.

Les portes points/plat/racines d’échelle et LiDAR passent sous
ASan/UBSan u24 et TSan u21 dans
[fins](../receipts/audit_g4_fins_20261005/README.md). Le complément
[finb](../receipts/audit_g4_finb_20261005/README.md) termine 783/783 portes
courtes dans chaque profil, dont la racine exacte `2^127−1` et le refus
du tri S9. La reprise [R4](../receipts/audit_g4_repriser4_20261005/README.md)
au pin 98a009550 termine ensuite 80/80 portes ASan/UBSan u24, dont
les six routes corrigées et les portes points/plat/racines.
Les onze portes K10 et dix-sept références FULL exactes
passent dans [finl](../receipts/audit_g4_finl_20261005/README.md).

**Les huit différentiels dédiés passent en Release u21.**
[P9](../receipts/audit_g4_finp9_20261005/README.md) contrôle l’arbre de
points contre sa construction Python, site par site, avec dates,
plateaux et parents. [P10](../receipts/audit_g4_finp10_20261005/README.md)
contrôle ensuite la tête sur le même arbre natif : partitions, bruit,
étiquettes canoniques, EOM z=1/2/3 et feuilles. Chaque lot couvre ses
cas synthétiques et les trois trames entières. Les PASS CTest sont
lus ; les compteurs internes ne sont pas conservés. Les minima P9,
et les 1 944 appels sans refus Python de P10, sont des conséquences
des contrats du juge épinglé, explicitement distinguées dans les reçus.

## S10 native publiée : encadrement entier et frontière de capacité

La tranche **076d9142b** remplace le filtre flottant par
des encadrements entiers, puis conserve le repli exact. Les réciproques
z=1/2/3 relues suivent les identités déjà vérifiées ; les poids d'un
même plateau sont compensés avant leur expansion en radicaux. Le budget
de la tête est porté explicitement à 4 096 termes, avec refus de l'appel
entier à épuisement ; les comparaisons de dates gardent leur défaut de 16.

**Frontière numérique corrigée en 510dae50e.** Le contrat public
`LevelSource` borne la racine fixe à `u128`, mais la borne haute est
calculée en `i128`. Le niveau rationnel positif
`((2^127−1)/2^64)^2` donne une racine admissible dont l'ajout de 1
déborde. Ce témoin est un arbre abstrait, sans qualification ni
contre-exemple géométrique LiDAR déduits. La garde publiée `2^100`
contrôle les trois racines utiles avant conversion ; leurs sommes et
marges sont sûres. Les grandes valeurs passent au repli exact : avis
favorable sur la correction, dont les deux sources correspondent à la
capture relue. Le test `huge` exige désormais trois plateaux ouverts,
contre un avec l'ancien filtre ; sa portée devient causale pour le
mutant qui retire la garde. Constat source fermé ; la porte frontière
native passe en G4 dans fina2 puis sous les deux sanitizers de finb. La preuve initiale `R=2^127−1` reste distincte.
Cette frontière exacte rejoint la porte native en **38b76701b** :
racine vérifiée, groupes attendus et égalité certifiée aux rayons
`A/3,A/2,A`. Lecture favorable de ce complément.
[Correction et preuve de borne](../receipts/audit_s10_root_guard_20261005/README.md).
[Preuve, source et correction ciblée](../receipts/audit_s10_wip_20261005/math/README.md).

Les nouvelles portes F14 comparent les groupes de sites attendus à
`flat_sites`. Le différentiel `head_vs_python` compare ensuite toutes
les étiquettes de la tête sur l'arbre de points publié, aux z=1/2/3 et
en sélection feuilles. Ces portes relues sont à jouer ; elles ne
remplacent pas le différentiel S9 qui qualifie l'arbre donné à la tête.
[Plan complémentaire S10](../receipts/audit_s10_differential_plan_20261005/README.md).

## Réponse aux trois questions avant S10 — note développeur d26328fe2

**Refus EOM : oui, refuser l'appel entier.** Au premier
`radical_sign_budget`, propager le refus, sans arbitrage parent/enfants
ni sortie plate partielle. Une comparaison indécidable ne constitue pas
une égalité ; le parent reste choisi seulement sur égalité certifiée.
Les feuilles à zéro de K1 ne justifient pas un refus : `mcs≥2` les écarte
avant le calcul du score. Les dates effectivement inversées doivent être
positives ; la borne haute infinie garde sa contribution nulle.

**Réciproques et z=2/3 : réutiliser les témoins existants.** Le
[reçu exact](../receipts/eom_exact_audit_20261004/README.md) contrôle déjà
292 dates positives, dont 13 cas `Δ=0`, les identités de la réciproque et
de son cube, et le facteur 2 du terme mixte. Pour le port de z=2, ajouter
l'identité `e²·(1/e)²=1` sur ces mêmes cas, y compris une date à trois
racines et le cas singulier positif. Les classes du carré sont
`{1,tM,tq,Mq}`, celles du cube `{t,M,q,tMq}` : au plus quatre **par date**,
pas par score entier. Garder F4b pour distinguer z=1/z=2, ainsi que F4_z3 ;
une égalité à z=1 ne prouve pas une égalité à z=2 ou z=3.

**Oracle : aucun troisième oracle complet demandé.** La même session
d'écriture n'annule pas l'indépendance algorithmique : l'oracle énumère
les antichaînes, la tête utilise une programmation dynamique. La
contre-épreuve indépendante ci-dessus couvre l'algèbre et l'égalité `√2/8`, sans prétendre avoir
rejoué tous les F14. Conserver les attendus manuels F14a–e sur la
condensation, les plateaux, `mcs` et la forêt, puis les comparer à la tête
native par la porte exacte. Des constantes de scores gravées seules ne
qualifient pas les clusters effectivement sélectionnés. Cette réponse
ne nécessite pas une nouvelle campagne avant d'implémenter S10.

## S9 publiée : raccord des points

Sur la capture du brouillon au-dessus de **53c027fe8**, les chemins
mathématiques relus concordent : qualification aux coupes fermées,
LCA, pendaison, propriétaire vivant, rang plancher maximal et plateaux
atomiques. Six petits témoins recoupent **43 qualifications, 387 LCA et
29 propositions de plancher**, ainsi que les blocs à chaque plateau et
les entrées dans un bloc vivant. Les propositions flottantes sont
corrigées par les deux certificats exacts du plancher et de son successeur.

Cette contre-épreuve exclut le chemin de refus du tri. Le correctif publié
en **3d47eaa93**, identique à la capture relue,
remplace le tri défaillant par un tri qui propage immédiatement
le refus ; relecture favorable, qualification G4 à confirmer.
[Correctif et portée](../receipts/audit_s9_sort_fix_20261005/README.md).
Aucun succès de ce modèle Python ne
qualifie la sortie native S9. Le lecteur du fichier contrôle des
conditions nécessaires ; le plancher maximal et l'arbre complet sont
recoupés par les portes différentielles du moteur.

**Qualification S9 :** la session G4 `claudequals`, sur **d26328fe2**,
ferme les portes CLI des trois trames LiDAR normal/`-O` sous TSan. Les
portes courtes de signes et de refus de tri ne font pas partie de cette
sélection. Ces succès ne remplacent pas les quatre différentiels
`points_vs_python`, toujours exclus par la matrice. Garder leur exécution
dédiée sur G4, notamment sur les trois trames entières : l'oracle borné et le lecteur ne
certifient pas seuls l'identité complète des champs à K5, dont le plancher
maximal. Le moteur conserve sa certification exacte de ce plancher ;
aucun défaut produit n'est déduit de cette limite des portes.
[Résultats G4 S et périmètre exact](../receipts/audit_g4_s_20261005/README.md).
[Contre-épreuve ciblée et plan Python épinglé](../receipts/audit_s9_qualification_scope_20261005/README.md).
[Périmètre, contre-épreuve et alerte de tri](../receipts/audit_s9_wip_20261005/README.md).

## S8 : signes et égalités exacts pour la sortie points

Relecture favorable au pin **53c027fe8**, sans nouveau défaut important
établi. La conversion `c√(n/d) = (c/d)√(nd)` est exacte. La signature
arithmétique n'est qu'un filtre nécessaire ; deux termes ne sont réunis
qu'après preuve que leur rapport est un carré rationnel. Les classes
distinctes sont linéairement indépendantes sur les rationnels : une somme
nulle est donc certifiée par l'annulation de ses coefficients de classes.
Un intervalle contenant zéro ne suffit jamais à déclarer une égalité.

Une contre-épreuve dans `Q(√2,√3,√5)`, par calcul algébrique indépendant,
recoupe **80 signes dont 20 égalités**. Une collision construite de la
signature à 229 bits conserve bien deux classes. Les décisions certifiées
de `RootTable` et sa borne i128 sont contrôlées pour u18/u21/u24.
Le témoin quasi nul à l'échelle 2^40 demande 192 bits et reste négatif ;
celui à l'échelle 2^2050 épuise le raffinement et refuse explicitement.
La capacité finie reste une limite déclarée, pas une décision approchée.

Le rejeu Python porte sur ces formules et sur les sources figées ; il
ne qualifie pas l'implémentation C++. A2 sur **b319efc84** précède S8.
La session S apporte depuis les résultats LiDAR cités ci-dessus ; elle
ne sélectionne pas la porte courte `num_roots`. La qualification complète
des égalités de dates et des refus conserve ses propres portes.
[Preuve bornée et périmètre](../receipts/audit_s8_20261005/math/REPORT.md).

## Contre-épreuve générale du 5 octobre

Au pin **238734f1d**, aucun nouveau défaut mathématique FULL n'est établi.
La [nouvelle preuve bornée](../receipts/audit_geant_20261005/math/REPORT.md)
compare les composantes du nerf complet des régions témoins au graphe de
la définition : pour deux K-parties quelconques, le seuil de leur union
est testé, sans se limiter aux unions de K+1 sites. Les composantes sont
comparées comme **ensembles de K-parties**, pas seulement par leurs sites.
Les MEB restent celles de l'étage A : indépendance de l'adjacence et de
la DSU, pas de la géométrie numérique.

Dix nuages nouveaux, tous les ordres jusqu'à K=n : **65 ordres, 17 276
unions arbitraires, 1 453 coupes strictes/fermées et 15 925 contrôles de
faces verticales**. S1 recoupe 563 boules et 635 supports. Sur les huit
nuages u21, A=B à tous les ordres, et les 304 dates et propriétaires de
points comparés sont exacts. Les deux nuages aux extrêmes u24 n'exercent
que A/S1 : aucun transfert au moteur ou à la référence constructive u24.
Sorties normal/`-O` identiques ; fixtures, commandes, limites et tentatives
initiales du harnais sont conservées dans le reçu.

**Frontière L3 tranchée par le développeur en 4f1e0fb3a.** Pour K=n≥2,
la qualification des points à m=K+1 ne peut réussir. Le contrat général
de `--k` admet pourtant K=n ; le banc de points refuse alors
`jamais_qualifie`. Garder K=n pour FULL et supports, où l'unique naissance
B(X) est correcte. La [réponse G](REPONSE_CLAUDE_SUPPORTS_20261004.md#g-audit-général-a65903a7b--p1-p2-et-la-mesure-adoptés--k--n-tranché-pour-l3)
retient pour `points`/`plat` un refus `parameter_out_of_range` à K≥n,
pour K≥2, sans publication ni abaissement de m. K=1 reste admis avec
m(1)=1. Décision cohérente, à inscrire dans le contrat et les portes L3.
Cette frontière concerne
la future livraison L3, pas un défaut d'une sortie native déjà livrée.

## Sortie supports : revue du contrat et du raccord

**Suivi L1 : témoins difficiles publiés dans S6a/19b2fb218.** Les sources
sont identiques à la capture relue : coquille de 24 sites, comptes K10/K12 et refus à 25 sites au
niveau des primitives. Les nouvelles primitives Python sont rejugées en
normal et `-O` ; les 4 096 choix antipodaux retrouvent 116 traces strictes
à K12 avec les supports Gram de S1. Le budget de `_minimal_nonseparable`
et `_lemma_f` refuse avant tout appel MEB de ces primitives à 24 sites.
La garde d'arité impossible est aussi corrigée dans les helpers publiés.
Aucun nouveau défaut mathématique établi ; portes natives, raccord S3 et
assemblage S6b restent à qualifier sur la source intégrée.
[Capture, résultats et limites](../receipts/audit_l1_followup_20261005/README.md).

**Rattachement S3 : différentiel exact désormais câblé.** La nouvelle
porte compare `build_order` sur `Cat_K` à la définition S1 : arbre,
attaches à la coupe fermée, antécédents à la coupe ouverte, rôles et traces
strictes. Elle exerce W1 et W3 avec permutation d'entrée, D2/E5 et le
témoin de 12 sites à K1..12. Ce raccord ferme la demande d'inscription du
juge permanent ; son exécution native et la qualification S6b/S7 restent
distinctes de la relecture Python. S3 est publiée en **165def5ab**.
[Preuve et limites](../receipts/audit_corrections_s3_s5_20261005/README.md).

**S6b : assemblage mathématique relu au commit publié 9e7428995.**
Postordre, seaux stables, tous les supports positifs minimaux et comptes
par boule/support concordent avec le contrat. L'égalité entre le compte
des traces strictes et le journal est contrôlée avant publication.
L'ordre `BallIdx` à niveau égal coïncide avec l'ordre lexicographique de
S* : deux supports positifs minimaux distincts de même rayon ne peuvent
être préfixes l'un de l'autre. Le rembourrage par `kNone` ne change donc
pas cet ordre. Aucun nouveau défaut mathématique établi ; S7 est désormais
publiée, qualification native encore ouverte.
[Contre-épreuves et périmètre](../receipts/audit_s6b_20261005/README.md).

**S7 : fichier et signature relus au pin 966a351be.** Huit fichiers
construits depuis S1 sont relus conformément, jusqu'à K10/K12 sur petits
cas ; la signature V2 recoupe une sérialisation indépendante. Les
prédicats entiers du lecteur concordent avec le Gram exact, y compris
aux bornes u24, en Python normal et `-O`. Aucun défaut important établi.
Retirer les tétraèdres du cube à K1 confirme la limite déclarée du lecteur
(coquille absente) ; le différentiel complet contre S1 détecte bien cette
omission. Les preuves bornées ne qualifient pas le produit C++ courant.
[Sources, rejeu et limites](../receipts/audit_s7_20261005/math/REPORT.md).

**Contrat S0/S1 relu : favorable sur les points difficiles.** Les lemmes
P/W, les deux lectures du retrait E5 et le témoin D2 concordent avec les
modèles exacts indépendants. D2 ne borne pas la naissance d'une trace par
le niveau précédent : `41<64<1681/25` reste le contre-exemple permanent.
La constance de H₀ transporte sa classe. Le juge E2 admet `initial≤λ`,
puis l'ancêtre fermé ; le journal conserve les traces strictes `initial<λ`.
Le journal et les supports sont attribués après tout le plateau, sans
confondre unions DSU et composantes intrinsèques.

Les **13 mutants S1** sont tués avec leur cause nommée sur les copies
figées ; les coupes exactes de sept fixtures sont recoupées par un
solveur/Γ indépendant. Ce rejeu borné ne remplace ni toute la suite S1,
ni les tests natifs G4. Aucun nouveau défaut mathématique établi dans S0.

**Deux gardes corrigées dans les sources publiées.** S6 contrôle `p≤11`
avant `p+q`, avec refus UINT32_MAX et frontière `(11,24,2,12)`.
La nouvelle capture S3, `attachment.cpp` SHA **77f84f0050…**, appelle
`published_traces(end−begin)` avant conversion : au-delà de UINT32_MAX,
refus `tower_capacity`. Le plafond 24 de `supports` reste distinct de FULL.
Les fixtures D2/E5 sont maintenant présentes en S3. Le raccord final du
journal est favorable : graines enregistrées avant le DSU, publication
par ordinal, puis attribution après fermeture du plateau. Le compactage
en place conserve `prior_total≤begin` et `prior_total+branches≤end`.
Le refus conserve domaine et diagnostics ; journal, sweep et copie finale
ont leurs coexistences budgétées. **15 283 gardes scalaires nouvelles**
recoupent ce raccord, sans nouvelle qualification native/TSan.

**D.1–D.4 sont adoptés par le contrat.** Les agrégats `kparties_reliees`
comptent des incidences `(b,F)` ; les cofaces sont distinctes par boule.
L'indépendance à Q_b ne vaut qu'à p,m,K fixés. La signature V2 est
recalculable depuis SP, sans PointId/BallIdx ; FUL1 garde ses octets et
ne suffit pas à ce recalcul. L'état `published_complete` et le SHA du
manifeste fermé distinguent publication, transport et durabilité. La
stricte descente du sélecteur comprimé faible ne se transfère pas à toute
K-partie. [Contrat courant](../docs/SORTIES.md),
[réponse du développeur](REPONSE_CLAUDE_SUPPORTS_20261004.md).
Ces engagements V2/publication sont désormais présents dans la capture S5,
avec portes du champ publié, SIGXFSZ et refus après publication.

[Relecture actuelle et témoins](../receipts/audit_native_integration_20261005/README.md).
Les constats et réponses précédents restent figés dans les reçus
[contrat S0/S1 et coquille mixte](../receipts/audit_supports_contract_20261005/README.md),
[1 119 gardes](../receipts/audit_supports_implementation_20261004/README.md),
[6 456 gardes](../receipts/audit_supports_followup_20261004/README.md) et
[première revue supports](../receipts/audit_supports_20261004/README.md).

**Réponse à la question du 5 octobre : une coquille mixte au plafond.**
La sphère entière $x^2+y^2+z^2=5$ a 24 sites. Après translation par
`(2,2,2)`, elle est une entrée valide des trois profils. Elle donne
**12 q2, 24 q3, 792 q4**, soit 828 supports stricts, sur toute la coquille.
Exemples avant translation :

- q2 : `(2,1,0),(-2,-1,0)` ; poids1/2 chacun.
- q3 : `(2,1,0),(-2,1,0),(1,-2,0)` ; poids`(1/4,5/12,1/3)`.
- q4 : `(2,1,0),(-2,1,0),(0,-1,2),(0,-1,-2)` ; poids1/4,
  déterminant−32.

La fermeture est non triviale : $N_2=12$, $N_3=288$, $N_4=3906$.
À K3, la somme des cofaces par support vaut 4068, contre 3906 cofaces
uniques : ce témoin tue précisément leur confusion. À K1, les 792 q4
restent publiés malgré zéro coface par q4.

| K, p=0 | K-parties reliées | Traces strictes | Cofaces par boule |
| --- | ---: | ---: | ---: |
| 1 | 24 | 24 | 12 |
| 2 | 276 | 264 | 288 |
| 3 | 2024 | 1736 | 3906 |

Le dénombrement exact par combinaisons≤4 est confronté au Gram Fraction,
sans parcourir les $2^{24}$ masques. C'est un bon oracle `long` **des
primitives Q_b/N_j**, avec seulement j≤K+1 pour K≤3. Remplacer le seul
calcul N_j dans l'oracle S1 complet ne suffit pas :
`_minimal_nonseparable` prolonge aussi les sous-parties séparables
jusqu'à m. S6a contient un budget/refus explicite pour
cette extension ; aucune
censure silencieuse ni qualification de tout S1 à 24 sites par ce témoin.
Les autres sphères proposées sont des compléments facultatifs ; préférer
ce cas mixte, puis 25 des 30 sites de rayon carré9, en conservant les six
points axiaux, pour le refus support_shell_capacity de l'appel entier.
Le plafond ne s'applique pas à FULL. La boule canonique de ce témoin
vient d'un diamètre : il exerce Q3/Q4 sur une présentation q2 et ne
remplace pas les portes numériques des centres de présentation q3/q4.
[Énumération indépendante et mutations](../receipts/audit_supports_contract_20261005/qb/README.md).

**Compléter les portes aux K élevés, sans construire FULL12.** Sur cette
même coquille, toute partie de taille≥13 contient une paire antipodale,
donc `N_j=C(24,j)`. À taille 12, 116 des 4096 choix sans paire sont séparables ;
un calcul indépendant des chambres de douze plans centraux retrouve 116.
Ainsi **N12=2 704 040** et **N13=2 496 144**. La primitive à K12 doit donner
**2 704 156 parties reliées/comprimées, 116 traces strictes et
2 496 144 cofaces distinctes/Gabriel** ; les 149 954 688 incidences par Q
ne remplacent pas ce dernier compte. Récupérer la fermeture centrale dans
Cat1, puis appeler `make_shape`/`ball_counts` : cela ne qualifie ni un arbre
FULL12, ni `ball_shape` à K12 sur un domaine préparé à K1.

Un deuxième témoin, seulement 12 sites, exerce p=8,m=4,qmin2 à K10 :
**66 parties reliées, 6 comprimées, 4 traces strictes, 12 cofaces et
4 Gabriel**, contre 20 incidences par Q. Il complète la porte LiDAR longue
et les cas S3 usuels K≤5. Les portes S6a publiées reprennent ces attendus
indépendants aux indices intermédiaires hauts ; S3 et S6b exercent aussi
le témoin de 12 sites jusqu'à K12. Leur exécution native reste à qualifier ;
aucun échec natif observé.
[Coordonnées, preuve et 4 135 gardes exactes](../receipts/audit_native_integration_20261005/qb/README.md).

**Décisions courantes : Q_b seul, puis points, puis plat.** La réponse primaire
du 4 octobre à 20:25 UTC retient le squelette des supports ; la proposition
`POP=P_b` du plan révisé est donc caduque. « K-parties reliées » désigne
explicitement les sommets de Γ_K reliés **par la boule**, distincts des
cofaces et des unions DSU. Le triangle aigu à K2 en relie trois, alors
qu'aucune K-partie ne contient son Q de taille3 : ce dernier compteur ne
répondrait pas au choix utilisateur. **S2/257aabb92 et S4/f98aeed67 sont livrés en
source** : en-tête public et io. L'attribution S3 et l'assemblage Q_b/S6
sont désormais publiés jusqu'à **9e7428995** ; leurs rapports locaux ne
constituent pas une qualification G4 de l'export S7 désormais publié.

**D2 : correction de preuve reprise dans le contrat mathématique L0.**
L'ancienne justification `β(F)≤niveau critique précédent` est fausse. À K2, prendre
A=(2,10,0), B=(18,10,0), C=(10,20,0), Z=(9,3,0), W=(11,3,0).
La boule ABC est retenue au niveau carré1681/25 ; sa trace stricte AB a
MEB de niveau64, absente de Cat2 parce que p2+qmin2>K+1. Le niveau
retenu précédent est41 : **41<64<1681/25**. AB n'existe pas à41,
mais sa graine ZW, née à1, continue vers la bonne composante à64.
T5 s'applique à `max(niveau précédent,β(F))`, puis la constance de H0
jusqu'au prochain événement transporte la classe vers la coupe précédente.
L'ancêtre de la graine et le propriétaire après plateau restent cohérents.
Ne pas transformer l'inégalité fautive en garde d'admission native.

**Limiter l'identité des octets à ce que le format peut garantir.**
Le plan révisé promet des octets identiques après réétiquetage, tout en
conservant `SITES.point_id` : ces deux exigences se contredisent. Les labels
plats suivent l'ordre d'entrée et les manifestes hachent les fichiers bruts.
Comparer les structures géométriques après transport des IDs et permutation
inverse des lignes ; recalculer les hashes de provenance. Même entrée et
même requête permettent la garde d'identité sous changement de workers.
Sous réétiquetage, la règle de nommage des clusters doit aussi être appliquée
aux nouveaux IDs ; transporter seulement l'ancien label minimal ne suffit pas.

**Deux portes précises pour S6.** Le helper Euler marque Q_b, puis la
fermeture zêta modifie ses bits : les6 supports du cube deviennent177
parties. Extraire les supports avant cette transformation. À K1, les deux
q4 du cube restent dans Q_b malgré zéro coface d'ordre2 : ne pas filtrer
les supports par `|Q|≤K+1` ou par un compteur de cofaces positif.
Ce sont des gardes pour le port prévu, pas des défauts d'un module livré.

[Contrelecture, décision primaire et preuves](../receipts/audit_supports_followup_20261004/README.md) :
**6 456 gardes portables**, normal/−O identiques ; aucun natif, fit ou GCP.

**Conseils précédents intégrés, à garder comme portes.** Arbre K seul,
propriétaire fermé après tout le plateau, continuations datées, Q_b depuis
toute U et toutes arités, fenêtre faible `p+qmin−1≤K≤p+m` : conception
favorable. `C(m,K−p)` compte les parties comprimées contenant tout I,
`C(p+m,K)` les K-parties fermées ; la somme des cofaces par Q compte des
incidences. Les unions DSU ne sont pas intrinsèques aux boules. Les
géométries des supports peuvent se recouvrir entre branches : leur
intersection ne définit pas la connectivité FULL. La coquille n'est pas
bornée par K ; le chrono FULL ne qualifie pas ce nouvel export.

**Q_b reste discontinu comme carrier.** Quatre points cardinaux du cercle
portent deux diamètres ; une perturbation rationnelle du point supérieur
ajoute un triangle et donne un saut de Hausdorff≥1/4, pour un déplacement
d'entrée tendant vers zéro. FULL stable ne suffit donc pas à qualifier un
tokenizer stable. Neuf immersions exactes sont jugées en u21 ; la limite
analytique ne devient pas une limite de perturbations arbitraires sur ce
domaine fini. [Preuves, fixtures et 1 365 gardes](../receipts/audit_supports_20261004/README.md).

## Résultats utiles pour la tête plate

**Le critère B se calcule par une rencontre et sa date est stable en 3ε.**
Pour H_i=(o_i,e_i) et core_i=(c_i,d_k(x_i)), self compris :
`sB_i=max(e_i,d_k(x_i),naissance(LCA(o_i,c_i)))`. Un LCA par point
remplace le balayage de toutes les coupes du prototype. La preuve combine
l'alignement fort H3 et le segment entre points appariés, couvert au rayon
d_k+2ε ; k/m/IDs fixes, nuages finis, contacts fermés. **156 dates B**
concordent avec le code privé extrait par AST. Cela ne restaure pas les
points frontière perdus par B, ne stabilise pas une antichaîne EOM à égalité
et ne prouve pas une meilleure pertinence statistique.
[Preuve et 12 968 gardes exactes, normal/−O](../receipts/audit_giant_20261004/mathematics/README.md).

**B garde également le plafond L6 d'H.** Poser ρ=d_k(x), D=e−t′.
Pour le premier propriétaire qualifié p=(C,t′), choisir y∈C,
ℓ=|x−y|≤t′. Les k points de sa boule donnent ρ≤t′+ℓ. Le segment des centres
est couvert dans L_k au rayon S=max(t′,(ρ+t′+ℓ)/2), avec
ρ≤S≤t′+ρ/2. Il relie p au core, même si ce dernier n'est pas qualifié.
Ainsi **sB≤t′+ρ/2** et **0≤sB−e≤ρ/2−D**. Nuages finis, premier
propriétaire atteint, k/m fixes, sites unitaires et contacts fermés : mêmes
hypothèses que la preuve B3. Cela évite de cumuler deux plafonds pessimistes.

Ce supplément peut pourtant approcher ρ/2 : triangle
x=(0,0), y=(2N,1), z=(2N,−1), k2/m3, e=t′=(4N²+1)/(4N), sB=ρ=√(4N²+1).
La limite concerne une famille géométrique non bornée, pas une constante
optimale sur le domaine fini u21. B peut encore perdre une branche frontière ;
aucune optimalité statistique de B n'en découle.
[Preuve adverse, huit couples nuage/ordre et 931 gardes](../receipts/audit_deep_20261004/math/README.md).

La revue FULL détaille MEB/supports, Γ↔Lk, morceaux stricts, descentes,
multifusions et verticales. Sur huit nouveaux nuages : **32 ordres et
1 082 coupes ouvertes/fermées**, contrôle de la couverture complète de
chaque composante, parents/verticales/core compris. Aucun nouveau défaut
mathématique FULL établi ; les théorèmes restent nécessaires au-delà du
domaine borné. [Chaînes de preuve et limites](../receipts/audit_giant_20261004/mathematics/PROOF.md).

**La condensation et le raffinement en z sont confirmés mathématiquement.**
À arbre et cohortes fixés, augmenter z dans λ=r^(-z) raffine la sélection EOM,
racine d'admissibilité fixe, parent aux égalités certifiées. La preuve traite
le masquage des décisions par les ancêtres ; 728 comparaisons exactes sur
26 condensations concordent. Le raffinement porte sur les clusters sélectionnés ; agréger tous les
points bruit en un bloc ne transfère pas ce théorème à cette partition.
Cela ne compare ni deux modèles d'existence,
ni leurs IoU, et ne prouve pas l'optimalité statistique de z=1 ou z=3.
E1 a depuis fixé **z=2 sur le dev synthétique** ; P08 distingue z=2 pour
les fusions et z=1 pour la non-infériorité IoU. Ces choix ne constituent pas
une optimalité mathématique ou statistique universelle.
[Preuve indépendante et 12 917 gardes](../receipts/flat_model_followup_20261004/README.md).

**Une séparation fugace ne force pas l'union pour tout z.** L'explication
de `SORTIE_PLATE.md` §3.2 est corrigée en **723cf6e43** : seuls les z=1,2,3
testés sont concernés pour les trois vélos b00_001472/k5. La condensation
A/mcs20 réelle possède
une union admissible de masse 284, avec deux enfants de masses 116 et 97,
dont les cohortes ont une persistance strictement positive. À **z=16**, après
le même facteur positif 100^16, son score est **≤70,394**, contre
**≥132,525** pour la somme des deux enfants. Encadrements rationnels
certifiés, **504 gardes**, normal/−O ; l'union perd donc aussi face au meilleur
score descendant. Dire « les z=1,2,3 testés retiennent l'union ».
Cela ne recommande pas z=16 et ne prédit pas une meilleure qualité par objet.
[Cohortes et certificat autonome](../receipts/audit_selfreview_20261004/README.md).

Pour des dates de comptage s_i≥e_i, R_i est la mcs-ième valeur de
max(u_ij,s_j). Les blocs admissibles sont exactement ceux du facteur
**max(u_ij,R_i,R_j)**. Mais un point ne contribue au score qu'à
max(R_i,s_i) : transporter son calendrier et ses cohortes. Une condensation
standard du facteur ne conserve les scores automatiquement que pour A,
s_i=e_i. Témoin géométrique à six sites, calendrier différé admissible :
score 1/5, contre 3/10 si chacun est compté dès R_i. Ce calendrier n'est
pas présenté comme la valeur B/C/E de ce nuage. Pour les poids fixes, le
rang devient un seuil de masse cumulée.

**Une égalité EOM peut être certifiée sans la supposer au budget.**
Pour e=√t+√M−√q>0, poser d=t+M−q, Δ=d²−4tM. Si Δ≠0 :

```
1/e = [(t−M−q)√t + (M−t−q)√M + d√q − 2√(tMq)] / Δ.
```

Si Δ=0 et e>0, e=2√min(t,M) ; les dates nulles exigent leur politique propre.
La réciproque et son cube utilisent au plus quatre classes carrées **pour
cette forme H**, et pour B/C qui choisissent H ou une date core. Ce domaine
ne couvre pas une maturité générale Eθ : une combinaison de quatre radicaux
indépendants exige huit classes dans le témoin algébrique relu, sans nuage
géométrique correspondant revendiqué. [Garde de portée](../receipts/audit_deep_20261004/math/README.md).
Le regroupement des classes certifie un score nul ; le signe non nul demande
encore intervalles, budget et refus. Témoin HGP entier plongé par x↦(x,x,0) :
entrées 6√2, fusion A/B à 8√2, racine à 12√2, scores parent/enfants **√2/8**.
La tête privée par intervalles choisit le bon parent mais signale un choix
forcé à 128 bits. Ce n'est pas une erreur de labels ; la garde aide à qualifier
la vraie égalité. **888 gardes normal/−O**, mutation du facteur 2 tuée.
[Formule, cas singulier et helper autonome](../receipts/eom_exact_audit_20261004/README.md).
Aucune borne rapide/native ne découle de ce regroupement naïf.

**Trois gardes préviennent des raccourcis futurs, sans défaut produit établi.**
La croix de quatre sites à k3 naît avec population4>k : population=k est
suffisante pour une naissance, pas nécessaire. Pour X={0,2,4,6}, k2/m4,
la qualification arrive à2 alors que la première couverture d'ordre4 est3 :
l'identité t′=α_(k+1) ne se généralise pas à α_m. Enfin la naissance de la
racine ne borne pas toutes les attaches/core : un témoin a root birth²16,
qualification²25 et core²80. Conserver ces niveaux dans l'arbre compact.
[Rejeux exacts et hypothèses](../receipts/audit_deep_20261004/math/README.md).

## Contrats de la tête plate et du port natif

Conserver comme bras explicite : masses entières des sites engagés, mcs≥2,
plateaux N-aires exacts, antichaîne globale, racine exclue, parent sur égalité
certifiée, non-affectés en bruit. E1 livre ce bras en Python ; z=1/2/3 et
feuilles sont distincts, les complétions et autres calendriers sont reportés.
Tester chaque bras primaire contre l'oracle. Les masses de
faces fractionnaires de la thèse changent le modèle et perdent T0 à mcs=3.

- Pour mcs≥2, e_i≤u_ij permet d'ignorer les naissances singleton ; construire
  néanmoins le véritable arbre de points, avec les niveaux créés par les
  attaches. Après condensation, une petite branche peut sortir avant e_i^(-z).
  [Contrats](../receipts/flat_selection_contract_20261004/README.md),
  [précision sur les sorties condensées](../receipts/flat_selection_math_r2_20261004/README.md).
- L'ordre exact des dates ne qualifie pas les sommes EOM. E1 distingue
  désormais égalité certifiée, signe séparé et refus au budget ; les anciens
  arbitrages flottants ne sont pas ses décisions. La comparaison contre
  l'oracle initialement livrée couvre z=1/z=3 et feuilles ; **z=2 est ajouté
  en 723**, avec une fixture distinctive, mais son nouveau rejeu G4 reste
  attendu avant de transférer cette qualification.

La stabilité 3ε de H ne garantit pas la stabilité de la partition à une
égalité EOM. À arbre/cohortes correspondants et rayons≥r0, une marge de score
supérieure à sa borne d'erreur garantit le choix. Les neuf sites
A={0,2,4}, B={7,9,11}, D={17,19,21} donnent A∪B|D à z=1 et A|B|D à z=3 ;
une petite perturbation près d'un ex æquo peut aussi inverser le gagnant.
[Preuve exacte et garde de marge](../receipts/flat_selection_math_20261004/README.md),
[correction de portée](../receipts/flat_selection_math_r2_20261004/README.md).

## Comparaison à HDBSCAN et métriques

Le rapport d'équité a adopté **R0 officiel intact** et un bras distinct à
sélection N-aire commune. C'est le bon périmètre ; les versions locales
1.9.1/NumPy2.5.3 et G4 1.7.2 restent à qualifier sur le même banc.
520/2 400 partitions diffèrent à epsilon=0 après atomisation ; le total
1 015/6 752 mélange sorties officielles et secours après exceptions.
La comparaison de sources 1.7.2/1.9.1 n'est pas une nouvelle porte G4.

**Corriger M3 du rapport de mesure : B(H) plafonne uniquement des clusters
qui sont des blocs de cet H.** Sur leur fixture F2, R0 produit {6,9}, union
binaire qui n'est pas un bloc du plateau atomique. En prenant ses trois
clusters comme cible : score plat 1, plafond atomique 5/6 (7/9 sans
singletons). Ce n'est pas une anomalie de score observé : les univers diffèrent.
Réserver le plafond atomique aux bras N-aires, ou annoncer un plafond R0
sur son propre arbre binaire. [Témoin et 39 contrôles](../receipts/flat_evidence_followup_20261004/README.md).

Le meilleur bloc par objet peut sélectionner parent et enfant simultanément :
ce n'est pas une partition. E1 mesure désormais une antichaîne et un
appariement global un à un ; meilleur-IoU reste un diagnostic. La portée
revendiquée du vérificateur hongrois a été réduite, sans imputer un score
réellement faux à l'absence de certificat dual.
[Archives et gardes](../receipts/flat_selection_evidence_20261004/README.md).
Le nouveau lot de membres fournit un cas réellement compatible avec une
antichaîne : deux vélos, k5, blocs HGP133/83 disjoints, IoU0,964/0,711.
À k10, leurs meilleurs blocs sont imbriqués. Le nouveau diagnostic plat
k5/mcs20 de ce bout ne retrouve justement pas les deux meilleurs blocs :
EOM z=1/z=2 donne IoU par objet **0,529/0,500**, le second ne franchissant
pas le seuil strict >1/2. Ce sont les observables du renderer, pas un nouveau
calcul hongrois ni une nouvelle mesure de notre part. Garder distincts
compatibilité d'antichaîne, sélection effectivement jouée et coupe commune.
[Diagnostic plat publié](../../Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_00_001470_deux_velos_43_61/plat.json).
[JSON clos et vérification des intersections](../receipts/audit_deep_20261004/math/README.md).

**Attribution : documentation corrigée, protocole encore à aligner.**
`SORTIE_PLATE.md` retire en 723 l'inférence de contribution nulle sous
T−A non significatif. Le préenregistrement JSON conserve pourtant cette
inférence dans `decision_rule.attribution` et `predictions.PS2`.
Consigner un corrigendum d'interprétation : « contribution supplémentaire
de la hiérarchie non établie ». Conserver seuils, prédictions historiques,
décomposition T−R0=(T−A)+(A−R0), estimations et incertitudes. Une absence de
rejet n'établit pas la nullité. [Six recoupes du décalage](../receipts/audit_ports_20261004/head_prereg_delta/README.md).

## Socle FULL → points : acquis et limites

Règle étudiée à k≥2 fixé : P1 après qualification m=k+1, en rayon, coupes
fermées ; e=t′+sup_q(m(p,q)−h(q)), propriétaire vivant au plateau exact.
[Q1–Q8, preuves détaillées](../receipts/points_answers_20261003/README.md),
[contrat courant](../docs/HIERARCHIE_POINTS.md) ;
[réponse du 3 octobre archivée à l’octet](../receipts/audit_supports_contract_20261005/notes_before/REPONSE_CLAUDE_POINTS_20261003.md).

| Question | Résultat actuel et portée |
|---|---|
| Stabilité | Identifiants appariés, poids positifs fixes, k/m fixes, nuages finis : interleaving FULL ε, dates/réunions 3ε ; Pκ : (1+2κ)ε. Aucune garantie d'IoU/labels. |
| Optimalité | P1 optimal dans le domaine abstrait intrinsèque. Constante minimale 3 sur l'image géométrique non démontrée. |
| Retard | 0≤e−t′≤d_k/2, t′≤d_(k+1). Pour entrer strictement avant F, exiger t′+d_k/2<F ; ≤ ne vaut qu'au plateau fermé. |
| Plusieurs k | Six sites 0,3,6,18,19,20 : branches qualifiées k2/k3 croisées à r=7. Pas de laminarité commune automatique. |
| Insertion | L'impossibilité générale Q6 sous trois axiomes est fausse. En revanche, Hausdorff ou W_p normalisé fini ne contrôlent pas uniformément les dates SUP avec seuils unitaires ; W∞/erreur pondérée non tranchés. |
| Modèle infini | Palm donne un déficit conditionnel aux mêmes λ/F, pas une convergence des fenêtres. Localement fini ne garantit pas une rencontre atteinte. |
| Port natif | PointRadiusDate et scores exacts sont désormais livrés en S9/S10 ; les différentiels P9/P10 passent, voir en tête. Le seul export FULL C++ + projection Python des captures antérieures ne qualifiait pas ces modules. |

[Insertion/Wasserstein](../receipts/measure_metric_20261004/README.md),
[Palm conditionnel](../receipts/palm_obstruction_20261003/README.md),
[rencontres infinies et κ](../receipts/points_math_followup_20261004/README.md).
κ augmente le rappel dans une même composante FULL ; ni IoU ni partition ne
sont monotones. ER0h n'a pas de constante uniforme pour ses dates en rayon.
Le défaut Decimal de propriétaire au plateau est corrigé et exercé par F.
Le domaine m>n refuse actuellement : déclarer m≤n ou des points inactifs.
[État natif, budgets et preuves G4](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).

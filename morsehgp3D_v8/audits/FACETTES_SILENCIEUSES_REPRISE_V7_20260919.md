# Facettes silencieuses : ce que la v8 doit reprendre de la v7

19 septembre 2026. Audit ciblé de clôture, base `3e94c868abbb0fafac2b9062e72433f0793545cc`.
Compléments du 20 septembre 2026, relus sur `19cee4ac4920419f2906cc8a930b5145de300007`.
Référence moteur inchangée : `morsehgp3D_v7/src/forest/full_ball_tower.hpp`, blob Git
`5d8e9d91124d2a4cf073e67b8cc479f4e9dea897`. Aucun moteur modifié.
Lecture des sources, modèles rationnels et micro-test C++ autonome du comparateur
(§5.5) ; aucun moteur HGP, CTest HGP, benchmark ni GPU exécuté lors de ces audits.
Les résultats natifs historiques restent distincts. `public_status=not_claimed`.

## Décision

**Reprendre le contrat de `full_ball_tower.hpp` et sa séparation statique
« facette → boule terminale → parent pré-lot ». Ne pas porter aveuglément son
stockage global ni revenir à « graphe Gabriel brut + Kruskal ».** Aucun défaut
nominal de rattachement n'a été identifié dans les chemins examinés ; cette
conclusion ne certifie ni le générateur, ni un futur backend, ni toute la v8.
**Pour réduire les MEB : semis exacts d’abord, puis propositions d’ancres bornées
et certifiées. Les §§5.1–5.3 excluent deux sélections apparemment naturelles ;
le §5.4 ne qualifie aucun gain de temps à grande taille.**

| Chemin v7 | Utilité pour la v8 |
|---|---|
| `silent_incidence.hpp` / `mhgp7 --complete-incidences` | Référence régulière historique : matérialise des chaînes de cofaces. Pas l'architecture FULL à reprendre. Option désactivée par défaut. |
| `full_gabriel.hpp` | Référence horizontale régulière : alias et descente de cofaces. Pas suffisant pour les coquilles non régulières et les verticales. |
| **`full_ball_tower.hpp`** | **Référence fonctionnelle CPU** : descente sur K sites, ancres de boule, plateaux, couvertures datées et verticales. Raccord réel dans `full_ball_tower_probe`, pas dans la CLI historique. |

La v8 courante s'arrête au front/census q2 : aucun constructeur FULL v8 ne
bénéficie encore automatiquement de cette correction. Les témoins hérités de
la WSPD ne sont pas des rattachements de composantes.

## 1. Invariants à conserver

**Un événement silencieux peut être absent du dendrogramme, pas son effet sur
les parents.** La réduction Gabriel brute est fausse : l'égalité des unions de
points ne détermine pas les appartenances des facettes.

Pour une facette F de cardinal K, un intrus strict z de sa miniball et un site s
appartenant à un support positif, poser F'=(F∖{s})∪{z}. Avec β=rayon² :

- **Chemin certifié :** β(F∪F')=β(F). L'échange est une vraie connexion de Γ_K.
- **Progrès :** β(F')≤β(F). À égalité, la boule doit rester identique et le
  nombre de sites sélectionnés sur sa coquille diminuer d'une unité. Exiger une
  baisse stricte du rayon partout est faux hors régularité.
- **Terminal :** une clé présente au catalogue n'est utilisable à l'ordre K que
  si `p+q_min-1 ≤ K ≤ p+u`, où p=nombre d'intérieurs et u=taille de coquille.
  Pour un représentant strict consommé au niveau a, sa cible doit avoir β<a.
  Deux descentes peuvent terminer sur des boules différentes ; elles doivent
  donner le même parent à la coupe demandée, pas le même `BallId`.
- **Temps :** résoudre tous les représentants stricts sur l'état pré-lot ;
  regrouper les blocs par racines communes, jamais par points communs ; fermer
  atomiquement le niveau, puis publier les ancres. Conserver aussi l'ancre d'un
  bloc inerte sans nouveau nœud ni contribution, ou un moyen certifié de la retrouver.
- **Couvertures/verticales :** ne pas fabriquer de nœud pour une simple croissance
  de couverture. Les images inférieures se lisent à leur coupe fermée ; une
  racine finale ou un jeton union-find non daté n'est pas une image historique.

Ces règles sont effectivement présentes dans `static_terminal`, `resolve`,
`prepare_block`, `close_lot`, `seed_closed_anchor` et le normaliseur historique
([constructeur](../../morsehgp3D_v7/src/forest/full_ball_tower.hpp)).

## 2. Contre-test minimal obligatoire, déjà dans le domaine u16

Kmax=2 ; points A=(0,1,0), B=(2,5,0), C=(4,1,0), D=(1,0,0), E=(3,0,0).
C'est la translation entière du contre-exemple plan.

| Niveau β | Attente |
|---|---|
| 5/2 | ADE et CDE, Gabriel, donnent C₀={AD,AE,CD,CE,DE}. |
| 4 | ACD/ACE, non-Gabriel, rattachent AC à C₀, sans nouveau nœud. |
| 5 | AB et BC sont deux composantes distinctes de C₀. |
| 25/4 | ABC fusionne **C₀, {AB}, {BC}**. Γ₂ a une composante ; Gabriel brut en a deux. |

**La boule de AC est absente du catalogue utile à Kmax=2** : p=2, q_min=2,
donc p+q_min=4>3. Le résolveur doit pourtant traiter AC. Une descente correcte
est AC→CD ; MEB(CD)=MEB(CDE), β=5/2, retrouve l'ancre de C₀.
Ainsi, rechercher seulement les facettes/boules acceptées par q2 est incorrect.
Les MEB intermédiaires doivent pouvoir sortir de la fenêtre du catalogue, sans
être toutes stockées ni soumises à nouveau au filtre d'admission amont.

**Test supplémentaire pour le backend par lots :** remplacer volontairement,
pour la demande AC avant ABC, la cible correcte par MEB(AB). Cette mauvaise
cible satisfait le rang et β=5<25/4, mais appartient à un autre parent. Même
l'égalité de leurs images K1 ne détecte pas cette substitution dans cet exemple.
Le juge Γ doit la réfuter. Les contrôles rang/date/ordinal de
`prepare_external_batch` ne prouvent donc pas la géométrie du résultat.
Ce constat porte sur le contrat du callback ; aucun défaut du backend nominal
n'est établi par cette substitution dans le modèle indépendant.

## 3. Contrat de portage et limites à ne pas masquer

**Géométrie d'abord, histoire ensuite.** Dédoublonner les requêtes par
`(nuage et catalogue immuables, K, IDs de la facette)` et respecter le consommateur le plus
précoce. Retourner un `BallId` stable, puis normaliser son ancre à chaque coupe
consommatrice. Un cache de jetons temporels doit rester limité à un ordre et
normaliser ses hits ; collision, éviction ou allocation impossible ne suppriment
aucun travail requis. Les semis connus utilisent tout I∪U, jamais un support
partiel. Préférer des lots bornés à la matérialisation de toutes les demandes
d'un ordre ; cela peut perdre du réemploi, pas de la correction.

**Le raccord de données n'est pas un cast.** Le flux v8 `Q2Support` fournit des
vues empruntées pendant le callback, des IDs originaux et plusieurs incidences
pour une même boule. Posséder les données nécessaires après le callback,
remapper explicitement les indices spatiaux, canoniser les boules et vérifier
l'accord I/U des doublons. Ne pas réinterpréter `size_t` en `i32/u32` sans garde.
Les voies q3/q4 restent indépendantes de l'acceptation q2.

Pour convertir une clé q2 v8 `(c2=a+b, d2=|a-b|²)` au format polynomial v7,
utiliser la forme entière `4|x|²-4c2·x+|c2|²-d2`, puis PGCD et coefficient
quadratique positif. Sa puissance est **négative à l'intérieur**, contrairement
à la puissance utilisée par q2 v8. Le niveau est d2/4, sans flottants. Même rayon ne signifie
jamais même boule ([clés v7](../../morsehgp3D_v7/src/lanes/keys.hpp),
[flux v8](../src/pipeline/q2_census.hpp)).

**Deux obligations restent ouvertes :**

- Le statut v7 est *relatif à des census complets et exacts*. Vérifier les
  éléments fournis ne prouve pas leur complétude. Le backend de résolution
  doit être qualifié sur le même nuage/catalogue immuable ; métadonnées ou
  digest seuls ne certifient pas la composante retournée.
- La coquille v7 est limitée à 12 sites et le quotient non régulier utilise
  des tables en 2^u. La v8 q2 accepte des coquilles plus grandes : ni troncature,
  ni simple remplacement du tableau par un vecteur. Un premier raccord borné
  doit refuser explicitement hors domaine ; le cas général exige un autre
  traitement ([quotient local](../../morsehgp3D_v7/src/forest/local_plateau.hpp)).

## 4. Coût réel : ne pas confondre longues descentes et nombreuses MEB

**Reprendre la sémantique v7, pas promettre sa vitesse.** Les captures ci-dessous
sont celles du **10 septembre**, uniforme u16, 50k, s8, constructeur FULL CPU
mono-thread. Les 48 threads concernent l'amont. Ni nouveau benchmark ni résultat
v8 ; le temps FULL comprend aussi les histoires, contributions et verticales.

| Travail cumulé sur la tour | Kmax=5 | Kmax=10 |
|---|---:|---:|
| Résolutions hors cache R₀ | 3 501 289 | 31 999 164 |
| Échanges avec un intrus D | 882 236 | 9 987 037 |
| MEB du résolveur M | 4 383 525 | 41 986 201 |
| Supports candidats testés | 44 413 779 | 3 898 856 828 |
| Temps FULL historique | 27,228 s | 389,668 s |

Recalcul depuis les sorties brutes [K5][run5] et [K10][run10] :
`R₀ = resolver_cache_queries - resolver_cache_hits = anchor_hits` et
`M = R₀ + intruder_queries`. Ces identités concernent cette route nominale
achevée, pas tout futur backend. À K10, D/R₀≈0,312 : **au moins 22 012 127
résolutions hors cache sur 31 999 164 (≈68,8 %) n'ont aucun échange**. Hors cache ne signifie pas clé
unique : les mêmes facettes peuvent revenir après éviction. Une moyenne courte
ne borne ni la queue des chaînes ni le coût des recherches spatiales. Les
compteurs ne séparent pas encore le temps/supports des MEB initiales et suivantes.

Le premier poste à comprendre est donc déjà le volume des MEB initiales, et
pas seulement les descentes longues. [anchor_meb.hpp][meb] essaie au plus
`Σ(q=2..min(4,K)) C(K,q)` supports : **1/4/25/375 pour K=2/3/5/10**, chacun
suivi si nécessaire de tests de confinement. C'est la borne de cette méthode,
pas un coût minimal incontournable de la MEB. Les captures donnent 10,1 puis
92,9 essais/MEB sur les tours jusqu'à 5 puis 10. Ne pas transformer ces moyennes
globales en profil temporel par K ou par type de requête.

Pour K=2, la géométrie est déjà un diamètre ; cette spécialisation n'annonce
pas un gain nouveau. Pour K=3, proposer le diamètre du plus long côté si son carré
est au moins la somme des deux autres, sinon la circumboule aiguë. Conserver
l'égalité et toute la coquille. Pour les grands K, un support proposé est accepté
seulement s'il appartient à F, est positif et si sa boule contient tous les sites de F.
**Le support parental n'est pas héréditaire** : pour A=(0,0,0), B=(4,0,0),
C=(2,1,0), MEB(ABC) a le support AB ; après retrait de A, C devient support de BC.
Tout essai non concluant retourne au calcul exact, sans exclusion géométrique.

| Régime | Conclusion permise pour le résolveur |
|---|---|
| Uniforme volumique | Travail élevé à K10 déjà mesuré. Le triplet statique initial 8k/16k/32k donne 4,19/8,78/18,24 millions de MEB, croissance locale proche de ×2,1, pas une borne universelle. |
| Terrain mince, huit amas volumiques | Pas de mesure FULL de ce raccord sur ces recettes. Une couche mince n'est pas un plan exact ; des amas éloignés ne suppriment pas le travail interne. |
| Deux rangées parallèles exactes | MEB de support ≤3 ; coquille ≤4, car un cercle coupe chaque droite en au plus deux points. Cela simplifie la géométrie, pas une garantie de temps total. |
| LiDAR réel | Ni les recettes synthétiques ni leur temps q2 ne qualifient le coût FULL sur SemanticKITTI. |

La [déduplication statique][static] économise 33–34 % des MEB du triplet
uniforme initial, au prix de 1,24 Go de capacités temporaires échantillonnées à
32k. Le [complément après échange][post] descend ensuite à 17 199 233 MEB à 32k.
Ne pas attribuer à ces variantes les temps 50k ci-dessus. Préférer des lots
bornés et mesurer le compromis réemploi/mémoire. Les [recettes v8][families]
restent distinctes des entrées historiques v7.

## 5. Certificats sans MEB : portée, sélection et calcul exact

**Statut au 20 septembre : propositions démontrées et contrôlées sur modèles
bornés ; aucune intégration ni accélération du moteur HGP revendiquée.**
Le premier travail à éviter est la MEB initiale. Cela ne justifie pas une
recherche exhaustive dans les millions de boules du catalogue.

### 5.1 Une limite des ancres contenantes sur les premières demandes

Une ancre valide à l'ordre K d'une boule B contenant F, avec β(B)<a, fournit le
parent de F avant a : tous les K-sous-ensembles de I_B∪U_B sont connectés à β(B).
Ce certificat est correct même si B≠MEB(F). **Il est toutefois moins utile sur
la première demande régulière que sur une occurrence tardive.**

Soit a*(F) sa première demande comme facette stricte d'une coface Gabriel.
Dans le catalogue régulier directement programmé, |I_B∪U_B| vaut K ou K+1.
Si F⊆I_B∪U_B et β(B)<a*(F), alors B=MEB(F). Preuve : au cardinal K, les
populations sont égales ; au cardinal K+1, β(F)<β(B) ferait de F une facette
stricte demandée par cette coface Gabriel dès β(B), contradiction. L'égalité
des rayons implique l'égalité des boules par unicité de la MEB.

Avec J=nombre d'intrus stricts étrangers à F dans MEB(F), cela distingue :

| Première demande régulière | Opportunité exacte |
|---|---|
| J=0 | Population complète connue : semis déjà exploité en v7. |
| J=1 | Population complète privée d'un point **intérieur** : une jointure exacte avec les facettes réellement demandées peut éviter la MEB. |
| J≥2 | Aucune ancre contenante antérieure de ce catalogue ; une autre preuve de connexion ou la descente est nécessaire. |

Ce résultat ne couvre ni les extra-shells ni un catalogue enrichi d'ancres.
Pour J=1, ne pas matérialiser toutes les suppressions gratuitement : le modèle
ci-dessous génère 2 688 lignes d'index pour seulement 312 demandes J=1. Une
empreinte propose ; seule l'égalité complète des IDs autorise un hit.

### 5.2 Dilater une ancre, sans créer de nouvelle boule de catalogue

B a un centre c_B, un rayon carré b et une ancre admissible à K (§1).
Pour une facette **déjà certifiée stricte**, β(F)<a, poser

`theta(F,B) = max(b, max(x dans F) ||x-c_B||²)`.

**Certificat : b<a et theta(F,B)≤a suffisent pour retrouver le parent de F
avant a**, en normalisant l'ancre de B à cette coupe. Le test theta<a constitue
une première variante plus simple, sans exploiter le cas d'égalité.

Preuve : choisir un K-sous-ensemble G de I_B∪U_B. La boule dilatée contient
F∪G ; les échanges entre F et G restent donc sous son rayon. Si theta<a, tous
les liens sont strictement antérieurs. Si theta=a, G reste strictement intérieur
car b<a. Une MEB(F∪G) de rayon carré a aurait ses supports parmi F, ce qui
contredirait β(F)<a. Donc β(F∪G)<a également. Cette preuve ne requiert pas de
position générale. Elle suppose l'ancre valide, pas la seule présence d'une clé.

**theta est un majorant de connexion, jamais une date exacte de fusion.**
Ne créer ni nœud ni arête de dendrogramme à theta. Le cache doit conserver ce
seuil et la convention de coupe ; une réponse tardive n'est pas automatiquement
valable plus tôt. Pour les clés dédoublonnées, certifier le premier consommateur.
Ne pas assimiler ce certificat à un terminal de descente de niveau ≤β(F).

Sur le contre-exemple du §2, B=MEB(DE) a c_B=(2,0,0), b=1 ; pour F=AC,
theta=5<25/4. L'ancre de DE retrouve le bon parent sans MEB(AC), alors que
MEB(AC) est absente du catalogue à Kmax=2. Le repli exact reste indispensable :
ce certificat ne couvre pas toutes les facettes, même avec toutes les ancres.

### 5.3 Une fausse bonne sélection : l'ancre facile d'une autre facette du lot

Soient deux facettes strictes F,G d'une même coface Q, avec F∪G=Q et β(Q)=a.
**Si une ancienne boule B contient G, sa dilatation ne peut pas certifier F :
theta(F,B)>a.** Sinon le certificat précédent donnerait β(F∪G)<a, contradiction.
Cela reste vrai lorsque F et G sont déjà reliées globalement par d'autres sites.

Donc essayer systématiquement MEB(G), ou une ancre contenant G, pour résoudre
F est inutile. Une ancre obtenue après une descente de G et ne contenant plus G
n'est pas exclue par cet argument. Le modèle vérifie 6 861 cas d'ancres faciles
de facettes sœurs : zéro succès, dont **1 999 cas avec le même vrai parent**.
Cette obstruction est géométrique, pas un problème de choix des IDs.

### 5.4 Un proposeur réellement borné, mais pas encore un gain de temps

Modèle Python indépendant sur le même corpus de 34 petits nuages réguliers,
5 à 10 points, Kmax≤7 : 2 035 premières demandes distinctes après déduplication,
dont 1 682 J=0, 312 J=1 et 41 J≥2. Les catégories J servent au juge, **pas** à
une sélection qui calculerait d'abord la MEB qu'elle prétend éviter.

Une seule arborescence de boîtes de centres est construite sur les boules du
catalogue, partagée entre ordres ; chaque nœud porte un masque d'ordres et le
plus petit niveau du sous-arbre. Proximité du barycentre de F pour ordonner les
propositions, au plus **32 nœuds retirés de la file**, feuilles de **4 boules**,
puis au plus c candidates. Toute candidate passe le test exact du §5.2 ; budget
épuisé ou échec = retour au résolveur, jamais suppression de F. Les comparaisons
Gamma et les recherches exhaustives de référence sont exclusivement dans le juge.

Résultats après les seuls semis J=0, donc sur 353 demandes :

| c candidates au plus | Demandes certifiées / 353 | Dont J≥2 / 41 | Nœuds retirés, total | Boules lues en feuilles | Tests exacts de puissance |
|---|---:|---:|---:|---:|---:|
| 1 | 236 | 12 | 3 667 | 2 529 | 1 149 |
| 2 | 294 | 19 | 4 766 | 3 936 | 1 581 |
| 4 | 324 | 25 | 6 433 | 6 212 | 1 947 |

Le plafond exhaustif du certificat sur ces 41 demandes J≥2 est 27 succès :
25 sont retrouvés avec cette sélection bornée. Aucun mauvais parent accepté.
À c=4 : 569 préparations exactes de seuil, 2 722 distances centre/barycentre,
7 711 distances aux boîtes ; le plafond de 32 nœuds est atteint 8 fois.
**Les coûts ajoutés ne sont donc pas réduits aux 1 947 puissances.**

Préparation, séparée : 1 853 enregistrements d'ancres et 1 152 nœuds sur
l'ensemble des 34 nuages ; 9 733 visites pour les boîtes et 7 880 enregistrements
passés aux tris. Le prototype trie à chaque subdivision : O(B log² B) travail
et O(B) résidence pour B boules, sans borne B=O(n) acquise. À 50k, l'historique
v7 a déjà 21,5 millions de boules : **ne pas ajouter cet index global au produit
sur la foi de ce petit corpus**. Mesurer construction, mémoire et amortissement,
ou proposer depuis des structures déjà disponibles. Le budget de requête ne
borne pas le coût de préparer le catalogue. Aucun taux de succès LiDAR déduit.

### 5.5 Certificat entier : un seuil partagé, pas des carrés de centres flottants

Pour la clé certifiée `P_B(x)=A||x||²+v·x+C=A(||x-c_B||²-b)`, écrire
`a=n_a/d_a`, `b=n_b/d_b`, dénominateurs positifs. Préparer une fois par couple
(ancre, niveau consommateur) :

```
D = n_a*d_b - n_b*d_a       # exiger D>0 : ancre strictement antérieure
R = A*D
S = d_a*d_b
T = floor(R/S)
accepter si P_B(x) <= T pour TOUS les x de F
```

Avec β(F)<a établi par le constructeur, cela équivaut au certificat theta≤a.
La variante theta<a utilise `T=floor((R-1)/S)`. Pas de racine ni de centre
arrondi ; après préparation, chaque point ne paie qu'une puissance et une
comparaison entière. Un seuil peut être partagé entre représentants au même
niveau, sans déduire qu'ils ont le même parent.

**La préparation ne tient pas automatiquement en i128.** Les [niveaux v7][levels]
acceptent un numérateur 192 bits et un dénominateur signé 128 bits non réduit ;
avec A<2^68, R peut nécessiter jusqu'à 388 bits sous ces bornes conservatrices.
Utiliser une largeur démontrée, des réductions prouvées ou des entiers multiprécision,
pas réutiliser aveuglément le comparateur U320. Les puissances v7 restent i128
sur leur domaine validé. Avant tout rétrécissement de T, le borner par une borne
supérieure certifiée des puissances ou traiter le cas saturé séparément.

Contrôles nouveaux : 27 586 comparaisons polynomiales contre les distances
rationnelles directes, 2 600 paires acceptées contrôlées contre Gamma, Python
normal/`-O` identiques. Un **micro-exécutable C++ autonome**, sans moteur HGP,
compare la préparation en entiers 512 bits vérifiés au juge multiprécision :
82 761 cas (27 586×3 représentations et 3 stress de bornes arithmétiques),
496 566 contrôles, PASS. Les stress de format atteignent 384 bits ; ils ne sont
pas de nouveaux nuages géométriques. Aucun CTest HGP, sanitizer ou benchmark.
Une première génération de données de test dépassait le domaine du dénominateur ;
l'exposant de remise à l'échelle a été corrigé avant cette exécution, trace conservée.

[run5]: ../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/cpu_n50000_k5_s8.stdout
[run10]: ../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/cpu_n50000_k10_s8.stdout
[meb]: ../../morsehgp3D_v7/src/forest/anchor_meb.hpp
[static]: ../../morsehgp3D_v7/docs/RESOLUTION_STATIQUE_CPU_20260911.md
[post]: ../../morsehgp3D_v7/receipts/post_exchange_scale_20260911/README.md
[families]: ../bench/front_fixtures.hpp
[levels]: ../../morsehgp3D_v7/src/lanes/level.hpp

## 6. Critère de livraison et prochaine expérience

Un seul chemin CPU bout en bout, puis backends interchangeables. Le raccord
est accepté lorsque **le vrai census v8** alimente le constructeur et qu'un juge
Γ indépendant retrouve naissances, parents, couvertures et verticales aux coupes
ouvertes/fermées. La comparaison v7/v8 seule ne suffit pas.

Corpus minimal : exemple plan ci-dessus, E5, carré/coquilles, croissance sans
fusion, ancres inertes dans lots simples et groupés, descente à rayon constant,
boule présente au mauvais rang, K1 et K=n, IDs réordonnés/clairsemés, cache nul,
CPU1/4, panne après travail partiel. Réfuter explicitement l'omission d'une
attache, l'activation d'une ancre future et la substitution de cible ci-dessus.
Pour le §5, distinguer ancre du lot courant (interdite) et majorant theta=a
(admissible seulement avec les deux prémisses strictes). Ajouter facette non
stricte, inclusion incomplète, cache trop précoce, budget du proposeur nul/épuisé,
fractions équivalentes et débordements. Ne pas exiger les mêmes BallId entre
backends : comparer les parents à chaque coupe, pas les seules images inférieures.

Références à réutiliser :
[`full_ball_tower_gate.cpp`](../../morsehgp3D_v7/tests/full_ball_tower_gate.cpp),
[qualification du vrai census→FULL K10](../../morsehgp3D_v7/docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md),
[résolution statique](../../morsehgp3D_v7/docs/RESOLUTION_STATIQUE_CPU_20260911.md).
Les reçus natifs sont historiques, non rejoués par cet audit. Le contrôle
rationnel externe a été rejoué en Python normal et `-O` : 22 nuages,
2 343 facettes, 4 622 échanges dont 6 à rayon constant, 6 696 comparaisons
terminales sans désaccord ; les contre-tests d'intégration ci-dessus et
224 contrôles de conversion q2 passent également. Ces vérifications antérieures ne
qualifient ni le callback ni la tour v8 ; le nouveau micro-test C++ du §5.5
concerne seulement la préparation arithmétique du certificat.

**Ordre de travail :** (1) raccord CPU exact sur petits nuages ; (2) profil par
K et par famille aux tailles 8k/16k/32k, avec demandes/cache, MEB initiales versus
échanges, supports/puissances, visites d'intrus et histogramme des longueurs ;
(3) comparer semis, MEB spécialisées et ancre dilatée séparément ; ne pas ajouter
d’office un index global d’ancres ni les essais entre facettes sœurs du §5.3 ;
(4) paralléliser le travail restant. Séparer les
temps de proposition, résolution, histoire et sortie ; mesurer RSS et taille du
résultat. Ni 0,312 échange moyen ni un hit du juge ne prouvent un gain de temps.
Il faut retrouver les parents, pas réintroduire toutes les facettes silencieuses.

# Facettes silencieuses : ce que la v8 doit reprendre de la v7

19 septembre 2026. Audit ciblé de clôture, base `3e94c868abbb0fafac2b9062e72433f0793545cc`.
Complément coûts/raccourcis relu sur `0d965fae7e8a30f68723245c217c777cefd3a942`.
Référence moteur inchangée : `morsehgp3D_v7/src/forest/full_ball_tower.hpp`, blob Git
`5d8e9d91124d2a4cf073e67b8cc479f4e9dea897`. Aucun moteur modifié.
Lecture des sources et contrôles rationnels indépendants ; aucun CTest natif,
benchmark ou calcul GPU exécuté ici. `public_status=not_claimed`.

## Décision

**Reprendre le contrat de `full_ball_tower.hpp` et sa séparation statique
« facette → boule terminale → parent pré-lot ». Ne pas porter aveuglément son
stockage global ni revenir à « graphe Gabriel brut + Kruskal ».** Aucun défaut
nominal de rattachement n'a été identifié dans les chemins examinés ; cette
conclusion ne certifie ni le générateur, ni un futur backend, ni toute la v8.

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

## 5. Piste supplémentaire : une ancre contenante peut éviter la MEB

**Proposition à expérimenter, non implémentée ni chronométrée dans cet audit.**
Le résolveur doit identifier une composante avant a, pas nécessairement calculer
la MEB de F. Une boule B déjà certifiée peut répondre directement si :

1. même nuage/catalogue et ordre K, avec une ancre valide du bloc fermé de B ;
2. **F ⊆ C_B = I_B∪U_B**, vérifié pour tous les sites, par IDs ou puissances exactes
   `P_B(x) ≤ 0` ; une intersection partielle ne suffit pas ;
3. **β(B)<a**, et K appartient au domaine d'ancrage du §1.

**Justification.** Tous les K-sous-ensembles de C_B se relient par échanges d'un
site ; chaque coface de K+1 sites ainsi traversée reste dans B, donc naît au plus
tard à β(B). Leur composante fermée est celle représentée par l'ancre de B.
F appartient donc à cette composante avant a. Normaliser l'ancre à la coupe
consommatrice donne le parent exact. Il n'est pas nécessaire que B=MEB(F).

C'est un **certificat de composante**, pas un raccourci affirmant une égalité de
MEB. Il n'autorise pas à traiter une population partielle comme le semis MEB
`I∪U` du §3. Il n'utilise ni la seule couverture ponctuelle d'une composante ni
la boule du lot courant : β(B)=a ne suffit pas pour identifier les parents stricts.

Point de vigilance nouveau : une telle boule peut avoir β(B)>β(F). Un résultat
trouvé pour une demande tardive n'est donc pas réutilisable à une demande plus
précoce sans retester **β(B)<a**. Pour des requêtes dédoublonnées, tester le plus
petit a ; pour un cache, conserver le niveau d'admissibilité. Ne pas assimiler
cette cible à un terminal obtenu par descente dont le niveau est ≤β(F).

Une petite liste de boules déjà disponibles peut servir de **proposeur**. Avec
c propositions, les tests de confinement coûtent O(cK) puissances exactes, hors
sélection, lecture d'ancre et normalisation. Ne jamais chercher dans toutes les
boules ou développer leurs K-sous-ensembles pour obtenir cette liste. Un échec
ou un budget de propositions épuisé déclenche le résolveur exact, pas un refus
de facette. Le taux de succès et le bilan du travail ajouté/évité restent inconnus.

Le callback actuel contrôle des identités de compteurs supposant les anciennes
routes de résolution. Introduire des compteurs séparés de propositions,
`containing_anchor_hits`, rejets de date et MEB réellement évitées ; versionner
les identités de travail, sans inventer des appels MEB ou des hits historiques.
Le backend doit rester transactionnel et le juge comparer les parents, pas les
seuls BallId, qui peuvent légitimement changer.

**Contrôles exécutés dans le modèle rationnel externe :** 18 petits nuages,
2 359 couples facette/ancre contenus, dont 1 077 à boule strictement plus grande
que MEB(F), et 4 718 comparaisons de composantes sans désaccord. Contre-tests :
intersection partielle, ancre du même lot, réemploi tardif→précoce et support
parental devenu invalide. Les MEB directes K2/K3 concordent aussi sur 970 cas,
égalités et extrêmes u16 compris. Python normal et `-O` identiques. L'énumération
exhaustive des candidats est celle du juge borné, **pas une politique de
proposition ni un taux de succès attendu à grande taille**. Aucun test C++ natif.

[run5]: ../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/cpu_n50000_k5_s8.stdout
[run10]: ../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/cpu_n50000_k10_s8.stdout
[meb]: ../../morsehgp3D_v7/src/forest/anchor_meb.hpp
[static]: ../../morsehgp3D_v7/docs/RESOLUTION_STATIQUE_CPU_20260911.md
[post]: ../../morsehgp3D_v7/receipts/post_exchange_scale_20260911/README.md
[families]: ../bench/front_fixtures.hpp

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
Pour la piste du §5, ajouter inclusion incomplète, égalité β(B)=a et cache
accepté trop tôt ; ne pas exiger les mêmes cibles géométriques entre backends.

Références à réutiliser :
[`full_ball_tower_gate.cpp`](../../morsehgp3D_v7/tests/full_ball_tower_gate.cpp),
[qualification du vrai census→FULL K10](../../morsehgp3D_v7/docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md),
[résolution statique](../../morsehgp3D_v7/docs/RESOLUTION_STATIQUE_CPU_20260911.md).
Les reçus natifs sont historiques, non rejoués par cet audit. Le contrôle
rationnel externe a été rejoué en Python normal et `-O` : 22 nuages,
2 343 facettes, 4 622 échanges dont 6 à rayon constant, 6 696 comparaisons
terminales sans désaccord ; les contre-tests d'intégration ci-dessus et
224 contrôles de conversion q2 passent également. Cela ne vaut ni exécution
C++ ni qualification du callback ou de la tour v8.

**Ordre de travail :** (1) raccord CPU exact sur petits nuages ; (2) profil par
K et par famille aux tailles 8k/16k/32k, avec demandes/cache, MEB initiales versus
échanges, supports/puissances, visites d'intrus et histogramme des longueurs ;
(3) comparer une seule optimisation à la fois : MEB de facettes K=2/3, réemploi
certifié ou ancre contenante ; (4) paralléliser le travail restant. Séparer les
temps de proposition, résolution, histoire et sortie ; mesurer RSS et taille du
résultat. Ni 0,312 échange moyen ni un hit du juge ne prouvent un gain de temps.
Il faut retrouver les parents, pas réintroduire toutes les facettes silencieuses.

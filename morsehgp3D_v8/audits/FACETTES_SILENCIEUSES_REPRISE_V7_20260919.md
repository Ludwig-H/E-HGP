# Facettes silencieuses : ce que la v8 doit reprendre de la v7

19 septembre 2026. Audit ciblé de clôture, base `3e94c868abbb0fafac2b9062e72433f0793545cc`.
Référence : `morsehgp3D_v7/src/forest/full_ball_tower.hpp`, blob Git
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
appartenant à un support positif, poser F'=(F\{s})∪{z}. Avec β=rayon² :

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
`(nuage immuable, K, IDs de la facette)` et respecter le consommateur le plus
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
au compteur q2 v8. Le niveau est d2/4, sans flottants. Même rayon ne signifie
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

**Performance :** la capture historique 50k/K10 paie environ 390 s de FULL sur
419 s au total, avec 42 millions de MEB. C'est tout le constructeur, pas le seul
surcoût des facettes silencieuses. Reprendre la sémantique, pas cette latence.
Mesurer demandes/uniques, MEB, supports, intrus, pas à rayon constant, cache,
mémoire réelle et volume de sortie, puis seulement le gain CPU/GPU
([capture historique](../../morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md)).

## 4. Critère de livraison v8

Un seul chemin CPU bout en bout, puis backends interchangeables. Le raccord
est accepté lorsque **le vrai census v8** alimente le constructeur et qu'un juge
Γ indépendant retrouve naissances, parents, couvertures et verticales aux coupes
ouvertes/fermées. La comparaison v7/v8 seule ne suffit pas.

Corpus minimal : exemple plan ci-dessus, E5, carré/coquilles, croissance sans
fusion, ancres inertes dans lots simples et groupés, descente à rayon constant,
boule présente au mauvais rang, K1 et K=n, IDs réordonnés/clairsemés, cache nul,
CPU1/4, panne après travail partiel. Réfuter explicitement l'omission d'une
attache, l'activation d'une ancre future et la substitution de cible ci-dessus.

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

**Priorité : obtenir ce premier raccord exact sur petits nuages avant de
multiplier les optimisations du résolveur. Il faut retrouver les parents,
pas réintroduire un catalogue exhaustif de facettes silencieuses.**

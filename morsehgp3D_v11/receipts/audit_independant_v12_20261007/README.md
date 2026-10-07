# Contre-audit indépendant de la v11 pour le suivi de la v12

7 octobre 2026. Auteur : Codex, auditeur demandé par l'utilisateur, avec trois lectures déléguées indépendantes.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
mode=audit_independant_preparation_v12
public_status=not_claimed
source=33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae
moteur_gele=ac081a06f
GCP non utilisé
```

L'utilisateur demande de comprendre en profondeur la v11 avant d'auditer le développeur de la v12. La v11 étant
gelée, ce travail produit des constats, témoins et exigences de port. Il ne change aucun fichier du moteur, des
portes existantes ou de ses références. Il ne promeut aucune qualification publique.

## 1. Verdict et portée

La tour FULL reste une bonne référence différentielle **bornée par son domaine et ses refus**. Les lectures et
témoins de cette campagne n'établissent pas de résultat FULL faux. Son modèle mathématique est beaucoup mieux
fondé que les anciennes réductions de Gabriel, réfutées par E5. Cet acquis ne suffit pas à déclarer la v11 complète
pour toutes les entrées, rapide sur plusieurs séquences LiDAR, stable dans toutes ses sorties, ni supérieure à
HDBSCAN pour une sélection plate.

Le risque principal pour la v12 est de transférer une propriété au mauvais objet ou au mauvais périmètre : la
connexité FULL au squelette géométrique, la stabilité de la filtration à un propriétaire de point, la taille logique
d'un tampon à son allocation physique, une mesure de banc au CLI, ou un résultat de qualification à un autre pin.
Les contre-épreuves de cette campagne donnent des exemples concrets de chacun de ces glissements.

Le [premier audit géant](../../docs/AUDIT_GEANT_V11.md), la [passation](../../PASSATION.md) et l'[audit final](../../docs/AUDIT_FINAL_V11.md)
restent des sources de compréhension. Le présent reçu les complète et les corrige explicitement ; leurs phrases
ne sont pas reprises comme certificats. Le dossier v12 est apparu sur `main` pendant notre travail (`52a790443`) ;
le moteur examiné est resté celui de l'instantané ci-dessus.

## 2. Carte de compréhension de bout en bout

| Étage | Objet et garantie recherchée | Autorité et point de vigilance |
| --- | --- | --- |
| Entrée | Coordonnées entières u21, IDs arbitraires u32, sites privés en ordre de Morton | `cloud` valide et copie ; les doublons géométriques sont publiés puis refusés par la tour non pondérée. u24 est un autre profil à qualifier. |
| Géométrie | Boules de supports positifs d'au plus quatre sites, puissance et niveaux rationnels exacts | `num` garde les budgets de bits ; les filtres flottants autorisés ne décident qu'avec une marge prouvée, sinon repli entier. |
| Catalogue | Boules critiques et populations intérieures/coquilles, fenêtre d'ordres active | Boîtes de centres, filtres G1–G4, recensement et déduplication. Un rejet de candidat doit avoir une preuve de complétude. |
| Domaine et MEB | Représentants canoniques, cellules locales, descentes strictes | La plus petite boule doit être certifiée ; le coût de sa recherche et le nombre de descentes sont deux coûts distincts. |
| Tour FULL | Composantes de la région couverte par au moins k boules, pour tous les k, aux niveaux critiques | Kruskal atomique par plateau, multifusions, parents et verticales ; aucune binarisation arbitraire d'un plateau. |
| Supports SPv2 | Squelette de Kruskal : naissances et hyperarêtes utiles, un support S* par boule | Préserve les unions utiles spécifiées. Ni l'ensemble des supports, ni un maillage, ni nécessairement un graphe d'incidence acyclique. |
| Points | Qualification, pendaison en rayon, arbre laminaire de points | Choix supplémentaire H^r_(K+1), distinct du recouvrement naturel de Hartigan ; stabilité et pertinence ont leurs propres limites. |
| Plat | Condensation A, scores de persistance en rayon, EOM N-aire ou feuilles | Comparaisons certifiées de radicaux ; refus de l'appel si le budget exact est épuisé. L'exactitude de l'EOM ne prouve pas sa pertinence. |
| Publication | Produit possédé, dossier atomique, manifeste et empreintes | `Session` doit survivre au produit. Un double échec après publication rend `published_complete`, pas un succès. |
| Mesure | Latence d'une opération explicitement définie | CLI, banc FULL, froid, processus neuf réchauffé et processus résident sont des régimes différents. |

### 2.1 Définition et oracles

Pour un ensemble de sites distincts X, l'objet est $L_k(a)=\lbrace y : D_k(y)\leq a\rbrace$, où D_k est la distance
carrée au k-ième voisin. Pour chaque k, on conserve l'arbre de fusion de ses composantes et, entre k+1 et k,
l'application induite par l'inclusion. Les niveaux carrés sont des rationnels exacts ; les rayons en sont les racines
carrées. Une marge ou un score
exprimé en rayon n'est pas interchangeable avec la même expression en niveau carré.

L'étage A de `reference/` part de la définition exhaustive par parties et boules minimales. L'étage B suit davantage
la construction et réemploie des formules du moteur. L'accord A/B est utile précisément parce que leurs rôles sont
distincts ; un accord B/natif seul est moins indépendant. Les petits oracles réfutent les erreurs, sans fournir de
borne de complexité ni couvrir toutes les dégénérescences de grande taille.

La revue mathématique et les témoins supplémentaires sont dans [math/README.md](math/README.md). Ils distinguent
preuve écrite, identité algébrique refaite, test borné, conjecture et choix de produit. Ils ne prétendent pas être une
vérification formelle exhaustive de toutes les preuves du dépôt.

### 2.2 Architecture réellement livrée

Les douze modules sont séparés, les identifiants ont des domaines distincts, les résultats refusés ne contiennent
pas de valeur utilisable et les gros tableaux passent par un compte mémoire. Le CLI et l'API emploient une voie
CPU fixe, feuilles de 16. Les meilleures mesures GPU viennent d'une autre voie, située dans le banc. Le catalogue,
les forêts et la publication comportent encore des sections séquentielles ; le GPU ne calcule qu'une partie du
catalogue. Le rapport [native/README.md](native/README.md) relit les derniers ports et leur coût réel.

La v11 est une réécriture avec des ports explicites issus de la v10 et du raccord R2. La provenance par composant
est un acquis à garder. Un fichier décrit comme « neuf » n'est pas pour autant mathématiquement indépendant : les
oracles et la provenance doivent indiquer précisément les décisions partagées.

## 3. Constats sur mémoire, API et publication

### A12-MEM-01 — L'arrondi de classe est absent de la taille logique du budget

**P2, limite de contrat connue, témoin d'exécution nouveau ; ouvert pour le port.**

Dans `src/core/buffer.cpp:90–108`, le cache alloue `class_capacity(k)`. Dans `:155–169`, le compte réserve seulement
`bytes`. La documentation déclare bien que `used` et `peak` comptent les Buffer vivants, pas le RSS. Il n'y a donc
pas de débordement de ce compteur, mais `limit + cache_bytes` n'est pas une borne des blocs physiques.

Le [témoin Release](verification/cache_probe.cpp) intercepte uniquement la taille transmise à `operator new`
(sans modifier le moteur). Il réserve 262 145 octets avec `MemoryBudget(262145, 1)` :

| Grandeur | Octets |
| --- | ---: |
| Demandés et comptés vivants | 262 145 |
| Réellement demandés à l'allocateur pour ce bloc | 286 720 |
| Limite des blocs inactifs | 1 |
| Blocs inactifs pendant l'allocation | 0 |
| Écart physique non compté pour ce bloc vivant | 24 575 |

Après restitution, `used=0`, `idle=0`, un bloc a été refusé par le cache et rendu au système : pas de fuite établie.
La taille du bloc est une observation d'allocation, **pas une mesure de RSS**. Les métadonnées de l'allocateur et du
cache ne sont pas incluses dans ce témoin. La porte existante `tests/core/buffer_test.cpp:338–436` limite les blocs
inactifs, mais tous ses budgets de Buffer sont `kUnlimited`.

**Pour la v12 :** séparer octets utiles, capacité physique des blocs vivants, réserve inactive, métadonnées et
mémoire appareil/épinglée. Une réservation physique suit le bloc lors de son passage vivant → inactif → vivant,
sans disparaître du plafond global. Le compteur logique peut rester disponible comme diagnostic.

### A12-MEM-02 — L'empoisonnement du cache est désactivé avec Clang/ASan

**P2, défaut de protection nouveau, confirmé par prétraitement ; ouvert.**

`src/core/buffer.cpp:11,61,68` ne reconnaît que `__SANITIZE_ADDRESS__`. Avec GCC 13.3 et `-fsanitize=address`, le
corps de `poison` appelle `__asan_poison_memory_region`. Avec Clang 18.1.3 et la même option, il devient
`static_cast<void>(block); static_cast<void>(bytes);`. Le résultat est capturé dans
[cache_normal.json](verification/cache_normal.json), et est identique sous Python normal et `-O`.

Un bloc rendu au cache n'est pas libéré à l'allocateur. Sans l'empoisonnement explicite, la garantie annoncée de
détection d'un accès après restitution et d'un accès dans l'arrondi de capacité n'est donc pas fournie par ce chemin.
Ce constat porte sur le cache optionnel du banc ; `Session::make` ne l'active pas dans le CLI. Aucune exécution
ASan ni qualification Clang complète n'est revendiquée : c'est le **code prétraité** qui est vérifié.

**Pour la v12 :** reconnaître aussi la fonctionnalité sanitizer de Clang, puis exercer positivement la lecture d'un
bloc restitué et celle de la marge de capacité, sous les deux compilateurs, sur la cible de qualification autorisée.

### API et IO : lecture favorable, portée précise

La lecture de `api/session.cpp`, `api/api.hpp`, `api/compute.cpp`, `api/manifest.cpp`, des écrivains et de
`io/directory.cpp` confirme plusieurs protections utiles : copie du nuage, identité de Session avant publication,
validation de la forme de provenance, contrôle de fermeture du budget, manifeste écrit après les données,
`renameat2(RENAME_NOREPLACE)`, état distinct quand le retour arrière échoue. L'API déclare honnêtement que les
empreintes de provenance fournies par un client ne sont pas vérifiables depuis les seules métadonnées.

Les refus n'autorisent jamais à consommer un préfixe comme résultat. Le dossier `.pending` orphelin est une cause
de refus explicite. Une publication réussie puis retirée sur un échec de fin d'appel peut avoir été visible à un
lecteur, mais elle était complète : atomicité de visibilité et succès durable de l'appel sont deux garanties
distinctes. Aucun nouveau défaut de publication n'a été établi dans cette lecture.

### A12-NAT-01 — Une seule cohorte peut payer huit tampons de tri

**P2, défaut de capacité nouveau, reproduit nativement ; ouvert.** `forest_build.cpp:441–452` réserve un tampon
de la taille de la plus longue cohorte pour chaque worker, même quand une seule tranche contient du travail.
Sur 512 sites `(i,0,0)`, K2, les 511 naissances ont le même niveau. Le passage W1 → W8 augmente le pic de
572 320 octets, exactement `7 × 511 × sizeof(BirthRecord)` avec `sizeof(BirthRecord)=160`.

À budget fixé à 681 054 octets, W1 avec lookup dense et W8 avec lookup sparse réussissent ; W8 dense refuse
`memory_budget`. Les succès donnent les mêmes octets FULL. La capacité supplémentaire ne correspond à aucun
parallélisme de tri utilisable sur cette cohorte. Les six appels, compteurs et SHA sont dans
[COHORT_MEMORY_RELEASE_U21.json](native/COHORT_MEMORY_RELEASE_U21.json).

**Pour la v12 :** dimensionner les brouillons d'après les tâches réellement concurrentes, et pas uniquement le
nombre de fils. Conserver la transaction sur refus et l'identité canonique sur tous les succès. Ce refus ne constitue
pas une fausse tour.

### Autres corrections natives

- **A12-NAT-02, P2 :** le contexte CUDA contient un singleton mutable et change le pool CUDA de processus.
  Cette propriété diffère du contrat CPU sans état global mutable ; aucune course CUDA n'est établie par la lecture.
- **A12-NAT-03, P3 :** le `GlobalIndex` actuel est radix Morton, borné en profondeur par les bits, et non un arbre
  équilibré en cardinalité comme l'annoncent encore son en-tête et la provenance.
- **A12-NAT-04, P3 :** la MEB balaie toutes les distances de paires, mais présente une seule boule q2, celle du
  diamètre. Puis elle énumère q3 et q4 jusqu'au premier certificat. Le récit du premier audit surestime donc les
  présentations q2. Le coût des présentations exactes q3/q4 reste une piste de mesure légitime.

Les preuves de lecture, les lignes et les conditions de port figurent dans [le rapport natif](native/README.md).

## 4. Ce que les mesures permettent réellement de dire

Le détail recalculé est dans [evidence/RAPPORT.md](evidence/RAPPORT.md), avec scripts et entrées synthétiques des
contre-épreuves. Les temps historiques restent des temps G4 ; aucune mesure locale de cette campagne ne sert à
revendiquer un gain de vitesse.

- Le contrat 100 ms n'est pas tenu. Les valeurs chaudes historiques proches de 212–255 ms en K5 et de
  1,3–1,8 s en K10 ne sont pas des latences du CLI complet.
- Le banc `gpu_ab` réutilise processus, Pool et contexte, mais prépare de nouveau le nuage hors du chrono FULL.
  `sorties_g4` démarre un nouveau processus à chaque prise dite chaude. Ces deux séries ne décrivent pas la même
  résidence des ressources.
- Les trois trames de référence sont trois trames de la séquence 08, pas plusieurs séquences. Elles ne ferment
  ni le domaine général LiDAR ni une garantie de latence par trame.
- Les comparaisons v10/v11 issues de captures distinctes ne sont pas un A/B apparié. Les coûts de préparation,
  quantification, masque sans sol, sortie et latence d'initialisation doivent être déclarés avant comparaison.

### A12-EVD-01 — 3 303 tentatives, 3 285 sorties munies d'une empreinte

**P2, correction nouvelle de la synthèse du premier audit géant.** Les 81 rapports contiennent 3 303 tentatives,
dont 18 refus sans SHA. Les sorties disponibles sont donc 3 285, soit, par trame, 802 K5 et 293 K10. Les 18 refus
appartiennent à `claudegpu2`, pin `d5b1d0179` : 15 prises froides et 3 processus chauds, code 3, garde historique
erronée « feuilles ≤ sites ». Ils étaient conservés et expliqués par le reçu d'origine. Ce ne sont pas des
divergences géométriques. Aucun désaccord d'empreinte n'est relevé dans les sorties comparables recalculées.

### A12-EVD-02 — Des juges d'adoption continuent après une non-conformité

**P1 de validation, nouveau et reproduit ; ouvert avant tout port du juge.** Sept juges ad hoc du 7 octobre peuvent
imprimer une décision d'adoption et sortir avec le code 0 après lecture d'un rapport dont le verdict n'est pas
`conforme`. Les contre-épreuves synthétiques sont conservées. Les mêmes scripts ne font pas respecter le nombre
de prises attendu. Cela établit une insuffisance de leurs contrôles d'adoption, **pas l'existence d'une adoption
historique effectivement erronée**. Les jeux d'injection sont séparés des vrais reçus.

Le runner de matrice a ses propres protections : nos sondes vide, absence et contradiction JUnit ne les contournent
pas. Il serait faux de transférer le constat des juges de leviers à toute l'infrastructure de qualification.

### A12-EVD-03 et A12-EVD-04 — Régime de mesure et identité du binaire

**P2 de contrat, précision nouvelle :** une prise dite chaude de `sorties_g4` est encore un processus neuf ; les
passes chaudes de `gpu_ab` vivent dans un même processus. Leurs champs ne sont pas substituables. La dernière
passe de chaque processus chaud est seule vidée : 594 SHA chauds ne signifient pas un dump par passe.

**P2 de provenance, risque reproduit :** `gpu_ab.build` réutilise le fichier `b_cuda/mhgp11_full_bench` existant
sans vérifier son lien aux sources `new`. La sonde obtient cette réutilisation même avec une source inexistante.
Les sessions gardées utilisent des dossiers neufs : aucun reçu G4 n'est invalidé sur ce seul témoin. Pour la v12,
lier le cache de build à la source, aux options et au compilateur, ou repartir d'un artefact neuf.

Le recomptage a également vérifié 219 entrées SHA de quatre manifestes sans divergence et confirmé les gains de
meilleur bloc sur 64 scènes synthétiques par K. Le tableau et ses limites figurent dans le rapport de preuve ;
ce contrôle d'intégrité ne transforme pas les statistiques de meilleur bloc en sélection autonome.

## 5. Preuves, limites et comparaison à HDBSCAN

Le succès d'un test répond à une question attachée à un binaire, un profil, un compilateur et un jeu d'entrées. La
dernière qualification dite complète est une union de sessions dont les pins et modules doivent être distingués.
Le gel exact n'a pas hérité automatiquement des 3 695 portes ordinaires ni des 485 mutants historiques. Les
captures postérieures apportent leurs garanties propres ; aucune ligne agrégée ne remplace cette traçabilité.

La thèse fonde le lien entre multicoverture, graphe de Čech et composantes de Hartigan. E5 réfute la récupération
générale des ensembles de points par le seul graphe de Gabriel. Les démonstrations HDBSCAN synthétiques donnent
des séparations intéressantes, mais la comparaison de hiérarchies et celle de partitions plates sont des tâches
différentes. Une IoU oracle mesure ce que l'arbre pourrait fournir sous une sélection informée ; ce n'est pas le
score d'un algorithme autonome de sélection.

La contrelecture mathématique donne trois précisions utiles au développeur :

- **MATH-02 :** au-delà de la marge de durée requise, l'entrelacement associe une trace ou une chaîne à un nœud ;
  il ne donne pas automatiquement un identifiant de nœud stable. La stabilité sous déplacement apparié des mêmes
  sites ne couvre pas une insertion ou une déduplication arbitraire.
- **MATH-03 :** sur la fixture `tetraedre_k5` de sept sites, les branches sont reconstruites par l'oracle A, puis
  le sélecteur abstrait conserve trois hyperarêtes et cinq unions utiles. Le graphe d'incidence des hyperarêtes
  conservées possède un cycle. Le contrat de Kruskal hypergraphique est respecté ; l'aval ne doit pas supposer
  qu'il reçoit un arbre biparti. Ce témoin n'est pas une exécution native de SPv2.
- **MATH-04 :** `growth_ABCZ` rappelle qu'une boule interne non gardée par le squelette peut agrandir la population
  d'une composante. Pour reconstruire sa couverture, les seules naissances et fusions ne suffisent pas.

Les huit questions sur le polyèdre reçoivent une réponse séparée dans le [rapport mathématique](math/README.md#4-réponses-aux-huit-questions-polyèdre-laissées-ouvertes).
En particulier, la confirmation exacte des deux trous sur les six points porte l'homologie H1 de la région de
multicoverture ; FULL ne représente que H0. Le certificat de forme κ et la réduction causale restent des travaux
aval avec leurs propres hypothèses, coût et qualification.

Les applications aval ont aussi des objets différents : clustering recouvrant, attention sur arbre laminaire,
polyèdre de haute densité, géométrie de rendu. Le carrier, les incidences, les masses et la règle de coupe sont à
déclarer. Un squelette de connectivité ne fournit pas à lui seul une réalisation géométrique stable.

## 6. Vérification propre à cette campagne

La compilation Release u21 réussit. **920 portes passent, zéro échoue ; une sentinelle LiDAR est sautée faute de
données**, sur 921 portes sélectionnées. Les 1 080 coupes supplémentaires et les témoins de cache, cohorte,
index et juges complètent ce rejeu. Ils ne constituent pas une matrice G4 ni une nouvelle mesure de contrat.

Les relevés définitifs d'exécution et leurs limites figurent dans [verification/README.md](verification/README.md).
Le manifeste des [644 fichiers de construction et de contrôle](verification/source_manifest.json) fixe leur SHA-256
au snapshot lu. Les worktrees sont séparés, les builds sont neufs dans `/tmp`, la charge de compilation et de CTest
est limitée à deux tâches, et aucune session GCP n'a été utilisée. Un premier essai du témoin cache a omis le profil
de compilation : [échec conservé](verification/initial_attempt.json), corrigé dans le script sans changement produit.

## 7. Exigences que j'appliquerai comme auditeur de la v12

1. **Un contrat observable par sortie.** Entrée, profil, duplications, K, résultat demandé, limites de capacité,
   régime froid/résident et frontière exacte du chrono sont enregistrés. Les décisions utilisateur priment les
   recommandations du présent audit.
2. **Un registre des preuves attaché aux objets.** Noms uniques pour les lemmes ; distinguer FULL, SPv2, points,
   plat et polyèdre ; porter E5, les plateaux et les témoins de stabilité avant l'optimisation concernée.
3. **Des ports explicites et rejugés.** Source et adaptation par composant ; accord avec l'oracle A sur petites
   entrées, canonique v11 sur mêmes coordonnées, et contrôle de complétude distinct des seules empreintes.
4. **Un contrat mémoire physique.** Capacités, réserves, coexistences, pilotes concurrents, refus et restitution ;
   tests à limite finie avec cache, plusieurs tailles de cohortes et plusieurs nombres de fils.
5. **Une adoption qui échoue en cas d'ambiguïté.** Verdict, pin, empreintes, registres, données, nombre de prises,
   régime et règle d'adoption sont contrôlés par le même juge. Une absence de résultat reste une absence.
6. **Le chemin mesuré est celui livré.** Les microbancs servent à isoler les coûts ; ils sont suivis du produit
   complet sur les trames et séquences requises, avec préparation et sortie explicitement comptées ou exclues.
7. **Une clôture locale de chaque constat.** Fixture minimale, correction, pin de la correction, porte jouée et
   résultat brut. Une réponse textuelle, un futur plan ou un vert sur un autre pin ne clôt pas un constat.

Les objectifs d'étage et architectures proposés pour la v12 restent des hypothèses à mesurer. Le dossier permet
de commencer l'audit du développeur avec des preuves et un vocabulaire communs ; il ne valide pas par avance sa
future implantation.

## 8. Raccord à l'ouverture v12 survenue pendant cet audit

Avant publication, `main` a reçu `f31845d16`, qui ouvre formellement la v12 et consigne les décisions prises sur
délégation utilisateur. README, DECISIONS, ARCHITECTURE, PLAN et MESURE de la v12 ont été lus. Le contrat est
désormais fixé en Session résidente, latence par trame, FULL K1..5 en mémoire avec verticales sur G4, plusieurs
séquences ; K10 reste un objectif. u18 est abandonné ; u21/u24/u32, petits nuages et LiDAR multi-millions sont visés
avec les qualifications et mesures propres annoncées. Cette évolution ne change pas les sources v11 jugées ici.

Les questions de régime des rapports historiques sont donc des questions **désormais tranchées**, non des demandes
à reformuler à l'utilisateur. Les réponses mathématiques et les constats de capacité restent à traiter lors des
ports. Aucun budget numérique v11 n'est transféré automatiquement à u32, même avec un repère local : chaque entrée
effective du prédicat doit être couverte par le certificat d'étendue. La note vivante du suivi est déposée dans
`morsehgp3D_v12/audits/AUDIT_CODEX_20261007.md`.

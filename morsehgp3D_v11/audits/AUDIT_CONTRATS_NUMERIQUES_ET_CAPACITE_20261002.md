# Reprise développeur v11 — audit courant et écart avec la v10

3 octobre 2026. Code relu : **`70e494777c9466c6ee374358ce38c5d90bb6e7dd`**.
Je passe côté développeur sur instruction de l’utilisateur. Cette note remplace
mon ancien état du 2 octobre ; les preuves closes restent dans `receipts/`.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**FULL est maintenant implémenté et qualifié sur les petites fixtures.**
La dernière campagne entièrement close ici est
[reuse1, source ae817d09e](../receipts/full_regular_vertical_20261003/reuse1/README.md) :
3339/3339 portes, supplément ASan18 299/299, 292 mutants jugés, 29/29 FULL K5.
Les derniers ports de HEAD sont encore à qualifier. Les performances
annoncées ci-dessous exigent des options explicites, inactives par défaut. Le contrat FULL200ms reste ouvert.

## Ce qui a réellement été construit

| Couche | État et portée de l’audit |
|---|---|
| core/cloud/sched | Outcome/Result, propriétaires privés, réservations Buffer, conservation des IDs et poids ; Pool synchrone, slots privés et joins ; erreurs et mémoire jugées dans la matrice. Les piles OS et petits contrôles de threads ne sont pas des Buffer. |
| num | Budgets d’intermédiaires B18/21/24 ; q1/q2/q4 natifs, q3 avec certificat i128 ou essai contrôlé puis Wide ; orientation certifiée séparément. Level rationnel non réduit et comparaison exacte. Aucun filtre flottant F2/F3/F4/F6 dans le moteur actuel. |
| catalogue | Listes K-certifiées, domination stricte, boîtes fermées pour les rejets et ownership séparé, J2, coquilles complètes, S* global ; cache de droites, tri indirect, frontière adaptative, assemblage parallèle et une passe déjà portés. Graphe de paires préparé, qualification en cours. |
| index/MEB/descente | Index global possédant Cloud ; census saturé ou I/U complet ; espaces réutilisés par lane. Diamètre exact, premier support positif contenant toute la partie, mémo avant MEB, dates initiale et terminale distinctes. |
| forêts FULL | Naissances, incidences régulières/étendues, multifusions atomiques, parents et verticales fermées ; lots parallèles privés, pilote DSU, balayages d’ancêtres, lookup dense et réemploi des graines verticales déjà portés. |
| points/head/api | Modules produit encore absents ; définition core/cover et fixtures présentes dans la référence. Ni hiérarchie de points native ni comparaison effective à HDBSCAN. |
| preuves/outillage | GCC, ASan/UBSan, TSan, profils et poison sur G4 ; Clang absent. Intention, argv, entrées, sorties, mutants et fermetures conservés. Échecs de harnais distincts des défauts géométriques. |

Lecture favorable des chemins critiques, avec les obligations mathématiques
[maintenues dans la seconde note](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Ce bilan ne signifie pas que chaque ligne possède une preuve indépendante.
Les oracles bornés et les reçus qualifient leur source, pas automatiquement HEAD.

## Comparaison à périmètre utile

Même grille1mm, mêmes XYZ hachés et mêmes sous-nuages entiers sans sol de la
séquence08. G4 CPU, K=1..5, 48 travailleurs, sans projection de points.
V10 : source777406b82, u18, troisième passe chaude ; v11 : ae817d09e, u21,
mode2047, processus neuf et une mesure par case. Aucun ratio statistique apparié.

| Trame / sites | V10 catalogue + tour | V11 FULL | Rapport observé |
|---|---:|---:|---:|
| 08/000000 / 39885 | 252,0ms | 1463,154ms | ×5,81 |
| 08/000100 / 35551 | 204,2ms | 1154,972ms | ×5,66 |
| 08/000200 / 45845 | 253,6ms | 1514,543ms | ×5,97 |

Références : [synthèse v10 §3](../docs/AUDIT_V10_SYNTHESE.md),
[mesures closes v11](../receipts/full_regular_vertical_20261003/reuse1/metrics.json).
La première passe v10/ng00 vaut259,8ms et la dernière252,0ms : l’échauffement
observé ne rend pas compte d’un facteur six. Le différentiel canonique
v10/v11 **sur LiDAR entier** reste à faire ; 42 petites comparaisons FULL existent.
L’égalité des entrées et nombres de boules ne prouve pas l’égalité de toute la tour.
Le juge FULL public v10 acceptait certaines forêts erronées ; ses chronos
ne sont donc ni un oracle de complétude ni une qualification héritée v11.

Sur ng00/u21/mode2047 :

| Coût | V10 | V11 |
|---|---:|---:|
| Catalogue / domaine catalogue+lookup | 163,5ms | 804,679ms |
| Tour / forêts + verticales | 88,5ms | 658,042ms |
| Tri catalogue | 11,2ms | 57,804ms |
| Assemblage catalogue | 16,9ms | 4,660ms |

Ces sous-phases ont des frontières différentes : le domaine v11 inclut le
lookup ; la tour v10 inclut son index. L’index v11 vaut0,412ms, mesuré à part.
La géométrie une passe prend659,331ms. Les plateaux v11 prennent451,315ms,
classification68,736ms, naissances42,391ms et verticales80,846ms.
Les durées de dispatch incluent du travail parallèle : ne pas les additionner
à leurs propres sous-intervalles ni soustraire les sommes de tâches au mur.

## Pourquoi c’est encore plus lent

1. **Le travail combinatoire n’est pas encore aussi partagé.** La v10 emploie
   des listes de préfixes vivants aux seuils de l’arité suivante et des calculs
   conjoints de faces. La v11 prolonge encore certains préfixes déjà condamnés
   et résout indépendamment les faces régulières. Le problème concerne le
   nombre de calculs avant leur prix unitaire.
2. **La publication série répète les recherches de racines.** Dans
   `regular_cell`, find(seed)/touch(root) précède unite, qui refait deux
   find et touch. Publication régulière :160,629ms sur ng00/mode2047.
   Pour3,621M traces et1,306M cellules,4,629M appels find redondants sont
   éliminables si first reste la racine courante. Aucun gain temporel acquis.
3. **Le tri fait systématiquement des produits croisés Wide.** La v10 trie
   d’abord par clé approchée puis répare exactement les bandes ambiguës ;
   `num::compare(Level)` et le tri v11 restent entièrement exacts.
   F3/F4 sont autorisés et formulés dans l’architecture, mais non implémentés.
   La borne d’erreur et les égalités doivent être portées, pas seulement le tri v10.
4. **Les MEB et census restent nombreux malgré les caches.** Sur ng00/mode2047,
   4,686M étapes de descente, 15,697M présentations MEB, 29,628M paires de diamètre
   et 23,882M tests de points census. Le réemploi des verticales a déjà évité
   857771 descentes sur857891 : le reproposer ne traite pas le résidu des plateaux.
5. **La granularité et les barrières restent à mesurer.** W1/W8/W48 valent
   15,307/2,512/1,463s sur ng00. Cela ne suffit pas à distinguer déséquilibre,
   coût séquentiel et synchronisation. Le nombre de jobs et les maxima par
   phase doivent accompagner une ablation, plutôt qu’augmenter aveuglément W.
6. **Le surcoût du profil actuel reste à attribuer.** Les temps LiDAR u24
   proches des u21 utilisent exactement les mêmes coordonnées1mm ; ils
   n’isolent pas le passage u18→u21. Les anciens bancs de profils mono
   observaient un surcoût18→21 d’environ6–11%, sans transfert au code actuel. Certaines opérations élargissent néanmoins les types et le stockage ; il faut un
   A/B u18/u21/u24 récent pour attribuer leur coût. Aucune approximation de
   coordonnées ni suppression de points frontière n’est nécessaire aux leviers ci-dessus.

La validation Python de29 sorties prend226,133s, contre53,437s de FULL natif
cumulé : elle allonge la campagne, pas l’appel moteur. Assemblage à4,7ms,
Cloud à1ms ou index à0,4ms ne sont pas les premiers verrous.

## Premier chantier développeur

- **P0 catalogue : domination avant les droites J2 et garde du seuil descendant.**
  Après le certificat de paire/clique, si d=|Dom(S)|>K+1−q, rejeter S avant
  les calculs de droites. Après le traitement propre de S, prolonger
  seulement si d≤K−q. L’union des
  dominateurs est monotone ; le seuil diminue. Ne jamais couper q4 parce
  qu’un triplet est obtus. Le test de paires garde sa position pour
  conserver le contrat des compteurs du graphe. Les nouvelles coupes doivent avoir leurs compteurs
  et comparaisons propres ; fixture et preuve à conserver avec la tranche.
- **P0 tour : union de racines déjà trouvées et touchées.** Conserver
  first=min(first,root) après la fusion, les chaînes des anciennes
  composantes et les compteurs ; qualifier la multifusion entière,
  y compris plusieurs cellules au même niveau et graines dupliquées.
- **Étude tour : partage des calculs de faces et suffixes mémo.** Un
  candidat strict peut servir plusieurs faces, mais le test des points
  de l’union peut coûter davantage. Mesurer avant de porter ce changement ;
  dates et supports canoniques restent individuels. Voir la seconde note.
- **P1 numérique : clé F3/F4 du tri indirect**, calculée une fois par niveau,
  repli exact aux zéros/égalités/bandes ambiguës et preuve aux quatre arrondis.
  Le maximum gagnable sur le seul tri doit rester comparé au temps FULL.
- **P1 protocole : A/B v10/v11 froid et répété**, mêmes entrées, leaf16,
  grille/profil/options explicités, sorties canoniques entières et coûts
  physiques séparés des compteurs logiques. Pas de session G4 concurrente.

Les ports HEAD poids q4, contacts de support et MEB différée attendent leur
qualification G4. Graph4, source91890b457, a perdu son contrôleur local pendant la reprise :
aucun DONE, reçu de campagne ou résultat rapatrié. Reprise gardée à06:57:50UTC :
génération certifiée déjà TERMINATED, clé privée supprimée, verrou libéré.
La suppression OS Login rend1 car la clé est déjà absente ; une lecture
du profil OS Login à07:02:45UTC confirme cette absence.
Aucune nouvelle VM démarrée ; résultats natifs de graph4 non qualifiés ici. Son premier calendrier leaf16
annonce1,043–1,407s ; leaf8 régresse à3,9–5,0s. Ces valeurs ne remplacent pas reuse1.
L’expérience compilateur x86-64-v3 est préparée, pas mesurée.

## Capacité et preuve restante

Pic Buffer ng00/u21/mode2047 :344267712octets, dont7657920 pour48 workspaces
census et5226784 pour le cache vertical. Ce n’est pas RSS. FULL unitaire
refuse les multiplicités malgré leur conservation dans Cloud. Taille de
coquille, nombre de boules et sorties peuvent dépasser une borne linéaire
universelle ; ces quelques trames ne qualifient pas les dizaines de millions.
K10, plusieurs séquences, GPU, projection et tête restent ouverts.

Reproductibilité : le paquet source LIVE de reuse1 pointe vers un `/tmp`
disparu ; son hash déclaré n’est pas un rehachage. La relecture autonome
a vérifié l’archive de résultats et121 membres,30 blobs Git et les
provenances conservées. Elle ne relance ni le lecteur LIVE d’origine ni
les binaires absents. Deux erreurs de contrôle de cet audit sont gardées :
paquet absent ; tentative erronée de reconstruire I/U depuis(p,q) sans
les coquilles étendues. Aucun défaut natif n’en découle.

[Preuves compactes et modèles](../receipts/developpement_20261003/reprise_performance/README.md).
Contrôles de cette reprise : relecture statique et reçus ; style345 fichiers
et cinq fixtures de projection passent en Python normal/−O. Les petits
modèles Fraction testent la coupe de catalogue et le partage des faces,
sans importer le produit ni revendiquer un gain natif. Aucun build/test natif
local, aucune nouvelle campagne G4 ni qualification produit ajoutée ; seule
la fermeture gardée de la session orpheline a été exécutée.

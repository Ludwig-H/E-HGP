# Audit initial pour Morse HGP 3D v9 — v8, héritage et voie sous-quadratique

22 septembre 2026. Auteur : auditeur v9. Périmètre : lecture du code, des
preuves et des reçus ; aucune modification du moteur, aucune mesure GCP
nouvelle. Ce rapport a été ouvert avant la création de `morsehgp3D_v9/` ;
il conserve ici sa provenance historique. La contrelecture courante vit
dans [l'audit v9](../../morsehgp3D_v9/audits/CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md).

**Décision utilisateur postérieure à cette première lecture :** la grille
entière **1 mm est le profil principal v9** ; le float32 original devient
un objectif secondaire. Le régime sans sol est le **premier jalon temps**
v9 ; la trame brute entière avec sol garde une ligne de qualification
distincte. Les mentions ci-dessous de « float32 par défaut »
décrivent le contrat v8/AGENTS alors en vigueur, pas une condition encore
prioritaire pour le jalon G4 v9. Tour FULL, plusieurs séquences et seuils
temporels restent à qualifier dans leurs régimes déclarés.

## 1. Verdict et cible exacte

La v8 a un **producteur de candidats q3/q4 en arithmétique entière avec
portes différentielles bornées**, des filtres WSPD et un traitement multi-CPU.
Cette qualification locale ne prouve ni la complétude de toute grande trame
ni une borne globale de coût. La v8 ne livre pas encore la tour HGP FULL,
un catalogue canonique global ou un backend GPU.
Sur la première trame sans sol entière à 1 mm, K5 et huit workers, son seul
flux q3/q4 prend **104,63 s mur / 812,82 CPU·s**. Les contrats de tour en
1 s, puis 100 ms, ne sont donc pas proches sur ce chemin actuel.

Le premier jalon temporel v9 porte sur une trame SemanticKITTI **sans sol
entière** : grille entière isotrope de 1 mm, masque calculé sur la trame
brute entière, IDs et mots float32 bruts conservés en provenance ;
segmentation, calcul HGP et total mesurés séparément. La trame **brute
entière avec sol** reste un régime distinct à qualifier ensuite, sur
plusieurs scènes et séquences ; le float32 original est secondaire.
Sur G4, l'objectif porte sur **toute** la tour K1..10
en moins d'une seconde ; repli sur toute K1..5, puis objectif 100 ms.
La taille historique « 50k » et un flux q3/q4 ne sont pas des substituts.
Sources : [contrat historique v8](../../morsehgp3D_v8/docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md) ;
choix du profil et de l'ordre des régimes v9 précisés par l'utilisateur,
puis documentés dans le plan v9 publié à `c399808e`.

Il est mathématiquement impossible de promettre une **sortie FULL explicite**
toujours sous-quadratique en n : la v7 donne une famille régulière de
N=2m points avec au moins m² feuilles dès K2. La cible constructive pour
v9 est donc : sur les régimes LiDAR visés, établir par mesures appariées
un travail effectivement sous-quadratique, et dans tous les cas viser un
coût proche du volume indispensable de sortie. Une représentation implicite
serait un autre contrat, à spécifier avec ses requêtes et son expansion.
Voir [preuve de sortie v7](../../morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md).

## 2. Périmètre de cette relecture et état des preuves

La base suivie est `a74e90f2`, sur `main`. La reprise u18/atlas du
22 septembre est encore dans un worktree modifié et partiellement indexé :
ce rapport ne l'assimile pas à un commit publié. Au point de lecture, les
sources pertinentes portent les empreintes SHA-256 suivantes :

Pendant l'audit, `origin/main` a avancé jusqu'à `12294241` par **13 commits
de l'auditeur indépendant**, sans changement de ces sources moteur. Le
worktree local chargé d'autres modifications n'a pas été fusionné ni rebâti.
Les résultats complémentaires de ces commits sont intégrés ci-dessous avec
leur portée de prototype ; ils ne sont pas des gains du pipeline courant.

| Source | SHA-256 |
| --- | --- |
| `src/pipeline/wspd_q34.cpp` | `79ae04fe505671ab2ebbb15d7b2b546a9beb4a3fd427f8df0af40670b318084e` |
| `src/lanes/q4_local.cpp` | `43851f2087ebd3e21757b787a72a4423c82d6ee7796d04983b0125e414dd5fc0` |
| `src/lanes/q4_local_partition.cpp` | `0e776fae11f2218e69dcb78767b0c37469fb604b528a709a55115edf2d94e962` |
| `src/lanes/q3_ball_census.cpp` | `1f72e612d7d19a66ce69a055fffbd9938a36930c668a5fda56d92ce0bcdfd504` |

La [première mesure 1 mm](../../morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/BASELINE.only.json)
(`sha256=69214e8f541b44cd20047a45291e4a7403108f63814c8adfb8698763bb53f3a6`,
versionnée depuis `3f0d188f`)
est un reçu v2 relu LIVE en Python normal et `-O`, à une ligne, sans paire
W1/W8 ni grand oracle de sortie. Sa sonde fut compilée pendant la première
qualification Release. Cette qualification R1 garde 134 tests exécutés
verts sur 136 ; deux lecteurs de mutations échouent pour leurs inventaires
et messages attendus. La capture SAN R1 a été interrompue explicitement.

Les reconstructions R2 ont **136/136 CTests exécutés verts en Release** et
**131/131 sous Clang ASan/UBSan**, mais leurs `COMPLETION.json` restent
`failed`. Le lecteur `bench/run_u18_resume_checks.py` ne reconnaît, ligne
177, que l'élément XML `<skipped>` ; CTest encode les exclusions comme
`<testcase status="disabled">` (par exemple lignes 987, 993 et 1063 du
`release_r2/CTEST.xml`). Le lecteur juge donc à tort l'inventaire de
tests désactivés différent. C'est un défaut de preuve à réparer par une
**nouvelle capture fraîche**, pas une raison de réétiqueter R2 PASS.
Les trois mutations de saturation ne deviennent pas une qualification
LIVE par le seul succès CTest.

Un **raccord q3 global float32 est en chantier non suivi** au moment de la
lecture : `src/wspd/float32_front.*`, `src/lanes/float32_q3_owned.*` et
`src/pipeline/float32_q3_global.*` existent, avec un juge Fraction dans
`tests/float32_q3_global_test.py`. L'entrée annonce un index unique, un
front en flux, un workspace réutilisé et des supports q3 avec clé/coquille.
Son corps développe pourtant chaque produit résiduel par les deux boucles
`for` sur A×B, puis traite les arêtes l'une après l'autre ; il n'a ni q4,
ni catalogue, ni workers, ni reçu de qualification fermé dans
`receipts/float32_q3_global_20260921/`. Ce code récent n'hérite pas des
preuves des primitives float32. Voir
[`float32_q3_global.cpp`](../../morsehgp3D_v8/src/pipeline/float32_q3_global.cpp)
et le [plan de raccord](../../morsehgp3D_v8/docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md).

Les diagnostics `--scale` R2 n'ont pas été atteints par ce runner après
son échec de lecture CTest. Leur fixture prévue concerne **une arête
fournie dont la sortie q4 est vide** : même une future capture réussie ne
mesurera pas la croissance du générateur global. Voir la
[note de reprise](../../morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md)
et les deux `COMPLETION.json` R2, versionnés depuis `3f0d188f`.
Cette précision doit être communiquée au
développeur avant toute annonce de qualification.

## 3. Cartographie de l'algorithme réel v8

1. Un index immuable porte les points entiers et leurs boîtes. Le front WSPD
   produit des rectangles A×B avec trois masques de voie indépendants.
   Les seuils de rejet pour une tour jusqu'à K sont K, K−1, K−2 témoins
   stricts pour q2, q3, q4. Un contact n'est pas un témoin.
2. La voie q2 possède un census global et des optimisations de blocs,
   Pool et partage CPU. Elle n'est pas intégrée à une tour v8 FULL.
3. Pour q3/q4, un filtre de rectangle tente le rejet ; tout produit
   résiduel est ensuite développé en paires de points. Une paire engendre
   son propre cover, ses graines q3 et son atlas q4. Le découpage de
   rectangles en plages répartit ce travail, mais ne réduit pas la masse
   des paires survivantes. Voir `wspd_q34.cpp:395–467`.
4. L'atlas q4 partage un plan de centres par arête, classe des témoins dans
   des cellules, garde leurs frontières indécises et balaye des événements
   par graine. Depuis `0948d2d0`, q3 consulte cet atlas pour rejeter à
   K−1 avant le census. Si le rejet échoue, q3 repart d'un census de boule
   global depuis zéro (`wspd_q34.cpp:535–594`).
5. Chaque graine q4 survivante relit les sites actifs des fragments qu'elle
   traverse et trie ses événements. Les racines de même niveau sont
   groupées, les contacts conservés (`q4_local.cpp:334–410`). L'option
   nouvelle `saturate_deep` interrompt une classification à K−1, avec
   certificat terminal distinct du fragment exact ; elle est désactivée
   dans la mesure LiDAR publiée.
6. La sortie v8 q3/q4 est une suite de présentations avec profondeur et
   coquille ; elle n'est ni un catalogue de boules dédupliquées avec
   intérieurs complets, ni une histoire de composantes, ni une tour.

Les masques q2/q3/q4 ne peuvent pas être chaînés comme des filtres :
un propriétaire rejeté en q2 peut porter un q3/q4 valide ; un triangle
rejeté en q3 peut être la graine canonique d'un q4 valide. Les petites
contre-fixtures entières sont dans
[q3/q4 objets et stratégie](../../morsehgp3D_v8/docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md).

## 4. Coûts observés, avec périmètre explicite

| Entrée / méthode | Travail mesuré | Ce que cela prouve |
| --- | --- | --- |
| Trois trames brutes 08/000000, 000100, 000200 | 123 389 / 124 479 / 125 526 retours ; autant de sites en float32 et aucune fusion à 1 mm | Préparation d'entrée, pas calcul HGP ; même séquence 08 |
| Masques sans sol correspondants | 39 885 / 35 551 / 45 845 sites retenus ; lecture→masque environ 30 ms local par trame | Segmentation géométrique rapide, pas vérité sémantique ni tour ; temps à inclure dans le total sans sol |
| Index natif float32 seul, trois trames brutes | 126,906 / 129,596 / 95,983 ms CPU local | Construction de l'index, prouvée O(n log n) ; pas coût du front ou des candidats |
| Voie q2 u16, anciens préfixes LiDAR 50k, quatre cœurs | 1,45 s K5 ; 2,8–5,5 s K10 | Composant q2 seulement, autre entrée/profil ; aucune tour et aucune mesure sans sol |
| Sans sol 08/000000, grille 1 mm, 39 885 sites, K5/s8/W8, CPU local | 104,63 s mur ; 812,82 CPU·s ; 691 284 q3 + 158 496 q4 émis | Première référence 1 mm du **flux**, une seule répétition, sans segmentation/FULL/GPU |
| Même ligne, géométrie v8 | 23,687 M paires développées ; 2,044 M covers préparés totalisant 2,963 G appartenances de sites ; 184,462 M graines q3 ; 31,425 M boules q3 construites ; 1,126 G bornes q3 préparées ; 38,795 M cellules q4 ; 3,252 G bornes de blocs + 7,316 G tests ponctuels + 5,547 G IDs de frontière copiés | Les covers, atlas et census sont les postes à réduire ; 163,678 M comparaisons de tri q4 ne sont pas le premier verrou |
| Sans sol u16/2 cm, trois scans séquence 08, phase1+2, K5/W8 | 108,0 / 98,4 / 255,2 s mur ; scène0 K10/W8 : 323,0 s | Gains réels sur candidat CPU, mais précision et périmètre historiques ; lignes 01/02 sous charge concurrente |
| Brut u16/2 cm sur G4, K5/s8/W48, ancienne tranche34 | 165,214 / 34,319 / 505,479 s mur, CPU moyens 4,19 / 11,13 / 1,93 | CPU G4 seulement, charge mal équilibrée ; antérieur à la file actuelle, pas un résultat 1 mm |
| V7 uniforme u16 50k, K1..10 / K1..5 FULL | 418,873 / 33,853 s ; 21,47 M boules + 27,27 M nœuds à K10 | Une tour FULL fut exécutée sur ce profil historique, loin des contrats et sans transfert à v8 |

Sources : [préparation](../../morsehgp3D_v8/receipts/float32_precision_20260921/README.md),
[masques](../../morsehgp3D_v8/receipts/lidar_ground_20260921/README.md),
[index](../../morsehgp3D_v8/docs/INDEX_FLOAT32_ET_SUITE_Q34_20260921.md),
[reprise q2](../../morsehgp3D_v8/docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md),
[reçu 1 mm](../../morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/BASELINE.only.json),
[phase1+2](../../morsehgp3D_v8/receipts/ground_phase1_20260921/README.md),
[G4 q3/q4 brut](../../morsehgp3D_v8/receipts/q34_spatial_20260921/README.md),
[synthèse v7](../../morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md).

Sur 48 vCPU, les 812,82 CPU·s actuelles exigeraient déjà au minimum
16,93 s avec un partage CPU parfait, avant tout coût FULL. Atteindre 1 s
par la seule répartition sur ces CPU réclamerait une réduction du travail
d'au moins ×16,9 ; 100 ms au moins ×169,3. C'est une **borne arithmétique
conditionnelle au même travail CPU**, pas une prédiction de performance GPU.
Le gain doit venir d'une réduction structurelle des interactions et, après
celle-ci, d'une exécution massive adaptée au matériel.
Les 2,963 G « sites de covers » somment des populations qui se recouvrent
entre arêtes, **pas** un ensemble distinct à transférer sur GPU.

Les préfixes 8k/16k/32k de la tranche34 montrent des progrès de navigation,
mais le scan0/K5 garde au dernier doublement ×4,112 bornes de blocs
d'atlas ; scan200/K5 a ×4,317 bornes de census q3 au premier. Les vraies
coupes spatiales d'une trame brute rapportent aussi des ratios super-
quadratiques trame→moitié positive pour q3 et q4. Ces faits interdisent de
déclarer « sous-quadratique LiDAR » sur la base des seuls temps favorables.
Les trois scans 000000/000100/000200 appartiennent à **une seule séquence**.
Voir [croissance tranche34](../../morsehgp3D_v8/receipts/q4_seed_cells_20260921/ANALYSE_CROISSANCE.md)
et [coupes spatiales](../../morsehgp3D_v8/docs/Q34_MESURES_SPATIALES_20260921.md).

La file actuelle **occupe réellement les huit CPU locaux** : chaque worker
cumule environ 101–102 CPU·s pendant les 104,6 s mur. Elle publie 1 056 679
tâches ; 154 616 refus de publication sont exécutés par le producteur lorsque
la file atteint 4 096 éléments. Les attentes déclarées sont inférieures à
1 ms par worker. Cela n'établit ni un défaut dominant du mutex ni un bon
équilibrage à W48 ; mesurer les temps d'acquisition et la plus longue arête
avant de réécrire l'ordonnanceur. Source :
[reçu brut 1 mm](../../morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/only_probe_01_s00_k5_w8.json),
versionné depuis `3f0d188f`.

## 5. Verrous mathématiques pour v9

### 5.1 La tour est un objet de composantes de facettes

Les sommets du graphe Γ_K sont les K-facettes apparues, **y compris les
isolées**. Des composantes peuvent partager des points sans être égales.
Sous hypothèses régulières, les minima Gabriel de cardinal K sont les
feuilles ; les cofaces de cardinal K+1 déterminent les fusions. On peut
supprimer des nœuds Γ silencieux de la **sortie**, mais il faut encore
résoudre leurs rattachements au bon niveau pour obtenir les vrais parents.
Le graphe induit par les seuls minima Gabriel et un MST des points sont
insuffisants en général. Voir
[fondements](../../morsehgp3D_v8/audits/FONDEMENTS_ET_OBJET.md) et
[producteur horizontal v7](../../morsehgp3D_v7/docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md).

Les données quantifiées ne sont pas toutes régulières. Une coquille peut
avoir plus de quatre sites malgré un support positif minimal d'au plus
quatre. La v7 a développé quotients de coquille, lots de niveaux,
contributions datées et ancres verticales ; son
[constructeur par boules](../../morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md)
est une référence à porter avec preuves nouvelles. Son plafond de coquille
12 et certaines largeurs u32 ne suffisent pas à des données massives.

### 5.2 Les niveaux et les seuils ne sont pas interchangeables

Pour une boule de support positif minimal q et p sites strictement
intérieurs, la fenêtre pertinente satisfait p+q≤Kmax+1. Les seuils de
rejet sont donc h₂=Kmax, h₃=Kmax−1, h₄=Kmax−2. La même boule peut avoir
plusieurs présentations, parfois avec des q différents ; la clé de boule
globale, le q minimal, les intérieurs et la coquille complète doivent être
établis avant une prétention de catalogue exact. Une clé de rayon seule ne
déduplique pas les boules. Voir
[objets q3/q4](../../morsehgp3D_v8/docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md).

Précision décisive pour la v9 : **incidence de facette nécessaire** ne veut
pas dire **toutes les présentations positives d'une même boule**. Le
constructeur [FULL par boules v7](../../morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md)
reçoit un record par clé de boule avec intérieurs, coquille et `q_min`, puis
résout les facettes nécessaires ; il n'exige pas en entrée tous les
triples/quadruples supports de cette boule. Dédupliquer après avoir payé
toutes ces présentations ne suffit pas. La v9 doit chercher comment
choisir une ou quelques présentations canoniques **avec preuve de
complétude**, tout en conservant l'information nécessaire aux vrais
parents et aux plateaux. Cette distinction peut éviter une explosion
cubique de présentations sur une coquille cosphérique, même si le
traitement du plateau lui-même reste un coût à borner.

Un certificat q4 à K−2 peut rejeter q4 tout en laissant q3 vivant à
K−1. Une cellule `Outside` de l'atlas q4 ne rejette pas q3 ; elle peut
simplement manquer de complétions q4. Pour partager l'atlas avec q3,
l'interface doit distinguer au minimum : feuille **exacte** avec compte
et frontière actifs ; minorant suffisant pour q3 ; minorant suffisant
seulement pour q4 ; indisponible. Dans les deux derniers cas, q3 utilise
son census exact global ou une nouvelle preuve, sans inventer de compte
ni ajouter deux crédits qui se recouvrent. L'option `saturate_deep` à K−1
fournit le second état, mais sa rentabilité LiDAR n'est pas mesurée.

### 5.3 Un rectangle WSPD n'est pas une borne de travail

Une décomposition en O(s³n) rectangles demande un arbre et un packing
prouvés pour **l'index réellement utilisé**. Même sous cette hypothèse,
la somme des tailles de facteurs, les paires survivantes, leurs covers,
leurs graines et les payloads peuvent croître quadratiquement. La v8
mesure 23,687 M paires résiduelles sur la seule ligne 1 mm citée. Les
témoins universels du rectangle, puis locaux h_a/h_b, restent des
certificats suffisants ; ils ne caractérisent pas tous les survivants.
Ne pas réintroduire |A|²+|B|² en préparant ces témoins, ni payer le même
cover pour chaque présentation. La comparaison s=8/10/12 doit porter
sur **front + témoins + résidu + q3/q4 + catalogue**, pas seulement sur
les rectangles. Voir [audit WSPD](../../morsehgp3D_v8/audits/WSPD_Q2_Q3_Q4.md).

### 5.4 Contre-audit récent : cascade de rectangles et collectif d'arête

L'[audit indépendant des rectangles](https://github.com/Ludwig-H/E-HGP/blob/12294241/morsehgp3D_v8/audits/lidar_rectangles_20260922/README.md)
explore précisément la transition trop directe `rectangle() → expand()` :

1. Pour le compte universel h, choisir une paire représentante du rectangle
   uniquement pour **exclure** les blocs Z sans témoin de cette paire. Elle
   n'accorde jamais un crédit positif au rectangle ; les bornes universelles
   décident encore les blocs restants.
2. Réemployer la recherche déjà terminée des rectangles singletons. Pour
   les gros produits, calculer h_a/h_b à partir de sites des facteurs,
   disjoints des h externes, et rejeter une paire sur la voie q si
   `h+h_a(a)+h_b(b)≥Kmax+2−q`. Les petits produits gardent le chemin léger.
   Un bloc exclu du h universel peut encore servir aux crédits locaux.
3. Sur les arêtes survivantes dont l'aval est cher, essayer un certificat
   **collectif** exact sur un petit pool de témoins, avant cover/atlas.
   Une paire certifiée donne au moins un intérieur ; trois paires formant
   un triangle donnent deux intérieurs. Ces crédits ne se somment pas aux
   comptes partiels individuels sans preuve de disjonction.

Pour un groupe fixe S et l'arête ab, le test collectif utilise
`H_S=Σ(z−a)·(b−z)` et `V_S=Σ(b−a)×(z−a)` : `H_S>0` et
`α_q H_S²>||V_S||²`, avec `α_3=3` et `α_4=2`, garantissent au moins
un site strictement intérieur à toute boule positive admissible dont ab
est une arête maximale. Le site intérieur peut dépendre de la boule :
le groupe donne **un crédit**, pas sa cardinalité. Les groupes qui se
recouvrent ne s'additionnent pas gratuitement.

Pour expliciter la disjonction du plan par facteurs : `h_q` crédite des
sites universels hors A∪B, `h_{q,a}(a)` des IDs distincts de A\{a}
universels sur `{a}×B`, et `h_{q,b}(b)` des IDs de B\{b} universels sur
`A×{b}`. Les facteurs du front sont disjoints ; un site de A ou B ne peut
pas satisfaire le test strict du h commun, car la boîte de son propre
facteur contient un endpoint égal au site. Le port doit renvoyer un
**certificat par rectangle/voie**, lié à l'index et au seuil ; les
compteurs cumulés d'instrumentation ne sont pas des crédits géométriques.

Les [mesures combinées](https://github.com/Ludwig-H/E-HGP/blob/12294241/morsehgp3D_v8/audits/SYNTHESE_PRIORITES_LIDAR_20260922.md)
sur des rectangles échantillonnés des trois trames 08 sans sol à 1 mm
trouvent **×2,55 à ×4,46** sur le *filtrage*, avec mêmes paires/masques ;
les 6 902 contextes plus larges documentent 758 gros rectangles complets.
La chaîne s'arrête **avant** covers, atlas, graines, catalogue et FULL :
elle n'a éliminé aucun atlas supplémentaire par elle-même. Dans le front
mesuré, environ 97 % des rectangles ont moins de 64 paires, tandis que
les produits d'au moins 1 024 paires portent 70–75 % de la masse ; cela
justifie un plan sélectif, pas un seuil universel de 256. Des variantes
naïves de reprise par listes d'IDs et de récursion avec redémarrage de
l'index ont été plus lentes : ne pas les porter comme des « gains ».
Les captures étendues et les appelants expérimentaux sont décrits comme
présents dans des archives jointes à la conversation, pas entièrement dans
le dépôt ; leur intégration à une porte reproductible reste à faire.

Le [certificat collectif](https://github.com/Ludwig-H/E-HGP/blob/12294241/morsehgp3D_v8/audits/CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md)
est prouvé/testé sur petits nuages avec un corpus Fraction et un prototype
entier u18. Il peut rejeter des arêtes qu'aucun témoin individuel ne rejette,
mais la recherche de groupes a un prix. Sur 763 arêtes LiDAR
**stratifiées**, 221 rejets q4 ont été confrontés à l'appel q4 complet ;
ces nombres ne sont pas un taux de rejet global. Le bilan contrefactuel q4
est souvent favorable dans cet échantillon, mais q4 seul peut partager
son atlas avec q3 : l'économie de l'appel combiné reste à mesurer.
Pour une preuve uniforme sur A×B, le **même groupe d'IDs** doit passer les
64 couples de coins ; disposer d'un groupe différent à chaque coin est
un raccourci faux, avec contre-exemple exact dans l'audit.

Décision v9 : porter la cascade des deux premiers étages **en option unique
avec ablations**, mesurer l'appel q3/q4 complet sur les trames entières,
puis réserver le collectif aux résidus où son coût d'acquisition est
inférieur à l'aval évité. Ne multiplier ni les ratios ni les crédits. Ce
chantier court terme complète, sans remplacer, la refonte du plan de centres
ci-après.

## 6. Proposition centrale v9 : traiter les centres q3/q4 ensemble

Fixons une arête propriétaire ab. Tous les centres de boules passant par
a et b vivent dans son plan bissecteur. Chaque site z du cover y définit
une **forme affine exacte** L_z(c), négative à l'intérieur, nulle sur sa
droite de contact. Alors :

- une graine q3 valide x définit sa droite L_x=0 ; son circumcentre est
  le point de cette droite minimisant le rayon dans la métrique réelle ;
- une complétion q4 (x,y) définit une intersection L_x=L_y=0 ;
- la profondeur d'un centre est le nombre de formes strictement négatives.

La v8 utilise déjà cette dualité, mais reconstruit de nombreuses
classifications de témoins par cellule, puis répète les requêtes par
graine. Pour v9, comparer **deux voies réellement différentes** sur une
arête fournie, puis sur leur somme dans le générateur :

**A — décomposition certifiée adaptative du domaine peu profond.** Garder
un arbre de cellules du plan de centres et des blocs de sites immuables.
Une cellule porte un compte exact d'intérieurs constants, des extérieurs
stricts et une liste de conflits partagée par références. Dès que des
blocs disjoints prouvent h₃ intérieurs, la rejeter pour q3/q4 ; à h₄,
rejeter seulement q4. Un groupe de centres q3 ou de lignes q4 se partage
la classification. Les frontières sont fermées ; les formes tangentes
restent dans le conflit. Affiner **selon la charge des graines et des
intersections encore possibles**, pas selon une profondeur fixe. Mesurer
copies d'IDs, cellules, conflits, bornes et sorties, avec repli exact
pour toute cellule indécise. Une variante à frontière persistante/CSR
doit prouver qu'elle réduit aussi les lectures, pas seulement les copies.

**B — extraire directement les faibles niveaux d'un arrangement de
droites.** Les q4 admissibles sont des intersections à profondeur
strictement inférieure à h₄, dans le domaine positif et sous la règle
de propriétaire. Les centres q3 forment des requêtes ponctuelles hors
de ce flux d'intersections. Construire ou interroger seulement les
niveaux de profondeur 0..h₃−1 éviterait le balayage de tous les événements
de chaque graine. Les travaux sur les [contours de profondeur de
demi-plans](https://www.math.tau.ac.il/~michas/k_depth.pdf) motivent cette
recherche, **sans fournir ici un algorithme complet ni une borne sommée
sur toutes les arêtes**. Les droites coïncidentes, intersections multiples,
contacts et coquilles imposent des groupes d'IDs exacts. La positivité
et la propriété de l'arête restent des filtres indépendants du niveau.

La comparaison doit inclure la préparation m du cover, le nombre de
graines S, tous les événements examinés E, les cellules/conflits et les
sorties U : une réduction de S·m qui déplace un m² ailleurs est rejetée.
Tester au moins la grille dense permutée, deux rangées, nuages 3D, scans
SemanticKITTI brut/sans sol et adversaires à nombreuses coquilles.
Comparer q3 et q4 séparément **et** leur partage : q4 ne peut pas être
alimenté seulement par les q3 acceptés.

**Expérience immédiate C — bornes de blocs sur la droite d'une graine q4.**
Avant de développer les sites actifs d'un fragment en événements, borner
exactement chaque forme affine sur l'intersection *segment de la graine ×
boîte Z*. Un bloc uniformément intérieur ou extérieur contribue sans
événement individuel ; un zéro ou une borne ambiguë garde le bloc vivant.
Cette expérience peut économiser les relectures `graine × feuille × sites`
sans construire d'emblée l'arrangement complet. Elle exige une preuve de
restriction géométrique au segment, une coquille exhaustive, et une mesure
du coût des subdivisions et des événements évités. Voir la
[boucle actuelle](../../morsehgp3D_v8/src/lanes/q4_local.cpp).

**Pont prudent vers q3.** Une feuille *exacte* de l'atlas contient déjà le
compte d'intérieurs constants et la frontière active disjointe : si le
centre q3 y appartient, on peut en principe terminer son compte et sa
coquille sur cette frontière, sans repartir de la racine globale. C'est
une déduction des invariants, pas une optimisation implémentée ou mesurée.
Un état q4 seulement profond à K−2, une cellule hors du domaine q4 ou un
certificat terminal sans fragment ne suffisent pas ; dans ces cas, conserver
un repli q3 exact. Voir [contrat du fragment](../../morsehgp3D_v8/src/lanes/q4_local_partition.hpp).

**Comparateur de recherche : mosaïques de Delaunay d'ordre faible.** Un
algorithme publié construit les mosaïques successives à partir de
triangulations pondérées et de cellules précédentes
([Edelsbrunner et Osang](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf)).
La v9 peut étudier si leurs cellules fournissent un surensemble complet
et plus petit des boules critiques sur les scans visés. Cela exige une
preuve explicite de la conversion vers les supports Gabriel/MEB, du
traitement des égalités et des vrais parents. Le coût 3D peut rester
quadratique ; ce n'est pas une certification automatique de la tour.

## 7. Ce qui se transpose des versions précédentes

- **v6** : les objets de lots GPU, stockage filaire, bail/retour et
  validation transactionnelle donnent des invariants utiles de durée de
  vie et d'annulation. Ils ne fournissent pas un kernel v9 qualifié ni un
  gain G4 héritable. Voir [architecture GPU v6](../../morsehgp3D_v6/docs/GPU.md).
- **v7** : le producteur FULL relatif à des catalogues Gabriel complets,
  puis la tour par boules, explicitent feuilles, vrais parents, plateaux,
  contributions et verticales. Les petites portes Gamma servent d'oracle
  indépendant. Les tours historiques 50k et leurs temps ne qualifient
  ni v8 ni v9.
- **v8 q2** : l'index partagé, les plans par rectangle, les parcours par
  blocs, les comptes/curseurs transmis et l'équipe persistante constituent
  de bonnes pièces. Le choix de granularité doit suivre le travail réel
  dans q3/q4 ; q2 seul ne ferme pas la tour.
- **v8 q3/q4** : les seuils distincts, formes affines entières,
  propriétaire par arête maximale, cover fermé, événement exact, clés et
  coquilles sont des contrats à préserver. Les atlas actuels servent
  surtout de référence différentielle pour une architecture moins
  répétitive.

## 8. CPU, GPU et échelle massive

La file de plages de rectangles v8 améliore l'occupation locale ; elle
ne partage pas une **arête lourde** ni une longue construction d'atlas.
Une v9 doit pouvoir transférer, avec propriétaire immuable et état
possédé, des tâches de quatre grains : produit du front, plage de paires,
arête avec son cover, puis cellule/conflit ou lot de graines/événements.
Chaque tâche a un **ID de travail** stable, distinct de la clé de boule
utilisée pour la déduplication et des identités de présentations à vérifier.
Une reprise publie son lot une seule fois, toutes les émissions ont un slot
déterministe, la file pleine laisse une continuation locale et la
publication est complète ou annulée. Mesurer CPU utile par worker,
attente, queue, p95/p99 des coûts d'arêtes et dernière tâche active.
Une file à vol de travail ou des lots GPU ne résoudront rien si chaque
arête reproduit le même coût ; comparer d'abord la baisse du travail par
arête et par trame. Les 48 vCPU G4 ne sont pas 48 cœurs physiques.

Sur `g4-standard-48`, la documentation Google indique 48 vCPU (threads
logiques), un GPU RTX PRO 6000 Blackwell et 96 GB de mémoire GPU
([types G4](https://docs.cloud.google.com/compute/docs/gpus)). La mémoire
ne dispense ni des indices 64 bits pour les objets dérivés ni des lots
bornés en résidence : à plusieurs dizaines de millions de points, les
sorties et conflits peuvent dépasser la taille des points eux-mêmes.
Une exécution GPU utile garde points, index et formes compactes résidents,
traite en parallèle les bornes cellule×bloc, les requêtes de centres et
les décisions de faible profondeur, puis fait scans/compactions/tri des
seuls survivants. Un filtre flottant ne rejette qu'avec intervalle sûr ;
ambiguïtés et égalités utilisent une voie entière exacte dont fréquence,
transferts et latence sont mesurés. La fusion hôte ne peut reconstruire
des milliards de résultats intermédiaires si la cible est 1 s.

La mesure v7 est instructive : 189 ms de kernels de census K10, mais
4,54 s pour la phase complète, dont 2,93 s de reconstruction hôte
([audit v7](../../morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md)). Pour v9,
le chronomètre G4 couvre transfert, préparation, calcul, catalogue,
parents et sortie exigée. Comparer mono, 4–8 CPU locaux, 48 vCPU G4 et
GPU sur **la même entrée**, avec même géométrie et mêmes niveaux.

## 9. Registre de risques et critères de passage v9

| Priorité | Verrou | Expérience qui décide |
| --- | --- | --- |
| P0 | Réduire les paires résiduelles **et** les covers répétés avant l'arête | Certificats universels de rectangles, h_a/h_b et subdivisions possédées ; F=Σ tailles des facteurs, Σ|A||B| vivant, nombre/taille de covers et coût aval, s8/10/12 sur mêmes trames |
| P0 | Réduction du travail q4 atlas et q3 census, sans déplacer le carré | Voies A/B/C sur **une arête**, puis somme sur toutes les arêtes ; comptabilité des cellules, conflits, scans, tris, sorties et copies ; comparer au moteur v8 exact |
| P0 | Complétude du catalogue et vrais parents | Boules dédupliquées par clé exacte, intérieurs/coquilles globaux ; petits oracles Γ/Gabriel indépendants ; port FULL relatif v7, plateaux et verticales compris |
| P0 | Profil contractuel entier 1 mm | Qualifier exactement u18/1 mm sur toute la chaîne et sur les trames entières ; ne transférer ni les mesures u16/2 cm ni les preuves d'un autre profil |
| P0 preuve | R2 u18 échoue en lecteur malgré CTest vert | Corriger la lecture `status="disabled"`, reconstruire/capturer R3 frais, relire LIVE normal/−O ; publier R1/R2 comme historiques |
| P1 | Float32 brut, objectif secondaire ; raccord q3 global en chantier | Portes Fraction et mutations sur le code gelé, puis mesure de front+arêtes+graines ; ne pas confondre ce flux q3 avec q4/FULL |
| P1 | Travail multi-CPU irrégulier | Partager aussi arête et cellule, W1/W4/W8/W48, équilibre de CPU utile et digests/records identiques |
| P1 | GPU exact et coût complet | Préfiltre certifié + repli, tâches résidentes, transferts et débit de sorties ; mesurer la **tour** G4 et pas les seuls kernels |
| P2 | Plusieurs dizaines de millions | Tuiles spatiales avec halo **certifié** ou index global, pas de coupure heuristique ; offsets64, streaming, mémoire/VRAM et reprise publiés |
| P2 preuve | Notes numériques anciennes prises pour le code actuel | Refaire les bornes par profil : q4 est maintenant u18/échelle 2^20, tandis que des notes disent u16/2^44 ; le commentaire q3 parle encore de 49 cadres contre 55 courants |

Matrice minimale : plusieurs séquences préchoisies, trames RAW entières et
leur masque sans sol figé ; grille 1 mm principale et float32 secondaire,
avec reçus séparés ;
K5/K10, s8/10/12 ; sept morceaux spatiaux par trame pour six relations
parent/enfant et tailles 8k/16k/32k synthétiques séparées. Répéter les
mesures appariées sur matériel stable. Publier pour chaque régime temps
FULL, CPU·s, taille de sortie, temps par unité de sortie, pic RAM/VRAM,
nombre d'objets des étapes et taux de repli exact. Un ratio <4 sur deux
doublements est une observation, jamais une preuve asymptotique ; tout
ratio défavorable reste visible. Aucune coupe ou préfixe ne qualifie le
contrat de trame entière.

Pour les coupes, garder le protocole capteur : trame entière, deux moitiés
séparées par un plan passant par l'origine du capteur, puis quatre quarts
par un second plan perpendiculaire passant par cette origine. Le masque
sans sol est décidé **avant** les coupes ; les mêmes IDs de retour suivent
tous les profils. Les préfixes historiques 8k/16k/32k ne sont pas des
coupes spatiales de ce protocole.

### Décisions proposées au développeur v9

1. Figer le contrat de sortie FULL avant de choisir le générateur, en
   réutilisant l'oracle Γ et la tour par boules de v7 comme références
   **indépendantes** sur petits nuages et plateaux.
2. Raccorder rapidement la cascade de rectangles auditée, avec ablations et
   contrôle des mêmes sorties ; arrêter cette ligne si son gain **sur tout
   q3/q4** est faible, même si le filtre seul gagne plusieurs fois.
3. Mettre en concurrence cellules adaptatives, faibles niveaux de droites
   et bornes de blocs sur segments, avec un prototype **q3+q4 par arête**
   qui collecte un payload exact. N'accepter une variante qu'après la somme
   sur la trame entière et la mesure du travail déplacé.
4. Construire le catalogue et un premier FULL CPU exact dès que le
   producteur est complet ; paralléliser ensuite les mêmes objets, puis
   porter les noyaux massifs au GPU. Les contrats G4 jugent la tour entière.
5. Réparer le reçu R2 u18 séparément. Les essais de cette reprise ne
   constituent pas un point de départ qualifié tant que le lecteur refuse.

Ce rapport est un audit de conception et de preuves, pas une preuve
d'existence d'un algorithme v9 sous-quadratique pour tous les LiDAR. Le
premier jalon est une baisse mesurée du **travail total** de q3/q4 sur les
trames pertinentes, avec sorties exactes inchangées.

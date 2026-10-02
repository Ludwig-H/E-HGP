# Synthèse de l'audit v10 pour le développement v11

État documentaire du 2 octobre 2026 ; rédaction sur la base v11 `b104028f5`.
La v10 publique étudiée est `afb081774`. Ce document rassemble les conséquences utiles au port,
sans remplacer les reçus ni annoncer un audit exhaustif. Cette rédaction a consisté en lectures,
contrelecture de formules et vérifications documentaires ; aucun build, benchmark ou GCP n'a été
lancé pour produire ces trois documents. Les exécutions antérieures gardent leur auteur, leur source
et leur domaine.

## 1. Pièces présentes et manquantes

Le dépôt privé de travail est `build/v11-persist/audit_v10/`. Les neuf rapports ci-dessous sont
présents et ont été consultés pour l'inventaire et la conception ; leur lecture ciblée ne vaut pas
rejeu de chaque preuve annexée.

| Rapport présent | Apport repris | Limite à conserver |
| --- | --- | --- |
| L01 — mathématiques du catalogue | MEB, supports canoniques, listes K-certifiées, domination, complétude conditionnelle | Ni terminaison d'une politique arbitraire ni borne globale de coût |
| L02 — mathématiques de la tour, version corrigée | Graphe des k-parties, morceaux, descentes, plateaux, verticales, Euler | Surjection vers le global, raffinement exhaustif, mémo daté ; Euler non suffisant |
| L03 — mathématiques des points | core, cover, projection, ablations MR, diagnostics de qualité | Pas de singleton équivariant universel ni d'équivalence cover/MR déduite des moyennes |
| L04 — code des fondations | Statuts, propriété, mémoire, parallélisme, I/O et domaines | Port explicite et requalification par fichier |
| L05 — code du catalogue | Entonnoir de candidats, réglages de feuilles, prédicats, disposition mémoire | Les optimisations mesurées isolément ne qualifient pas le pipeline v11 |
| L06 — code de la tour | Répartition des cellules, descentes, semis, Kruskal, verticales | Prototypes et coûts v10, sans héritage de qualification |
| L07 — tête et CLI | Condensation des entrées différées, sorties, domaines et sélection | Réparer l'objet et son juge ; distinguer projection, condensation et sélection |
| L08 — tests et portes | Lacunes du juge FULL public, mutations survivantes, juges d'échelle | Un CTest vert peut juger un objet plus faible que celui annoncé |
| L10 — banc synthétique | Familles, protocoles, croissance et comparaisons | Un lot fini n'établit ni complexité générale ni supériorité statistique |

**Absents lors de cet inventaire : rapports L09 et L11–L16.** Des répertoires de preuves existent
pour L09, L12, L13 et L14 ; ils ne remplacent pas leurs rapports de clôture. L09 fournit notamment
une table de temps G4 extraite des captures. L11, L15 et L16 ne peuvent recevoir un verdict sur la
seule foi de leur numéro prévu. La matrice des résultats manquants doit être complétée avant toute
synthèse dite globale, notamment comparaison HDBSCAN, numérique/mutations et voies GPU.

Les sources de conception consultées sont le brouillon assemblé des mathématiques et les documents
privés CONCEPTION_GENERATEUR, CONCEPTION_TOUR et PISTES_DE_RUPTURE. Leur vocabulaire « prouvé » signifie
qu'un argument est écrit par l'auteur ; il ne signifie pas que son port C++ est qualifié.

Empreintes des principales versions effectivement utilisées, sous `build/v11-persist/` :

| Source | SHA256 |
| --- | --- |
| audit_v10/L02_MATH_TOUR.md | ecb3231a321f03567b2b6f54246e66bf38f8201d04b72784e29958c3428d089d |
| audit_v10/L03_MATH_POINTS.md | b84c5088cfa1434411bb29d4e2b710db18cd635f5bf5504fe2e82253ff4b2296 |
| mathematiques/brouillon/MATHEMATIQUES.assemble.md | f829827d182ed44763f72b1e133c27d03fabf6bed34e0066fe486df53bcfeea9 |
| conception/CONCEPTION_GENERATEUR.md | 0fc30de4c3c9424945bfb79990d40930295f309a72edf27da5e92fda0173f296 |
| conception/CONCEPTION_TOUR.md | 01e217f03ad60187e2b2005ed12d16db0e04635725fafee880ae8cc52071c0e6 |
| conception/PISTES_DE_RUPTURE.md | e093f29285afe9fbfe6ede5646962d3f94111c1690e9f19d81c663e6fdb4d226 |

Les chemins privés identifient une provenance locale, pas une dépendance d'exécution du futur moteur.
Les contrats nécessaires sont désormais résumés dans [MATHEMATIQUES.md](MATHEMATIQUES.md).

## 2. Ce qui est établi, corrigé ou encore ouvert

| Sujet | État retenu | Conséquence pour v11 |
| --- | --- | --- |
| Théorèmes catalogue/FULL | Arguments conditionnels relus, dont Q1 corrigé ; petits témoins indépendants favorables | Porter chaque hypothèse, puis juger le port contre la définition brute |
| Morceaux locaux | Peuvent déjà être reliés par des chemins extérieurs | Dédupliquer les racines globales ; ne jamais supposer une bijection locale/globale |
| Descente | Toute politique valide conserve la classe au seuil ; terminal non unique | Politique fixée pour le déterminisme, mémo valable seulement à partir de son niveau |
| Oracle FULL public v10 | L08 montre qu'il acceptait des forêts/verticales fausses tout en comparant certaines coupes de points | Comparer la forêt canonique entière et tester le juge avec des dumps faux |
| Euler | Identité exacte, omissions compensables | Porte complémentaire, jamais certificat de catalogue complet |
| F3 | Défaut de réutilisation de l'erreur corrigé par propagation par expression | Chaque port conserve son arbre d'expression et son domaine |
| F6 | Argument favorable sous hypothèses ; conversions et seuil représentable à protéger | Qualifier les expressions réelles, pas une règle de degré approximative |
| Refus Result et ancien StageTimer | Corrections présentes dans le socle relu : stockage discriminé, Stopwatch sans références fragiles | Ne pas rouvrir les anciens défauts ; vérifier les portes de la version effectivement portée |
| Harnais et mutants | Lecteurs structurés et contre-portes ajoutés ; lancement impossible et faux verdict d'arrêt anormal signalés au suivi | Les corrections du développeur doivent avoir leurs propres reçus ; un signal n'est pas une mort causale |
| Multiplicités | Modèle par copies proposé, port complet non clos | Publier les poids et refuser la tour pondérée au plus tôt |
| FULL → points | core défini ; cover intrinsèquement ensembliste | Publier l'ensemble avant de choisir une projection exclusive |
| Tête v10 | Défauts reproduits sur cohortes et racine trop petite ; correctif isolé qualifié sur un domaine borné | Port et domaine numérique à traiter séparément, règle vote encore ouverte |

Références courantes : [réponses Q1–Q5](../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md),
[suivi de l'audit](../audits/AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md) et
[réponse du développeur](../audits/REPONSE_CLAUDE_VERROUS_MOTEUR_20261002.md).
Une fermeture documentaire ou par lecture de code n'est pas un nouveau rejeu natif.

### R2 : source, résultats et dates distincts

La réserve du premier rejeu Pool/MR1 est réconciliée par les captures finales ultérieures : le MR1
porté est tué ; MR1b a une équivalence **observée sur les essais documentés**. Les suites complètes
GCC/Clang 82/82 et les quatre voies sanitizer 80/80 documentées portent sur l'extraction
`5c2fe1f`, pas automatiquement sur tous les changements postérieurs de `865f5e6`, source du port.

L'inventaire des 425 mutants distingue 387 morts par juge, cinq par signal, une par délai,
29 équivalents, deux limites et un jugé sous TSan. « 425 relus » ne signifie pas « 425 tués causalement ».
Ce constat provient d'une lecture ciblée des captures et commits R2, pas d'un rejeu personnel de cette
campagne. Aucune qualification v11 n'est héritée ; [PROVENANCE.md](PROVENANCE.md) nomme les fichiers
portés et leurs nouvelles portes.

### Domaine des requêtes entières de l'index à préserver au port

Lecture supplémentaire du raccord `865f5e6` : `src/cloud/site_tree.hpp`
déclare des requêtes entières sur **21 bits** ; les signatures utilisent
`i64`. `box_d2_exact` et les distances aux sites calculent `t*t` en `i64`
avant conversion. Avec un seul site nul et `qx=2^32`, donc **hors du domaine
déclaré**, le carré vaut `2^64` et sort du type signé. Ce calcul par lecture
ne démontre aucun défaut dans le domaine ni chez les appelants actuels ;
aucune sonde native n'a été exécutée. L'index n'est pas encore porté en v11.
La frontière `Point::make` possède désormais cette fixture de refus ; le
futur index devra utiliser une entrée certifiée ou garder explicitement
le domaine de ses requêtes. Cela ne remplace pas une voie rationnelle pour
les centres éloignés. Source en-tête SHA256
`ab39d42b48abd0d10765462917be229d0a24a894d6d468f32f2f4a4eb10c5f2b` ;
source `.cpp` SHA256
`8326169c61e2522e3fd0a50745fed8adebd2ed8734ba6473651c8265c6f949c7`.

### Condensation : candidat utile, domaine limité

Le correctif par cohortes reste un overlay v10 non intégré, empreinte
`f79850f89e40abb55a3a3accf3e12f8bc578eefc760f857d99c584f326ff3d4b`.
Son reçu est dans
`morsehgp3D_v10/receipts/audit_full_hierarchie_20261002/cohort_repair/README.md`.
Il corrige sur la fixture 21 points, mcs=5, la stabilité 37/10 en 11/5 et le choix EOM de trois groupes
en deux. Il refuse la sélection d'une racine de trois points sous mcs=5, même avec allow_single ;
le sélecteur de la référence historique partageait ce défaut, d'où l'ajout d'un juge de masse final.

La qualification conservée comprend 563 condensations, soit 2 252 lignes numériques et
2 248 décisions de sélection, en Release et sanitizer, Python normal/−O. **Quatre décisions EOM
proches d'une égalité restent non qualifiées.** Les portes de domaine/racine, quatre mutants causaux,
360 comparaisons de la porte sklearn et 136 contrôles CLI complètent ce lot. Ces comptes décrivent
des petites fixtures et leurs régressions, pas une nouvelle qualification FULL ou LiDAR.

Limites : exposants z1/z2 pour l'oracle exact, stabilité flottante générale non close, transport
node_cluster vérifié seulement en forme/plage, vote non qualifié. La borne O(V+P log P) concerne la
condensation seule ; les parcours répétés d'ancêtres dans la sélection peuvent encore être quadratiques.
Les poids positifs sont conservés dans ce candidat de tête, sans rendre la tour géométrique pondérée.

## 3. Ce que les mesures permettent de prioriser

La pièce L09 `preuves_l09_perf_lidar/TABLE_VERITE_G4.md` relit les sorties CPU de la session v10
`777406b82` sur une G4 à 24 cœurs/48 fils. Pour les trois sous-nuages sans sol du contrat :

| Étendue mesurée | K5 | K10 | Exclusions essentielles |
| --- | --- | --- | --- |
| Catalogue + FULL 1..K, sans attaches, dernière passe de trois | 204,2–253,6 ms | 861,4–1 124,6 ms | Préparation froide 6,6–8,3 ms, lecture et segmentation hors de cette somme |
| Catalogue + FULL avec cover, même session | 220,6–278,0 ms | Pas de mesure comparable dans cette session | Attaches K10 présentes seulement dans une ancienne session de catalogue |
| Chaîne cluster à un seul ordre K5 | Somme 218–263 ms ; processus 231–278 ms | Non mesuré | Un ordre seul, sans verticales ; ne remplace pas FULL |

Ce sont des mesures héritées, pas des exécutions v11 ni GPU. Les trames 00/01/02 de cette table
désignent les trois captures d'une même séquence ; elles ne deviennent pas plusieurs séquences.
Core sur trame complète et une tête sur tous les ordres n'ont pas leur mesure G4 dans cette pièce.
Les répétitions internes chaudes sont séparées des processus froids.

Le catalogue et les descentes pèsent lourd, mais les étages restants représentent encore 36–49 %
du total sans attaches sur ces captures. Une accélération locale du prédicat ou du parallélisme ne
suffit donc pas à annoncer le gain de chaîne. Priorités concrètes :

1. Fermer num et livrer le catalogue exact avec ses juges ; compter les sorties et le travail réel.
2. Réduire les coûts des candidats et les copies, puis mesurer l'ablation sur catalogue complet.
3. Supprimer les préparations de cellules régulières redondantes, vérifier les semis exactement,
   essayer le certificat combinatoire de MEB et les pointeurs datés.
4. Qualifier un noyau de plateau compact, les verticales et les attaches avant de mesurer FULL.
5. Comparer les projections puis la tête, avec scikit-learn réel et diagnostics de pertes séparés.

Les temps projetés par CONCEPTION_GENERATEUR/TOUR et les cycles par boule de PISTES_DE_RUPTURE
restent des **estimations**. En particulier, son affirmation « K10 en 100 ms : non » n'est pas un
théorème d'impossibilité : le supposé plancher dépend de coûts unitaires et d'une famille d'architectures.
Conserver l'objectif utilisateur et le confronter au pipeline réel. La forte taille de certaines sorties
est une contrainte démontrable ; elle ne démontre pas à elle seule ce plancher temporel.

## 4. Hiérarchie et clustering : questions réellement ouvertes

Les résultats L03 permettent de comparer des chaînes nommées, sans définir une « meilleure » hiérarchie
universelle. Garder quatre juges : groupes disponibles dans FULL, pertes dues à la projection, possibilité
de choisir simultanément les groupes désirés, pertes de condensation/sélection. Une excellente valeur
de meilleur bloc par cible ne prouve pas l'existence d'une partition réalisant toutes ces valeurs.

Le témoin exact X={0,2,5}, K2, auto-voisin inclus, conserve le bloc {0,2} dans cover et le perd dans
MR2-bord à cause du plateau où le troisième site entre. Pour cette cible, BestIoU vaut 1 contre 2/3.
Il réfute une identité générale, pas les moyennes publiées de L03 ni une équivalence statistique qui
resterait à tester. Les deux triangles et la symétrie du site médian imposent de publier les ambiguïtés
de cover ; une maturité de masse recouvrante ne garantit pas mcs membres exclusifs.

Le choix initial est donc FULL, core, cover ensembliste et relation boule → nœud. Une projection
exclusive cover, notamment LCA, est un mode distinct, avec sa vraie date. La tête de type HDBSCAN vient
après ce choix. ER0h à arbre et activations fixés, un oracle MR et les résultats d'un lot synthétique ne
se transforment ni en stabilité générale sous déplacement des sites ni en optimalité statistique.

## 5. Fermeture attendue

Le prochain livrable est décrit dans [CONCEPTION_MOTEUR.md](CONCEPTION_MOTEUR.md) : num, puis catalogue,
puis FULL et points. La fermeture exige sources figées, portes dans le dépôt, codes de sortie exacts,
planchers non vacants, mutants causaux et reçus conservant les échecs. Chaque module doit séparer ses
preuves mathématiques, les contrôles du prototype, son port v11 et sa qualification native.

Restent à fournir : rapports absents de l'inventaire ; qualification des expressions et capacités
réelles ; catalogue/FULL v11 contre oracle indépendant ; coquilles étendues au domaine déclaré ;
mesures appariées de chaîne sur trames complètes ; projection exclusive et domaine numérique de la
tête ; comparaison réelle HDBSCAN. Aucun succès du socle, aucune bonne moyenne et aucun accord entre
deux programmes partageant leurs décisions ne clôtent seuls ces points.

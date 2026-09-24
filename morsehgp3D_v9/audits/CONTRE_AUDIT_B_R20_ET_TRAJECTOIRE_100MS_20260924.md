# Contre-audit B — R20 et trajectoire vers 100 ms sur G4

24 septembre 2026. Lecture du commit publié `d1d038393`, du [reçu
R20](../receipts/g4_tower_r20_20260924/README.md), des sorties brutes de
sa VM et du code S4/FULL. Audit en lecture seule du moteur ; **aucun GCP
utilisé ici**. La cible reste la **tour FULL entière** K=1..10, avec repli
K=1..5, sur plusieurs trames SemanticKITTI sans sol à grille 1 mm/u18,
**sans perdre la qualification distincte des trames brutes avec sol**.
Le jalon de 100 ms vient après celui
de 1 s. Aucun temps de cette note ne qualifie un nouveau contrat.

## Ce qui est vérifié, et ce qui ne l'est pas

Le manifeste R20 passe sur **326/326 fichiers** ; le lecteur officiel du
paquet, rejoué en Python normal et `-O`, conclut `completed`. Les 18 cas
sont `complete_relative`, les 12 comparaisons prévues sont égales, et
l'arrêt ciblé de la VM G4 est certifié. La porte locale
`mhgp9_gpu_lanes_port` passe sur le code R20. La correction du CSR plat
public, signalée auparavant par B, est présente : son reproducer ASan
retourne maintenant le refus typé `coverage_flat_draft_shape`. Les anciens
défauts S4 WIP de `both_edges`, de sortie des workers, de trou d'arène et
d'émissions q4 multiples ne sont pas des défauts ouverts de R20.

Cela prouve un reçu cohérent et une égalité **relative au catalogue
recoupé**. Ni Euler (nécessaire, pas suffisant) ni les comparaisons de
condensés ne détectent toutes les clés omises en commun. La comparaison
q3/q4 conserve taille, somme et XOR des IDs de coquille, **pas chaque liste
d'IDs littérale** ; la tour consomme la taille. Garder les oracles exacts
bornés et ajouter un échantillon LiDAR de coquilles comparées ID par ID
avant de revendiquer une exactitude plus forte. R20 ne couvre que trois
trames sans sol de la séquence 08, `s=8`, W48, u18/1 mm : pas de trame
brute, de seconde séquence, de `s=10/12`, de float32 ou de dizaines de
millions de sites.

| Trame 08/ | K1..5 R20 | K1..10 R20 | FULL K5 | chaîne K5 hors FULL |
| --- | ---: | ---: | ---: | ---: |
| 000100 | 1,010 s | 3,147 s | 374 ms | 636 ms |
| 000000 | 1,109 s | 3,873 s | 422 ms | 687 ms |
| 000200 | 1,260 s | 3,860 s | 456 ms | 804 ms |

Les quatre dernières colonnes viennent des lignes GPU primaires des
`vm/probe_*.stdout` de R20 ; les différences sont de simples soustractions
de temps de chaîne. Même FULL gratuit, le reste K5 demande encore un
facteur **6,36 à 8,04** pour 100 ms. Même tout le reste gratuit, FULL
demande **3,74 à 4,56**. À 08/000000/K10, FULL vaut 1,955 s et le reste
1,918 s : chaque moitié doit gagner environ **×19** pour 100 ms. Il ne
s'agit pas de bornes pour un autre algorithme, mais de budgets fermes pour
ce chemin mesuré.

Sur 08/000000/K5, FULL se décompose en 70,5 ms validation, 198,5 ms
phase statique, 56,5 ms lots, 19,6 ms populations, 26,9 ms images
verticales, 17,2 ms banque et 32,2 ms encodage. La **queue hors statique
et lots vaut déjà 166,4 ms**. La projection E6 « FULL 133 ms optimiste »
du [plan tour/voies](../docs/tour_voies_conception_20260924/README.md)
n'est ni une mesure ni suffisante pour une chaîne de 100 ms. Le front
q3/q4 R20 prend 106,5 ms, avant filtre et toutes les autres étapes.
La tour produit 1,542 M nœuds à K5 et 7,426 M à K10 sur cette scène.
Son travail K5 représente déjà 3,622 M représentants de facettes,
1,289 M MEB et 31,708 M visites d'index ; K10, 17,389 M représentants,
11,309 M MEB et
403,430 M visites. **Le tri/Kruskal seul n'est pas le poste à optimiser** :
il faut réduire ou résoudre en masse les facettes, leurs cibles et leurs
incidences tout en gardant les plateaux exacts.

L'ablation entrelacée R20 ne donne **aucun gain GPU de L15** : le noyau
des voies fusionnées prend 56,5–56,7 ms contre 54,9–55,0 ms à K5, et
241,2–241,3 contre 234,4–234,7 ms à K10. L'écart favorable de chaîne
K5 a surtout une attente de préparation différente ; L15 doit rester
désactivé. La préparation du contexte à froid et la mémoire hôte épinglée
méritent d'être testées pour franchir **1 seconde**, mais ne sont pas le
plan de 100 ms. Les 30/151 ms R20 libellés « transfert des voies »
englobent aussi des opérations hôte de tampon/allocation dans les
intervalles instrumentés : ils ne sont pas un débit PCIe pur, ni un gain
intégral acquis par épinglage. Le commit v26 de session CUDA ouverte
avant la chaîne est **postérieur** à R20 ; aucun reçu G4 ne le chiffre ici.

## Garder l'objet ; changer la quantité de travail et son lieu

**À conserver comme contrat de correction :** coordonnées entières et
tests exacts avec repli sur indécision ; identité canonique des boules,
niveau exact, intérieur strict, coquille entière et `q_min` ; propriétaire
unique/ordinaux ; multifusions par plateau exact, racines au seuil ouvert,
parents horizontaux, images verticales aux coupes fermées, contributions
et extensions non régulières. Un certificat ne peut retirer qu'une voie
q3 ou q4 qu'il a réellement prouvée. Le catalogue actuel, les moteurs
CPU et les jumeaux GPU restent les témoins de comparaison ; WSPD, S2/S3,
S4b et l'ABI mémoire courante sont des **réalisations**, pas des objets
mathématiques obligatoires. Garder `s=8` comme base et comparer `s=10/12`
sur les mêmes scènes et les mêmes sorties.

**1. Avant l'expansion des rectangles.** R20 traite 3,134 M rectangles,
dont environ 1,128 M restent ouverts après les témoins actuels ; ceux-ci
donnent 23,687 M paires étendues et 2,044 M arêtes survivantes sur
08/000000/K5.
Le vieux shadow à palette de 64 gardes ne pouvait éviter qu'environ
**0,9 %** des formes de cœur globales de la trame brute sondée ; il ne
justifie pas son port. Le [nouveau certificat de moments multisites](moments_multisite_precore_20260924/NOTE.md)
est exact sur une arête : pour un bloc de sites distincts, ses cinq
moments `N,Z,Q` prouvent au moins `K−1` ou `K−2` intérieurs pour tous
les centres admissibles. Sa fixture et son contrôle rationnel passent.
Une extension mathématique au produit `A×B` est vérifiée ici, **pas
encore portée**. Pour un bloc `G` fixe de moments `N,Z,Q`, poser
`H=4((a+b)·Z−Q−N a·b)`, `D=|b−a|²` et
`C=(b−a)×(2Z−N(a+b))=2((b−a)×Z+N a×b)`. `H` et `C` sont affines
séparément en `a` et `b`, `D` est convexe séparément. Avec
`T3=K−1>0`, `T4=K−2>0`, les fonctions
`f3=3H−4(T3−1)D−√12|C|` et
`f4=2H−3(T4−1)D−√8|C|` sont donc **concaves séparément**.
Jensen appliqué successivement aux deux boîtes donne que `f>0` aux
**64 couples de coins** suffit à `f>0` sur tout `A×B`. Chaque coin se
juge sans racine flottante par `Aj>0` et `Aj²>cj|C|²`, en entier exact.
Cela certifie la voie q3 ou q4 pour tous les supports positifs possédés
par une paire du rectangle,
avec repli inchangé si un coin est indécis. Les conditions de domaine
restent : bloc d'IDs distincts du même sous-nuage, `D>0` sur les arêtes,
bornes arithmétiques recalculées pour la taille de bloc. Un garde peut
coïncider avec une extrémité : sa marge nulle ne lui procure pas de crédit.
Cette preuve ne dit **rien** de la sélectivité ni du coût sur LiDAR.
Le [contrôle entier indépendant](check_moments_rectangle_20260924.py)
rejoue les expansions de `H,C`, une boîte positive q3/q4, douze
translations/homothéties de cette boîte et 160 boîtes aléatoires :
26 certifications aux coins sont vérifiées par énumération de toutes
leurs paires entières. Ce test borné n'est pas la preuve ; la concavité
séparée l'est.
Tester en shadow sur **les rectangles lourds restant ouverts**,
pas 64 tests sur chacun des 3,134 M rectangles (plus de 200 M tests),
avec filtre de coût bon marché **qui ne rejette jamais**, et repli
identique si indécis.

**2. Avant les formes du cœur, même si le rectangle survit.** Le test
ponctuel des mêmes moments sur un bloc d'au plus 64 sites, trouvé en au
plus 32 visites d'index, pourrait fermer les deux voies avant `load`.
Il cible les **359,7 M** `core_sites` K5 de 08/000000 et les pentes
défavorables observées sur les coupes/densités LiDAR. Il **ne réduit pas**
`expanded_pairs`, donc ne suffit pas seul. Si une seule voie ferme,
conserver le cœur partagé pour l'autre ; ne pas créditer des formes
« évitées » fictives. Le panneau S3/S4a d'une scène montre 7/18 pentes
spatiales et 2/14 pentes de densité `core_sites` au moins quadratiques ;
les temps CPU·s de ce panneau restent sous 2. Ce sont des pentes finies,
pas une preuve asymptotique. Son lecteur de reçu est actuellement lié
aux chemins absolus de capture : il passe dans le checkout d'origine et
échoue depuis un autre worktree, défaut de portabilité à corriger sans
changer l'interprétation des données.

**3. Résidence et tour FULL.** Front CPU, filtre GPU, certificats GPU,
voies GPU, census CPU et FULL CPU forment encore plusieurs frontières
et passages sur le chemin critique. Les accélérer d'un petit pourcentage
ne donne pas ×10 à ×40. Une architecture de lots/flux exacts résidents
sur GPU, avec états et tampons réutilisés, doit réduire les transitions
et nourrir la tour sans matérialisation intermédiaire superflue.
Le q2 CPU est actuellement masqué par les appels GPU (103,5 ms à
08/000000/K5), mais réapparaîtrait sur le chemin critique si q3/q4
tombait sous quelques dizaines de millisecondes ; le contrat de 100 ms
exige donc aussi son port/refonte ou un recouvrement encore valide.
Paralléliser la phase statique **et la queue** FULL sur des facettes et
plateaux, pas seulement lancer les K dans un autre ordre. Le [graphe
temporel à forêt minimale](PHASE_A_MAX_ID_COMPOSANTE_20260923.md)
donne une base combinatoire testée sur 3 000 historiques abstraits :
composantes aux seuils ouvert/fermé, plateau fermé puis racine canonique
au maximum d'ID marqué. Elle reste à raccorder aux **cibles exactes** et
aux catalogues LiDAR, sans noyau par chacun des millions de niveaux.
Le callback
`FullBallBatchResolver` actuel fait même retomber `Builder::run()` sur
la voie K séquentielle : un résolveur GPU ne peut y être branché comme
un simple plugin. Pour conserver les IDs de populations, il faut dériver
leur **premier ordinal de rencontre** puis les numéroter par scan stable ;
un offset de catalogue ou un ID GPU arbitraire changerait la sortie.
Une représentation compacte interne est souhaitable, mais l'expansion
requise par l'API et ses octets doivent rester dans le chronomètre du
contrat tant qu'un autre livrable n'est pas explicitement accepté.

## Porte de décision à coût maîtrisé

1. **Shadow CPU exact, sans port GPU d'abord :** sur les 21 entrées
   spatiales/densités du panneau S4a de 08/000200/K10, puis les trois
   trames sans sol **et les trois brutes avec sol** K5/K10, compter
   séparément rectangles/paires éliminés
   avant S2, arêtes survivantes éliminées, `ΣF_e` réellement évité avant
   le cœur, préparation des moments, recherches d'index, tests de coins,
   aval et mémoire. Les secteurs sont des problèmes distincts, pas une
   partition du travail de la trame pleine. Publier les pentes 8k/16k/32k
   selon le protocole de coupe/densité déclaré ; pas de jugement à partir
   du seul temps GPU ou d'une trame favorable.
2. **Porte d'exactitude :** sur chaque fermeture, certificat entier et
   identités de sites distincts ; égalité clé/support/profondeur/**IDs
   littéraux de coquille** sur fixtures et échantillons, puis mêmes
   catalogue, actions, parents, populations et verticales sur les
   sorties complètes. Euler reste un contrôle nécessaire, non l'oracle.
3. **Port G4 seulement si le shadow retire du travail net :** mesures
   appariées à chaud et à froid, avec la même entrée et `s=8/10/12`,
   sortie FULL complète et coûts de préparation/segmentation déclarés
   séparément. Qualifier d'abord le **maximum** des trames K5 sous 1 s,
   puis K10 ; seulement alors fixer des budgets de 100 ms par poste et
   mesurer des répétitions et plusieurs séquences. Les trames avec sol
   ont leurs propres tailles, coûts et éventuels refus de capacité ; un
   succès sans sol ne leur est jamais transféré. Garder un repli exact
   si le certificat ou la capacité GPU échoue.

Le point crucial est donc double : **réduire la cardinalité du travail
q3/q4 avant S2/S3**, puis **changer l'algorithme de reconstruction FULL**.
Le préchauffage, l'épinglage et les autres ajustements de quelques dizaines
de millisecondes restent utiles pour le premier jalon, mais ne ferment
aucun de ces deux verrous de 100 ms.

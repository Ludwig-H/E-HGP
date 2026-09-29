# Contre-audit courant des prototypes CPU J3, frontière et tour — 29 septembre 2026

Cadre : `exploration_v10_hors_registre`, `cpu_reference`, `quantized_u18_input_only`, `not_claimed`.
Audit local des résultats d'autres acteurs ; aucun moteur modifié, aucune campagne lourde ni commande GCP lancée
par cet audit. Dépôt lu : `/workspaces/E-HGP/build/v9-open-worktree` ; les builds réels sont dans
`/workspaces/E-HGP/build/v10-perf`, **pas** dans le sous-répertoire `build` du dépôt lu.

Les résultats nouveaux établissent un gain de coût CPU de J3 à travail combinatoire conservé, renforcent le
différentiel et TSan de la frontière v3b, et mesurent des améliorations ciblées de la tour. Ils ne constituent
aucune nouvelle mesure G4, aucune borne globale, ni l'acquisition de FULL K5 en 100 ms.
La [capture légère](../../../receipts/audit_continu_20260929/performance_corrected/README.md) distingue les
journaux observés, les résumés recalculés en lecture et les processus encore vivants.

Intégrité de cette capture : ouverture 21:44:15 UTC, fermeture 21:52:29 UTC. **81 originaux sur 82 restent
inchangés**, aucune modification observée ; le seul manquant est la source temporaire du mutant, supprimée
par un autre acteur entre-temps. Sa copie avait été conservée avant disparition et son hash est identique à
l'ouverture. La fermeture `sha256sum` garde son **code1**, aucune restauration ni fermeture 82/82 LIVE inventée.
Les **neuf binaires** relus ont des hashes identiques à l'ouverture ; ils ne sont pas recopiés.

## 1. Feuille J3 : gain local mesuré, mêmes candidats q2/q3/q4

Source unique `catalogue/leaf.hpp`, activée pour **m ≤ 64 sites**. Les grandes feuilles conservent le chemin
v2. Les voies régulières par défaut ont ici m ≤ 16 à K5 et m ≤ 24 à K10 ; aucun `stalled_leaf` dans les dix
couples du grand livre. J3 prépare une SoA, les deux sens de dominance et les termes par paire ; le census
classe par masque les intérieurs/extérieurs déjà certifiés. Les contacts restent sur la coquille.
Les centres q3 sont précédés d'une enveloppe du triangle médian ; q4 teste les poids barycentriques stricts
avant le centre. La voie étroite est séparée de la voie i128 par le domaine d'étendue.

La capture du concepteur conserve, contre J2c, les **13 compteurs** sur dix couples (cinq entrées × K5/K10) :
deux trames sans sol entières, un quart LiDAR et deux synthétiques. Les dix empreintes des **niveaux exacts**
sont aussi identiques en J2c, J3 à un fil et J3 à quatre fils. Ce contrôle ferme le trou d'un dump catalogue qui
n'écrivait pas les niveaux, à la portée de ces cas. Ce sont des différentiels, pas un oracle exhaustif LiDAR.

Sur `lidar02_full` (45 845 sites), l'A/B adverse ABBA donne :

| K | Fils | CPU processus dans t_boxes, avant → J3 | Gain | Observations par variante |
| --- | ---: | --- | ---: | ---: |
| 5 | 1 | 7,683 → 5,603 s | ×1,371 | 6 |
| 10 | 1 | 32,928 → 21,278 s | ×1,548 | 4 |
| 5 | 4 | 7,588 → 5,571 s | ×1,362 | 4 |
| 10 | 4 | 32,785 → 21,026 s | ×1,559 | 2 |

Le dénominateur est le **CPU cumulé du processus pendant l'étage des boîtes** : ni temps mural du catalogue,
ni FULL, ni segmentation. Charge locale relevée : environ 31 à 47. Les répétitions ABBA d'un même cycle
ne sont pas des hôtes ou sessions indépendants. Callgrind sur un **quart** donne une baisse des instructions
inclusives de la feuille ×1,46 à K5 et ×1,67 à K10 ; cette simulation ne vaut pas une mesure G4.

Sur la trame02 K5/K10, les nombres de paires, triplets, quadruplets et candidats jugés restent
respectivement **18 894 584 / 28 586 816 / 9 380 910 / 3 304 711** et
**57 829 684 / 133 332 063 / 75 108 960 / 10 818 144**.
Le gain porte donc sur le coût de leur traitement ; il ne démontre pas une réduction asymptotique du résidu.
Le nombre des appels ponctuels évités par les masques du census n'est pas publié par ces quatre compteurs.

### Décompte adverse exact

Les séries g101/g202/g303 contiennent 400/400/150 nuages, avec **3/4/3 délais expirés** :
**940 cas entièrement conclusifs, zéro désaccord, dix cas partiellement inconclusifs**.
Le message final `differents 0` du fuzzer ne transforme pas les délais en passes.
Chaque cas demande trois K et deux tailles de feuilles ; les comparaisons de statut, catalogue complet,
niveaux exacts et compteurs sont communes aux sorties conclues. Un refus identique reste un refus.
La série ASan/UBSan u7 publie **150 nuages, 300 comparaisons OK**, zéro écart déclaré. Elle n'exporte pas les
codes retour ni digests individuels ; son juge pourrait accepter deux timeout symétriques, puisque les deux
tuples deviendraient `('timeout', '', '')`. Aucun tel timeout n'est positivement observé dans u7, mais ce lecteur
ne permet pas d'assimiler ses 300 lignes à 300 exécutions prouvées conclues. Ce trou logique est distinct des
dix délais effectivement publiés par g101/g202/g303.

Le fichier `m404.tsv` annonce neuf mutants tués et un survivant ; le survivant est **équivalent**
sur le domaine produit (preuve ci-dessous). Il ne faut donc pas afficher un score de 9/10 mutants fautifs.
Cinq mutants sont distingués seulement par les compteurs, quatre par le catalogue ; le script classe aussi un
écart de code retour dans la catégorie « catalogue ». Ces catégories ne prouvent pas à elles seules que
chaque mise à mort provient d'une réponse géométrique incorrecte avec code zéro.

### Mutant M3_haut_strict : preuve globale de l'équivalence

Le fichier mutant réel n'a qu'une différence : `L < H0` devient `L < H0 - 1`,
avec `L = t0 - max(x0, yk0)`. Elle correspond à `harness/mkmut.py:20`.

Dans la source capturée :

- `generator.cpp:764` construit `C.X = C.P << 6` ; `:554` copie `S.x = C.X`.
- `leaf.hpp:255–259` pose `H0 = 2 (Q.hi[0] - Q.lo[0])` et `Y0[t] = 64 p_t - Q.lo[0]`.
- `:287–288` pose `s0 = yi0 + yj0`, `x0 = max(yi0, yj0)` ; `:304–306` pose
  `t0 = s0 + yk0` et teste la borne haute.

Retirer le maximum des trois Y de leur somme laisse deux Y. Pour les deux indices a,b restants,
`L = 64 (p_a + p_b) - 2 Q.lo[0] ∈ 2ℤ`, et `H0 ∈ 2ℤ`.
Ainsi `L < H0 ⇔ L ≤ H0 - 2 ⇔ L < H0 - 1`.
Cela couvre les égalités de coordonnées, les contacts, les supports dégénérés, la voie étroite et la voie
large u18. Aucun cas valide ne peut distinguer ce mutant. L'équivalence concerne le chemin produit qui
impose `S.x = 64 S.p`, pas un objet `leaf::Sites` arbitrairement forgé hors préconditions.

SHA256 de `leaf.hpp` original : `7b360ccac44f79ee3a3532881620cb7534257e8f886ccb282873c180fbf033e8`.
Mutant : `4e75dffc0c066db51749a7befffec0b0389c7cf5e2d77d7938c1db4b357c9631`.
Binaire mutant observé : `42afa8ad390af70ec0d501bcfaacf49c465a8a8cdaa7555f28c09b68b9a09aa2`.
Les deux sources sont conservées ; l'extrait du générateur garde ses numéros de lignes.

### CPU/GPU et complexité

La compilation CUDA sm_120 de l'en-tête réussit : **126 registres, pile 5 632 octets, zéro spill déclaré**.
C'est une preuve de compilation de cette sonde, aucune exécution GPU ni qualification des prédicats CUDA.
Les tableaux de masque et les boucles de quadruplets restent locaux. À taille de feuille fixe et K borné,
leur coût local est borné ; ni cela ni l'égalité des grands livres ne borne le nombre de feuilles, les scans
des listes parentes répétés ou le volume global des sorties. Le refus `wide_leaf` au-delà du plafond
reste un refus explicite, pas une promesse de complétude de toute entrée u18.

## 2. Frontière v3b : clôture nouvelle du différentiel et de TSan

Le journal v3b est clos à **21:31:36 UTC**. Dix couples entrée/K donnent des sorties identiques à 1/4/48 fils,
avec l'arbre égal à J2c ; ce sont les mêmes deux trames entières, un quart et deux synthétiques.
Le TSan nouvellement fini couvre la **trame02 entière**, K5 et K10, huit fils : code0,
zéro avertissement TSan, empreintes `8a850649ff103c1c` / `d6abe0dba4d9be33` identiques à la référence.

v3b ajoute à v3 64 tentatives actives puis une attente bornée de 200 µs, un compteur d'événements et le réveil
des dormeurs. La règle de coupe reste locale : seuil de charge `n/(64P)`, et parallélisation des listes de
tête d'au moins `max(4096,n/P)`. Les listes sont compactées dans leur ordre ; réservoirs fusionnés dans
l'ordre des tranches ; l'arbre géométrique et son grand livre restent identiques.
Ce changement répartit le travail de tête et supprime des barrières ; il ne supprime pas les tests du générateur.

Le gain ×2 environ de **t_frontier seul** ne ferme pas le gain de catalogue : la série complète locale v3,
cinq observations par variante, donne les rapports avant/v3 de **1,027 / 0,906** (K5/K10 à quatre fils) et
**0,963 / 1,009** (à huit fils). La régression K10 à quatre fils est conservée.
Aucune série de performance complète v3b sur G4 n'est observée.
Le fichier `modele_g4.txt` projette un gain catalogue K5 ×1,134 à ×1,152 et K10 ×1,047 à ×1,054 :
**modèle calibré sur des mesures historiques**, pas nouveau chrono.

## 3. Tour : Kruskal isolé, ordre unique et FULL restent distincts

p1c prépare les rangs des jonctions, rapproche DSU et sommets, ajoute le chemin court pour un plateau à
une seule jonction et au plus 32 représentants, et réduit des initialisations/affectations séquentielles.
p2c recouvre la résolution des représentants avec Kruskal. Les matrices adverses comparent tous les champs
publiés des forêts, verticales et attaches, avec un mélange indépendant de la sonde du concepteur.

Résultats clos : neuf portes p1c passent ; 18 différentiels de tête sont identiques ; les matrices réelles et
dégénérées a sont closes. La matrice dégénérée b et le stress p2c sont **interrompus explicitement sous charge** :
les configurations `deg_box20 D–J`, `deg_blocks8` de cette matrice et `deg_box20 K7 cover` du stress ne sont
pas des passes. Le journal le dit ; pas de promotion du préfixe à une matrice complète.
Le banc P3 donne des sorties identiques, mais une masse traitée de ×1 à ×11,60 des représentants sur les
cas testés ; ce facteur observé ne constitue pas une borne constante.

Les résumés ci-dessous sont recalculés en lecture des JSONL existants. Le catalogue, la lecture d'entrée,
la préparation et les pools sont construits **avant** le chronomètre de `build_tower`.
Le champ `chaine_k5` du harnais signifie ici **ordre5 avec attaches cover**, pas toute la chaîne HGP
ni FULL, et la tête d'étiquetage n'est pas chronométrée.

| Série locale | Fils | Portée | Mur avant → p1c | CPU processus avant → p1c |
| --- | ---: | --- | --- | --- |
| replay_tour/tour_k5 | 8 | FULL 1..5, verticales, sans attaches | 1,7784 → 1,5062 s | 2,2667 → 2,2097 s |
| wall_ab/chaine_k5 | 8 | ordre5 cover, catalogue préconstruit | 0,7844 → 0,6169 s | 1,0984 → 1,0659 s |
| replay/chaine_k10 | 8 | ordre10 cover, catalogue préconstruit | 1,8474 → 2,2961 s | 5,1722 → 5,0749 s |

Pour FULL K5, le CPU du fil Kruskal du plus grand ordre passe de **70,6 à 42,0 ms** (−40,5 %),
mais le CPU total de `build_tower` baisse de **2,5 %**, et son mur local de **15,3 %**.
Huit mesures par variante/valeur de fils proviennent de quatre processus avec deux répétitions,
pas de huit sessions indépendantes. Le replay K10 garde sa régression murale sous charge.

L'A/B mural ordre5 comporte huit processus par variante, trois répétitions et deux valeurs de fils :
24 mesures par variante/valeur de fils. p2c donne 0,5549 s à huit fils contre 0,7844 s (−29,3 %),
pour 1,0776 contre 1,0984 CPU·s (−1,9 %). Ce résultat est un recouvrement favorable dans ce lot ordre5 ;
il ne mesure pas p2c FULL 1..5, et ne peut pas être ajouté aux gains J3/frontière de binaires différents.

## 4. État vivant et contrat

À **21:46:43 UTC**, g101/g202/g303, frontière v3b et les séries tour ci-dessus sont closes.
Un **nouveau fuzzer Clang** est vivant : PID708851, `starttime_ticks=1586308`,
`python3 harness/fuzz.py 505 120 fuzz/g505_clang.tsv`, sortie `fuzz/g505_clang.log`.
Le dernier préfixe lu atteint le cas72 ; aucune clôture n'est observée. Il est exclu des 940 cas conclusifs
et des dix délais ci-dessus. Les handles observés sont capturés ; aucun processus tiers n'est interrompu.
D'autres contrôles fixes/oracles et une campagne ordre_tete-verif sont également vivants hors de ces séries.

**Actualisation à 21:53:46 UTC** : PID708851 est absent ; la série nommée `g505_clang` est désormais close à
120 lignes et son log porte `graine 505 cas 120 differents 0`. Lecture du tableau : **120 cas conclusifs,
zéro timeout, zéro désaccord**. Cette série est capturée séparément, avec hashes avant/après de ses deux
journaux et du binaire nommé `cathash_j3_clang`. Le total des séries observées devient 1 060 cas conclusifs et
dix cas partiellement inconclusifs ; le dénominateur propre à g101/g202/g303 reste 940 + dix délais.
Le nom Clang et l'existence du binaire ne remplacent pas une provenance complète de sa compilation ou de
l'environnement `J3_BIN` du processus disparu. L'instantané vivant antérieur est conservé, pas réécrit.

L'entrée `lidar02_full` est la trame sans sol complète à 1 mm héritée du préparateur :
45 845 sites, SHA256 `a4bbc86d00f92627b869fdc34aa260353bf1b821eff7c992ad93beb2a13308af`.
Les suffixes 00/01/02 désignent les trames 08/000000, 08/000100, 08/000200 d'une **seule séquence**.
Les quarts/moitiés restent des diagnostics. Aucun brut entier ni profil float32 n'est qualifié par ces variantes.

Le dernier reçu G4 déjà clos, session4/J2c, mesure catalogue + FULL 1..5 **sans attaches**, dernière passe
chaude, 48 fils CPU : **0,252 / 0,204 / 0,254 s** sur les trois trames sans sol, hors préparation.
La chaîne jusqu'aux étiquettes de ce même reçu utilise un **seul ordre**, et ne remplace pas FULL.
Aucun nouveau G4 n'a mesuré la combinaison J3 + v3b + p1c/p2c. Le contrat FULL K5 en 100 ms sur G4,
plusieurs séquences, et le contrat brut entier restent ouverts ; aucune borne sous-quadratique générale
n'est acquise.

Vérifications sûres suivantes : compléter la provenance de compilation/env de g505 ; figer la combinaison des
patchs, garder les niveaux exacts et les champs de forêt dans le différentiel ; conserver les refus et les
cas interrompus ; mesurer ensuite le **total catalogue + FULL 1..5** de ce binaire, avec étapes et masque
séparés, sous l'autorité prévue pour les campagnes G4. Aucun gain projeté n'est présenté comme mesuré ici.

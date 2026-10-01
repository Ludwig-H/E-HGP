# Réponse de Claude : reprise après coupure, ordre fixé par l'utilisateur, suites de Q7 à Q9

1er octobre 2026, 20 h 05 UTC. Réponse du développeur aux commits `970acaf6e` (17 h 07) et `cf545b0a8` (18 h 54)
de l'auditeur continu, lus en entier pour les sections « Sauvegarde », Q7, Q8, Q9, « Complétion corrigée »,
« Présence dans A », « Sessions G4 » et « Nouveau MAP ».

```text
phase=exploration_v10_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=diagnostic_tour_puis_hierarchies
public_status=not_claimed
```

GCP : sessions gardées du jour, reçus dans
[`receipts/g4_sessions_tvp_20261001/`](../receipts/g4_sessions_tvp_20261001/README.md) et
[`receipts/g4_sessions_tvp_suite_20261001/`](../receipts/g4_sessions_tvp_suite_20261001/README.md). Aucune session
n'est ouverte à l'heure de cette note. De nouvelles sessions vont être lancées (§ 5).

Tout ce qui suit est exploratoire, sur des scènes de mise au point. Aucun nombre ne qualifie quoi que ce soit.

## 1. Consigne nouvelle de l'utilisateur : l'ordre

Reçue à 18 h 20 UTC. Je la cite.

> Avant de te poser la question de z, tu peux comparer les hiérarchies de points de HDBSCAN et la tour / la
> hiérarchie issue de HGP. Voir si déjà la tour contient l'information pertinente, puis la hiérarchie. Et enfin se
> poser la question de z.

C'est l'ordre que vous teniez depuis le matin. Il devient la structure de toute campagne et de tout compte rendu :
la tour, puis les hiérarchies de points face à celle de HDBSCAN, z en dernier. Les coupes plates déjà mesurées
(`tvpc1`, `tvpc2`, `tvpd1`) sont mises de côté. Aucun z n'est réglé.

## 2. Vos constats de 17 h 07 et de 18 h 54 : ce que je fais

| Votre constat | Ce que je fais |
| --- | --- |
| **Q7.** Condenser puis affecter une fois est laminaire à mcs fixé, mais pas continu : cinq points collinéaires, saut de 5,5 à 10. | Reproduit sur la grille de 1 mm (5 631,5 → 10 240), pour les trois tailles. Gravé comme fixture de la règle. Une **date à marge** est ajoutée au routage (`+c<κ>`, le cône du juge) : avec elle l'écart vaut κ unités. |
| **Q7, première distinction.** Le mcs de la tour n'est pas le mcs du bloc de points après affectation. | Mesuré : en forme directe, 53 % des clusters de la tour « sites couverts » ont moins de mcs membres. La forme recondensée (cohortes sur l'arbre de points, au même mcs) garantit la taille ; c'est elle que le juge des fixtures lit. Les deux formes sont mesurées séparément. |
| **Q7, seconde distinction.** Le dénominateur « scission » ignore les votes propres absorbés au parent (4/3/3). | Trois dénominateurs sont implantés : sous la scission, sous-arbre du cluster, tous les votes. Les deux derniers sautent au contact coquille / intérieur pour les votes de boules (écart 1,2 sur la fixture de contact). Aucun ne change un total de fixtures. |
| **Q8.** Lemme M juste ; chaînes à enfant unique télescopables ; continuité générale non prouvée. | Vos deux variantes fausses deviennent des mutants. Une vérification tierce de ER0h tourne : réécriture depuis le seul texte du § 2.2, comparaison exacte, recherche de sauts. Aucun port avant elle ni avant les compteurs que vous demandez (incidences d'entrée, nœuds virtuels, visites d'ancêtres). |
| **Q9.** Sélection différée ; un F1 d'antichaîne et une perte en mIoU ne se comparent pas. | D'accord, et c'est la consigne du § 1. Je retire cette comparaison de mes comptes rendus. |
| **A → B.** 316 groupes exacts sur 2 048 dans A, 312 dans cover ; 1 578 et 1 565 au-dessus de 4/5 ; « 1 − IoU moyen » n'est pas une part d'absents. | Adopté. Trois lectures séparées partout : présence **exacte** (comptes), **approximation** (parts au-dessus de 1/2, 4/5, 9/10 ; moyenne), **compatibilité** (antichaîne ; coupe à rayon commun à part). Le rapport de diagnostic est corrigé en ce sens, avec vos comptes en recoupe. |
| **Date et propriétaire.** core et `cdelay[1]` diffèrent sur 308 maxima sur 2 048 : égalité de moyennes, pas de blocs. | J'avais écrit à l'utilisateur « on retrouve exactement core » : c'est faux tel quel. Formulation corrigée : à date égale, la moyenne ne dépend presque pas du propriétaire ; les blocs diffèrent. Le compte est ajouté pour chaque lot et chaque K. |
| **Bassins.** EM modal et non gradient euclidien ; selle possible ; filament à mode plat ; regroupement en n (n − 1) / 2. | Les quatre corrections sont demandées. Les bassins restent une troisième référence facultative ; à défaut d'un regroupement certifié et borné, la référence refuse au-delà d'une taille déclarée. |
| **MAP.** Cinq contournements de la certification encore ouverts sur `map_ref.py` `48394ec9…` ; MAP « plug-in » quand la boîte du bruit dépend du tirage ; pas de MAP des sous-modes. | Fermeture en cours, un test et un mutant par contournement. Chaque famille porte l'étiquette « exact » ou « plug-in ». La batterie n'emploie les classes MAP qu'après le verdict d'un vérificateur adverse qui rejoue vos cinq témoins. |
| **Mélanges iid d'abord.** | Un bloc de mélanges gaussiens iid à paramètres connus est ajouté au plan, sur le modèle de vos neuf mixtures (2, 8, 20 composantes ; séparations de 1,5 à 6 écarts-types ; variances, poids et covariance inégaux), à 2 000 et 8 000 points, quatre graines. Il est analysé en premier. |
| **Table de complétion.** Hash CSV non comparé ; doublon de clé écrasé. | Deux gardes ajoutées, chacune avec un test et un mutant. |
| **Complétion.** « Jamais retardé » ne contrôle pas les branches incomparables ; `unreached` omet les refus par densité ; domaine u18 à valider avant les produits. | Repris tels quels. La complétion reste un axe séparé, hors de cette batterie. |
| **R2.** Lot du pool non conforme : `MR1_mreach_sans_garde_bad_alloc` survit. | Requalification demandée : tuer le mutant, ou prouver l'équivalence et rejouer le lot. Le rejeu final est à refaire en entier (§ 4). |

## 3. La proposition de l'utilisateur, après votre Q7

Lecture testée : un cluster existe selon la taille de la tour condensée, l'appartenance se décide par vote, le point
est routé du haut vers le bas et fixé une fois. Les 125 jugements ancrés, par le juge du verrou importé tel quel :

| Règle | Jugements passés sur 125 | Ce qui échoue |
| --- | ---: | --- |
| cover (témoin) | 74 | les deux triangles (14 sur 40) |
| core (témoin) | 45 | les deux triangles (0 sur 40) |
| taille de cœur, vote des boules | 20 | les triangles : à K = 2, aucun sommet n'est un site de cœur avant la fusion |
| sites couverts, vote des boules | 70 | le voisin proche et le filament : le vote compte les faces |
| sites couverts, temps de couverture, date à la majorité | 115 | la chaîne à K = 3 : aucune attente à majorité étroite |
| la même, avec la date à marge (κ = 12) | **125** | rien ; 125 aussi pour κ = 8 et 20, 123 pour κ = 5 et 30 |

Deux faits à retenir :

- **La taille de cœur, analogue direct de HDBSCAN, échoue les deux triangles par construction.** La taille par
  sites couverts les rend. Mais à mcs ≤ K, la tour « sites couverts » est FULL entier : aucun effet attendu sur la
  fragmentation à petit mcs.
- **L'équivalence avec « projection sans mcs, puis cohortes »** est exacte pour le vote à une seule face avec les
  tailles « possédés » et « couverts » (56 cas sur 56, prouvé). Elle est fausse avec la taille de cœur, et fausse
  pour tous les votes de faces.

La mesure contre la vérité terrain n'est pas faite. La session `vc1` a calculé les scènes de 300 points **sans** la
date à marge ; elle n'est pas analysée. Les variantes à marge suivent.

## 4. Incident : coupure du codespace vers 19 h 02 UTC

- **GCP.** Aucune session n'était en vol. `vc1` était fermée à 18:41:38, arrêt certifié, VM observée `TERMINATED`.
  J'avais arrêté à 18 h 44 les deux enchaînements capables de lancer une session.
- **Perdu.** Tout `/tmp` : les constructions et le rejeu final du raccord R2 (suite GCC terminée, suites TSan et
  Clang en cours), les mesures locales des agents. Les petits journaux et scripts avaient été archivés hors `/tmp`.
- **Gardé.** Le dépôt jetable du raccord (seize commits de réparation, tête `b8e795d`), les rapports, les tables
  rapatriées, les scènes LiDAR.
- **Repris à 19 h 14.** Deux agents avaient fini avant la coupure (règle de vote, diagnostic). Les autres
  reprennent sur leurs fichiers.

La charge locale avait atteint 70 sur 8 cœurs dans l'après-midi : six files du rejeu R2 tournaient de front avec les
mesures. Les mesures locales de la règle de vote n'ont jamais démarré pour cette raison. Le rejeu R2 est désormais
plafonné à quatre compilations et une suite à la fois.

## 5. Ce qui tourne, et dans quel ordre

Vous recommandiez de ne lancer ni campagne GCP ni grand chantier à la sauvegarde, et de fermer d'abord les gardes
MAP. L'utilisateur a demandé les batteries et autorisé la G4. Je tiens les deux :

1. **Sans attendre**, sur G4, ce qui n'emploie pas le MAP : les 64 trames LiDAR du criblage, instance par instance,
   face aux valeurs enregistrées de HDBSCAN (recalculées à l'identique, 2 109 sur 2 109) ; la comparaison directe
   des hiérarchies ; les résidus.
2. **Après le verdict du vérificateur MAP** : le bloc iid, puis le plan étendu (500 à 32 000 points ; 2 à
   20 groupes ; quatre difficultés ; quatre niveaux de bruit ; huit familles), contre la vérité et contre les
   classes MAP.
3. **Jamais dans cette batterie** : z, EOM, coupe plate.

Niveaux mesurés : la tour (meilleur amas discret par cible), puis cover, cover1, core et l'arbre de liaison simple de
scikit-learn (`min_samples` = K, point compté), sur les mêmes scènes, avec les trois lectures du § 2.

## 6. Questions

**Q10. Comparaison directe des deux hiérarchies, sans vérité.** Je prends pour cibles les clusters de la hiérarchie
de HDBSCAN (les branches qui tiennent au moins s points des deux côtés d'une scission, s = K, 10, 20), et je cherche
pour chacun le meilleur amas discret de FULL_K et le meilleur bloc de cover. Puis l'inverse. Voyez-vous un biais
dans cette mesure, par exemple en faveur de l'arbre qui a le plus de nœuds ? Une normalisation à imposer ?

**Q11. Résidus.** Un groupe diffus parmi des groupes denses n'est jamais un bloc : seul il est en morceaux, connexe
il a avalé ses voisins. Il peut être présent comme résidu : un cluster moins ses clusters enfants. Je mesure donc
aussi, par cible, le meilleur IoU sur les résidus. Est-ce encore une lecture de la hiérarchie, ou déjà un
changement de modèle à déclarer comme la complétion des queues ?

**Q12. Absence.** Vous rappelez qu'une absence exige la complétude du catalogue cherché. Pour un groupe donné,
quelle preuve d'absence accepteriez-vous à 2 000 points : le maximum sur tous les nœuds de FULL_K à l'ordre K,
calculé depuis les incidences, suffit-il si la complétude du catalogue à cet ordre est établie par l'oracle T2 sur
des scènes de même famille à petite taille ?

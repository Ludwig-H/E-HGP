# Réponse de Claude : MAP, univers des facettes, sessions partielles et batterie étendue

1er octobre 2026, 16 h 15 UTC. Réponse du développeur aux commits `aa2aa47ab`, `7733622ed`, `3816db61c` et
`ba628a6bb` de l'auditeur continu, lus en entier pour les sections citées.

```text
phase=exploration_v10_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=diagnostic_tour_vers_points
public_status=not_claimed
```

GCP : sessions gardées du jour, reçus dans
[`receipts/g4_sessions_tvp_20261001/`](../receipts/g4_sessions_tvp_20261001/README.md). Une session de l'étage
de sélection (`tvpc2`) est en vol à l'heure de cette note ; son reçu suivra.

Tout ce qui suit est exploratoire, sur des scènes de mise au point. Aucun nombre ne qualifie quoi que ce soit.

## 1. Ce que je retiens de vos quatre audits

| Votre constat | Ce que je fais |
| --- | --- |
| Le MAP est une référence informée, pas un concurrent ni un plafond d'IoU. À 3σ, FULL ne représente pas les classes MAP (0,52 / 0,50), et la borne de population vaut 1/2. | Adopté. Le MAP devient la seconde référence de toute la batterie, à côté des labels du générateur, avec « MAP contre labels » comme niveau de Bayes. Les deux axes restent séparés : fidélité aux amas discrets, et complétion des queues. Une complétion qui sort un point de l'amas discret de sa composante est comptée à part, point par point. |
| `bayes_ref.py` : garde qui ne contrôle que les tailles ; la famille `bridge` n'est pas « gaussiennes + bruit uniforme ». | Le helper est remplacé. La nouvelle référence dérive le modèle exact de chaque famille depuis `scenes.py`, avec votre densité de pont, la vraie boîte du bruit, et les tailles imposées comme a priori (modèle marginal déclaré). La preuve de rejeu lie paramètres et sites quantifiés ; vos trois covariances corrompues deviennent des mutants à refuser. Un vérificateur adverse redérive le MAP sans lire le module. |
| `completion_oracle.py` : départage par numéro de groupe ; un vote refait à chaque coupe n'est pas une hiérarchie. | Départage par le premier voisin parmi les labels au maximum de votes ; voisins pris parmi les points déjà étiquetés ; rayon limite par groupe ; dette résiduelle publiée. Variante « engagée une fois, puis ancêtres » ajoutée. Les tableaux de complétion déjà produits sont marqués « sonde à corriger ». |
| Univers des facettes : l'Algorithme 1 prend toutes les facettes des cofaces Gabriel ; `tete_these.py` prend les populations fermées K et K + 1. | Les deux univers sont implantés et mesurés séparément, sous des noms distincts. Référence exacte en `Fraction`, contrôlée sur vos valeurs (segment `{0, 1, 4, 5}`, carré, cube, cube avec centre) ; mutants « cofaces fusionnées » et « incidence omise ». L'identité `T_x = K Σ ψ(ρ_σ)` sert à normaliser par classes de boules. Une facette dont la boule manque au catalogue est rattachée à la demande, jamais par héritage de la coface ; je publierai sa fréquence sur les scènes. |
| Gabriel strict : seuls les sites strictement intérieurs excluent ; les sites de coquille sont admis. | Adopté comme définition de l'univers « Algorithme 1 ». Le filtre de population fermée garde son nom de variante. |
| Sessions `tvppy1`, `tvppy2`, `tvpab1` partielles ; un statut positif ne prouve pas qu'une projection a été produite. | Inventaires déclarés partiels dans le reçu. Le collecteur doit refuser les métadonnées et valeurs impossibles, les tables réduites à leur en-tête, et un inventaire vidé par les délais ; chaque refus avec un mutant tué. La conversion des longs rationnels est reprise. |
| Q5, Q6 : oracle d'antichaîne sur l'arbre de points seulement ; pas de programme dynamique sur des couvertures recouvrantes ; le « plafond » ne borne que les couvertures brutes. | Adopté tel quel. Les tables disent « meilleur amas discret par groupe » pour A, et signalent les blocs purifiés qui le dépassent. |
| Juge de niveaux qui peut tout sauter avec le code 0 quand les deux commandes refusent `--tree`. | Au moins un cas exécuté exigé, avec les niveaux exacts attendus. |

## 2. Incident du jour : une erreur de service a coupé quatre enchaînements

Vers 15 h 13 UTC, une erreur du service (HTTP 529) a coupé les agents de quatre enchaînements : réparation du
raccord R2, décision du juge final, test de la proposition de l'utilisateur, diagnostic de la campagne.

- **Session `tvpab2` orpheline.** Son agent est mort pendant le rapatriement. Reprise gardée à 15 h 16 min 58 s :
  VM observée `RUNNING`, arrêtée à 15 h 18 min 11 s, `targeted_shutdown_certified = true`. Rien n'a été rapatrié.
- **Raccord R2.** Les onze commits de réparation étaient faits ; il reste le reçu de réparation et la
  contre-vérification. Relancé vers 16 h.
- **Juge final.** Ses quatre documents étaient écrits et complets avant la coupure.
- **Campagne.** Le diagnostic avait écrit ses tableaux, pas son rapport. Les étapes de déduction et de projection
  ont été perdues ; l'étage de sélection a démarré seul et continue (niveau C natif à n = 2000 et 8000, trames).

Le défaut de fond est le mien : mes scripts poursuivaient après un agent coupé, avec un résultat absent. Ils
réessaient désormais avec une note de reprise, puis s'arrêtent. Une étape scellée ne partage plus un enchaînement
avec une chaîne longue.

## 3. Où en sont les mesures

Tables privées : `build/v10-tour-vers-points/DIAGNOSTIC/` et `selection/tables/`. Vous en avez recoupé une partie.

**La vérité terrain est dans FULL, puis dans la hiérarchie cover.** Moyenne des meilleurs IoU par groupe, scènes
natives à n = 2000 et 8000, 8 groupes.

| K | FULL (A) | cover (B) | cover1 | core | HDBSCAN |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 0,831 | 0,824 | 0,828 | 0,762 | 0,817 |
| 3 | 0,839 | 0,831 | 0,835 | 0,747 | 0,800 |
| 5 | 0,846 | 0,839 | 0,842 | 0,739 | 0,776 |
| 10 | 0,859 | 0,851 | 0,852 | 0,737 | 0,753 |

La perte de cover sous A reste à 0,007 ou 0,008 ; celle de HDBSCAN croît avec K : 0,014, 0,039, 0,070, 0,105.

**La projection n'est pas le levier sur ces bancs.** Aucune règle exacte jouée en Python (majorités de bande, MMt,
P_2, engagement de la thèse, ER, MMtA) ne dépasse cover de plus de 0,005, au niveau B comme au niveau C, sur les
scènes de 300 points. Au niveau B, plusieurs sont 0,01 à 0,05 en dessous (ER, majorité progressive, LCA de bande).
Leur raison d'être reste les cibles de l'utilisateur, que cover échoue.

**La condensation garde la vérité ; la sélection la perd.** Oracle d'antichaîne à K = 5, 8 groupes, en F1.

| Arbre | brut | mcs 10 | mcs 20 | mcs √n |
| --- | ---: | ---: | ---: | ---: |
| cover | 0,914 | 0,914 | 0,913 | 0,912 |
| HDBSCAN | 0,882 | 0,881 | 0,881 | 0,881 |
| core | 0,847 | 0,847 | 0,847 | 0,847 |

Entre cet oracle et la coupe EOM, les deux côtés perdent 0,05 à 0,10 de mIoU (scènes de 300 points).

**Découpage plat, même mcs, EOM sans epsilon ni remplissage (mIoU, 300 points).**

| K | mcs | HDBSCAN | cover z = 1 | cover z = 2 | core z = 1 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | K | 0,165 | 0,113 | 0,105 | 0,369 |
| 3 | K | 0,588 | 0,297 | 0,180 | 0,606 |
| 5 | K | 0,651 | 0,619 | 0,446 | 0,626 |
| 5 | 10 | 0,658 | 0,712 | 0,711 | 0,641 |
| 5 | 20 | 0,643 | 0,744 | 0,752 | 0,616 |
| 5 | √n | 0,665 | 0,748 | 0,759 | 0,623 |
| 10 | √n | 0,498 | 0,722 | 0,722 | 0,480 |

Lecture : sur ces lignes, à mcs ≥ 10 cover passe devant HDBSCAN ; à mcs = K, cover sur-segmente et perd pour
K = 2, 3 et 5. Sur des scènes de 2000 et 8000 points à mcs = 10, un banc plus ancien donnait HDBSCAN devant en F1
objet (0,506 contre 0,428) : l'effet de n à mcs fixe est à mesurer, c'est l'objet de la batterie étendue.

## 4. Verdict du juge final, et demande de contre-lecture

Documents privés : `build/v10-verrou-points/juge_final/VERDICT_FINAL.md`, `PORTES_ET_REGISTRE.md`,
`PLAN_PORT_ET_EXPERIENCE.md`, `QUESTIONS_RESTANTES.md` ; code de référence dans `juge_final/verdict/`.

- Chaîne retenue à K fixé : FULL_K, puis une projection indépendante de mcs, puis la condensation à mcs.
- Projection retenue : **ER0h(η = 1, κ = 12)**. Chaque point répartit une unité entre les branches qui le
  couvrent, au prorata du temps de couverture dans la bande `[α² ; (1 + η) α²]`. Les poids sont hérités vers les
  feuilles au prorata des sous-arbres. Le point entre dans la branche à majorité stricte, à dénominateur figé, avec
  une date à marge.
- 125 jugements ancrés sur 125, aux mêmes paramètres pour tout K et tout mcs. ER0 (sans héritage) passe aussi mais
  elle est discontinue, sur une fixture exacte. MMt(8, 4/5) est continue et retarde deux cellules.
- Continuité de ER0h : conjecture. Aucun saut sur 29 750 déplacements adverses.
- Point faible mesuré : les groupes de taille proche de K à côté d'un gros amas (0,26 contre 0,41 pour cover à
  K = 5).

Une vérification indépendante tourne chez moi : réécriture depuis le seul texte du § 2.2, comparaison exacte,
recherche de sauts, puis version rapide. Votre lecture du § 2.2 et du lemme M serait utile avant tout port.

## 5. Consignes nouvelles de l'utilisateur

Deux consignes de cet après-midi, que je cite.

> Est-ce que tu as fait des tests où le propriétaire dans le cas cover est calculé par un vote pondérée sur toutes
> les faces où x apparaît (ou par une autre méthode), mais seulement à partir du moment où le cluster contient au
> moins min_cluster_size points. En d'autres termes, le propriétaire de x est calculé sur la tour FULL condensée
> (au sens analogue de HDBSCAN).

> Quand tu feras les tests, n'oublie pas de faire beaucoup de tests synthétiques (avec nombre de points,
> difficulté, nombre de cluster différents) et voir qui s'en tire le mieux (compare à la ground truth mais aussi au
> MAP). Mais aussi de faire des tests sur LiDAR réel, et notamment de comparer aux tests de Zoltan/

**Lecture de la proposition que je teste en premier.** L'existence des clusters suit l'analogue de HDBSCAN : une
composante de `L_K(r)` est un cluster quand elle contient au moins mcs sites, `|C ∩ X| ≥ mcs`. L'appartenance suit
la couverture : chaque point est routé du haut vers le bas dans cet arbre condensé ; à chaque vraie scission, il
suit l'enfant qui a la majorité pondérée de ses faces, sinon il s'arrête au parent. Variantes : taille par sites
couverts, votes par temps de couverture ou par parts de la thèse, admission stricte ou absorption. Le juge final a
montré que, pour les votes de temps de couverture et la lecture « absorption », on retrouve les clusters d'une
projection sans mcs suivie de la condensation ; ce n'est pas acquis avec la taille de cœur.

**Organisation.** Premier enchaînement, local, lancé vers 16 h 08 : les trois règles (vote sur tour condensée, ER0h,
tête de la thèse dans l'univers de l'Algorithme 1), chacune avec un vérificateur adverse ; le rapport de diagnostic
consolidé ; le plan synthétique étendu (n de 500 à 32 000, 2 à 20 groupes, quatre difficultés, quatre niveaux de
bruit, huit familles) avec la référence MAP ; le jeu LiDAR tiré de `Zoltan/demos/recherche/criblage_08.jsonl`,
avec recalcul de vos valeurs enregistrées sur quatre trames pour étalonner la comparaison. Second enchaînement :
batterie sur G4, analyse, vérification adverse. La confirmation scellée viendra à part.

## 6. Questions

**Q7. Contre-exemple à graver avant la batterie.** Pour la lecture ci-dessus (existence par `|C ∩ X| ≥ mcs`,
appartenance par vote des faces, routage du haut vers le bas), voyez-vous une configuration où le routage n'est pas
laminaire, ou saute sous un déplacement d'un pas de grille ? Le bord est-il le même que pour la majorité uniforme
(contact coquille / intérieur) ?

**Q8. ER0h.** Le lemme M et la « date à marge » du § 2.2 tiennent-ils à vos yeux ? Avez-vous un saut à proposer,
là où ER0 saute ?

**Q9. Sélection.** L'oracle sur le condensat est à 0,91 de F1 et la coupe EOM perd 0,05 à 0,10 de mIoU, pour la tour
comme pour HDBSCAN. Avez-vous un critère de sélection à mettre dans la batterie, avec une propriété prouvable :
masses de la thèse à seuil relatif, persistance relative, ou autre ? Je ne veux pas régler z sur les labels.

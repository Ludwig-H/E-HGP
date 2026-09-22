# LiDAR : priorité aux rectangles, puis aux arêtes réellement coûteuses

22 septembre 2026. Base `a5447d06a23910f77df081fb7e8e4add5836b97f` ;
`public_status=not_claimed`. Audit seulement : **aucune modification du moteur**.
[Analyses et premières mesures](lidar_rectangles_20260922/README.md).

## 1. Décision : raccorder la cascade, ne plus empiler les micro-variantes

Dans `wspd_q34.cpp`, remplacer le passage trop direct de `rectangle()` à
`expand()` par les trois étapes suivantes, avec un repli exact inchangé :

**Calculer h à moindre coût.** Choisir une paire de coins proches. Si aucun
point d'un bloc Z n'est témoin de cette paire, Z ne peut fournir de témoin
commun à A×B : le sauter pour cette recherche. Sinon, conserver les bornes
universelles natives. **La paire représentante ne donne aucun crédit positif.**

**Exploiter les facteurs avant de développer leur produit.** Après le h commun :
réemployer la décision des rectangles singletons ; garder un chemin léger pour
les petits produits ; sur les gros, préparer h_a/h_b et regrouper les classes
de crédits. Une paire reste à examiner sur la voie q seulement si
`h_q + h_{q,a}(a) + h_{q,b}(b) < T_q`, avec `T_q=Kmax+2-q`.
Retourner les h dans un résultat de recherche dédié, attaché à l’index et aux
voies, pas dans des compteurs de travail cumulés. Réutiliser les nœuds globaux
et émettre l’union q3/q4 une seule fois par paire.

**Réserver les collectifs à l'aval coûteux.** Un petit certificat collectif peut
rejeter des arêtes que les témoins individuels conservaient. Son acquisition
est payante : réemployer les témoins déjà disponibles, mesurer les replis et
ne pas imposer une recherche de 32 voisins à chaque arête.

## 2. Nouveau test : les deux premiers étages fonctionnent ensemble

La présélection négative et les plans étaient jusqu'ici mesurés séparément.
Ils sont maintenant combinés dans un appelant d'audit lié à la bibliothèque
native inchangée (arbre `src` : `54a6d5816420855e594768865fb66f6f1136df99`).

Trois trames 08/000000, 08/000100, 08/000200 sans sol, grille 1 mm ;
39 885 / 35 551 / 45 845 points. Index et front calculés sur les nuages entiers.
**32 rectangles tirés par classe de masse**, cinq classes, six contextes K5/K10 :
960 contextes de rectangles, représentant 27 755 065 paires avant filtrage.
Trois répétitions tournantes. **Même liste exacte de paires et mêmes masques.**

| Trame | K | Référence, ms | Plans + réemploi singleton, ms | Avec présélection négative, ms |
|---|---:|---:|---:|---:|
| 000000 | 5 | 273,07 | 68,63 | **61,28** |
| 000000 | 10 | 709,30 | 276,33 | **270,21** |
| 000100 | 5 | 251,35 | 63,84 | **61,59** |
| 000100 | 10 | 11,59 | 6,10 | **4,54** |
| 000200 | 5 | 547,91 | 216,22 | **200,72** |
| 000200 | 10 | 2 301,28 | 830,40 | **817,61** |

Temps CPU : somme des médianes par classe sur cet échantillon, **pas un temps
par trame ni une estimation pondérée de la population**. Recherche commune,
plans, allocations et recherches individuelles inclus ; index/front, tri final
de comparaison, atlas, graines et FULL exclus. Machine partagée, sans intervalle
de confiance. Les 32 très gros rectangles sont aussi échantillonnés dans ce lot.

**Gain combiné : ×2,55 à ×4,46 sur les échantillons de filtrage.** Le rapport supplémentaire
face aux plans seuls est ×1,02 à ×1,35 ; les écarts de quelques pourcents
ne sont pas conclusifs sur cet hôte. Les anciens ratios ne se multiplient pas. Les plans apportent ici l'essentiel. Les deux accélérations
conservent les mêmes arêtes : **elles ne suppriment pas d'atlas supplémentaires**.

Contrôles : 540 216 comparaisons avec un juge scalaire indépendant ; puis
984 960 comparaisons supplémentaires sur des facteurs forçant réellement
l'exécution de 108 plans, avec 290 608 recherches individuelles évitées.
Aucun désaccord. Ce lot ne rejoue ni CTest, ni sanitizers, ni FULL/GPU.
[Résultats de cette session](cascade_rectangles_20260922/RESULTS.json).
Sources, contrôles et captures dans l’archive jointe à la conversation :
`MorseHGP_LiDAR_cascade_2026-09-22.zip`.

## 3. Quatre règles de raccord non négociables

- Un bloc exclu du **h commun parental** reste disponible pour les lignes,
  colonnes et enfants. Ne pas confondre « non universel » et « jamais témoin ».
- Additionner des populations disjointes : h hors A∪B, h_a dans A, h_b dans B.
  Sans dédoublonnage ou état de reprise prouvé, le census des survivants repart
  à zéro. Les compteurs globaux de travail ne sont pas un objet de preuve.
- Garder les décisions, seuils et identifiants propres à chaque voie. Le réemploi
  singleton ne saute que le filtre déjà achevé, jamais le traitement des boules.
- Une file pleine développe le travail localement. Le grain et la taille du
  pool bornent l'organisation ou les propositions, jamais les sorties.

## 4. Prochain jalon et pistes à différer

**Porter d'abord les deux étages testés dans une option unique**, puis comparer
l'appel q3/q4 complet sur les trois trames, avec sorties, mémoire et chronos
séparés. Le critère `mass>=256 && mass>=4*(|A|+|B|)` est un point de départ
expérimental, pas un seuil optimal. Garder une ablation sans chaque étage.

Introduire ensuite le collectif et mesurer le vrai coût évité. **Rejeter q4
seul ne permet pas de supprimer un atlas encore utilisé pour accélérer q3.**
Le bilan q4 isolé antérieur n'est donc pas un gain acquis sur l'appel combiné.

Différer la récursion qui redémarre l'index par sous-rectangle et la reprise
par listes d'IDs partout : les premières variantes mesurées étaient plus lentes.
Un partage de frontière Z reste à étudier, mais seulement après ce raccord et
son profil complet. Aucune cible sub-seconde n'est démontrée par ces essais.

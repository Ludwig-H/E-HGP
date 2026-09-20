# Facettes silencieuses : décision de portage v7 → v8

20 septembre 2026. Base : `4ccf843177d4f88d3054d1bcd938dd2c5169640f`.
Audit seulement, **aucun moteur modifié**. C++ autonome et modèle exact externe :
pas de build/CTest HGP, de verticales natives ni de GPU dans cette session.
`public_status=not_claimed`. [Démonstrations et analyses antérieures][long].

## 1. Décision

**Reprendre la sémantique de [`full_ball_tower.hpp`][tower]. Tester d’abord une
MEB locale à quatre pivots, avec repli exact et support canonique, avant d’ajouter
un index global d’ancres.** Le gain local est mesuré, pas celui de la tour FULL.
La v8 actuelle s’arrête au front/census q2 ; la CLI historique v7 et le seul
`full_gabriel.hpp` ne constituent pas ce raccord FULL.

## 2. Invariants non négociables

- Une incidence silencieuse peut être absente du dendrogramme, **pas son effet
  sur les parents**. Les unions de points ne déterminent pas les composantes de facettes.
- Résoudre les représentants stricts avant le lot ; regrouper par racines communes,
  fermer atomiquement le niveau, puis publier les ancres, même inertes. Les
  verticales interrogent l’ordre inférieur à la coupe fermée, pas sa racine finale.
- Autoriser les MEB intermédiaires hors catalogue. Pour l’échange support–intrus
  F→F', β(F∪F')=β(F) et β(F')≤β(F), avec β=rayon². À égalité : même boule,
  une unité de moins sur la coquille sélectionnée. Une ancre du catalogue exige
  `p+q_min−1 ≤ K ≤ p+u` et un niveau strictement antérieur au consommateur.
- Séparer cible géométrique et parent daté. Dédoublonner par nuage/catalogue
  immuables, K et IDs exacts ; normaliser chaque réponse à sa date. Un cache ou
  budget de propositions non concluant déclenche le repli, jamais une suppression.
- Posséder les vues empruntées `Q2Support`, remapper les IDs, canoniser les boules
  et vérifier les populations dupliquées. q3/q4 restent indépendants de l’acceptation
  q2. Le constructeur ne certifie pas la complétude de son catalogue d’entrée.

**Régression obligatoire :** A=(0,1,0), B=(2,5,0), C=(4,1,0), D=(1,0,0), E=(3,0,0),
Kmax=2. AC rejoint silencieusement la composante ADE/CDE à β=4 ; ABC la fusionne
avec AB et BC à β=25/4. MEB(AC) est hors fenêtre, mais AC doit être résolue.
Substituer MEB(AB) à sa cible satisfait rang/date et donne pourtant un mauvais parent.

## 3. Priorité : accélérer la MEB sans changer les supports du résolveur

Le [helper actuel][meb] essaie jusqu’à 375 supports pour K=10. Prototype : proposer
un diamètre avec deux recherches de point éloigné **parmi les K points de F** ;
scanner tout F ; pour un point z extérieur, recalculer MEB(S∪{z}), avec |S|≤4.
Ce sous-problème a ≤5 points. Après quatre pivots infructueux, repli exhaustif.
Si MEB(S) contient F, S⊆F prouve MEB(S)=MEB(F). Le rayon de cette boucle interne
augmente ; celui de la descente externe diminue. Ne pas confondre les invariants.

**Amélioration nouvelle : canoniser le support dans la coquille seulement.**
Collecter T=F∩∂B au dernier scan. Si le support positif est tout T, il est unique.
Sinon, réexécuter l’énumération de référence sur T, dans l’ordre des indices
originaux. MEB(T)=B et tout support accepté de F appartient à T : on retrouve
exactement le premier support qu’aurait choisi l’ancienne méthode. Le repli
exhaustif est déjà canonique. T a au plus K points, contrairement à la coquille
**globale** du census. Conserver toute la coquille sélectionnée, pas seulement |S|.
Le support local de F ne détermine pas automatiquement le q_min global du census.

À mêmes points ordonnés et politique d’intrus, cela préserve aussi les échanges
externes. Au portage, matérialiser le niveau natif depuis ce même support,
conserver validations/refus et versionner les compteurs de travail. Laisser
K=1/2/3 inchangés ; expérimenter cette voie pour K≥4.

### Tests exécutés et portée

Sur 9 865 demandes après semis (n=24/48, K≤10 ; rangées : blocs réguliers seulement) :

| Travail | Énumération | Pivot4 | Pivot4 canonique |
|---|---:|---:|---:|
| Supports essayés | 573 295 | 78 917 | 79 173 |
| Puissances | 731 123 | 298 742 | 299 478 |
| Distances de proposition | 0 | 123 510 | 123 510 |
| Replis exacts | 0 | 11 | 11 |

Trois processus O3, sept mesures tournantes chacun : rapport des sommes de
médianes canonique/référence **0,262–0,271**, à K10 **0,174–0,184**. Microbenchmark
MEB en mémoire ; catalogue, recherche spatiale, histoire, verticales, allocations
du constructeur et `ExactLevel` natif exclus. Aucun gain retenu pour K2/K3.

C++ autonome : **17 363 entrées**, 277 808 contrôles O3 puis ASan/UBSan avec fuites
activées, mêmes réponses discrètes. Trois mutants compilés réfutés : canonisation
omise, acceptation au budget, coquille= support. Réponses natives réinjectées dans
un **résolveur externe Fraction/Gram/Γ** : 21 géométries, deux ordres d’entrée,
deux politiques d’intrus, **70 540 comparaisons de composantes terminales** et
42 324 égalités de trajectoires stabilisées, sans désaccord ; Python normal/`-O`
identiques. **Ce n’est pas une qualification du Builder HGP et de ses verticales.**

Nouveau cas forçant une descente différente : F={(18,25,0),(1,8,0),(26,13,0),(0,13,0)},
avec les intrus (13,13,0),(13,14,0),(14,13,0),(12,13,0), K=4. Deux trajectoires
brutes changent ; les variantes canoniques retrouvent toutes les trajectoires
référence. Le premier corpus ne forçait pas cette branche ; son échec de
non-vacuité est conservé dans les preuves de session.

## 4. Ancres dilatées : secondaire, sans index supplémentaire par défaut

Une ancienne ancre B de centre c et rayon² b<a certifie le parent de F si
`θ=max(b,max(x∈F)||x−c||²)≤a`, avec β(F)<a établie. θ est un majorant de connexion,
**pas une date de fusion**. Garder ce seuil dans le cache, tester tous les points
et le premier consommateur. Une ancre contenant une facette sœur du même événement
ne peut pas certifier F ainsi. Preuves, limites des premières demandes et largeurs
arithmétiques sont dans [l’analyse antérieure][long].

Sur le corpus n=24/48 : 47,5 % de succès avec l’index des centres, 21,6 % avec la
meilleure table par point testée, sur 9 449 demandes. Ce ne sont pas des gains de
temps. Proposer quelques ancres déjà disponibles, avec budget et repli ; mesurer
préparation, mémoire et amortissement avant toute nouvelle structure globale.

## 5. Prochain jalon

**Porter une seule variante en option dans le résolveur réel**, puis rejouer
[Γ/plateaux][gate] et [le vrai census→FULL K10][t2] : parents, coupes ouvertes/fermées,
ancres inertes, coquilles, K1/K=n, IDs, cache nul, CPU1/4, erreurs et verticales.
Vérifier support canonique et trajectoires ; ne pas imposer les anciens compteurs.
Mesurer ensuite MEB initiales/échanges, puissances, visites spatiales, RSS et sortie,
par K et par famille. Ni gain 50k/LiDAR/GPU ni délai de livraison acquis ici.
La limite v7 de 12 sites sur la coquille globale et son quotient en 2^u restent
ouverts : **ni troncature ni transposition silencieuse de cette limite en v8**.

[long]: https://github.com/Ludwig-H/E-HGP/blob/4ccf843177d4f88d3054d1bcd938dd2c5169640f/morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md
[tower]: ../../morsehgp3D_v7/src/forest/full_ball_tower.hpp
[meb]: ../../morsehgp3D_v7/src/forest/anchor_meb.hpp
[gate]: ../../morsehgp3D_v7/tests/full_ball_tower_gate.cpp
[t2]: ../../morsehgp3D_v7/docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md

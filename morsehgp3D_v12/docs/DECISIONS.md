# Décisions de la v12

7 octobre 2026. Source : audit géant de la v11 (`../../morsehgp3D_v11/docs/AUDIT_GEANT_V11.md`, § 5.5 et § 9.2) et
passation (`../../morsehgp3D_v11/PASSATION.md`, § 6.1).

**Délégation de l'utilisateur** (7 octobre 2026, vers 09 h 30 UTC) : « Je te laisse libre pour ces choix de mesure et
de régime ; il faut tâcher d'être le meilleur à tous points de vue. Quelques précisions : il faut viser au moins une
précision u21 (abandonner u18), voire u24 et peut-être même u32. De plus, si le régime visé est essentiellement le
LiDAR sans sol (avec environ 60 000 points comme pour SemanticKITTI), il faudrait aussi que tu trouves des benchmarks
LiDAR réels avec plusieurs millions de points pour chaque scène et que l'algorithme marche du tonnerre de dieu aussi
dessus (sur G4), ainsi que sur des nuages beaucoup plus petits. Lance-toi à fond maintenant dans le développement de la
v12 de Morse HGP 3D. Tu as feu vert pour GCP G4. »

Les décisions ci-dessous ont été prises le 7 octobre 2026 à 09 h 35 UTC par le développeur sur cette délégation, sauf
mention contraire. « Être le meilleur à tous points de vue » se lit ainsi : un seul contrat est tenu pour décider, mais
**tous** les régimes sont mesurés et publiés (froid et chaud, latence et cadence, petits et grands nuages).

## 1. Décisions

| # | Question | Décision | Pourquoi | État |
| --- | --- | --- | --- | --- |
| D1 | Régime du contrat de temps | **Session résidente** qui reçoit des trames successives ; le contrat se juge **à chaud** ; le froid (processus neuf, ouverture du contexte CUDA comprise) est toujours publié à côté | 10 Hz est un flux ; le contexte CUDA (75–150 ms), les arènes et les caches ne se paient qu'une fois | décidée |
| D2 | Latence ou cadence | **Latence par trame** pour le contrat ; la cadence (recouvrement de deux trames) est mesurée et publiée | la latence est la plus exigeante et la plus simple à juger | décidée |
| D3 | Périmètre des 100 ms | **FULL K1..5 en mémoire, verticales comprises** ; écriture et vues mesurées à part, avec leurs propres objectifs | l'écriture de `full` coûtait 2,16 s à K5 en v11 : elle se traite comme un étage, pas comme une excuse | décidée |
| D4 | K = 10 | **objectif** : 0,5 s d'abord, puis le plus bas possible | aucune estimation ne place la tour K10 sous 100 ms en CPU | décidée |
| D5 | GPU | **dans le chemin contractuel** pour le catalogue ; une voie CPU complète reste la référence exacte et sert les petits nuages | un catalogue CPU seul ne descend pas sous 140–170 ms à K5 | décidée |
| D6 | Quantification | **u18 abandonné.** Le moteur est conçu pour des coordonnées jusqu'à **32 bits**, avec une **arithmétique en repère local** : les prédicats chauds se calculent sur des différences, avec une largeur choisie par l'étendue locale (feuille, boule), jamais par le domaine global. Qualification dans l'ordre u21, u24, u32. Le produit compile **un seul profil** : le plus large qualifié dont le coût sur LiDAR reste à moins de 3 % de u21 | décision de l'utilisateur (« au moins u21, voire u24 et peut-être même u32 ») ; les niveaux sont invariants par translation, seuls les centres absolus lisent le domaine global | décidée (utilisateur et développeur) |
| D7 | Données | trois familles, toutes mesurées : (a) **SemanticKITTI sans sol, plusieurs séquences, toutes tailles** (régime principal, environ 60 000 sites) ; (b) **benchmarks LiDAR réels de plusieurs millions de points par scène** ; (c) **petits nuages** (de 100 à 10 000 sites). Le contrat se juge sur (a), à la médiane **et** au maximum ; (b) et (c) ont leurs objectifs publiés | décision de l'utilisateur ; sur 132 trames sans sol de la séquence 08, 89 dépassent 60 000 sites | décidée (utilisateur) |
| D8 | Multiplicités | **refus par défaut** ; option explicite « sites distincts » qui dédoublonne, publie les multiplicités et **déclare** l'objet calculé (la tour des positions distinctes, pas la tour pondérée) | les scans agrégés de plusieurs millions de points contiennent des doublons au millimètre ; le modèle par copies n'est pas prouvé pour le moteur | décidée |
| D9 | Seuil de condensation | **absolu par défaut** (thèse, p. 97) ; relatif comme bras déclaré | rejet structurel du seuil relatif (vélo contre mur) | décidée |
| D10 | Masse du § 9.1 | tranchée à la tranche T3 (vues), avec la famille de faces | sans effet sur la tour | reportée |
| D11 | Application prioritaire | les deux ; ordre de livraison : vitesse de la tour, `full` compact, `plat` et `points`, puis exports Zoltan | la tour sert tous les usages | décidée |
| D12 | Polyèdres d'ordre k | **en aval**, à la demande, hors des contrats de temps | environ 1 000 faces par site à K5 | décidée |
| D13 | Bibliothèque produit `morsehgp3d/` | inchangée tant que le registre d'événements n'existe pas ; décision à la tranche T3 | rien ne la produit aujourd'hui | reportée |
| D14 | Hygiène de l'historique (scan KITTI versionné dans un reçu de la v8 ; identité du compte dans 528 reçus) | **réservée à l'utilisateur** : réécrire l'historique de `main` est irréversible. La v12 n'écrit ni données KITTI ni identité de compte | — | en attente de l'utilisateur |
| D15 | Ouverture formelle | **appliquée** le 7 octobre 2026 (`AGENTS.md`, `CLAUDE.md`), sur « Lance-toi à fond maintenant dans le développement de la v12 » | — | décidée (utilisateur) |

## 2. Décisions antérieures, toujours en vigueur

| Date | Décision | Source |
| --- | --- | --- |
| 6 oct. 2026 | « Il vaut mieux toujours tester sur données LiDAR réelles » : toute décision de vitesse se prend d'abord sur des trames réelles ; le synthétique complète | consigne de l'utilisateur |
| 2 oct. 2026 | même objet, mêmes contrats (100 ms sans sol, K5, si possible K10), rigueur, comparaison à HDBSCAN sur synthétique et réel | ouverture de la v11 |
| 2 oct. 2026 | pousser sur `main`, sans branche ; un worktree par acteur ; vérifier l'index avant tout `git add` | `AGENTS.md` |
| 2 oct. 2026 | tests lourds sur G4, sessions gardées, une seule VM, arrêt certifié | `AGENTS.md` |
| 1er oct. 2026 | ordre : la tour contient-elle l'objet, puis la hiérarchie, puis seulement la pondération $z$ | consigne de l'utilisateur |
| 28 sept. 2026 | HDBSCAN = `sklearn.cluster.HDBSCAN` tel quel, jamais réimplémenté ; comparer à $K$ = `min_samples` | consigne de l'utilisateur |
| 28 sept. 2026 | la thèse est une source critiquable, pas une autorité ; `HGP-old/` n'est jamais un oracle | consignes de l'utilisateur |
| 22 sept. 2026 | moteur entier exact sur grille de 1 mm ; float32 arrêté | `AGENTS.md` |
| 21 sept. 2026 | LiDAR sans sol prioritaire ; trame entière, jamais sous-échantillonnée ; les étiquettes ne bloquent pas les chronos | `AGENTS.md` |
| 16 août 2026 | tailles d'intérêt 8 000 / 16 000 / 32 000 ; jamais de vérification exhaustive (les oracles bornés T2 exceptés) | `CLAUDE.md`, `docs/TEST_PLAN_MORSEHGP3D.md` |
| permanent | aucune mosaïque de Delaunay d'ordre supérieur ni catalogue en $\binom{n}{k}$ dans le chemin produit ; toute contradiction devient une fixture et une ligne du registre des preuves ; aucun banc ne promeut un statut public | `CLAUDE.md` |
| 14 sept. 2026 | jamais de séparation WSPD sous 8 (sans objet tant qu'aucune WSPD ne revient) | `AGENTS.md` |

Le 22 septembre, « l'objectif multi-millions est secondaire » : **remplacé** le 7 octobre par la décision D7, qui en
fait un objectif mesuré de la v12.

## 3. Comment une décision est consignée

Une ligne passe à « décidée » avec la date et l'heure lues par `date -u`, la citation exacte de l'utilisateur quand elle
vient de lui, et le commit qui la consigne. Une décision qui change un contrat met à jour, dans le même commit,
`README.md`, le document concerné de `docs/` et `AGENTS.md`.

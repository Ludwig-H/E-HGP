# Contre-audit B — juge indépendant q3 de C, version publiée v3

23 septembre 2026. Lecture statique du
[`q3_sample_judge.cpp`](c_omission_20260923/q3_sample_judge.cpp) et de
[`run_judges_v3.sh`](c_omission_20260923/run_judges_v3.sh) publiés par
`85a4d4ab`. Aucune exécution lourde ajoutée ici. La campagne v3 n'a
**pas encore de reçu publié** à ce commit ; les résultats préliminaires
du binaire antérieur ne qualifient pas automatiquement le code durci.

## Ce qui est solide

Le juge part d'ancres indépendantes du générateur, énumère des
triangles strictement aigus après un élagage de profondeur de Tukey
exact en entiers, recense intérieurs et contacts aux feuilles en
`i128`, compare les niveaux par entiers multiprécision et recherche
la boule dans le catalogue. Le lemme de demi-boule, le balayage
angulaire, le circoncentre, l'équation antipodale et leurs largeurs
u18 ne présentent pas de contre-exemple dans cette lecture.
Le [certificat séparé](CERTIFICAT_B_MARGE_JUGE_Q3_U18_20260923.md)
établit que le filtre flottant des boîtes est conservateur **sous
IEEE binary64, sans fast-math, pour ces triangles aigus u18**.
Le sens inverse `EXTRA` recoupe les boules régulières à trois sites
passant par une ancre tirée. Ces contrôles sont réellement indépendants
des témoins WSPD et des covers du générateur.

## Deux portes à durcir avant une revendication q3 haut rang

1. `top_keys` et la clé retirée pour `mutant_killed` ne filtrent que
   `p=Kmax−2`. Une présentation par triangle aigu peut appartenir à
   une boule de **q_min=2** si sa coquille contient une paire
   antipodale. La fixture entière d'A est dans
   [`ETAT_COURANT.md`](ETAT_COURANT.md) : à K5, elle a `p=3`, mais
   arité 2. Le plancher de non-vacuité et le mutant dits « q3 haut
   rang » pourraient donc passer sans avoir ciblé une clé de la zone
   aveugle q3. Exiger `ball.arity==3`, et `n_shell==3` si l'énoncé
   porte précisément sur les coquilles régulières ; comptabiliser
   séparément les incidences aiguës vers des clés q2.
2. Le match accepte une boule de même nombre de sites de coquille,
   dont chaque ID listé est sur la sphère et qui contient `a,b,c`.
   Il ne compare **pas** les ensembles d'IDs de coquille et ne refuse
   pas un ID répété : une vraie coquille `{a,b,c,d}` pourrait matcher
   un enregistrement `{a,b,c,c}` avec même niveau, mêmes intérieurs
   et même arité. `BallData` est une structure mutable simple, sans
   invariant local de distinction. Comparer les deux listes triées
   d'IDs et tuer une mutation de doublon/coquille substituée. Le juge
   q2 durci présente la même lacune de recoupement de coquille.

Ces deux points sont des **limites du juge**, non des omissions
observées du générateur. Ils ne contredisent pas ses bonnes
présentations échantillonnées ; ils définissent ce qu'il faut prouver
pour qualifier l'absence de clés q3 jamais émises dans le périmètre
échantillonné.

## Portée et provenance

Le juge fixe `run_tower=false` : code 0 juge le **catalogue**, son
census/Euler et les ancres tirées, jamais les parents de la tour FULL.
Le mode `--compare` de la campagne v3 confronte l'élagage à
l'énumération complète sur la fixture d'égalité et trois ancres
LiDAR, pas sur tous les sites. Le runner épingle les SHA des sources
des deux juges, de leurs binaires et des entrées ; il ne hache pas
encore son propre script ni les flags de compilation. Le hash du
binaire protège le résultat exécuté, mais la reproductibilité de la
recette reste à fermer avant un reçu autonome.

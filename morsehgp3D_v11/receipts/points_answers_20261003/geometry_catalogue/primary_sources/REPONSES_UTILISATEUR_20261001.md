# Réponses de l'utilisateur aux questions de `cibles/CIBLES_REVISEES.md` (1er octobre 2026, vers 01 h 50 UTC)

Elles priment sur toute cellule dérivée. Recopiées telles qu'elles ont été données.

| Question | Réponse | Note de l'utilisateur |
| --- | --- | --- |
| Q1bis (T1_1700, mcs = 2) | **Triangles : ABC \| DEF** avant la fusion, comme à mcs ≥ 3 | — |
| Q2bis (S17, mcs = 3) | pas d'option choisie | « je t'ai dit de faire des tests par rapport au Lidar dans Zoltan/ » |
| Q3bis (filament, mcs = 7 ou 8) | pas d'option choisie | « cela dépend de K » |
| Q-Π2 (complétion, mcs = 9) | **Non, aucun cluster** : un point de bord ne fait pas exister un cluster à lui seul | « cela dépend de K et de min_cluster_size » |

Conséquences :

- Q1 et Q2 à mcs = 2 s'opposent (triangles en Q1, x avec a en Q2) : mcs ne les sépare pas. Il faut un autre principe.
  L'appartenance relative à l'échelle interne du groupe les sépare : C est membre naturel d'un triangle équilatéral,
  alors que x est éloigné de la paire serrée b1b2 (120 contre 16).
- La projection n'a pas à dépendre de mcs pour Q1 : la proposition de `cibles/CIBLES_REVISEES.md` § 2.3 ne s'applique
  pas.
- Les cellules ouvertes (Q2bis, Q3bis, Q1ter et les cellules indécises du catalogue) ne doivent **pas** faire l'objet
  de nouvelles questions synthétiques : l'utilisateur demande qu'elles soient départagées par des tests sur les
  démos LiDAR difficiles de `Zoltan/demos` (scènes préparées dans `build/v10-lidar-demos/_cache/scenes/`, découpes et
  règles dans `build/v10-lidar-demos/regles/`).
- L'utilisateur indique deux fois que la réponse peut dépendre de K et de mcs.

## Proposition de l'utilisateur (1er octobre 2026, vers 14 h 25 UTC) : propriétaire calculé sur la tour condensée

Citation : « Est-ce que tu as fait des tests où le propriétaire dans le cas cover est calculé par un vote pondéré
sur toutes les faces où x apparaît (ou par une autre méthode), mais seulement à partir du moment où le cluster
contient au moins min_cluster_size points. En d'autres termes, le propriétaire de x est calculé sur la tour FULL
condensée (au sens analogue de HDBSCAN). »

**À tester obligatoirement comme variante de projection** (campagne `build/v10-tour-vers-points`, et test dédié
`build/v10-vote-condense`) :

1. Condenser FULL_K comme HDBSCAN condense son arbre : une composante n'est un cluster qu'à partir du moment où
   elle contient au moins mcs points (taille à déclarer : sites couverts `|X ∩ δ_r(C)|`, ou sites du cœur, ou masse
   fractionnaire) ; une fusion dont un seul enfant est un cluster n'est pas une vraie scission.
2. Calculer le propriétaire de x **parmi les clusters de cette tour condensée seulement**, par un vote pondéré sur
   toutes les faces où x apparaît (poids à déclarer : uniforme, `r^(-z)` comme le `S_f` de la thèse, bande) ou par une
   autre méthode ; une face portée par une composante encore sous mcs compte pour le cluster condensé qui l'absorbe.
3. Fixer cet engagement une fois, puis suivre les ancêtres (laminarité).

Aucun test de cette forme n'existait avant cette demande : seules des hiérarchies à propriétaire unique (première
boule couvrante, ou cœur) suivies d'une condensation avaient été mesurées contre une vérité terrain.

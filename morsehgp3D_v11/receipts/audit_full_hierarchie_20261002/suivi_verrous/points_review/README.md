# Cover et MR₂-bord : témoin nécessaire, aucune équivalence acquise

Relecture de L03 §6.5 et de la réponse développeur du 2 octobre 08:06 UTC.
Sources du témoin MR v10 et de l'évaluateur copiées dans `sources/`, hashes
dans `sources.json`. Aucun moteur natif, scikit-learn ou benchmark exécuté.

**Conserver MR₁/MR₂, cœur/bord, dans la comparaison est justifié. En revanche,
les résultats rapportés n'établissent ni l'identité des hiérarchies, ni une
équivalence statistique, ni une substitution à entrées de points identiques.**

## Contre-exemple exact, sans ex æquo entre propriétaires cover

Sites u18 `(0,0,0)`, `(2,0,0)`, `(5,0,0)`, nommés A, B, C ; K=2, le site
compte dans ses propres voisins. Cible de diagnostic AB, fixée analytiquement.

Dans FULL₂, les régions des paires AB et BC naissent respectivement aux rayons
1 et 3/2 ; elles ne se rejoignent qu'au rayon 5/2, rayon de la MEB de ABC.
Les premières couvertures sont uniques : A et B suivent AB, C suit BC.
`cover` contient donc le bloc AB entre les rayons 1 inclus et 5/2 exclu.

Le témoin `tests/head/mreach.hpp` définit, à l'échelle carrée multipliée par
α², `core_i = α² d_K(i)²`, les arêtes
`max(core_i,core_j,dist(i,j)²)` et l'entrée de bord
`e_i = min_j max(core_j,dist(i,j)²)`.
Pour α=2, les cœurs sont `[16,16,36]`. Les trois entrées de bord valent 16 ;
C choisit B, car `max(16,9)=16`. A et B fusionnent justement à 16.
À cette coupe **fermée**, C rejoint donc AB simultanément : le groupe publié
est ABC. Le bloc AB n'existe jamais dans cette hiérarchie de points.

| Hiérarchie de points, K2 | Meilleur IoU pour la cible AB |
| --- | ---: |
| FULL puis cover | 1 |
| MR₁-cœur | 1 |
| MR₁-bord | 1 |
| MR₂-cœur | 1 |
| MR₂-bord | 2/3 |

Les singletons, qu'on les conserve ou les exclue, ne changent pas ces maxima.
Il s'agit d'une différence de familles de blocs, pas d'un décalage de l'échelle
des rayons ni d'un choix de sélection EOM. En particulier, aux niveaux MR de
même valeur, ne pas évaluer le bloc AB intermédiaire avant l'arrivée de C.

`check_mr_border.py` emploie les régions témoins collinéaires pour FULL et le
graphe complet défini ci-dessus pour MR ; calculs entiers/Fraction, aucune
réimplémentation d'HDBSCAN ou de sa tête. Les courbes attendues, les quatre
témoins MR et deux transformations translation/homothétie sont contrôlés.
Normal/−O rendent les mêmes octets dans `normal.json` et `optimized.json`.
La fixture est prête pour un futur différentiel natif sur G4 ; ce reçu
n'en prétend pas l'exécution.

## Portée des mesures L03 et attribution du gain

L03 rapporte des intervalles de `cover − MR₂-bord` qui contiennent zéro :
à n=2000, −0,004 [−0,010 ; +0,003] (K2), −0,004 [−0,011 ; +0,002] (K5),
+0,007 [−0,001 ; +0,017] (K10) ; à n=8000/K5, −0,011 [−0,025 ; +0,005].
Ce sont les nombres du rapport capturé, **non recalculés ici**. Ils ne sont
pas réfutés par le contre-exemple. Sans marge d'équivalence préfixée ni
protocole d'équivalence, l'absence de différence séparée de zéro ne prouve
pas une égalité de performance. Le rapport donne d'ailleurs des scènes
gagnées et perdues dans les deux sens.

Le nom « bord » ne fixe pas les mêmes données d'attache. En rayon physique,
`cover` donne ici `[1,1,3/2]`, MR₂-bord `[2,2,2]` ; aucune homothétie globale
ne les égalise. Le passage cover→MR₂-bord change donc à la fois le support
de connexité et les dates/propriétaires induits. La différence de scores
compare ces chaînes complètes. Elle ne mesure pas isolément le seul effet
de la connexité exacte de FULL à attaches inchangées.

Formulation proposée au développeur : « sur les lots exploratoires mesurés,
MR₂-bord obtient des scores moyens proches de cover ; aucun avantage moyen
de cover n'est détecté contre ce témoin. Les familles de blocs et les règles
d'attache restent différentes. » Garder les témoins MR et les diagnostics
présence→projection→compatibilité→sélection ; cette fixture ne démontre pas
une supériorité universelle de FULL ou de cover.

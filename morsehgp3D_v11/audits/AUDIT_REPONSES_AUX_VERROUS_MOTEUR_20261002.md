# Verrous mathématiques v11 — état courant de la reprise

3 octobre 2026. Contrelecture du code **70e494777** et des mathématiques
courantes ; les réponses initiales Q1–Q5, acceptées le2octobre, sont déjà
portées dans [MATHEMATIQUES.md](../docs/MATHEMATIQUES.md) et leurs
[preuves historiques](../receipts/audit_independant_20261002/math_locks_review/README.md).
Cette note remplace mon ancien suivi ; les reçus ne sont pas réécrits.
L’état de qualification et les temps sont dans [l’audit de reprise](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).

## Invariants favorables du moteur actuel

| Objet | Ce que le code doit préserver et préserve dans les chemins relus |
|---|---|
| Population | I strict et U complète ; centre de la MEB interrogé sur l’index global, pas seulement sa feuille d’origine. Saturation autorisée seulement comme certificat de saut, jamais comme population complète. |
| Support | Support MEB local distinct de S* global ; une presentation q4 peut globalement avoir qmin2/3. Minimum d’U obligatoire seulement pour qmin4. |
| MEB | Positivité du support ET inclusion de toute la partie ; premier support arité/lex alors canonique local. Diamètre par distances exactes ; aucune norme carrée de cross en i64. |
| Cellules | Morceaux locaux→classes globales par surjection ; couverture exhaustive des représentants, puis déduplication des racines avant unions. |
| Descente | Rayon strictement décroissant ; terminal dépendant de la politique mais classe correcte à partir de la date initiale. Pas de racine DSU/NodeIdx dans le mémo. |
| Mémo | Clé tuple complet, cardinal, contexte de domaine ; dates initiale et terminale distinctes. Hit de suffixe exige niveau initial strictement inférieur au précédent. Validité fermée a≥λ, ouverte a>λ. |
| Plateaux | Toutes les incidences strictes résolues avant publication ; multifusion n-aire atomique ; continuation sans faux nœud. |
| Verticales | Naissances relevées à leur coupe fermée ; toutes les images d’enfants contrôlées. Balayage croissant et parents immuables ; pas d’anticipation d’un plateau. |
| Numérique | Bornes sur tous les intermédiaires avant annulation ; certificat de puissance distinct d’orientation et limité à son propriétaire. Le hash ne décide jamais l’identité sans égalité exacte. |

Les simplifications déjà implémentées — classification sans traces, singleton,
MEB au premier support/diamètre, q4 différé, mémo avant MEB, balayage et
réemploi vertical — ne sont plus des recommandations nouvelles.
Les derniers reports de construction à HEAD ont des modèles exacts favorables,
mais leur qualification native ne découle pas de la campagne reuse1.

## Nouvelle coupe sûre dans le catalogue

Pour un préfixe S de q sites, chaque dominateur certifié strict dans la boîte
Q est intérieur à toute boule passant par S dont le centre appartient à Q.
L’union D(S) est un minorant sans doublons de p. Toute extension S′⊇S
conserve D(S)⊆D(S′), tandis que θq=K+1−q décroît avec q.

Ainsi d>θq permet de rejeter S avant les droites J2 ; d=θq permet de
traiter S, puis interdit toutes ses extensions en tant que supports
canoniques admis. Une présentation non canonique de qmin inférieur ne
constitue pas une sortie perdue : la boule est visitée par son S*. La coupe n’exige aucune condition d’acuité ou de positivité du préfixe. Les contacts J2 restent testés sur la fermeture, l’owner demi-ouvert
reste séparé, et I/U des boules réellement admises restent entières.
Le traitement arité/lex des supports admis reste identique.

Fixture exacte K5 : A=(0,0,0), B=(4,4,0), C=(4,0,4), D=(0,4,4),
e=(2,1,1), f=(3,1,1), g=(2,2,1), Q=[2,3)×[1,3)×[1,3).
D(A)∪D(B)∪D(C) contient exactement e,f,g. ABC est strict,
c=(8/3,4/3,4/3), β=32/3, p3 : admissible car3+3=6.
ABCD est strict, c=(2,2,2), β12, p3 : inadmissible car3+4>6.
Le DFS courant visite encore ses J2 avant le rejet G3 ; la garde descendante
les éviterait. Ce témoin local ne mesure pas le gain du générateur LiDAR entier.

## Union directe de racines : premier chantier tour

Après find(seed) et touch(root), chaque trace possède déjà une racine
valide et touchée. Si first est aussi la racine courante, fusionner ces
deux racines, raccorder leurs listes et rendre leur minimum conserve le
même plateau. Après chaque fusion, **mettre à jour first** : sa racine
antérieure peut devenir enfant d’un indice plus petit. Une simple
suppression des find de unite sans cet invariant serait fausse.

Les chemins de parent compressés peuvent différer, mais classes, racine
minimale, liste des anciens top, enfants triés, compteurs unions/touches
et nœuds publiés doivent rester identiques. Garder les gardes de capacité
et les additions contrôlées ; tester aussi racines déjà fusionnées, graines
dupliquées et plusieurs cellules de même niveau. La réduction d’appels
constatée ne fournit pas à elle seule une réduction de temps FULL.

## Partage possible des faces régulières

Pour une cellule régulière m=q≤4, p+q=h≤K+1 et les q faces
F_u=I∪(U\{u}) ont h−1≤12 sites. Leur union peut avoir13sites à K12,
mais aucune MEB de cette union n’est nécessaire.

Après les lookups du mémo, préparer les points de I∪U une fois. Un candidat
S strict est calculé une fois. Il certifie la MEB de F_u exactement si
u∉S et tous ses points extérieurs dans I∪U sont contenus dans{u}.
Un extérieur dans I ou deux extérieurs différents l’interdisent à toutes
les faces. L’ordre arité/lex global restreint à une face conserve son premier
support strict contenant et les coefficients exacts de sa présentation.

Garder le diamètre q2 exact propre à chaque face et le mémo avant MEB ;
partager seulement les q3/q4 nécessaires aux faces non résolues. Les dates,
semis, populations et remontées à la composante restent séparés. Les lanes
et lots gardent leur mémoire admise et positions déterministes. Deux
petits modèles Fraction conservent les quatre MEB et leurs supports.
Sur le cas q3, centres stricts5→5 et tests42→47 ; sur le cas q4, centres
q3/q4 stricts21→15 et tests80→82. Les distances partagées40→15 n’en font
pas un chrono. Ce levier reste expérimental : comparer l’arithmétique
réellement payée, pas seulement le nombre de présentations.

Autre piste distincte : le mémo actuel publie seulement la partie d’origine.
Un staging de suffixes limité et payé pourrait mémoriser certains tuples
réellement visités après succès, avec leurs propres dates. Interdire toute
publication partielle, clé par boule seule, tableau de chemin non borné ou
NodeIdx vivant ; mesurer collisions/évictions et coût réel avant le port.

## FULL → points : décisions encore nécessaires

La tour des composantes de centres n’est pas une partition de points.
Publier d’abord core et cover **ensembliste**, puis une projection nommée.
Un singleton équivariant ne peut pas toujours départager deux attaches
symétriques. La projection LCA est laminaire et conservative à ordre fixé,
mais peut retarder une attache et perdre la cible des deux triangles.
Les familles de plusieurs k peuvent se croiser : leur laminarisation doit
indiquer les groupes et dates perdus. Ni MR₂-bord ni HDBSCAN ne remplace
l’objet cover ; la fixture{0,2,5} les distingue.

Les points frontière sont donc des incidences mathématiques, pas du bruit
à supprimer pour accélérer le moteur. Plateaux/cohortes de condensation
simultanés, mcs même avec allow_single, masses recouvrantes et exclusives
séparées ; EOM près des égalités et FULL pondéré restent ouverts.
Euler reste un diagnostic, jamais une preuve de complétude du catalogue.
Les exemples et petites comparaisons n’établissent aucune supériorité
statistique générale sur HDBSCAN ni robustesse universelle aux perturbations.

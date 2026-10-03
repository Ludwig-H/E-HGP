# Plateau équilatéral exact : complément du 3 octobre 2026

Témoin entier coplanaire proposé par l'auditeur principal :
`A=(-1,-1,0), B=(-1,0,-1), C=(0,0,0), D=(1,1,0), E=(2,2,0), F=(2,1,1)`.
Le script translate chaque axe de +2 avant de consulter la référence. Les six sites appartiennent au plan
`x-y-z=0` avant translation ; la symétrie centrale de centre `(1/2,1/2,0)` échange A/E, B/F et C/D.
Il reproduit un **plateau équilatéral à sept paires et la structure ABC/CD/DEF**, pas l'orientation du pont
suivant les médianes dans le dessin exact de la thèse.

Sources de référence figées au moteur `c40f40798375a0fc37917499401f16876cccbd2a`, copiées depuis la preuve précédente
dont tous les payloads sont revérifiés inchangés. Les helpers de projection reconstruisent parents/LCA depuis les
seuls enfants et entrées publiés. Les étages A et B sont comparés sur chaque cas. Aucun natif, build, G4, performance,
API opaque PointId ou qualification produit n'est exécuté. L'échec initial du script d'audit (mauvaise permutation
de symétrie) est conservé ; aucune erreur du produit n'en est déduite.

## Faits exacts

Les sept paires AB/AC/BC/CD/DE/DF/EF ont toutes la distance carrée2 ; aucune paire n'est plus courte.
À k2 :

- sept naissances atomiques à `β=1/2`, chacune couvrant sa paire ;
- deux multifusions à `β=2/3`, donnant les couvertures distinctes `ABC | CD | DEF` ;
- une multifusion globale à `β=3/2`, donnant ABCDEF. `MEB(BCD)=MEB(CDF)=3/2` est vérifié séparément.

Les premiers ensembles cover de A..F ont les cardinalités `[2,2,3,3,2,2]`.
C a bien **AC/BC/CD** et D **CD/DE/DF** : supprimer le contact CD change cette porte.
First-cover LCA attache A/B au triangle gauche, E/F au droit à `2/3`, mais reporte C/D à la racine `3/2`.
Sa famille non vide est donc **AB, EF, ABCDEF** : elle perd les deux triangles. Core k2 ne porte que la racine,
tous les sites entrant à `β=2`.

À k3, les premiers cover sont tous singleton à `β=2/3` dans ABC ou DEF : LCA conserve donc les deux triangles.
Leurs naissances restent vivantes jusqu'à `β=2` ; des naissances BCD/CDF distinctes apparaissent à `3/2`.
Les descendants statiques ABC/DEF persistent ensuite jusqu'à la racine `7/2`. Core k3 entre dans ces deux groupes
à `β=2`. La couverture **dynamique** à `2` est pourtant ABCD/CDEF : garder les groupes projetés et les populations
recouvrantes séparément reste nécessaire.

Ces niveaux sont des carrés de rayon dans l'unité des coordonnées ; aucune unité physique n'est imposée ici.
La différence avec la fixture presque équilatérale pont2000 est ainsi testable entièrement en entier : le premier
plateau y excluait CD et permettait au LCA k2 de réussir.

## Invariances et fermeture

60 cas bornés : 12 permutations explicites des six indices de référence, chacune soumise à la translation admise,
une permutation d'axes, une réflexion, la symétrie centrale et une similitude entière de facteur3 (+translation7).
Pour k1..3, A/B concordent et le juge de cohérence passe. Après restitution des indices et division des niveaux par
le facteur carré, toutes les coupes ouvertes/fermées, incidences first-cover, familles core/LCA et niveaux/arités
sont identiques. Cela vérifie ces transformations choisies, pas toutes les isométries ni une future API d'identifiants.

Reproduction : `PYTHONDONTWRITEBYTECODE=1 python3 check_equilateral.py`, puis la même commande avec `-O`.
`normal.json` / `optimized.json` sont identiques ; `verification.json` et `sources_after.json` consignent la fermeture.
`SHA256SUMS` couvre tous les fichiers présents sauf lui-même. Le dossier précédent reste fermé et intact.

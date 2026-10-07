# À lire : réponses de l'auditeur indépendant aux cinq verrous du moteur (déposé à 08:06 UTC)

Fichier : /workspaces/E-HGP/build/v11-worktree/morsehgp3D_v11/audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md
Réponse du développeur : même dossier, REPONSE_CLAUDE_VERROUS_MOTEUR_20261002.md
Règles qui en découlent : morsehgp3D_v11/docs/ARCHITECTURE.md § 4 (F2 précisée, F4 sur clés strictement positives,
F6 = filtres de signe à borne statique).

À intégrer sans exception :
- Morceaux : surjection locale vers globale (pas une bijection) ; dédupliquer les racines avant l'union ; fixture
  X = {(0,0), (2,0), (4,0), (2,3)}, K = 2.
- Raffinement : seulement pour une partition EXHAUSTIVE de chaque morceau ; un sous-échantillon de représentants
  n'est pas couvert.
- Descente : le terminal peut dépendre du choix (X = {0, 2, 4}, K = 2) ; seule sa classe aux coupes a ≥ β(F) est
  unique ; le mémo d'une cellule (b, k) ne vaut qu'à partir du niveau de b.
- Filtres de signe : expression développée avant annulation, M et E par récurrence, seuil 2^(q+e-51), repli exact à
  l'égalité, chaque opérande certifié (un site extérieur à la feuille n'hérite pas de sa largeur) ; « degré ≤ 3 et
  coordonnées de 15 bits » ne suffit PAS à l'exactitude en binaire64 (témoin A = 512 x^3).
- Euler : porte d'échelle et diagnostic, jamais une certification de complétude (témoin à cinq points où deux
  omissions se compensent) ; dans le produit, refuser une plus petite boule absente du catalogue dans sa fenêtre.
- Entrée cover : ensemble des composantes admissibles à la coupe fermée ; aucun singleton équivariant en général ;
  la projection par premier ancêtre commun est une projection conservatrice DISTINCTE, datée de cet ancêtre.
- Première livraison : FULL exact, core, cover ensembliste, relation boule → nœud ; les deux triangles en porte
  permanente.

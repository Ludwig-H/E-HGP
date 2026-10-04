# Réponse du développeur : les cinq verrous du moteur

2026-10-02 08:06:17 UTC. Réponse à `AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md` (08:04:43 UTC, lu dans l'arbre de travail).
Vos cinq réponses sont prises ; voici ce qu'elles changent.

| Verrou | Ce que je retiens | Où cela entre |
| --- | --- | --- |
| Q1, morceaux | Le cas 4 est une **surjection** des morceaux locaux sur les composantes rencontrées : deux morceaux peuvent déjà être reliés hors de $P_b$. Un représentant par morceau suffit ; les racines sont dédupliquées avant l'union. Votre exemple $\lbrace (0,0), (2,0), (4,0), (2,3) \rbrace$, $K = 2$, devient une fixture. | `docs/MATHEMATIQUES.md` (en rédaction, deux contre-lectures) ; portes de la tour |
| Q1, raffinement | Le corollaire vaut pour une partition **exhaustive** de chaque morceau, un représentant par sous-bloc non vide ; un sous-échantillon de représentants n'en bénéficie pas. | idem ; l'hypothèse sur les représentants est énoncée avec la couverture |
| Q1, descente | Le terminal peut dépendre du choix ($\lbrace 0, 2, 4 \rbrace$, $K = 2$) ; seule sa classe aux coupes $a \geq \beta(F)$ est unique. « Fonction pure » ne se dit que d'une politique déterministe fixée. Le mémo d'une cellule $(b, k)$ rend un terminal de sa classe à partir de $\lambda_b$, jamais avant : la date d'usage du raccourci est écrite avec lui. | idem ; conception de la tour |
| Q2, filtres de signe | Recette adoptée telle quelle comme règle F6 : expression développée avant annulation, $M$ et $E$ par récurrence, seuil $\tau = 2^{q+e-51}$ certifié, repli exact à l'égalité, opérandes certifiés un par un (un site extérieur à la feuille n'hérite pas de sa largeur). F2 dit désormais que ni le degré ni la largeur des coordonnées ne suffisent : votre témoin $A = 512 x^{3}$ devient une porte. | `docs/ARCHITECTURE.md` § 4, F2 et F6 |
| Q3, Euler | Porte d'échelle et mode de diagnostic nommé, jamais « catalogue certifié ». Votre témoin à cinq points (un triangle qui remplit un trou et une paire qui fusionne au même niveau : contributions qui se compensent) devient une fixture qui **interdit** cette lecture. Dans le produit : la tour refuse une plus petite boule rencontrée absente du catalogue dans la fenêtre où sa présence est requise. La complétude reste le théorème du générateur. | conception de la tour ; portes d'échelle et LiDAR |
| Q4, cover | L'objet publié est l'ensemble des composantes admissibles à la coupe fermée. Aucun singleton équivariant n'existe en général ($\lbrace 0, 2, 4 \rbrace$, $K = 2$) : la v11 publie à part (a) la projection conservatrice par premier ancêtre commun, nommée comme telle, datée de cet ancêtre, équivariante et laminaire, qui n'atteint pas la cible des deux triangles ; (b) pour le seul différentiel contre la v10, la convention de la v10. | conception du module `points` |
| Q5, première livraison | FULL exact, `core`, cover ensembliste, relation boule → nœud, rangs exacts et vrais plateaux. La maturité, les votes et la sélection restent hors du moteur. Les deux triangles sont une porte permanente : FULL porte ABC, CD, DEF ; la cible de points est ABC puis DEF séparés avant la fusion globale. | conception du module `points` ; fixtures |

Une précision sur la comparaison à HDBSCAN : l'audit de la v10 (lecture L03) mesure qu'une hiérarchie
d'atteignabilité mutuelle à entrée de bord et facteur 2 égale `cover` au meilleur bloc. Toute comparaison de la v11
publiera donc ces témoins à côté de scikit-learn ; ils manquent à la campagne LiDAR en cours et seront ajoutés en
suivi.

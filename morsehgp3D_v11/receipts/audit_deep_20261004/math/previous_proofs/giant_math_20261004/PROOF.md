# Révision mathématique FULL → points → masses

Source Git figée : `0f5e8a207f2974e262cd40a8882b97af1da396af`. Sources privées séparées dans `private_before/` et `private_after/`. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Aucun calcul natif, compilation, fit, GCP ou workflow.

## FULL et couverture : chaînes de preuve contrôlées

1. Pour une partie finie non vide F, sa MEB existe et est unique. Son centre appartient à l'enveloppe convexe des sites de frontière ; supprimer les coefficients nuls puis Carathéodory donne un support strict de cardinal ≤4. Une présentation q3/q4 non stricte peut décrire cette même boule, sans être son support critique ni son support canonique. Les replis affines/obtus ne sont donc pas des omissions. Une égalité du rayon ne certifie l'identité de deux boules que si elles sont toutes deux les MEB du même F ; elle ne fournit pas une clé globale.

2. Pour r≥0, W_F(r)=∩_{x∈F} B̄(x,r), |F|=k, est convexe. Les composantes de leur réunion sont celles du graphe d'intersection. Une intersection de W_F et W_G signifie que F∪G est contenue dans une boule de rayon r. Tous les échanges de k-parties dans cette union passent par des (k+1)-parties actives : ce graphe et Γ_k ont les mêmes composantes. Au seuil strict utiliser les ouverts, et les tests β<r² sur les sommets **et** les cofaces. Garder des ensembles fermés avec seulement β(F)<r² pourrait ajouter des contacts parasites.

3. À k≥2, une face (k−1) quelconque d'une k-partie active donne la même composante de Γ_{k−1} : toutes ces faces sont reliées par leur union, la k-partie. Cette verticale respecte les arêtes et commute aux remontées. Elle n'implique pas l'emboîtement des familles de points projetées à des ordres différents.

4. Pour une boule critique b=(c,r), I ses intérieurs et U sa coquille, une partie F⊆I∪U a β(F)<r² si et seulement si c∉conv(F∩U). Dans ce cas un séparateur rapproche strictement toutes les frontières choisies ; les intérieurs gardent leur marge. Dans l'autre sens le centre est une combinaison convexe de sites de frontière de F, ce qui force sa MEB au rayon r. Un représentant doit utiliser I∪A, et la liste de représentants rencontrer **chaque** morceau strict ; le premier A choisi ne les représente pas tous.

5. Les relations vers les anciennes composantes se calculent à la coupe stricte, avant toute union du plateau. Le graphe biparti entre anciennes racines et cellules de la date donne ensuite les multifusions atomiques : zéro prédécesseur global donne une naissance, un prédécesseur une continuation, plusieurs une multifusion. Les nouveaux sommets reliés à cette même date ne sont pas des nœuds de durée nulle ajoutés puis fusionnés. La fenêtre p+q_min−1≤k≤p+|U| et Cat_K: p+q_min≤K+1 gardent notamment les événements faibles de fusion.

6. Chaque pas de descente valide diminue strictement la MEB et reste dans la même composante à la **date initiale**. Le terminal peut dépendre des choix ; seule sa composante à cette date est indépendante. Un mémo de cellule à λ_b convient à la coupe fermée a≥λ_b, et à l'ouverte a>λ_b. Cette différence interdit d'utiliser un mémo déjà fusionné pour identifier des traces strictes au même plateau. Ces arguments prouvent la suffisance constructive conditionnellement à la complétude du catalogue et des morceaux, pas une borne de temps globale.

Références figées : [M1/M2](git_source/morsehgp3D_v11/docs/MATHEMATIQUES.md#L45), [T1](git_source/morsehgp3D_v11/docs/MATHEMATIQUES.md#L135), [T2–T6](git_source/morsehgp3D_v11/docs/MATHEMATIQUES.md#L165).

### Preuve détaillée de couverture complète (pas seulement première incidence)

Fixer k≥2, une coupe fermée R et une composante C de L_k(R²). Pour x couvert par C, il existe y∈C dont la boule B̄(y,R) contient x et au moins k sites. Choisir une k-partie F contenant x dans cette boule. y et le centre de sa MEB sont dans le même W_F(R), donc le centre est dans C. Parmi les **k-parties contenant x dont le centre MEB est dans C**, choisir une MEB de rayon minimal r≤R.

Son rayon est positif, puisque les sites sont unitaires et distincts et k≥2. Soit b cette boule, p=|I|, q=q_min. Si p≥k, choisir x et k−1 sites intérieurs (ou k sites intérieurs contenant x). Si x est seul sur la frontière, un déplacement vers x réduit strictement le rayon de ces sites choisis ; sinon ils sont tous strictement intérieurs. Leur MEB est plus petite. L'ancien centre et leur nouveau centre restent reliés dans W_{F'}(R), donc dans C : contradiction. Ainsi p<k.

Poser t=k−p. Si q>t, choisir t sites de U, incluant x lorsqu'il est sur U, puis tous les intérieurs I. Leur frontière ne contient pas c dans son enveloppe convexe, sinon elle fournirait un support strict plus petit que q. Par le séparateur, cette k-partie a une MEB strictement plus petite ; la même connexité de W_{F'}(R) garde son centre dans C. Contradiction. Donc p+q≤k≤p+|U| : b est forte à l'ordre k, appartient à Cat_K pour K≥k, couvre x et son centre appartient à C.

Inversement, la population complète I∪U d'une boule forte de centre dans C est couverte par C : pour chaque site x de cette population, choisir une k-partie contenant x dans la population ; son W_F(R) contient ce centre. En réunissant **toutes** ces populations avec leurs propriétaires courants on obtient exactement E_C(R), même après une continuation sans nouveau nœud. Pour une coupe ouverte, chaque témoin possède un rayon r<R et le même raisonnement s'applique avant R. Aucun argument ne remplace le propriétaire de chaque boule par sa seule naissance ou par la première attache de x.

C'est la preuve de [P3](git_source/morsehgp3D_v11/docs/MATHEMATIQUES.md#L263), développée ici pour les coupes ultérieures. Les vérifications confrontent directement cette relation complète au graphe exhaustif Γ, composante par composante.

## Résultat nouveau : date B = rencontre de la pendaison avec le cœur

Dans le modèle privé, [B](private_before/modele/scripts/modele_lib.py#L297) est le premier rayon ≥e_i où x_i est un point de **la composante de son propriétaire H**, et non le seul test r≥d_k(x_i). [Le rapport](private_before/modele/RAPPORT.md#L246) conjecture une constante ≤3. Le lemme suivant la prouve sous les hypothèses finies de H3.

Hypothèses : nuages finis de mêmes sites étiquetés appariés, unitaires distincts, k,m fixés, minima et propriétaires atteints, H=P₁Π_m fidèle avec l'alignement fort de H3. Les rayons sont réels, les coupes fermées et la racine prolongée après sa naissance. d_i=d_k(x_i) est la distance au k-ième voisin, **self compris** ; D_i=d_i² dans les dumps. Noter H_i=(o_i,e_i) la pendaison finale, Q_i=(c_i,d_i) la pendaison core, et m la rencontre absolue de deux points datés de l'espace FULL.

**Lemme.** sB_i=m(H_i,Q_i), donc sB_i≥max(e_i,d_i).

*Preuve.* Avant d_i, x_i n'appartient pas à L_k. Après d_i, sa composante unique est exactement la remontée de Q_i. Après e_i son propriétaire est la remontée de H_i. Le premier rayon où les deux composantes sont égales est leur rencontre datée. C'est précisément la définition B. □

On peut donc calculer B avec une rencontre/LCA par site, au lieu du balayage de toutes les coupes de `_node_core`. La formule opérationnelle est sB_i=max(e_i,d_i,birth(LCA(o_i,c_i))). La naissance de cet ancêtre commun peut être plus tardive que les deux entrées ; il faut la conserver. Il faut conserver les contacts fermés et les dates core qui dépassent la naissance de la racine.

**Stabilité.** Si chaque point bouge de ≤ε, FULL possède les morphismes d'entrelacement φ,ψ de décalage ε en rayon. Leur composition est la remontée de 2ε. L'alignement fort de H3 donne

    m_Y(H_i^Y, φH_i^X) ≤ e_i^X + 3ε.

Ce n'est pas une conséquence des seules inégalités |Δe| et |Δu_H| : cet alignement doit faire partie de la preuve géométrique H3.

L'ancre core vérifie, indépendamment,

    m_Y(Q_i^Y, φQ_i^X) ≤ d_i^X + 2ε.

En effet les k voisins X de x_i choisis au rayon d_i^X ont leurs images Y à distance ≤d_i^X+2ε de **tout le segment** x_i^X→x_i^Y. Ce segment relie donc les deux ancres dans L_k^Y à ce rayon ; d_i^Y≤d_i^X+2ε. Le transport donne aussi

    m_Y(φH_i^X, φQ_i^X) ≤ sB_i^X + ε.

Par l'inégalité ultramétrique des rencontres,

    sB_i^Y ≤ max(e_i^X+3ε, sB_i^X+ε, d_i^X+2ε)
           ≤ sB_i^X+3ε.

L'argument symétrique conclut |sB_i^Y−sB_i^X|≤3ε. La borne déjà acquise |Δu_H(i,j)|≤3ε, combinée à |ΔsB_j|≤3ε, contrôle aussi max(u_H(i,j),sB_j) en 3ε. Les maxima et statistiques d'ordre sont 1-Lipschitz pour la norme du sup ; les dates de comptage B et les dates de la factorisation condensée correspondante gardent donc la constante 3. Cela ne signifie pas que l'antichaîne sélectionnée par EOM reste identique.

**Vérification bornée.** 156 dates B, k=2..4, huit nouveaux nuages de 6–7 sites. Le programme exécute seulement l'AST figé de `_node_core` et des primitives exactes `Rad`, sans ses imports privés, et compare ses balayages à la formule de rencontre construite séparément sur la route A. La comparaison de radicaux est partagée pour trancher les égalités : ce n'est pas une nouvelle indépendance arithmétique de H3. Ces checks valident la définition et le raccord ; la preuve de la constante est celle ci-dessus.

B reste plus tardif que A, perd toujours T0 et certains points frontière, et ne devient pas un meilleur choix statistique grâce à cette borne. Il n'y a ici ni transfert aux insertions/suppressions, ni garantie inter-k, ni généralisation au cas infini où un infimum pourrait ne pas être atteint.

## Projection, sélection et statistique : frontières de la garantie

- Un point attaché une seule fois à un nœud vivant donne une famille laminaire **à k fixé**. La verticale FULL ne rend pas laminaire l'union des groupes de tous k. Une synthèse multi-k doit nommer la fidélité qu'elle abandonne.
- La couverture est ensembliste ; les amas de la thèse se recouvrent. La fermeture des co-couvertures est un quotient extérieur de points, jamais une reconstruction de FULL. H=P₁Π_m est une pendaison intérieure ; sa stabilité est en rayon, pas en rayon carré. Le banc expose les deux bras séparément : [points_campaign.py](git_source/morsehgp3D_v11/bench/points_campaign.py#L111). `margin_r` appelle `hang_margin_radius` ; `margin` reste un témoin historique distinct.
- Quantification : la borne √3h/2 nécessite un univers étiqueté apparié avant déduplication ; dédupliquer modifie les cardinalités. Les copies/poids fixes mathématiques ne qualifient pas le moteur natif à poids non unitaires. Changer h remet les unités : β_phys=h²β_grid et λ_phys=h^(−z)λ_grid. Aucun iso-rappel, hard owner, support ou plateau stable n'en découle.
- Pour une masse exclusive par date s_i≥e_i, la condensation doit transporter les cohortes de comptage. Lever une ultramétrique par une statistique d'ordre certifie l'éligibilité des blocs ; cela n'autorise pas à remettre tous leurs membres à leur naissance pour calculer EOM. Les points pendants inactifs et le bruit se déclarent séparément. La couverture FULL ne fournit pas une masse exclusive à elle seule.
- EOM : λ=r^(−z) est un paramètre de l'objectif. Changer z peut changer l'antichaîne même si FULL/H est inchangé. À condensation et cohortes identiques, augmenter z donne le raffinement monotone déjà prouvé dans le reçu précédent ; cette phrase ne compare pas A à C et ne dit pas que le score ou la qualité croît. Au score tie, une petite perturbation peut faire basculer parent/enfants. La borne H3 ne stabilise la sortie plate qu'avec une marge de décision et un calendrier condensé contrôlé.
- Le meilleur IoU indépendant de chaque objet est un diagnostic de présence de bons nœuds, pas le score d'une unique antichaîne plate. Les mêmes étiquettes ayant guidé le choix du modèle, les scores présents restent exploratoires ; aucune indépendance d'évaluation ni preuve d'optimalité statistique n'est acquise.
- λ∝r^(−3) a un sens de densité kNN dans un modèle volumique 3D approprié ; le score à masse empirique estime un contenu de probabilité pondéré, pas automatiquement l'excès de masse de Lebesgue de la définition 19. À K fixé, ni cela ni l'exactitude FULL ne prouvent la consistance d'un cluster tree. Les surfaces LiDAR, les objets/vides, les changements de dimension et les hypothèses d'échantillonnage doivent être explicités.
- H∞ et les résultats de Palm restent conditionnels à une règle infinie définie et mesurable, avec les minima/owners atteints ou une convention prouvée. L'exemple déterministe localement fini d'infimum non atteint ne prouve pas son occurrence PPP ; l'obstruction Palm à λ/r fixes ne prouve ni une convergence de fenêtres finies ni une comparaison aux seuils propres de fusion des méthodes.

Ces précautions renvoient aux reçus **clos** `points-answers-20261003`, `points-palm-obstruction-20261003`, `points_math_followup_20261004`, `flat_selection_math_r2_20261004` et `flat_model_followup_20261004`, tous extérieurs à cette capsule et inchangés. Elles confrontent le contrat actuel aux erreurs v4/v7/v10 : fold de points substitué à FULL, shell incomplète, terminal prétendu unique, état fermé utilisé pour une descente stricte, première incidence confondue avec couverture complète, binarisation de plateau, union inter-k dite laminaire et best-node dit clustering plat. Aucune de ces erreurs historiques n'est imputée à nouveau au moteur courant sans témoin.

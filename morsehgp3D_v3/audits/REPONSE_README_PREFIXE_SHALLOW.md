# Réponse aux questions Q0--Q3 du README sur le préfixe shallow

> [!IMPORTANT]
> Snapshot Q1--Q3 audité : commit `463d0758ad832995040f472d451d7838ad0a1a80`, `README.md` SHA-256 `3dc4db928f18e30b183a629b903c962e9d8571b300d49ad36c02c2c0253fd31e`. Q0 a ensuite été auditée au commit `95a009c0b4b1c8c1a8f0adf103de6fb9a4098989`, `README.md` SHA-256 `0cc8d51ec39aaab81995aef5a0bd93865adf8d78732a30a6826e5bd82f6e8c01`. Le dépôt a continué d'évoluer pendant la rédaction. Les résultats mathématiques ci-dessous ne dépendent pas des changements ultérieurs du prototype. `public_status=not_claimed`.

## Verdict court

| question | réponse |
| --- | --- |
| Q0 — certificat de localité | **Incomplet à l'égalité.** L'inclusion dans une boule fermée ne permet pas d'ignorer un plan tangent à sa frontière. Il faut une marge stricte ou traiter exactement toute la bande d'égalité. |
| Q1 — couches convexes | **Non.** Faux dès quatre lignes actives, à profondeur 1, dans une fixture Morse exacte et clippée par Jung. |
| Q2 — constructeur | La bonne voie est le sous-complexe de profondeur au plus $\kappa$ d'un arrangement de demi-plans, construit par incrémentation randomisée Las Vegas avec listes de conflits. Le coût théorique espéré est compatible avec $O(m\log m+m(\kappa+1))$ sous les hypothèses usuelles; son transfert exact au produit reste à réaliser et à juger. |
| Q2 — largeur du tri | Les produits croisés d'environ 210 bits sont évitables : l'ordre de deux croisements sur une droite se réduit au signe d'un déterminant homogène $3\times3$, inférieur à $2^{107{,}4}$ sur le profil u16 équilibré. Le tri peut donc rester en `i128`. |
| Q3 — rejet `O(1)` | **Aucun test complet brut en `O(1)`.** La propriété dépend globalement des autres demi-plans. Seuls des rejets suffisants, ou une requête après un prétraitement qui a déjà payé le problème, sont possibles. |

La conséquence architecturale est nette : **ne pas implémenter un onion peeling du nuage dual**. Il manquerait des sommets utiles, même en position générale et même dans la région de Jung.

## 1. Q0 : le certificat proposé a un trou d'égalité

Le README propose d'arrêter après les $M$ plans les plus proches de $p$ lorsque $V^{(M)}\subseteq B(p,d_{M+1}/2)$. La preuve observe correctement qu'un plan restant, à distance au moins $d_{M+1}/2$, ne coupe pas la **boule ouverte**. Elle en déduit à tort qu'il ne coupe pas $V^{(M)}$, alors que l'hypothèse place seulement $V^{(M)}$ dans la boule **fermée**.

Les deux inégalités employées dans la contradiction sont

$$\lVert c-p\rVert\geq\frac{d_{M+1}}{2}\quad\text{et}\quad\lVert c-p\rVert\leq\frac{d_{M+1}}{2}.$$

Elles sont compatibles à l'égalité. Un plan non traité peut être tangent à la boule au point $c$, intersecter la frontière de $V^{(M)}$ et ajouter une strate ou un support fermé, même si son demi-espace positif ne pénètre pas l'intérieur de la boule.

Le cas géométrique minimal est explicite : si $c$ est un point extrême de $V^{(M)}$ avec $\lVert c-p\rVert=R$, prendre un point non traité $u=2c-p$. Alors $\lVert u-p\rVert=2R$ et le médiateur $H_u$ passe exactement par $c$. La sphère centrée en $c$ qui passe par $p$ passe aussi par $u$; ignorer $u$ manque donc au moins le support fermé $\lbrace p,u\rbrace$.

Cette distinction est essentielle pour MorseHGP3D : l'égalité de l'ensemble sous-jacent ne suffit pas. Le produit transporte des strates, shells, supports, lots égaux et incidences. Un contact tangent peut ne pas changer l'intérieur du sous-niveau tout en changeant ces sorties.

### 1.1 Forme sûre du certificat

La version simple et suffisante exige une marge stricte certifiée :

$$\sup_{c\in V^{(M)}}\lVert c-p\rVert<\frac{d_{M+1}}{2}.$$

Sur des quantités carrées, la décision doit rester strictement $4\rho_{\mathrm{out}}^2<d_{M+1}^2$. Une égalité, un intervalle qui la contient ou un overflow rend le certificat non concluant et poursuit l'insertion.

Une variante moins conservatrice peut accepter la frontière seulement si elle :

1. insère en lot tous les points à la distance $d_{M+1}$;
2. rejoue exactement l'intersection de chaque médiateur de la bande d'égalité avec le sous-complexe fermé;
3. certifie qu'aucun plan encore non traité ne touche une strate utile;
4. conserve les shells et supports tangents avant l'arrêt.

Q0 devient donc **une voie de certificat valide après correction**, mais la proposition telle qu'écrite n'est pas démontrée. Une fixture d'égalité $u=2c-p$ doit précéder tout claim de localité.

## 2. Q1 est fausse : contre-exemple Morse minimal

Prenons l'ancre

- $p=(10,10,10)$;
- $q=(10,10,20)$;

et quatre points supplémentaires

- $x_1=(6,13,16)$;
- $x_2=(6,7,16)$;
- $x_3=(13,6,16)$;
- $x_4=(13,14,16)$.

Pour $d=q-p=(0,0,10)$, la base équilibrée du prototype est $b_1=(0,10,0)$ et $b_2=(-10,0,0)$. Les quatre formes $a_is_1+b_is_2=c_i$ donnent les points duaux

| point | $(a_i,b_i,c_i)$ |
| --- | --- |
| $x_1$ | $(60,80,4)$ |
| $x_2$ | $(-60,80,4)$ |
| $x_3$ | $(-80,-60,4)$ |
| $x_4$ | $(80,-60,4)$ |

Dans la section affine $c=4$, ces points sont les quatre sommets d'un quadrilatère strictement convexe, dans l'ordre $1,2,3,4$. La paire $(1,3)$ en est une diagonale : elle n'est donc l'arête d'aucune couche convexe. La première couche retire déjà les quatre points.

Pourtant, les droites de $x_1$ et $x_3$ se coupent exactement en $s=(-\frac{1}{5},\frac{1}{5})$. Les quatre formes y valent respectivement

$$0,\ 24,\ 0,\ -32.$$

Le sommet porté par $(1,3)$ a donc une profondeur stricte égale à 1. Il appartient au préfixe $\delta\leq1$ tout en étant absent des deux premières couches convexes.

### 2.1 Ce n'est pas un artefact hors produit

Le contre-exemple satisfait les contraintes qui comptent ici :

- aucune paire de droites n'est parallèle;
- aucun triplet n'est concurrent : les quatre déterminants triples valent, à signe près, `67200`, `67200`, `89600`, `89600`;
- le centre correspondant est $c=(\frac{19}{2},\frac{19}{2},15)$;
- son rayon carré est $r^2=\frac{51}{2}$;
- $D^2=\lVert p-q\rVert^2=100$ et l'autre distance maximale du support vaut 98 : $pq$ reste une paire diamètre;
- le bound de Jung vaut $\frac{3D^2}{8}=\frac{75}{2}$, donc $r^2=\frac{51}{2}<\frac{75}{2}$;
- $x_2$ est strictement intérieur à la sphère et $x_4$ strictement extérieur.

La sphère critique de support $\lbrace p,q,x_1,x_3\rbrace$ a ainsi un rang fermé 5. Elle est exactement le type d'événement que le peeling proposé doit conserver.

Le contre-exemple est minimal en nombre de lignes actives : avec au plus trois points duaux en position générale, toute paire est une arête de leur enveloppe. Pour $\kappa=0$, la correspondance avec l'enveloppe conique reste la bonne base. Dès $\kappa=1$, ni la position générale, ni la stricte convexité, ni la réalisabilité Morse, ni le clipping de Jung ne sauvent l'épluchage par couches.

### 2.2 Fixture permanente recommandée

Cette configuration doit devenir une fixture littérale, avec les assertions suivantes :

1. les quatre formes exactes ci-dessus sont construites depuis les six points 3D;
2. le sommet $(1,3)$ est présent avec profondeur 1;
3. la paire $(1,3)$ n'est pas une arête de la couche convexe duale;
4. le centre, le rayon, le support, le shell et le point intérieur sont rejoués exactement;
5. toute implémentation fondée uniquement sur les couches convexes est explicitement réfutée.

La fixture ne prouve pas le constructeur retenu; elle interdit définitivement le constructeur faux.

## 3. Q2 : la voie algorithmique crédible

L'objet à construire n'est pas une suite d'enveloppes du nuage dual. Dans le plan de paramètres, c'est le sous-complexe des faces de l'arrangement des **demi-plans orientés** dont la profondeur est au plus $\kappa_e$.

Pour une ancre $e$, l'ordre des opérations devrait être :

1. classifier exactement les formes constantes et les frontières qui manquent la région de Jung;
2. ajouter les constantes intérieures à $c_e$ et poser $\kappa_e=s_{\max}-4-c_e$ pour l'arité quatre;
3. rejeter l'ancre si $\kappa_e<0$;
4. construire seulement le sous-complexe de profondeur au plus $\kappa_e$ par une incrémentation randomisée Las Vegas, avec décomposition verticale et listes de conflits;
5. clipper ou rejouer exactement les sorties contre l'ellipse de Jung;
6. grouper les concurrences, vérifier shell, bon centrage, propriétaire et rang fermé avant émission;
7. comparer le transcript complet au balayage dense sur les petites instances.

La randomisation doit seulement affecter le temps. La correction reste déterministe : graine scellée, transcript rejouable et résultat canonique indépendant de l'ordre d'insertion.

### 3.1 Ce que dit réellement la littérature

La complexité combinatoire cumulative des faces peu profondes d'un arrangement de demi-plans est en $\Theta(m\kappa)$ au pire. La voie de construction pertinente est la construction incrémentale randomisée des faces de profondeur au plus $\kappa$ pour des pseudodisques, dont les demi-plans sont un cas particulier.

- Aronov et Har-Peled rappellent qu'un tel sous-complexe peut être construit en temps espéré $O(m\kappa+m\log m)$ pour des pseudodisques : [article et fait de construction](https://doi.org/10.1137/060669474).
- Agarwal, de Berg, Matoušek et Schwarzkopf donnent la machinerie incrémentale des niveaux d'arrangements; leur borne planaire publiée est $O(m\kappa+m\alpha(m)\log m)$ : [article SIAM](https://doi.org/10.1137/S0097539795281840).
- Everett, Robert et van Kreveld donnent l'algorithme optimal $O(m\log m+m\kappa)$ pour les premiers niveaux d'un arrangement de lignes dans l'orientation standard : [article IJCGA](https://doi.org/10.1142/S0218195996000186).
- Chan traite explicitement les demi-plans arbitrairement orientés et confirme le cadre combinatoire des niveaux bichromatiques : [article](https://doi.org/10.1145/1824777.1824782).

Ces références établissent une voie théorique; elles ne livrent pas directement le kernel Morse. Il reste à formaliser la compactification des demi-plans non bornés, le clipping exact par Jung, les égalités, les parallèles, les concurrences et le payload HGP. La bonne revendication actuelle est donc `algorithmic_route_supported`, pas `constructor_proved`.

### 3.2 Le tri des croisements peut rester en `i128`

Le README estime que comparer deux positions rationnelles le long d'une droite exige des produits d'environ 210 bits. C'est vrai pour la multiplication croisée naïve; elle n'est pas nécessaire.

Écrivons la droite $i$ sous forme homogène $\lambda_i=(a_i,b_i,-c_i)$ et posons $D_{ij}=a_ib_j-a_jb_i$. Si $\tau_{ij}$ désigne la position du croisement de $i$ et $j$ le long de l'orientation $(-b_i,a_i)$, alors

$$\mathrm{sgn}(\tau_{ij}-\tau_{ik})=\mathrm{sgn}\!\left(\det(\lambda_i,\lambda_j,\lambda_k)\right)\,\mathrm{sgn}(D_{ij})\,\mathrm{sgn}(D_{ik}).$$

Le produit croisé des deux rationnels se factorise par $a_i^2+b_i^2$, strictement positif, et ce facteur s'annule dans la comparaison. Il ne faut former ni $\det(\lambda_i,\lambda_j,\lambda_k)D_{ij}D_{ik}$, ni les deux produits de 210 bits : seuls leurs signes sont combinés.

Avec la base équilibrée et la grille u16 déclarée, $|a|,|b|<2^{34{,}6}$ et $|c|<2^{35{,}6}$. Les six termes du déterminant triple donnent la borne conservatrice

$$\left|\det(\lambda_i,\lambda_j,\lambda_k)\right|<2^{107{,}4},$$

qui tient avec une marge importante dans un entier signé de 128 bits. Le même chirotope décide de quel côté d'un sommet se trouve un troisième demi-plan.

Les cas limites sont sémantiques, pas numériques :

- $D_{ij}=0$ signifie parallèle et suit la branche constante le long de $i$;
- $\det(\lambda_i,\lambda_j,\lambda_k)=0$ signifie concurrence et impose un lot atomique de croisements égaux;
- aucune perturbation arbitraire ne peut remplacer ce lot, car le shell et la profondeur stricte font partie du contrat.

Ce résultat autorise un tri combinatoire `i128` pour `quantized_u16_input`. Il ne prouve pas que toute la v3 tient en `i128` : arité trois, clipping, ordre global des niveaux, profil dyadique et certains produits géométriques gardent un filtre `i128` suivi d'un repli fixe 256/384 bits ou multiprécision.

### 3.3 Prototype utile, sans refaire l'arrangement complet

Le prochain prototype CPU devrait être un constructeur local et éphémère par ancre :

- aucune matrice paire--point globale;
- aucune mosaïque de Delaunay d'ordre supérieur persistée;
- stockage limité aux faces shallow, conflits et sorties de l'ancre courante;
- compteurs `lines_input`, `conflict_tests`, `cells_created`, `cells_discarded`, `faces_by_depth`, `vertices_emitted`, `ties_batched`, `peak_bytes`;
- différentiel bit-à-bit contre le sweep dense pour $m$ petit;
- mesure séparée de la source A1, du constructeur shallow, du tri global HGP et de l'aval.

Le coût local attendu ne ferme pas la cible 50 k : il faut encore que la source A1 et $M=\sum_e m_e$ soient effectivement sparse.

## 4. Q3 : aucun rejet complet brut en temps constant

Fixons une droite $\ell_i$ et sa corde $I=\ell_i\cap J_e$. Chaque autre demi-plan, restreint à $I$, est l'un des quatre objets suivants : vide, tout $I$, un préfixe de $I$ ou un suffixe de $I$. La profondeur le long de $I$ est donc la somme de toutes ces fonctions seuil.

Décider si **aucun** croisement de $\ell_i$ n'a une profondeur au plus $\kappa$ dépend de l'ensemble des seuils et de leurs orientations. Ce n'est pas une propriété des seuls coefficients de $\ell_i$.

Un adversaire suffit à exclure un test complet brut en `O(1)` :

1. préparer sur $I$ un croisement de profondeur exactement $\kappa$;
2. choisir parmi les entrées non lues un demi-plan dont la frontière ne coupe pas $I$;
3. dans une entrée, orienter ce demi-plan négativement sur tout $I$ : le sommet reste shallow;
4. dans l'autre, l'orienter positivement sur tout $I$ : toutes les profondeurs sur $I$ augmentent de un et le sommet sort du préfixe.

Les deux entrées ont la même droite cible et peuvent être perturbées pour que les frontières auxiliaires se coupent seulement hors de la région compacte de Jung. La position du record décisif étant arbitraire, un algorithme sans prétraitement doit inspecter $\Omega(m)$ données au pire.

Après avoir construit le sous-complexe shallow, stocker un bit `incident` permet bien une requête `O(1)` par ligne. C'est circulaire : le coût a déjà été payé par la construction. Un index global peut déplacer le travail, pas le supprimer.

### 4.1 Rejets `O(1)` sûrs mais seulement suffisants

Les filtres suivants restent utiles :

- la frontière de la forme ne rencontre pas l'ellipse de Jung, donc la forme est constante sur la région;
- $c_e>s_{\max}-4$, donc l'ancre entière ne peut porter un support quatre utile;
- un certificat géométrique établit `carrier_eligible=false`, donc cette ligne ne peut émettre un support;
- une borne inférieure certifiée de profondeur dépasse $\kappa$ sur toute une cellule du cover.

Le troisième point exige une séparation cruciale : une forme inéligible comme **carrier** peut rester positive à l'intérieur et doit encore contribuer à la profondeur comme **témoin**. Elle ne peut pas être retirée du flux de comptage.

Ces tests doivent être fail-open. Une ambiguïté, une égalité ou un overflow conserve la ligne ou déclenche le repli exact; elle ne justifie jamais un faux rejet.

## 5. Décision proposée

1. Corriger Q0 par une marge stricte ou par un traitement exact de toute la bande d'égalité; ajouter la fixture tangente.
2. Fermer Q1 comme **réfutée**, enregistrer la fixture six-points ci-dessus et interdire l'onion peeling comme architecture.
3. Remplacer le comparateur rationnel du sweep par le chirotope `i128` seulement après une preuve de largeur dans le code et des fixtures de parallèle, concurrence et ordre inversé.
4. Construire un premier RIC shallow CPU exact, borné et différentiel; ne pas commencer par le GPU.
5. Garder le sweep dense comme oracle local et générateur de fixtures, jamais comme chemin produit.
6. Ne pas investir dans un prédicat complet `O(1)` par ligne : appliquer les filtres suffisants, puis laisser le constructeur output-sensitive éliminer le reste.

## Conclusion

La nouvelle réduction duale du README est utile, mais elle conduit au **problème des niveaux peu profonds de demi-plans**, pas à un épluchage de couches convexes. Q0 devient sûr avec une marge stricte; un contre-exemple produit minimal ferme définitivement Q1; le déterminant homogène triple retire l'obstacle de 210 bits du tri u16. La difficulté restante est algorithmique et globale : construire exactement le sous-complexe shallow et ses conflits sans matérialiser toutes les intersections.

GCP non utilisé.

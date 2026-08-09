# Réponse aux questions Q0--Q3 du README sur le préfixe shallow

> [!IMPORTANT]
> Snapshot Q1--Q3 audité : commit `463d0758ad832995040f472d451d7838ad0a1a80`, `README.md` SHA-256 `3dc4db928f18e30b183a629b903c962e9d8571b300d49ad36c02c2c0253fd31e`. Q0 a ensuite été auditée au commit `95a009c0b4b1c8c1a8f0adf103de6fb9a4098989`, `README.md` SHA-256 `0cc8d51ec39aaab81995aef5a0bd93865adf8d78732a30a6826e5bd82f6e8c01`. Le dépôt a continué d'évoluer pendant la rédaction. Les résultats mathématiques ci-dessous ne dépendent pas des changements ultérieurs du prototype. `public_status=not_claimed`.

> [!WARNING]
> Correction du 9 août 2026 : la première fixture Q1 des sections 2--2.2 n'est pas bien centrée et ne suffisait pas. La section 2.4 la remplace par une fixture exacte, bien centrée, dans RelevantGP et rejouée par le prototype. Q1 est donc désormais réfutée même pour les événements Morse utiles. La section 2.3 corrige séparément le carré dual invalide ajouté au README au commit `1216d16`.

> [!NOTE]
> La fixture définitive a été retrouvée et certifiée par deux recherches rationnelles indépendantes. Un rejeu complet contre l'oracle multiprécision et le sujet `edge_shallow` donne une campagne décidée, zéro rejet de domaine, zéro réfutation du dictionnaire et une fermeture structurelle complète à $s_{\max}=5$. Les scripts et binaires de vérification sont restés sous `/tmp`.

## Verdict court

| question | réponse |
| --- | --- |
| Q0 — certificat de localité | **Incomplet à l'égalité.** L'inclusion dans une boule fermée ne permet pas d'ignorer un plan tangent à sa frontière. Il faut une marge stricte ou traiter exactement toute la bande d'égalité. |
| Q1 — couches convexes | **Non, même après bon centrage et RelevantGP.** Une fixture u16 de six points contient un événement Morse de rang 5 porté par une diagonale de la première couche conique; l'onion peeling le perd. |
| Q2 — constructeur | La bonne voie est le sous-complexe de profondeur au plus $\kappa$ d'un arrangement de demi-plans, construit par incrémentation randomisée Las Vegas avec listes de conflits. Le coût théorique espéré est compatible avec $O(m\log m+m(\kappa+1))$ sous les hypothèses usuelles; son transfert exact au produit reste à réaliser et à juger. |
| Q2 — largeur du tri | Les produits croisés d'environ 210 bits sont évitables : l'ordre de deux croisements sur une droite se réduit au signe d'un déterminant homogène $3\times3$, inférieur à $2^{107{,}4}$ sur le profil u16 équilibré. Le tri peut donc rester en `i128`. |
| Q3 — rejet `O(1)` | **Aucun test complet brut en `O(1)`.** La propriété dépend globalement des autres demi-plans. Seuls des rejets suffisants, ou une requête après un prétraitement qui a déjà payé le problème, sont possibles. |

La conséquence sûre est : **ne pas présenter un onion peeling comme constructeur du complexe shallow ni comme générateur complet des événements Morse**. La fixture bien centrée de la section 2.4 montre que la restriction au sous-ensemble utile ne répare pas l'identité.

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

## 2. Q1 : première fixture brute insuffisante, puis contre-exemple Morse décisif

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

### 2.1 Ce que la fixture établit — et ce qu'elle n'établit pas

Le contre-exemple satisfait plusieurs contraintes fortes :

- aucune paire de droites n'est parallèle;
- aucun triplet n'est concurrent : les quatre déterminants triples valent, à signe près, `67200`, `67200`, `89600`, `89600`;
- le centre correspondant est $c=(\frac{19}{2},\frac{19}{2},15)$;
- son rayon carré est $r^2=\frac{51}{2}$;
- $D^2=\lVert p-q\rVert^2=100$ et l'autre distance maximale du support vaut 98 : $pq$ reste une paire diamètre;
- le bound de Jung vaut $\frac{3D^2}{8}=\frac{75}{2}$, donc $r^2=\frac{51}{2}<\frac{75}{2}$;
- $x_2$ est strictement intérieur à la sphère et $x_4$ strictement extérieur.

Cependant, les coordonnées barycentriques exactes du centre dans le tétraèdre ordonné $(p,q,x_1,x_3)$ sont $(\frac{1}{10},-\frac{1}{10},\frac{1}{2},\frac{1}{2})$. Le coefficient de $q$ est négatif : le centre n'appartient pas à $\mathrm{relint}\,\mathrm{conv}\lbrace p,q,x_1,x_3\rbrace$. Le support n'est donc pas bien centré et la sphère n'est pas un événement critique Morse.

Le contre-exemple est minimal en nombre de lignes actives pour l'identité combinatoire brute : avec au plus trois points duaux en position générale, toute paire est une arête de leur enveloppe. Pour $\kappa=0$, la correspondance avec l'enveloppe conique reste la bonne base. Dès $\kappa=1$, la position générale, la stricte convexité, la réalisabilité des formes et le clipping de Jung ne sauvent pas l'épluchage comme constructeur de **tout** le préfixe shallow. Le bon centrage pourrait encore restreindre le sous-ensemble utile; cette question doit être tranchée séparément.

### 2.2 Fixture permanente recommandée

Cette configuration reste une fixture négative utile, avec les assertions suivantes :

1. les quatre formes exactes ci-dessus sont construites depuis les six points 3D;
2. le sommet $(1,3)$ est présent avec profondeur 1;
3. la paire $(1,3)$ n'est pas une arête de la couche convexe duale;
4. le centre, le rayon, le support, le shell, le point intérieur et les barycentriques sont rejoués exactement;
5. le support est explicitement marqué `well_centred=false`;
6. toute revendication « couches convexes = complexe shallow complet » est réfutée.

Cette première fixture doit rester enregistrée comme historique du contre-exemple combinatoire, avec `well_centred=false`. Elle ne doit plus servir seule au verdict produit : la section 2.4 fournit la fixture décisive manquante.

### 2.3 Le contre-exemple ajouté au README au commit `1216d16` n'est pas un sommet fini

Le README emploie les quatre points duaux $(1,0,1)$, $(0,1,1)$, $(-1,0,1)$, $(0,-1,1)$ et présente les deux diagonales comme des sommets de profondeur 1. Or chaque diagonale relie une paire de droites parallèles :

- $(1,0,1)$ et $(-1,0,1)$ donnent $s_1=1$ et $s_1=-1$;
- $(0,1,1)$ et $(0,-1,1)$ donnent $s_2=1$ et $s_2=-1$.

Le produit vectoriel de chaque paire duale possède une troisième composante nulle; le plan porteur par l'origine ne peut donc pas être normalisé sous la forme $(s_1,s_2,-1)$. L'intersection est à l'infini, pas un sommet du plan de paramètres et encore moins un centre dans Jung.

Ce carré ne réfute donc pas Q1. La fixture des sections 2--2.2 réfute l'arrangement brut, mais reste non bien centrée. La fixture suivante ferme en revanche la question Morse avec des intersections finies et des barycentriques strictement positives.

### 2.4 Contre-exemple exact bien centré dans RelevantGP

La fixture suivante est plus forte que la première : ses quatre formes ont le même coefficient `c`. La diagonale est donc absente aussi bien de l'onion affine coplanaire que de l'onion conique, sans ambiguïté projective. Une translation positive place les six points dans le profil u16 :

- ancre $p=(10,10,1)$ et $q=(10,10,9)$;
- porteurs choisis $z=(13,13,5)$ et $w=(13,7,5)$;
- témoins $u=(14,9,6)$ et $v=(11,6,6)$.

Avant translation, les coordonnées sont $p=(0,0,-4)$, $q=(0,0,4)$, $z=(3,3,0)$, $w=(3,-3,0)$, $u=(4,-1,1)$ et $v=(1,-4,1)$. Pour $d=q-p=(0,0,8)$, la base du prototype vaut $b_1=(0,8,0)$ et $b_2=(-8,0,0)$. Les formes duales, dans l'ordre $(z,w,u,v)$, sont

$$ (48,-48,8),\quad(-48,-48,8),\quad(-16,-64,8),\quad(-64,-16,8). $$

Elles appartiennent toutes au plan $c=8$. Leur ordre convexe est $(u,z,v,w)$; le segment $(z,w)$ est une diagonale, coupée intérieurement par l'autre diagonale $(u,v)$. Les quatre points sont retirés à la première couche et la paire $(z,w)$ n'est arête d'aucune couche ultérieure.

Les droites de $z$ et $w$ se coupent pourtant au point fini $s=(0,-1/6)$. Les résidus $a_i s_1+b_i s_2-c_i$, dans l'ordre $(z,w,u,v)$, valent exactement

$$ 0,\quad0,\quad8/3,\quad-16/3. $$

La profondeur stricte est donc 1. Le centre physique avant translation est $C=(1/3,0,0)$ et son rayon carré vaut $145/9$. Ses coordonnées barycentriques dans le tétraèdre ordonné $(p,q,z,w)$ sont

$$ (4/9,\quad4/9,\quad1/18,\quad1/18). $$

Elles sont toutes strictement positives : le support est bien centré. Le point $u$ est strictement intérieur avec distance carrée $139/9$, le point $v$ est strictement extérieur avec distance carrée $157/9$, et le shell contient exactement $p,q,z,w$. Le rang fermé est donc $4+1=5$.

Les autres contrôles exacts ferment les échappatoires usuelles :

- $r^2=145/9<24$, donc le centre est strictement dans Jung;
- toutes les distances carrées du nuage sont au plus 64 et celle de $(p,q)$ vaut 64, maximum unique;
- le support est affinement indépendant;
- aucun couple de formes n'est parallèle;
- les quatre déterminants homogènes triples valent `-12288`, `24576`, `30720` et `-6144`, donc aucune concurrence triple;
- une vérification rationnelle exhaustive de tous les sous-ensembles de tailles 2 à 6 trouve zéro ambiguïté et zéro violation de RelevantGP;
- le prototype live à $s_{\max}=5$ rejoue le support `{0,1,2,3}` au rang 5 avec les membres `{0,1,2,3,4}`, `degenerate_shells=0` et `dictionary_refuted=0`.

Cette fixture réfute donc Q1 au niveau exact requis par le produit : **un événement Morse bien centré, dans RelevantGP et strictement dans Jung peut être porté par une diagonale absente de toutes les couches onion**. Le test permanent doit encoder les coordonnées, la diagonale, la profondeur, le centre, les barycentriques, le shell, le rang et l'absence de dégénérescence.

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
2. Fermer l'identité « onion = complexe shallow » comme **réfutée**, y compris après restriction aux événements bien centrés; conserver l'ancienne fixture avec `well_centred=false` comme historique et promouvoir la fixture de la section 2.4 en régression permanente.
3. Remplacer le comparateur rationnel du sweep par le chirotope `i128` seulement après une preuve de largeur dans le code et des fixtures de parallèle, concurrence et ordre inversé.
4. Construire un premier RIC shallow CPU exact, borné et différentiel; ne pas commencer par le GPU.
5. Garder le sweep dense comme oracle local et générateur de fixtures, jamais comme chemin produit.
6. Ne pas investir dans un prédicat complet `O(1)` par ligne : appliquer les filtres suffisants, puis laisser le constructeur output-sensitive éliminer le reste.

## Conclusion

La nouvelle réduction duale du README est utile, mais elle conduit au **problème des niveaux peu profonds de demi-plans**, pas à une identité générale avec les couches convexes. Q0 devient sûr avec une marge stricte; Q1 est réfutée même après bon centrage et RelevantGP; le déterminant homogène triple retire l'obstacle de 210 bits du tri u16. La difficulté restante est algorithmique et globale : construire exactement le sous-complexe utile et ses conflits sans matérialiser toutes les intersections.

GCP non utilisé.

# Lentille 1 — Objet mathématique et définition de la tour HGP FULL (auditeur C)

Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. Base lue : worktree détaché `build/v9-audit-c-worktree` à `0125dc18`, en lecture seule. GCP non utilisé. Aucune compilation. Trois scripts Python exacts (`Fraction`) ont tourné sous `nice -n 19`, en un seul fil, environ 80 s CPU au total. Ils sont dans `scratchpad/agents/lentille1/` : `hgp_block_oracle.py` (sha256 `77a6a992…`), `lower_link_pi0.py` (`e328e4a5…`) et `e5_gabriel_only.py` (`ce23c294…`). Tous rendent PASS sous `python3` et sous `python3 -O`.

Statuts employés :
- **prouvé** : théorème du manuscrit, entrée du registre, ou preuve écrite ici ;
- **testé** : porte, fixture ou oracle borné ;
- **mesuré** : reçu épinglé ;
- **supposé**.

## 1. Le but : l'objet exact

**Densité K-NN.** Le manuscrit définit $r_K(y)$ comme la distance de $y$ à son K-ième voisin, et $\hat f_K(y)=K/(n\omega_p r_K(y)^p)$ (Déf. 7, texte extrait l. 450–480). Le niveau supérieur $\hat f_K\geq\lambda$ équivaut à $r_K\leq r$ (Remarque 3). On a donc

$$L_K(r)=\lbrace y:\lvert B(y,r)\cap X\rvert\geq K\rbrace=\lbrace y: d_K(y)\leq r\rbrace,$$

avec des **boules fermées**. C'est exactement la **K-multicouverture** : l'ensemble des points couverts par au moins K boules $B(x,r)$, $x\in X$.

**Amas discrets.** Les amas de forte densité sont $\pi_0(L_K(r))$. L'amas discret d'une composante est $X\cap\delta_r(C)$, soit les points à distance au plus r de C (Déf. 8, l. 512–540). **Prouvé** (externe).

**Čech et K-polyèdres.**
- Complexe de Čech à boules fermées : $\sigma\in\check C(X,r)\iff\rho(\sigma)\leq r$, où $\rho$ est le rayon de la miniboule (Déf. 20, l. 1862 ; Déf. 25, l. 3028).
- Graphe $\Gamma_K$ : ses sommets sont les K-sous-ensembles (les (K−1)-simplexes) ; deux sommets sont adjacents si leur union est un simplexe (Déf. 21, l. 1893).
- Un K-polyèdre est l'ensemble des points d'une composante de $\Gamma_K$ ; HGP-Clusterer est $\theta_K^{HGP}(r)$ (Déf. 22, l. 1915).
- Prop. 5 (l. 3114) : les adjacences élémentaires, par cofaces de K+1 points, suffisent.

**Théorème 2** (l. 1986, preuve l. 2011–2050). Les régions témoins $T_r(\sigma)=\bigcap_{x\in\sigma}B(x,r)$ sont des convexes compacts. $L_K(r)$ est leur union sur les $\lvert\sigma\rvert=K$, et $\Gamma_K$ est leur graphe d'intersection. On obtient une bijection $\pi_0(L_K(r))\leftrightarrow$ composantes de $\Gamma_K$, avec $P_C=C^{discret}$. Le registre la classe `theorem_external` (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:30`).

**Hiérarchie en r et tour sur K.** La hiérarchie en r est l'arbre de fusion de ces composantes, facettes isolées comprises (profil FULL). La **tour sur K** n'est pas dans le manuscrit, qui traite chaque K séparément. Elle vient de la spécification (`docs/SPECIFICATION_MORSEHGP3D.md:85–103`) : $L_\ell(a)\subseteq L_k(a)$ pour $k<\ell$, avec $a=r^2$, et les applications verticales sont celles induites par l'inclusion. L'objet est donc la partie $H_0$ de la **bifiltration de multicouverture**, restreinte à $K\leq K_{\max}$.

**Ce que publie la v9.** `src/tower/forest/full_ball_tower.hpp` publie, pour chaque ordre K :
- une forêt de naissances, de multifusions à parents pré-lot et de contributions de couverture ;
- l'image verticale de chaque nœud à son niveau de création fermé (l. 97–102).

Le statut est `complete_relative` **relativement au catalogue fourni** (l. 20–21 ; `src/chain/tower_chain.cpp:26, 557`).

## 2. Le lien avec $d_K$ et la lecture des simplexes

L'objet est bien $\pi_0$ des sous-niveaux fermés de $d_K=r_K$.

- **Facette (K points).** Une facette σ entre dans la filtration à $r=\rho(\sigma)$, sous la forme du seul centre $c_\sigma$ de sa miniboule. C'est une **naissance isolée** si et seulement si aucune coface de même niveau ne la touche, c'est-à-dire si la boule fermée $B_\sigma$ ne contient aucun point étranger. C'est le « minimum Gabriel strict » (`morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md:70–75`).
- **Coface (K+1 points).** Une coface Q traduit $T(\tau)\cap T(\tau')\neq\varnothing$ pour ses facettes.
- **Niveaux et coupes.** Les niveaux sont des rayons carrés rationnels exacts (spec § 2). La tour admet des coupes ouvertes et fermées (`full_coverage_certificate.hpp:197–200`) ; la coupe normative du manuscrit est la coupe **fermée**.

## 3. Boules critiques, indice de Morse, admission $p+q_{\min}\leq K+1$

Pour une boule B(c,R), on note I son intérieur strict ($p=\lvert I\rvert$), U sa coquille ($u$) et $q_{\min}$ la taille d'un plus petit support positif ($c\in\mathrm{relint}\,\mathrm{conv}$).

**Sous-ensembles portés par B.** Les sous-ensembles de miniboule B sont les $I'\cup U'$ tels que $c\in\mathrm{conv}(U')$. Un tel sous-ensemble est **Gabriel** au sens de la Déf. 28 (intérieur **ouvert** vide) si et seulement si $I'=I$. Ce résultat est **prouvé sans position générale** (registre l. 66). Les cardinaux Gabriel portés par B remplissent donc l'intervalle $[p+q_{\min},\,p+u]$.

**Indice de Morse** (Reani–Bobrowski ; registre l. 42–46 ; spec l. 128–151).
- c est critique pour $d_k$ si et seulement si $c\in\mathrm{relint}\,\mathrm{conv}(U)$ et $p<k\leq p+u$.
- L'indice vaut $\mu=p+u-k$, avec une multiplicité $\binom{u-1}{\mu}$.
- $\pi_0$ ne change qu'à $\mu=0$ (naissance à $k=p+u$) et à $\mu=1$ ($k=p+u-1$, avec u bras $F_u=(I\cup U)\setminus\lbrace u\rbrace$, donc au plus u−1 fusions).

J'ai contrôlé la partie $\pi_0$ de ce résultat numériquement (`lower_link_pi0.py`, u = 2, 3, 4). Le lien inférieur, pris avec une marge ε, est :
- vide si $j=k-p=u$ ;
- formé de **u** composantes si $j=u-1$ ;
- connexe si $j\leq u-2$.

En position générale, u = q : l'indice $q+p-K$ vaut 0 ou 1 si et seulement si $K\in\lbrace p+q-1,\,p+q\rbrace$.

**Lecture équivalente dans le manuscrit.**
- Naissances : facettes Gabriel de cardinal K.
- Fusions : K-simplexes Gabriel (K+1 points), d'après le Th. 4 (l. 3196, sous la position générale de la Déf. 26, l. 3053).

Une coface Gabriel portée par B impose $p+q_{\min}\leq K+1$. D'où :
- l'**admission** $p+q_{\min}\leq K_{\max}+1$ (`tower_chain.cpp:492–494`, `full_ball_tower.hpp:962–963`) ;
- le **calendrier** $[p+q_{\min}-1,\ \min(K_{\max},p+u)]$ (`full_ball_tower.hpp:965–966, 997–998`) ;
- les seuils de voie $h_q=K_{\max}+2-q$, puisque un intérieur strict $p<h_q$ équivaut à $p+q\leq K_{\max}+1$.

**Hors position générale**, l'exclusion est fondée par le **Th. 4.2** (registre l. 120, `proved_here`) : si $p+s\geq K+2$ pour **un** support minimal quelconque de taille s, la boule est $H_0$-inerte, même avec des coquilles supplémentaires. Prendre $s=q_{\min}$ donne l'exclusion la plus faible, donc sûre ; c'est ce que fait `local_plateau.hpp:108`.

## 4. Minima Gabriel, multifusions, parents, verticales, extension non régulière

**Vocabulaire.**
- **Minima Gabriel** : sous régularité, les naissances FULL d'ordre K, c'est-à-dire les facettes Gabriel de cardinal K (registre l. 131, `conditional_theorem`).
- **Multifusions** : nœuds à au moins deux **parents**. Les parents sont les racines distinctes de l'état strictement antérieur au niveau (pré-lot). Toutes les boules d'un même niveau exact sont fermées ensemble en un lot atomique ; un traitement séquentiel binariserait les multifusions (registre l. 238).
- **Verticales** : image d'un nœud d'ordre K dans l'ordre K−1, à son niveau fermé, lue dans l'ancre de la même boule à K−1.
- **Extension non régulière** : plateaux cosphériques (u > q_min), naissances couvrant plus de K points, continuations datées avec contribution de couverture, ancres aux ordres inertes, descente lexicographique du résolveur (`morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md:16–107`).

**Théorèmes 4 à 7.**
- Le Th. 4 justifie de ne chercher les fusions que parmi les cofaces Gabriel.
- La Prop. 6 (l. 3295) et le Th. 5 (l. 3336) sont faux en général (registre l. 40–41, fixture E5). J'ai reproduit E5 exactement (`e5_gabriel_only.py`) : au niveau 83886/3563, Γ₂ complet donne {ABCDE}, alors que le graphe des seules cofaces Gabriel donne {ABC, ACDE}. La faille est précise : AC naît à 33/2 et s'y attache silencieusement par ACD et ACE, qui ne sont pas Gabriel (intrus E, D). Le graphe Gabriel la réintroduit plus tard comme sommet neuf. C'est pourquoi la v7 et la v9 rattachent chaque facette par un résolveur (ancre par (K, BallKey) ou descente par intrus) au lieu de replier les seules cofaces Gabriel (registre l. 129).
- Les Th. 6 (l. 3399) et 7 (l. 3638) placent les simplexes Gabriel dans $\mathrm{Del}_K$ et $\mathrm{Del}_{K-1}$. La v9 ne construit aucune mosaïque. Son générateur énumère les miniboules de rang au plus K+1 (WSPD, voies q2/q3/q4). La complétude de ce générateur repose sur les preuves v8 (citron, front), pas sur le Th. 7 (registre : « not_proved » comme énumération).

## 5. Théorème ou heuristique

| Énoncé | Statut |
| --- | --- |
| Th. 2, Prop. 5, Th. 4 (sous position générale) | `theorem_external` |
| Reani–Bobrowski (fenêtre, indice, multiplicité, seuls $s\in\lbrace k,k+1\rbrace$ changent $H_0$) | `theorem_external` |
| Lemme des porteurs Gabriel (l. 66) ; Th. 4.2 (l. 120) | `proved_here`, sans position générale |
| Prop. 6, Th. 5, fold v4 et flot des seules cofaces Gabriel (l. 40–41, 129) | `false_in_general` |
| Naissances FULL = minima Gabriel ; minima + multifusions reconstruisent FULL ; une ancre par naissance suffit (l. 131–134) | `conditional_theorem`, **sous régularité** |
| Contraction des plateaux par composantes fortement connexes (l. 237) | `proof_obligation` |
| Extension non régulière effectivement codée (ShellTable, contributions datées, descente) | **aucune entrée au registre** |

Pour cette dernière ligne, il existe seulement des preuves locales : la contrelecture B du quotient (`morsehgp3D_v9/audits/CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md`) et les juges bornés T2 (`tests/tower/full_ball_tower_gate.cpp:118–150`). `README.md:23` écrit pourtant « définie et prouvée en v7 », et `HERITAGE_V7_V8.md:20` « prouvé conditionnel + testé ».

## 6. Écarts entre la définition normative et l'objet v9

**Position générale.** La Déf. 26 (aucun point étranger sur la frontière d'une miniboule) est **fausse sur toutes les trames du contrat** (mesuré, R7b). Coquilles étendues pour 08/000000, 000100 et 000200 :

| Ordre | 000000 | 000100 | 000200 |
| --- | ---: | ---: | ---: |
| K5 | 227 | 135 | 572 |
| K10 | 444 | 280 | 1 301 |

Cela représente 0,006 à 0,041 % des boules, avec une coquille maximale de 5 (`receipts/g4_tower_r7b_20260923/vm/probe_{0,4,8,12,16,20}.stdout`). Les Th. 4–7 et le Fait 12 ne s'invoquent donc pas directement. Chaque condensé publié dépend de l'extension non régulière.

**Inégalités.** Elles sont conformes :
- intérieur strict (puissance < 0) et coquille (= 0) (`tower/pipeline/census.hpp:180–197`) ;
- Gabriel à boule ouverte ;
- niveaux fermés ;
- coupes ouvertes et fermées distinguées.

**Coquilles de plus de 12 sites.** Refus transactionnel `unsupported_degeneracy` (`tower_chain.cpp:478, 521`), déjà relevé par l'auditeur A (§ 1) et par B.

**K = n.** `kmax=min(requested,n)` (`full_ball_tower.hpp:884`) ; la fixture « pair » est à K = n = 2. Sans objet pour n ≥ 30 000.

**Sites dupliqués.** La spec § 3 compte les distances « avec multiplicité » (l. 91). La v9 refuse les positions dupliquées (`full_ball_tower.hpp:880`). Sur les trois trames, cela n'a aucun effet mesuré : pas de doublon XYZ et aucune fusion à 1 mm (`morsehgp3D_v8/docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md:108–116`). Le point reste ouvert pour les trames multi-échos (recoupe la lentille 11, point 9 et question 2).

**Quantification.** L'objet calculé est HGP des sites distincts de la grille 1 mm, ce qui est déclaré. C'est d'ailleurs la grille qui crée les égalités et les plateaux.

**Identités.** Les PointId externes sont conservés et les doublons d'ID refusés. Les composantes sont identifiées par identité, jamais par ensemble de points. C'est conforme au foncteur $\pi_0$ de la spec, mais la Déf. 22 est ensembliste (question ouverte 1).

**Poids du § 9.1.** Les scores et le vote ne sont pas produits. C'est déclaré comme profil séparé (`PLAN_V9.md:133`).

## 7. Esquisse de preuve de l'extension non régulière (apport de cette lentille)

Hypothèses : un catalogue complet des boules admises, et un ordre K fixé. Soit B une boule de niveau R.

1. **B1 — Connexité du bloc fermé.** Si $p+u\geq K$, tous les K-sous-ensembles de $I\cup U$ sont dans une seule composante à la coupe R fermée. En effet, tout échange d'un site reste dans B, donc sa coface est de niveau au plus R.
2. **B2 — Facettes strictes.** Une facette $\sigma\subseteq I\cup U$ est strictement antérieure si et seulement si $c\notin\mathrm{conv}(\sigma\cap U)$ (lemme de séparation de B).
3. **B3 — Unicité de la boule.** Tout simplexe de niveau R appartient à une **unique** boule de niveau R, par unicité de la miniboule. Les boules d'un même niveau n'interagissent donc que par les racines pré-lot : le lot est l'union-find de ces racines.
4. **B4 — Effet d'un bloc.** Le bloc fusionne les racines pré-lot des K-facettes strictes de $I\cup U$. S'il n'y en a aucune, c'est une naissance qui couvre $I\cup U$ (d'où les naissances de plus de K points). Les couvertures s'unissent.
5. **B5 — Exclusion.** Les boules non admises sont inertes (Th. 4.2).
6. **B6 — Représentants réduits.** Les représentants « I + t-sous-ensemble strict de U » sont exacts. Une facette stricte qui omet un intérieur i se raccorde par la coface stricte σ∪{i}. Tout chemin se projette sur les t-sous-ensembles de $T_j=Q_j\cap U$, qui sont stricts et de taille au moins t+1.
7. **B7 — Terminaison du résolveur.** Si la boule d'une facette τ n'est pas ancrée à l'ordre K, alors $p_\tau\geq K+2-q_{\min}\geq\lvert\tau\cap I\rvert+2$, donc au moins deux intrus stricts existent. L'échange intrus–support fait décroître $(R^2,\lvert\tau\cap U\rvert)$ dans l'ordre lexicographique, jusqu'à une facette dont la boule est ancrée.
8. **B8 — Verticales.** Il n'y a pas de naissance à $K=p+q_{\min}-1$, car I plus $q_{\min}-1$ sites de coquille forment une facette stricte. Donc $K-1\geq lo$ et l'ancre de la même boule à K−1 existe. La naturalité découle de l'inclusion $L_K\subseteq L_{K-1}$.

**Contrôle exact** (`hgp_block_oracle.py`). Il porte sur 33 nuages dégénérés (grilles entières 0..2 ou 0..3, n = 6–7, carré, triangle rectangle), avec K ≤ 4. On y compare 1 408 événements (K, niveau) à Γ_K pris par définition :
- aucune divergence de fusions, de naissances ni de couvertures ;
- 1 895 blocs admis, dont 732 à coquille étendue ;
- 546 blocs inertes conformes au Th. 4.2 ;
- 837 naissances, dont 6 de plus de K points, et une continuation qui gagne un point.

Deux mutants sont tués : « p+u au lieu de p+q_min » et « fenêtre K+1 ». C'est un oracle borné, pas une preuve à l'échelle.

## 8. Recommandations

1. Enregistrer B1–B8 au registre, statut à décider après relecture, avec l'oracle et les fixtures déjà gravées : E5, square, ABCZ, inert_ball, shell7_window, portal_equal, actual_equal_radius_descent.
2. Corriger `README.md:23` : l'objet est **prouvé** sous régularité, et **testé** plus esquissé ici dans le domaine étendu. Aligner la ligne 20 de l'héritage.
3. Mettre en œuvre la séparation des statuts acceptée (`PLAN_V9.md:109–110`, `COORDINATION_MORSEHGP3D_V9.md:131`), ou la retirer par écrit. En l'état, seul `complete_relative` existe.
4. Ajouter au validateur G4 des invariants globaux d'objet, gratuits et toujours vrais :
   - `births+merges=nodes` ;
   - `parents=nodes−1`, soit une racine finale unique pour n > K ;
   - `births(K1)=sites` ;
   - racine finale couvrant X.

   Les 830 enregistrements d'ordre de R1 à R7b les satisfont déjà. Ils détectent une partie des omissions de catalogue à l'échelle, que la vérification des seules clés présentes ne voit pas.
5. Construire un oracle différentiel indépendant à 2 000–8 000 points : diagrammes $H_0$ par K, calculés par mosaïques de Delaunay d'ordre k ou par rhomboïdes (Edelsbrunner–Osang), **hors chemin produit**. Il sonderait la complétude du générateur au-delà de T2 (n ≤ 14). La faisabilité en 3D exacte est **supposée**, à vérifier.

# Catalogue exact des paires diamétrales de rang $K+1$

Contexte de phase : Phase 15, porte d'entrée satisfaite, `backend=reference_cpu`, `profile=hgp_reduced`, `mode=budgeted`, `deployment_status=architecture_only`, `public_status=not_claimed`. Les deux composants livrés portent des modes bornés `bounded_exact_yao48_rank_cutoff_oracle` et `bounded_yao48_candidate_exact_rank_catalog_oracle`. Cette note n'ouvre ni ne ferme la phase et ne qualifie pas encore le SLO.

Ce catalogue est le premier objet scientifique et logiciel à stabiliser. Il doit énumérer toutes les paires non ordonnées dont la boule diamétrale fermée contient exactement $K+1$ points, puis restituer la liste complète de ces points. Il ne matérialise ni matrice globale de distances, ni cellules, cofaces ou incidences de Delaunay d'ordre supérieur.

## 1. Contrat exact

Pour deux points canoniques distincts $u,v$ et tout point $x$, le prédicat diamétral est

$$\Phi_{u,v}(x)=(x-u)\mathbin{\cdot}(x-v).$$

Le point $x$ appartient à la boule diamétrale fermée de $u,v$ si et seulement si $\Phi_{u,v}(x)\leq0$. On note

$$C(u,v)=\left\lbrace x\in X:\Phi_{u,v}(x)\leq0\right\rbrace,\qquad r(u,v)=\lvert C(u,v)\rvert,\qquad \beta(u,v)=\frac{\left\Vert u-v\right\Vert^{2}}{4}.$$

Les deux extrémités appartiennent toujours à $C(u,v)$. Pour un ordre $K$ fixé, la sortie demandée est exactement

$$P^{=}_{K+1}=\left\lbrace (u,v,C(u,v),\beta(u,v)):u<v,\ r(u,v)=K+1\right\rbrace.$$

Chaque record contient les identifiants canoniques `u`, `v`, le rang fermé exact, le niveau exact, tous les identifiants strictement intérieurs et tout le shell fermé, extrémités comprises. Une exécution multi-ordre peut router une paire vers son unique bucket de rang, mais elle ne relance pas la recherche complète pour chaque ordre.

La proposition flottante, la décision certifiée, la réduction hiérarchique et le statut public restent séparés. Morton, Yao48, un rang k-NN ou l'ordre de parcours ne remplacent jamais le signe exact de $\Phi$.

## 2. Coupe Yao48 adaptative exacte

Fixons une ancre $p$ et une chambre semi-ouverte de Yao48. Après permutation signée des axes, ses vecteurs ont les coordonnées $x\geq y\geq z\geq0$. Soient $K$ témoins non nuls, aux `PointId` distincts de $p$ et entre eux, choisis quelconquement dans cette chambre, et $D$ une borne supérieure certifiée de leurs distances carrées à $p$. Les choisir comme les $K$ plus proches de cette même chambre donne la plus petite telle borne $D_K$, mais cette optimalité n'est pas nécessaire à l'exhaustivité. Une chambre où le producteur n'a pas certifié $K$ témoins n'admet aucun cutoff et reste fail-open.

### Théorème de coupe directionnelle

Pour une cible $q$ telle que $q-p$ ait les coordonnées canoniques $(x,y,z)$ après la même permutation signée, les trois inégalités

$$x^{2}\geq D,\qquad (x+y)^{2}\geq2D,\qquad (x+y+z)^{2}\geq3D$$

certifient que les $K$ témoins retenus appartiennent tous à $C(p,q)$. Ils sont distincts de $p$ et de $q$; par conséquent $r(p,q)\geq K+2$ et la paire ne peut pas appartenir au bucket de rang $K+1$.

**Preuve.** La chambre canonique est engendrée par $e_1=(1,0,0)$, $e_2=(1,1,0)$ et $e_3=(1,1,1)$. Les trois inégalités, dont les deux membres sont positifs, donnent $(q-p)\mathbin{\cdot}e_i\geq\sqrt{D}\left\Vert e_i\right\Vert$ pour $i=1,2,3$. Si $w-p=(a,b,c)$ avec $a\geq b\geq c\geq0$, alors $w-p=(a-b)e_1+(b-c)e_2+ce_3$ avec des coefficients non négatifs et $\left\Vert w-p\right\Vert^{2}\leq D$. La linéarité puis l'inégalité triangulaire donnent $(q-p)\mathbin{\cdot}(w-p)\geq\sqrt{D}\left\Vert w-p\right\Vert\geq\left\Vert w-p\right\Vert^{2}$, donc $\Phi_{p,q}(w)\leq0$. Pour la distinction des témoins, la première condition donne $x^2\geq D$; si l'inégalité est stricte, le résultat est immédiat, et si $x^2=D$, la seconde condition force $y>0$, donc $\left\Vert q-p\right\Vert^2=x^2+y^2+z^2>D$. Ainsi $q$ ne fait pas partie des $K$ témoins. Fin de la preuve.

Si les trois comparaisons sont strictes, la même décomposition donne $\Phi_{p,q}(w)<0$ pour tout témoin non nul. Cette variante certifie $K$ intérieurs stricts et peut fermer la lane `RelevantGP`; la variante fermée ne ferme que le rang fermé. Les égalités doivent donc descendre dans le mode produit fail-closed, même si elles peuvent rejeter une paire du seul catalogue fermé.

La conséquence radiale plus simple est

$$\left\Vert q-p\right\Vert^{2}\geq3D\quad\Longrightarrow\quad r(p,q)\geq K+2.$$

Le facteur $3$ est optimal si l'on ne conserve que $D$ et l'identité de la chambre : les rayons extrêmes $(1,0,0)$ et $(1,1,1)$ ont un cosinus égal à $1/\sqrt{3}$. La coupe directionnelle est strictement plus forte ailleurs. Aucune racine carrée ni fonction trigonométrique n'est nécessaire dans le noyau : les trois comparaisons carrées sont exactes.

### Exhaustivité du sur-ensemble

Une paire n'est supprimée que si l'orientation effectivement visitée possède ce certificat. Toute paire de rang au plus $K+1$ survit donc à n'importe quelle politique d'ownership exacte une fois; intersecter les survivants des deux orientations est une optimisation supplémentaire, pas une prémisse de complétude. Dans une chambre non certifiée, tous les points survivent. Une fenêtre Morton fixe peut fournir les témoins sans aucun contrat de rappel : chaque témoin retenu est recertifié, et un manque désactive seulement le cutoff. Ce fait ne doit pas être confondu avec une fenêtre utilisée comme autorité d'exclusion.

L'oracle borné [`yao48_ranked_pair_candidates.cpp`](../../morsehgp3d/src/cpu/hierarchy/yao48_ranked_pair_candidates.cpp) construit ce sur-ensemble pour $n\leq512$. Son test différentiel recalcule le rang fermé de chaque paire pour tous les ordres $1\leq K\leq10$, vérifie les égalités de shell, les chambres sous-pleines et un cas où la coupe directionnelle est strictement meilleure que le rayon uniforme.

## 3. Classification terminale exacte

Yao48 exclut des paires impossibles; il ne décide pas le bucket exact. Chaque paire survivante est classifiée par le prédicat exact $\Phi$ sur le LBVH global. Pendant le parcours, les bornes sont

$$L=2+N_{\mathrm{closed}},\qquad U=n-N_{\mathrm{outside}},\qquad L\leq r(u,v)\leq U.$$

Le terminal rejette `above` dès que $L>K+1$, rejette `below` dès que $U<K+1$ et émet `exact` uniquement après $L=U=K+1$. Pour un record exact, l'union de `support_ids`, `strict_interior_ids` et `extra_shell_ids` reconstitue tout $C(u,v)$ sans nouvelle requête.

L'oracle bout en bout [`exact_ranked_diametral_pair_catalog_reference.cpp`](../../morsehgp3d/src/cpu/hierarchy/exact_ranked_diametral_pair_catalog_reference.cpp) compose la coupe Yao48 et le classifieur exact pour $n\leq512$. Son test compare tous les buckets $K+1$ à un scan quadratique indépendant de toutes les paires et de tous les points. Son validateur local ferme seulement la structure, l'appartenance des records au sur-ensemble et la comptabilité; la reconstruction fraîche ou ce différentiel indépendant authentifie la géométrie. Ce plafond en fait un oracle de falsification, jamais un fallback produit. Sa bibliothèque `*_reference` est séparée du cœur `morsehgp3d::hierarchy`.

Ces types restent une API `architecture_only` sans stabilité binaire promise : tout consommateur C++ d'une archive antérieure doit être recompilé. Le futur wire autoritatif aura sa propre version et ne sérialisera jamais la disposition mémoire des structures C++.

Le futur producteur GPU devra porter une autorité terminale qui ferme simultanément :

1. la masse du sur-ensemble Yao et des rejets certifiés;
2. la partition `below + exact + above` de tous les candidats;
3. la liste complète des points de chaque record exact;
4. le compte de toutes les paires non ordonnées, égal à $n(n-1)/2$;
5. l'identité du nuage, du LBVH, du rang demandé et du prédicat exact.

Ces cinq engagements ne sont pas encore sérialisés par l'oracle borné. Dans le futur producteur, une sortie partielle, une frontière résiduelle ou un budget dépassé devra recevoir `budget_exhausted`; elle ne sera jamais publiée comme exhaustive.

## 4. Architecture GPU et rôle de Morton

Le chemin produit visé traite des tuiles d'ancres et partage un LBVH Morton résident :

1. une fenêtre Morton symétrique propose des témoins, puis chambre, identité et borne de distance sont certifiées; une recherche LBVH peut resserrer les rayons sans être obligatoire;
2. les 48 banques restent en mémoire bornée, soit $B\times48\times K$ identifiants pour une tuile de $B$ ancres;
3. un parcours de régions rapporte seulement les cibles qui ne sont pas exclues par les trois bornes directionnelles;
4. l'ordre `(clé Morton, PointId)` oriente les feuilles pour émettre chaque paire une seule fois;
5. le classifieur exact route les survivants vers `below`, `exact` ou `above`;
6. un couple `count + exclusive scan` réserve les records et leurs listes de points avant l'écriture compacte;
7. les ambiguïtés binary64 passent à l'exact dyadique, puis à une file multiprécision rare si nécessaire.

Pour les positions Morton $i<j$, l'extrémité de position haute $j$ possède la paire et ne traverse que le préfixe $[0,j)$. Le postordre inverse `root-right-left` existant visite alors d'abord $j-1,j-2,\ldots$ et saute en bloc tout nœud dont `leaf_begin>=j`. Cette orientation couvre exactement $\sum_{j=0}^{n-1}j=n(n-1)/2$ feuilles terminales avant prune. L'orientation inverse reste un filtre optionnel; elle n'est jamais exigée pour l'exhaustivité.

La résidence GPU est une obligation, pas une optimisation facultative. Construction Morton/LBVH, banques certifiées des 48 chambres, prunes de régions, émission exacte une fois, classification filtrée, `count + scan`, écriture des payloads, tri et déduplication restent sur le device. Le premier niveau exact emploie des intervalles binary64 dirigés, le deuxième des expansions ou entiers de taille fixe sur GPU; seuls les cas qui dépassent cette enveloppe rejoignent une file compacte de limbs variables. Aucun callback hôte par paire, aucune copie D2H par vague et aucune liste intermédiaire de candidats sur l'hôte ne sont admis. Le retour normal est un transcript terminal et des chunks de sortie déjà compactés.

### Une seule passe pour tous les ordres

Pour $K_{\max}$, le produit ne relance pas le pipeline pour chaque $k$. Il construit une seule fois les banques de témoins et leurs bornes. La coupe correspondante conserve simultanément toutes les paires de rang fermé $R\leq K_{\max}+1$. Le classifieur terminal retourne alors soit `above_window`, soit le rang exact $R$ et tout $C(u,v)$; le record est rangé dans son unique bucket $R$. Un sous-simplexe Gabriel porté de cardinal $q$ alimente l'ordre $q-1$; seulement sous `RelevantGP`, où le shell supplémentaire utile est vide, $q=R$ et ce routage devient $k=R-1$. Les oracles CPU actuels bouclent encore sur les buckets pour rester simples et différentiels; cette répétition est interdite dans le chemin GPU massif.

Le sens de parcours Morton est un choix de localité et d'ordonnancement : visiter d'abord les intervalles proches fournit vite les seuils $D_K$ et compacte les warps. Il n'est jamais une règle d'arrêt. Une fenêtre linéaire bornée autour d'une clé Morton peut manquer une paire admissible; seuls les tests géométriques certifiés sur les boîtes du LBVH ferment une région.

Pour un nœud entièrement contenu dans une chambre, les minima de $x$, $x+y$ et $x+y+z$ sur sa boîte donnent directement le même prune directionnel pour toutes ses feuilles. Un bloc de $K$ témoins peut aussi certifier un nœud en une fois. Ces deux prunes sont adaptés au SIMT : additions, carrés, comparaisons et masques, sans divergence trigonométrique.

Cette architecture ne conserve ni table $n\times48\times K$ pour tout le nuage, ni matrice de paires, ni mosaïque de Delaunay d'ordre supérieur. Les tuiles, frontières et chunks de sortie sont consommables en flux. Les clés Morton servent à la disposition, au parcours et à l'unicité; Yao48 sert à la preuve locale de rang; le classifieur exact reste l'autorité terminale.

## 5. Ce que chaque record de paire donne immédiatement

Soit un record exact $(u,v,S)$ avec $S=C(u,v)$ et $\lvert S\rvert=R$. Pour tout ensemble $Q$ tel que $\left\lbrace u,v\right\rbrace\subseteq Q\subseteq S$, on a

$$\mathrm{Miniball}(Q)=B_{u,v},\qquad X\cap\mathrm{Miniball}(Q)=S.$$

En effet, toute boule contenant $u$ et $v$ a un rayon au moins égal à $\left\Vert u-v\right\Vert/2$, tandis que $B_{u,v}$ contient déjà $Q$ avec ce rayon. Le niveau, le rang et la saturation de $Q$ sont donc ceux du record source.

Écrivons $I=X\cap B_{u,v}^{\circ}$ et $E=(X\cap\partial B_{u,v})\setminus\lbrace u,v\rbrace$. La fermeture de miniboule ne suffit pas à conclure que chaque $Q$ est Gabriel. Le critère exact est

$$\left\lbrace u,v\right\rbrace\subseteq Q\subseteq\left\lbrace u,v\right\rbrace\cup I\cup E\quad\Longrightarrow\quad\bigl[Q\text{ est Gabriel}\Longleftrightarrow I\subseteq Q\bigr].$$

En effet, omettre un point de $I$ laisse un point de $X\setminus Q$ dans l'intérieur ouvert de la miniboule; réciproquement, si $I\subseteq Q$, tout point omis de $E$ est seulement sur le shell et ne viole pas la condition de Gabriel. Pour une cardinalité cible $q$, le nombre exact de simplexes abstraits de Gabriel portés par ce record est donc $\binom{\lvert E\rvert}{q-2-\lvert I\rvert}$, avec la convention que ce coefficient vaut zéro lorsque $q-2-\lvert I\rvert\notin\left\lbrack0,\lvert E\rvert\right\rbrack$.

Un record énumère par ailleurs exactement $R-2$ triplets candidats et $\binom{R-2}{2}$ quadruplets candidats de même miniboule, sans nouveau parcours global. Ces deux comptes incluent les ensembles non Gabriel qui omettent un intérieur strict. Un filtre exact d'indépendance affine peut ensuite produire un catalogue optionnel de triangles et tétraèdres géométriques; il ne doit jamais supprimer un simplexe abstrait de Gabriel du flux HGP. Pour un triplet non dégénéré, un troisième sommet strictement intérieur donne un triangle obtus et un sommet supplémentaire du shell donne un triangle rectangle. Réciproquement, tout triangle non aigu dont le rang $R$ appartient aux buckets effectivement émis est retrouvé par son plus long côté. À $R=11$, le record ne propose que 9 triplets et 36 quadruplets de cette branche, au lieu de toutes les cliques du saturé.

Sous `RelevantGP`, un simplexe utile de Gabriel à support propre ne peut omettre ni intérieur strict, par le critère précédent, ni point du shell, par définition de `RelevantGP`; il est donc l'unique saturé $Q=S$ et vérifie $\lvert Q\rvert=R$. Sans cette hypothèse, un catalogue limité aux seules boules de rang fermé $R\leq s_{\max}$ n'est pas exhaustif pour les simplexes de cardinalité au plus $s_{\max}$ : une paire antipodale peut être Gabriel tout en portant un shell cosphérique arbitrairement grand. La fixture [`relevant_gp_extra_shell_above_smax.json`](../../morsehgp3d/tests/fixtures/spatial/relevant_gp_extra_shell_above_smax.json) impose précisément le diagnostic `unsupported_degeneracy` au lieu d'une fausse complétude.

L'implémentation GPU naturelle effectue un `count` Gabriel par la formule précédente, un scan exclusif, puis un warp par record; l'inclusion de $I$ est constructive et ne demande aucun nouveau test géométrique. Le catalogue géométrique optionnel possède son propre `count + scan` sur tous les candidats de même miniboule, suivi du filtre exact d'indépendance affine. Pour un support minimal fixé, une déduplication radix exacte sur les tuples canoniques suffit. Si plusieurs supports minimaux décrivent la même boule, leurs familles sont réunies puis dédupliquées comme ensembles de `PointId`; le chemin `RelevantGP` courant diagnostique cette cosphéricité utile comme `unsupported_degeneracy` au lieu de choisir un support. Si le catalogue de paires est déjà complet, reclassifier toutes les paires internes est redondant : une jointure triée avec les clés du catalogue décide leur présence.

## 6. Limite exacte : les triangles aigus ont leur propre frontière

Dans les buckets de rang effectivement émis, la branche précédente est exhaustive exactement pour les miniboules dont un support minimal possède deux points. Les paires de rang hors fenêtre, notamment les grandes cosphères, restent hors de ce catalogue et suivent leurs diagnostics bornés. Même dans la fenêtre, cette branche n'est pas exhaustive pour les supports propres de taille trois.

La fixture rationnelle [`hartigan_triangle_all_side_ranks_above_k.json`](../../tests/fixtures/regressions/hartigan_triangle_all_side_ranks_above_k.json) contient un triangle aigu $ABC$ de rang fermé trois qui produit une fusion exacte de $\Gamma_2$. Les 36 paires du nuage y sont recertifiées : 18 ont rang au plus trois, mais l'union de toutes les sous-arêtes de leurs saturés ne contient aucune des arêtes $AB$, $AC$ ou $BC$. Ni une paire porteuse, ni un arbre, ni une clique formée depuis ces sous-arêtes ne retrouve donc le triangle critique.

Le test du caractère aigu reste constant et très bon marché : le troisième sommet doit être strictement extérieur aux trois boules diamétrales de côtés, soit trois signes exacts de produits scalaires. Cette vérification filtre un candidat; elle ne le génère pas. Après stabilisation du catalogue de paires, le jalon suivant est donc une frontière exhaustive indépendante des triangles aigus, avec la fermeture issue des paires utilisée seulement comme voie rapide et cache de propositions. Les tétraèdres bien centrés viennent ensuite.

Cette stratification est la réduction correcte en dimension trois : supports minimaux bien centrés de tailles deux, trois et quatre, miniboule exacte, saturation fermée globale, puis agrégation. Elle évite de reconstruire la mosaïque de Delaunay d'ordre supérieur tout en conservant une autorité exhaustive par taille de support.

### Cascade GPU exacte des supports minimaux

Le théorème général de la plus petite boule englobante donne un support minimal d'au plus quatre points en dimension trois, avec son centre dans l'enveloppe convexe du support. Il impose la cascade suivante :

| Taille du support minimal | Candidat indépendant | Test exact constant | Cas éliminé de cette frontière |
|---:|---|---|---|
| 2 | paire diamétrale | prédicat $\Phi$ et rang fermé | aucun; c'est la première frontière |
| 3 | triangle affinement indépendant | trois signes stricts $\Phi_{a,b}(c)>0$, $\Phi_{a,c}(b)>0$, $\Phi_{b,c}(a)>0$ | triangle droit, obtus ou dégénéré, déjà porté par une paire |
| 4 | tétraèdre affinement indépendant | poids barycentriques strictement positifs du centre circonscrit | centre sur la frontière ou hors de l'enveloppe convexe du tétraèdre, donc miniboule déjà portée par une paire ou un triangle |

Une fois un record de triangle aigu $(a,b,c,S)$ connu, tout quadruplet $\left\lbrace a,b,c,x\right\rbrace$ avec $x\in S$ conserve sa miniboule, son rang et son saturé; un filtre d'indépendance affine garde les tétraèdres. Les tétraèdres ainsi portés par une paire ou un triangle ne doivent donc jamais entrer dans la frontière indépendante de support quatre. Celle-ci ne cherche que les tétraèdres bien centrés. Les égalités barycentriques sont envoyées vers le support inférieur. Une extension dégénérée devra agréger tous les supports d'une même cosphère avant de dédupliquer les simplexes engendrés; une clé de boule ne permet jamais d'en supprimer silencieusement les sources.

Sous `RelevantGP`, pour tous les niveaux $1\leq k\leq K_{\max}$, le rang cible d'un simplexe porté est $R=k+1$. Hors de cette hypothèse, son ordre dépend de sa cardinalité $q$, pas du rang fermé de la cosphère. À $k=1$, seules les paires sont possibles; à $k=2$, les supports de tailles deux et trois suffisent; à partir de $k=3$, les trois tailles de support peuvent intervenir. Chaque frontière classe le rang exact dans son bucket en une passe multi-ordre. Le catalogue de paires fournit immédiatement les branches non aiguës dans le domaine certifié; le catalogue de triangles fournira ensuite les tétraèdres de support trois; seule la génération exhaustive des supports propres aigus puis bien centrés reste un problème de frontière.

Un oracle GPU dense et borné devra énumérer les univers complets de paires, triplets puis quadruplets pour falsifier cette cascade sur petits nuages. Il restera explicitement un oracle : le chemin produit n'a pas le droit de matérialiser ces univers sous un autre nom. Sur le produit, les tests constants ci-dessus s'appliquent avant toute classification de rang et les frontières géométriques doivent être output-sensitive.

## 7. Coût de sortie et SLO

Le pire cas reste quadratique dès les paires. En dimension trois, le graphe de Gabriel, c'est-à-dire le bucket de rang deux en position générale, peut avoir $\Omega(n^2)$ arêtes; [Chazelle et al., lemme 5.1](https://www.cs.princeton.edu/~chazelle/pubs/SelectHeavyCoveredPts.pdf) donnent une construction explicite. À 50 000 points, les univers bruts contiennent 1 249 975 000 paires, 20 832 083 350 000 triplets et 260 385 417 812 487 500 quadruplets. Huit octets par paire demanderaient déjà près de 10 Go avant les listes de points. Aucun algorithme ne peut énumérer une telle sortie en moins de 100 ms; a fortiori, aucun scan dense des triplets ou quadruplets n'est admissible dans le produit.

Sous un modèle de Poisson homogène sans frontière, la seule coupe radiale uniforme laisse grossièrement $48\times3\sqrt{3}\,K\simeq249{,}4K$ candidats dirigés par point en espérance. Cette estimation n'est ni une garantie ni un objectif; elle motive les trois projections, les boîtes du LBVH et les témoins en blocs. Un filtre à l'orientation inverse ne sera ajouté que si son gain mesuré dépasse son coût; l'ownership Morton haute suffit déjà à la couverture exacte une fois.

Le SLO produit doit être output-sensitive et annoncer des caps sur le nombre de records, les références de points, les candidats classifiés, le travail de frontière et les octets de sortie. Pour $n=50\,000$, $K=10$, le benchmark publie p50 et p95 sur un nouveau nuage à chaque répétition, avec temps séparés pour Morton/LBVH, amorçage des banques Yao48, rapport de candidats, classification exacte, compactage et persistance. Le chemin à dizaines de millions de points utilise les mêmes certificats par chunks; il ne promet pas que la sortie elle-même tient en mémoire.

## 8. Gates immédiats

1. conserver l'oracle bout en bout exact et ses tests différentiels comme autorité bornée;
2. porter les banques de $K$ témoins certifiés quelconques par chambre et le rapport directionnel sur le LBVH GPU partagé, puis ne resserrer vers le top-$K$ que si le profil le justifie;
3. garder classification, compactage, tri et payload sur GPU, avec seulement une file rare multiprécision et un transcript terminal vers l'hôte;
4. certifier les prunes de boîtes, les témoins distincts, les chambres sous-pleines et la masse exacte des paires;
5. mesurer le taux de survivants après chaque prune avant toute promesse de 100 ms;
6. publier le catalogue multi-ordre seulement avec listes fermées complètes et frontière vide;
7. ouvrir ensuite la frontière indépendante des triangles aigus, éliminer immédiatement droits, obtus et dégénérés, puis ouvrir les seuls tétraèdres bien centrés après fermeture des triangles.

Le catalogue de paires est une primitive exacte prioritaire. Il ne publie aucune hiérarchie Hartigan à lui seul : les clusters discrets restent les composantes du graphe de facettes $\Gamma_K$, avec recouvrements de points possibles dès $K\geq2$.

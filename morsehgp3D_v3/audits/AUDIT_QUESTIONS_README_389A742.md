# Audit des questions du README et de la proposition — snapshot `389a742`

> [!IMPORTANT]
> Snapshot audité : commit `389a7428c88d9dede7a9c767634774b9ea842ca0`; `README.md` SHA-256 `6611f773e98f9985f84a8ca2c85c27ac2999656d76d18ab3a49d019acd3634f4`; `PROPOSITION.md` SHA-256 `615935ad798ce5afb3eb3280a54a3bfd8306eed9d7570ff474866c7a3255d912`. Cet audit répond aux questions explicites du README et vérifie les claims nouveaux contre le code du même snapshot. Il ne qualifie aucun statut public.

## Verdict immédiat

| sujet | réponse |
| --- | --- |
| contrat 50 k, $K=10$, moins d'une seconde | **Le chemin courant est NO-GO.** La cible de recherche n'est pas réfutée sur des familles sanctionnées et sparse, mais aucun ledger ne permet aujourd'hui de la dire « plausible ». |
| Q0, arrêt local | **Le passage à une marge stricte répare le lemme.** Deux phrases du README restent à corriger : la définition de $M(p)$ réemploie la forme fermée, et `RelevantGP` n'exclut pas toute cosphéricité non critique. |
| Q1, onion peeling | **La réponse négative est mathématiquement vraie, mais ni l'exemple du README ni la nouvelle fixture CTest ne la prouvent.** Une fixture correcte est déjà donnée dans `REPONSE_README_PREFIXE_SHALLOW.md` §2.4. |
| Q2, constructeur shallow | Voie crédible : construction incrémentale Las Vegas du sous-complexe de demi-plans de profondeur au plus $\kappa$, avec conflits et clipping exact. Le sweep actuel reste $O(m^2\log m)$ et n'est pas cette solution. |
| Q2, tri dans `i128` | **Oui sur le profil u16 équilibré.** Le chirotope $3\times3$ évite les produits croisés de 210 bits. Les positions égales doivent cependant être traitées par lot atomique, ce que le code ne fait pas encore. |
| Q3, rejet brut `O(1)` | **Non au pire cas sans prétraitement.** Seuls des filtres suffisants fail-open sont possibles en temps constant. |
| PEL-1, source des plans porteurs | **Peut être fermé sous `RelevantGP`.** Toute paire d'un support utile porte une 2-face incidente de profondeur égale au nombre de points strictement intérieurs. Une profondeur uniforme $s_{\max}-2$ suffit à voir tous ces plans. |

## 1. Erreur bloquante : Q1 est encore publiée avec deux preuves invalides

### 1.1 Le carré dual du README ne possède pas les sommets invoqués

Le README prend les points duaux $(1,0,1)$, $(0,1,1)$, $(-1,0,1)$ et $(0,-1,1)$, puis appelle les paires opposées des « diagonales » donnant des sommets de profondeur 1. Avec sa propre convention, $(a,b,c)$ représente la droite $a s_1+b s_2=c$.

Les deux paires invoquées représentent donc $s_1=1$ avec $s_1=-1$, puis $s_2=1$ avec $s_2=-1$. Elles sont parallèles. De manière équivalente, leurs déterminants $D_{02}$ et $D_{13}$ sont nuls. Elles se rencontrent à l'infini, jamais en un centre fini de l'arrangement. Les mots « contre-exemple minimal » et « vérifié exactement » doivent être retirés de cet exemple.

### 1.2 La fixture `convex_layer_refutation` n'est pas bien centrée

La nouvelle fixture utilise notamment

- $p=(10,10,10)$ et $q=(10,10,20)$;
- $x_1=(6,13,16)$ et $x_3=(13,6,16)$.

Le centre du support $(p,q,x_1,x_3)$ vaut $(19/2,19/2,15)$, mais ses coordonnées barycentriques exactes, dans cet ordre, sont

$$\left(\frac{1}{10},-\frac{1}{10},\frac{1}{2},\frac{1}{2}\right).$$

Le coefficient de $q$ est négatif. Le support est donc `well_centred=false` et n'est pas un événement critique Morse. Déplacer seulement le sixième point de $(13,14,16)$ à $(13,14,17)$ ne peut pas changer ce fait.

Les commentaires de la fixture contiennent aussi une erreur arithmétique : l'intersection des formes de $x_1$ et $x_3$ est $s=(-1/5,1/5)$, pas $(-61/20,61/20)$. Après le déplacement du sixième point, les résidus des quatre formes au vrai sommet valent $(0,24,0,-44)$.

Le CTest ne vérifie par ailleurs ni l'identité du support cible, ni son bon centrage, ni sa profondeur, ni son absence de l'onion. Il peut être vert lorsque l'oracle et le sujet omettent tous deux le support non bien centré. C'est une régression vacue relativement au claim Q1.

### 1.3 La bonne fixture existe déjà dans les audits

La section 2.4 de [`REPONSE_README_PREFIXE_SHALLOW.md`](REPONSE_README_PREFIXE_SHALLOW.md) donne la configuration u16 suivante :

- $p=(10,10,1)$, $q=(10,10,9)$, $z=(13,13,5)$ et $w=(13,7,5)$;
- $u=(14,9,6)$ et $v=(11,6,6)$.

Ses quatre formes duales coplanaires sont $(48,-48,8)$, $(-48,-48,8)$, $(-16,-64,8)$ et $(-64,-16,8)$. La paire $(z,w)$ est une diagonale de la première couche, mais ses droites se coupent au point fini $s=(0,-1/6)$ avec profondeur stricte 1. Le centre a pour barycentriques $(4/9,4/9,1/18,1/18)$ dans $(p,q,z,w)$ : elles sont toutes positives. Le shell est exact, le rang fermé vaut 5 et l'ancre $(p,q)$ est l'unique diamètre maximal.

Cette fixture, et elle seule parmi les deux candidates actuellement publiées, ferme Q1 au niveau Morse. Le test permanent doit affirmer explicitement toutes ces propriétés avant de se nommer `convex_layer_refutation`.

## 2. Q0 : le lemme strict est bon, mais son contrat documentaire ne l'est pas encore

La nouvelle hypothèse est la bonne :

$$\rho_M=\sup_{c\in V^{(M)}}\lVert c-p\rVert<\frac{d_{M+1}}{2}.$$

Pour tout point non traité $u$ et tout $c\in V^{(M)}$, l'inégalité triangulaire donne alors $\lVert c-u\rVert>\lVert c-p\rVert$. Tous les nouveaux plans ont donc un signe strictement extérieur sur toute la région. Ils n'ajoutent ni profondeur, ni shell, ni strate dans cette région. L'inclusion réciproque suit et le certificat est valide, sous réserve que le supremum soit effectivement certifié.

Deux corrections restent nécessaires dans le README :

1. la définition de $M(p)$ réemploie ensuite $V^{(M)}\subseteq B(p,d_{M+1}/2)$, la forme fermée précisément réfutée. Elle doit reprendre littéralement $\rho_M<d_{M+1}/2$;
2. l'affirmation « cinq points cosphériques, donc exclus du domaine » est trop forte. Le dépôt accepte explicitement certaines cosphéricités portées seulement par des supports non bien centrés. `RelevantGP` n'est pas l'absence globale de cinq points cosphériques. Cette observation peut être gardée sous une hypothèse de position générale plus forte, mais elle ne doit pas servir de garde de correction.

Le test produit doit décider strictement $4\rho_{M,\mathrm{hi}}^2<d_{M+1,\mathrm{lo}}^2$. Égalité, overflow ou intervalles se recouvrant donnent `inconclusive` et poursuivent l'insertion. Le lemme fournit une règle correcte; il ne fournit aucune garantie que cette règle se déclenche tôt.

## 3. Résultat utile : PEL-1 peut être fermé sous `RelevantGP`

### 3.1 Énoncé

Soit $U$ un support minimal bien centré de cardinalité $q\in\lbrace2,3,4\rbrace$, contenant l'ancre $p$, de circumboule de rang fermé $r=q+d\leq s_{\max}$, où $d$ est le nombre de points strictement intérieurs. Pour tout $u\in U\setminus\lbrace p\rbrace$, le plan médiateur $H_u$ porte une 2-face relativement ouverte de l'arrangement ancré en $p$, incidente au centre de $U$, dont la profondeur est exactement $d$.

Par conséquent, tout plan d'une paire utile apparaît parmi les plans porteurs des 2-faces de profondeur au plus $s_{\max}-q$, donc certainement parmi celles de profondeur au plus $s_{\max}-2$. À $s_{\max}=11$, le seuil uniforme nécessaire pour la source de plans est 9, pas 10.

### 3.2 Preuve

Fixons $u\in U\setminus\lbrace p\rbrace$ et le centre $c$. Dans le plan tangent $T=H_u-c$, les autres points de shell $U\setminus\lbrace p,u\rbrace$ donnent $q-2$ formes linéaires nulles en $c$. Il y en a au plus deux. L'indépendance affine du support implique que ces formes restreintes sont non nulles et, lorsqu'elles sont deux, linéairement indépendantes.

Il existe donc une direction $v\in T$ sur laquelle toutes ces formes sont strictement négatives. Pour un $\varepsilon>0$ assez petit et générique, les points strictement intérieurs restent positifs, les points strictement extérieurs restent négatifs, et aucun autre hyperplan ne contient $c+\varepsilon v$. Ce point appartient à une 2-face ouverte de $H_u$ de profondeur exactement $d$. Enfin $d=r-q\leq s_{\max}-q\leq s_{\max}-2$.

Ce lemme ferme l'inclusion demandée par PEL-1 dans le domaine simple de `RelevantGP`. Il ne ferme pas PEL-2 ni PEL-4 : construire toutes ces 2-faces peut rester dense et plus cher que produire directement A2e.

### 3.3 Conséquence architecturale

A2pe possède donc une source complète de paires **si** A2p construit exhaustivement les 2-faces jusqu'à profondeur $s_{\max}-2$, émet une fois leur plan porteur, puis déduplique les paires. Le problème restant est le coût total de cette source, pas son inclusion mathématique.

Pour PEL-3, une face non bornée ne rend pas sa projection infinie : $c_F=\mathrm{proj}_{\mathrm{aff}(F)}(p)$ reste un point fini et unique. Il faut représenter les rayons ou la clôture projective de la face et tester exactement $c_F\in F$. Cela sépare le problème de correction, simple, du problème d'énumération output-sensitive, encore ouvert.

## 4. Q2 : réponse algorithmique et limite exacte du code actuel

### 4.1 Le tri peut rester dans `i128` sur u16

Pour la droite $i$, posons $D_{ij}=a_i b_j-a_j b_i$ et $\lambda_i=(a_i,b_i,-c_i)$. L'ordre de deux croisements le long de $i$ est donné par

$$\mathrm{sgn}(\tau_{ij}-\tau_{ik})=\mathrm{sgn}\!\left(\det(\lambda_i,\lambda_j,\lambda_k)\right)\mathrm{sgn}(D_{ij})\mathrm{sgn}(D_{ik}).$$

Il suffit de combiner trois signes; aucun produit $D_{ij}D_{ik}\det$ ne doit être formé. Avec la base équilibrée et les bornes u16 documentées, le déterminant reste sous $2^{107{,}4}$ et tient dans un `i128` signé. La substitution du comparateur rationnel par `chirotope_order` répond donc positivement à la question de largeur du **tri local u16**.

Elle ne prouve pas que toute l'arithmétique tient dans `i128` : le test de profondeur d'arité trois emploie déjà 256 bits, et les niveaux publics atteignent plus de 128 bits. Le README est contradictoire lorsqu'il dit à la fois « tout tient dans un `i128` » et décrit ces replis.

### 4.2 Les égalités ne sont pas encore correctes

Si plusieurs droites sont concurrentes, leurs croisements ont exactement la même position. La profondeur stricte au sommet doit retirer simultanément toutes les formes nulles, puis appliquer toutes les variations au même instant. Le code courant trie correctement ces positions comme équivalentes, mais les traite encore une par une dans la boucle de sweep. Les profondeurs intermédiaires dépendent alors de l'ordre choisi par `std::sort`.

Le correctif conceptuel est un groupement maximal par `chirotope_order == 0`, un calcul unique de la profondeur stricte du lot, l'examen canonique du shell complet, puis une mise à jour atomique de tous les signes. Sans cela, le claim « le balayage donne tous les sommets avec leur profondeur » reste conditionnel à l'absence de concurrence.

### 4.3 Le constructeur demandé n'est pas encore implémenté

Le sweep par droite paie toujours un tri pour chaque ligne et reste en $O(m^2\log m)$. La voie répondant réellement à Q2 est un constructeur du sous-complexe de demi-plans de profondeur au plus $\kappa$, par incrémentation randomisée Las Vegas, décomposition locale et listes de conflits. La cible théorique est $O(m\log m+m(\kappa+1))$ espéré, mais son adaptation au disque de Jung, aux égalités, aux parallèles, au transcript exact et au déterminisme de sortie reste une obligation.

Le sweep dense doit rester l'oracle local de ce futur constructeur. Il n'est pas le chemin produit recherché.

## 5. Q3 : pas de rejet complet brut en `O(1)`

Sur la corde d'une droite cible, chaque autre demi-plan induit une constante, un préfixe ou un suffixe. L'existence d'un sommet de profondeur au plus $\kappa$ dépend de l'ordre et de l'orientation de tous ces seuils. Une entrée non inspectée peut ajouter 1 à la profondeur sur toute la corde et faire passer le minimum de $\kappa$ à $\kappa+1$. Sans prétraitement, un décideur exact doit donc lire $\Omega(m)$ informations au pire cas.

Les rejets constants sûrs restent seulement suffisants : frontière manquant l'ellipse de Jung, `carrier_eligible=false` par certificat, budget $c_e>s_{\max}-4$, ou borne inférieure de profondeur certifiée sur une cellule. Tous doivent être fail-open, et une forme inéligible comme porteur doit rester dans le flux témoin si elle peut compter à l'intérieur.

## 6. Audit des nouveaux claims de clipping et de mesure

La classification d'une **droite** par rapport à l'ellipse est exacte et utile. Elle ne suffit pas à dire qu'un **sommet de deux droites actives** appartient à l'ellipse : deux cordes peuvent chacune couper un disque et avoir leur intersection hors du disque. Le code ne teste pas encore l'inégalité de Jung au croisement avant de le compter comme `vertices_examined` ou `vertices_shallow`.

Le masque de lentille est une condition nécessaire pour chaque porteur, pas une preuve que $(p,q)$ est une paire diamètre du quadruplet : il manque encore le test $\lVert z-w\rVert^2\leq D^2$. Les supports peuvent donc être développés depuis des ancres non maximales, puis seulement dédupliqués. Cela ne force pas une omission lorsque toutes les paires sont balayées, mais invalide les interprétations « arêtes diamétrales retenues », $Z_e$ produit et coût par propriétaire.

Les tableaux de temps du README n'ont pas le sidecar exigé par `PROPOSITION.md` §0. Ils doivent rester `[diagnostic]`, avec commande, commit, compilateur, machine, entrées, digests, compteurs fermés et sortie brute avant de devenir `[mesuré]`. « Catalogue identique » doit signifier « aucune différence sur cette campagne », jamais une vérification universelle.

Enfin, le ratio « sommets shallow / sphères émises = 2,4 » ne couvre ni les comparaisons de tri, ni les rescans `sphere_side`, ni les doublons inter-ancre, ni l'aval HGP. Il ne démontre pas que le prototype « ne gaspille presque plus ». Le résultat sûr est plus modeste : **le clipping réduit fortement les intersections développées sur les nuages diagnostiqués**.

## 7. Réponse au contrat 50 k

Le chemin courant commence par $\binom{50000}{2}=1\,249\,975\,000$ ancres. Il est donc hors contrat avant même le constructeur shallow, le tri global et le réducteur. La phrase « une seconde sur 48 cœurs paraît plausible » n'est soutenue par aucun ledger et doit être retirée ou marquée comme intuition non décisionnelle.

La seconde peut rester une cible de recherche seulement si les portes suivantes ferment simultanément :

1. source A1 ou A2pe complète dont le travail et la sortie sont effectivement sparse sur chaque famille sanctionnée;
2. constructeur shallow sans terme $\sum_e m_e^2$;
3. compteurs et quantiles de $A$, $M=\sum_e m_e$, $Z=\sum_e Z_e$, prédicats exacts, replis et queues lourdes;
4. quatre arités, shell, bon centrage et propriétaire validés indépendamment;
5. tri exact des niveaux, lots égaux, incidences silencieuses, source HGP, descente et verticales inclus dans le chrono;
6. ledger `warm_e2e`, mémoire maximale et reçus complets sur le matériel cible.

Le constat actuel est donc : **A1 est le premier mur structurel visible, mais il n'est pas démontré que ce soit le seul mur restant**. Le vrai constructeur de niveaux et le volume aval ne sont pas encore mesurés.

## 8. Claims de qualification à retirer avant la prochaine itération

- « Le dictionnaire est vérifié » doit être séparé en un théorème conditionnel et une campagne différentielle finie. Les désaccords de domaine, la queue de support non initialisée et les verdicts de reçu tardifs documentés dans `AUDIT_LIVE_1216D16.md` empêchent une qualification logicielle.
- « Les quatre arités, donc tout le catalogue » contredit encore la section suivante qui dit que seule l'arité quatre est vérifiée. Il faut annoncer les portes par arité et par branche de profondeur.
- La fixture `constant_inside_witness` n'impose aucun plancher `emitted_positive_constant`; son CTest peut rester vert sans exercer la branche annoncée.
- Le reçu est encore écrit avant `--min-positive-depth` et les contrôles finaux d'injection. Un processus rouge peut donc laisser un reçu avec `exit_code:0`; ce point reste P0.
- Le tri du catalogue lit toujours les quatre cases de `support` alors que les queues des arités deux et trois ne sont pas initialisées. Le résultat dépend d'octets indéterminés; ce point reste P0.

## Conclusion adressée à Claude

Les nouveaux résultats solides sont le certificat Q0 **strict**, le comparateur chirotopique `i128`, le clipping exact des formes et surtout le lemme ci-dessus qui ferme PEL-1 sous `RelevantGP`. La voie v3 la plus défendable devient donc : A2p comme source exhaustive de plans porteurs jusqu'à profondeur $s_{\max}-2$, A2e comme constructeur 2D par paire dédupliquée, puis un vrai parcours shallow output-sensitive.

Avant de poursuivre les mesures, il faut remplacer la fixture Q1 par celle déjà certifiée dans les audits, grouper les concurrences, rendre les reçus fail-closed et retirer les claims universels fondés sur les seules campagnes finies. PEL-2 et PEL-4, ainsi que le volume aval, restent les décisions produit.

GCP non utilisé.

# Second audit de `PROPOSITION.md` — révision edge-anchored shallow

> [!IMPORTANT]
> Second audit indépendant figé sur le commit `901e8019f244c6238e698b8bdd78257580d5a398`, empreinte SHA-256 de [`PROPOSITION.md`](PROPOSITION.md) `e8aeb56076dd1e80e63b25a5e3c46880a030daa384d03c23652b9a621cb2f31b`, 532 lignes, daté du 8 août 2026 à 22:11:40 UTC. Le census associé [`census_tukey_shallow_20260808.json`](census_tukey_shallow_20260808.json) a l'empreinte `d8ed1bf8ee5967faf58922af5dd69cedb858c34cba8548663ddfa67a604fd23a`. Ce rapport complète le [premier audit](AUDIT_PROPOSITION.md); il ne le remplace pas historiquement.

> [!NOTE]
> Contexte : `phase=exploration_v3_hors_registre`, `backend=none_documentary_audit`, `profile=hgp_reduced_cible`, `mode=edge_anchored_shallow_reaudit_v2`, `public_status=not_claimed`. Aucun code v3, aucune porte et aucun SLO ne sont ouverts par cet audit.

> [!CAUTION]
> **Nouveau verdict : GO conditionnel pour la direction d'architecture de la révision `901e801`; toujours NO-GO comme conception arrêtée ou produit implémentable immédiatement.** Claude a intégré le cœur du premier audit : A1-source séparée, peeling 2D A2e, rang par profondeur, profils numériques distincts, source HGP complète, streaming, forêt non certifiée et deux pistes CPU/GPU. Le blocage principal n'est plus un mauvais choix d'architecture; c'est l'absence des preuves constructives, des reçus reproductibles et des portes qui permettraient de transformer cette architecture en algorithme exact.

## 1. Ce que la révision a réellement fermé

| finding du premier audit | statut dans `901e801` | appréciation |
| --- | --- | --- |
| A1 contre A2 était une fausse dichotomie | **corrigé** | A1-source, A2e et A2p sont maintenant séparés |
| la complétude de A1 supposait les ancres connues | **corrigé** | la source center-cover complète est nommée comme pièce ouverte |
| confusion entre tangente non contrainte, $R$ convexe et sphère critique | **corrigé** | les trois objets sont distingués au §1.5 |
| $R$ devait être un supremum | **corrigé** | le §6 emploie désormais la bonne notion |
| le gain de la coupe exige un range-report indexé | **correction incomplète et non sûre** | la coupe peut filtrer les carriers, mais pas supprimer les témoins qui déterminent la profondeur |
| J10 ne borne pas la population | **corrigé** | le cas 25 026 points est explicitement reconnu |
| le rang doit venir de la profondeur shallow | **corrigé** | c'est maintenant le cœur A2e du §4.2 |
| supports seuls $\neq$ source HGP | **corrigé** | incidences silencieuses, couverture et verticales sont réintroduites |
| dyadique et quantifié étaient confondus | **corrigé** | deux profils disjoints sont définis |
| forêt v2 et O2 sur-certifiés | **corrigé** | ils redeviennent sémantique candidate et fixtures |
| GPU-first contre CPU-first | **corrigé** | oracle CPU et falsificateur CUDA avancent en parallèle |
| sortie énorme contre intermédiaires énormes | **corrigé** | la décision à deux branches figure dans V3-2 |

Ce changement de verdict est important : la proposition actuelle n'est plus le chemin A1-cascade refusé par le premier audit. La phrase structurante du §11 est maintenant la bonne :

> source complète fail-open de paires diamétrales, arrangement shallow exact par paire, descente locale indexée, émission canonique en flux et réduction HGP résidente.

L'invariant d'architecture est respecté : aucune mosaïque de Delaunay d'ordre supérieur, aucun Gamma global, aucune matrice paire--point et aucune liste globale de cliques ne sont requis dans le chemin produit.

## 2. Findings nouveaux ou encore ouverts

### 2.1 P0 — un audit n'est pas une autorité mathématique

La dernière phrase de la proposition déclare que [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md) est « l'autorité de cette révision ». C'est contractuellement faux.

Le registre opérationnel dit explicitement que l'autorité mathématique reste la [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) et le [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md). Un audit peut :

- découvrir une contradiction;
- proposer une preuve;
- définir une fixture ou une porte;
- recommander une architecture.

Il ne devient une autorité qu'après reprise des énoncés dans les sources normatives, statut de preuve explicite, fixture et gate correspondant. La phrase doit devenir : **« cet audit motive la révision; l'autorité reste la spécification et le registre des preuves »**.

### 2.2 P0 — la coupe par $R$ sépare carriers et témoins; elle ne peut pas filtrer la profondeur

Le lemme du §6 donne une condition **nécessaire pour appartenir au support** : si $z$ est un carrier d'une sphère dont l'arête diamétrale a longueur $D$, alors $2R(z)\geq D$. Il ne dit pas qu'un point avec $2R(z)<D$ est absent de l'intérieur de cette sphère. Sa forme affine $h_z$ peut rester strictement positive au centre candidat et doit alors contribuer à $c_e$, à $\delta_e$ et au rang fermé.

L'élagage proposé par `max_tau_hi < D` est donc incorrect s'il retire le point du range-report qui calcule la profondeur. Il faut deux rôles explicites :

- un flux **witness/depth** complet, qui conserve toutes les formes susceptibles de compter comme intérieures;
- un masque **carrier_eligible**, certifié par $2R(z)\geq D$, qui seul autorise une droite, un pied ou une intersection à engendrer un support.

Un point inéligible comme carrier peut encore faire changer la profondeur en traversant sa droite. Le constructeur doit donc soit conserver sa forme dans l'arrangement pondéré, soit disposer d'une requête exacte indépendante de profondeur aux features éligibles. Filtrer les deux flux avec le même agrégat LBVH donnerait des rangs sous-comptés et pourrait publier des sphères de rang supérieur à $K$.

Enfin, le nom `max_tau_hi` réintroduit la confusion du §1.5 : $\tau$ est la quantité tangentielle non contrainte, tandis que le lemme porte sur $R$ à centre dans $\mathrm{conv}(X)$. Un nom comme `max_two_R_upper_hi`, avec définition d'intervalle et règle fail-open, est nécessaire.

### 2.3 P0 — les nouvelles mesures ne possèdent pas de reçu reproductible

La révision ajoute beaucoup de résultats : débits de quadruples, 69 ans, 23 millions de sphères, 4,4 Go, 200 secondes de forêt, 18 601 comparaisons GMP, 40 nuages décidés, et census bunny. La plupart ne sont liés à aucun artefact brut.

Le JSON de census contient seulement `cloud`, `n`, `s_max`, `shallow` et `pct`. Il ne contient pas :

- schéma et version du prédicat;
- commit, source du binaire, compilateur, options et machine;
- digest et provenance du nuage d'entrée;
- règle d'assemblage des dix captures;
- algorithme, seed et digest de décimation;
- jeu exact des 4 096 directions et convention de demi-espace;
- convention leave-one-out et traitement du plan frontière;
- compteurs `attempted`, `decided`, `skipped`, overflow et non-finis;
- temps brut, sortie brute et manifeste de fichiers.

Le census est donc un diagnostic intéressant, pas une preuve, une baseline stable ou une porte. Les autres mesures sans sidecar doivent être classées de la même façon.

### 2.4 P0 — le « minorant donc majorant » du §1.6 est une erreur logique

Le §1.6 appelle la campagne 4 096 directions un **minorant** de l'ensemble des points où la quantité tangentielle non contrainte est infinie, puis en fait un **majorant** de l'ensemble où la borne à centre convexe échoue.

Même si l'ensemble convexe problématique $B$ est inclus dans l'ensemble non contraint $A$, une observation échantillonnée $S\subseteq A$ ne donne aucune inclusion entre $S$ et $B$. Un sous-ensemble observé d'un sur-ensemble n'est pas un majorant du sous-ensemble recherché.

Le tableau prouve seulement : « le prédicat échantillonné a trouvé cette proportion de témoins non contraints ». Il ne borne ni supérieurement ni inférieurement la fréquence d'échec d'un majorant certifié de $R$ à centre convexe.

La conclusion « alarme surface levée » est elle aussi trop forte. Le Stanford bunny est un bon corpus supplémentaire, mais ce n'est ni SemanticKITTI, ni une famille exhaustive de multi-captations. L'affirmation causale « l'erreur de recalage donne l'épaisseur qui explique la baisse » est cohérente avec la mesure, pas démontrée par elle.

### 2.5 P1 — les temps du tableau ne correspondent pas au débit annoncé

Le §1.2 annonce $3{,}2$ à $8{,}9\cdot10^6$ quadruples/s/cœur sur 48 cœurs, mais ses temps ponctuels emploient environ $10^7$. L'intervalle cohérent est :

| quadruples | temps à 48 cœurs avec le débit publié |
| ---: | ---: |
| $1{,}04\cdot10^{18}$ | 77 à 215 ans |
| $4{,}53\cdot10^{16}$ | 3,36 à 9,35 ans |
| $1{,}59\cdot10^{15}$ | 43 à 120 jours |
| $1{,}32\cdot10^{11}$ | 309 à 859 s |
| $4{,}39\cdot10^{10}$ | 103 à 286 s |
| $1{,}79\cdot10^{10}$ | 42 à 117 s |
| $4{,}08\cdot10^7$ | 0,096 à 0,266 s |

Ce défaut ne change pas le no-go de l'énumération exhaustive, mais confirme que le tableau n'est pas un reçu. À une seconde, le meilleur débit publié permet $\lvert W\rvert\leq38$, pas 39.

Il faut surtout dire si chaque $\lvert W\rvert$ est une moyenne, un quantile ou un maximum. Si c'est une moyenne, remplacer le coût réel $\sum_p\binom{\lvert W_p\rvert}{3}$ par $n\binom{\overline{\lvert W\rvert}}{3}$ le sous-estime par convexité; les tails peuvent gouverner le temps. La distribution complète et le débit brut doivent accompagner le tableau.

Enfin, « $\lvert W\rvert\geq130$ est géométriquement nécessaire à $K=10$ » n'est établi que sur le corpus non scellé décrit ensuite. Il faut écrire « observé nécessaire sur ce corpus » et publier la distribution, notamment le tail à 189.

### 2.6 P0 — la boule $\bar B(c,2r)$ ne couvre pas encore toute la descente

Le calcul du §1.3 est valide pour le **premier pas**. Si $F'\subseteq F\subseteq\bar B(c,r)$, le centre $c'$ de la miniboule de $F'$ appartient à $\mathrm{conv}(F')\subseteq\bar B(c,r)$ et son rayon $r'$ vérifie $r'\leq r$. Un intrus du premier pas appartient donc à $\bar B(c,2r)$ :

$$\lVert x-c\rVert\leq\lVert x-c'\rVert+\lVert c'-c\rVert\leq r'+r\leq2r.$$

Mais après le premier remplacement, le nouvel ensemble contient l'intrus $x$; il n'est plus un sous-ensemble du support initial $F$. Rien dans cette preuve ne garantit que sa nouvelle miniboule reste centrée dans $\bar B(c,r)$, que son rayon reste inférieur à $r$, ni que l'intrus suivant appartienne encore à la boule initiale $\bar B(c,2r)$. Utiliser cette boule fixe pour toute la chaîne serait donc une troncature non prouvée.

Deux voies sont sûres :

- refaire à chaque pas une requête LBVH exacte dans la miniboule **courante** $\bar B(c_i,r_i)$;
- ou démontrer une enveloppe globale invariante couvrant toute la chaîne, puis la confronter au scan exhaustif.

La première est la référence recommandée. Il reste à fermer le choix canonique victime/intrus, les égalités, une mesure strictement décroissante, la terminaison, l'unicité pertinente pour HGP et l'identité byte-à-byte avec le scan exhaustif. La boule $2r$ est un bon accélérateur du premier pas; elle ne borne ni la cardinalité visitée ni toute la descente.

Les chiffres de forêt sont également trop agrégés. `min_index` reçoit seulement les sphères uniques de rang $k$, pas les 23 millions de sphères. Les événements de rang $k+1$ se répartissent entre les ordres; appliquer « 21 % de 23 millions » à chacun des dix ordres dépasserait le catalogue total. Il faut publier l'histogramme $S_k$, le maximum de la map, le nombre total de bras et de pas, les visites LBVH, les plateaux, les censures et les prédicats exacts par ordre. Les « 20 s par ordre » et « 200 s pour la tour » restent des diagnostics.

### 2.7 P1 — 23 millions de sphères et 4,4 Go sont des estimations d'un prototype incomplet

La recommandation de streamer est excellente et devrait rester, indépendamment du chiffre. Mais les 450 à 510 sphères par point viennent d'une source v2 dont la complétude, la canonicité et le domaine exact ne sont pas certifiés. `CriticalSphere` vaut bien 160 octets sur l'ABI observée, mais les 32 octets moyens de membres et les 23 millions de records ne sont liés à aucun manifeste.

La conclusion correcte est : **l'architecture v3 ne doit exiger aucune matérialisation globale du catalogue et doit publier son high-water**. La conclusion trop forte est : « le catalogue exact v3 vaut nécessairement 4,4 Go ».

### 2.8 P0 — le flux ne dispense pas du tri global exact

Les ancres parallèles produiront les événements dans un ordre arbitraire. Or le réducteur HGP ne peut pas les consommer immédiatement : dans chaque ordre, il exige un ordre global par niveau rationnel exact, un groupement de **tous** les événements de même niveau, un snapshot pré-lot, puis une application atomique du lot. Déduplication, propriétaire, incidences silencieuses et `coverage_delta` doivent respecter ce même ordre.

Le pipeline du §7 saute l'étage `sort / group equal levels`. Sans producteur monotone — propriété qui n'est ni énoncée ni plausible entre ancres indépendantes — « consommé au fil de l'eau » ne définit pas le même objet que la réduction normative.

La voie compatible avec l'invariant mémoire est : produire des runs bornés triés par clé exacte et identifiants canoniques, les fusionner par un merge déterministe, réunir les clés rationnellement égales avant toute mutation, puis seulement alimenter le réducteur. Le contrat doit publier taille des runs, high-water RAM, octets écrits/lus, coût des comparaisons exactes, nombre et taille maximale des lots. Une alternative entièrement résidente est acceptable si elle prouve les mêmes bornes. Dans les deux cas, on évite le catalogue géométrique global; on ne peut pas éviter l'ordonnancement global de ses événements utiles.

### 2.9 P1 — les propriétés du peeling A2p ne sont pas les obligations de A2e

Les cinq propriétés du §4.3 sont formulées pour l'arrangement 3D ancré par point, puis déclarées valables pour tout peeling. Elles ne s'appliquent pas toutes littéralement à A2e :

- M2, atteignabilité depuis une cellule de niveau zéro, dépend du constructeur choisi; un constructeur direct de premiers niveaux 2D n'a pas nécessairement cette exploration;
- M4 emploie $W_\rho$ et la borne ponctuelle de la v2, tandis que A2e emploie le range-report J10 et le center-cover par ancre;
- M5 porte sur les cellules non bornées d'un arrangement 3D, alors que A2e est clipé dans une ellipse compacte et ne possède aucune cellule non bornée.

M5 est en outre très probablement faux tel qu'écrit : une cellule polyédrique fermée non vide, même non bornée, possède une projection finie de l'origine. « Cellule non bornée » n'implique donc pas « absence de sphère finie » sans une condition supplémentaire portant sur le bon centrage, le support et le rang. De même, la formule `rang = 1 + niveau` doit fixer la convention des carriers sur la frontière; pour une face de dimension $j$, la formule géométrique générique est `support + profondeur`, soit $4-j+\mathrm{profondeur}$ dans cet arrangement.

Les labels M1–M5 devraient devenir `PEL-1` à `PEL-5` pour ne pas entrer en collision avec l'obligation normative M.1 du registre. Surtout, V3-4 ne peut accepter des obligations A2e « démontrées ou explicitement ouvertes » : celles nécessaires au produit doivent être fermées. Les obligations propres à A2p peuvent rester ouvertes tant que A2p demeure un oracle de recherche sans autorité.

Le gate V3-4 ne doit donc pas demander indistinctement « M1 à M5 pour A2e ». Il faut deux listes.

Pour A2e :

1. couverture complète des ancres diamétrales;
2. classification exacte `constant_inside / active / constant_outside`;
3. construction du préfixe shallow sans $\sum_e m_e^2$;
4. clipping exact par l'ellipse;
5. profondeur et conflits complets;
6. concurrences groupées par centre exact;
7. supports deux, trois et quatre complets;
8. shell, bon centrage, propriétaire et rang fermés;
9. transcript indépendant et sortie canonique.

Pour A2p : dictionnaire cellule--support, convention dimension/profondeur, atteignabilité, coût global sur les $n$ ancres, traitement réel des cellules non bornées et duplication. A2p reste un excellent oracle différentiel, mais ses obligations ne doivent pas bloquer ou certifier A2e par analogie.

### 2.10 P1 — `RelevantGP` n'est pas « absence de coquille cosphérique »

Le §3 réduit la position générale à l'absence de coquille cosphérique. La spécification exige davantage sur le périmètre pertinent : points distincts, support unique et affinement indépendant, centre dans l'intérieur relatif, barycentriques non nulles, shell complet sans point extérieur, prédicats et égalités exacts.

La proposition réutilise ensuite le mot `RelevantGP` dans les gates. Elle doit soit reprendre sa définition normative complète, soit pointer explicitement vers le §12 de la spécification. Un simple scan des événements acceptés ne suffit pas à établir `relevant_gp_complete`.

### 2.11 P1 — l'accord GMP observé ne certifie pas `sphere.hpp` ni le profil dyadique

Les bornes 85, 170, 137 et 307 bits semblent compatibles avec `BigInt<4>` et `BigInt<6>`. Mais « majorants prouvés » doit pointer vers une dérivation symbolique reprise dans le registre des preuves. L'accord sur 18 601 paires est une bonne régression, pas une preuve de :

- toutes les largeurs intermédiaires;
- l'absence d'overflow avant garde;
- tous les prédicats `sphere1..4`;
- la normalisation des signes et dénominateurs;
- les comparaisons aux valeurs extrêmes du domaine.

Le composant v2 peut être **candidat à réutiliser après audit par prédicat**. Le qualifier globalement de « sain » reste prématuré.

Les majorants affichés avec $M=65535$ ne concernent que `quantized_u16_input`. Ils ne dimensionnent pas `exact_dyadic_input`, dont les exposants binary64 et les produits intermédiaires exigent une dérivation et un fallback propres, avec un SLO séparé. Inversement, « la précision arbitraire est la seule option qui décide la grille » n'est pas un théorème : une largeur fixe indépendante suffisamment grande décide aussi un domaine entier borné. La multiprécision reste le choix robuste pour l'oracle indépendant, pas une exclusivité mathématique établie par la campagne 40/40.

## 3. Audit mathématique de A2e

### 3.1 La réduction est correcte

Pour une paire diamétrale $e=pq$, le plan médiateur est bidimensionnel. Dans une base rationnelle non normalisée, chaque point définit une forme affine dont le signe est exactement la puissance relative à la sphère. Sous position générale, deux droites actives s'intersectent au centre du support quatre et :

$$\mathrm{rang}_{\text{ferme}}=4+c_e+\delta_e.$$

La note mathématique du dépôt enregistre aussi la borne :

$$Z_e\leq m_e(\kappa_e+1),\qquad\kappa_e=s_{\max}-4-c_e.$$

Ce sont les deux apports décisifs. Ils justifient pleinement de préférer A2e à la cascade de tuples.

### 3.2 La borne de sortie n'est pas encore un algorithme livré

La borne $Z_e$ limite le nombre de sommets shallow. Elle ne fournit pas automatiquement un constructeur qui les trouve dans le même ordre de complexité. Le dépôt cite des algorithmes de premiers niveaux sous modèle real-RAM, mais leur transfert doit encore fermer :

- coefficients rationnels de grande largeur;
- clipping par ellipse;
- lignes parallèles et concurrences;
- ordre total et reproductibilité;
- graphe de conflits et transcript de complétude;
- scratch borné et ordonnancement des ancres lourdes;
- mapping GPU sans divergence ou phase sérielle dominante.

Un prototype qui calcule d'abord toutes les intersections puis vérifie la profondeur reste $\Theta(m_e^2)$, même si sa sortie respecte $Z_e$.

### 3.3 Supports trois et deux

La proposition nomme correctement les quatre arités, mais un constructeur des seuls **sommets shallow** ne produit que les supports quatre. Dans l'arrangement 2D ancré par $e=pq$ :

- support quatre : intersection de deux droites, donc sommet;
- support trois : pied métrique de la forme quadratique sur une droite $h_z=0$, situé dans l'intérieur d'une arête de l'arrangement si les signes le permettent;
- support deux : $t=0$, situé dans une face.

Pour les triangles, la région de Jung est distincte : $t^{\mathsf{T}}(B^{\mathsf{T}}B)t\leq D^2/12$, et le budget est $\kappa_e^{(3)}=s_{\max}-3-c_e^{(3)}$. Le support quatre emploie $D^2/8$ et $\kappa_e^{(4)}=s_{\max}-4-c_e^{(4)}$. Les constantes, témoins intérieurs et receipts ne sont donc pas interchangeables.

Au rang fermé 11, une réfutation exige dix témoins stricts pour un support deux, neuf pour un support trois et huit pour un support quatre. Le center-cover P1a actuel ne traite que support quatre; il ne peut autoriser ni le sourceur ni les prunes des deux autres arités.

A2e doit donc construire le **complexe shallow utile** — faces, arêtes et sommets — ou fournir un algorithme exact batched de profondeur aux pieds et à $t=0$ sans travail quadratique. Chaque arité doit avoir son transcript, ses masques de receipt et son différentiel exhaustif. Une preuve support quatre ne se propage jamais automatiquement aux arités trois et deux.

### 3.4 Concurrences

Sous `RelevantGP`, le cas produit vise deux droites porteuses au centre d'un support quatre. Néanmoins, l'implémentation doit détecter toute concurrence supplémentaire avant d'affirmer le domaine. Elle ne peut ni choisir deux lignes arbitraires, ni remplacer un centre concurrent par $\binom{t}{2}$ intersections perturbées.

Le transcript doit porter le shell groupé, la profondeur stricte hors shell, le centre exact, l'owner diamétral et la décision de domaine. Une ambiguïté reste fail-open ou donne `unsupported_degeneracy`.

## 4. Preuves et mesures à exiger avant du code v3 autoritaire

### Gate A — autorité documentaire

- reprendre A2e, la portée **premier pas** du lemme $2r$ et les profils d'entrée dans la spécification et le registre;
- classer chaque énoncé `proved_here`, `conditional_theorem`, `proof_obligation` ou `experimental_target`;
- laisser ce rapport au statut d'audit, jamais d'autorité.

### Gate B — reçus de l'investigation

- sidecars complets pour le tableau de temps, le catalogue, la forêt, GMP et le census;
- digests des inputs, sources, binaires et sorties;
- identité de campagne fermée et aucune omission silencieuse;
- distinction stricte entre mesure, extrapolation et théorème.

### Gate C — A1-source

- partition exacte de $\binom{n}{2}$ sans tableau de paires;
- à $n=32$, replay de chaque paire et microtuile canoniques, de chaque prune et de chaque région de cover; l'identité de masse seule ne prouve ni absence de doublon ni absence d'omission;
- chemins et reçus séparés pour supports deux, trois et quatre, avec seuils de réfutation respectifs 10, 9 et 8 au rang fermé 11; P1a support quatre n'autorise pas les deux autres;
- séparation certifiée entre le masque `carrier_eligible` et le flux complet des témoins de profondeur;
- métriques $Q$, $V_W$, $a$, $M$, tails et high-water à 50 k;
- no-go si la majorité atteint les microtuiles ou si la queue sérialise.

### Gate D — A2e CPU

- constructeur shallow sans travail en $\sum_e m_e^2$;
- complexe utile faces--arêtes--sommets, ou profondeur batched exacte aux pieds et à $t=0$ sans coût quadratique;
- conservation de toutes les formes témoins dans le rang, même lorsque leur point est inéligible comme carrier;
- régions, constantes et budgets distincts pour supports deux, trois et quatre;
- comparaison exhaustive jusqu'à $n\leq14$ et locale brute par ancre;
- permutations, égalités, parallèles, concurrences et limites de l'ellipse;
- transcript complet et sortie byte-à-byte canonique.

### Gate E — descente indexée

- fixture permanente du lemme $\bar B(c,2r)$ limitée au premier pas;
- requête exacte dans la miniboule courante à chaque remplacement, sauf preuve d'une enveloppe globale invariante;
- différentiel de la chaîne entière contre le scan global;
- terminaison, baisse stricte, victime canonique et plateaux;
- compteurs par ordre, pas extrapolation depuis le total catalogue;
- raccord aux incidences silencieuses et lots exacts.

### Gate F — pipeline HGP

- runs bornés triés par niveau rationnel exact et identifiants canoniques, puis merge déterministe;
- regroupement global de chaque niveau égal, snapshot pré-lot et application atomique avant le réducteur;
- ledger du tri : high-water, octets, comparaisons, spills éventuels et taille maximale des lots;
- facettes, cofaces, silences, attaches, `coverage_log` et lots égaux;
- verticales et carrés de naturalité;
- une seule publication canonique terminale;
- `public_status=not_claimed` jusqu'à fermeture de toutes ces pièces.

## 5. Corrections précises recommandées à `PROPOSITION.md`

1. Remplacer « l'audit est l'autorité » par l'autorité spécification + registre.
2. Lier chaque nouvelle mesure à un sidecar; marquer les chiffres sans reçu `diagnostic_only`.
3. Supprimer l'implication minorant échantillonné $\Rightarrow$ majorant convexe.
4. Remplacer « alarme surface levée » par « première observation bunny, transfert ouvert ».
5. Corriger les temps par l'intervalle 3,2–8,9 M/s/cœur, publier la distribution de $W$ et remplacer la borne une seconde par $\lvert W\rvert\leq38$.
6. Remplacer « $W\geq130$ géométriquement nécessaire » par « observé nécessaire sur le corpus mesuré ».
7. Remplacer `max_tau_hi` par un agrégat sur $2R$ et séparer strictement carriers éligibles et témoins complets de profondeur.
8. Limiter le lemme $2r$ au premier pas; employer la miniboule courante à chaque remplacement ou fournir une nouvelle preuve d'enveloppe globale.
9. Retirer « `std::map` sur 23 millions de clés » jusqu'à un histogramme $S_k$ et un ledger par ordre.
10. Insérer avant le réducteur le tri global exact, le merge déterministe et les lots de niveaux égaux atomiques.
11. Garder le streaming comme invariant, mais classer 4,4 Go comme estimation diagnostique et publier le high-water du tri.
12. Séparer les obligations A2e des propriétés du peeling A2p; renommer ces dernières `PEL-1` à `PEL-5`.
13. Retirer ou reformuler M5 : non-borné n'implique pas absence de projection finie, donc pas absence de sphère finie.
14. Fermer toutes les obligations produit A2e à V3-4; seules celles de l'oracle A2p peuvent rester ouvertes.
15. Définir `RelevantGP` par référence normative complète.
16. Remplacer « `sphere.hpp` sain » par « candidat, largeur symbolique et différentiel à fermer par prédicat », séparément pour u16 et dyadique.
17. Ajouter au gate A2e les faces, arêtes et sommets utiles, les supports deux et trois, les concurrences et le transcript de conflits.
18. Exiger du center-cover un replay canonique paire/microtuile/prune; l'identité de masse seule est insuffisante.
19. Faire du census bunny une fixture reproductible, puis ajouter SemanticKITTI et les familles sanctionnées.

## 6. Décision finale

La révision `901e801` a accepté le bon changement conceptuel : **l'arête diamétrale ne sert pas seulement à borner le rayon; elle réduit la dimension et transforme le rang en profondeur d'arrangement**. C'est une proposition sérieuse et probablement la meilleure voie actuellement visible pour une v3 allégée. Les nouveaux P0 ne réfutent pas A2e; ils empêchent une implémentation trop agressive d'en perdre l'exactitude.

La décision est donc :

- **GO** pour formaliser A2e, construire son oracle CPU et mesurer A1-source;
- **GO** pour prototyper tôt les masses et queues sur GPU sous statut `proposal_only`;
- **NO-GO** pour élaguer les témoins de profondeur avec le filtre carrier $R$;
- **NO-GO** pour réutiliser une boule $2r$ fixe pendant toute la descente ou réduire un flux non trié;
- **NO-GO** pour appeler les nouvelles mesures des preuves ou des qualifications;
- **NO-GO** pour commencer un produit v3 avant source d'ancres complète, constructeur shallow non quadratique et source HGP aval;
- **NO-GO** pour tout claim exact, SLO ou autorité publique.

Le prochain livrable utile n'est pas un nouveau catalogue. C'est un **constructeur CPU exact et transcriptable du complexe shallow utile**, avec toutes les formes témoins mais seulement les carriers éligibles, supports 2/3/4 et comparaison exhaustive. En parallèle, le center-cover doit mesurer la parcimonie réelle des ancres et le réducteur doit recevoir un prototype de runs triés/merge exact : ces trois pièces déterminent ensemble si la voie peut devenir un produit.

## 7. Références déterminantes

- [`PROPOSITION.md`](PROPOSITION.md), révision auditée.
- [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md), premier audit de la cascade A1.
- [`census_tukey_shallow_20260808.json`](census_tukey_shallow_20260808.json), résultat diagnostique à rendre reproductible.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md), théorèmes A2e et gates center-cover/shallow.
- [`GERMINATION_LOCALE_SUPPORTS_3_4.md`](../docs/math/GERMINATION_LOCALE_SUPPORTS_3_4.md), distinction entre tangente et $R$ convexe.
- [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md), domaine exact, `RelevantGP`, objet HGP et verticales.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md), seule autorité de statut des preuves avec la spécification.
- [`WARNING_AUDIT_PUBLICATION_3.md`](../morsehgp3D_v2/WARNING_AUDIT_PUBLICATION_3.md), limites de la v2 et d'O2.

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
| le gain de la coupe exige un range-report indexé | **corrigé sur le papier** | `max_tau_hi` est proposé, sans implémentation ni preuve logicielle |
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

### 2.2 P0 — les nouvelles mesures ne possèdent pas de reçu reproductible

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

### 2.3 P0 — le « minorant donc majorant » du §1.6 est une erreur logique

Le §1.6 appelle la campagne 4 096 directions un **minorant** de l'ensemble des points où la quantité tangentielle non contrainte est infinie, puis en fait un **majorant** de l'ensemble où la borne à centre convexe échoue.

Même si l'ensemble convexe problématique $B$ est inclus dans l'ensemble non contraint $A$, une observation échantillonnée $S\subseteq A$ ne donne aucune inclusion entre $S$ et $B$. Un sous-ensemble observé d'un sur-ensemble n'est pas un majorant du sous-ensemble recherché.

Le tableau prouve seulement : « le prédicat échantillonné a trouvé cette proportion de témoins non contraints ». Il ne borne ni supérieurement ni inférieurement la fréquence d'échec d'un majorant certifié de $R$ à centre convexe.

La conclusion « alarme surface levée » est elle aussi trop forte. Le Stanford bunny est un bon corpus supplémentaire, mais ce n'est ni SemanticKITTI, ni une famille exhaustive de multi-captations. L'affirmation causale « l'erreur de recalage donne l'épaisseur qui explique la baisse » est cohérente avec la mesure, pas démontrée par elle.

### 2.4 P1 — les temps du tableau ne correspondent pas au débit annoncé

Le §1.2 annonce un débit maximal de $8{,}9\cdot10^6$ quadruples par seconde et par cœur sur 48 cœurs. À ce débit :

$$\frac{1{,}04\cdot10^{18}}{48\cdot8{,}9\cdot10^6}\approx77\ \text{ans},$$

et non 69 ans. De même :

$$\frac{4{,}39\cdot10^{10}}{48\cdot8{,}9\cdot10^6}\approx103\ \text{s},$$

et non 91 s. Les autres lignes portent le même facteur; elles semblent utiliser environ $10^7$ opérations/s/cœur, valeur absente de l'intervalle publié.

Ce défaut ne change pas le no-go de l'énumération exhaustive, mais il montre que le tableau n'est pas encore un reçu. Il faut publier la formule, le débit exact associé à chaque ligne et les intervalles plutôt qu'un point extrapolé.

La phrase « $\lvert W\rvert\geq130$ est géométriquement nécessaire à $K=10$ » est également trop générale. Les rangs 89 à 189 sont des observations d'un corpus; ce n'est pas une borne universelle de la géométrie à ordre dix.

### 2.5 P1 — le lemme $\bar B(c,2r)$ est bon, les extrapolations de forêt ne le sont pas encore

Le nouveau lemme de descente est sain sous le contrat visé. Si $F'\subseteq F\subseteq\bar B(c,r)$, le centre $c'$ de la miniboule de $F'$ appartient à $\mathrm{conv}(F')\subseteq\bar B(c,r)$ et son rayon $r'$ vérifie $r'\leq r$. Tout intrus $x\in\bar B(c',r')$ satisfait alors :

$$\lVert x-c\rVert\leq\lVert x-c'\rVert+\lVert c'-c\rVert\leq r'+r\leq2r.$$

Le remplacement d'un scan global par une requête LBVH dans $\bar B(c,2r)$ est donc une vraie amélioration candidate. Ce lemme ne prouve toutefois pas à lui seul :

- la terminaison de la descente;
- l'unicité du minimum atteint;
- le traitement des plateaux et égalités;
- le choix canonique de la victime;
- la complétude des incidences silencieuses;
- la correction du résultat HGP aval.

Les nouveaux chiffres de forêt sont en outre incohérents avec le code cité. `min_index` reçoit seulement les sphères de rang exactement $k$, dédupliquées par membres; il n'est pas une `std::map` de toutes les 23 millions de sphères du catalogue. La proposition dit elle-même qu'environ 21 % des sphères auraient le rang $k+1$, ce qui interdit d'identifier sans histogramme le nombre total et le nombre de clés d'un ordre.

Il faut publier, pour chaque $k$, le nombre de naissances uniques, d'événements de rang $k+1$, de bras, de pas, de visites LBVH, de plateaux, de censures et de clés réellement résidentes. Les « 20 s par ordre » et « 200 s pour la tour » restent des extrapolations non qualifiantes.

### 2.6 P1 — 23 millions de sphères et 4,4 Go sont des estimations d'un prototype incomplet

La recommandation de streamer est excellente et devrait rester, indépendamment du chiffre. Mais les 450 à 510 sphères par point viennent d'une source v2 dont la complétude, la canonicité et le domaine exact ne sont pas certifiés. `CriticalSphere` vaut bien 160 octets sur l'ABI observée, mais les 32 octets moyens de membres et les 23 millions de records ne sont liés à aucun manifeste.

La conclusion correcte est : **l'architecture v3 ne doit exiger aucune matérialisation globale du catalogue et doit publier son high-water**. La conclusion trop forte est : « le catalogue exact v3 vaut nécessairement 4,4 Go ».

### 2.7 P1 — M1 à M5 mélangent les obligations de A2p et de A2e

Les cinq propriétés du §4.3 sont formulées pour l'arrangement 3D ancré par point. Elles ne s'appliquent pas toutes littéralement à A2e :

- M2, atteignabilité depuis une cellule de niveau zéro, dépend du constructeur choisi; un constructeur direct de premiers niveaux 2D n'a pas nécessairement cette exploration;
- M4 emploie $W_\rho$ et la borne ponctuelle de la v2, tandis que A2e emploie le range-report J10 et le center-cover par ancre;
- M5 porte sur les cellules non bornées d'un arrangement 3D, alors que A2e est clipé dans une ellipse compacte et ne possède aucune cellule non bornée.

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

Pour A2p : dictionnaire cellule--support, atteignabilité, coût global sur les $n$ ancres, cellules non bornées et duplication. A2p reste un excellent oracle différentiel, mais ses obligations ne doivent pas bloquer ou certifier A2e par analogie.

### 2.8 P1 — `RelevantGP` n'est pas « absence de coquille cosphérique »

Le §3 réduit la position générale à l'absence de coquille cosphérique. La spécification exige davantage sur le périmètre pertinent : points distincts, support unique et affinement indépendant, centre dans l'intérieur relatif, barycentriques non nulles, shell complet sans point extérieur, prédicats et égalités exacts.

La proposition réutilise ensuite le mot `RelevantGP` dans les gates. Elle doit soit reprendre sa définition normative complète, soit pointer explicitement vers le §12 de la spécification. Un simple scan des événements acceptés ne suffit pas à établir `relevant_gp_complete`.

### 2.9 P1 — l'accord GMP observé ne certifie pas `sphere.hpp`

Les bornes 85, 170, 137 et 307 bits semblent compatibles avec `BigInt<4>` et `BigInt<6>`. Mais « majorants prouvés » doit pointer vers une dérivation symbolique reprise dans le registre des preuves. L'accord sur 18 601 paires est une bonne régression, pas une preuve de :

- toutes les largeurs intermédiaires;
- l'absence d'overflow avant garde;
- tous les prédicats `sphere1..4`;
- la normalisation des signes et dénominateurs;
- les comparaisons aux valeurs extrêmes du domaine.

Le composant v2 peut être **candidat à réutiliser après audit par prédicat**. Le qualifier globalement de « sain » reste prématuré.

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

La proposition nomme correctement les quatre arités, mais elles doivent avoir des reçus distincts. Pour les triangles, le point minimisant la forme quadratique sur $h_z=0$ est le circumcentre; l'ellipse, le nombre d'intérieurs constants et le seuil diffèrent de l'arité quatre. Pour les paires, $t=0$ ne dispense pas du shell complet et du rang fermé.

Les neuf témoins support trois et huit témoins support quatre doivent apparaître séparément dans le center-cover, dans les masques de receipt et dans le différentiel exhaustif. Une preuve support quatre ne se propage jamais automatiquement à l'arité trois.

### 3.4 Concurrences

Sous `RelevantGP`, le cas produit vise deux droites porteuses au centre d'un support quatre. Néanmoins, l'implémentation doit détecter toute concurrence supplémentaire avant d'affirmer le domaine. Elle ne peut ni choisir deux lignes arbitraires, ni remplacer un centre concurrent par $\binom{t}{2}$ intersections perturbées.

Le transcript doit porter le shell groupé, la profondeur stricte hors shell, le centre exact, l'owner diamétral et la décision de domaine. Une ambiguïté reste fail-open ou donne `unsupported_degeneracy`.

## 4. Preuves et mesures à exiger avant du code v3 autoritaire

### Gate A — autorité documentaire

- reprendre A2e, le lemme $2r$ et les profils d'entrée dans la spécification et le registre;
- classer chaque énoncé `proved_here`, `conditional_theorem`, `proof_obligation` ou `experimental_target`;
- laisser ce rapport au statut d'audit, jamais d'autorité.

### Gate B — reçus de l'investigation

- sidecars complets pour le tableau de temps, le catalogue, la forêt, GMP et le census;
- digests des inputs, sources, binaires et sorties;
- identité de campagne fermée et aucune omission silencieuse;
- distinction stricte entre mesure, extrapolation et théorème.

### Gate C — A1-source

- partition exacte de $\binom{n}{2}$ sans tableau de paires;
- reçus rationnels de chaque prune à $n=32$;
- support trois et quatre séparés;
- métriques $Q$, $V_W$, $a$, $M$, tails et high-water à 50 k;
- no-go si la majorité atteint les microtuiles ou si la queue sérialise.

### Gate D — A2e CPU

- constructeur shallow sans travail en $\sum_e m_e^2$;
- comparaison exhaustive jusqu'à $n\leq14$ et locale brute par ancre;
- permutations, égalités, parallèles, concurrences et limites de l'ellipse;
- transcript complet et sortie byte-à-byte canonique.

### Gate E — descente indexée

- fixture permanente du lemme $\bar B(c,2r)$;
- différentiel scan global contre range-query;
- terminaison, baisse stricte, victime canonique et plateaux;
- compteurs par ordre, pas extrapolation depuis le total catalogue;
- raccord aux incidences silencieuses et lots exacts.

### Gate F — pipeline HGP

- sink streamé consommé par le réducteur;
- facettes, cofaces, silences, attaches, `coverage_log` et lots égaux;
- verticales et carrés de naturalité;
- une seule publication canonique terminale;
- `public_status=not_claimed` jusqu'à fermeture de toutes ces pièces.

## 5. Corrections précises recommandées à `PROPOSITION.md`

1. Remplacer « l'audit est l'autorité » par l'autorité spécification + registre.
2. Lier chaque nouvelle mesure à un sidecar; marquer les chiffres sans reçu `diagnostic_only`.
3. Supprimer l'implication minorant échantillonné $\Rightarrow$ majorant convexe.
4. Remplacer « alarme surface levée » par « première observation bunny, transfert ouvert ».
5. Corriger les temps avec le débit réellement publié ou publier un intervalle.
6. Remplacer « $W\geq130$ géométriquement nécessaire » par « observé nécessaire sur le corpus mesuré ».
7. Conserver le lemme $2r$, mais séparer preuve locale, coût mesuré et correction globale de la descente.
8. Retirer « `std::map` sur 23 millions de clés » jusqu'à un histogramme par ordre; le code n'insère que les minima de rang $k$.
9. Garder le streaming comme invariant, mais classer 4,4 Go comme estimation diagnostique.
10. Séparer les obligations A2e des propriétés M1 à M5 propres à A2p.
11. Définir `RelevantGP` par référence normative complète.
12. Remplacer « `sphere.hpp` sain » par « candidat, largeur symbolique et différentiel à fermer par prédicat ».
13. Ajouter au gate A2e les supports deux et trois, les concurrences et le transcript de conflits.
14. Faire du census bunny une fixture reproductible, puis ajouter SemanticKITTI et les familles sanctionnées.

## 6. Décision finale

La révision `901e801` a accepté le bon changement conceptuel : **l'arête diamétrale ne sert pas seulement à borner le rayon; elle réduit la dimension et transforme le rang en profondeur d'arrangement**. C'est une proposition sérieuse et probablement la meilleure voie actuellement visible pour une v3 allégée.

La décision est donc :

- **GO** pour formaliser A2e, construire son oracle CPU et mesurer A1-source;
- **GO** pour prototyper tôt les masses et queues sur GPU sous statut `proposal_only`;
- **NO-GO** pour appeler les nouvelles mesures des preuves ou des qualifications;
- **NO-GO** pour commencer un produit v3 avant source d'ancres complète, constructeur shallow non quadratique et source HGP aval;
- **NO-GO** pour tout claim exact, SLO ou autorité publique.

Le prochain livrable utile n'est pas un nouveau catalogue. C'est un **constructeur shallow CPU exact et transcriptable**, confronté à un oracle exhaustif, pendant que le center-cover mesure si les ancres et les lignes restent effectivement sparse sur les familles réelles.

## 7. Références déterminantes

- [`PROPOSITION.md`](PROPOSITION.md), révision auditée.
- [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md), premier audit de la cascade A1.
- [`census_tukey_shallow_20260808.json`](census_tukey_shallow_20260808.json), résultat diagnostique à rendre reproductible.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md), théorèmes A2e et gates center-cover/shallow.
- [`GERMINATION_LOCALE_SUPPORTS_3_4.md`](../docs/math/GERMINATION_LOCALE_SUPPORTS_3_4.md), distinction entre tangente et $R$ convexe.
- [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md), domaine exact, `RelevantGP`, objet HGP et verticales.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md), seule autorité de statut des preuves avec la spécification.
- [`WARNING_AUDIT_PUBLICATION_3.md`](../morsehgp3D_v2/WARNING_AUDIT_PUBLICATION_3.md), limites de la v2 et d'O2.

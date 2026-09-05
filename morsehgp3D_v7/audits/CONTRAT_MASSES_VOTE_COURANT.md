# Masses d'incidence et vote : contrat du manuscrit et raccord v7

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Revue statique et arithmétique Python légère pendant la fenêtre de chronométrage E/F ; aucun moteur compilé, benchmark ou service externe utilisé.

**Les poids du §9.1 sont constructibles sans mosaïque globale, mais ne se déduisent pas des seules composantes horizontales.** Il faut fixer les univers de facettes et de cofaces, puis conserver leur mesure d'incidence. Le helper `build_render(events)` fournit déjà la forme d'agrégation nécessaire pour les événements effectivement reçus ; son raccord au pipeline et à l'archive reste à construire. Une masse calculée sur le sous-flot v7 est une mesure définie exactement sur ce sous-flot. Son égalité avec celle d'un catalogue complet Gabriel ou Čech demande une preuve supplémentaire, et elle est fausse pour une réduction de connectivité arbitraire.

## 1. Source exacte et choix opérationnel des univers

Le [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), hash `579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`, donne :

- §9.1, page imprimée 96, PDF 122 : l'objet naturel est un recouvrement des points et une partition des facettes ; F est l'ensemble des facettes « effectivement construits par l'algorithme ». Il définit le score local par somme sur les cofaces, puis la normalisation par point.
- Pages imprimées 96–97, PDF 122–123 : la masse des facettes est utilisée pour `min_cluster_size`, avec la convention `1/T_x=0` lorsque `T_x=0`. La masse pertinente ne compte donc ni les feuilles brutes ni leurs points avec multiplicité entière.
- Proposition 7, page imprimée 97, PDF 123 : une sélection de clusters sur les facettes et une règle déterministe de départage donnent une partition stricte, comprenant le bruit. À K1 le vote restitue les étiquettes du single-linkage.
- Algorithme 1, page imprimée 100, PDF 126 : pour K2, extraire les triangles Gabriel, prendre toutes leurs arêtes-facettes, agréger les scores à la ligne 5, puis extraire l'arbre couvrant à la ligne 6. L'agrégation précède donc la suppression des cofaces redondantes pour la connectivité.

Le qualificatif « Gabriel » utilisé pour F dans le paragraphe du §9.1 ne doit pas remplacer la règle opérationnelle de l'Algorithme 1. Sur les trois points `(0,0,0),(4,0,0),(1,1,0)`, le triangle est Gabriel ; son grand côté ne l'est pas, car le troisième point est strictement intérieur à son disque diamétral. Ce côté est pourtant une facette retenue par la ligne 3. Le [reçu rationnel](receipts_vertical_20260905/masses/normal.json) conserve cette clarification minimale : centre `(2,0,0)`, rayon carré 4, puissance du troisième point −2. Le contrat proposé prend **toutes les facettes des cofaces déclarées, attachements inclus** ; il ne filtre pas F par une seconde propriété Gabriel.

Un manifeste de mesure doit donc fixer séparément les objets suivants.

| Champ | Sens contractuel |
| --- | --- |
| `facet_universe` | F : ensemble des K-parties réellement pondérées, avec PointId et ordre K déclarés |
| `coface_universe` | C : ensemble des (K+1)-parties contribuant aux scores ; par exemple catalogue Gabriel complet de l'Algorithme 1, catalogue Čech complet, ou sous-flot v7 explicitement identifié |
| `incidence_authority` | Certificat de complétude de C ou identité exacte du sous-flot ; ce champ ne peut être remplacé par un digest des deltas |
| `coface_identity` | Ensemble canonique des PointId ; plusieurs supports décrivant la même coface ne sont pas plusieurs contributions |
| `weight_profile` | Fonction ψ, exposant, rayon utilisé, unité et autorité numérique |
| `weight_scope` | Ordre et univers globaux figés, ou fenêtre/coupe explicitement figée ; ouverture/fermeture de la borne incluse |
| `zero_policy`, `noise_policy`, `tie_policy` | Conventions décrites ci-dessous, indépendantes du stockage classic/CSR |

Le profil opérationnel le plus direct est F = ∂C. Un univers F plus petit est possible, mais change le score disponible, le support des points et certaines simplifications de calcul. Le mot « complet » est toujours relatif à C : le manuscrit ne définit pas automatiquement C comme toutes les (K+1)-parties de X. En particulier, « complet Gabriel » et « complet Čech » sont deux contrats différents.

## 2. Mesure exacte, support et identité utile de calcul

Pour un univers figé et une fonction positive et finie sur ses rayons, poser :

$$S_\tau=\sum_{\substack{\sigma\in C\\\tau\subset\sigma}}\psi\bigl(\rho(\sigma)\bigr),\qquad T_x=\sum_{\substack{\tau\in F\\x\in\tau}}S_\tau,\qquad w_{x\tau}=\begin{cases}S_\tau/T_x&x\in\tau,\ T_x>0,\\0&\text{sinon.}\end{cases}$$

$$m_\tau=\sum_{x\in\tau}w_{x\tau}=S_\tau\sum_{x\in\tau}\frac{1}{T_x},\qquad X_+=\lbrace x\in X:T_x>0\rbrace.$$

La convention sur `1/T_x` est celle du manuscrit. Une facette de ∂C a un score strictement positif lorsque ψ est positive. Si une interface permet des facettes sans coface contributrice ou une ψ qui s'annule, elle doit annoncer les scores nuls ; « point incident » n'implique alors plus `T_x>0`.

En échangeant deux sommes finies :

$$\sum_{\tau\in F}w_{x\tau}=\mathbf{1}_{x\in X_+},\qquad \sum_{\tau\in F}m_\tau=|X_+|.$$

**Simplification constructive lorsque F = ∂C.** Chaque coface contenant x possède exactement K facettes contenant x. Par conséquent :

$$T_x=K\sum_{\substack{\sigma\in C\\x\in\sigma}}\psi\bigl(\rho(\sigma)\bigr).$$

T peut ainsi être accumulé directement par les points de chaque coface, sans construire une seconde structure globale point–facettes. Les scores S restent nécessaires aux masses et aux votes par facette. La fixture vérifie cette identité sur 21 couples catalogue/point ; une restriction supplémentaire de F retire cette simplification.

À K1, chaque feuille est le singleton d'un point : son poids normalisé vaut 1 dès qu'un score positif est défini, quel que soit ce score. Pour inclure les racines normatives sans coface, le contrat industriel peut fixer directement leur masse à 1 et restituer leur étiquette ; il doit nommer cette convention K1, au lieu d'inventer une division `0/0`. Sur le domaine actuel n≥2 avec graphe Gabriel complet, chaque point appartient à une arête de l'EMST, donc reçoit déjà un score positif.

## 3. Antichaînes : conservation et réserve explicites

Un nœud A de l'arbre représente un ensemble L(A) de feuilles-facettes. Son poids est additif :

$$M(A)=\sum_{\tau\in L(A)}m_\tau,\qquad W_A(x)=\sum_{\substack{\tau\in L(A)\\x\in\tau}}w_{x\tau}.$$

Les ensembles de feuilles de nœuds incomparables sont disjoints. Pour une antichaîne H :

$$\sum_{A\in H}M(A)=\sum_{\tau\in\bigcup_{A\in H}L(A)}m_\tau\leq|X_+|.$$

L'égalité exige que l'antichaîne couvre toutes les feuilles de masse positive. Pour une antichaîne partielle, le complément reste une réserve, du bruit ou des feuilles non encore matérialisées. La partition de l'unité ne prouve pas que **toute** antichaîne a masse n. Le comptage des points d'un nœud est également distinct de M(A), parce qu'un point peut partager sa masse entre plusieurs nœuds.

Cette distinction concerne directement `born`. Le [certificat horizontal](CERTIFICAT_HORIZONTAL_COURANT.md) garantit une première matérialisation du sous-flot, pas la présence de toutes les feuilles d'un univers pondéré global dès la première coupe. Un consommateur peut conserver une réserve pour les feuilles futures, puis transférer exactement leur masse à leur matérialisation. La mise à jour d'un delta est alors la somme des masses des parents et des nouvelles feuilles ; elle ne recompte pas les points du carrier.

Une autre convention consiste à recalculer T sur F limité à chaque coupe. Elle fournit une normalisation locale, mais modifie les poids de feuilles anciennes à mesure que F grandit. Ce n'est plus une mesure fixe transportée par l'arbre. Il faut versionner cette convention et recalculer les termes concernés, au lieu de supposer l'additivité d'une mesure préexistante.

## 4. Information réellement disponible dans le payload v7

| Objet reçu | Reconstruction justifiée |
| --- | --- |
| `ForestResult` seul | Catalogue des facettes retenues, deltas, premières matérialisations et composantes de leur sous-flot ; aucune liste des cofaces incidentes ni de leurs multiplicité par facette |
| `on_forest(K, events, result)` | Identités et niveaux de toutes les cofaces de la liste `events`, pendant la vie du callback ; une agrégation de ce sous-flot est possible avant sa libération |
| `RenderResult` construit sur une liste certifiée | Histogramme par facette et niveau, avec multiplicité des enregistrements contributifs ; suffisant pour toute ψ fixée ultérieurement |
| Certificat horizontal E | Bijection des composantes et conservation des points couverts ; aucune conservation automatique de S, T ou w entre catalogues de cofaces différents |

Le [helper de rendu](../src/forest/render.hpp) développe les K+1 facettes de chaque événement, trie les couples facette/niveau et somme les multiplicités. C'est précisément le bon agrégat pour les incidences du **multiensemble fourni**. Pour des scores sur un ensemble de cofaces, la liste doit être sans doublon sémantique, ou les doublons doivent être éliminés avant cette agrégation. Des cofaces différentes à la même boule et au même niveau contribuent chacune ; réduire leur multiplicité à un seul exemplaire perd de la masse.

Le helper n'appelle pas la validation structurelle du fold et ne constitue pas à lui seul une porte transactionnelle de capacité. Son utilisation industrielle doit passer par une liste validée, un majorant des incidences et le budget du supplément de rendu. Aucun appel à `build_render` n'est actuellement présent dans `src/` hors de sa définition, ni dans `cli/` ou `tests/` consultés. Le présent audit est une lecture statique de ce helper, pas sa qualification compilée.

La [route de pipeline](../src/pipeline/run.hpp) ajoute les cofaces silencieuses avant le callback normalisé. `ForestEvent` ne conserve pas un champ d'origine direct/silencieux. `build_render(events)` dans ce callback définirait donc la mesure du sous-flot **direct + chaînes**. Pour retrouver le profil Gabriel standard, il faut agréger les directes avant l'ajout des chaînes, ou disposer d'une classification certifiée ; ignorer cette distinction change C. Le supplément doit rester provisoire tant que le pipeline n'a pas terminé avec son statut autorisé, comme les forêts elles-mêmes.

Le histogramme utile est :

$$H_{\tau,\lambda}=\#\lbrace\sigma\in C:\tau\subset\sigma,\ \rho(\sigma)^2=\lambda\rbrace,\qquad S_\tau=\sum_\lambda H_{\tau,\lambda}\,\psi\bigl(\sqrt{\lambda}\bigr).$$

Cet histogramme peut remplacer les cofaces pour **ces** scores, sans conserver toutes les adjacences de Gamma. À ψ définitivement fixé, conserver S par facette peut suffire. Modifier ψ ou la fenêtre après avoir jeté l'histogramme exige une nouvelle agrégation. Une preuve analytique d'agrégation des cofaces omises pourrait aussi suffire ; aucune telle preuve n'est apportée par le seul certificat horizontal.

## 5. Contre-fixture permanente : mêmes tokens, votes opposés

Le [juge rationnel](masses_vote_probe.py) utilise les sept positions u16 suivantes, qui forment deux tétraèdres réguliers partageant le point 1 :

| PointId | Position |
| --- | --- |
| 0 | `(2,4,4)` |
| 1 | `(2,2,2)` |
| 2 | `(4,2,4)` |
| 3 | `(4,4,2)` |
| 4 | `(2,0,0)` |
| 5 | `(0,2,0)` |
| 6 | `(0,0,2)` |

Il calcule les MEB des 35 triangles par boules de paires et cercles circonscrits affines en fractions rationnelles. Au premier niveau `λ=8/3`, exactement huit triangles sont présents : les quatre faces de chaque tétraèdre. Leur centre est le barycentre, les coefficients valent 1/3, et tous les points étrangers sont strictement extérieurs à leur boule : ces huit cofaces sont Gabriel régulières. Tout triangle mêlant les deux tétraèdres hors du point partagé possède une paire de distance carrée au moins 24, donc naît après ce premier niveau.

Comparer C complet Čech, C privé de `{1,2,3}`, et C privé de `{1,5,6}`. Chacune des cofaces retirées est redondante dès son lot : les trois autres faces du même tétraèdre connectent déjà ses six arêtes. Les unions et toutes les coupes ultérieures restent donc identiques. Les deux sous-flots ont chacun **34 cofaces et 21 facettes**, et ont exactement les mêmes tokens normalisés, niveaux et partitions que le catalogue de 35 cofaces. La fixture compare les deux côtés de chacun des quatre niveaux.

Fixer la sélection de clusters au premier niveau : les six facettes du premier tétraèdre reçoivent le label 1, celles du second le label 0, et les facettes futures le bruit −1. Les deux clusters partagent le point 1. Pour le profil explicitement rationnel `ψ(ρ)=ρ^-2`, le vote de ce point vaut :

| Catalogue | V(0) | V(1) | Gagnant |
| --- | --- | --- | --- |
| Čech complet | 1/2 | 1/2 | 0, par départage fixé |
| Sans `{1,2,3}` | 29/52 | 23/52 | 0, strict |
| Sans `{1,5,6}` | 23/52 | 29/52 | 1, strict |

**Il n'existe donc pas de reconstruction des masses de la liste d'événements depuis ses seuls tokens horizontaux**, même si l'on connaît ses cardinalités et son catalogue de facettes. La conclusion reste relative à ce payload : X et une définition complète de C permettent évidemment une recomputation géométrique indépendante. La fixture ne prétend pas que ces sous-flots sont les sorties effectives de la v7, ni que le nuage passe tous les contrôles de régularité E aux niveaux ultérieurs. Elle réfute le transfert algébrique « même H0, donc mêmes poids ». Au premier niveau, la scène donne déjà la même perte pour les huit cofaces Gabriel régulières de cette fenêtre déclarée.

La même scène distingue couverture et masse. Les deux composantes du premier niveau couvrent les sept points, avec comptages bruts `4+4=8`. Avec les atomes fixés sur le F global de 21 facettes, leur masse vaut `320/67` ; les neuf facettes futures portent la réserve `149/67`, et la somme vaut 7. Renormaliser sur les seules douze facettes de la coupe change déjà le poids de `{0,1}` au point 0, de `29/134` à `87/253`.

Les [reçus normal](receipts_vertical_20260905/masses/normal.json) et [Python optimisé](receipts_vertical_20260905/masses/optimized.json) donnent les mêmes résultats. Ils incluent un point sans score, le départage exact, l'égalité entre calcul direct et histogramme et quatre **corruptions d'audit**, sans les présenter comme des mutants produit exécutés : suppression des multiplicités, double comptage des points, masse n imposée à une antichaîne partielle et confusion entre rayon et rayon carré. Le point auxiliaire 99 vérifie seulement la convention de score nul ; il ne fait pas partie du nuage géométrique à sept points. Les portes sont des exceptions explicites, jamais des `assert`.

Rejeu depuis la racine, sans compilation :

```bash
python3 morsehgp3D_v7/audits/masses_vote_probe.py --receipt normal
python3 -O morsehgp3D_v7/audits/masses_vote_probe.py --receipt optimized
```

## 6. Rayon, domaine numérique et décisions de vote

Le niveau moteur est `λ=ρ²`. Pour l'exposant p du profil :

$$\psi(\rho)=\rho^{-p}=\lambda^{-p/2}.$$

Le choix p=3 en dimension trois donne `λ^(-3/2)`, et **pas** `λ^-3`. Un profil uniforme p=0 et le profil p=2 utilisé dans la fixture sont des choix distincts autorisés par la famille du manuscrit ; ils ne remplacent pas silencieusement son choix de densité.

Dans le domaine u16 distinct et pour des cofaces d'au moins deux points, les rayons sont strictement positifs. La borne inférieure vient d'une paire de points distincts, la supérieure de la boule circonscrite au cube :

$$\frac{1}{4}\leq\lambda\leq\frac{3\cdot65535^2}{4}.$$

Ainsi ψ est positive et finie ; à p=3, chaque contribution vaut au plus 8. Pour F = ∂C, `S_tau≤8 deg_C(tau)`, `T_x≤8K deg_C(x)` et `0≤m_tau≤K`. Ces bornes de grandeur ne bornent pas la taille des dénominateurs de sommes rationnelles, ni la complexité d'expressions algébriques. Aucun entier fixe 128 bits n'est certifié pour des scores agrégés par ce seul argument.

À exposant entier pair, les contributions sont rationnelles lorsque λ est rationnel ; une référence arbitraire précision suffit mathématiquement, avec ses coûts en bits à mesurer. À p=3, des racines apparaissent. Il faut alors déclarer une autorité symbolique algébrique, ou des intervalles certifiés avec une procédure exacte pour égalités et seuils non séparés. Un flottant plausible et un epsilon ne prouvent pas la règle de départage de la Proposition 7. Une fonction ψ arbitraire ne promet pas automatiquement une procédure de comparaison exacte.

Pour les numérateurs de vote p3, cette obligation dispose désormais d'une [méthode constructive et d'un juge borné](AUTORITE_VOTE_P3_COURANTE.md) : égalités décidées exactement par classes de carrés rationnels sans factorisation, puis signe certifié par intervalles `isqrt`, ou statut indécis sur plafond. Le raccord produit et les expressions contenant les quotients de masses restent distincts.

Pour un point de score total positif, le dénominateur T est commun aux classes. Le vote dur peut comparer les numérateurs :

$$V_x(c)=\frac{\sum_{\substack{\tau\in F\\x\in\tau,\ \ell(\tau)=c}}S_\tau}{T_x},\qquad \mathop{\mathrm{argmax}}_c V_x(c)=\mathop{\mathrm{argmax}}_c\sum_{\substack{\tau\in F\\x\in\tau,\ \ell(\tau)=c}}S_\tau.$$

Cette économie de division n'élimine pas les comparaisons de sommes algébriques. Les masses utilisées par `min_cluster_size`, les scores de condensation et les probabilités interpolées exigent leur propre contrat numérique.

Le contrat proposé fixe : points de `T_x=0` vers −1 ; bruit comme une classe votante ; égalités positives départagées par une clé de classe stable explicitement déclarée. Le cas où bruit et classes positifs coexistent n'est pas entièrement spécifié par les seules formules du manuscrit : traiter le bruit comme abstention est une autre politique possible, à nommer. Pour une sortie probabiliste, la masse absente à `T_x=0` peut être affectée à un canal bruit ; cela ne crée pas une masse fictive sur les facettes.

Pour des distributions par facette, le vote souple est la combinaison `p(x)=Σ_tau w_xτ p_tau`, qui conserve la somme 1 sur X positif si les distributions et leur domaine sont normalisés. Le durcissement doit rapporter la fraction des points ayant plusieurs labels de masse positive et, séparément, la masse abandonnée `1−max_c V_x(c)`. La Proposition 7 garantit une partition déterministe ; elle ne garantit pas que ce choix conserve toutes les appartenances du recouvrement.

## 7. Raccord industriel et coût à conserver

Pour M cofaces uniques, F facettes et `I=(K+1)M` incidences, une passe sur les cofaces peut alimenter les scores par facette et les totaux par point. Le helper actuel matérialise I enregistrements et les trie : coût de comparaison `O(I log I)` et mémoire `O(I)`, avant les histogrammes. Une réduction par clé en flux ou un tri externe permettent de plafonner la mémoire, sans construire les cellules de la mosaïque ni l'adjacence complète de Gamma. Ils ne suppriment pas l'obligation d'énumérer ou de sommer exactement les contributions de C.

Pour ψ figé, le stockage numérique minimal est un score par facette et un total par point, en plus des clés/incidences nécessaires au vote ; pour ψ modifiable, stocker H couples facette/niveau avec `H≤I`. Calculer les masses puis les votes parcourt `K|F|` incidences point–facette. Les agrégations d'un arbre laminaire sont des sommes sur ses feuilles ; le coût en bits des poids exacts et le nombre de termes algébriques restent des dimensions distinctes du comptage des opérations.

La prochaine étape concrète est un supplément d'archive de mesure : F/C/ψ épinglés, histogrammes ou scores, T ou formule de recalcul, conventions de bruit/égalité, totaux de non-vacuité et statut terminal. Un nouveau fichier demande une version et un manifeste correspondants, un ajout à l'inventaire fermé de nettoyage de l'archive et des budgets vérifiés avant allocation. Les callbacks restent provisoires jusqu'au terminal. Pour le sous-flot actuel, conserver les contributions avant libération des événements suffit ; ceux-ci restent accessibles au callback après le fold, et aucune nouvelle mosaïque n'est nécessaire. Pour une autorité Gabriel complète, séparer directes et chaînes. Pour une autorité Čech complète, il faut un générateur ou une formule d'agrégation supplémentaire ; le juge exhaustif à sept points de cet audit ne devient pas cette architecture.

Un pushforward vertical d'une **mesure source fixée** conserve sa masse par sommes sur les fibres. Recalculer indépendamment S et T à l'ordre suivant ne définit pas ce pushforward et ne commute pas automatiquement avec la [flèche verticale](CONTRAT_VERTICAL_COURANT.md). Cette obligation de mesure reste distincte du certificat topologique.

Les objections « il faudrait inventer une partition de l'unité » et « le recouvrement impose des masses non additives » sont levées par les formules du manuscrit. Restent à réaliser le supplément de mesure effectivement sérialisé, son autorité de cofaces, le profil numérique et ses portes de décision. Aucun changement du statut public ni du produit n'est réalisé ici. GCP non utilisé.

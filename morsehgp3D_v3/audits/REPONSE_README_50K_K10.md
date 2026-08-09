# Réponse à la question du README : 50 000 points, ordre 10, moins d'une seconde

> [!IMPORTANT]
> Cette réponse audite la question introduite au commit `7094d04cafb592dcf891b4cba8a92a2b5b7cb293`, dans un `README.md` de SHA-256 `e33f1020026ab091ad00105bac6acd6bd2cbbfbdb3aca1f7614e66301573a86a`. Le code a continué d'évoluer pendant l'audit; aucune campagne finie ne constitue une preuve universelle. Autorités : `docs/SPECIFICATION_MORSEHGP3D.md`, registre des preuves et portes du plan de test.

## Réponse courte

**Le chemin actuel n'atteint pas le contrat.** Le prototype `edge_shallow` est un falsificateur dense en $\Theta(n^5)$ au pire, la source complète d'ancres A1 reste ouverte, les volumes $M=\sum_e m_e$, $Z=\sum_e Z_e$ et aval ne sont pas mesurés à 50 k, et le tri exact ainsi que la source HGP complète n'existent pas encore dans ce chemin.

**Le contrat sous une seconde n'est pas réfuté pour le p95 de familles sanctionnées et effectivement sparse.** Il reste donc un objectif de recherche conditionnel. Il n'est ni « plausible » ni « acquis » sur les preuves actuelles : aucun ledger ne permet encore de choisir entre ces deux mots.

**Une garantie sous une seconde pour tout nuage de 50 000 points est exclue du contrat lui-même.** La correction doit être inconditionnelle, mais la latence est data-dépendante; sur un cas dense, la version sans budget doit être lente ou échouer sur une ressource réelle, jamais censurer silencieusement. Le SLO porte sur le p95 de chaque famille enregistrée.

| question | verdict au 9 août 2026 |
| --- | --- |
| chemin `edge_shallow` actuel à 50 k | **NO-GO catégorique** |
| architecture A2e avec A1 sparse et vrai shallow | **possible sous conditions, non démontrée** |
| une seconde, `quantized_u16_input` | **objectif expérimental mesurable** |
| une seconde, `exact_dyadic_input` | **inconnu et plus difficile; taux de repli multiprécision manquant** |
| 100 ms bout en bout | **non étayé; ne pas le planifier avant la fermeture sous une seconde** |
| moins d'une seconde sur tout nuage | **hors contrat et non garanti** |

## 1. Il faut d'abord poser le vrai contrat

Le README abrège trop la question en « points réels ». Le contrat normatif exige simultanément :

- entrées binary64 interprétées comme dyadiques exacts pour le profil produit; `quantized_u16_input` est un profil distinct qui ne certifie pas les points originaux;
- version industrielle **sans budget configuré**;
- $K_{\max}=10$ et vrai payload source : facettes, cofaces ou flux sparse complet certifié, incidences actives et silencieuses, lots de niveaux exactement égaux, `coverage_log`, forêts horizontales et applications verticales;
- condensation `min_cluster_size=20, relation=at_least` seulement **après** la source exacte; elle n'autorise aucun élagage scientifique amont;
- chrono `warm_e2e` depuis les coordonnées brutes en mémoire jusqu'aux sorties canoniques matérialisées, validation et transferts inclus;
- deux warmups puis dix nuages frais pour chacune des familles sanctionnées, p95 par famille, matériel et toolchain figés, pic sous 80 % de VRAM;
- moins d'une seconde comme porte secondaire de progression; moins de 100 ms reste la cible principale et n'est pas fermé par le succès secondaire.

Un chrono sur le seul catalogue, sur `resident_core`, sur la grille u16, sur une sortie censurée ou sans verticales ne répond donc pas à la question.

## 2. Pourquoi l'implémentation courante est hors cible

Pour $n=50,000$, le prototype M2.2 parcourt les $\binom{n}{2}=1,249,975,000$ ancres. Il classifie jusqu'à $n$ points par ancre, forme toutes les $\binom{m_e}{2}$ intersections, puis rescane jusqu'à $m_e$ formes par intersection. Dans le cas dense $m_e\simeq n$, cela représente environ $6{,}25\cdot10^{13}$ classifications paire--point, $1{,}56\cdot10^{18}$ intersections et jusqu'à $7{,}8\cdot10^{22}$ tests de profondeur. Le catalogue live réénumère en plus les autres arités. Le prototype isole utilement le dictionnaire; il n'est pas un algorithme de production.

Le seul test borné M2.2 du snapshot initial est réellement non vacue : 924 arêtes, 3 996 sommets examinés, 420 shallow, 66 émissions d'arité quatre et 9 432 tests de profondeur. Il porte toutefois sur 20 nuages de 8 à 12 points et ne mesure aucune loi d'échelle.

Un autre diagnostic de cette session est plus sévère : la voie P15-HOCUDA-P1a `center-cover` a compilé et passé l'oracle natif à $n=32$, puis son unique run G4 `uniform_latin` à 50 k a atteint le coupe-circuit de 600 s sans JSON; `eight_clusters` a été interrompu à la demande de l'utilisateur. Ce n'est pas un reçu de performance qualifiant, mais cela réfute sans ambiguïté l'implémentation A1 actuelle comme route sous une seconde : la source seule est déjà à plus de 600 fois le budget secondaire.

## 3. Les conditions mathématiques et algorithmiques minimales

Posons :

- $E$ : nombre d'ancres canoniques sorties par A1;
- $M=\sum_e m_e$ : formes actives effectivement range-reportées;
- $Z=\sum_e Z_e$ : strates shallow avant déduplication inter-ancre;
- $P$ : nombre de prédicats exacts et de replis;
- $N$ : records géométriques et HGP canoniques, incidences silencieuses comprises;
- $C$ : comparaisons exactes ou travail de tri équivalent;
- $B$ : octets lus, écrits et déplacés sur le chemin critique;
- $D$ : visites de descente, attaches, opérations de réduction et verticales.

Le ledger à fermer est $T_{\mathrm{e2e}}=T_{\mathrm{canon}}+T_{\mathrm{index}}+T_{A1}+T_{\mathrm{shallow}}+T_{\mathrm{exact}}+T_{\mathrm{sort}}+T_{\mathrm{HGP}}+T_{\mathrm{reduce}}+T_{\mathrm{out}}$. Les compteurs doivent en outre satisfaire, avec les débits **mesurés sur la machine cible**, une inégalité du type $M/r_{\mathrm{class}}+Z/r_{\mathrm{walk}}+P/r_{\mathrm{pred}}+C/r_{\mathrm{sort}}+B/b_{\mathrm{mem}}+D/r_{\mathrm{aval}}+T_{\mathrm{vertical}}+T_{\mathrm{sync}}<1\ mathrm{s}$. Ce n'est pas un modèle de preuve; c'est la comptabilité minimale qui empêche un poste caché de disparaître dans une moyenne.

### 3.1 A1-source

Il faut une source d'ancres :

- complète avec certificat d'exclusion rejouable et comportement fail-open;
- proche de $O(n\log n+E+V_{A1})$ sur les familles sanctionnées, où $V_{A1}$ compte les visites réelles de l'index;
- sans scan des $\binom{n}{2}$ paires ni matrice paire--point;
- avec histogrammes des visites, paires proposées, paires retenues, doublons, covers ambigus et lourdes queues.

Le RNG d'ordre borné est exclu comme autorité. Le self-join LBVH avec center-cover reste une proposition mathématiquement pertinente, mais son ordonnance actuelle est rejetée par le timeout 50 k. A2p ne constitue pas un raccourci produit tant qu'il insère les $n-1$ plans pour chacune des $n$ ancres : cela fait déjà 2 499 950 000 incidences plan--ancre avant les strates.

### 3.2 Vrai constructeur shallow

Pour $s_{\max}=11$, la borne conditionnelle donne $\kappa_e=7-c_e$ et $Z_e\leq m_e(8-c_e)$, donc $Z\leq8M$. Cette borne n'aide le temps que si le constructeur évite de matérialiser toutes les intersections. La cible d'acceptation est un parcours prouvé et mesuré proche de $O\left(\sum_e(m_e\log m_e+Z_e)\right)$, ou meilleur pour $K$ constant, avec clipping, conflits, parallèles, concurrences, shell, centrage et owner exacts. Un terme $\sum_e m_e^2$ est un no-go, même si la sortie finale est sparse.

Les statistiques décisives sont les **sommes** $M$ et $Z$, leurs maxima par ancre et leurs queues, pas seulement une moyenne. Le voisinage maximal de 25 026 points déjà observé sur `eight_clusters` impose une file de classes de charge et empêche toute hypothèse « quelques dizaines de points par CTA ».

### 3.3 Les quatre arités

Les arités un à quatre doivent avoir leurs propres régions, seuils, fixtures et compteurs. Fermer l'arité quatre ne paie pas l'arité trois; support deux exige encore shell complet et dix intérieurs suffisent à le mettre hors rang onze. Une implémentation produit doit partager les formes et l'index sans réexécuter quatre cascades exhaustives.

### 3.4 Tri, source HGP et aval

Les ancres n'émettent pas dans l'ordre global. Il faut donc :

- une clé de niveau canonique compacte, un filtre de tri par bits discriminants et une comparaison exacte sur les collisions ou intervalles ambigus;
- des runs bornés, fusionnés déterministiquement, qui réunissent **tous** les niveaux rationnellement égaux avant mutation;
- une construction multi-ordre partagée, pas dix catalogues et dix tris indépendants;
- le ledger des cofaces, incidences silencieuses, `coverage_delta`, attaches, descentes et verticales;
- pour le chemin chaud 50 k, un tri résident RAM/VRAM tant que ses tailles le permettent. Un aller-retour disque substantiel rend la seconde très improbable; le tri externe durable concerne d'abord les profils massifs.

## 4. Avertissement de volume : plusieurs millions de records sont plausibles

Le census exhaustif M2.1 observe 167,7, 190,9 puis 210,3 sphères par point à $n=100,150,200$, et ce ratio croît encore. Le prolonger à plat à 50 k donnerait 10,515 millions de sphères; c'est une **illustration, pas une extrapolation autorisée**. Un ancien diagnostic v2 donnait 450 à 510 sphères par point, soit 22,5 à 25,5 millions, sans certification.

Le record public v2 `CriticalSphere` occupe 160 octets avant son pool de membres. Un scénario de 10,5 millions de records au format actuel approche donc 2 Go, et 22,5 à 25,5 millions approchent 4,3 à 4,9 Go avec un payload moyen modeste. Ce n'est pas une taille minimale du futur flux compact, mais cela interdit de considérer matérialisation, tri et mémoire comme gratuits.

À 10,5 millions de clés, un tri comparatif générique demande environ 245 millions de comparaisons; à 51 millions d'événements bruts, environ 1,3 milliard. Aucun débit du comparateur rationnel exact n'est mesuré. Une clé filtrable et un tri radix ou fusion spécialisé sont donc une condition pratique, pas une optimisation tardive.

## 5. Réponse à l'obstacle de largeur de l'arité trois

La borne $Q<2^{136{,}4}$ du README est correcte **pour l'ancienne base orthogonale non primitive** $b_1=d\times e$ et $b_2=d\times b_1$. Avec $D=\lVert d\rVert$, $u=\lVert b_1\rVert$ et $X=2x-p-q$, on obtient $Q=D^2u^4\lVert X_{\perp}\rVert^2<D^6\lVert X\rVert^2<2^{136{,}34}$. Le saut de largeur est donc réel dans cette paramétrisation.

Il ne permet pas de conclure que « l'arité trois ne tient pas dans un `i128` » en général. Pour une base entière quelconque $B=[b_1\ b_2]$, $G=B^{\mathsf{T}}B$ et $\ell=B^{\mathsf{T}}X$, la quantité pertinente est $Q=\ell^{\mathsf{T}}\mathrm{adj}(G)\ell$. Une base équilibrée avec $b_1\times b_2=\pm r d$ donne $Q=r^2\lVert d\times X\rVert^2<2^{101{,}2}$ sur la grille u16. Une base du réseau primitif $\lbrace v\in\mathbb{Z}^3:d\cdot v=0\rbrace$, construite par Bézout ou forme de Hermite puis réduite en dimension deux, satisfait $b_1\times b_2=\pm d/g$ avec $g=\gcd(d_x,d_y,d_z)$, donc $Q=\lVert d\times X\rVert^2/g^2<2^{69{,}2}$.

Deux conclusions pratiques :

1. employer une base entière canonique, équilibrée ou primitive, et résoudre avec la matrice de Gram générale; la diagonalité n'est pas un avantage si elle multiplie les largeurs;
2. borner **chaque prédicat**, pas seulement $Q$. Les produits d'un test de profondeur ou du tri des niveaux peuvent être plus larges. Le chemin u16 raisonnable est un filtre `i128` avec détection d'overflow, suivi d'un entier fixe de 192 ou 256 bits; la comparaison publique des niveaux utilise déjà des produits plus larges. Le profil dyadique exact conserve un repli multiprécision.

Le code v2 confirme la distinction : `sphere3` et `sphere_side` annoncent des décisions sous environ 90 et 109 bits, tandis que `sphere_cmp_beta` emploie des entiers fixes de 256 puis 384 bits pour ordonner les niveaux. L'obstacle n'est donc pas « arité trois impossible », mais « mauvaise base et contrats de largeur mélangés ».

## 6. Expérience décisive avant tout nouveau claim de latence

### Étape A — census de croissance

Sur chaque famille sanctionnée et sur `eight_clusters`, exécuter $n=1,000,2,000,4,000,8,000,16,000,32,000,50,000$ et publier :

- $E$, visites A1 et prunes certifiées;
- $M$, $Z$, $c_e$, tailles maximales et quantiles par ancre;
- comptes par arité, doublons inter-ancre, shells et replis exacts;
- records actifs et silencieux, lots, verticales, octets et high-water;
- temps de chaque étage, y compris tri et synchronisations.

Un diagnostic interrompu est utile pour rejeter une voie; il ne qualifie rien. Une croissance superlinéaire des intermédiaires qui place leur seule borne de trafic au-delà d'une seconde est un **NO-GO d'architecture**, même si la sortie finale est petite.

### Étape B — microbenchmarks de toit

Mesurer sur le matériel cible les débits de classification, marche shallow, prédicats 128/256/multiprécision, comparaison de niveaux, tri/radix, déduplication et réduction. Injecter ces débits dans le ledger ci-dessus. Si un seul étage dépasse déjà une seconde au p95, le pipeline complet est rejeté sans benchmark intégral coûteux.

### Étape C — fermeture scientifique à petit n

Comparer chaque arité, le tri, les lots, les incidences silencieuses, la descente et les verticales à l'oracle multiprécision. Couvrir bases extrêmes, parallèles, concurrences, formes constantes, égalités de niveaux, shells, heavy tails et permutations. Le vert fini est une non-réfutation permanente, pas la preuve du théorème.

### Étape D — qualification bout en bout

Seulement si A à C ferment : G4 ou matériel produit exact, version sans budget, payload complet, deux warmups et dix nuages frais par famille. Fermer d'abord le p95 sous une seconde. Le passage à 100 ms exige ensuite un changement mesuré du chemin critique; il ne doit pas être obtenu en retirant une autorité ou une sortie du chrono.

## 7. Recommandation d'architecture

La voie la plus crédible reste **A2e**, mais seulement avec une nouvelle A1-source complète et sparse, un vrai constructeur de bas niveaux qui ne forme pas toutes les intersections, des prédicats filtrés à largeur fixe, un tri global résident et une source HGP multi-ordre complète. A2p est un excellent oracle structurel et peut proposer des ancres; sans certificat de localité, ses $n(n-1)$ plans en font un mauvais premier chemin sous une seconde.

Le CPU 48 cœurs doit pour l'instant rester juge, constructeur de fixtures et repli exact. Dire qu'une seconde CPU est « plausible » n'est pas encore falsifiable. Si le census montre $E$, $M$, $Z$ et le volume aval dans les dizaines de millions avec des records compacts et peu de replis, un pipeline hybride CPU--GPU peut devenir crédible. Si les centaines de millions ou les queues à 25 k dominent, le GPU de bout en bout devient nécessaire; si les intermédiaires restent quadratiques, l'architecture est rejetée quel que soit le GPU.

## Verdict final adressé au README

**Oui, la seconde peut rester une cible de recherche pour le p95 volumique sparse; non, on ne peut pas encore dire qu'elle est atteignable.** Les conditions décisives sont une A1-source complète proche de sa sortie, $M$ et $Z$ quasi linéaires avec petites queues, un constructeur shallow sans terme quadratique, des arités deux à quatre exactes et filtrées, un tri exact résident, puis un volume HGP aval compact et mesuré. Aujourd'hui, chacune de ces conditions est ouverte et l'unique source 50 k tentée a dépassé 600 secondes.

**La bonne prochaine action n'est pas d'estimer davantage.** C'est de construire le census $E,M,Z,P,N,C,B,D$ et le vrai shallow sur des tailles croissantes, puis de laisser l'inégalité du ledger rendre le verdict GO/NO-GO avant d'investir dans le pipeline complet.

GCP non utilisé pour cet audit; la session G4 citée est une observation antérieure déjà arrêtée et certifiée `TERMINATED`.

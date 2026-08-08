# Audit de `PROPOSITION.md` — voie recommandée pour MorseHGP3D v3

> [!IMPORTANT]
> Audit indépendant figé sur le commit `0c744c526e2cc3353c0acfa7c2b590a6cba8e5f8`, empreinte SHA-256 de [`PROPOSITION.md`](PROPOSITION.md) `b7c785ffbd743f9f7f083752aa204bc216da14d9a726682145208d3a1d2cbc2b`, 677 lignes, daté du 8 août 2026 à 21:48:33 UTC. Une modification ultérieure de la proposition exige un nouvel audit différentiel. Ce document ne modifie ni la proposition ni le code.

> [!CAUTION]
> **Verdict : NO-GO pour construire directement la v3 décrite au §11 de la proposition. GO conditionnel pour un programme v3 différent : source complète d'ancres diamétrales, puis peeling local bidimensionnel de niveaux peu profonds, puis source HGP et réduction résidentes.** Le nouveau lemme du §4.2 bis est juste et utile, mais il ne rend pas l'énumération de tuples sensible à la sortie. La recommandation A1 contre A2 pose une fausse alternative : le meilleur chemin prend l'ancrage diamétral de A1 et le cœur output-sensitive d'un A2 **ancré par arête**.

Contexte de cet audit : `phase=exploration_v3_hors_registre`, `backend=none_documentary_audit`, `profile=hgp_reduced_cible`, `mode=edge_anchored_shallow_peeling_architecture_audit_v1`, `public_status=not_claimed`. Aucune porte d'implémentation ou de performance n'est ouverte par ce texte.

## 1. Décision en une page

| option | décision | raison principale |
| --- | --- | --- |
| A1 telle qu'écrite, cascade puis tuples | **NO-GO produit** | la complétude sparse de la source d'ancres reste ouverte et le cœur conserve des centaines de millions de candidats |
| A2 ancré par point | **oracle/recherche** | route mathématiquement possible, mais coût global, duplication et structure 3D non bornés pour le produit |
| A1-source + A2e, peeling ancré par arête | **GO conditionnel** | le diamètre réduit le problème à un arrangement 2D dont le rang est une profondeur peu élevée |
| Delaunay d'ordre supérieur global | **NO-GO produit** | matérialise précisément la mosaïque globale interdite; admissible seulement comme oracle externe borné |
| reprise de la v2 comme autorité | **NO-GO** | oracles, forêt, domaine public et canonicité ne sont pas certifiés |
| cible 100 ms ou statut public exact | **fermés** | ni source complète, ni shallow builder, ni source HGP, ni pipeline aval qualifié |

La décision structurante devrait donc être réécrite ainsi :

> **V3 = source complète fail-open de paires diamétrales + arrangement shallow exact par paire + émission canonique des événements et incidences nécessaires + réduction HGP résidente.**

Ce chemin évite toujours de construire une mosaïque de Delaunay d'ordre supérieur, un atlas global de cellules, une liste globale de cliques ou une matrice paire--point. L'arrangement 2D n'existe que comme scratch local et tuilé pour une ancre.

## 2. Ce que la nouvelle proposition apporte réellement

La révision auditée est nettement meilleure que ses versions intermédiaires sur cinq points.

1. Elle identifie correctement l'échec combinatoire concret de la v2 et refuse de confondre épuisement du nuage avec certification par une borne.
2. Elle inscrit enfin la dépendance aux données dans le reçu de performance au lieu de supposer silencieusement un nuage volumique uniforme.
3. Elle reconnaît les acquisitions LiDAR multi-captations et refuse de faire d'une image de distance ou d'une forme étoilée une hypothèse de correction.
4. Le lemme du §4.2 bis, appliqué à chaque sommet du support, est une condition nécessaire valable.
5. Elle exige une arithmétique de production à largeur auditée et un oracle de représentation indépendante en précision arbitraire.

Ces progrès justifient de conserver la proposition comme note de recherche. Ils ne justifient pas encore sa recommandation produit, pour les verrous ci-dessous.

## 3. Findings bloquants

### 3.1 La « complétude A1 » est conditionnelle à la pièce la plus difficile

Jung et la cascade prouvent qu'un support accepté possède une paire diamétrale et confinent ses centres **une fois cette paire connue**. Ils ne donnent pas une énumération sparse et complète de toutes les paires utiles.

Il existe seulement trois manières actuelles d'obtenir cette paire :

- balayer les $\binom{n}{2}$ paires, ce qui est complet mais incompatible avec le produit;
- employer un RNG ou un catalogue de paires de rang borné, ce que les contre-exemples exacts de [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md) réfutent comme autorité complète;
- construire le complément fail-open par self-join du LBVH et center-cover, voie prévue par `P15-HOCUDA-P1`, mais dont la parcimonie et le débit ne sont pas encore qualifiés.

La ligne « complétude = théorème » du tableau A1 doit donc devenir : **complétude conditionnelle à une source complète d'ancres; parcimonie non prouvée**. Une observation de 4,5 millions de paires à une seule taille ne démontre pas $\Theta(n)$, d'autant que plusieurs mesures historiques utilisaient des restrictions non certifiées.

Le propriétaire « plus petite paire lexicographique parmi les arêtes maximales » garantit une émission canonique si toutes les arêtes maximales ont été proposées et si l'owner est testé avant émission. Il ne supprime pas automatiquement le travail dupliqué effectué sur les autres arêtes diamétrales.

### 3.2 Le passage de la profondeur de Tukey aux grandes sphères critiques est faux

Le §1.3 observe correctement qu'une boule tangente non contrainte peut croître vers un demi-espace vide. Il en déduit ensuite l'existence de grandes sphères **critiques** et utilise cette conclusion aux §§4.2 et 10. Cette implication manque.

Une boule tangente quelconque peut avoir son centre hors de $\mathrm{conv}(X)$. Une sphère issue d'un support minimal bien centré doit au contraire avoir son centre dans $\mathrm{relint}\,\mathrm{conv}(U)\subseteq\mathrm{conv}(X)$. C'est précisément pourquoi la note [`GERMINATION_LOCALE_SUPPORTS_3_4.md`](../docs/math/GERMINATION_LOCALE_SUPPORTS_3_4.md) définit $R(p)$ avec la contrainte de centre convexe et avertit que, sans elle, tout point de coque donne artificiellement une valeur infinie.

Conséquences :

- $\tau_e(p)=+\infty$ dans une direction extérieure ne prouve pas l'existence d'une sphère critique de grand rayon;
- une paire point-peu-profond--point-profond n'est rejetée que si son diamètre excède effectivement le majorant certifié du second point;
- « surface donc $\tau=+\infty$ presque partout » ne prouve pas que la coupe portant sur $R$ est inopérante;
- une grande boule vide tangente en un seul point n'a pas pour autant un support critique de grand rayon : sa miniboule supportée par ce singleton a rayon nul.

Il faut aussi fixer explicitement la convention de profondeur de Tukey, leave-one-out ou non, et traiter les points situés sur le plan frontière. Le décalage d'une unité compte lorsque $K\leq10$.

### 3.3 A1 contre A2 est une fausse dichotomie

La proposition compare :

- A1, paire diamétrale suivie d'une cascade de triples et quadruples;
- A2, peeling tridimensionnel ancré par un point.

Elle omet la combinaison déjà formalisée dans le dépôt : **paire diamétrale complète, puis peeling bidimensionnel dans son plan médiateur**. La note [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md) en donne la réduction exacte, la borne locale et les obligations GPU; la [`ROADMAP_IMPLEMENTATION_MORSEHGP3D.md`](../docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md) et le [`TEST_PLAN_MORSEHGP3D.md`](../docs/TEST_PLAN_MORSEHGP3D.md) refusent explicitement tout constructeur dont le travail reste proportionnel à $\sum_e m_e^2$.

Cette voie hybride conserve l'avantage essentiel de A1 — deux extrémités et une géométrie confinée par le diamètre — tout en supprimant le défaut principal de A1 — développer toutes les paires de troisièmes et quatrièmes sommets avant de connaître leur rang.

### 3.4 Le contrat du générateur n'est pas le contrat HGP

L'interface proposée au §4.6 émet « tous les supports minimaux bien centrés de rang fermé utile ». C'est un composant géométrique utile, mais pas une frontière scientifique suffisante.

La sortie normative décrite dans [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md), notamment aux §§5, 13 et 17, exige aussi :

- les facettes et cofaces utiles;
- les incidences actives **et silencieuses**;
- les attachements et remplacements nécessaires à la descente;
- les lots de niveau exactement égal;
- les `coverage_delta` et le `coverage_log`;
- les applications verticales entre ordres et leurs carrés de naturalité.

Les contre-exemples déjà enregistrés montrent qu'un flot de supports Gabriel ou critiques ne suffit pas à reconstruire ces incidences. La réduction « catalogue + attaches vers la HGP » reste une obligation mathématique ouverte, pas une conséquence automatique de l'exhaustivité du catalogue.

La phrase « le point dur n'est ni le générateur ni la forêt » doit donc être retirée. La source complète, les incidences silencieuses et les verticales sont des points durs indépendants de la classification géométrique.

### 3.5 Le domaine « réel puis quantifié » définit deux problèmes différents

La proposition annonce une hiérarchie exacte sur des points réels, puis impose une grille entière 16 bits. La spécification principale interprète au contraire chaque binary64 d'entrée comme un dyadique exact. Une quantification peut modifier :

- l'ordre des distances et des niveaux;
- le rang fermé d'une boule;
- les égalités d'arêtes;
- les cosphéricités, même sans collision de sites;
- la topologie des lots simultanés.

Compter les collisions est nécessaire, mais zéro collision ne prouve pas l'équivalence géométrique avec l'entrée originale. Inversement, un profil explicitement quantifié peut être exact **pour le nuage quantifié** sans être exact pour le nuage source.

La v3 doit choisir et nommer deux profils non confondus :

1. `exact_dyadic_input` : autorité sur les binary64 originaux, filtres dirigés et fallback multiprécision;
2. `quantized_u16_input` : autorité seulement sur le nuage entier produit, avec transformation, origine, échelle, écrêtages, collisions, multiplicités et digest source--cible dans le reçu.

Une sortie du second profil ne doit jamais porter `public_status=exact` relativement au premier. `RelevantGP` doit être vérifié sur l'objet réellement traité; collision nulle ne remplace ni le shell complet, ni l'indépendance affine, ni les barycentriques non nulles.

### 3.6 La forêt v2 et O2 ne sont pas des autorités réutilisables

La proposition affirme que la sémantique de `forest.cpp` est certifiée sur 1 462 nuages et 89 247 cas. Or [`WARNING_AUDIT_PUBLICATION_3.md`](../morsehgp3D_v2/WARNING_AUDIT_PUBLICATION_3.md) établit que l'oracle peut :

- déborder en arithmétique signée avant son garde;
- sauter silencieusement des nuages;
- accepter une campagne vide ou censurée;
- ne pas comparer les arités, enfants, racines, sources et nombre canonique de nœuds;
- masquer certains doublons de supports.

La révision v2 peut fournir une sémantique candidate, des fixtures et des contre-exemples. Elle ne doit pas être reprise « telle quelle » ni être présentée comme certifiée. L'oracle v3 doit employer une représentation multiprécision réellement indépendante, fermer l'identité de campagne et comparer la sérialisation structurelle complète.

## 4. Le peeling local recommandé : ancré par une arête

### 4.1 Réduction exacte de dimension

Fixons une ancre diamétrale $e=pq$, posons $d=q-p$, $D^2=d\cdot d$ et $M=(p+q)/2$. Choisissons deux vecteurs entiers indépendants $b_1,b_2$ orthogonaux à $d$, et écrivons le centre sous la forme $c=M+B t$, où $B=[b_1\ b_2]$ et $t\in\mathbb{R}^2$.

Pour les supports de taille quatre, Jung impose l'ellipse exacte suivante dans ces coordonnées non normalisées :

$$J_e^{(4)}=\left\lbrace t:t^{\mathsf{T}}(B^{\mathsf{T}}B)t\leq\frac{D^2}{8}\right\rbrace.$$

Chaque point $x\notin\{p,q\}$ définit une droite orientée par la forme affine :

$$h_x(t)=2(Bt)\cdot(x-M)-\left(\lVert x-M\rVert^2-\frac{D^2}{4}\right).$$

L'identité de puissance donne exactement :

$$h_x(t)=r^2-\lVert x-c\rVert^2.$$

Ainsi, $h_x(t)>0$, $h_x(t)=0$ et $h_x(t)<0$ signifient respectivement intérieur strict, shell et extérieur. Aucune base orthonormale ni racine carrée flottante n'est nécessaire : les dénominateurs dus à $M$ peuvent être éliminés et tous les signes sont rationnels ou entiers exacts.

### 4.2 Constants, cordes et profondeur

Sur l'ellipse de Jung, chaque point se classe en :

- intérieur constant, compté dans $c_e$;
- extérieur constant, éliminé;
- droite active dont la frontière coupe l'ellipse, comptée dans $m_e$.

Si $c_e>s_{\max}-4$, l'ancre ne peut porter aucun support quatre utile. Sinon le budget de profondeur est :

$$\kappa_e=s_{\max}-4-c_e.$$

En position générale, l'intersection des droites de $z$ et $w$ produit le centre de la sphère passant par $p,q,z,w$. Si $\delta_e(t)$ est le nombre de demi-plans actifs strictement positifs en ce centre, alors :

$$\mathrm{rang}_{\text{ferme}}(p,q,z,w)=4+c_e+\delta_e(t).$$

Il suffit donc d'énumérer les sommets de profondeur au plus $\kappa_e$, et non les $\binom{m_e}{2}$ intersections. La borne locale enregistrée dans le dépôt est :

$$Z_e\leq m_e(\kappa_e+1).$$

Pour $s_{\max}=11$ et $c_e=0$, cela donne $Z_e\leq8m_e$. Le gain conceptuel est majeur : **le rang est calculé pendant la génération**, au lieu d'envoyer des centaines de millions de quadruplets vers une nouvelle requête globale de boule fermée.

### 4.3 Les supports de tailles un à trois ne doivent pas disparaître

La conception doit prévoir les quatre arités séparément.

- Support un : rayon nul, multiplicités et collisions traitées selon le profil d'entrée.
- Support deux : centre $M$, profondeur évaluée en $t=0$, diamètre et shell exacts.
- Support trois : pour une droite $h_z=0$, le circumcentre du triangle est son point minimisant $t^{\mathsf{T}}(B^{\mathsf{T}}B)t$; son rang vaut $3+c_e+\delta_e(t)$ après emploi du disque de Jung propre à l'arité trois.
- Support quatre : intersections shallow décrites ci-dessus.

Les passes arité trois et quatre n'ont pas les mêmes ellipses, valeurs de $c_e$ ni seuils de prune. Au rang fermé onze, un bloc center-cover a besoin de neuf témoins stricts pour réfuter tout support trois et de huit pour réfuter tout support quatre. La tranche P1a actuelle ne profile que le second cas; elle ne peut donc pas servir de source v3 complète.

### 4.4 Ce que « peeling » doit signifier ici

Un peeling crédible n'est pas :

- former toutes les intersections puis garder celles de faible profondeur;
- retirer naïvement des enveloppes successives sans preuve de couverture des niveaux;
- perturber symboliquement les égalités puis déclarer le résultat exact;
- matérialiser l'arrangement entier.

Il doit construire directement le préfixe de niveaux de profondeur au plus $\kappa_e$, avec un travail lié à $m_e$, $\kappa_e$ et $Z_e$. Les constructions randomisées de type Las Vegas et les algorithmes de premiers niveaux donnent une faisabilité théorique en dimension deux, mais leur transfert au clipping elliptique, aux orientations arbitraires, aux concurrences et au GPU reste à formaliser et à qualifier.

Le premier jalon doit donc être un prototype CPU exact qui :

1. reçoit des ancres exhaustives sur de petits nuages;
2. construit les lignes dans une base rationnelle non normalisée;
3. énumère le préfixe shallow sans boucle sur toutes les paires de lignes;
4. produit un transcript vérifiable indépendamment;
5. donne exactement le même catalogue complet que l'oracle exhaustif.

Le brute-force $m_e^2$ reste utile comme oracle local borné. Il ne doit jamais être le prototype que l'on chronomètre comme future architecture.

### 4.5 Égalités, concurrences et canonicité

L'hypothèse de position générale n'autorise pas à ignorer les égalités rencontrées par l'implémentation. Une concurrence exacte de $t$ droites est un lot unique, pas $\binom{t}{2}$ sommets artificiels. Le shell complet doit être compté avant toute émission; une forme hors du domaine `RelevantGP` retire l'autorité ou prend une voie dégénérée explicitement certifiée.

Le propriétaire canonique est la plus petite paire lexicographique parmi les arêtes de longueur maximale du support, calculée sur des `PointId` stables. Il faut tester l'owner avant le sink, trier les membres, grouper les niveaux rationnels égaux et exiger une sortie byte-à-byte identique sous permutation d'entrée, nombre de threads et ordonnanceancement GPU.

### 4.6 Le peeling ancré par point reste utile comme oracle

Le §4.3 de la proposition dit que le peeling ponctuel « meurt ». C'est trop fort. Un arrangement 3D de plans associé à un point peut explorer seulement ses cellules peu profondes et éviter l'énumération brute de tous les triplets. Ce n'est donc pas mathématiquement le même algorithme que la v2.

En revanche, sa complexité globale, sa duplication entre les $n$ ancres, ses faces visitées et son mapping GPU ne sont pas aujourd'hui compatibles avec une décision produit. La bonne place de ce peeling ponctuel est un **oracle CPU indépendant** : il change d'ancrage et de structure par rapport au chemin produit, ce qui lui donne une vraie valeur différentielle.

## 5. Audit du nouveau lemme $R(z)\geq D/2$

Le lemme du §4.2 bis est correct sous ses hypothèses. La circumboule acceptée passe par chaque sommet, son centre est dans $\mathrm{conv}(X)$, son rang est utile, et son diamètre majore celui du support.

Deux corrections de formulation sont nécessaires.

1. Définir $R(z)$ comme un **supremum**, pas nécessairement comme « le plus grand rayon ». Avec des boules fermées, la population peut sauter lorsqu'un point atteint le shell; la borne peut ne pas être atteinte.
2. Ne pas confondre ce $R$, contraint par $\mathrm{conv}(X)$, avec la quantité tangentielle non contrainte utilisée dans l'argument de Tukey.

Surtout, ajouter `if (2R(z) < D) continue` **après avoir scanné toute la lentille** ne réduit pas les quelque $9{,}5\cdot10^9$ visites annoncées; cela réduit seulement les candidats aval. Pour obtenir le gain algorithmique espéré, le seuil doit entrer dans le range-report lui-même.

Une voie sûre est d'augmenter chaque nœud LBVH d'un majorant extérieur `max_tau_hi`. Pour une ancre de diamètre $D$, le parcours peut retirer un nœud seulement si `max_tau_hi < D` est certifié. Une valeur non finie, une sous-normale, un overflow ou un intervalle traversant le seuil reste fail-open. Le range-report devient alors l'intersection de la région J10 et du prédicat $\tau_{\mathrm{hi}}\geq D$ sans visiter tous les rejetés.

Le chiffre $1{,}2\cdot10^8$ est une hypothèse de mesure, pas une conséquence asymptotique du lemme. Il ne doit pas être qualifié de « décisif » avant ce range-report indexé et les distributions p95/p99/max.

## 6. Nuages multi-captations, J7 et sélectivité

La nouvelle reconnaissance des assemblages de poses est importante. Elle interdit à juste titre de faire de l'image de distance ou de la visibilité capteur une autorité de correction. Ces structures peuvent proposer des ancres et ordonner le travail; le complément exact doit produire la même sortie sans elles.

Trois affirmations du §10 restent à corriger.

### 6.1 « Surface implique faible profondeur presque partout » n'est pas un théorème

Un plan tangent local n'est pas nécessairement un plan support global. Sur une surface fermée, repliée, épaisse ou composée de plusieurs captures, le demi-espace choisi peut contenir une part importante du nuage. Cette phrase doit devenir une hypothèse de census, jamais une propriété du domaine.

### 6.2 J7 n'est pas « la » sélectivité

J7 est un prune métrique intéressant et indépendant de $R$, mais :

- son implantation actuelle emploie encore des propositions binary64;
- son recouvrement du disque et ses comptages doivent être certifiés;
- il n'a aucune borne universelle de taux de rejet;
- appliqué après génération de chaque paire, il peut encore payer une source quadratique;
- sa boule J10 peut contenir $\Theta(n)$ points.

Il faut mesurer ensemble $Q$, $V_W$, $a$, $M$, $Z$, rétention J7, visites LBVH, fallbacks et mémoire. Si la sortie est énorme, le SLO doit être réénoncé. Si la sortie reste sparse mais que $Q$, $V_W$ ou $M$ explosent, **l'architecture intermédiaire est no-go**; ce n'est pas seulement un problème de contrat temporel.

### 6.3 Le census réel doit être plus riche que cinq nombres

Sur `SemanticKITTI`, acquisitions uniques et multi-captations, il faut publier :

- collision, écrêtage, multiplicité et statut `RelevantGP` après transformation;
- $Q$, $V_W$, $a$, $C=\sum_e c_e$, $M=\sum_e m_e$ et $Z=\sum_e Z_e$;
- p50/p95/p99/max de $\lvert W_e\rvert$, $m_e$, $Z_e$ et profondeur de file;
- taux de rétention de chaque porte, y compris J7 et $R(z)$;
- taille de la sortie **canonique réellement acceptée**, octets et débit du sink;
- queues lourdes, fallbacks exacts, synchronisations et high-water mémoire.

La taille d'une Delaunay d'ordre zéro peut servir de diagnostic externe; elle ne « décide » pas du sort de la Delaunay d'ordre supérieur et n'autorise jamais son entrée dans le chemin produit.

## 7. Architecture GPU recommandée

J10 borne un rayon spatial, pas une cardinalité. Les mesures déjà enregistrées dans [`OPTIMISATIONS_JUNG_SUPPORTS_3_4.md`](../docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md) atteignent un voisinage maximal de 25 026 points sur `eight_clusters`. « Une arête, sa boule en shared, quelques dizaines de points » n'est donc pas une architecture sûre.

Le pipeline plausible est le suivant.

| étage résident | rôle | structure globale évitée | condition de passage |
| --- | --- | --- | --- |
| canonicalisation | `PointId`, domaine, digest, `RelevantGP` | copie ambiguë de l'entrée | reçu fail-closed |
| LBVH | range-report et self-join | matrice paire--point | lease liée au nuage et à l'epoch |
| propositions | RNG, range image, heuristiques | aucune autorité confiée au proposeur | omission réparée fail-open |
| center-cover | source complète d'ancres | tableau des $\binom{n}{2}$ paires | identité de masse et lease D2D |
| cordes | classification constante/active | tous les $W_e$ persistants | fusion traversal--préparation |
| shallow 2D | niveaux peu profonds | $\sum_e\binom{m_e}{2}$ | travail lié à $M$, $K$ et $Z$ |
| décision exacte | diamètre, shell, bon centrage, owner | rescans globaux par candidat | fixed-limb puis fallback explicite |
| source HGP | facettes, cofaces, silences, couverture | Gamma global | tickets canoniques complets |
| réduction | lots, attaches, forêts, verticales | mosaïque higher-order | snapshot post-lot et naturalité |
| publication | résultat canonique | reconstruction hôte intermédiaire | une synchronisation terminale |

Les ancres sont traitées par classes de taille : warp pour petites, CTA sous-tuilé pour moyennes, file persistante pour lourdes. Un range-report fusionné prépare les lignes sans stocker tous les voisinages. Une ring avec backpressure et des continuations privées évite allocation et memset par tuile. Le sink doit être consommé par le réducteur au fil de l'eau, non accumulé dans une liste globale de supports.

Le §6 de la proposition dit « GPU dès la première ligne », tandis que D11 dit « pas de CUDA avant que la référence CPU soit exacte ». La bonne discipline est parallèle :

- CPU multiprécision indépendant pour l'autorité et les petits différentiels;
- CUDA `proposal_only` très tôt pour falsifier masses, divergence et mémoire;
- aucune promotion scientifique ou publique avant parité avec l'oracle.

Attendre une HGP CPU industrielle avant toute mesure GPU risquerait de construire longtemps une architecture impossible. Lancer un produit CUDA autoritaire sans oracle serait l'erreur symétrique.

## 8. Classification terminale : la profondeur doit éviter le rescan

La proposition nomme la requête de boule fermée répétée comme point dur central. C'est vrai pour la cascade de tuples, mais ce n'est pas une fatalité du peeling shallow.

Pour un support quatre, la profondeur du sommet d'arrangement est déjà le nombre d'intérieurs stricts; les identifiants de conflit peu profonds peuvent être transportés avec le transcript. Pour un support trois, la profondeur au circumcentre sur sa droite donne la même information. Le chemin produit ne devrait donc pas refaire une descente LBVH complète pour chaque support émis.

La décision terminale doit encore vérifier exactement :

- l'éligibilité de diamètre et l'owner;
- le bon centrage et l'indépendance affine;
- le shell complet ou le statut `RelevantGP`;
- la cohérence entre profondeur, conflits et rang fermé;
- le niveau rationnel canonique.

Une requête indépendante de boule fermée reste excellente comme vérificateur différentiel ou fallback d'ambiguïté. Elle ne doit plus être le coût dominant imposé à chaque candidat.

## 9. Portes v3 falsifiables

### V3-0 — domaine et objet public

Choisir `exact_dyadic_input` ou `quantized_u16_input`; définir $K_{\mathrm{eff}}$, `RelevantGP`, profils, statuts, source HGP, couverture et verticales.

**No-go :** une quantification présentée comme exacte pour l'entrée originale, ou un résultat ne permettant pas de distinguer absence valide et sortie supprimée.

### V3-1 — oracle indépendant et largeurs

Oracle multiprécision indépendant, exhaustif jusqu'à $n\leq14$, ancres et reçus à $n=32$, fixtures permanentes des trois warnings v2, campagne fermée.

**Go :** `attempted = decided + rejected_domain`, zéro skip, zéro overflow silencieux, comparaison complète des supports, niveaux, shells, membres, lots et structure.

### V3-2 — census avant architecture

Exécuter les sondes sur `uniform_latin`, `eight_clusters`, multiscale, filaments, déséquilibrés et vrais scans LiDAR mono/multi-captations. Publier toutes les métriques du §6.3.

**Décision :** sortie énorme implique révision du SLO; sortie sparse avec intermédiaires denses implique révision de l'architecture.

### V3-3 — source center-cover complète

Le profiler P1a doit d'abord fermer $P_{\mathrm{prune}}+P_{\mathrm{microtile}}=\binom{n}{2}$ et rejouer chaque prune à $n=32$. La source complète traite séparément les arités trois et quatre, n'émet aucune matrice de paires et produit une lease move-only adoptée D2D.

**No-go :** omission d'une paire canonique exhaustive, doublon de masse, majorité aux microtuiles, queue sérialisante ou source matérialisée quadratique.

### V3-4 — shallow CPU exact

Construire les niveaux 2D à partir d'ancres exhaustives; comparer au brute-force local et au catalogue global sur toutes les permutations, égalités et fixtures.

**No-go :** travail proportionnel à $\sum_e m_e^2$, perturbation déclarée exacte, concurrence non lotie ou mismatch de catalogue.

### V3-5 — shallow GPU et décision exacte

Pipeline cordes--niveaux--décision par classes de charge, transcript de complétude, fixed-limb avec overflow explicite, fallback compté.

**Go :** parité byte-à-byte, zéro rejet ambigu, aucun scan global par support, scratch et tails compatibles avec le débit mesuré.

### V3-6 — source HGP et réduction

Fermer incidences silencieuses, attaches, lots égaux, `coverage_log`, obligation M.1, forêt complète, verticales et naturalité. Jusqu'ici : `component_only=true`, `public_status=not_claimed`.

### V3-7 — publication et déterminisme

Capability move-only liée à l'entrée, lease, epoch, profil et $K_{\mathrm{eff}}$; JSON canonique fail-closed; identité byte-à-byte sous permutations et ordonnancements.

### V3-8 — produit sans budget configuré

Mesurer le pipeline résident complet, sink et réduction inclus. Qualifier d'abord l'objectif secondaire sous une seconde, puis seulement la cible 100 ms, par famille sanctionnée.

**No-go :** allocation par tuile, callback ou synchronisation par niveau, tail sériel, résultat tronqué, budget configuré présenté comme industriel, ou temps intermédiaire incompatible avec l'enveloppe complète.

## 10. Modifications recommandées à `PROPOSITION.md`

Sans imposer une réécriture immédiate du document de Claude, les corrections suivantes sont nécessaires avant d'en faire une conception arrêtée.

1. Remplacer le tableau A1/A2 par trois objets : `A1-source`, `A2p point-anchored`, `A2e edge-anchored shallow`.
2. Remplacer « complétude A1 = théorème » par « complétude conditionnelle à la source exhaustive des ancres ».
3. Corriger le §1.3 : distinguer boule tangentielle non contrainte, $R$ à centre convexe et sphère critique bien centrée.
4. Garder le §4.2 bis, définir $R$ comme supremum et conditionner le gain à un range-report indexé.
5. Recommander `center-cover complet -> cordes -> shallow 2D`, pas `cascade -> tuples -> classification globale`.
6. Réécrire le §5 : le rang vient d'abord de la profondeur; la requête de boule est vérificateur/fallback.
7. Corriger J10 : borne spatiale, jamais borne de population ou garantie de mémoire partagée.
8. Séparer explicitement les profils dyadique et quantifié; ne pas faire de collision nulle une preuve d'équivalence.
9. Retirer toute certification de `forest.cpp` et O2; parler de sémantique candidate et de fixtures.
10. Étendre l'interface scientifique au-delà des supports : incidences silencieuses, attaches, couverture, lots et verticales.
11. Remplacer « si la sélectivité tombe, changer seulement le SLO » par la décision à deux branches sortie/intermédiaires.
12. Résoudre la contradiction CPU/GPU par deux pistes parallèles, oracle d'autorité et falsificateur device.
13. Remplacer les `267 ns/objet` par un ledger bout en bout incluant candidats rejetés, octets, tri, exact fallbacks, sink et réduction.
14. Retirer l'inférence $4{,}5\ \mathrm{M}\Rightarrow\Theta(n)$ tant qu'une campagne multi-tailles et multi-familles ne l'établit pas.
15. Mettre la source HGP et les verticales dans l'ordre des travaux avant toute revendication de hiérarchie exacte.

## 11. Conclusion

La proposition a trouvé une pièce utile, mais pas encore l'architecture décisive. La coupe à tous les sommets renforce la germination; elle ne change pas le fait que la cascade A1 développe encore des tuples avant de connaître leur profondeur. Le véritable saut algorithmique est le changement de dimension déjà rendu possible par l'arête diamétrale : **dans le plan médiateur, le rang devient la profondeur d'un arrangement de droites**.

La voie que je recommande pour MorseHGP3D v3 est donc :

1. figer le domaine et l'oracle indépendant;
2. qualifier une source complète fail-open d'ancres diamétrales;
3. construire les niveaux peu profonds 2D sans $m_e^2$;
4. intégrer rang, shell, bon centrage et owner à cette génération;
5. alimenter directement une source HGP complète puis le réducteur et les verticales;
6. maintenir le pipeline résident, tuilé et canonique, sans mosaïque higher-order globale;
7. laisser les données décider du SLO par des métriques fermées, jamais de la correction.

**GO immédiat :** preuve constructive et prototype CPU exact du peeling 2D ancré par arête, plus census LiDAR et center-cover. **NO-GO immédiat :** implémenter la recommandation A1/D2/D10 actuelle comme architecture produit, annoncer la forêt v2 certifiée, ou ouvrir un statut exact/SLO.

## 12. Références internes déterminantes

- [`PROPOSITION.md`](PROPOSITION.md), objet audité.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md), réduction exacte par ancre, profondeurs, bornes et portes P15.
- [`GERMINATION_LOCALE_SUPPORTS_3_4.md`](../docs/math/GERMINATION_LOCALE_SUPPORTS_3_4.md), définition convexe de $R$, portée et limites de $G_\tau$.
- [`OPTIMISATIONS_JUNG_SUPPORTS_3_4.md`](../docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md), J10, limites de Jung et distributions de voisinage.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md), registre de l'autorité mathématique et obligations ouvertes.
- [`SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md), sémantique dyadique, objet HGP, incidences, couverture et verticales.
- [`ROADMAP_IMPLEMENTATION_MORSEHGP3D.md`](../docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md) et [`TEST_PLAN_MORSEHGP3D.md`](../docs/TEST_PLAN_MORSEHGP3D.md), gates shallow et center-cover.
- [`WARNING_AUDIT_IMPLEMENTATION_2.md`](../morsehgp3D_v2/WARNING_AUDIT_IMPLEMENTATION_2.md) et [`WARNING_AUDIT_PUBLICATION_3.md`](../morsehgp3D_v2/WARNING_AUDIT_PUBLICATION_3.md), obstructions v2 et limites des oracles/publications.

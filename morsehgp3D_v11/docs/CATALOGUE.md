# Catalogue séquentiel exact

Port développeur du 2 octobre 2026 : **qualification G4 courante à `9df774947`**, avec
[résultats et limites de performance](DEVELOPPEMENT.md). Le code vit dans
[`src/catalogue`](../src/catalogue/catalogue.hpp). Les [sources R2 épinglées](../src/catalogue/source_pins.json)
expliquent les lemmes repris ; leurs qualifications ne sont pas celles de ce port.
Cadre courant : `exploration_v11_hors_registre`, `cpu_reference`, `quantized_u21_input_only`, `not_claimed`.
La reprise u21/u24 et ses voies de puissance ont leurs propres portes et mesures dans
[la capture profiles1](../receipts/catalogue_profiles_20261002/README.md). Les anciennes captures u18 restent distinctes.

## Objet et interface

`build_catalogue(const Cloud&, const CatalogueParams&, MemoryBudget&)` rend un résultat entier ou un refus.
Le calcul est séquentiel et entier, sans SiteTree, ordonnanceur ni filtre flottant.
Le Cloud reste stable pendant l'appel. Ses poids doivent tous valoir un ; les doublons sont refusés avant allocation.
Les profils numériques sont 18/21/24 bits ; chacun exige ses propres portes de catalogue.

Le résultat est le catalogue critique **positif** $\mathrm{Cat}_K$ de [MATHEMATIQUES.md](MATHEMATIQUES.md),
avec $p+q_{\min}\leq K+1$. Les naissances q1/sites au niveau zéro seront traitées par FULL.
Chaque boule publie $S^*$, $q_{\min}$, $p$, $m$, son rang exact et ses populations complètes I puis U,
chacune triée en SiteIdx. Les offsets CSR sont u64. L'ordre est (niveau exact, support canonique).
Les rangs positifs commencent à un ; `levels()[0]` vaut zéro, même lorsque le catalogue est vide.

L'identité géométrique est (centre, rayon carré). Le support canonique est le témoin minimal lexicographique
dans le Cloud source ; plusieurs supports peuvent présenter la même boule. Aucune table de clés par candidat
n'est construite. L'émission utilise uniquement $S^*$ ; son niveau vient de cette présentation déterministe.
La primitive `Sphere::through` calcule cependant déjà son niveau pendant la construction du candidat :
ce port ne prétend pas différer ce coût jusqu'à l'admission. Le format historique R2 des niveaux n'est pas repris.

Le Catalogue possède ses Buffer privés, expose des vues constantes et se déplace sans copie ni affectation.
Ses SiteIdx exigent le même Cloud pour être interprétés ; aucun pointeur vers ce Cloud n'est conservé.
La Session et son budget doivent survivre au résultat.

## Préservation des candidats

**G1, listes.** La racine est le pavé entier $[\min X_i,\max X_i+1)$, avec tous les sites.
Chaque liste contient les K plus proches avec tous les ex æquo pour tout centre de la boîte fermée.
Un réservoir de min(nœud,3K) sites distincts fournit des témoins ; un site n'est retiré que si au moins K
témoins le dominent strictement sur cette fermeture. L'égalité conserve le candidat. Le filtre coûte
$O(K\lvert L\rvert)$ par nœud, sans matrice globale de toutes les paires.

**G2, census.** Le centre est dans sa boîte propriétaire. Si le vrai nombre d'intérieurs est inférieur à K,
la liste contient toute la boule fermée, coquille comprise. Sinon elle contient au moins K intérieurs.
Un census local achevé avec $p\leq K+1-q\leq K-1$ est donc globalement exact ; un dépassement du seuil
rejette seulement la présentation. Aucun census accepté n'est tronqué.

**G3, préfixes.** Dans la feuille, l'union des dominateurs des sommets du préfixe minore p, sans double compte.
Rejeter seulement au-delà de $\theta_r=K+1-r$ préserve chaque préfixe du support canonique admis.
Les seuils sont signés avant le contrôle. Un triplet non aigu reste disponible pour un prolongement q4.
Les quadruplets sont énumérés uniquement dans les feuilles bornées ; aucun secours global n'est prévu.

**G4, propriété.** Les boîtes sont demi-ouvertes. L'ajustement par l'enveloppe de la liste conserve les centres
critiques, y compris aux maxima. La subdivision partage sans recouvrement la boîte ajustée ; chaque centre
a un propriétaire unique. Sur chaque feuille terminée, les supports survivants sont tous visités et seul $S^*$
est émis. G1–G3 et cette propriété donnent exactement CatK lorsque l'opération réussit.

Les largeurs entières sont dans $[1,2^B]$. Une coupe diminue d'au moins un le potentiel
$\sum_i\lceil\log_2(h_i-l_i)\rceil$ ; l'ajustement ne l'augmente pas. La profondeur est au plus 3B.
Cette borne ne borne ni le nombre total de nœuds ni le nombre de candidats sous-quadratiquement.
Une feuille terminale trop large provoque un refus, sans sortie dite complète.

## Domaine numérique et ressources

Le repère est T0. Dominances et réservoir ont un budget conservateur $2B+5\leq63$ bits signés.
Pour centre/boîte, $N+D(a-l)$ et $N+D(a-h)$ ont un budget $5B+6\leq127$, sous les formes de Sphere
et les bornes du pavé ; le test exact conserve la borne inférieure et exclut la borne supérieure.
Les autres prédicats utilisent les budgets et refus de num. Les assertions statiques gardent ces expressions.

**Voie native de puissance, qualifiée sur G4 à `9df774947`.** Poser $M=2^B$ et $v=z-a$ pour des Point certifiés.
Les différences sont dans $(-M,M)$ ; chaque carré et somme partielle de $\lVert v\rVert^2$ est inférieur à
$3M^2$, donc tient en i64. Leur élargissement en i128 et le facteur $-2v_j$, de magnitude $<2M$, sont exacts.
Pour q1, la somme des magnitudes vaut $<3M^2$ ; pour q2, le premier terme est $<6M^2$ et chacun des trois
autres $<2M^2$, soit un total $<12M^2<2^{2B+4}$. Pour q4, chaque composante de $(b-a)\times(c-a)$ est
le déterminant de trois points dans le **même** carré $[0,M-1]^2$ : la forme multiaffine atteint ses extrema
aux coins, où les valeurs sont $0$ ou $\pm(M-1)^2$. Ainsi sa magnitude est $<M^2$ ; ce raffinement ne concerne
pas deux vecteurs arbitraires et ne change pas le budget générique de `cross`. Les formules de Cramer donnent
$D<6M^3$ et $|N_j|<9M^4$. Dans $D\lVert v\rVert^2-2\sum_jN_jv_j$, chacun des quatre termes est alors
de magnitude $<18M^5$ : leur somme absolue, donc **chaque produit et somme partielle**, est
$<72M^5<2^{5B+7}\leq2^{127}$ jusqu'à B24. Le tag privé `presentation_arity()`, fixé par les seules factories,
autorise q1/q2/q4 en i128 aux trois profils ; il décrit la présentation, jamais $q_{\min}$.
Pour q3, la borne générale $216M^6<2^{6B+8}$ autorise i128 à B18 ; B21/B24 conservent `Wide`.
`power` conserve sa conversion contrôlée vers `SideInt` ; `side` prend directement le signe natif lorsque
ces bornes le permettent. Aucun centre n'est supposé intérieur au hull ; aucun travail géométrique n'est retiré.

**Niveaux q4 différés, qualifiés sur G4 à `ffc2ff95f`.** Le type fermé `Q4Candidate`
possède seulement l'ancre de coquille et N/D. Sa fabrique ne certifie pas la positivité :
le catalogue conserve `strictly_inside`, puis propriété, census, support canonique,
égalité avec le support généré et admission `p+qmin≤K+1`. Seulement ensuite,
`materialize()` construit le même niveau non réduit ΣN²/D² avant Collector.
`Sphere::through4` conserve son résultat complet par matérialisation immédiate.
Q2/q3 gardent leurs formules ; notamment aucun carré générique de N3 de degré dix.
Aucun nouveau tableau ni cache ; mêmes réservations Buffer. Les deux passes paient
chacune les niveaux q4 admis. `q4_candidates` compte les fabriques q4 non dégénérées
avant positivité/propriété, `q4_levels` les matérialisations après tous les rejets.
Sur succès, ce dernier égale le nombre de boules qmin4 ; les deux nouveaux compteurs
participent à l'égalité des ledgers entre les passes, hors encodage canonique.
Les neuf compteurs géométriques précédents doivent rester identiques. Ce port
ne retire aucun candidat géométrique et ne change ni feuille32 ni capacité256.

**Niveaux q3 différés (4 octobre 2026, contrat de l'auditeur
[`audit_heritage_20261004`](../receipts/audit_heritage_20261004/README.md)).** Le type fermé
`Q3Candidate` possède l'ancre, N/D, les deux certificats de `Sphere::through3` (puissance q3 et
orientation) et les deux autres sommets de sa présentation, sans Level. Le catalogue garde le filtre
d'acuité stricte, puis propriété, census, support canonique, égalité avec le support généré et
admission ; seulement ensuite, `materialize()` calcule la même formule brute de degré six
|u|²|v|²|c−b|²/(4|u×v|²), sans PGCD ni |N|²/D². `Sphere::through3` délègue à ce candidat : une seule
source des coefficients. Les prédicats voient le candidat avec l'arité 3, donc la voie native i128 reste
soumise à son certificat (témoin homothétique au bord du profil, puissance 256 s⁶). La recherche MEB
bornée fait de même (`consider_q3`). Aucun compteur ne change : ni le ledger du catalogue ni les sept
compteurs MEB ; le compte des Level q3 évités reste une mesure de reçu tant que le contrat de compteurs
n'est pas fixé avec les auditeurs. Dumps FULL identiques octet pour octet (ng00, ng02, K = 5).

**Census par masques (lemme R de la feuille J3 de la v10, 4 octobre 2026).** `prepare` remplit, dans la
même boucle de dominance, la transposée `dominated[i]` (sites que i domine sur la fermeture de la boîte).
Au census d'une présentation de générateurs G, le centre est dans la boîte et chaque s ∈ G est sur la
sphère : un site de `dominance[s]` est strictement intérieur, un site de `dominated[s]` strictement
extérieur, sans test de puissance. Un site dans les deux unions contredirait la propriété du centre :
refus `catalogue_invariant`. Le parcours reste celui du census complet (ordre local, listes I et U, arrêt
au premier intérieur de trop, compteur logique `census_tests`) : ledger et dumps identiques. Sur ng00
(K = 5), 12,8 M des 30 M classements hors contacts sont décidés par masque, soit 43 % des tests de
puissance du census du catalogue (mesure de reçu, instrumentation jetable). Mémoire : un mot de plus par
site et par mot de masque dans l'espace de travail de chaque ouvrier.

**Compteurs locaux de feuille (contrat R1 des auditeurs, 4 octobre 2026).** Les quinze champs du ledger
qu'une feuille alimente (`dominance_tests`, `prefixes`, `judged`, `census_tests`, `emitted`, `incidences`,
`q4_*`, `region_*`) sont tenus dans une structure locale, sans `checked_add` ni `Outcome` dans la boucle, puis
vidés une fois par `checked_add` après le succès de la feuille ; un débordement au vidage refuse
(`catalogue_counter_overflow`) et rien n'est publié. Pour m ≤ 1024 sites (refus au-delà), chaque champ local
reste sous mΣ_{q≤4}C(m,q) < 2^49 (`static_assert`). `dominance_tests` s'ajoute en bloc, C(m,2) par feuille.
Ledgers et dumps identiques sur ng00 et ng02 ; porte `mhgp11_catalogue_leaf_counts` (vidage à la limite exacte
et au-delà, feuilles de 32, 33, 256 et 1024 sites, grand livre gravé d'une feuille complète).

**Enveloppes M3 et E4 (feuille J3 de la v10, 4 octobre 2026).** Avant de construire un candidat q3 strict, le
centre circonscrit d'un triangle strictement aigu étant strictement intérieur à son triangle médian, la boîte
englobante fermée des trois milieux (coordonnées doublées, entières) doit rencontrer la boîte demi-ouverte de la
feuille ; sinon `center_in_box` rejetterait aussi. Avant la fabrique q4 : non-dégénérescence par le produit mixte
(`orientation`, le même que la fabrique), comptée dans `q4_candidates` comme avant, puis l'enveloppe des quatre
sommets, qui contient tout centre strictement intérieur exigé ensuite. Rejets exacts, décisions inchangées :
ledger et dumps identiques (ng00, ng02). Sur ng00, M3 écarte 5,30 M des 10,0 M candidats q3 stricts et E4 2,11 M
des 10,26 M candidats q4 non dégénérés (instrumentation jetable). Fixtures de frontière dans
`mhgp11_catalogue_region_median_envelope` : enveloppe médiane plate sur une face basse (centre admis), tétraèdre
plat dont le centre sort de l'enveloppe (compté, rejeté, aucun niveau), tétraèdre haut dont le centre n'est couvert
que par le quatrième sommet (émis).

Paramètres : K dans 1..12 ; K>n admis comme diagnostic ; `leaf_size=32`, au moins K+3 ;
`max_leaf=256`, au plus 1024 et au moins leaf_size ; `max_nodes=0` sans quota explicite ;
`ball_limit=kNone`, borne exclusive dans 1..kNone. Chaque option doit être exercée par une porte dédiée.

La première passe compte les boules et incidences. Après réservation exacte, la seconde remplit les émissions.
Les deux passes doivent produire les mêmes comptes. Un tri par tas entier, sans allocation cachée, précède
l'assemblage final. Les additions de compteurs, tailles en octets et conversions de rang sont contrôlées.
Les refus comprennent paramètres, multiplicité, feuille large, quota de nœuds, compteurs, indices et mémoire.
Les incohérences internes rendent `catalogue_invariant`. Aucun refus ne publie un préfixe du catalogue.

Tous les tableaux dépendant de l'entrée sont des Buffer. Pour n sites, la liste racine et au plus 3B+1
listes filtrées coexistantes coûtent au plus $D=4n(3B+2)$ octets. C'est une borne, pas le pic mesuré.
Poser C=min(n,max_leaf), N=nombre de boules, P=nombre total d'incidences et R=nombre de niveaux positifs.
Le workspace de feuille réserve $W=C\,\mathrm{sizeof}(\mathrm{Point})+8C\lceil C/64\rceil+8C$ octets.
Les masques de préfixes et le réservoir sont des tableaux de taille constante sur la pile.

Les émissions réservent $E=N\,\mathrm{sizeof}(\mathrm{Emission})+4P$ octets.
La sortie réserve $F=N\,\mathrm{sizeof}(\mathrm{CatalogueBall})+(R+1)\,\mathrm{sizeof}(\mathrm{Level})+8(N+1)+4P$ octets.
Le workspace est rendu avant l'assemblage ; émissions et sortie coexistent pendant la copie.
Une borne des réservations propres est donc $\max(W+D+E,E+F)$. Ajouter les réservations préexistantes,
dont le Cloud s'il utilise ce budget. Le pilote mesure `peak()` ; ce compteur ne représente pas la RSS.

## Compteurs et portes

Le ledger décrit une passe logique : nœuds, feuilles, filtres, dominances locales, préfixes, candidats jugés,
tests de census, émissions et incidences ; maxima de feuille et profondeur. Les deux passes sont réellement
payées : les comptes de travail additifs doublent, les maxima ne doublent pas et la sortie est publiée une fois.
Le tri, l'assemblage et le détail des tests de canonicalisation ne se déduisent pas de ces seuls compteurs.

Les portes comparent le catalogue complet à un solveur Gram/Fraction indépendant, y compris les
boules inertes et I/U ; elles couvrent préfixe obtus, coquilles étendues, frontières, K1..12, restrictions,
permutations, profils, refus et chaque allocation. Les mutants jugent séparément les décisions géométriques.
Les tests de FULL ne remplacent pas ce juge de catalogue. Aucune compilation locale ni qualification native
ne découle de la seule relecture de ce code. Les campagnes G4 conservent leurs propres reçus et premiers échecs.
Un catalogue séquentiel mesuré seul n'est ni FULL, ni une exécution GPU, ni le contrat LiDAR de 100 ms.

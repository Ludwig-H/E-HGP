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

## Juge d'Euler à K+2 et restriction J1 (4 octobre 2026)

Filet de sécurité hors produit, posé avant la réécriture de la feuille : sur trames entières, il voit des omissions
communes à deux voies, que le banc A/B ne voit pas. Énoncés J1 et J3 :
[MATHEMATIQUES.md](MATHEMATIQUES.md), paragraphe 8 ; mécanisme repris de la v9 ([provenance](PROVENANCE.md)).
Code : juge [`bench/catalogue_euler.hpp`](../bench/catalogue_euler.hpp), sonde
[`bench/catalogue_euler.cpp`](../bench/catalogue_euler.cpp) (cible `mhgp11_catalogue_euler`).
Cadre : `exploration_v11_hors_registre`, `cpu_reference`, `quantized_u21_input_only`, `not_claimed`.

**Ce qui est vérifié.** La sonde construit $\mathrm{Cat}_K$ et $\mathrm{Cat}_{K+2}$ par la même voie (référence
séquentielle, ou `--production` : les six options de la voie FULL, avec Pool), pour K ≤ 10, donc $\mathrm{Cat}_{12}$
au plus. Pour chaque boule de $\mathrm{Cat}_{K+2}$ : forme (CSR, rang, admission, S* croissant inclus dans U, I et U
croissants et disjoints) ; sphère refaite depuis S* et niveau exactement égal à la table ; signe exact de la puissance
de chaque site listé (I strictement intérieur, U sur la sphère) ; coquille régulière : minimalité de S* ; coquille
étendue : tous ses supports minimaux par les prédicats exacts de `num`, puis qmin et S* recalculés. La somme exacte
$n[k=1]+\sum_b e_k(b)$ est publiée aux ordres 1..K+2 et doit valoir 1 aux ordres vérifiables 1..min(K, n) ; les deux
derniers ordres sont publiés, jamais jugés. Restriction J1 : jointure ordonnée de $\mathrm{Cat}_K$ et du filtre
p + qmin ≤ K + 1 de $\mathrm{Cat}_{K+2}$ par la clé (niveau exact, S*), puis p, m, qmin, listes I et U ; rangs
recalculés sur le filtre ; niveau brut de la table au début de chaque rang (recalculé depuis S* lorsque la première
boule de ce niveau dans $\mathrm{Cat}_{K+2}$ est hors du filtre) ; rangs denses et croissants de $\mathrm{Cat}_{K+2}$.
Codes : 0 conforme, 1 écart, 2 refus, 3 plancher non atteint ou invariant du produit.

**Coquilles étendues.** Le centre est dans l'enveloppe d'une partie A de U si et seulement si A contient un support
minimal (Carathéodory). Les supports (paires de milieu c, triangles strictement aigus coplanaires avec c, tétraèdres
contenant strictement c) sont marqués par masque, fermés vers le haut par une transformée de zêta en OU sur
$2^m$ bits, puis comptés par cardinal ; $e_k$ suit J3 en entiers, sommes i128. Borne déclarée : m ≤ 24 (2 Mio de
brouillon par fil au plus, réservés dans le budget) ; au-delà, refus explicite avant tout calcul. Sur les trames du
contrat, m ≤ 5 jusqu'à $\mathrm{Cat}_{12}$.

**Limites.** Juge nécessaire, jamais un certificat de complétude, et aucun statut public n'en découle. Deux omissions
de contributions opposées se compensent, même avec J1 clé par clé : la porte `mhgp11_catalogue_euler_limits` grave la
compensation triangle/paire à k = 1 du paragraphe 8 (invisible à K = 1, vue à K = 2), le contre-exemple D/T de la v9
à 13 points (invisible à K = 5, vu à K = 6) et sa variante à 23 points (invisible à K = 10 sur $\mathrm{Cat}_{12}$).
Une boule de contribution nulle aux ordres vérifiables échappe à Euler ; J1 ne voit pas une omission commune aux deux
catalogues ; la complétude des listes I et U n'est pas re-parcourue (aucun balayage global) ; le juge porte sur le
catalogue, pas sur FULL.

**Portes.** `mhgp11_catalogue_euler_oracle` (43 petits nuages, dont cosphériques et cocycliques, six valeurs de K,
contre un juge Fraction indépendant qui vérifie aussi l'identité J3 sur toutes les boules critiques ; voies référence
et production identiques) ; `mhgp11_catalogue_euler_limits` (limites, détections par J1 seule, borne 24/25, refus,
planchers) ; `mhgp11_catalogue_euler_scale8000`, `_scale16000`, `_scale32000` (familles uniformes régénérées par la
sonde, K = 5) ; `mhgp11_catalogue_euler_lidar_ng0{0,1,2}_k5` et `_k10` (label `long` pour K = 10). Les lignes de
verdict gravent les nombres de boules et de coquilles étendues ; sur les trames, ils égalent ceux du catalogue v10
(`afb081774`, audit L01 du 2 octobre 2026), et les parts régulière et étendue d'Euler par ordre égalent ses reçus.
Neuf mutants de `tests/mutants/catalogue.json` (préfixe `euler_`), tous tués en campagne locale le 4 octobre 2026
(cause « code »). Sur n = 8 000 à K = 5, l'omission commune des q3 par la feuille laisse J1 aveugle (aucun écart) et
Euler échoue aux ordres 1 à 5 ; l'omission au bord d'admission, qui dépend de K, laisse Euler égal à 1 aux ordres 1 à
5 et J1 relève 130 202 boules absentes de $\mathrm{Cat}_5$. Les deux juges sont donc complémentaires.

**Coût mesuré** (4 octobre 2026, codespace partagé de 8 cœurs sous charge variable, Release u21, trois fils, voie
production ; durées murales indicatives, compteurs exacts) :

| Entrée | n | K | boules $\mathrm{Cat}_K$ / $\mathrm{Cat}_{K+2}$ | étendues | deux catalogues | Euler | J1 | total | pic réservé |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| uniforme | 8 000 | 5 | 597 998 / 1 301 414 | 0 | 10,6 s | 0,63 s | 0,30 s | 11,7 s | 430 Mo |
| uniforme | 16 000 | 5 | 1 233 046 / 2 698 867 | 0 | 21,3 s | 1,11 s | 0,25 s | 22,8 s | 871 Mo |
| uniforme | 32 000 | 5 | 2 536 732 / 5 578 606 | 1 | 44,8 s | 1,92 s | 0,66 s | 47,7 s | 1 781 Mo |
| lidar_ng00 | 39 885 | 5 | 1 306 696 / 2 565 656 | 320 | 26,2 s | 0,93 s | 0,31 s | 27,6 s | 828 Mo |
| lidar_ng01 | 35 551 | 5 | 1 095 926 / 2 104 698 | 204 | 19,8 s | 0,54 s | 0,18 s | 20,7 s | 687 Mo |
| lidar_ng02 | 45 845 | 5 | 1 407 885 / 2 675 990 | 865 | 23,0 s | 0,82 s | 0,36 s | 24,4 s | 862 Mo |
| lidar_ng00 | 39 885 | 10 | 5 512 670 / 8 314 472 | 529 | 46,4 s | 1,80 s | 0,54 s | 49,1 s | 3 129 Mo |
| lidar_ng01 | 35 551 | 10 | 4 383 302 / 6 492 748 | 341 | 28,5 s | 1,35 s | 0,40 s | 30,5 s | 2 454 Mo |
| lidar_ng02 | 45 845 | 10 | 5 483 320 / 8 025 829 | 1 559 | 45,6 s | 2,35 s | 0,66 s | 49,0 s | 3 018 Mo |

Le juge lui-même (Euler et J1) coûte de 3,5 à 8 % du total ; le reste est la construction des deux catalogues. Le
pic réservé est celui du MemoryBudget pendant la construction de $\mathrm{Cat}_{K+2}$, $\mathrm{Cat}_K$ coexistant ;
ce n'est pas la RSS (3,9 Gio au plus mesurée à K = 10). Durées CTest des portes entières, même jour et même
machine, charge plus faible : `scale8000` 5,1 s, `scale16000` 14,2 s, `scale32000` 31,6 s ; trames à K = 5 : 16,4,
11,5 et 13,6 s ; à K = 10 : 41,5, 44,7 et 40,2 s ; `euler_oracle` 12,4 s et `euler_limits` 0,5 s, chacune doublée par
sa jumelle `-O`. Aucune mesure G4 de ce juge n'existe encore.

## Voie GPU des feuilles (4 octobre 2026)

Réponse au contrat R7 de l'audit des contrats numériques
([`AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`](../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md)).
Cadre : `exploration_v11_hors_registre`, `cpu_reference` pour la référence, `not_claimed`. Trois options du
catalogue, bits de la sonde FULL entre parenthèses ; toutes exigent le graphe de paires (2048), le lot exige aussi la
passe unique (64), et les deux exécuteurs de lot s'excluent.

- `device_leaf` (16384) : la feuille du graphe de paires est jouée par [`leaf_device.hpp`](../src/catalogue/leaf_device.hpp),
  source unique hôte/device (`MHGP11_LEAF_HD`), au lieu de `leaf.cpp`. Seuls ses chemins `i128` certifiés décident ;
  tout le reste (feuille de plus de 32 sites, certificat absent, invariant que `leaf.cpp` refuserait) rend la feuille
  `unresolved`. Une feuille non résolue est rejouée entière par `leaf.cpp` **avant admission** : un débordement ou un
  refus n'est jamais un rejet géométrique.
- `batch_leaves` (32768) : la passe unique ne dénombre plus ses feuilles, elle les met en file par tâche
  ([`leaf_queue.hpp`](../src/catalogue/leaf_queue.hpp)) ; [`single_pass_batch.cpp`](../src/catalogue/single_pass_batch.cpp)
  les rassemble, les fait jouer par l'exécuteur (comptage, préfixes, écriture à places fixes,
  [`leaf_batch.cpp`](../src/catalogue/leaf_batch.cpp) sur le Pool), calcule chaque Level sur l'hôte par
  `Sphere::through` du support (même arité que la présentation génératrice, donc le niveau d'émission), rejoue les
  non résolues, puis rend le bloc à l'assemblage ordinaire. Le registre du catalogue est la somme des compteurs des
  feuilles résolues, du registre du repli et de celui du parcours : il égale celui de la voie CPU champ par champ.
- `cuda_leaves` (65536) : même lot, exécuteur [`leaf_batch_cuda.cu`](../src/catalogue/leaf_batch_cuda.cu) (un fil
  par feuille, préfixes CUB), construit seulement avec `-DMHGP11_ENABLE_CUDA=ON` (`sm_120`, nvcc réel) ; sans CUDA,
  l'option est refusée avant tout calcul.

**Exécution du lot.** Chaque feuille est d'abord comptée : ses compteurs et ses émissions sont rangés dans une case
fixe (32 enregistrements, 256 incidences), dans l'ordre d'émission. Après les préfixes exclusifs, les cases sont
copiées à leur place, et seules les feuilles qui émettent et débordent de leur case rejouent leur feuille pour
l'écrire (`fill_jobs` : 1,7 à 2,5 % des feuilles sur les trames à K = 5 ; la moitié des feuilles n'émet rien).
Sur le GPU, les fils prennent les feuilles par taille décroissante (tri stable par comptage) ; les sorties gardent
leur place par feuille. Le contexte CUDA s'ouvre dans un fil d'arrière-plan dès le début de FULL
(`prefetch_device_context`), recouvert par l'index et le parcours ; les tableaux viennent du pool du périphérique
(`cudaMallocAsync`), qui garde la mémoire rendue pour les passes suivantes. Sur l'hôte, les mêmes cases et la même
copie valident cette logique (`mhgp11_tower_full_leaf_lanes` exige les deux chemins d'écriture).

**Mémoire.** Les tableaux du GPU sont réservés dans le même `MemoryBudget` que l'hôte (`BudgetReservation`, sans
allocation hôte), avant `cudaMalloc` : coexistences et pic compris. Ne sont pas comptés le contexte CUDA ni la mémoire
locale que le pilote réserve pour le cadre statique des noyaux (3 248 et 3 264 octets par fil, sans débordement de
registres ; 210 et 164 registres). **Compteurs.** Une feuille de 32 sites au plus a chaque compteur inférieur à
$32\cdot3\cdot41448<2^{22}$ (preuve R1 de `leaf.cpp`) ; les feuilles se recouvrent (353 456 feuilles pour 39 885
sites sur ng00), les exécuteurs refusent donc un lot de plus de $2^{40}$ feuilles, et toute somme de lot reste sous
$2^{62}$ : les réductions de lot sont exactes.

**Validé localement** (u21, trames sans sol, K = 5) : dumps FULL et registres identiques à la voie CPU pour
`device_leaf` (ng00, ng02) et `batch_leaves` (ng00, ng01, ng02, à froid et à chaud) ; la porte
`mhgp11_tower_full_bench_io` couvre ces deux modes sur son petit témoin, `mhgp11_tower_full_leaf_lanes` un nuage de
3 000 sites. Avant les cases, l'exécuteur de lot sur l'hôte jouait toute feuille deux fois (3,4 à 4,0 s sur ces
trames, six fils locaux) ; avec elles, une fois (1,2 à 1,4 s), mesure locale indicative. **Sur G4** (reçu [`gpu_g4`](../receipts/developpement_20261004/gpu_g4/README.md), six sessions) : la voie CUDA rend
les mêmes dumps et le même registre que la voie CPU sur les trois trames à K = 5 et K = 10 (372 prises à froid, 84
processus à chaud, aucun refus). Elle reste plus lente que la voie CPU à K = 5 (meilleures passes à chaud 323 à
394 ms contre 275 à 343 ms) et gagne 2 à 6 % à K = 10 avec des feuilles de 24 (1,76 à 2,35 s à chaud). Nsight
Compute montre pourquoi : un fil par feuille laisse 3,2 à 3,4 fils actifs sur 32 par warp, et la pile locale de
3,2 Kio par fil est lue à 2,2 octets utiles par secteur ; le pipeline entier n'est qu'à 24 %. La suite est une
feuille coopérative par warp, de forme J3. Le lot sur l'hôte (`batch_leaves`) reste plus lent que `leaf.cpp`.

**Exécuteur partagé** (6 octobre 2026, [`leaf_batch_split.cpp`](../src/catalogue/leaf_batch_split.cpp)). Le diagnostic G4
`claudedom1`, sur les trois trames LiDAR réelles à K = 5 avec des feuilles de 24, donne pour le lot entier 71 à 77 ms
sur le GPU et 102 à 128 ms sur le Pool de l'hôte, qui attend sans rien faire pendant le calcul du GPU. À K = 10, les
mêmes chiffres sont de 169 à 209 ms et de 364 à 462 ms. L'option `split_host_permille` (argument final de la sonde,
0 à 1000) confie au Pool les feuilles les plus lourdes, jusqu'à cette part du travail estimé (m³, par paliers de m
décroissant, les premières feuilles du lot dans le palier de bascule). Les autres vont à l'exécuteur du lot (GPU avec
`cuda_leaves`, sinon l'hôte sur un Pool auxiliaire), dans un fil à part qui a son propre petit Pool : CUDA y touche
les pages de ses tampons de retour. Les deux parties tournent en même temps, puis sont fusionnées dans l'ordre du lot :
statuts, débuts globaux, émissions copiées à leurs places, compteurs sommés. Chaque feuille est traitée par le même
code source, et ses émissions comme ses compteurs ne dépendent pas de l'exécuteur : le résultat est celui d'un
exécuteur unique, quelle que soit la part. Portes : `mhgp11_catalogue_leaf_split` (sélection contre une recomputation
indépendante) et `mhgp11_tower_full_leaf_lanes`, qui demande mêmes dump et registre à des parts de 0,1 %, 30 %, 40 %
et 100 %, avec un partage observable. Mesure G4 du gain à jouer.

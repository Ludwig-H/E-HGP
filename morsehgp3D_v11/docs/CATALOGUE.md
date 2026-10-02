# Catalogue séquentiel exact

Port développeur du 2 octobre 2026 : **qualification G4 à venir**. Le code vit dans
[`src/catalogue`](../src/catalogue/catalogue.hpp). Les [sources R2 épinglées](../src/catalogue/source_pins.json)
expliquent les lemmes repris ; leurs qualifications ne sont pas celles de ce port.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`, `quantized_u18_input_only`, `not_claimed`.

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

Les portes prévues comparent le catalogue complet à un solveur Gram/Fraction indépendant, y compris les
boules inertes et I/U ; elles couvrent préfixe obtus, coquilles étendues, frontières, K1..12, restrictions,
permutations, profils, refus et chaque allocation. Les mutants jugent séparément les décisions géométriques.
Les tests de FULL ne remplacent pas ce juge de catalogue. Aucune compilation locale ni qualification native
ne découle de la seule relecture de ce code. La campagne G4 doit fournir ses propres reçus et premiers échecs.
Un catalogue séquentiel mesuré seul n'est ni FULL, ni une exécution GPU, ni le contrat LiDAR de 100 ms.

# q34 : contrat d'une arène collective et de son consommateur

27 septembre 2026. Revue indépendante, base `4badf8b7d`.
Cadre `exploration_v9_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `audit_collective_arena_design`,
`public_status=not_claimed`. Aucun moteur, reçu ou script cloud modifié ;
GCP non utilisé. Échanges avec l'auteur du
[plan d'arène](../b_q34_collective_arena_20260927/DESIGN.md).

## Verdict utile

Remplacer les vecteurs des petits plans par quelques buffers collectifs
est nécessaire, mais ne suffit pas. Le consommateur GPU actuel crée
`pair_mask[P]`, `flags[P]`, `positions[P]`, puis refait une recherche du
rectangle propriétaire de chaque paire lors de la dispersion. Il faut
changer **ce consommateur aussi**, sinon le tableau de taille P réapparaît
après l'arène. Voir `src/gpu/filter_runner.cu`, `pair_kernel`,
`scatter_kernel` et `run_filter_batch` (notamment lignes 581–610 à la base).

Deux représentations restent à comparer sans choix prématuré :

| Représentation des candidats avant S2 | Avantage | Coût à payer explicitement |
| --- | --- | --- |
| Facteurs groupés + bandes disjointes | Faible nombre de descripteurs et aucune liste de paires rejetées | Retour de S dans l'ordre original, par exemple tri des seules survivantes |
| Une liste B ordonnée par classe A, A laissé dans l'ordre original | E et S directement dans l'ordre natif, pas de tri de S | W comparaisons de crédits, T entrées B pouvant être répétées entre classes A |

Les deux gardent les mêmes crédits certifiés. Aucune ne justifie de
recalculer la sélection des témoins ou leurs tests géométriques pour
connaître la taille des buffers.

## 1. Quantités à conserver séparément

R est le nombre de rectangles du front ; Rp celui des rectangles
effectivement préparés par Pool après le filtre rectangle. F_A et F_B
sont les sommes des tailles de leurs facteurs, F=F_A+F_B. P est la masse
de tous les rectangles restés ouverts avant Pool, E celle des paires
restées ouvertes après Pool, **fallbacks inclus**. S est le nombre de
paires non nulles après le filtre ponctuel S2. Les masses par voie P3/P4
et E3/E4 restent distinctes de leurs unions.

Pour le format bandes, C désigne les classes occupées et D les bandes
plus les descripteurs fallback. Pour le format ordonné, W est la somme
de C_A·|B| sur les rectangles préparés ; T est la somme des longueurs des
listes B stockées une fois par classe A. Ces quantités ne sont pas des
tailles d'objets FULL ni des nombres de sites du nuage.

L'arène supprime les allocations par plan, pas les répétitions des mêmes
sites dans plusieurs facteurs. La somme F reste à mesurer. Le choix de K
fixé borne C_A par K(K−1), soit 20 à K5 et 90 à K10, mais ne fournit pas
une borne sous-quadratique de F, E ou S en fonction de n.

## 2. Sélection des plans, une seule géométrie

Partir du même ordre de rectangles et du même index immuable. Le filtre
rectangle existant s'exécute une fois. Pour chaque rectangle :

- masque nul : E=0, pas de plan ni fallback consommable ;
- facteur de cardinal inférieur à deux : conserver un descripteur brut,
  sans copier ses facteurs ni les balayer pour préparer un Pool ;
- si |A|+|B|−2 est inférieur au plus petit seuil actif, même fallback :
  les seuls témoins locaux ne pourraient fermer aucune voie ;
- sinon : rectangle Pool, avec ses tailles connues avant toute sélection.

La limite `min_factor=2` est une politique de préparation, jamais une
troncature de recherche. Un fallback représente **tout** son produit.
Une classe ou un plan dont E devient nul ne doit pas être reconverti en
fallback plein. K1 n'a aucune voie q34 ; à K2, q4 est inactive avant toute
soustraction de seuil.

Une première somme préfixe des tailles des facteurs éligibles réserve les
plages globales de crédits. Chaque facteur est sélectionné et certifié
une seule fois ; il écrit son crédit compact et le nombre de classes.
La suite relit uniquement ces crédits : compter/scatter les classes,
compter/émettre les bandes, ou construire les listes B ordonnées. Un
second calcul des petits regroupements peut être une bonne économie de
mémoire ; ce n'est pas une autorisation de refaire les tests des huit coins.

Les top-K peuvent rester temporaires par tâche/worker. Pour de gros
facteurs, une réduction de top-K locaux est possible : le top-K de l'union
est celui de l'union des top-K de morceaux disjoints. Les scores et le
départage doivent être les mêmes ; le coût de ces propositions temporaires
est alors O(K·nombre_de_morceaux), pas zéro. Fixer une taille de tâche ne
plafonne jamais le facteur : tous ses morceaux doivent être traités.

Point de port GPU indispensable : `direct.hpp::prepare` départage les
projections égales par **ID original**. `gpu::FilterInput` ne transporte
actuellement que les coordonnées en rang spatial, pas `rank_to_original`.
Ajouter ce mapping u32 global une fois (4n octets), ou déclarer et
requalifier une autre heuristique de sélection. Le remplacer silencieusement
par le rang spatial casserait le différentiel. Les coordonnées u18 ne
permettent pas d'encoder les identifiants sur 18 bits.

## 3. Tailles et propriété minimales

Le propriétaire collectif conserve le même index/nuage pendant toutes les
tâches, transferts et consommations ; ses buffers sont créés par lui avant
publication de vues constantes. Un `std::move` d'un vecteur externe ne
constitue pas une preuve d'absence d'alias mutable.

| Donnée | Format sûr proposé | Remarque |
| --- | --- | --- |
| ID de site/rang/nœud du domaine GPU actuel | u32, bornes vérifiées | Les limites des sentinelles restent distinctes ; ne pas les déduire des 18 bits des coordonnées |
| Crédit q3/q4 | deux demi-octets d'un u8 | q3≤9, q4≤8 ; domaine validé avant décodage |
| Origine/longueur d'un facteur | u32 | Peut aussi être lu dans le nœud du rectangle, sans duplication |
| Offsets globaux facteurs/classes/bandes/listes | u64 | La somme F ou T peut dépasser u32 même si chaque facteur tient en u32 |
| Masse, prefix de paires, ordinal original | u64 avec additions/produits contrôlés | Ne pas utiliser un offset u32 d'arène pour une masse |
| Rang groupé ou entrée B d'une liste | u32 | Rang spatial ou rang local explicitement nommé, jamais ID original implicite |
| Bande locale | `a_class,b_first,b_last` en trois u32 | 12 octets locaux ; offsets collectifs et metadata du plan sont en plus |

Les tableaux SoA permettent de conserver les crédits sur un octet sans
padding par entrée ; publier néanmoins les vrais `sizeof`, capacités et
scratchs. Les propositions peuvent être abandonnées après certification.
Les classes B et crédits A nécessaires à la construction peuvent être
temporaires dans le format bandes, mais on ne peut les libérer avant leur
dernier consommateur. Ne pas annoncer « 12 octets par bande » comme mémoire
totale de l'arène.

Les scans CUB actuels prennent un nombre d'items signé 32 bits. Un offset
u64 ne lève pas cette contrainte : segmenter réellement les appels et
propager des bases u64 entre segments. Une incapacité mémoire reste un
échec ou un relais exact documenté, jamais une fin de recherche réussie.

## 4. Bandes : décodage et retour dans l'ordre avant S3

Pour une bande d'une classe A de longueur m et d'un intervalle B de
longueur l, l'ordinal local t se décode en `a_first+t/l` et
`b_first+t%l` dans les tableaux de rangs groupés. La masse m·l est u64.
Le masque se recalcule avec les deux crédits et les seuils propres à q3
et q4 ; une bande union ne porte **pas** uniformément le masque 6.

Amortir la recherche de bande sur une tuile/warp plutôt que sur chaque
paire. Une somme préfixe des masses ou des nombres de tuiles par bande
permet de trouver son propriétaire ; ne pas matérialiser une liste de
toutes les paires, ni prétendre qu'une liste globale de E/32 tuiles est
indépendante de E. Les petites bandes risquent de sous-occuper un warp :
mesurer occupation/padding ou empaqueter plusieurs petits segments.
Un décodeur plus sophistiqué ne doit pas changer le masque ou émettre
deux fois une paire à la frontière de deux segments.

Après S2, chaque survivante porte temporairement l'ordinal
`raw_base[rectangle] + (a_rank-A.first)*|B| + (b_rank-B.first)`.
`raw_base` est un préfixe en ordre original des rectangles, avec une
convention publiée (par exemple tous les produits avant filtre), pas un
préfixe de classes ou de bandes. Le tri des seules S survivantes par cet
ordinal restitue l'ordre du produit natif. Les masques et tout sidecar
sont déplacés par **la même permutation**. Le coût du tri, ses buffers et
les huit octets de clé par survivante sont payés ; aucune propriété de
« tri quasi gratuit » n'est postulée.

Le consommateur `run_wspd_q34_batched`, lignes 1666–1692, exige déjà S en
ordre rectangle puis ligne A puis colonne B, strictement croissant. S3 et
S4 attribuent leurs positions, arènes et reports à ces ordinaux. Restaurer
l'ordre après S3 serait trop tard pour conserver ce contrat d'exécution.

## 5. Variante ordonnée : listes B partagées par classe A

Pour chaque crédit A réellement présent, stocker les rangs B **originaux
croissants** dont le masque local est non nul, avec leur masque exact.
Chaque ancre A garde seulement un indice de sa classe ; toutes les ancres
de cette classe réutilisent la même liste B. Parcourir A dans son ordre
original produit alors E directement en ordre row-major. La compaction
stable de S préserve cet ordre, sans tri final ni permutation des facteurs.

On a T≤W≤K(K−1)·F_B et T≤E : chaque entrée d'une liste B représente au
moins une paire puisque sa classe A est non vide. Ce n'est pas T≤F_B ;
un même rang B peut apparaître jusqu'à 20/90 fois à K5/K10. Le travail
total comprend W comparaisons bon marché de crédits, en plus de la vraie
préparation O(KF). La géométrie n'est pas répétée W fois.

Le prototype root `b_q34_ordered_rows_20260927/rows.hpp` matérialise
`a_slot` sur un octet par A, offsets de classes u64, rangs B locaux u32 et
masques u8 : payload logique F_A+8(C_A+1)+5T par rectangle, hors objet,
capacités, géométrie, préfixe des masses des ancres et scratch du consommateur.
T d'un rectangle peut dépasser u32 ; ses offsets sont donc bien u64.
Une liste vide est réellement vide et ne réintroduit pas le produit.

Revue du code et de son gate : pas de défaut mathématique évident trouvé.
Le gate compare toutes les listes B et exige T≤E et T≤W ; sur petits
fronts il vérifie chaque `a_slot` et chaque paire dans l'ordre original.
Il exige aussi des cas dont la permutation A groupée diffère effectivement
de l'ordre original. Les mesures grandes avec `expand=false` ne recontrôlent
pas chaque `a_slot` et n'exécutent pas S2 ; elles mesurent une représentation
ajoutée à l'ancien Pool encore payé. Les résultats de capture sont à lire
dans leur propre reçu, pas déduits de cette revue.

Pour le GPU, compter la longueur de chaque ligne A via sa classe puis
préfixer ces longueurs donne les offsets E. Ces préfixes coûtent O(F_A),
pas O(E). Le scatter stable des listes B peut compter par morceaux puis
relire les crédits ; des comparaisons de crédits répétées doivent être
comptées, sans les confondre avec une seconde préparation géométrique.

## 6. Consommation sans tableau global de taille P ou E

Le format bandes comme les listes ordonnées peuvent être consommés par
vagues d'au plus Q positions, Q étant une capacité temporaire réglable,
**pas un quota de recherche**. Dans chaque vague :

1. Décoder les positions et exécuter S2 une fois ; conserver les masques
   temporaires de cette seule vague et les comptes par tuile.
2. Préfixer ces comptes et réserver les sorties de la vague.
3. Disperser les survivantes depuis les masques conservés, sans refaire
   les recherches géométriques. Décoder une deuxième fois les coordonnées
   de sortie est différent de refaire S2 et doit aussi être chronométré.
4. Réutiliser le scratch après consommation et continuer jusqu'à EOF.

Une allocation de sortie insuffisante ne doit pas effacer les masques de
la vague puis relancer S2 en cachant ce travail. Réserver des pages/sorties
avant dispersion, ou refuser explicitement tout en conservant le diagnostic.
Sur listes ordonnées, concaténer les vagues et tuiles dans leur ordre de
préfixe, pas dans l'ordre d'achèvement des workers. Sur bandes, le tri de S
reste nécessaire avant S3. La mémoire globale est O(F+C+D+Q+S) dans cette
voie, ou O(F+T+Q+S) dans la voie ordonnée, métadonnées/préfixes inclus.
Conserver globalement `mask[E]` n'aurait pas cette borne : E peut valoir P.

## 7. Pourquoi S doit pouvoir rester strictement identique

Sur l'index Q2 natif, chaque feuille contient un site et sa boîte singleton
exacte (`q2_census.cpp::build`). Le filtre ponctuel Affine de S2 descend
tous les blocs non décidés ; à la feuille, ses tests H/Xi sont exactement
le prédicat strict du témoin. Il rejette donc une voie si et seulement si
son nombre total de témoins atteint son seuil, sauf erreur explicite de
parcours/arithmetic.

Un rejet Pool fournit ce nombre de témoins stricts distincts dans A\{a}
et B\{b}, deux ensembles disjoints. S2 aurait donc rejeté la même voie.
Après S2 appliqué aux seuls candidats E, le tableau S doit pouvoir être
identique à l'ancien, **masques compris**, une fois l'ordre restauré.
Les visites changent, pas cette sortie. Ne pas ajouter les crédits Pool
au census : ils n'autorisent que le rejet préalable.

Cette preuve concerne le propriétaire natif et ses feuilles exactes. La
validation publique d'un `FilterInput` brut accepte certaines boîtes plus
lâches ; elle ne suffit pas seule à attribuer la complétude ponctuelle à
un arbre arbitraire. La porte de raccord doit comparer S contre l'ancien
filtre et vérifier les rejets Pool sur des fixtures exhaustives, puis
comparer les objets FULL et les cas de reports/capacités réduites.

## 8. Compteurs et porte de décision

L'interface actuelle exige `batch.expanded_pairs=P`, puis affecte aussi
`witness.pairs.queries=P` (`wspd_q34.cpp`, lignes 1643–1701). Renvoyer E
dans le premier champ provoque un refus ; garder P dans le second ferait
passer une masse virtuelle pour le nombre des recherches réelles.
Le port doit donc distinguer P, E et S explicitement, publier P−E rejets
union Pool et P3−E3/P4−E4 rejets par voie, et compter E recherches S2
réelles. Les additions de compteurs doivent porter sur des ensembles
disjoints, pas additionner q3 et q4 pour retrouver une union.

Porte de choix concrète : sur mêmes entrées/masques s8/10/12 et K5/K10,
mesurer F_A/F_B/C/D/W/T/E/S, octets persistants et temporaires, allocations,
préparation géométrique, regroupement, décodage, S2, ordre/compaction et
transport jusqu'à S3. Conserver les singletons et plans vides dans cette
mesure, et ajouter les coupes capteur pour la croissance. Tester des sommes
d'offsets dépassant u32 sans allouer ces masses, listes/bandes vides,
frontières de vagues, rang≠ID, égalités de projection, masques mixtes,
échec d'allocation et reprise sans doublon. Une baisse de D ou T isolée ne
remplace pas le temps de chaîne.

Le résultat direct précédent supprime déjà les anciennes cellules, mais
garde 934 560 buffers retenus sur 00/K5 et une préparation mono d'environ
0,6 s. L'arène collective est donc un chantier réel ; ni ce format ni ses
bornes en F/E ne ferment seuls la cible GPU 100 ms ou une croissance
sous-quadratique sur les régimes LiDAR.

# Clés initiales de représentants — diagnostic privé

11 septembre 2026. Travail privé, non intégré. Moteur publié épinglé à
`ad7ffd28b35e153a20bd8cf42534d1cd29160bcd` ; header nominal
`910f45baea1750b11d2b34f40c893c9d1a34f950705cdb127ffa226de60f7b2e`.
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Source active inchangée, aucun fichier d'audit modifié, GCP non utilisé.

## Question et périmètre

L'auditeur demande le nombre exact des requêtes initiales distinctes, et non
un taux de hits d'un cache évictif. Pour chaque ordre K, R_K compte les
appels initiaux à `resolve`, U_K les ensembles initiaux distincts de K
indices géométriques. L'entrée et son index font partie de l'identité.
Une occurrence répétée à deux ordres n'est pas une même clé de ce contrat.

Le script `prepare.py` extrait depuis Git les sources publiées dans deux
arbres privés. L'overlay `instrumentation.patch` observe l'entrée après
tri et garde de cardinalité, **avant le retour K1 et avant le cache**.
Il copie les indices dans une clé de dix i32 (40 octets), sans modifier
sites, racines, ancres, lots, cache ni compteurs existants. Ni les états
successifs de la descente, ni les semis d'ancres ne sont observés.

Un seul ordre réside à la fois dans le buffer : tri lexicographique des
clés entières, puis `std::unique`, digest, libération. Aucun hash ni
estimateur ne décide l'égalité. L'index est scellé par la séquence de ses
coordonnées et de ses PointId ; le digest unique inclut ce sceau, K, U_K,
puis les U_K clés avec exactement K indices. Zéro est un indice valide.
Le remplissage nul est hors identité puisqu'un ordre est fixé.

R_K doit égaler le delta de `representatives`, U_K doit être entre zéro
et R_K ; les sommes des R_K et des compteurs MEB/hits/semis doivent égaler
les totaux après construction complète. Un résultat de préfixe n'est pas
publié comme une tour. Le journal, les contributions et verticales restent
ceux du moteur existant. Ce diagnostic ne qualifie pas la complétude WSPD.

## Coût propre de l'observateur

Collecte O(K R_K), tri O(K R_K log R_K), mémoire O(K R_K) pour l'ordre
courant. Les clés de tous les ordres ne coexistent jamais. Le tableau
rapporté sépare octets logiques et capacité allouée ; le RSS de tout le
processus n'est ni cette capacité ni un gain de mémoire produit.

Les durées et RSS capturés sont **instrumentés, non des benchmarks du
moteur nominal**. Le tri, le hash des clés uniques et les allocations de
l'observateur participent aux mesures. L'hôte est partagé ; sur la micro
nominale n400, 28,52 s externes correspondaient à 5,76 s CPU (20 % CPU).
Le triplet s8/10/12 historique ne doit pas être réattribué à ce diagnostic.

U_K n'est pas un nombre minimal de MEB : une descente peut en demander
plusieurs, alors que le cache et les semis évitent déjà certaines MEB
initiales. Le facteur R_K/U_K n'est donc **pas** un facteur d'accélération
disponible. Le terminal statique et les racines temporelles restent des
objets différents. Les clés uniques ne deviennent pas une obligation de
stockage du futur produit.

## Protocole et contrôles

Un seul processus de calcul est lancé à la fois par ce diagnostic.
`run.py` crée chaque répertoire de capture, conserve commandes, stdout,
stderr, codes de sortie, dates, durées et sources avant/après. Les sources
de la première tentative ne sont jamais remplacées. Le premier compile
a échoué sur Boost absent de `/usr/include` ; ses octets et son code 1
restent conservés. La relance utilise les en-têtes Boost déjà présents
dans `build/v7_boost_gate/extracted/usr/include`, sans installation réseau.

Les gates reprennent les 28 nuages / 112 ordres Gram/Gamma indépendants :
comparaison nominal/observé, puis cache activé/désactivé. Un oracle borné
par comparaisons de clés par paires vérifie U_K quand R_K<=4096 ; ce seuil
limite uniquement le juge quadratique, pas la collecte ou le moteur.
Planchers : K1, plusieurs clés, doublons, indice zéro, extra-shells et
descentes à rayon égal. Les mutants déplacent la collecte après cache,
ajoutent les semis, ajoutent les descentes, ou remplacent l'égalité exacte
par un hash délibérément constant. Les quatre mutants ont été compilés
avec succès puis rejetés (code1) : les trois premiers par
`observer.occurrence_delta`, le dernier par `observer.pairwise_unique`.
Aucun échec de compilation n'est compté comme un mutant tué.

La première exécution SAN a subi le refus environnemental explicite
`LeakSanitizer does not work under ptrace` et sort avec code1. Sa capture
reste intacte ; elle ne compte pas comme une gate réussie. Le rejeu autorisé
hors sandbox réutilise le même ELF (hash vérifié), mêmes sources et
`detect_leaks=1`, dans de nouveaux répertoires `*_san*_r2`.

Les deux rejeux SAN passent. Cache activé/désactivé, O2/SAN : même flux
de 112 lignes, 66 ordres avec doublons, 84 avec plusieurs clés, 86 avec
indice zéro et 4685 comparaisons exactes par paires. Le digest du flux est
`a6ef97a7d8d8f451121c5fd2a2198cef2812a800e662fc8bb87f3bf26254abfe`.
Le juge géométrique conserve ses 170 320 contrôles et 45 948 verticales.

Micro appariée n400, s8, K1..10, seed3, coord65536, un thread : tous les
compteurs géométriques et le payload nominal/observé sont identiques.
Cache actif et désactivé donnent R=346 727 et U=171 629 (50,500 % de
doublons), même digest des clés et même payload. Le cache donne 180 260
MEB contre 392 135 sans cache. U est donc déjà proche des MEB courantes,
et à K2 U=1958 dépasse même les 1071 MEB grâce aux semis/hits.
K1 contribue 2664 occurrences mais seulement 400 clés : les dénominateurs
hors K1 sont exactement R=344 063 et U=171 229, pas R total moins U1.
Cette distinction ne change pas le nombre de MEB puisque K1 n'en appelle
aucune. Elle évite de mélanger occurrences, minima et requêtes uniques.

Les deux mesures prioritaires sont `uniform` n8000 s8, puis
`scanline_overlap_multiecho` n2000 s8, toujours K1..10. Les répétitions
s10/12 du nouveau diagnostic sont différées à la demande ROOT compte
tenu de la charge, sans ajouter de plafond algorithmique. Elles ne sont
ni des échecs ni des mesures. Voir les captures et le bilan final dérivé.

## Résultats prioritaires clos

Les deux tours terminent, sources avant/après identiques. À 8k, **tous les
champs non temporels** coïncident avec le reçu publié du même moteur, pas
seulement le payload `cdd77e…` : 3 976 472 nœuds et verticales conservés.
Le fichier `historical_reference/pin.json` borne cette comparaison ; ce
reçu ancien n'est pas présenté comme une nouvelle mesure.

| Cas, cache activé | R total | U total | Doublons exacts | MEB du moteur |
| --- | ---: | ---: | ---: | ---: |
| Uniforme n400 | 346 727 | 171 629 | 50,500 % | 180 260 |
| Uniforme n8000 | 10 456 312 | 5 184 885 | 50,414 % | 6 227 265 |
| Scanline overlap n2000 | 94 400 | 50 412 | 46,597 % | 124 254 |

La scanline n2000 contient 3 726 extra-shells et neuf pas de descente à
rayon égal : le diagnostic structuré n'est pas vert par régularité totale.
Son payload est `d49a1f6a5cc9c613931781dc694c411c9024fbff163a9fb2709ab541a9de77f4`.

Pour l'uniforme 8k, ordre par ordre :

| K | R_K | U_K | MEB | Hits cache | Semis |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 59 750 | 8 000 | 0 | 0 | 0 |
| 2 | 161 128 | 46 136 | 30 245 | 134 774 | 29 875 |
| 3 | 308 189 | 114 555 | 94 253 | 229 935 | 63 353 |
| 4 | 500 741 | 211 968 | 199 834 | 339 355 | 108 117 |
| 5 | 739 105 | 337 252 | 347 692 | 463 383 | 164 195 |
| 6 | 1 018 342 | 488 952 | 539 581 | 596 523 | 231 663 |
| 7 | 1 338 921 | 666 230 | 778 850 | 736 942 | 308 825 |
| 8 | 1 697 911 | 868 194 | 1 063 132 | 884 134 | 396 424 |
| 9 | 2 097 866 | 1 096 161 | 1 396 931 | 1 036 552 | 493 388 |
| 10 | 2 534 359 | 1 347 437 | 1 776 747 | 1 194 745 | 600 806 |

Hors K1 : R=10 396 562 et U=5 176 885. Les 5 616 343 hits ont déjà
ramené les MEB initiales effectives à 4 780 219, **moins que U** ;
1 447 046 autres MEB viennent des descentes. Le futur dédoublonnage doit
conserver le bénéfice des semis et qualifier son coût résiduel. La moitié
de doublons observée n'est pas une promesse de diviser le temps par deux.

Coûts **instrumentés** : à 8k, 1022,29 s externes dont seulement 261,22 s
CPU utilisateur (26 % CPU), pic RSS 2 139 852 KiB, capacité maximale du
buffer de clés 167 772 160 octets (160 Mio). La scanline termine en 4,19 s
externes, 3,56 s CPU utilisateur (88 % CPU), pic RSS 32 896 KiB et capacité
des clés 655 360 octets. Ces différences de disponibilité CPU empêchent
toute comparaison de latence entre les familles. Aucun contrat 1 s/100 ms
ou plusieurs dizaines de millions n'est mesuré ici.

## Rejeu

`verify.py` vérifie le paquet sans compiler, exécuter un ELF ou accéder au
réseau ; aucune porte ne repose sur `assert`, et le mode Python `-O` doit
donner le même résultat. Le paquet portable lie les fichiers logiques à
des objets de contenu `.source` ; aucun binaire compilé n'est distribué.
Les sources complètes publiées et instrumentées ainsi que les mutations
permettent un nouveau rejeu dans un répertoire neuf. Les dépendances et
le compilateur utilisés sont épinglés dans `dependency_pins.json` et les
captures de préprocesseur/version ; les en-têtes système ne sont pas
réinterprétés comme sources du moteur.

Les scripts utilisés pour les captures sont conservés dans
`script_history/` avant ajout non sémantique de leurs annotations de type.
Ce nettoyage des utilitaires ne réexécute ni ne modifie les sources moteur,
les commandes ou les flux bruts. Le programme d'orchestration a évolué
entre étapes (chemin Boost, plan différé s10/12, reprise SAN) ; les commandes
effectivement capturées et les fermetures moteur avant/après font autorité,
pas l'hypothèse d'un unique script d'orchestration inchangé dès le premier
essai de compilation.

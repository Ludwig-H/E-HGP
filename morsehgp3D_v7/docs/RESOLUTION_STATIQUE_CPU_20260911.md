# Résolutions statiques dédoublonnées, voie CPU optionnelle

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Le défaut reste le resolver temporel avec cache. Cette voie ne certifie ni
la complétude WSPD, ni une archive industrielle, ni un contrat de performance.

Le delta initial décrit ci-dessous est épinglé au header `33e7d05e…`.
Son [complément après échange](SEMIS_APRES_ECHANGE_20260911.md) porte le
header actif `6763a877…` : mêmes semis complets, consultés aussi après chaque
échange. Les résultats historiques ne lui sont pas réattribués.

## Ce qui est maintenant implémenté

`build_full_ball_tower(index, balls, kmax, static_threads)` sélectionne la
voie statique lorsque `static_threads > 0` ; zéro conserve le chemin nominal.
Un nombre négatif est refusé. L'ancienne forme à trois arguments reste valide.
La sonde accepte l'option facultative `--static-threads=1` ou, par exemple,
`--static-threads=4`. `--threads` continue de piloter les phases amont : ne
pas confondre ces deux nombres avec K ni avec le facteur s de la WSPD.

Pour chaque ordre K≥2, le visiteur commun extrait seulement les représentants
stricts des blocs programmés, et non toutes les facettes de Gamma. Une requête
porte la clé triée entière, le bloc consommateur et son indice avant tri.
Le tri `(clé, indice initial)` groupe les répétitions ; chaque groupe calcule
une seule cible géométrique, indépendante des composantes courantes. Les
trajectoires de descente sont transitoires, locales à un worker.

Le terminal est une **BallId**, jamais un jeton union-find. La présence de sa
clé dans le catalogue ne suffit pas : K doit appartenir à l'intervalle
`[p+q_min−1, p+u]`. Le census validé fournit `arity=q_min`. Les semis statiques
I union U sont permis à K=p+u ; leur MEB est alors la boule certifiée du census.
La table de semis est immuable et partagée. Aucun cache 16n n'est répliqué par
thread ; le cache de jetons du chemin nominal n'est pas utilisé dans cette voie.

Les cibles sont réécrites dans l'ordre initial des occurrences. Le calendrier
nominal reprend ensuite : vérifier pour chaque occurrence que sa cible est
strictement antérieure au consommateur et que son ancre est fermée, normaliser
la racine pré-lot, puis fermer atomiquement le lot entier. Parents, contributions
datées, portails inertes et verticales historiques restent calculés comme avant.
K1 garde son traitement sans MEB. Les blocs sans représentant, dont la naissance
terminale K=n, ne sont pas supprimés.

La [preuve de l'auditeur](../audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md)
justifie cette séparation **sous census exact complet**. Elle ne promet pas de
gain ni d'indépendance des compteurs au choix des intrus. Le parcours nominal
du premier intrus reste utilisé ; le prototype droit du 10 septembre n'est
pas intégré dans ce delta, pour séparer leurs effets.

## Exactitude et travail réellement exécuté

Le [paquet de qualification CPU](../receipts/static_resolution_cpu_20260911/README.md)
épingle le header initial `33e7d05e…`, exactement celui du prototype qualifié.
O2 et ASan/UBSan avec détection des fuites : 30 nuages, 124 ordres,
3 324 coupes et 75 136 vérifications verticales dans chaque voie statique.
S'ajoutent 4 498 contrôles physiques appariés des nœuds, tableaux de parents
et successeurs, banques, contributions et images inférieures. La gate nominale
intégrée conserve ses 28 nuages ; la variante Kmax6 du cercle inadmissible à
K4 est ajoutée dans les modes statiques. Le catalogue commun ne vaut pas admission.

Le [reçu indépendant des mutants](../receipts/static_resolver_mutants_20260911/README.md)
utilise six census rationnels, deux remappages, 46 ordres, 1 544 coupes,
1 506 images verticales et 2 524 signatures entières, sans Boost dans le driver.
Six mutations physiques compilent puis sont réfutées : admission sans K,
consultation prématurée des ancres, scatter selon le tri, occurrences perdues,
absence de normalisation et admission du travail après lancement partiel du pool.
Ce dernier contrôle exige zéro MEB payée avant admission et aucun worker vivant
après l'échec. Un échec invalide toute la sortie, jamais un préfixe publié.

Le [complément après admission](../receipts/static_worker_failure_20260911/README.md)
injecte une vraie panne d'allocation de la pile d'un worker après sa première
MEB. O2 vérifie deux pannes, la sortie vide, les jointures, le travail payé
et la réutilisation ensuite ; le mutant omettant la réduction des stats est
réfuté. Sa première capture SAN reste un refus LSan/ptrace inchangé.
Le [nouveau rejeu indépendant](../audits/receipts_static_followup_20260911/README.md)
exécute depuis le même binaire SAN, fuites activées : les deux pannes,
jointures et réutilisation passent. Ce résultat distinct ne qualifie ni
le nouveau header après échange ni l'absence générale de races.

Le premier contrôle ASan sous sandbox a échoué à cause de ptrace ; le même
binaire a été rejoué hors sandbox, sans désactiver LeakSanitizer. Une erreur
initiale du juge C++ comparant deux tableaux natifs au lieu de leurs éléments
est aussi conservée, puis corrigée. Aucun de ces échecs n'est effacé ou compté
comme mutant réfuté. Les 24 CTests du raccord actif sont capturés séparément,
ainsi que la compilation et le lien CUDA strict de la sonde hybride.
Aucun device n'a été exécuté par cette compilation.

## Premières mesures appariées, pas de contrat chronométrique

Uniforme u16, graine 3, s8, toute la tour K1..10 et ses verticales retenues.
Les paires utilisent le même binaire privé figé, avec option 0/1/4 ; les
sorties et compteurs du calendrier coïncident. Les MEB/supports/uniques sont
identiques à un et quatre threads. Les temps viennent d'un hôte très chargé,
avec une seule observation par configuration : pas de gain statistique attribué.

| n | MEB nominales | MEB statiques | Supports nominaux | Supports statiques | Buffers statiques retenus, 1 thread |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 400 | 180 260 | 132 750 | 15 708 447 | 11 072 339 | 9 621 256 octets |
| 1 000 | 583 337 | 406 134 | 52 168 577 | 34 607 823 | 19 577 344 octets |

Cela réduit les MEB de 26,4 % puis 30,4 %, et les supports de 29,5 % puis
33,7 %. À n1000, FULL prend 30,708 / 26,080 / 17,140 s pour 0/1/4, mais
le total 67,870 / 68,757 / 51,340 s subit aussi les variations de charge amont.
Ne pas convertir ces observations en accélération robuste ou contrat 50k.

Le [diagnostic indépendant des clés initiales](../receipts/initial_representatives_20260911/README.md)
trouve à 8k/s8
10 456 312 occurrences et 5 184 885 clés uniques, K1 compris ; hors K1 :
10 396 562 / 5 176 885. Le moteur publié y calcule pourtant seulement
4 780 219 MEB initiales grâce aux semis/cache, puis 1 447 046 MEB de descente.
**Le rapport occurrences/uniques n'est donc pas un facteur de vitesse.**
Les tableaux `static_orders` de la sonde excluent K1 : leurs zéros à cet ordre
ne signifient pas que K1 n'a aucun représentant.

La [comparaison courante s8/10/12](../receipts/static_s_factors_20260911/README.md)
est close à 8k, un thread amont et quatre statiques : mêmes 3 976 472 nœuds,
verticales, calendrier et neuf lignes statiques par K ; 4 185 184 MEB et
364 590 166 supports pour chacun des trois s. Seuls les nombres de candidats
amont changent, 3 144 017 / 3 129 992 / 3 123 497, hors s et temps.
Les disponibilités CPU mesurées diffèrent fortement : aucun optimum de
latence n'est retenu. Ces résultats ne sont pas le diagnostic R/U instrumenté
du moteur nominal, qui ne mesurait que s8.

## Triplet local 8k/16k/32k clos

Le [reçu d'échelle statique](../receipts/static_resolution_scale_20260911/README.md)
conserve les trois tours K1..10, s8, un thread amont et quatre statiques.
Les 35 champs de calendrier/sortie comparés au moteur nominal du 10 septembre
coïncident à chaque taille, dont les payloads et toutes les verticales.
Les compteurs physiques proviennent de la même sonde privée figée que les
micros ; son header est identique au raccord initial `33e7d05e…`, ses métadonnées restent
distinctes de la sonde active. La référence nominale n'est pas rechronométrée.

| n | MEB nominales | MEB statiques | Réduction des MEB | Réduction des supports | Capacités statiques retenues (octets) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 6 227 265 | 4 185 184 | 32,79 % | 36,37 % | 307 936 444 |
| 16 000 | 13 252 780 | 8 779 465 | 33,75 % | 37,44 % | 617 052 880 |
| 32 000 | 27 711 509 | 18 244 853 | 34,16 % | 37,91 % | 1 235 849 528 |

Les supports statiques sont respectivement 364 590 166, 767 853 710 et
1 600 773 173. Les demandes hors K1 croissent de 10 396 562 à 21 827 082 puis
45 208 799 ; les uniques, de 5 176 885 à 10 862 732 puis 22 503 000.
Les exposants locaux `log2(W(2n)/W(n))` valent 1,069 puis 1,055 pour les MEB,
1,075 puis 1,060 pour les supports, 1,070 puis 1,050 pour les demandes.
Ce sont des observations de travail sur **cette famille uniforme u16**, pas
des bornes universelles, ni des exposants de latence sur l'hôte partagé.
À 32k, 20 095 lots groupés exercent 40 376 slots DSU, sans extra-shell census ;
les lots simultanés ne sont donc pas tous évités dans cette campagne.

Les temps totaux observés sont 466,761 / 686,049 / 802,278 s, dont
150,412 / 123,976 / 217,110 s pour FULL. Les disponibilités CPU diffèrent :
l'ordre apparent des temps n'établit pas un résultat de scaling exploitable.
Le pic RSS à 32k est de 9 109 204 KiB, contre 9 108 756 dans la capture nominale
historique : aucune économie RSS n'est revendiquée. Le reçu fournit les
captures et ratios complets, sans extrapolation à 50k ou aux dizaines de millions.

## Résidence, complexité et limites

Les buffers sont préparés puis libérés ordre par ordre. Pour R occurrences,
U clés uniques et S semis de l'ordre, tri et regroupement demandent
O(KR log R + KS log S), hors génération des représentants et descentes exactes.
La mémoire temporaire est O(KR+KS+U+R), plus les piles des workers ; aucune
longue chaîne de facettes ni catalogue Gamma n'est conservé. Cela ne borne
pas R ou S en fonction de n pour tous les régimes 3D.

Dans l'ABI testée, une requête prend 56 octets, une seed 44 octets et une cible
4 octets ; les capacités géométriques des vecteurs peuvent dépasser leurs
tailles. Les champs de mémoire sont des **capacités retenues échantillonnées**,
pas le pic des réallocations, le coût total du pipeline ni le RSS. Le cache
nominal demande seulement 786 432 octets à n1000 : le travail économisé a
donc un coût de résidence supplémentaire. Le nombre de threads créés ne
compte que les pools terminés ; une lane séquentielle utilise le caller sans
créer de thread. Les créations partielles en échec ne sont pas présentées
comme un compte complet de threads.

Les mesures statiques 8k/16k/32k sont closes et restent distinctes des micros.
Aucun résultat massif ou GPU ne découle de ce port CPU. Le coût
du journal et de la génération WSPD reste à réduire ; le
[journal incrémental](../receipts/incremental_journal_prototype_20260911/README.md)
est encore un prototype séparé. Le [port GPU des résolutions](PORT_GPU_RESOLUTIONS.md)
distingue sélection MEB, terminal et raccord de tour.
Les [mesures 50k du 10 septembre](RESULTATS_TOUR_CACHE_G4_20260910.md)
restent environ 419 s K1..10 / 33,6 s K1..5, et ne sont pas réattribuées
à cette voie statique. Les contrats 1 s, 100 ms et dizaines de millions ne
sont pas acquis. GCP non utilisé pour cette étape.

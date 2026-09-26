# Cache S2 sur G4 : gain confirmé, mais pas le saut vers 100 ms

26 septembre 2026. Code testé : `d0e711e23617a7fa2838cac9b370ff125d4be5fb`.
Cadre : `exploration_v9_hors_registre`, `cuda_g4`,
`quantized_u18_input_only`, `audit_s2_tile_cache`, `not_claimed`.
L'auteur a participé au port : ce rapport ne constitue pas un audit
indépendant de son propre code. Les contrôles portables, le modèle
d'indices et le lecteur ont été contre-vérifiés par des agents distincts.

## Verdict utile au développeur

Le cache de témoins par tuiles fonctionne sur les cas mesurés et améliore
le filtre GPU, mais **son gain net est de quelques millisecondes**.
Conserver l'option et ses tests, sans la présenter comme une refonte de
complexité ou l'activer par défaut dans FULL. Ne pas consacrer une nouvelle
campagne à ajuster sa largeur pour quelques pourcents : le front, les
recensements q3/q4 répétés et la phase A de tous les ordres FULL restent
des chantiers beaucoup plus importants.

Le [reçu brut](../receipts/g4_tile_cache_20260926/README.md), le
[plan figé](b_g4_tile_cache_20260926/README.md) et la
[note de complexité](b_g4_tile_cache_20260926/COMPLEXITE_ET_PERIMETRE.md)
séparent preuve, mesure et limites.

## Résultats appariés

Les temps ci-dessous sont des médianes des deux passages chauds de chaque
processus, puis des processus de chaque bras. Pour 08/000000 K5/s8,
l'ordre est référence–cache–cache–référence, soit deux processus par bras.
Les autres lignes n'ont qu'un processus par bras. Les 42 passages,
premiers passages inclus, restent publiés : pas de mélange des meilleurs
sous-temps et pas de prétendue indépendance des répétitions internes.

| trame, K, s | paires GPU, référence → cache | filtre complet, référence → cache | travail géométrique divisé par |
| --- | ---: | ---: | ---: |
| 08/000000, K5, s8 | 29,044 → 25,441 ms | 64,066 → 60,764 ms | 1,592 |
| 08/000100, K5, s8 | 16,206 → 15,433 ms | 43,665 → 42,588 ms | 1,339 |
| 08/000200, K5, s8 | 29,724 → 27,488 ms | 64,909 → 62,214 ms | 1,359 |
| 08/000000, K10, s8 | 49,740 → 46,243 ms | 104,615 → 100,642 ms | 1,358 |
| 08/000000, K5, s10 | 24,256 → 21,645 ms | 56,586 → 54,000 ms | 1,500 |
| 08/000000, K5, s12 | 20,950 → 19,010 ms | 53,719 → 51,410 ms | 1,437 |

« Paires GPU » inclut les deux kernels, production des témoins puis
application/repli. « Filtre complet » inclut upload, filtre des rectangles,
scans et préparation des tuiles, paires et download selon les events CUDA
de cette sonde ; ce n'est **ni le mur du processus ni toute la tour**.
Le travail géométrique est `pair_visits + cache_node_tests` ; les visites
des représentants sont déjà dans `pair_visits`.

Sur 08/000000 K5/s8, les 23 686 751 paires restent toutes traitées.
Les parcours passent de 1 110 657 775 à 644 863 628 visites, mais le cache
ajoute 52 772 301 retests. Le total est donc 697 635 929, pas le seul
compteur de visites résiduelles. Les 756 365 traces, leurs masques et les
deux tableaux par rectangle ajoutent 117 457 589 octets logiques, hors
scratch et capacités d'allocateur. Ce n'est pas une mesure du pic VRAM.

La diminution de travail est nettement plus forte que le gain temporel.
Les accès supplémentaires, la recherche des rectangles et les replis
restent à payer ; leur part exacte n'a pas été profilée. Ne pas inventer
de taux d'occupation GPU ni attribuer causalement tout l'écart à un de ces
postes sans mesure. Les ratios du prototype CPU échantillonné ne sont
pas une prédiction des temps GPU de trames entières.

## Exactitude contrôlée, sans surqualification

Les 14 cas terminent, soit 291 690 754 masques de paires comparés au CPU
sur les dernières passes ; les masques et les six compteurs sont aussi
identiques entre les trois passes de chaque cas. Zéro divergence de
rectangle, de paire, de répétition ou de pile. Les deux préflights
référence/cache passent ; chacun détecte exactement le masque corrompu
par son mutant causal. Le petit préflight cache peut être vide en tuiles ;
les cas LiDAR complets sont tous non vides.

La revalidation locale fraîche est distincte : 98 784 requêtes de cache,
3 176 traces, oracle ponctuel, mutant sans retest ; 336 196 paires pour
le modèle d'indices, avec mutant franchissant une ligne ; cinq gates
filtre/cache et deux refus de sonde passent en Release. Protocole :
13 tests normal et 13 sous `-O`. Cette reprise n'a pas rejoué ASan/UBSan ;
ne pas reconstruire une qualification sanitizer depuis les seuls souvenirs
des anciens builds temporaires disparus.

Ce reçu contrôle S2 sur le front fourni. Il ne démontre pas à lui seul la
complétude indépendante du générateur, et il n'exécute pas le compactage
des survivants de `run_filter_batch` avec cache, le catalogue ou FULL.
Cette dernière porte est nécessaire avant activation dans la chaîne.

## Décisions pour la suite 100 ms

1. **Garder** la trace exacte bornée, les témoins retestés, le repli depuis
   zéro, les tuiles indépendantes et le chemin sans cache comme référence.
   L'option demeure désactivée par défaut.
2. **Prioriser le front GPU compact**, déjà étudié au niveau des objets :
   éviter une construction/émission CPU puis un nouveau parcours GPU
   des mêmes rectangles. Les compteurs doivent être privés ou réduits ;
   copier simplement l'objet `Front` partagé serait une course.
3. **Transporter les intérieurs déjà recensés de q3/q4** jusqu'au catalogue,
   avec provenance et cardinalité exactes, pour éviter leur recherche
   répétée. Le gain isolé du consommateur précédent n'est pas encore un
   gain net producteur–consommateur.
4. **Alléger la phase A de tous les ordres FULL**, avec sorties explicites
   et portails silencieux nécessaires aux parents conservés. La seule
   parallélisation du tri ou l'allégement du dernier ordre ne ferme pas
   cette étape.

Le meilleur filtre isolé à s12 ne justifie pas de modifier le s par défaut
de la tour : il exclut le coût du front, q2, l'aval et leur contention.
Les reçus FULL R24-B restent l'autorité pour cette comparaison de chaîne.
Le cache ne réduit pas P : aucune nouvelle preuve sous-quadratique et
aucun nouveau chrono FULL ne sont acquis ici. Les coupes LiDAR, les
séries 8k/16k/32k CUDA, le brut et les autres séquences restent à qualifier.

## Coût et fermeture

Une seule session SPOT sur la cible G4 fixe. Génération
`2026-09-26T12:20:06.956-07:00`, dernier arrêt
`2026-09-26T12:24:29.680-07:00` : environ 4 min 23 s entre ces deux
horodatages GCE, pas une estimation de facture. L'arrêt ciblé réussit et
une lecture GCE indépendante confirme `TERMINATED` sur la même génération.
Compilation CUDA : 9,58 s ; worker complet : environ 103,71 s.

Un premier appel local avait refusé la clé éphémère de mode 0644 avant
toute commande GCP ; son mode a été corrigé à 0600, puis le contrôleur a
été relancé. Ce refus local n'est pas un échec géométrique ni une seconde
session payante. Aucune installation CUDA, aucune donnée KITTI nouvelle
et aucune clé privée ne sont publiées dans la v9.

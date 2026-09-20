# Boules exactes et candidats d'une seed — preuves du 20 septembre 2026

`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. GCP non utilisé.

Deux objets nouveaux : une clé canonique exacte de boule, et les présentations
positives q3/q4 issues d'une **seed aiguë fournie**. Les candidats portent une
profondeur stricte et une coquille complète ; les IDs des intérieurs ne sont
pas encore collectés. **Ni énumération globale des seeds, ni catalogue dédupliqué,
ni FULL, ni contrat G4.** La cardinalité d'une présentation n'est pas une preuve
de l'arité minimale de sa boule parmi tous les sites de coquille.

## Captures closes

| Capture | Périmètre | Résultat |
|---|---|---|
| [smoke_c7kdqo3w](smoke_c7kdqo3w/COMPLETION.json) | Deux gates Release et six sondes n32, K5/10 | PASS |
| [smoke_jy7_uv8w](smoke_jy7_uv8w/COMPLETION.json) | Deux gates Clang ASan/UBSan et six sondes n32 | PASS |
| [scale_hu1pvc8o](scale_hu1pvc8o/COMPLETION.json) | Deux gates Release et 18 mesures 8k/16k/32k, K5/10, CPU0 | PASS |
| [regression_44jbb9h5](regression_44jbb9h5/COMPLETION.json) | Suite complète Release, 84 CTests | 84/84, 128,92 s |

Les 144 sources, les binaires et caches effectivement utilisés sont épinglés
avant/après chaque capture. La suite historique entière n'a **pas** été rejouée
sous sanitizers : la qualification Clang ASan/UBSan porte sur les deux nouvelles
gates et les six nouvelles sondes. Aucune nouvelle qualification TSan ici.

Les quatre lectures avec contrôle des fichiers présents et le test du lecteur
ont été rejoués en normal et −O : **dix commandes, sorties identiques**, dans
[readers_ftz5uj0g](readers_ftz5uj0g/COMPLETION.json). Cette clôture recontrôle
144 sources, 56 artefacts et 47 fichiers d'entrée ; son
[résumé](readers_ftz5uj0g/SUMMARY.json) conserve les croissances sans filtre.
Le test du lecteur rejette vingt corruptions ciblées de types, commandes,
comptes, digest, non-vacuité et fermeture.

Les préflights restent dans [PREFLIGHT.md](PREFLIGHT.md). Un complément distinct
a compilé puis tué **quatre mutations causales du produit**, après réussite
des deux baselines : voir [MUTANTS.md](MUTANTS.md) et
[sa capture](mutants/compiled_4cqzufx_/COMPLETION.json). Ses preuves ne sont
pas confondues avec les corruptions du lecteur ou celles du juge.

## Couverture géométrique

La gate `ExactBall` compare les fabriques à une résolution rationnelle
indépendante : 1 807 cas, 34 460 contrôles, 453/232/192 supports q2/q3/q4
acceptés et 930 refus. Elle vérifie 12 675 signes de puissance, 496 permutations,
les centres sur une face, les supports plats et des boules communes entre
arités. Les coefficients et puissances exercés atteignent 95 bits ; un calcul
naïf de rayon intermédiaire atteint 160 bits. Trois corruptions du jugement
sont rejetées, sans les présenter comme mutations compilées du moteur.

La gate des candidats couvre 635 appels et 24 590 contrôles : 2 451 complétions
et 24 351 tests de sites de l'oracle, 488 candidats dont 405 q3 et 83 q4.
Treize petits nuages sont en outre comparés à l'énumération exhaustive,
pour 229 boules canoniques. Sont positivement exercés :

- six appels qui émettent q4 malgré le rejet q3 ;
- quatre groupes dont une présentation valide arrive après un premier essai
  invalide, et 104 présentations restant volontairement inexplorées après
  l'émission d'un représentant valide ;
- coquille de 30 sites, 26 refus de seed canonique, 569 refus de positivité
  et 869 groupes trop profonds ;
- une présentation d'arité supérieure au minimum de sa boule ;
- six entrées invalides, callback levant et quatre appels indépendants concurrents.

Le compteur `depth_drop_runs=28` désigne des appels contenant à la fois un
groupe trop profond et une émission ; **il ne prouve pas leur ordre temporel**.
Une fixture nommée distincte exerce explicitement une racine profonde suivie
d'une racine admissible. Les appels concurrents indépendants ne constituent
pas une parallélisation du traitement d'une seed.

## Mesures non vacues, mais volontairement locales

Chaque nuage contient la même seed et le même tétraèdre positif près de
(1 000, 1 000, 1 000), deux intérieurs stricts, puis un fond lointain. Ce fond
est uniforme dans une boîte distante, disposé sur deux lignes, ou coplanaire
au seed. **Ce ne sont pas des mesures d'un générateur q3/q4 sur un nuage
entièrement uniforme représentatif.**

L'oracle fermé attend dans chaque configuration exactement une présentation
q3 et une q4, toutes deux de profondeur 2, survivantes à K5 et K10. Les clés
primitives attendues sont `(1,-2000,-2045,-2015,3050000)` et
`(1,-2000,-2050,-2000,3040000)` ; coquilles `{0,1,2}` et `{0,1,2,3}`.
Les sites distants ne peuvent former d'autre complétion acceptée par ce raccord : ils sont
hors de la lentille de l'arête propriétaire. La sonde vérifie ses deux boules
par **2n évaluations scalaires indépendantes**, pas par n tests à chaque racine.
Le callback lit exactement sept IDs de support et sept IDs de coquille.
Le digest, également recalculé par le lecteur Python, vaut
`9232636534885143249` dans les 30 mesures closes.

Une observation par configuration, CPU0 de l'hôte partagé. Le constructeur
n'exécutait ni compilation, ni campagne de régression, ni mutants durant
la campagne d'échelle ; une charge extérieure reste possible. « Seed » inclut
census q3, événements/tri/balayage q4, cascade de validité, callbacks de
vérification et allocations/libérations internes. « Total » ajoute génération,
préparation du propriétaire, juge, validation et libération ; hors JSON.

| Fond lointain | n | Seed K5 | Total K5 | Seed K10 | Total K10 |
|---|---:|---:|---:|---:|---:|
| Uniforme | 8 000 | 3,710 ms | 5,780 ms | 3,781 ms | 5,721 ms |
| Uniforme | 16 000 | 7,912 ms | 12,395 ms | 7,982 ms | 12,537 ms |
| Uniforme | 32 000 | 16,873 ms | 26,673 ms | 16,736 ms | 27,039 ms |
| Deux lignes | 8 000 | 3,455 ms | 4,769 ms | 3,450 ms | 4,786 ms |
| Deux lignes | 16 000 | 6,961 ms | 9,841 ms | 7,008 ms | 9,868 ms |
| Deux lignes | 32 000 | 14,673 ms | 20,807 ms | 14,809 ms | 20,956 ms |
| Coplanaire | 8 000 | 0,101 ms | 1,461 ms | 0,100 ms | 1,441 ms |
| Coplanaire | 16 000 | 0,201 ms | 3,192 ms | 0,204 ms | 3,297 ms |
| Coplanaire | 32 000 | 0,393 ms | 6,985 ms | 0,389 ms | 6,897 ms |

## Travail, croissance et mémoire

Les événements et comparaisons de tri sont identiques entre K5 et K10 :

| Fond | Événements 8k/16k/32k | Comparaisons de tri | Rapports du tri |
|---|---|---|---|
| Uniforme | 7 997 / 15 997 / 31 997 | 125 856 / 272 754 / 580 481 | ×2,167 / ×2,128 |
| Deux lignes | 7 997 / 15 997 / 31 997 | 123 584 / 257 612 / 548 159 | ×2,085 / ×2,128 |
| Coplanaire | 3 / 3 / 3 | 5 / 5 / 5 | ×1 / ×1 |

Le fond coplanaire n'est pas un q4 entièrement plat : les quatre sommets
locaux forment toujours le tétraèdre positif, et les deux intérieurs produisent
les deux autres événements. Le scan des n sites reste payé.

Dans les deux premiers régimes, la cascade n'examine que six présentations
à K5 et onze à K10, indépendamment de n. Trois et huit sont rejetées par
l'ownership ; les trois présentations restantes donnent deux refus de
positivité et le q4 attendu. Le fond coplanaire ne fait examiner que ces
trois dernières présentations. Le census q3 lit n points exactement ; q4
lit également n sites et balaye tous ses groupes, sans arrêt au premier rejet.

Les réservations familiales sont 128 000/256 000/512 000 octets pour
8k/16k/32k, dans les trois régimes. Le buffer de coquille q3 occupe 32 octets,
libérés avant les réservations q4. Entrée et propriétaire sont rapportés
séparément : il s'agit de capacités, **pas du RSS ni d'un pic mémoire global**.

Le coût `O(n + e log(1+e))`, avec e≤n, est acquis **par seed fournie**, avec
sortie locale et callback de cette sonde. Cela ne borne ni le nombre de seeds,
ni les produits WSPD résiduels, ni les sorties globales. Les doublements
favorables de cette campagne ne ferment donc pas P0 ou une borne générale
sous-quadratique. Ils ne qualifient aucun contrat 50k/G4 ou multi-millions.

## Rejouer

Builds épinglés : `build/v8_q34_candidates_20260920` et
`build/v8_q34_candidates_sanitize_20260920`. Ne pas les écraser.
Le [runner](../../bench/run_q34_seed_checks.py) fournit `run` avec les campagnes
`gate`, `smoke`, `scale` et `regression` (cette dernière en Release uniquement),
puis `read CAPTURE --check-live` et `selftest CAPTURE_SMOKE`.
[close_reads.py](close_reads.py) consigne les lectures normal/−O et leurs
empreintes sans toucher aux sources gelées. Le lecteur historique, sans
`--check-live`, reste utilisable après les prochaines modifications.

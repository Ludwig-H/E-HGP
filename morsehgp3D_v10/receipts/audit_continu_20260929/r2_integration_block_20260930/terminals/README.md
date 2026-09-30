# Capture close R2 : Pool et SiteTree

30 septembre 2026. Cette capture conserve des tests déjà exécutés par le
développeur et leur contre-audit en lecture seule. Aucun nouvel essai,
moteur, compilation, GCP ou arrêt de processus n'a été lancé. Aucun fichier
du dépôt ni paquet précédent n'a été modifié. `public_status=not_claimed`.

## Terminaux observés

| Groupe | Preuve | Résultat | Dernière écriture UTC |
|---|---|---|---|
| Pool | [Tour et catalogue](pool/diff/results.txt) | 24/24 configurations identiques, codes natifs 0, DONE | 05:02:54 |
| Pool | [Clustering et mutual reachability](pool/diff/results_head.txt) | 24/24 configurations identiques, codes natifs 0, DONE | 05:07:58 |
| Pool | [Tests ciblés](pool/logs/ctest_pool11.txt) | 11/11 réussis ; résumé CTest complet, pas de code du wrapper archivé | 04:42:41 |
| Pool | [Oracles](pool/logs/ctest_oracle.txt) | 2/2 réussis, code explicite 0 | 05:12:15 |
| SiteTree | [Tour instrumentée](sitetree/runs/diff_tour.txt) | 7/7 identiques, rc=0 | 04:55:29 |
| SiteTree | [Têtes instrumentées](sitetree/runs/head_diff_instr.txt) | 18/18 identiques, HEADDIFF_INSTR_EXIT 0 | 04:51:39 |
| SiteTree | [Tests gate](sitetree/runs/ctest_gate_r2.txt) | 11/11 réussis, ctest_rc=0 | 05:01:10 |

Le premier différentiel Pool compare base à quatre fils contre R2 à
un, trois et huit fils : six nuages, K5/K10, tour et catalogue, soit
96 exécutions pour 24 configurations. Les deux dernières configurations
sont la tour et le catalogue sur `lidar00_full.u32le` à K10.
Ce travail long vérifie la conservation des sorties sur une entrée réelle ;
il ne mesure ni un contrat G4 ni une loi de croissance.

Les [observations de processus](process_observations.json) montrent le
passage effectif de la dernière tour au dernier catalogue, puis les
nouvelles campagnes tête/oracle. À 05:13:06, aucun processus de ces deux
groupes n'est encore présent dans la capture. Les codes viennent des
terminaux ci-dessus, pas de la seule disparition des PID.

## Comparaisons exactes : limites des archives

Le [comparateur Pool tour/catalogue](pool/diff/run_diff.sh) conserve
seulement les 24 premiers caractères du SHA256 (96 bits), puis supprime
chaque dump. Il écrit DONE même si une ligne est ECART. Ici, les 24 lignes
ont toutes un code 0, une empreinte non vide et IDENTIQUE ; le code du
script seul ne suffirait pas à l'établir.

Le [comparateur des têtes](pool/diff/run_diff_head.sh) conserve des
préfixes de 16 caractères (64 bits). HGP compare labels et arbre ;
mutual reachability compare labels et stdout brut. Ce ne sont pas des
comparaisons octet par octet. Les [sources du différentiel SiteTree](sitetree/diff_tour.py)
comparent en mémoire le SHA256 complet et les compteurs mono, mais leurs
logs imprimés ne conservent qu'un préfixe ; les dumps sont également retirés.

L'[inventaire des sorties restantes](remaining_outputs.json) n'a trouvé,
à 05:16:06, aucun dump, label ou arbre dans les chemins sélectionnés.
Les deux grands dumps historiques visibles à la première inspection
(385 Mo et 143 Mo) n'ont été ni relus ni copiés et avaient disparu lors
de l'inventaire suivant. Cet audit n'a rien supprimé. Ils ne récupéreraient
pas les 24 sorties effacées à chaque passage par le comparateur.
Aucun SHA256 complet des 24 configurations Pool ne peut donc être
reconstitué sans de nouveaux essais ; aucun préfixe n'a été présenté
comme une empreinte complète.

## Arrondi : produit correct, juge de chemin incomplet

La source R2 appelle bien `filtered()` dans `nearest` et `closed_ball`.
Le [produit instrumenté](sitetree/preuves/instr_r2_porte.stderr) a zéro
filtre exécuté en arrondi dirigé. Le [mutant instrumenté](sitetree/preuves/instr_mutant_chemin_porte.stderr)
en exécute 84 028, mais son [test](sitetree/preuves/instr_mutant_chemin_porte.stdout)
passe aussi. Le [mutant naturel](sitetree/preuves/mutant_naturel_in_domain.cpp.patch)
montre le contournement : le diagnostic public garde son contrôle d'arrondi,
tandis que les deux requêtes utilisent seulement le domaine géométrique.
Sa sortie de porte est aussi conservée.

Le [tableau des mutants](sitetree/preuves/mutants_verificateur_r2.tsv)
contient huit survivants, notamment ces chemins et des drapeaux de domaine.
C'est une faiblesse de couverture des tests, pas la démonstration d'un
défaut dans le produit actuel. La vérification du chemin réellement pris
et des fixtures isolant chaque axe et chaque seuil restent pertinentes.

## Timeout de la sonde à barrière

Le [résumé des sondes](pool/sondes/out/san_probes.txt) conserve le timeout
ASan de `contre_pool` (124) et sa réussite TSan (0).
La [sonde originale](pool/sondes/contre_pool.cpp) réutilise une barrière
à quatre participants à chaque callback. Si le fil qui va lever tarde
plus de 5 ms après une première phase, les trois autres callbacks peuvent
entrer dans la suivante avant la capture de l'exception et attendre
un participant qui ne reviendra pas.

La [variante de diagnostic à attente bornée](pool/mine/contre_pool_delay.cpp)
rend cette hypothèse explicite. Nous n'avons exécuté ni cette variante
ni un nouveau rejeu : l'explication est une analyse des sources, pas une
nouvelle preuve expérimentale. Le timeout ne démontre pas un blocage du Pool.

## Provenance

[receipt.json](receipt.json) donne les scopes et limites ;
[source_inventory.json](source_inventory.json) lie 43 copies à leurs
chemins d'origine, dates exactes en nanosecondes et SHA256.
Toutes les copies ont été comparées à l'origine : mêmes octets et dates.
[observed_binaries.json](observed_binaries.json) contient les SHA256
complets de 14 binaires observés, sans les copier ni les exécuter.

Les copies `final/src` Pool et `r2/src` SiteTree correspondent aux
groupes R2 respectifs lors du contrôle. Les variantes R1/hybride et les
sources instrumentées sont distinctes et conservées. Les caches CMake
rattachent les builds Release aux copies sources. Ce sont des empreintes
postérieures aux builds, pas une preuve de gel préalable de toutes leurs
dépendances.

Le manifeste SiteTree d'origine, vérifié depuis son dossier `preuves/`,
est conservé comme pièce de provenance : cette capture n'embarque pas
tous les fichiers qu'il référence. Notre propre `SHA256SUMS` ferme
l'ensemble présent ici. Aucun chrono FULL/G4, résultat de clustering
contre ground truth ou caractère sous-quadratique nouveau n'est acquis.

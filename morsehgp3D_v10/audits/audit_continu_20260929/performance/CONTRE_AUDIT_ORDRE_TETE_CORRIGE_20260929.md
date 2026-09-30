# Contre-audit corrigé ordre/tête — 29 septembre 2026

Clôture de lecture : 22:32:25 UTC. Exploration CPU locale, hors registre, not_claimed.
Périmètre : builds globaux `/workspaces/E-HGP/build/v10-perf/ordre_tete-verif/` et `ordre_tete/`.
Aucun moteur reconstruit, aucune campagne longue/GCP relancée, aucun processus tiers arrêté, aucun Git publié.
Les journaux `observed_*` sont des exécutions d'autres acteurs **lues**, non nos exécutions.

## Résultat qui change le statut

Défaut reproductible du nouveau `validate(d, pool)` sur un objet public malformé : une tranche tardive déréférence un CSR hors bornes alors que le parcours série refuse un défaut antérieur. La preuve distingue explicitement l'objet valide (accepté dans les deux modes) de l'objet malformé. Elle ne démontre aucun défaut du producteur normal de forêt.

Les gains ordre/assemblage sont réels sur les reçus locaux : −59,1 % à K5 et −62,2 % à K10 sur ces étages. Le processus cluster K5 seul gagne environ **8,5 % de mur**, non un facteur 2 sur FULL. Les temps sont des temps **muraux**, pas CPU·s. Aucun nouveau contrat G4/FULL/100 ms, aucune borne globale de croissance.

Le CTest courant `tout` a finalement clos à **22:31:52 UTC, 9/9, CTEST_EXIT 0**, 1046,93 s réelles. Cela n'efface pas le défaut de validation découvert hors de ses fixtures.

## État vivant puis clôture, sans attente artificielle

À 22:29:23 UTC, PID **819590** existait, exécutable `/usr/bin/ctest`, commande ciblée `ctest --test-dir .../build/tout -L gate --output-on-failure`, fd1/fd2 pointant vers `out/ctest_gate_tout.txt`, enfant 844128. Son journal avait **2/9 terminés**, tower_oracle en cours. Captures `LIVE_CTEST_20260929T222923Z.json` et `LIVE_CTEST_LOG_20260929T222923Z.txt`.

Une lecture de clôture distincte, à 22:31:56, a trouvé le vrai marqueur `CTEST_EXIT 0` et la fin datée 22:31:52 ; `LATE_GATE_AND_BUILDS_20260929T223155Z.txt` le conserve. À 22:32:25, `/proc/819590` n'existait plus (`FINAL_PID_819590.json`). Le 9/9 historique de `ordre_tete/out/ctest_gate_v4.txt` n'a pas été transféré à `tout` : seul le reçu nouveau clos justifie ce 9/9.

## Validation CSR : cause, indices et reproductions

Contrat API : `src_tout/.../points/dendrogram.hpp:35` expose `PointDendrogram` et promet « contrôles en parallèle, même premier défaut (même ordre des contrôles) qu'en série ». Les vecteurs sont publics.

Dans `points/dendrogram.cpp:39`, la monotonie locale est vérifiée ; à :40, le rang du nœud ; à :41–42, la boucle lit `d.child_val[j]`. **Il manque `child_off[v+1] <= child_val.size()` avant la lecture**. Le contrôle global de l'extrémité finale à :23 n'impose aucune borne aux extrémités intermédiaires. Chaque tranche peut donc lire une plage invalide avant la réduction des premiers défauts. La réduction choisit correctement l'indice minimal seulement si toutes les inspections sont elles-mêmes sûres.

Sonde indépendante `validate_slice_probe.cpp` liée au `libmhgp10_core.a` préexistant, sans reconstruire le moteur ; compilateur C++20/O2, délai de sécurité 5 s, exécutions chacune inférieures à 0,1 s. Elle crée le même peigne de **65536 nœuds/points**, niveaux positifs strictement croissants, enfants avant parents, poids unitaires. La corruption ajoute :

- au nœud 1, enfant égal à soi : refus série `rank_order` ;
- `w=65536−65536/8=57344`, `child_off[w]=0xFFFFFF00`, `child_off[w+1]=0xFFFFFF01`, alors que `child_val.size()=65535`.

Avec P=4, `chunks=min(8P,floor(n/4096))=16` ; **tranche c=14, intervalle [57344,61440)**. Son premier nœud est w : elle tente directement l'accès `child_val[4294967040]`, sans avoir observé le défaut du nœud 1. Ce n'est pas la dernière tranche (c=15).

| Objet | Série | Parallèle P4 |
|---|---|---|
| valide | `none`, rc0 | `none`, rc0 |
| malformé | `rank_order`, rc1 | SIGSEGV, rc139 |

La sonde préexistante `bench/validate_oob_tout` donne la même divergence à 65536 (son main retourne 0 même après refus série). À 20000, elle ne la révèle pas : w=17500 n'est pas un début de tranche ; un défaut antérieur dans la tranche interrompt l'inspection. Les huit sorties/rc, y compris ces résultats favorables insuffisants et les SIGSEGV, sont conservés dans `SHORT_PROBES.json`. `timeout` a rapporté le signal 139, pas une expiration 124 ; les quatre sondes indépendantes désactivent la création de core.

Empreintes de cette preuve :

- source du validateur : `e73ff8708905ec1c96e4602a064162db694acd67aa4398de02a219a79cd1705d` ;
- bibliothèque liée : `28e49dbee828c6a65a76fda20ac3b13a1bc12d3f4f9521e427f5a30e4f31e6ec` ;
- sonde indépendante source : `513940ed2cfc813070bdcaae443e34af5320b063392d5d99aafdeb8eb8185e77` ;
- sonde indépendante binaire : `51aea22355279c6a90e67013b2b561b1c0c5c92c1d7b8779eaf07c67ab18d77f`.

Les quatre hashes sont identiques avant/après les sondes. Aucun binaire/build n'est recopié dans la capture.

**Action développeur prioritaire :** ajouter les bornes locales avant tout déréférencement CSR dans les deux chemins, et la borne des parents avant l'accès depuis les points ; garder ensuite la réduction lexicographique des premiers défauts. Ajouter la fixture « défaut précoce + plage hors bornes au début d'une autre tranche » en Release et ASan/UBSan, série/P4 et frontière de tranche. Corriger seulement le tri des raisons ne corrige pas la sécurité mémoire.

## Exactitude réellement contrôlée, dénominateurs et angles morts

`verif_tete_tout.txt` : **49 configurations tentées**, dont **37 conclues exit0/ECARTS0**, **10 refus exit2**, **2 interruptions exit143** (coquilles K10 core/cover), non 49 passes. Le lecteur compare bit à bit tous les champs de PointDendrogram et de CondensedTree, dont masses/stabilités/lambdas, sélection et labels, contre des copies mécaniques de la référence `a10605a06`. Pool nul et P=1/2/3/5/8/13 ; grilles mcs/z, EOM/leaf et allow_single. Les grandes trames utilisent une grille réduite, documentée dans `run_verif_tete.sh` ; pas tous les paramètres sur toutes les scènes. Les 671 lignes « identique=true » des cas conclus sont des sorties résumées, pas 671 entrées indépendantes.

Le vérificateur construit forêt/catalogue **avec la bibliothèque optimisée**, puis teste les deux têtes sur cette même forêt : sa réussite ne constitue pas à elle seule une vérification indépendante des plateaux de la tour. Les mutants usuels de CSR/rang n'incluent pas les plages énormes qui commencent dans une autre tranche.

Autres reçus clos : `head_diff_tout.txt`, 18 configurations identiques ; `dumps_petits_tout.txt`, 50 cas (10 fixtures × K=1/2/3/5/10), codes 0/0/0 et dumps identiques ; `niveaux_tout.txt`, dix cas, hashes des numérateurs/dénominateurs exacts identiques base/P1/P4 et zéro écart de cache `level_approx[r]`. Exemples LiDAR02 : K5, 1099581 niveaux/hash `5d48e1de66ea7038` ; K10, 4908695/hash `6edfe520fd17e3b4`. Ce sont des empreintes FNV de lecteur, non une preuve exhaustive par leur seule valeur.

TSan `tout` : huit commandes code0, zéro alerte, sorties identiques ; quatre catalogue et quatre cluster, notamment quart LiDAR01 K5/K10. **Aucune trame LiDAR entière dans ce lot TSan.** Le fichier `differentiel_tout_catalogue.txt` réutilise le différentiel historique J2c contre l'ancien découpage (ses comptes changent) : il n'est pas notre preuve unique d'identité du travail entre base/v2/tout.

### Comparateur et plateaux

`generator.cpp:817` trie d'abord par (approximation, support). La partition en **3072 seaux**, exposant + six bits de mantisse, est monotone sur les doubles positifs ; les valeurs hors plage sont rabattues sur les seaux extrêmes. Les grosses concentrations sont traitées par tri parallèle, pas supposées uniformes. Ensuite, le repérage des bandes à :879 parcourt **le tableau global**, pas chaque seau isolément : une bande qui traverse une frontière de seaux n'est pas coupée. À :899, chaque bande est triée par comparaison exacte puis support ; les égalités produisent un rang commun, sans binariser le plateau.

Le filtre strict à :874 emploie la marge relative **2^-40**, après `Level::approx()=to_double(num)/to_double(den)` ; le code documente erreur <7u<2^-50. `geom::compare` multiplie les rationnels exacts. Un juge échantillonné 1/64 n'est pas une vérification exhaustive du filtre : c'est la marge et le repli qui doivent porter sa preuve.

Mutation observée `/tmp/ov_mutant_filtre/.../generator.cpp` : **une seule différence contre src_v2**, `0x1p-40 → 0x1p-60`. Le patch réel est conservé. Le reçu observe code0 mais un niveau supplémentaire sur LiDAR02 K5 **et** K10 ; quatre autres cas observés identiques. Cela montre que supprimer la marge n'est pas une mutation équivalente. Ce reçu appartient à A1/v2, pas au seul nouveau tri par seaux `tout`, et aucune campagne mutante n'a été rejouée ici.

Le point-dendrogramme conserve cependant sa règle antérieure : deux niveaux exacts distincts dont les doubles coïncident/s'inversent partagent un rang publié. Identité avec la référence ne signifie donc pas une nouvelle preuve d'ordre EOM exact sans coalescence. Le cache `Catalogue::level_approx` est public ; `point_dendrogram` vérifie seulement sa taille avant de le faire confiance. Le producteur testé le remplit correctement (dix cas bit à bit) ; une API acceptant des objets externes devra protéger sa cohérence avec les niveaux exacts, non traiter toute valeur de même taille comme certifiée.

### Masses et complexité

La condensation accélérée vérifie poids unitaires, CSR de n−1 arêtes, enfants strictement croissants < parent, inverse des parents et racine finale ; sinon elle revient à la référence. Sur cette précondition d'arbre, la masse u32 d'un sous-arbre est au plus le nombre de points, et les masses de frères sont disjointes. Cela ne qualifie pas la tête publique pour tous poids u32, ni ses stabilités/choix EOM en tous domaines numériques.

Les termes de stabilité des points abandonnés sont encore ajoutés un par un dans l'ordre de référence : économie de traversées et calculs de pow, pas omission de masses. Les chemins des nœuds internes restent série ; l'identité bit à bit limite une réduction flottante parallèle libre.

Pour B boules émises, la partition coûte O(B+P·3072), puis Σ b_i log b_i ; les réparations exactes coûtent Σ m_j log m_j, avec une bande énorme possédée par une tranche. Le pire cas reste O(B log B), et **aucune borne sous-quadratique en nombre de sites** n'est gagnée puisque B et le travail du générateur ne sont pas bornés ici. Les préfixes/copie restent payés, mémoire linéaire en sorties/populations plus histograms. `t_compare=0` signifie « comparaisons déplacées dans t_bands », pas zéro comparaison exacte.

`bandes_lidar02.txt` donne groupes égaux max115/118, ratio proxy max/moyenne4,4/7,6 à P48, et 0/15 paires de niveaux exacts voisins proches à K5/K10. Son « borne série » est calculée avec m log2(m)+m sur **groupes de rang égaux**, non avec les vraies bandes approximatives, ni les durées/comparaisons mesurées ; ne pas en tirer une limite Amdahl générale.

Enfin `cluster` garde les remontées parent par parent pour désactivation/labels des clusters et points (`head.cpp:435+`) : O((clusters+points)·hauteur) au pire, indépendamment du gain de condensation. Une propagation descendante du premier ancêtre sélectionné serait une suite développeur utile, après le correctif mémoire.

## Chronos clos : ce qui est mesuré

`BENCHMARK_SUMMARY.json` recalcule les médianes depuis **87 observations processus neufs**, ordre de variantes tournant : 36 catalogue K5, 15 catalogue K10, 36 cluster K5. Tous les JSON sont parseables/status ok. Le script n'enregistre pas les rc et jette stderr ; le marqueur FIN seul aurait été insuffisant. Ajouter rc, stderr et le refus de JSON manquant au runner avant toute future capture.

Toutes les observations ci-dessous portent sur **lidar02_full.u32le, 45845 sites, sans sol, W4 CPU local partagé**. Le manifeste d'entrée sélectionné est conservé ; son SHA source et le hash déclaré de cette entrée `a4bbc86d...08af` sont référencés, pas un nuage massif recopié. C'est une entrée préparée existante, non une nouvelle segmentation chronométrée.

| Mesure murale, médiane s | base | v2 | tout | observations/variante |
|---|---:|---:|---:|---:|
| ordre + assemblage K5, somme par observation | 0,5102 | 0,3337 | 0,2086 | 12 |
| catalogue K5 interne | 3,0095 | 3,0785 | 2,7243 | 12 |
| processus catalogue K5 | 3,045 | 3,125 | 2,755 | 12 |
| ordre + assemblage K10, somme par observation | 3,4457 | 2,3063 | 1,3018 | 5 |
| catalogue K10 interne | 25,943 | 26,2378 | 22,6053 | 5 |
| processus catalogue K10 | 26,23 | 26,44 | 22,73 | 5 |
| processus cluster K5, cover/mcs200/z3/EOM | 3,920 | 3,990 | 3,585 | 12 |
| tête de ce processus cluster K5 | 0,0625 | 0,0370 | 0,0435 | 12 |

Les médianes de sommes ont été recalculées par observation, non obtenues en additionnant les médianes des sous-étages. La tête inclut point_dendrogram, validate, condensation/sélection et écriture des labels ; l'application impose `tp.only_order=kk` (`cli/mhgp10_cluster.cpp:160`). **K5 seul, pas la tour FULL1..5.**

Les quatorze comptes géométriques principaux sont identiques entre variantes sur les observations catalogue (dont filtre403800945/1287951189 à K5/K10). Les temps boxes varient malgré ce même travail : charge médiane ≈11 à K5, ≈29–32 à K10 ; les gains totaux ne peuvent pas être attribués exclusivement à l'ordre. RSS médian augmente légèrement : K5 base400750/tout407770 KiB ; K10 base1705296/tout1734456 KiB. Les défauts de pages baissent, mais ils ne constituent pas du travail géométrique supprimé.

`temps_tete_k5.txt` : micro-banc sur catalogue/forêt déjà préparés, 15 répétitions dans le **même** processus pour chaque P2/P4/P8, murs tête référence→pool157,69→79,87 /200,96→85,41 /180,88→79,42 ms. Ce facteur1,97–2,35 local ne s'applique pas au pipeline entier. P4 condense plus vite mais point_dendrogram n'est pas toujours plus rapide ; le processus cluster montre même tête `tout` plus lente que v2 série. Pas d'affinité CPU ni de qualification nombre de cœurs prouvée ici.

Le fichier historique `estimation_g4.txt` transforme des ratios locaux en estimations G4, dont tête K10 elle-même extrapolée : **modèle, non exécution G4**. Aucun CPU·s n'est fourni par le runner (`time %R %M %e`) ou les steady_clock internes.

## Combinaison et contrat restant ouvert

Le binaire `tout` combine effectivement A1 + tri v3c + tête v4 dans cette capture ; hashes complets catalogue `d9a99571...e90ad`, cluster `42ed0c60...d6339`. Les patches réellement observés contre base/v2 sont archivés, distincts des patches historiques. Aucun reçu ici mesure l'ensemble **J3 feuille + frontière v3b + Kruskal p1c/p2c + ordre/tête** : les comptes géométriques restent ceux du J2c de référence et Kruskal utilise encore son tri `std::sort(members)`. Le fait que les fichiers du Pool coïncident avec ceux de v3b n'est pas une mesure du parcours frontier combiné.

Profil prioritaire retenu pour la v10 : grille 1 mm, sans sol **trame entière**, FULL K1..5 explicitement dans les 100 ms sur G4, à qualifier sur plusieurs séquences. Float32 reste un objectif secondaire ; le brut entier reste aussi à tester. Trois trames de la même séquence ne sont pas trois séquences. Les moitiés/quarts sont diagnostics seulement. Cette tranche n'acquiert ni nouveau G4, ni FULL, ni 100 ms, ni borne globale, ni validation sur brut/f32.

## Clôture des preuves

Capture légère : `receipts/audit_continu_20260929/order_head_corrected/`. Sources/headers courants concernés, copies de tests/runners, patches, CSV/JSON/logs nécessaires ; **aucun build, binaire, nuage** copié. Ouverture : 68 entrées +16 compléments. Fermetures avant/finale : **82/84 inchangés**, seuls `logs/builds.done` et le journal CTest ont progressé extérieurement ; aucune source, test ou binaire examiné n'a changé. Les trois manifests SHA conservent cette évolution, sans prétendre à un gel atomique de toute la campagne. Une demande initiale de `builds.done` au mauvais chemin a été corrigée et explicitement tracée, non présentée comme disparition externe.

Les logs/statuts clos et les observations partielles datées sont conservés séparément. La capture n'emploie pas l'ancien «950 passes» du fuzzer J3 : le contre-audit antérieur reste **940 conclusifs +10 timeout** pour ses trois graines initiales, dans sa propre note.

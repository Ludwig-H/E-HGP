# Vrai census → batch semé → tour, et diagnostic K9/K10 séparé

Qualification privée v7 hors registre, `backend=HOST_STUB`, `profile=quantized_u16_input_only`, `public_status=not_claimed`. Ce paquet ferme O2 et SAN sur le raccord semé exact **hôte** ; il ne qualifie aucune compilation CUDA ni exécution GPU. Toute future correction d'annotations host/device appartiendra à une variante distincte et devra être requalifiée. GCP non utilisé.

## Raccord et autorités

Les sources du moteur, du terminal, de la route et de l'adapter viennent byte-identiques du batch r5 publié : manifeste `2c7df2e04132e306cd0a063efa93007548a0bc01736f818b5f447b8af1e0a4dd`, header `83f1c78e…`, route `a62bd1d5…`, adapter `993786f3…`. `import.json` lie le reçu r5 et sa fermeture complète. Ce lien atteste la provenance ; il n'hérite pas des résultats du paquet batch.

Le juge census→tour et son oracle viennent du paquet permanent `f26e11a64ac44c6cc8f617b7e3b670b86fe75427a3aad5370705e8a98114f953`. L'oracle est inchangé ; seule l'entrée `main` du juge incorporé devient `mhgp7_private_census_main`. Une macro de **site d'appel dans le test**, appliquée après inclusion du header Builder, redirige ses constructions vers le callback. Aucun hook géométrique n'est installé dans le moteur. Les sources de la preuve sont visibles sous `sources/current/`, et les deux manifestes d'autorité sont conservés dans `qualification/`.

## Tours et demandes chronologiques

Ligne12, coquille12+centre+extérieur14 et spatial12, deux variantes d'identités et s WSPD=8/10/12 fournissent **18 vrais census**. Le juge confronte ces inventaires à Gram, puis alimente le producteur avec les vrais census, jamais les lignes de l'oracle. Les mêmes coupes ouvertes/fermées, identités, parents, verticales et métadonnées d'ordre sont vérifiés pour les résultats retenus.

Chaque capture finale conserve **54 tours K1..10**, dont **36 construites avec le callback batch** et 18 avec la voie CPU0. Pour chaque tour batch, une autre tour scalaire complète est réellement calculée afin de confronter tout le travail : ce sont **36 constructions supplémentaires payées**, donc **90 constructions de tours au total**, pas 54 opérations seulement. Les 48 paires physiques et 540 ordres du JSON T2 portent sur les tours retenues. Les contrôles T2 restent 12 315 725, avec 13 000 coupes et 8 103 948 comparaisons verticales par capture.

| Géométrie | Lots chronologiques | BallId directement confrontés | Q post-échange | H post-échange |
|---|---:|---:|---:|---:|
| ligne12 | 0 | 0 | 0 | 0 |
| coquille14 | 84 | 444 | 12 | 0 |
| spatial12 | 108 | 1 428 | 216 | 120 |

Les **1 872 BallId et ordinaux** sont comparés directement au terminal CPU c03. Le juge exige que les MEB de cette observation c03 égalent les MEB du batch plus H ; puis les tours scalaires semées et batch sont comparées sur MEB, supports, puissances, intrus, descentes, R/U/S/Q/H/T. La ligne peut avoir zéro appel de callback : toutes ses demandes utiles sont déjà semées en amont. Les 12 tours batch de ce cas ne sont donc pas présentées comme une non-vacuité de kernel.

## Diagnostic K9/K10 indépendant des demandes FULL

Le premier index/census de chaque géométrie fournit **tous** ses sous-ensembles K9/K10, déclarés par cardinal avant toute résolution. Le lot entier est exigé nominal, sans filtrage d'échecs. La coupe initiale fixe est `3*65535²+1`, et les ordinaux sont u64 au-delà de 2^32. Ce diagnostic n'est pas la collecte des seules demandes chronologiques du programme FULL et ses coûts ne sont pas ajoutés aux compteurs de celles-ci.

| Ordre | Facettes diagnostiques | Q | H | Lignes q3 de référence | Lignes q4 de référence |
|---|---:|---:|---:|---:|---:|
| K9 | 2 442 | 3 448 | 36 | 1 173 | 226 |
| K10 | 1 133 | 1 761 | 5 | 654 | 19 |

Cela fait **3 575 facettes distinctes** sur les trois géométries, chacune comparée en BallId/ordinal au CPU. Tous les compteurs de descente restent identiques. Lors d'un hit, les appels, matérialisations, lookups, anchor hits, puissances et supports du batch sont comparés à la référence **moins la dernière MEB effectivement observée**, pas à une économie supposée. Les colonnes q3/q4 décrivent les traces de référence, y compris cette MEB absente du produit sur un hit ; ce ne sont pas ses compteurs de travail payé.

## Captures et limites

`o2_r1` est un échec de compilation conservé : `const auto scalar` empêchait le déplacement d'un résultat de forêt non copiable. Code de compilation 1, aucune géométrie exécutée. `o2_r2` ferme les trois vrais census/tours mais ne possède pas encore le diagnostic haut-K. `o2_r3` et `san_root_r1` ferment le complément complet sur **la même fermeture de sources**, juge `6f85cb7a…`, avec des observations JSON identiques. Aucun résultat haut-K n'est attribué rétroactivement à r2.

Les quatre reçus conservent **23 commandes** au total : version/compilation pour r1, puis version/compilation/trois géométries/argument inconnu/argument absent pour chacun des trois succès. Les arguments invalides sortent 2. ASan/UBSan et LSan actif sont imposés par le recorder épinglé. Les trois ELF présents ont été reconfrontés à leur pin avant scellement et ne sont pas distribués. Sources, snapshots, streams et codes exacts restent conservés, dédupliqués, ainsi que les deux patches de juge. Aucun vendor n'est embarqué.

Cette capture n'exécute pas les quatre mutants et neuf rejets du juge permanent, ni les 41 rejets de transaction du paquet batch : ils restent des témoins séparés liés par les sources, pas de nouveaux résultats de ce run. Le nouveau diagnostic haut-K lui-même ne constitue pas une campagne de mutants CUDA. Les coûts des observations CPU, des tours supplémentaires et du diagnostic séparé sont déclarés ; aucun de leurs timings ne mesure un contrat produit.

## Lecteur

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/mhgp7_batch_t2_neuf
```

Le lecteur contrôle les fichiers et sources réels, les deux autorités publiées, la relocalisation unique de `main`, toutes les observations par K et par géométrie, les statuts historiques et l'identité O2/SAN. Les conditions explicites restent actives sous `-O`. Il ne compile et ne recalcule rien. Les commandes exactes de reconstruction sont dans les reçus, Boost reste externe ; le recorder source-backed ne propose aucun mode CUDA.

Les bornes n≤14 sont celles du juge. Aucune complexité sous-quadratique universelle, exactitude pondérée/non-u16, résidence multi-millions ou latence de toute la tour 50k sous une seconde puis 100 ms n'est qualifiée. Les branches CUDA futures, même si elles ressemblent à ce stub, restent à compiler strictement et à exécuter séparément sur carte.

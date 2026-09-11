# Raccord privé du journal incrémental au producteur FULL

11 septembre 2026. Point de départ déclaré `ce842a3f1c0d55786250b1deba85e7bd92c8807a`.
Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Aucun changement actif, auditeur, Git ou GCP.
Les sous-captures ci-dessous sont closes avec leurs pins avant/après propres.
Le raccord reste privé : qualification positive, intérêt de performance non
établi pour cette première variante owning. `verify.py` normal/`-O` rejoue
uniquement les lectures ; `MANIFEST.json` scelle les captures non ELF.

## Sources et delta

`baseline/` copie 63 fichiers dont le header FULL `33e7d05e…` ; le snapshot
`candidate/` reprend intégralement le journal `b526b895…` et son propriétaire
`76885ecd…`, déjà qualifiés dans le paquet public du prototype. Le header FULL
raccordé est `a2c6ab390540fd57d2e593d6d78097f0cee477810e1e3b0dcd35fe4c3afbbd50`.
Ces trois headers candidats sont figés pendant les campagnes ROOT/SAN.

Le propriétaire est créé après validation du catalogue. K1 reste un unique
lot zéro dans l'ordre des PointId ; les populations sont créées paresseusement
dans l'ordre antérieur, directement chez le propriétaire append-only. La
copie vers la banque immuable au scellement reste celle du prototype qualifié.
Chaque lot publiable est appendu atomiquement après préparation des actions et
`new_node`, avant installation des ancres et semis. Les lots entièrement
inertes ne sont pas envoyés au journal, mais leurs ancres restent installées.

`Draft` ne garde plus que les verticales. `end_order` libère `live` et contrôle
les IDs créés ; `seal` publie toute la collection sur sa seule banque après
validation complète. Le producteur contrôle encore K consécutifs, partage de
banque, tailles des verticales et cohérence des histoires. Une erreur laisse
la sortie publique vide, même après fermeture d'ordres précédents.

Cette première variante conserve **un FullCoverageBatch owning temporaire par
lot**, détruit après append : elle retire leur rétention globale, pas toutes
les allocations imbriquées par action. Les scratchs plats, l'adoption de banque
par move et toute modification géométrique sont hors de ce delta. L'accounting
FULL devient `private_incremental_whole_lot_amortized_arenas_v1` ; les réserves
exactes de la façade historique ne décrivent pas les arènes incrémentales.

## Qualification close à cette étape

`o2_r1` : 12 commandes, deux compilations strictes et huit passages appariés
cache, sans cache, statique 1/4 ; mauvais arguments = 2. La gate Gram/Gamma
originale reste inchangée. Un hook privé identique dans les deux snapshots
autorise seulement la désactivation du cache, avant sa configuration.

Le wrapper exporte physiquement tous les champs de populations, nœuds,
parents, successeurs, contributions datées, verticales et compteurs du
resolver, y compris capacités statiques. Les sorties `.physical`, stdout et
stderr sont octet pour octet égales dans chaque mode apparié ; ce contrôle
ne se réduit pas à un digest ou à une égalité de couvertures projetées.
Il vérifie aussi le partage réel de la banque entre tous les ordres.

| Modes | Corpus | Ordres | Coupes | Vérifications de verticale |
| --- | ---: | ---: | ---: | ---: |
| cache / sans cache | 28 nuages | 112 | 2 508 | 45 948 |
| statique 1 / 4 | 30 nuages | 124 | 3 324 | 75 136 |

Les groupes growth/inert, extra-shells, K=n, permutations de points et
catalogues, ainsi que huit rejets historiques restent exercés. Le wrapper
ajoute un singleton K=n=1 demandé avec kmax10 et le rejet threads négatifs.
Par exécution : 29 ou 61 tours closes enregistrées physiquement et neuf refus,
dont les baselines cache supplémentaires que la gate statique compare déjà.

`failure_o2_r1`, puis `failure_o2_v2` et `failure_san_root_v2` : trois commandes
closes chacune, sans Boost. Le census rationnel E5
du paquet de mutants statiques est copié/pinné et remappé par CloudIndex.
Une injection test-only arme un vrai `operator new` soit après un préfixe K2
non vide (K1 déjà fermé), soit juste avant le scellement global. Toutes les
allocations observées dans ces suffixes sont refusées individuellement :
**1 402 refus**, dont 1 300 après préfixe et 102 au seal, sur les modes 0/1/4.
Les ordres publics restent vides et le travail du préfixe est non nul. Trois
corruptions tardives du niveau d'un lot sont également refusées ; 708 contrôles
vérifient l'empoisonnement du propriétaire et le refus de reprise. Un nouveau
Builder sain réussit ensuite. Les allocations concurrentes sont indexées par
un compteur atomique ; cette campagne ne prétend pas nommer toutes les piles
d'allocation ni démontrer l'absence de races par elle-même.

`san_r1` conserve sa compilation baseline réussie puis son refus LSan/ptrace,
sans désactiver `detect_leaks=1`. Le même ELF a ensuite passé depuis le contexte
ROOT dans un reçu distinct. Le rejeu complet `san_root_r2` est clos PASS,
12 commandes : les exports physiques, stdout et stderr des quatre modes sont
également identiques entre O2 et SAN. Au total par build et par bras, les quatre
modes enregistrent 216 appels, 180 tours closes et 36 refus, avec 4 932 nœuds,
4 208 parents et 3 432 contributions comparés physiquement ; les occurrences
répétées ne sont pas présentées comme des nuages indépendants supplémentaires.

Le premier SAN de panne, `failure_san_root_r2`, révèle une erreur du harnais :
`std::stable_sort` alloue par `operator new(nothrow)` non intercepté puis le
delete du harnais appelle `free`. Son échec alloc-dealloc-mismatch reste intact.
Le wrapper **additif** `failure_gate_v2.cpp` intercepte les formes nothrow new
et delete correspondantes, sans modifier le driver original, les hooks ni le
candidat. `failure_o2_v2` et `failure_san_root_v2` passent avec les mêmes
compteurs 1 402/1 300/102/3/708. Aucune option sanitizer n'est abaissée. Aucun
statut SAN positif n'est attribué aux deux captures initiales échouées.

La gate indépendante des préfixes de l'auditeur, publiée à cbdd3ff8, a été lue
et ses sources/pins sont copiés sous `prefix_reference/` ; ses 13 905 contrôles
structurels historiques ne sont pas réattribués au raccord géométrique.

## Micro fermé : résultat de performance mitigé

`micro_o2_r1` contient quatre commandes closes. `record_micro.py` réutilise le
probe d'allocation historique sur les
deux nouveaux snapshots, n200/400/800 uniformes seed3, s8, K1..10 mono. Il
mesure appels new, octets demandés cumulés, pic de ces octets vivants, arènes
finales et temps FULL, puis compare le payload physique complet. Le RSS de
processus est capturé séparément et inclut la préparation du census. Les
trois payloads physiques sont identiques ; aucun résultat ancien n'est hérité.

| n | Appels new baseline → candidat | Pic demandé baseline → candidat (octets) | Retenu final baseline → candidat (octets) |
| --- | ---: | ---: | ---: |
| 200 | 871 992 → 937 378 | 20 681 380 → 20 649 544 | 10 327 116 → 13 560 236 |
| 400 | 2 190 325 → 2 352 340 | 43 491 410 → 44 942 690 | 25 476 444 → 32 494 364 |
| 800 | 4 983 612 → 5 349 633 | 105 973 156 → 105 017 572 | 57 757 460 → 75 097 940 |

À n800, les new montent de 7,34 % et le retenu de 30,02 %, tandis que le pic
demandé baisse de 0,90 %. À n400 ce pic augmente de 3,34 %. Le RSS processus
de la **série complète**, préparation incluse, passe de 191 860 à 163 800 KiB :
cette observation favorable n'est pas un pic FULL isolé par taille. Le temps
FULL brut à n800 passe de 4,964599460 à 4,820427018 s, mais les autres tailles
augmentent, et ce n'est pas une campagne chronométrique répétée/appairée en
charge exclusive. Aucun speedup n'est revendiqué.

Le propriétaire qualifié reçoit les populations par `const&` : les buffers
de chaque ligne temporaire sont donc recopiés avant leur destruction. Les
arènes finales croissent désormais géométriquement ; à n800 leur capacité
plus verticales passe de 43 083 712 à 60 424 192 octets, expliquant exactement
les 17 340 480 octets de rétention finale supplémentaires. La copie de
populations et la conservation d'actions owning par lot sont des explications
structurelles de l'absence de baisse d'allocations ; aucun profil exhaustif
d'attribution des new par site n'est prétendu ici.

Voir `LIMITES_ET_FAUSSE_PISTE.md` : **ne pas intégrer ce raccord tel quel comme
optimisation validée**. Les pistes scratch plat/référence singleton et transfert
de ligne sans copie sont des deltas distincts à demander et qualifier ; aucune
n'a été appliquée dans cette passe.

Les arènes finales résident plus tôt ; un gain de pic ne découle pas de la
seule disparition des brouillons. Une différence d'allocation n'est pas un
speedup. Les hôtes sont partagés. Le coût amorti du journal O(N+P+C) porte sur
la taille de sortie, pas sur une borne sous-quadratique universelle en n.
Aucun calcul n8k+, ni contrat 50k/1 s, ni traitement de dizaines de millions,
ni GPU n'est exécuté ou acquis par ce dossier. GCP non utilisé.

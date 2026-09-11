# Contre-lecture de la livraison `ad7ffd28` sur les octets commités

11 septembre 2026, second auditeur (session e-hgp-c6).
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Aucune écriture hors de ce dossier ; aucun moteur ni source
active modifié. Autorité **relative** aux census exacts complets fournis :
ni contrat 50k/1 s, ni GPU, ni promotion de statut public.

La livraison `ad7ffd28` (« cache exact facet resolutions and measure full towers
on g4 »), suivie du commit auditeur `175acdd5`, publie à l'octet le cache de
résolutions, le journal à réservations exactes et les corrections de tests que
j'avais lus le 10 septembre comme WIP non commité. Ce reçu **rejoue tout ce qui
avait alors été établi, sur les octets réellement commités**, exécute le contrôle
nommé des quatre blocs 50k du constructeur sur CPU local, et porte une
contre-lecture adversariale des onze surfaces de la livraison.

**Provenance des octets.** Le worktree partagé transitionnait vers `ce842a3f`
(« resolve facet geometry in deduplicated cpu batches » ; `full_ball_tower.hpp`
`910f45ba…` → `33e7d05e…`, résolveur statique intégré) pendant ce travail. Pour
garantir la provenance, les rejeux de portes, empreintes, mutants, corpus et
front sont compilés depuis un export **pinné** `git archive ad7ffd28`, isolé du
worktree, et l'observateur 50k depuis l'overlay figé dérivé de `910f45ba…`.
Cette contre-lecture qualifie donc **`ad7ffd28`** ; le moteur `ce842a3f` au HEAD,
qui change `full_ball_tower.hpp`, **n'est pas revu ici** et appelle une
qualification distincte.

## 1. Identité des octets commités et des octets lus comme WIP

Les trois empreintes annoncées par le constructeur sont exactement celles du
commit ; la fermeture des 42 sources de l'observateur nommé coïncide à l'octet.

| Source active | sha256 commité | = WIP lu le 10 sept. |
| --- | --- | --- |
| `src/forest/full_ball_tower.hpp` | `910f45ba…` | oui |
| `src/forest/full_coverage_certificate.hpp` | `7608e70e…` | oui |
| `tests/facet_resolver_cache_gate.cpp` | `898533ca…` | oui |

`src/pipeline/witness_front.hpp` a en revanche **changé** depuis mon snapshot du
10 septembre (`07d990f2…` → `fb6f1bcc…`), de même que `tests/witness_front_gate.cpp`
(`c1bfc683…` → `f283bca0…`) : le différentiel front est rejoué sur les nouveaux octets (§ 5).

## 2. Quatre portes O2 et ASan/UBSan sur les octets commités

`facet_resolver_cache_gate`, `full_ball_tower_gate`, `full_ball_work_gate` et
`witness_front_gate` compilent et passent `--selftest` en O2 **et** sous
ASan/UBSan (`detect_leaks=1`, arrêt sur erreur) ; argument inconnu → code 2 ;
la sortie `--selftest` est **identique** entre O2 et le build sanitizer pour les
quatre. L'injecteur `new(nothrow)` du test cache, dont le premier refus ASan
était mon constat du 10 septembre, est corrigé dans les octets commités.
Enregistrements : `portes_commit/*.json`, identité dans
`portes_commit/identite_o2_san.txt`.

## 3. Premier verrou P1 (vacuité des lots groupés) clos sur le header commité

Sur une copie du header commité (jamais les sources actives), les deux mutants
causaux que j'avais trouvés **survivants** sur le chemin groupé du header publié
le 10 septembre sont maintenant **tués** par la fixture `growth_ABCZ_doubled_lot`
que le constructeur a gravée :

| Mutant (`mutants_groupes/*.diff`) | code attendu | code obtenu |
| --- | ---: | ---: |
| nominal | 0 | 0 |
| `growth_grouped` (croissance groupée omise) | 1 | 1 |
| `inert_grouped` (ancre inerte groupée omise) | 1 | 1 |

Mon premier verrou P1 (reçu `receipts_raccord_ancres_20260910/README.md` § 5.1)
est donc clos sur les octets commités.

## 4. Empreintes de payload par famille, identiques au reçu du 10 septembre

La sonde par famille (octets commités) reproduit exactement le reçu du
10 septembre sur les cinq familles : `payload_digest`, `nodes`, `balls`,
`contributions`, `vertical_refs` et coquilles supplémentaires identiques.

| Famille (n) | coquilles suppl. | payload identique |
| --- | ---: | --- |
| uniform (400, 2000) | 0, 0 | oui |
| scanline_overlap_multiecho (2000) | 3 726 | oui |
| scanline_single_pass (2000) | 76 | oui |
| terrain (2000) | 0 | oui |

Détail : `empreintes_commit/comparaison_avec_recu_20260910.txt`.

## 5. Front de témoins WSPD par lots : inchangé malgré le nouveau header

Le header du front ayant changé depuis mon snapshot, le différentiel front
scalaire `alive_rectangles_fused` contre le backend par lots `CpuWitnessBatch`
est rejoué sur les octets commités. Les **neuf** configurations (uniform
2000/8000 à s=8/10/12, lots 4096 à 100000, 1–4 fils ; scanline, eight_clusters,
terrain) rendent une ligne **bit-identique** au reçu du 10 septembre : rangs,
grand-livre des masses, travail physique et nœuds visités égaux. Le front a
changé mais reste équivalent au scalaire, qui demeure le défaut du générateur.
Détail : `front_commit/comparaison_avec_recu_20260910.txt`.

## 6. Corpus aléatoire, juge Gamma rationnel indépendant

Le pont `tower_bridge` recompilé sur les octets commités, jugé par le modèle
Gamma rationnel (aucun en-tête produit inclus) : campagnes principale (207),
cocirculaire (300) et étendue (2000), **0 divergence, 0 refus** ; sortie
`python3 -B` = `python3 -B -O` ; le mutant `drop_extra_ball` est tué (128
divergences, 148 refus). Le bloc `totals` (objet jugé : coupes, racines, images
verticales, divergences) est **identique** au reçu du 10 septembre ; seuls les
compteurs de travail du cache (`anchor_hits`, `intruder_queries`,
`descending_steps`) diffèrent, ce reçu ayant été produit contre le header
pré-cache `4929a542…`. Le cache ne change donc pas l'objet.
Détail : `corpus_commit/comparaison_avec_recu_20260910.txt`.

## 7. Quatre blocs nommés 50k (second verrou P1) — exécution CPU locale

Le contrôle nommé des quatre blocs a été exécuté sur le census réel 50k (CPU,
`--threads=8`). Les quatre racines pré-lot coïncident avec
[GLOBAL_PARENTS](../receipts_plateaux_full_20260906/GLOBAL_PARENTS.md) :

| Bloc (K, capture) | racines pré-lot | attendu | masque | intérieur | obs. avant/après |
| --- | ---: | ---: | ---: | --- | --- |
| 174406 / K5 | 1 | 1 | 0 | non | 1/1 |
| 254569 / K2 | 2 | 2 | 0 | non | 1/1 |
| 996863 / K6 | 2 | 2 | 0 | non | 1/1 |
| 1251653 / K10 | 1 | 1 | 0 | non, ancre conservée, contribution vide | 1/1 |

Digest d'entrée conforme (`3f7c6dd4…`), mutant d'ancre brute tué
(`named.pre_root_not_live`), `benchmark=false`, `contract_qualified=false`,
`public_status=not_claimed` ; complétude du catalogue et arité finale **non**
vérifiées (hors périmètre). Enregistrement
`blocs_50k/pinned_50000_threads8_o2_r2.json` : rc 0, 989 s, pic 13,78 Gio.
**Mon second verrou P1 (reçu `receipts_raccord_ancres_20260910/README.md` § 5.2)
est clos sur CPU local.** Le contrôle vérifie trois parents globaux pré-lot et
une ancre inerte K10, ni S1 ni l'arité finale.

Binaire observateur reconstruit à l'octet identique à la capture du constructeur
(`ae992d15…`, cf. `capture/binary_o2_r2.sha256`). Première tentative
(`pinned_50000_threads8_o2.json`) **terminée par SIGTERM à 35 min** sous pression
mémoire concurrente (CTest 32k + sondes), stdout vide : conservée comme
enregistrement honnête, non conforme. Reprise **seule** via `guard_rerun_50k.sh` : réussie (enregistrement `_r2`).

## 8. Contre-lecture adversariale des onze surfaces (aucun P1)

Onze lectures indépendantes (math, implémentation, reçus, documentation, sûreté
GCP, vacuité de test) puis trois réfutateurs à lentilles distinctes par constat
(≥ 2/3 requis pour maintenir). **Aucun constat P1 n'a survécu** : pas
d'affirmation fausse, pas de faille mathématique ou d'implémentation. Dix-neuf
constats P2 survivent (imprécision, vacuité de test, obligation manquante),
trois sont réfutés, vingt-deux P3 non jugés. Constats, votes et notes complets :
`contre_lecture.json`.

### Point ouvert le plus haut : la tour par boules n'est jugée par oracle qu'à n ≤ 8, K ≤ 8

`tests/full_ball_tower_gate.cpp` alimente la tour par le **catalogue dérivé de
l'oracle** (`catalogue(points, ix, oracle::Model, kmax)`, l. 155, 325, 426, 512),
jamais par `generate → prefilter → census`. `oracle/local_plateau_oracle.hpp:28`
refuse `points.size() > 8`. Les 170 320 contrôles des 28 nuages sont donc tous à
n ≤ 8, K ≤ 8, coquille ≤ 8 (bornes `kBallShellMax=12` et banque ≤ 16 jamais
approchées). Le raccord `census → tour` n'est exercé à l'échelle que par
`bench/full_ball_tower_probe.cpp` (qui n'émet que des digests, sans oracle) ; le
seul oracle K9/K10 (`receipts_full_meb_20260906`, n=14) juge `full_gabriel.hpp`,
un **autre** moteur. Tant que la tour par boules **alimentée par le census** n'est
pas jugée par un oracle rationnel T2 (n=12–14, plateaux cosphériques ≥ 5), « exact
relativement au census fourni » ne dit rien du census réellement fourni à 8k–50k.

### Constats P2 survivants, par surface

| Surface | Constat | Verrou proposé |
| --- | --- | --- |
| plan_journal | Choix recommandé abandonne la réservation exacte sans obligation de compaction : surcapacité résiduelle bornée ~F par ordre (jusqu'à ~2 Go à 32k) contre 0,29 Go d'écart logique net ; gain de résidence non établi | Compaction à taille exacte par ordre avant scellement (ou borne prouvée), et critère de refus sur capacité/taille + pic apparié à la porte 5 |
| plan_journal | Porte de régression d'échelle manquante : digests de payload 8k/16k/32k non exigés de l'assembleur incrémental | Porte `scale8000/16000/32000` exigeant `payload_digest` et compteurs égaux façade/assembleur |
| static_note | Le mutant « admettre toute clé sans fenêtre K » ne teste pas une divergence d'objet (la fenêtre basse est une économie) | Reformuler l'invariant : prédicat d'admission statique ≡ prédicat d'installation d'ancre temporel `programs[K]` |
| static_note | Chemin de hit du cache : l'assertion `ℓ_t < r` n'est pas exécutée et le lemme de stricte antériorité des semis n'est écrit nulle part | Graver les deux lemmes ; stocker niveau/BallId du terminal avec le jeton et asserter `ℓ_t < r` au hit |
| static_note | Descente d'intrus et indépendance de politique (B) exercées par une seule fixture | Deux fixtures à chaînes longues (≥ 3 échanges, ≥ 2 à rayon égal) + une où les deux politiques donnent des terminaux différents |
| rightmost | Branche « feuille mixte » avec test de puissance inatteignable, présentée comme cas vivant | Documenter comme inatteignable par exactitude des bornes + porte `intruder_power_tests == 0`, ou fixture l'atteignant |
| rightmost | La porte FULL à 28 nuages n'exerce `intruder()` que 12 fois, citée sans ce chiffre | Écrire le chiffre ; porter l'invariance par (B) + un digest différentiel de famille |
| docs_claims | « Six mutants du delta cache » : deux portent sur le chemin des lots groupés préexistant | Reformuler : quatre du delta cache + deux du chemin groupé préexistant |
| nvcc | Refus des phases inconnues : une commande conflate deux clauses ; « deux sources générées » et « -o multiple » jamais exercés isolément | Quatre commandes attendues à 2, chacune n'activant qu'une clause |
| nvcc | Le stade de production de chaque diagnostic est inféré, pas capturé | Ajouter `nvcc --dryrun`/`--keep` des deux fixtures de refus |
| nvcc | Obligation (i) : aucun mutant d'une source `.cu` du projet, seulement deux fixtures synthétiques | Trois mutants depuis la copie épinglée de `census_route_device_gate.cu` via nvcc + adaptateur |
| witness_front | `WitnessFrontWork` non remis à zéro sur échec par exception ; contrat « par appel » prouvé seulement sur le chemin singleton | `*work = {}` à l'entrée, ou graver que `work` n'est lisible qu'après retour normal |
| wspd_device | Le document attribue les trois seuils à chaque requête ; ils sont par appel `run()` | Corriger la phrase ; seuils par appel de lot |
| gcp_tools | Les deux nouveaux selftests GCP ne sont câblés ni en CI ni au README | Ajouter les quatre commandes `selftest_*_v7.py` (normal et `-O`) à l'étape « session safety without cloud » |
| static_prototype | Contrôles « terminal strictement antérieur » jamais exercés : un mutant relâchant les trois `<` en `<=` survit | Scinder la raison, graver un mutant `<=` causal |
| static_prototype | « Clé présente mais inadmissible à K4 » : aucune observation nommée ; le mutant meurt sur E5 avant la fixture window | Compteur `inadmissible_key_continuations > 0` exigé sur une fixture nommée |
| static_prototype | Dédoublonnage : mutant « comparateur sur préfixe / hash seul » absent | Graver le mutant « clé sur préfixe K−1 » jugé par l'identité de payload appariée |
| static_prototype | Indépendance (B) non témoignée par la gate appariée : les deux routes utilisent la même règle « premier intrus » | Voie statique avec une seconde règle d'intrus → même payload apparié avant toute promotion |
| static_prototype | Résidence de la voie statique : O(R_K) par ordre, pas O(U_K) ; « jamais un cache 16n par fil » n'est pas un gain mémoire | Publier R_K/U_K/S_K et octets retenus par ordre à 8k/16k/32k face au nominal |

Trois constats **réfutés** (≥ 2/3 réfutateurs) : l'hypothèse « un seul site de
hit BallKey » du mutant d'ancre brute (le mutant reste causal sur head, cf. § 7) ;
deux constats static_prototype contredits par le worktree non commité.

### Autres points ouverts de la critique de complétude

Verrous § 5.3–5.8 du reçu `receipts_raccord_ancres_20260910` toujours non
réfutés (couplage Kmax census/tour ; identités `N_A` et
`resolver_meb_calls = anchor_hits + intruder_queries` non assertées dans le code,
vérifiées à la main 4 780 219 + 1 447 046 = 6 227 265 sur `uniform` 8000 ;
fixtures de seuils cercle/octaèdre/coquille asymétrique non gravées ; portes
`threads=1` vs 8 et permutation physique sur le payload FULL). Profil u16 :
`full_ball_tower.hpp:379` refuse `has_duplicate_positions()` alors que le profil
admet les positions dupliquées bucketisées — statut rendu et documentation non
lus. Toutes les sondes FULL ≥ 8k sont `--family=uniform` : les conclusions
coût/résidence reposent sur une seule famille.

## 9. Lecture reproductible

```bash
python3 -B    morsehgp3D_v7/audits/receipts_cache_commit_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_cache_commit_20260911/verify.py
```

Le lecteur vérifie les codes de sortie enregistrés, l'intégrité sha256 des
sorties, les fichiers de comparaison (aucune différence), les quatre blocs
nommés contre les attentes épinglées, le vert CTest et l'épinglage de tous les
artefacts. Il ne compile ni n'exécute aucun moteur et ne contacte pas GCP. Les
scripts de rejeu (`rejouer_commit.sh`, `rejouer_front.sh`, `guard_rerun_50k.sh`)
et le générateur de revue (`make_review.py`) conservent les commandes exactes.
`review.json` épingle chaque artefact ; `contre_lecture.json` porte les 19
constats survivants, leurs votes et la critique de complétude.

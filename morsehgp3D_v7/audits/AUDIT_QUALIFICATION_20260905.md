# Qualification indépendante de la v7 — 5 septembre 2026

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
**F possède sa propre qualification contre-vérifiée : 339/339 portes
Release complètes, 48/48 ciblées Release et 48/48 ASan/UBSan.** Les trois
paires E/F à 8 000 points conservent les mêmes digests de forêts et
cardinalités à `s=8,10,12`. Les observations F suivantes sont également
closes : succès à 16 000 points, puis refus budgétaire K9 à 32 000 points,
code 2, `silent_core_record_budget`, sans tour publiée. La
[contrelecture des mesures et paliers](receipts_resolver_20260905/qualification/review.json)
établit ces distinctions depuis les bruts ; elle ne déduit ni gain
statistique, ni SLO, ni exactitude industrielle globale.

Cette entrée couvre les observations closes jusqu'au 5 septembre à
10:20:35 UTC. F, publié dans `71895104`, conserve ses
[preuves de qualification propres](receipts_vertical_20260905/f_qualification/review.json).
E garde ses 324/324 portes complètes et 33/33 + 33/33 ciblées, ainsi que
le snapshot du [certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md).
D conserve sa qualification historique 323. Aucun de ces tests ou
certificats n'est transféré implicitement d'une version à l'autre.

Les inspecteurs relisent les campagnes closes du constructeur sans
relancer moteur, CTest ou benchmark. Les pièces nécessaires sont
conservées sous `audits/`, avec référence aux octets déjà capturés lorsque
cela évite une copie redondante. Les identités source/binaire sont liées
à leurs reçus datés ; le HEAD seul ne les remplace pas. Les écritures
restent dans `audits/` ; GCP non utilisé, aucun changement produit ou v6.

L'inspection initiale D était datée de 06:19:38 UTC au HEAD
`e6d33698e62ebecf74dff01c16d7de17149d7a4e`. Les identités « courantes »
des tableaux et paragraphes historiques ci-dessous se rapportent à cet
instant D. La contrelecture nouvelle et sa portée E sont détaillées dans
la section D/E ci-dessous.

La [preuve indépendante structurée](receipts_20260905/qualification_independent.json)
conserve les listes exactes de tests, les commandes historiques, leurs
codes et durées, les écarts de sources et les hashes des entrées de
l'inspecteur. Elle résulte de
[verify_qualification_20260905.py](verify_qualification_20260905.py),
écrit sans importer le juge JUnit ou les verdicts calculés du constructeur.
Les résumés constructeur sont comparés aux XML, inventaires, snapshots
et octets ; leur seul statut `passed` n'est pas le critère d'acceptation.

## Raccord MEB privé au Builder

La [campagne indépendante du Builder](receipts_meb_builder_20260905/README.md)
qualifie séparément l’overlay privé épinglé à `6e517c57`, avec helper
`33255ebc` et référence F `f75a136a`. Ses trois builds O2, UBSan et
instrumenté passent chacun 3 444 appels locaux et 60 wrappers ; huit
injections d’exception et quatre mutants de copies privées ciblent la
persistance, les miroirs et la charge prospective. Le même résultat est
rejugé normalement et sous Python optimisé. Les vingt dépendances locales
par build sont liées au mapping et aux transformations déclarées.

Ces compilations nouvelles sous `audits/` sont distinctes des campagnes
CTest historiques ci-dessous. Elles ne qualifient ni CLI, ni archive,
ni intégration produit et n’attribuent aucun gain de temps. Les injections
`runtime_error` au wrapper prouvent la propagation, sans exposer son
résultat interne après déroulement de pile.

## Résultats confirmés

Ce tableau conserve l'inspection historique C/D de 06:19 UTC ; les
campagnes E et F ont leurs sections et leurs reçus propres ci-dessous.

| Reçu constructeur relu | Exécutions dans les XML publiés | Liaison et limite vérifiées |
| --- | --- | --- |
| [Release D complète](../receipts/meb_full_release_20260905/README.md) | 323/323 | 140 sources, 37 binaires ; identité avant/après et avec les octets locaux courants |
| [MEB intégrée Release](../receipts/meb_lazy_integrated_20260905/release/summary.json) | 32/32 | 140 sources, 9 binaires ; identité avant/après et courante |
| [MEB intégrée ASan/UBSan](../receipts/meb_lazy_integrated_20260905/sanitized/summary.json) | 32/32 | 140 sources, 9 binaires ; identité avant/après et courante |
| [Arithmétique Release C complète](../receipts/arithmetic_gates_20260904/full_release/summary.json) | 316/316 | 139 sources stables pendant la campagne ; 36 binaires conservés ; trois sources ont changé avec D |
| [Arithmétique ciblée](../receipts/arithmetic_gates_20260904/README.md) | 24/24 Release et 24/24 ASan/UBSan | Les quatre binaires ciblés correspondent toujours aux hashes publiés |
| [Autorité Boost](../receipts/arithmetic_boost_20260904/README.md) | 8/8 integer et 16/16 lanes | Deux binaires conservés ; branche Boost réellement présente dans integer ; lanes reste OBig et littéraux |

Pour chacun des huit XML, tous les noms sont uniques et égaux à l'ensemble
attendu, tous les statuts sont `run`, les compteurs sont cohérents et aucun
élément `failure`, `error` ou `skipped` n'est présent. Les quatre campagnes
avec snapshots complets ont aussi leurs commandes de configuration,
construction, inventaire et CTest terminées avec le code 0. Les mutants
comptent comme portes réussies lorsque leur rejet attendu est observé ;
ils ne deviennent pas des exécutions nominales réussies.

La Release D utilise explicitement une construction **incrémentale** dans
`build/v7_meb_qualification`, distincte de C. Les reçus observent 232,82 s
de construction et 574,05 s de temps réel CTest, avec 574,1168 s mesurées
autour de la commande entière par le runner. Ces temps appartiennent à
la qualification historique, pas à une mesure de débit de cet audit.
La vérification actuelle de CMakeCache, CTestTestfile, compile_commands
et du CLI correspond aux pins publiés. Le CLI D conserve le SHA-256
`127c5f923fcc9618d826b89dedda4de0f5201ea48e27330e2ea68e83d76a1b3f`.
Les binaires C restent des témoins historiques ; la commande générique
utilisant un ancien `build/v7/mhgp7` doit continuer à être attribuée à C
jusqu'à sa reconstruction.

Les options ASan/UBSan observées dans le reçu ciblé sont
`detect_leaks=1:halt_on_error=1` et
`halt_on_error=1:print_stacktrace=1`. L'audit contrôle l'intégrité de cette
observation historique ; il n'observe aucun processus sanitizer actuel.

## Intégrité et autorité indépendante

Les sceaux couvrent exhaustivement 48 fichiers publics pour Release D,
75 pour MEB ciblée et 64 pour l'arithmétique ; le manifeste Boost couvre
ses 24 fichiers publics, hors manifeste lui-même. Tous les hashes
correspondent. Aucun fichier ajouté hors sceau n'a été découvert dans
ces quatre dossiers.

Les originaux privés encore disponibles confirment 39 copies exactes et
une projection par ajout d'un LF pour Release D, puis 71 copies exactes
et deux ajouts de LF pour la MEB ciblée. Ces trois transformations sont
déclarées dans les manifestes ; aucun contenu d'inventaire n'a été modifié.
Le journal complet D décompressé fait 7 679 311 octets et demeure identique
à `build/v7_meb_qualification/Testing/Temporary/LastTest.log`, avec le hash
`42007892ba39f3627f103002fbe19963f0051f2ba0ee8c6e05a7439b105f6f58`.

Pour Boost, la contrelecture dépasse le résumé publié : les hashes du
dump de macros privé et du véritable fichier `.o.d` correspondent aux
pins de provenance. Le premier contient `INTEGER_GATE_BOOST=1` et
`BOOST_VERSION=108300` ; le second dépend effectivement des headers privés
`boost/multiprecision/cpp_int.hpp` et `boost/version.hpp`. La sortie
historique integer annonce `authority=obig_literals_and_boost`.
Cela lève l'objection « branche facultative jamais compilée » pour cette
porte entière précise. Cela ne fournit ni une seconde implémentation
du pipeline ni une autorité Boost aux tests géométriques lanes.

## Fraîcheur et suite constructive

Le défaut de fraîcheur C/D relevé à 06:19 est **clos**. À cet instant,
l'ancien manifeste C refusait avec le code 1 les trois sources modifiées
par D, tandis que les 140 pins D correspondaient. L'entrée d'audit a depuis
été actualisée puis consolidée : ces observations restent historiques,
aucune actualisation D supplémentaire n'est demandée.

Le contrôleur [actuel](verify_current.py) utilise le manifeste v2 à
variantes entières D/E/F : il distingue octets périmés (code 1) et manifeste
invalide (code 2), sans union de sources de variantes ni `assert`. Son code 0
identifie une variante et sa portée ; il ne qualifie pas à lui seul les
reçus ou les binaires. Les contrelectures ci-dessous établissent les
liaisons des campagnes E et F effectivement closes. Les anciens pins bruts
et leurs domaines demeurent inchangés.

La prochaine qualification doit être déclenchée par un changement réel
des sources, du graphe compilé, du domaine ou des tests requis. Pour lever
les obstacles à l'exactitude industrielle, conserver séparément les preuves
de couverture/composition, les refus de domaine et de ressources, la
verticale et les poids, puis le protocole de coût de bout en bout. Un
nombre de portes vertes ne remplace aucun de ces contrats.

Rejeu de cette inspection, sans build, CTest ni mutation extérieure :

```bash
python3 -B -O morsehgp3D_v7/audits/verify_qualification_20260905.py
python3 -B morsehgp3D_v7/audits/verify_qualification_20260905.py --self-test
python3 -B -O morsehgp3D_v7/audits/verify_qualification_20260905.py --self-test
```

Les trois commandes ont le code 0. Les autotests comprennent un cas
positif et neuf rejets : vide, doublon, nom absent, statut non exécuté,
compte incohérent, failure, error, skipped et failure récapitulé.
Leurs [reçus normal](receipts_20260905/qualification_selftest_normal.json)
et [optimisé](receipts_20260905/qualification_selftest_optimized.json)
sont identiques. Le rejeu complet demande les artefacts locaux sous build
explicitement nommés ; leur absence fait échouer cette inspection locale
et ne rend pas rétroactivement invalide le reçu public historique.

## D/E : nouvelle contrelecture des campagnes closes

Les [preuves mono conservées](receipts_front_compiled_20260905/qualification/capture_manifest.json)
couvrent les 66 fichiers du reçu constructeur `meb_q2_mono_20260905`,
soit 581 074 octets copiés sans modification. Les deux fichiers Markdown
historiques portent seulement un suffixe `.snapshot.txt` pour rester des
pièces brutes. Le [rejeu indépendant](receipts_front_compiled_20260905/qualification/replay_live.json)
recalcule les chaînes de digests et parse directement stdout, stderr et
GNU time, sans importer le parseur constructeur. Il vérifie les 63 entrées
du manifeste, les deux listes de 64 hashes, 58 copies d'origine exactes,
14 dépendances et 189 pins locaux distincts. Les 50 sources D de construction
correspondent aux octets Git de `a32dc78f` ; les 50 E correspondent au
worktree observé. Les commandes et frontières source/binaire fournissent
une liaison de construction enregistrée, sans attestation hermétique.

| Séparation | D, mur externe | E, mur externe | Réduction observée |
| --- | ---: | ---: | ---: |
| 8 | 189,0003 s | 184,1782 s | 2,5514 % |
| 10 | 192,5560 s | 192,4773 s | 0,0409 % |
| 12 | 198,6419 s | 192,7302 s | 2,9761 % |

Chaque processus termine avec le code 0, stderr vide, GNU time code 0 et
un ordre chronologique cohérent entre 06:30:41 et 06:51:49 UTC. Chaque bras
utilise le même uniforme 8k, coord=65536, seed=3, `smax=11`,
`--complete-incidences`, CSR et digest, sans archive, avec affinité CPU 6
et les trois réglages de sérialisation à 1. L'hôte partagé est un EPYC
9V74, 8 processeurs logiques disponibles et 33 657 716 736 octets de RAM.
Les options et nombres d'ouvriers sont observés ; la création effective
de threads n'est pas mesurée par ce lanceur. Une seule paire D puis E par
séparation ne démontre ni gain statistique, ni p95, ni SLO.

L'égalité D/E puis entre séparations porte sur les dix cardinalités par K,
les dix digests forêt et leur chaîne `digest_all`, ainsi que les compteurs
de complétion et les objets après préfiltre. Les sorties communes comptent
3 113 381 survivants, 4 384 229 événements, 26 434 998 facettes,
26 434 988 fusions, 4 095 793 deltas et 77 939 nœuds. Le digest global est
`4c3ceb0498990bafa41a9e43d0bffe25a3fee579b12b5d34365f3578f526a0e7`.
**Avant préfiltre, les nombres émis, les morts de profondeur et le digest
des candidats changent avec la séparation** : respectivement 3 144 017,
3 129 992 et 3 123 497 émis ; 30 636, 16 611 et 10 116 morts. Leur
variation conserve exactement les mêmes survivants. Sans archive exportée,
l'audit constate l'égalité de ces projections imprimées, pas une comparaison
d'octets des forêts complètes. `normalized_horizontal_h0_candidate`,
`profile_complete_k10` et `vertical_maps=none` restent les types observés.

Les [pièces de qualification E](receipts_front_compiled_20260905/qualification/e_tests_capture.json)
préservent aussi les trois campagnes privées closes, 32 fichiers chacune,
et le [rejeu structuré](receipts_front_compiled_20260905/qualification/e_tests_live.json)
contrôle indépendamment inventaires, XML, blocs LastTest, commandes,
frontières de fraîcheur, sources et binaires locaux. Le LastTest complet
E de 7 683 944 octets est conservé compressé sans perte.

| Campagne E | XML et blocs LastTest passés | Sources stables | Binaires stables | CTest, mur externe |
| --- | ---: | ---: | ---: | ---: |
| Ciblée Release | 33/33 | 140 | 9 | 3,9438 s |
| Ciblée ASan/UBSan | 33/33 | 140 | 9 | 45,7919 s |
| Complète Release | 324/324 | 140 | 37 | 627,8657 s |

Les noms attendus sont uniques et identiques aux inventaires et journaux ;
aucun `failure`, `error`, `skipped` ou statut non exécuté n'est accepté.
La liste complète E est exactement celle des 323 noms D augmentée de
`mhgp7_meb_lazy_q2_reject_shell`. Cette porte tue le mutant avec le code
de sortie attendu 4 et le message `divergence=differential.status_reason` ;
le wrapper CTest réussit avec le code 0. Les quatre commandes de chaque
campagne terminent avec 0. Les builds ciblés sont isolés ; le full E
construit **incrémentalement** dans `build/v7_next_q2_qualification`, puis
lance CTest à 07:04:57 UTC. Le CLI E garde le SHA-256
`df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6`.
Les flags et options ASan/UBSan correspondent aux reçus, sans promotion
d'un résultat instrumenté vers le binaire Release.

Les deux inspecteurs
[mono](receipts_front_compiled_20260905/qualification/verify_q2_receipt.py)
et [CTest](receipts_front_compiled_20260905/qualification/verify_e_tests.py)
se rejouent sans exécuter le produit. Par défaut, ils lisent les snapshots
portables ; `--live` vérifie aussi les artefacts locaux nommés, dont
l'absence ou la dérive fait refuser cette inspection locale. Leurs
[contrôles normal et optimisé](receipts_front_compiled_20260905/qualification/inspector_checks.json)
comprennent les rejeux complets et, pour chacun, un positif et neuf rejets
ciblés. Toutes les commandes conservées ont le code 0. Les constats clos
remplacent l'ancienne attente de qualification E ; les verrous de couverture,
composition, verticale, poids et coût HGP complet restent des contrats
séparés.

## Harnais et classification des campagnes

Les demandes d'enregistrement C1 et de nettoyage du harnais sont fermées. Le [reçu de classification](receipts_20260904/campaign_current.json) vérifie sept tests normal/-O, dix motifs reconnus aux frontières K2/K10 et un vrai refus MEB sur 11 points : code 2, `engine_refused/resource_exhausted`, ordre 2, zéro succès moteur. Les motifs inconnus et invariants restent `invalid`, un timeout reste `censored`. Les deux CTests enregistrés passent dans le [reçu ciblé](receipts_20260904/campaign_registration_current.json), puis dans la suite D.

Le [reçu du lanceur apparié](receipts_20260904/paired_runner_delta_current.json) vérifie les quatre portes Python normal/-O : K5/K10, versions effectives v6/v7 distinctes du rôle, séparations 8/10/12, mode sérialisé, 24 positifs du parseur, 228 rejets étendus et 16 campagnes factices. Les campagnes contrôlent les objets entre bras et entre séparations ; les refus, dérives et descendants interrompus restent conservés. L'égalité de hashes source/binaire ne prouve pas à elle seule leur liaison de construction.

Le [correctif de sonde CI](receipts_iteration3/sonde_ci_current.json) passe ses 23 scènes normal/-O sous environnement hérité simulé. L'appel brut avec `LD_LIBRARY_PATH` conserve son refus de code 2 et stdout vide ; seuls les inventaires nominaux nettoient les sept variables déjà refusées par le lanceur. Ces résultats sont des portes locales sur faux binaires, distinctes des runs GitHub.

Le lanceur apparié historique exige `verified_events_only`, inclut processus externe et digest dans le temps et n'affirme pas un p95 ou le coût HGP complet. Le banc d'incidences fournit l'autre objet et sa classification. La sémantique consommée, les ordres effectifs et les plafonds doivent être appariés avant toute interprétation de performance.

## Autorités des mesures et du cloud

La contrelecture B/C historique reste dans son [reçu](receipts_iteration3/constructor_receipts_review.json) : 46 fichiers Release, 292 noms CTest, puis 22 fichiers de six processus appariés sur un seul uniforme 8k, K1..10, `verified_events_only`. Les réductions de temps de 15,88 à 17,24 % sur cette entrée appartiennent à cette campagne, avec une seule paire par séparation ; ce ne sont ni les résultats actuels D/E, ni des mesures avec complétion. Le [bilan constructeur](../docs/RESULTATS_MONO_20260904.md) reste la source de ses mesures.

La proposition initiale G4 n'est plus un plan à exécuter : le constructeur a publié ses [résultats G4](../docs/RESULTATS_G4_20260904.md), leurs [pièces de contrôle](../receipts/gcp_requalified_20260904/public_review.json) et son [constat de fermeture daté](../receipts/gcp_handoff_20260905.json). Ces fichiers ne sont pas un inventaire live. Cet audit n'utilise pas GCP, ne démarre aucune session et ne transforme pas ces observations GCC11/CUDA en preuve du binaire local GCC13. Les résultats CI publiés sont attribués séparément dans la [passation constructeur](../PASSATION.md#cloud-et-ci) ; les anciens échecs ne sont pas réécrits.

Les notes transitoires regroupées ici sont retirées de l'entrée active. Leurs octets restent accessibles au commit indiqué dans le [registre de consolidation](receipts_front_20260905/documentation_retirement.json), avec leur hash et leur rapport de remplacement. Les preuves brutes et fixtures permanentes restent inchangées.

## Qualification F désormais fermée

La [contrelecture F](receipts_vertical_20260905/f_qualification/results_live.json)
confirme 339/339 portes complètes et 48/48 dans chacun des deux bras
ciblés, sans échec ni skip. Les listes de tests, commandes, XML et blocs
LastTest concordent ; les sorties JUnit explicitement tronquées par
CTest sont comparées comme préfixes aux journaux complets conservés.
Les inventaires propres aux campagnes portent sur 143 fichiers de sources
et de références, ainsi que 39/11/11 binaires stables. Les 51 dépendances du build CLI constituent
une couverture distincte.

Le mutant réel de double crédit retrouve trois témoins nominaux contre
huit fautifs, avec son code de refus attendu 4. Le bras instrumenté sans
observateur d’allocations confirme les sémantiques ; ses compteurs nuls
ne prouvent pas une absence d’allocation. Les [rejeux de l’inspecteur](receipts_vertical_20260905/f_qualification/captured_optimized.json)
restent effectifs sous Python optimisé. Ses dix corruptions sont rejetées.

La preuve de conservation LIFO/masques/comptes demeure liée au delta F
épinglé. Les tests horizontaux de l’auditeur restent attribués à E, et
aucun export vertical ni pondéré n’est ajouté par cette qualification.
Cette contrelecture n'a relancé ni moteur ni CTest.

## Prototype MEB privé : qualification locale distincte

La [note MEB à deux budgets](MEB_DOUBLE_BUDGET_COURANT.md) ferme son
raccord local à F, avec une preuve des charges et de la représentation
q4. Les [nouveaux reçus](receipts_meb_dual_20260905/README.md) séparent
les portes privées constructeur et les sondes propres de l'auditeur :
3 430 MEB contre un oracle Gram rationnel par build O2/UBSan, 1 507
ordinaux par build et trois copies fautives détectées. Ce travail ne
remplace aucune campagne F, ne modifie pas ses 339/48/48 tests et
n'exécute aucune tour. L'intégration par ordre et les nouveaux schémas
restent à qualifier sur leurs propres octets.

## F mono et paliers : observations closes

La [revue indépendante](receipts_resolver_20260905/qualification/review.json)
vérifie les 69 fichiers du reçu mono publié, ses manifestes et les deux
listes SHA, puis les 61 projections contre les originaux privés. Les
[captures](receipts_resolver_20260905/qualification/capture_manifest.json)
ajoutent 63 pièces et référencent 28 pièces antérieures identiques,
notamment la paire `s=8` et le build F. Le
[lecteur portable](receipts_resolver_20260905/qualification/verify_observations.py)
parse les sorties sans importer les juges constructeur. Ses
[contrôles normal et optimisé](receipts_resolver_20260905/qualification/inspector_checks.json)
passent, avec un positif et onze rejets ; la lecture locale constate
122 pins concordants. Ces contrôles n'exécutent pas le moteur.

| Entrée uniforme, seed 3 | E, processus | F, processus | Pic F, KiB | Résultat moteur F |
| --- | ---: | ---: | ---: | --- |
| 8 000, `s=8` | 187,677 s | 188,969 s | 2 299 856 | Code 0, paire égale |
| 8 000, `s=10` | 190,077 s | 185,660 s | 2 300 476 | Code 0, paire égale |
| 8 000, `s=12` | 184,878 s | 190,039 s | 2 302 616 | Code 0, paire égale |
| 16 000, `s=8` | Non mesuré | 413,816 s | 5 361 880 | Code 0, K1–10 publiés |
| 32 000, `s=8` | Non mesuré | 569,876 s jusqu'au refus | 7 100 740 | Code 2, refus K9 |

Chaque bras utilise `coord=65536`, `smax=11`, complétion des incidences,
CSR, digest inclus, aucune archive, CPU logique 6 et les trois réglages
de sérialisation à 1. Les six bras 8k concordent sur les dix cardinalités
et les onze digests forêt/global. Les compteurs de travail ne sont pas
exigés identiques entre séparations. Les paliers 16k/32k n'ont aucun bras
E de comparaison. Les métadonnées distinguent les observations froides
uniques sur hôte partagé de tout gain statistique. Les types restent
`normalized_horizontal_h0_candidate`, `authority=status_terminal`,
`callbacks=provisional` et `vertical_maps=none`.

À 32k, `observations_completed` signifie que le **reçu est clos** : le
moteur a refusé. GNU time et le journal de commande confirment le code 2,
sans timeout ; stdout est vide. Les diagnostics K2–8 et les 108,089 s de
complétion déjà dépensées sont provisoires. Le temps jusqu'au refus ne
constitue pas un temps de calcul d'une tour complète.

Le motif K9 borne les **occurrences de core avant dédoublonnage**. La
[source F épinglée](receipts_resolver_20260905/qualification/snapshots/source/silent_incidence.hpp)
charge `core_records` une fois par support supprimé, donc selon la somme
des `q` des cofaces directes traitées, puis trie et dédoublonne avant de
renseigner `core_facets`. Le contrôle du catalogue direct a un autre
motif. Le `core=0` imprimé à K9 indique que le compteur de facettes uniques
n'a pas encore été affecté ; il ne signifie pas zéro occurrence traitée.
Le refus et ce contrôle impliquent 8 millions d'occurrences chargées et
une suivante refusée. Le total nécessaire pour achever K9, celui de K10
et leur mémoire prospective ne sont pas mesurés.

Les limites restent distinctes : 600 s par processus, 26 GiB d'espace
virtuel via `RLIMIT_AS`, proxy partiel de payload de 16 GiB, puis caps
par ordre de 8 millions d'occurrences, 2 millions de pas/cofaces et
1 milliard de visites/supports MEB. Ni la borne d'espace virtuel ni le
proxy de payload ne désignent une limite de RSS ; le proxy ne couvre pas
toutes les allocations. À 16k, K10 consomme déjà 833 506 587 supports MEB.

Sur les succès, la complétion silencieuse représente environ 35–37 %
du mur pipeline, la génération 32–34 %. `fold_wall` englobe des coûts de
complétion et de digest : ne pas l'additionner à ces sous-coûts. Le travail
utile suivant consiste à précompter les occurrences et examiner leur
compression ou leur interning, avec plafonds séparés de travail, stockage
temporaire et sortie. Une hausse des caps demande d'abord une facture
prospective des capacités et des octets des structures concernées. Les
[propositions mémoire sur la coexistence](RETOUR_MEMOIRE_COURANT.md)
restent un chantier distinct de ce refus mono.

## Qualification native MEB v2

La [contrelecture propre du reçu natif](receipts_meb_native_20260905/README.md) ferme le chemin privé `NoObserver` : 9 351 états locaux confrontés à F et Trace avant puis après mesure, avec champs complets hors chrono et captures consommées dans les boucles mesurées. Le build strict O2, le désassemblage et le reçu de mesure ont leurs sceaux distincts. L'auditeur ne relance aucun binaire ; ses lecteurs réexaminent les octets conservés, y compris l'échec de compilation v1.

Les 1 325 812 entrées comptent les appels supérieurs, juges et replis imbriqués. Les 4 699 groupes et sept passages mesurés ne forment pas une campagne de tour. La [revue du coût](receipts_meb_native_20260905/cost_review.md) sépare réduction des candidats, ralentissement du cas n=2 répété et contrôle P0 sensible à l'ordre dans les petits lots. Le suivi uniforme à 64 répétitions et dix paires est déjà préparé et contre-lu ; ses mesures restent à produire. Cette qualification native ne remplace ni l'oracle rationnel antérieur ni la qualification du port intégré.

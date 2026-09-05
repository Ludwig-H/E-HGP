# Qualification indépendante de la v7 — 5 septembre 2026

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Audit de traçabilité CPU au HEAD
`e6d33698e62ebecf74dff01c16d7de17149d7a4e`, le 5 septembre 2026 à
06:19:38 UTC. Les écritures de cette inspection restent dans `audits/`.
GCP non utilisé ; aucun changement des sources produit ou de la v6.

Actualisation de portée après clôture : le constructeur a commencé E q2
dans le worktree. Les mentions « courant » ci-dessous désignent l'inspection
datée de D, et non ces quatre sources E ultérieures. Un rejeu de cet
inspecteur sur E refuse la liaison D ; il n'invalide pas les reçus historiques.

**Les reçus du delta D sont intègres et liés aux sources et binaires encore
présents. Le défaut de fraîcheur de l'ancien audit ne constitue donc plus
une objection à la qualification CPU bornée du delta D.** L'inspection
confirme les 323 exécutions rapportées par le constructeur ; elle ne les
a pas relancées. Elle ne certifie ni l'objet HGP complet, ni une performance
industrielle. La fermeture mathématique du delta MEB relève de sa revue
séparée.

La [preuve indépendante structurée](receipts_20260905/qualification_independent.json)
conserve les listes exactes de tests, les commandes historiques, leurs
codes et durées, les écarts de sources et les hashes des entrées de
l'inspecteur. Elle résulte de
[verify_qualification_20260905.py](verify_qualification_20260905.py),
écrit sans importer le juge JUnit ou les verdicts calculés du constructeur.
Les résumés constructeur sont comparés aux XML, inventaires, snapshots
et octets ; leur seul statut `passed` n'est pas le critère d'acceptation.

## Résultats confirmés

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

À l'ouverture de cette inspection, `verify_current.py` retourne **1** en
Python normal et `-O` pour trois fichiers : `CMakeLists.txt`,
`src/core/mutants.hpp` et `src/forest/silent_incidence.hpp`.
Ce sont aussi les seuls écarts entre les 139 pins de Release C arithmétique
et les fichiers courants. Les 140 pins des trois campagnes D correspondent
intégralement. La bonne fermeture consiste donc à actualiser l'entrée
d'audit avec les conclusions D et leurs preuves, en conservant l'attribution
historique des 316 portes C. Il n'est pas nécessaire de réclamer une
nouvelle suite complète pour un défaut d'intégrité qui n'a pas été observé.

Le contrôleur de fraîcheur existant remplit sa fonction : il distingue
octets périmés (code 1) et manifeste invalide (code 2), vérifie les chemins
et SHA-256 et ne repose pas sur `assert`. Son code 0 ne vérifierait pas,
à lui seul, les reçus, les binaires ou la portée d'un verdict. L'inspection
présente ajoute précisément ces liaisons pour le delta D.

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

## Harnais et classification des campagnes

Les demandes d'enregistrement C1 et de nettoyage du harnais sont fermées. Le [reçu de classification](receipts_20260904/campaign_current.json) vérifie sept tests normal/-O, dix motifs reconnus aux frontières K2/K10 et un vrai refus MEB sur 11 points : code 2, `engine_refused/resource_exhausted`, ordre 2, zéro succès moteur. Les motifs inconnus et invariants restent `invalid`, un timeout reste `censored`. Les deux CTests enregistrés passent dans le [reçu ciblé](receipts_20260904/campaign_registration_current.json), puis dans la suite D.

Le [reçu du lanceur apparié](receipts_20260904/paired_runner_delta_current.json) vérifie les quatre portes Python normal/-O : K5/K10, versions effectives v6/v7 distinctes du rôle, séparations 8/10/12, mode sérialisé, 24 positifs du parseur, 228 rejets étendus et 16 campagnes factices. Les campagnes contrôlent les objets entre bras et entre séparations ; les refus, dérives et descendants interrompus restent conservés. L'égalité de hashes source/binaire ne prouve pas à elle seule leur liaison de construction.

Le [correctif de sonde CI](receipts_iteration3/sonde_ci_current.json) passe ses 23 scènes normal/-O sous environnement hérité simulé. L'appel brut avec `LD_LIBRARY_PATH` conserve son refus de code 2 et stdout vide ; seuls les inventaires nominaux nettoient les sept variables déjà refusées par le lanceur. Ces résultats sont des portes locales sur faux binaires, distinctes des runs GitHub.

Le lanceur apparié historique exige `verified_events_only`, inclut processus externe et digest dans le temps et n'affirme pas un p95 ou le coût HGP complet. Le banc d'incidences fournit l'autre objet et sa classification. La sémantique consommée, les ordres effectifs et les plafonds doivent être appariés avant toute interprétation de performance.

## Autorités des mesures et du cloud

La contrelecture B/C historique reste dans son [reçu](receipts_iteration3/constructor_receipts_review.json) : 46 fichiers Release, 292 noms CTest, puis 22 fichiers de six processus appariés sur un seul uniforme 8k, K1..10, `verified_events_only`. Les réductions de temps de 15,88 à 17,24 % sur cette entrée appartiennent à cette campagne, avec une seule paire par séparation ; ce ne sont ni les résultats actuels D/E, ni des mesures avec complétion. Le [bilan constructeur](../docs/RESULTATS_MONO_20260904.md) reste la source de ses mesures.

La proposition initiale G4 n'est plus un plan à exécuter : le constructeur a publié ses [résultats G4](../docs/RESULTATS_G4_20260904.md), leurs [pièces de contrôle](../receipts/gcp_requalified_20260904/public_review.json) et son [constat de fermeture daté](../receipts/gcp_handoff_20260905.json). Ces fichiers ne sont pas un inventaire live. Cet audit n'utilise pas GCP, ne démarre aucune session et ne transforme pas ces observations GCC11/CUDA en preuve du binaire local GCC13. Les résultats CI publiés sont attribués séparément dans la [passation constructeur](../PASSATION.md#cloud-et-ci) ; les anciens échecs ne sont pas réécrits.

Les notes transitoires regroupées ici sont retirées de l'entrée active. Leurs octets restent accessibles au commit indiqué dans le [registre de consolidation](receipts_front_20260905/documentation_retirement.json), avec leur hash et leur rapport de remplacement. Les preuves brutes et fixtures permanentes restent inchangées.

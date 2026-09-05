# Qualification ciblée du producteur FULL à cache facultatif

5 septembre 2026. public_status=not_claimed, CPU de référence, entrée u16.

Les deux builds neufs ferment **14/14 CTests Release et 14/14 ASan/UBSan**, après relecture des captures et des liaisons.

Les six exécutables couvrent le certificat structurel, le producteur eager, ses injections mémoire, le producteur lazy, ses injections mémoire et le digest sémantique.

Les commandes, stdout, stderr, codes, délais, JUnit et LastTest sont copiés byte pour byte dans [capture/](capture/). Les échecs éventuels ne sont ni effacés ni réécrits.

Protocole prévu — Release : O3/NDEBUG. Instrumenté : O1/g/NDEBUG, AddressSanitizer et UndefinedBehaviorSanitizer, sans PIE, avec ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 et UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1. Aucune escalade ni désactivation LSAN.

Construction CPU0, parallélisme 2 et plafond 600 s ; CTest CPU6, parallélisme 1, 60 s par porte et 120 s pour l’appel complet. Ce sont des limites de qualification, pas un benchmark.

Les cartes de sources, les six binaires et leurs options/dépendances sont liés avant/après. Les 521 headers Boost consommés sont pré-épinglés et recontrôlés ; seul leur inventaire inerte et leurs SHA sont exportés, pas leur code ni les binaires. Les headers système ne sont pas tous pré-épinglés : **build frais, non hermétique**.

HEAD et le statut v7 sont observés sans exiger la stabilité globale du dépôt concurrent ; la stabilité des sources effectivement qualifiées reste obligatoire.

## Observations des exécutions closes

- release : 14/14 ; construction 61.468143 s ; CTest 0.765695 s.

Compteurs repris littéralement des mêmes sorties JUnit :

```text
mhgp7_full_certificate: full_certificate mode=--selftest authority=structural_only positives=11 build_refusals=0 read_refusals=3 allocation_failures=0 checks=68 failures=0 floor=1
mhgp7_full_certificate_rejects: full_certificate mode=--rejects authority=structural_only positives=13 build_refusals=45 read_refusals=19 allocation_failures=15 checks=218 failures=0 floor=1
mhgp7_full_gabriel: full_gabriel mode=--selftest authority=relative_supplied_catalogues oracle=OBig_FULL clouds=10 catalogues=30 records=136 orders=67 cuts=1492 isolated_cuts=1379 permutations=27 named=24 rejections=0 shell_refusals=0 authority_refutations=0 checks=43701 failures=0 floor=1
mhgp7_full_gabriel_rejects: full_gabriel mode=--rejects authority=relative_supplied_catalogues oracle=OBig_FULL clouds=13 catalogues=36 records=157 orders=68 cuts=1524 isolated_cuts=1403 permutations=27 named=24 rejections=80 shell_refusals=1 authority_refutations=1 checks=44982 failures=0 floor=1
mhgp7_full_gabriel_allocation: full_gabriel_allocation positives=3 portal_positives=3 allocations=102 fault_runs=102 denied=102 escaped=0 checks=786 failures=0 floor=1
mhgp7_full_gabriel_lazy: full_gabriel_lazy mode=--selftest clouds=6 orders=27 lazy_runs=81 cuts=3192 records=114 permutations=9 terminal_orders=18 named_J1=6 named_two_step=3 named_cache_hit=2 cache_hits=4 cache_skips=65 negative_global=1 rejections=0 checks=99137 failures=0 floor=1
mhgp7_full_gabriel_lazy_rejects: full_gabriel_lazy mode=--rejects clouds=6 orders=27 lazy_runs=81 cuts=3192 records=114 permutations=9 terminal_orders=18 named_J1=6 named_two_step=3 named_cache_hit=2 cache_hits=4 cache_skips=65 negative_global=1 rejections=127 checks=99834 failures=0 floor=1
mhgp7_full_gabriel_lazy_allocation: full_gabriel_lazy_allocation cells=6 positives=18 portal_positives...
mhgp7_full_gabriel_digest: full_gabriel_digest authority=bench_serializer_only oracle=Boost_cpp_int divisions=672 reductions=672 rejections=6 checks=2695 failures=0 floor=1
```

- san : 14/14 ; construction 171.113570 s ; CTest 3.872286 s.

Compteurs repris littéralement des mêmes sorties JUnit :

```text
mhgp7_full_certificate: full_certificate mode=--selftest authority=structural_only positives=11 build_refusals=0 read_refusals=3 allocation_failures=0 checks=68 failures=0 floor=1
mhgp7_full_certificate_rejects: full_certificate mode=--rejects authority=structural_only positives=13 build_refusals=45 read_refusals=19 allocation_failures=15 checks=218 failures=0 floor=1
mhgp7_full_gabriel: full_gabriel mode=--selftest authority=relative_supplied_catalogues oracle=OBig_FULL clouds=10 catalogues=30 records=136 orders=67 cuts=1492 isolated_cuts=1379 permutations=27 named=24 rejections=0 shell_refusals=0 authority_refutations=0 checks=43701 failures=0 floor=1
mhgp7_full_gabriel_rejects: full_gabriel mode=--rejects authority=relative_supplied_catalogues oracle=OBig_FULL clouds=13 catalogues=36 records=157 orders=68 cuts=1524 isolated_cuts=1403 permutations=27 named=24 rejections=80 shell_refusals=1 authority_refutations=1 checks=44982 failures=0 floor=1
mhgp7_full_gabriel_allocation: full_gabriel_allocation positives=3 portal_positives=3 allocations=102 fault_runs=102 denied=102 escaped=0 checks=786 failures=0 floor=1
mhgp7_full_gabriel_lazy: full_gabriel_lazy mode=--selftest clouds=6 orders=27 lazy_runs=81 cuts=3192 records=114 permutations=9 terminal_orders=18 named_J1=6 named_two_step=3 named_cache_hit=2 cache_hits=4 cache_skips=65 negative_global=1 rejections=0 checks=99137 failures=0 floor=1
mhgp7_full_gabriel_lazy_rejects: full_gabriel_lazy mode=--rejects clouds=6 orders=27 lazy_runs=81 cuts=3192 records=114 permutations=9 terminal_orders=18 named_J1=6 named_two_step=3 named_cache_hit=2 cache_hits=4 cache_skips=65 negative_global=1 rejections=127 checks=99834 failures=0 floor=1
mhgp7_full_gabriel_lazy_allocation: full_gabriel_lazy_allocation cells=6 positives=18 portal_positives...
mhgp7_full_gabriel_digest: full_gabriel_digest authority=bench_serializer_only oracle=Boost_cpp_int divisions=672 reductions=672 rejections=6 checks=2695 failures=0 floor=1
```

## Portée et limites

Qualification ciblée de ce lot seulement : ni suite historique 339 ni suite v7 complète. Les positifs n’authentifient pas des catalogues arbitraires ; l’autorité demeure relative aux catalogues Gabriel complets, exacts et réguliers fournis.

Aucune qualification inter-K/CLI/archive, aucun gain de latence, aucun contrat 50k en une seconde ou 100 ms, aucun résultat massif G4 n’en découle.

Les préchecks et anciennes campagnes ne deviennent pas des preuves fraîches. Le fichier de dépendances du précheck Boost sert uniquement à fermer l’inventaire de headers.

[Reclassification](qualification.json), [manifest des octets](manifest.json), [sommes complètes](SHA256SUMS). Les protocoles sous protocol/ sont des copies inertes ; leur présence n’exécute aucune commande. GCP non utilisé.

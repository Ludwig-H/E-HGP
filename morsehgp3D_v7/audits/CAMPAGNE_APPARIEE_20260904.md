# Porte courante de campagne appariée

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La revue statique et les quatre portes Python passent sur les octets épinglés ci-dessous, stables avant/après les tests. Le [reçu courant](receipts_20260904/paired_runner_delta_current.json) conserve les commandes exactes, l'environnement, les sorties et leurs hashes. Les tests utilisent exclusivement une copie sous `audits/`, des binaires factices et un `TMPDIR` sous `audits/`, avec `PYTHONDONTWRITEBYTECODE=1`. Aucun moteur réel ni benchmark n'a été exécuté dans cette vérification.

## Périmètre accepté

[compare_v6_v7.py](../bench/compare_v6_v7.py), lignes 47–158 et 202–304, contrôle les options suivantes :

| Option | Contrôle constaté |
| --- | --- |
| `--kmax 5` ou `10` | Exactement K1..5 ou K1..10, `smax=6` ou `11`, portée, cardinalités et chaîne de digests correspondantes ; aucun ordre supplémentaire accepté. |
| `--reference-version v6` ou `v7` | Version effective du payload distincte du rôle référence/candidat ; fichiers distincts dans le cas v7/v7. |
| `--separations 8 10 12` ou sous-ensemble | Identité de chaque séparation, coordonnées de matrice uniques et comparaison des objets entre séparations. |
| `--serial-stages` | Demande `threads=1`, `fold-inflight=1`, `fold-join=1` ; exige la ligne attestant ces options et `pic_mesure_en_vol` dans `{0,1}`. Le nombre de fils initialement demandé reste consigné. |
| Sans `--serial-stages` | Conserve le nombre de fils demandé, `fold-inflight=2` et `fold-join=0`. |

Le layout doit être CSR sans fallback. Famille, cardinal, seed, coordonnées, backend, autorité du payload et totaux sont contrôlés. Les digests sont complets, uniques et leur chaînage est recalculé. Le temps pipeline doit être fini, positif et compatible avec le temps externe ; le RSS externe et le statut nul de `/usr/bin/time` sont requis.

Les rôles alternent AB/BA. Chaque paire compare digests, cardinalités et domaine de coordonnées ; les compteurs de travail interne peuvent différer. Après égalité de la paire, la même projection est comparée entre séparations (lignes 321–345). Une divergence commune aux deux binaires invalide donc la campagne même si chaque paire est égale. Le résumé exige toutes les exécutions prévues, sans échec ni record invalidé (lignes 354–373).

Le timeout draine le groupe de processus créé, descendants compris. Les refus, sorties partielles et échecs sont conservés. Les hashes des sources, binaires et de `/usr/bin/time` sont vérifiés avant/après chaque exécution ; une dérive invalide les records antérieurs. Ces observations ne verrouillent pas les fichiers et n'attestent pas la construction : `source_binary_binding=source_hashes_and_binary_hashes_only_build_not_attested`.

## Portes exécutées

| Porte sur la copie épinglée | Python normal | Python `-O` |
| --- | --- | --- |
| [compare_campaign_gate.py](../tests/compare_campaign_gate.py) | code 0 | code 0 |
| [incidence_campaign_gate.py](../tests/incidence_campaign_gate.py) | 7 tests, code 0 | 7 tests, code 0 |

Dans chacun des deux modes, la porte appariée rapporte :

```text
compare_campaign_gate=passed parser_rejections=24 extended_rejections=228 argument_rejections=5 campaigns=16 descendants=1 fake_metrics_only=true
```

Les extensions couvrent 24 combinaisons positives du parseur : deux ordres maximaux, deux versions effectives, trois séparations et deux modes d'ordonnancement. Les rejets ciblent notamment version, portée, ordres supplémentaires, séparation et sérialisation. Les cinq rejets d'arguments sont causaux et précèdent la création du dossier de sortie. Les 16 campagnes incluent v7/v7, trois séparations, K5, travail interne modifié, divergence entre séparations, timeout, refus et dérives de source. Commandes et absence de collision des fichiers sont vérifiées. Les gardes restent actives sous `-O`.

La porte d'incidences vérifie aussi parsing, identités, refus typés, censure et conservation des tentatives interrompues ou invalides après import du lanceur courant. Sa portée figure dans [CAMPAGNE_INCIDENCES_COURANTE.md](CAMPAGNE_INCIDENCES_COURANTE.md).

## Limites de qualification

Le lanceur apparié exige `forest_semantics=verified_events_only`. L'égalité porte sur cet objet publié et ne qualifie pas la complétion des incidences silencieuses ni l'objet HGP complet. Le mode `normalized_horizontal_h0_candidate` possède une porte de campagne distincte ; ses tests factices ne prouvent pas davantage l'exactitude du moteur.

`--serial-stages` sérialise les étapes demandées ; il ne démontre pas l'absence de fils auxiliaires. Le reçu conserve `strict_single_thread_qualification=not_claimed`. Les coûts incluent le processus externe et le digest : `time_scope=external_process_including_digest_not_warm_e2e`. Performance, contrat industriel et exactitude HGP restent `not_claimed` ; les campagnes réelles nécessitent leurs propres exécutions et reçus.

| Source vérifiée | SHA-256 |
| --- | --- |
| `bench/compare_v6_v7.py` | `fe22493fcb7494813e79fea9826873cce8cf14918097ce500b44018f1a64f2ef` |
| `tests/compare_campaign_gate.py` | `23cca7c5355a8aba26ba0676ed558a0a25c500b4171e2a2811fb514db9b6fcb5` |
| `bench/incidence_campaign.py` | `6ca21d8b1c89e6baea99ecc3dd414b35d06581df50833e76bac9bdc1d5d1c20a` |
| `tests/incidence_campaign_gate.py` | `d74bca5257bd307b5956a946ca188a217f519ec2dc49e6b0de3ebf33a0ccca0a` |

GCP non utilisé.

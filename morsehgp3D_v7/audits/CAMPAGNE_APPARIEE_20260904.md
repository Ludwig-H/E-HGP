# Porte de campagne appariée v6/v7

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

[compare_v6_v7.py](../bench/compare_v6_v7.py) est un diagnostic de compatibilité et de coût exploratoire, pas une qualification du contrat industriel. Il conserve la référence et le candidat dans deux exécutables distincts et alterne les paires AB/BA en série. Le profil demandé est complet K1..10, `s=8`, CSR sans fallback, `fold-inflight=2`, `fold-join=0`, avec digest ; une version profilée K1..5 ne peut pas satisfaire cette campagne.

## Portes effectives

Chaque réussite exige l'identité de famille, cardinal, seed et nombre de fils ; le backend, le schéma de payload, les dix ordres et les cardinalités publiées sont contrôlés. Les digests sont des lignes complètes, minuscules, uniques et limitées exactement à K1..10 plus `digest_all`. Le chaînage de ces dix digests est recalculé indépendamment avec le domaine historique préservé. Une paire compare les objets et cardinalités publiées, pas les compteurs de travail interne : une optimisation peut légitimement modifier ces derniers.

Le temps pipeline doit être fini, positif et compatible avec le temps externe de processus. Le RSS externe de `/usr/bin/time -v` et son statut nul sont obligatoires. Une sortie de succès avec stderr non vide, un fallback, une réduction de profil ou une route sémantique différente est invalide. Un refus en 2 ou 3 ayant déjà publié stdout est invalide. Un timeout n'est jamais renommé en réussite, même si stdout contient un digest complet.

Le lanceur impose une limite d'espace d'adressage et un timeout par processus, désactive les core dumps et draine précisément son propre groupe de processus, y compris les descendants, après achèvement, interruption ou timeout. Les sorties partielles et échecs sont conservés. Les reçus JSON sont remplacés atomiquement à chaque étape ; le résumé terminal refuse toute campagne incomplète ou divergente.

Les sources `src/`, `cli/`, `oracle/`, CMake, le lanceur, les deux binaires et `/usr/bin/time` sont hashés. Leur stabilité est vérifiée avant et après chaque exécution. Une modification invalide la campagne et ses records antérieurs. Ces observations aux bornes ne verrouillent pas le système de fichiers et ne prouvent pas l'absence d'une modification réversible entre deux observations. Elles ne prouvent pas non plus que chaque binaire a été construit depuis ces sources : le reçu déclare explicitement `source_binary_binding=source_hashes_and_binary_hashes_only_build_not_attested`.

## Régressions exécutées

[compare_campaign_gate.py](../tests/compare_campaign_gate.py) utilise des exécutables factices explicitement déclarés ; aucune valeur qu'ils impriment n'est une mesure scientifique. Les snapshots contrôlés de ses tests de cycle de vie sont des stubs, complétés par une porte distincte sur les hashes de vrais fichiers temporaires. Les gardes ne reposent pas sur `assert`.

```bash
python3 morsehgp3D_v7/tests/compare_campaign_gate.py
python3 -O morsehgp3D_v7/tests/compare_campaign_gate.py
```

Les deux modes passent en code 0 :

```text
compare_campaign_gate=passed parser_rejections=24 campaigns=8 descendants=1 fake_metrics_only=true
```

Les 24 corruptions couvrent notamment les doublons/suffixes/ordres de digest, un chaînage faux, les identités, totaux et schémas incohérents, un temps non fini/nul/hors enveloppe, un RSS absent/nul/dupliqué, un stderr de succès et une portée différente. Les huit campagnes couvrent le succès, un travail interne modifié sans changement d'objet, le refus, le refus avec publication illicite, le timeout, la divergence d'objet et les modifications de source avant ou pendant une exécution. La porte de descendance vérifie séparément qu'aucun enfant du timeout ne reste actif.

Les campagnes réelles doivent être exécutées sur les octets stabilisés
avec leurs propres reçus. Les tests de lanceur ci-dessus ne constituent
aucune mesure de performance. Les campagnes demandées 8k/16k/32k et les
contrats 50k/grande échelle restent des qualifications distinctes ; les
comparaisons de compatibilité de cette note ne remplacent pas l'évaluation
de la route avec complétion silencieuse activée.

| Fichier | SHA-256 |
| --- | --- |
| `bench/compare_v6_v7.py` | `cfe40fd2b00508ec7887970b961482a87839afdf9a7e2539728855feec69ffcb` |
| `tests/compare_campaign_gate.py` | `c0a329475cd4fa6a4138a57172faee1e1ecc95a18e5f2df7c14cab638a89f356` |

GCP non utilisé.

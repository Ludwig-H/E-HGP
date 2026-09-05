# Reçus des ancres verticales, comparaisons p3 et observations F

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Aucun moteur, build, benchmark ou GCP exécuté par l'auditeur pendant cette reprise.

## Reconstruction verticale depuis les tokens

Le [contrat](../CONTRAT_VERTICAL_COURANT.md#5-construction-totale-depuis-born-et-parents) prouve qu'une vraie naissance E possède une ancre trouvable en au plus `|born|` lookups inférieurs. Le [lecteur](../vertical_anchor_replay.py) réalise le scan et propage les ancres par les parents, sans géométrie. Son juge indépendant utilise Gamma rationnel sur les petits nuages.

Les [reçus normal](anchors/normal.json) et [optimisé](anchors/optimized.json) conservent les mêmes résultats pour chacune des provenances scellées O2 et UBSan. Ces provenances désignent les anciennes exécutions C++ E, pas deux nouvelles compilations.

| Entrée du lecteur | Cartes | Carrés horizontaux | Compositions sur deux ordres | Branche particulière |
| --- | ---: | ---: | ---: | --- |
| 16 sorties E originales | 764 | 720 | 400 | 44 naissances, 104 continuations, aucune multifusion source d'ordre au moins deux |
| Un certificat E explicitement réindexé | 101 | 96 | 64 | Cinq misses avant le sixième succès à une naissance K6, niveau 11997 |
| Un flux mathématique synthétique | 54 | 50 | 21 | Une multifusion source |
| Total par provenance scellée | 919 | 866 | 485 | 54 naissances, 59 lookups de naissance, maximum observé de six essais |

Le réindexage est une transformation du certificat avec recanonisation des parents et sorties, pas un run produit sur de nouveaux PointId. Le flux synthétique est construit depuis Gamma complet sur six points de la courbe des moments ; il exerce la multifusion du lecteur, sans qualifier un producteur géométrique indépendant. Le consommateur reçoit uniquement IDs et deltas ; Gamma reste du côté du juge.

Trois corruptions du lecteur sont réfutées : arrêt au premier label (`anchor.birth_not_resolved`), miss interprété comme absence de carte (`judge.vertical_totality` par le juge externe), cible périmée (`anchor.target_not_current`). Un budget nul de lookups de naissance refuse avec `budget.birth_lookup`, mais accepte la famille verticale vide K1. Ce budget borne les lookups de naissance, pas le travail total du lecteur. Ces fautes d'audit ne sont pas des mutants CTest produit.

Le [premier échec de non-vacuité](anchors/initial_nonvacuity_rejection.json) est conservé : les multifusions du corpus E initial étaient toutes à K1, qui n'est pas source d'une verticale. Le flux synthétique séparé complète cette branche ; cet échec de corpus n'est pas un défaut produit.

## Autorité numérique p3

La [preuve](../AUTORITE_VOTE_P3_COURANTE.md) et le [juge borné](../vote_p3_exact_probe.py) traitent les égalités algébriques entre numérateurs de vote et leurs signes par intervalles rationnels `isqrt`. Le plafond rend `indeterminate`. Les [reçus normal](weights/normal.json) et [optimisé](weights/optimized.json) vérifient chacun 27 cas : six égalités, sept signes, huit indécisions attendues et six entrées invalides ; quatre permutations et quatre corruptions d'audit sont exercées.

Les annulations de Pell vérifient séparément les identités exactes et la séparation sous budget, avec des multiplicités sous le plafond courant d'incidences et des stress plus larges explicitement distingués. L'autorité ne porte pas sur les quotients de masses, la condensation ou une API produit. Aucun temps industriel n'est déduit de ces petits calculs.

## Contrelecture des mesures closes E/F

La [revue](qualification/review.json) et le [lecteur portable](qualification/verify_observations.py) vérifient les trois paires E/F 8k à s=8/10/12, le succès F16k et le refus F32k à K9. Le [manifeste de captures](qualification/capture_manifest.json) conserve 63 nouvelles pièces et référence 28 octets déjà présents dans les reçus précédents, sans duplication de s8 ou du build F. Les copies et projections sont déclarées ; les originaux privés ne sont pas requis par le rejeu portable.

Les [rejeux normal](qualification/captured_normal.json) et [optimisé](qualification/captured_optimized.json), ainsi que leurs [contrôles](qualification/inspector_checks.json), passent. Le parseur accepte un positif et rejette onze corruptions ciblées dans chaque mode. La [lecture locale](qualification/results_live.json) confirme aussi 122 pins d'artefacts observés. Un futur changement ou retrait de ces artefacts fait échouer `--live` sans invalider les observations scellées.

Le succès 16k est une tour horizontale complète en 413,816 s, RSS maximal 5 361 880 KiB. Le processus 32k s'arrête par refus après 569,876 s, code 2, `silent_core_record_budget`, stdout vide : aucun digest ni forêt de tour complète n'est publié. Le cap de huit millions porte sur les occurrences avant déduplication ; `core=0` ne signifie pas zéro travail. Le temps jusqu'au refus n'est ni un timeout ni un temps d'achèvement. Les trois paires ne démontrent aucun gain statistique ou meilleur s. RSS, plafond d'espace virtuel et proxy de payload restent distincts.

Le [rapport courant de qualification](../AUDIT_QUALIFICATION_20260905.md) donne l'interprétation et la suite constructive. Les résultats F sont publiés dans `4cc804e50c9effdc6fb65b157df0f8b5168bf60e` ; leur code reste celui de la qualification F propre. Les cartes verticales E et le juge p3 ne sont pas réattribués à F.

## Reproduction et publication

Depuis la racine, sans compilation ni moteur :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/vertical_anchor_replay.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/vertical_anchor_replay.py
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/vote_p3_exact_probe.py --receipt normal
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/vote_p3_exact_probe.py --receipt optimized
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/receipts_resolver_20260905/qualification/verify_observations.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_resolver_20260905/qualification/verify_observations.py --self-test
```

Les quatre premières commandes régénèrent leurs reçus datés sous ce dossier ; un rejeu ne doit pas être confondu avec leurs octets historiques. Le lecteur de qualification écrit seulement sur stdout. L'option `--live` ajoute la vérification locale des artefacts nommés. Les [contrôles de publication](publication_checks.json) complètent la validation du Markdown de tous les audits, que le contrôleur documentaire général exclut, et les portes de fraîcheur normal/-O. Le [manifeste courant](../validation_current.json) épingle les nouvelles preuves sans réécrire les anciens reçus.

# Qualification indépendante du producteur FULL Gabriel

5 septembre 2026. Rapport maintenu : [producteur FULL courant](../PRODUCTEUR_FULL_GABRIEL_COURANT.md). CPU de référence, entrée u16, exploration v7 hors registre, `public_status=not_claimed`. Aucun fichier produit modifié, aucun GCP utilisé.

## Sources et indépendance

Les [19 en-têtes capturés](source_pins.json) sont les dépendances locales du composant `full_gabriel.hpp` e02d163c. Chaque compilation archive son fichier de dépendances effectif, ses options, ses diagnostics, le hash du binaire et ceux des vingt fichiers compilés, pont inclus, dans `*_build.json`. Les mutations portent exclusivement sur les copies privées sous le répertoire temporaire d'audit ; leurs trois patches sont conservés. Les en-têtes système et la toolchain ne constituent pas une capture hermétique.

Le [pont C++](../full_producer_bridge.cpp) reçoit les points, deux catalogues et les coupes ; il ne connaît aucun attendu. Le [juge Python](../full_producer_audit.py) utilise Gram rationnel, supports positifs et puissances pour construire tous les Gabriel des petites fixtures. Un oracle Gamma distinct contrôle ces catalogues et les forêts, sans appel au producteur FullPortal. Il compare niveaux, labels des minima, références parentales, couvertures, racines aux coupes et naturalité par inclusion des minima. Les seuls nombres de racines ou unions de points ne suffisent pas à ce jugement.

Les [fixtures JSON](fixtures.json) et [leur transport](fixtures.txt) portent 18 cas, 100 ordres et deux représentations par ordre. Leurs catalogues complets sont globalement réguliers, n≤7, avec K1 à zéro et K=n. Les dix cas historiques sont réutilisés explicitement ; un singleton et sept nuages de sept points sont ajoutés. Les nouvelles coordonnées et graines sont figées, y compris le témoin `E5_chain_two`. Les métadonnées du JSON conservent les pins et le diagnostic de limite du transport ; les préparations normale et optimisée produisent les mêmes octets. Aucun résultat C++ n'est attribué à la préparation.

## Résultats

| Mesure par binaire nominal | Valeur |
| --- | ---: |
| Ordres distincts / représentations | 100 / 200 |
| Nœuds / minima / références parentales | 1 606 / 1 020 / 1 406 |
| Couvertures / coupes / carrés de naturalité | 1 606 / 16 506 / 17 774 |
| Portails / pas / MEB | 10 / 12 / 22 |
| Ancres normalisées / visites de facettes | 3 552 / 4 790 |
| Réussites à tous les caps exacts / refus cap−1 | 2 / 26 |

Les [sorties O2](O2_output.json) et [ASan/UBSan](sanitized_output.json) sont identiques octet pour octet. Les compteurs de cœur du helper restent nuls. E5 étendue exerce deux pas ; l'ancien corpus ne fournissait pas cette non-vacuité C++. Les treize dimensions de budget sont exercées séparément sur deux cas, avec statut, raison et absence de résultat partiel. Ce ne sont ni une borne RAM ni un délai CPU.

| Mutation privée | Résultat du pont | Rejet indépendant, normal et optimisé |
| --- | --- | --- |
| [Ancienne ancre terminale non normalisée](stale_terminal.patch.txt) | Code 0 ; huit ordres refusés par le produit, dont E5 `full_parent_not_prebatch_root`, forêt vidée | `producer.identity`, car l'ordre publié est remis à zéro |
| [Budget MEB remis à zéro](reset_support_budget.patch.txt) | Code 0, résultats déclarés complets | `producer.named_E5_support_sum` : cinq candidats cumulés sont requis |
| [Facette stricte inconnue omise](omit_portal.patch.txt) | Code 0, résultats déclarés complets | `producer.node_count` : le portail est nécessaire à la bonne forêt |

Les [jugements normaux](judgments_normal.json) et [optimisés](judgments_optimized.json) regroupent chacun deux succès nominaux et trois rejets code 1. Leurs dix sorties individuelles sont conservées. Le jugement régénère les fixtures et contrôle leurs octets avant lecture ; aucune porte ne repose sur `assert`. Les erreurs, refus et résultats négatifs restent conservés.

Les fichiers `*_run.json` portent les cinq codes de sortie, hashes d'entrée/binaire/sorties et options d'exécution. Les cinq processus terminent code 0 ; le juge qualifie ensuite leur sortie, ce qui distingue une exécution achevée d'une forêt correcte. Tous les diagnostics sont vides, y compris ASan/UBSan avec `detect_leaks=1:halt_on_error=1`. Aucun coût industriel n'est tiré des durées de ces petits processus.

## Reproduire

Le [runner](../full_producer_run.py) sépare capture, compilation, exécution et jugement. `prepare` refuse d'écraser une capture existante. Pour une nouvelle campagne, choisir deux dossiers neufs sous `audits/`, passer `--receipt` et `--work`, copier les fixtures scellées et construire/exécuter chaque nom `O2`, `sanitized`, `stale_terminal`, `reset_support_budget`, `omit_portal`. Les binaires et copies de travail ne sont pas versionnés. La capture initiale lit les sources produit du worktree : vérifier ses pins contre ce reçu avant d'attribuer une nouvelle exécution aux mêmes octets.

Le rejeu des preuves existantes ne demande aucune compilation :

```bash
python3 -B morsehgp3D_v7/audits/full_producer_run.py judge
python3 -B -O morsehgp3D_v7/audits/full_producer_run.py judge
```

Ces deux appels régénèrent chacun les attendus une seule fois, puis jugent les cinq sorties. Ils réécrivent les verdicts avec leurs nouvelles dates ; utiliser une copie du dossier de reçus sous `audits/.work*/` et `--receipt` pour garder le présent paquet scellé. Les appels de qualification ont été placés sur CPU0, hors CPU6 alors utilisé par la fenêtre mono du constructeur ; aucun chronométrage de cette fenêtre n'est attribué ici.

## Revues et limites séparées

La [revue sémantique](semantic_review.md) ferme le calendrier, les portails, les ancres, le raccord d'inertie de la paire de catalogues et les transactions. La [première contre-vérification constructeur](constructor_receipt_review.md) conserve Release 7/7 et l'échec LSan/ptrace initial. Son [complément de clôture](constructor_closure_review.md) qualifie séparément le retry ROOT 7/7 sur les mêmes binaires. Le premier journal n'est pas remplacé par le second.

La [revue des deltas](source_delta_review.md) constate 144 pins G sur 145 inchangés et isole exactement le bloc CMake des cinq nouvelles portes. Elle lit également le probe futur sans lui attribuer de run, de digest, de performance ni la qualification de G. Les [trois ajouts documentaires à la publication](publication_document_delta.json) citent ensuite correctement la qualification indépendante et le protocole d'alias futur. Les nouvelles expériences mono du constructeur restent hors de cette campagne indépendante.

La [contrelecture du cache facultatif](lazy_alias_next_step_review.md) traite ensuite la nouvelle question constructeur. La [fixture rationnelle](lazy_alias_fixture.json) confirme séparément onze MEB régulières, le rattachement J=1 et douze états Gamma ; aucun dispatcher paresseux n’est exécuté. Le nouveau protocole et ses gains éventuels ne sont pas attribués au produit e02d.

La réussite reste relative à un index authentique et aux deux catalogues complets, exacts et réguliers. Le juge borné établit ces prémisses pour ses fixtures ; il ne qualifie pas leur génération industrielle sur une entrée quelconque. Le corpus atteint K7 seulement. Les refus sur géométrie irrégulière, l'oracle constructeur n≤8 et le certificat structurel K10 conservent leurs attributions propres. CLI, manifeste terminal, archive, verticale, masses et coût massif restent distincts. Statut public `not_claimed`.

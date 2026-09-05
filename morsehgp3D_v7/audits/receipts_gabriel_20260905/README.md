# Certificat FULL, portails et projection réduite

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. La [décision courante](../NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md) est l’entrée maintenue. Ce dossier conserve les preuves et observations qui la fondent, avec leur portée propre ; il ne qualifie aucun nouveau C++ produit.

## Preuves et sources

La [preuve FULL](full_proof_review.md) traite minima, multifusions, plateaux, inertie hors fenêtre, K=n et ancres verticales fermées. Ses [pins et fixtures analytiques](full_proof_review.json) sont distincts de la [première preuve réduite](level_proof_review.md). La [revue du certificat FULL](full_certificate_review.md) prouve aussi la projection réduite sans bit de première non-trivialité et distingue les minima des facettes pondérées. Ses [sources](full_certificate_pins.json) incluent l’extrait des définitions 21–22 et de la figure 6.5.

Deux états de la note constructeur sont attribués séparément : la [capture initiale](constructor_note_at_review.md.txt), épinglée dans [source_review.json](source_review.json), et la [capture FULL publiée](constructor_full_note_at_review.md.txt), SHA256 `0b9cd8e17636fcaeb2211bc2c9446bc7ebc6a356e07c399c42529a6f84c9abfd`, attribuée dans [full_source_review.json](full_source_review.json). Les premières contrelectures ne sont pas rétroactivement présentées comme des revues FULL. La [portée exacte d’E5 et d’ACDE](counterfixture_scope.md) et la [première revue du certificat réduit](minimal_certificate_review.md) restent des pièces de preuve, sans seconde autorité courante concurrente.

La [contrelecture des documents constructeur en cours](concurrent_document_review.json) conserve les anciens hashes et les captures nouvelles de trois clarifications FULL. Les pins documentaires courants suivent les seuls octets relus et publiés ; une modification concurrente non publiée reste un écart de fraîcheur déclaré. Les reçus historiques E/F ne sont pas réécrits. Le contrôle par liste de hashes ne qualifie aucun nouveau fichier FULL absent de cette liste.

## Modèles exécutés

| Sonde | Résultats par mode Python |
| --- | --- |
| [Portails réduits](../gabriel_portal_probe.py) | 8 cas, 36 ordres ; 118 lots directs, 122 cofaces ; 1 752 coupes, 1 372 carrés horizontaux ; 30 naissances, 34 multifusions, 36 croissances |
| [Minima et portails FULL](../gabriel_full_probe.py) | 10 cas, 50 ordres K1..n ; 123 lots directs, 128 cofaces ; 2 265 coupes, 2 225 carrés horizontaux ; 178 minima, 107 multifusions, zéro delta ponctuel |
| [Projection des journaux scellés](../full_to_reduced_replay.py) | 100/100 événements réduits appariés sur 36 ordres ; 1 288 coupes appariées, 1 610 coupes de projection au total ; dix projections K=n vides |

Le corpus réduit comprend E5, ACDE, sept points de la courbe des moments et deux triangles séparés, avec PointId normaux puis inversés. FULL ajoute deux triangles obtus : minima de niveaux distincts ou égaux, puis une fusion à un intrus ; il inclut aussi l’ordre terminal de chaque cas. Les nombres d’événements des deux corpus ne constituent pas un ratio de compression.

Les constructeurs voient les points et les catalogues Gabriel de fixtures ; ils n’interrogent pas la partition Gamma. Le juge énumère séparément Gamma sur au plus sept points, isolés compris pour FULL. La primitive MEB rationnelle est commune, les constructions d’incidence et les rejeux sont distincts. Il s’agit d’une vérification de réduction topologique, sans nouvelle indépendance géométrique revendiquée. La projection ne lit que deux JSON épinglés : elle vérifie une correspondance unique des événements, niveaux, parents et deltas, avec des identifiants volontairement différents.

FULL contrôle 123 groupes couverts par leurs parents stricts, 100 frontières à zéro, dix feuilles terminales K=n et les lots simultanés. Les deux portails exécutés, E5 et son réindexage, ont chacun un seul pas strict. Leurs ancres terminales diffèrent selon l’ordre des PointId ; les normalisations historiques sont aussi exercées dans les cas de triangles séparés. Aucun corpus de longues descentes n’est rapporté.

| Mutant ou frontière | Rejet observé |
| --- | --- |
| Réduit : supprimer les portails | `judge.coverage` |
| Réduit : supprimer les croissances | `judge.minimal_journal_coverage` |
| FULL : omettre les minima | `full.judge_surjective` |
| FULL : inventer une naissance isolée à un intrus | `full.gabriel_birth_incident_same_lot` — garde du constructeur d’audit |
| Projection : perdre les points des parents-feuilles | `projection.cut_state` — E5 perd A puis A+B |
| Projection : conserver les minima comme racines réduites | `projection.cut_state` — racines présentes trop tôt |

Ces six mutants appartiennent aux modèles d’audit. L’extra-shell AB=(0,0,0),(2,0,0), C=(1,1,0) est une fixture Gamma valide mais hors domaine régulier : refus de la même garde de naissance et de `portal.unsupported_extra_shell`, conservés séparément dans le reçu FULL. Le lecteur des sondes n’est pas une validation générale de flux hostiles. La projection effectue ses contrôles structurels sur deux reçus scellés.

## Reproduction

Depuis la racine, ces commandes écrivent leurs résultats sous `audits/`. Chaque commande a terminé avec le code 0 ; les trois paires sont égales après retrait du seul champ `python_optimized`.

```bash
python3 -B morsehgp3D_v7/audits/gabriel_portal_probe.py --output morsehgp3D_v7/audits/receipts_gabriel_20260905/portal_normal.json
python3 -B -O morsehgp3D_v7/audits/gabriel_portal_probe.py --output morsehgp3D_v7/audits/receipts_gabriel_20260905/portal_optimized.json
python3 -B morsehgp3D_v7/audits/gabriel_full_probe.py --output morsehgp3D_v7/audits/receipts_gabriel_20260905/full_normal.json
python3 -B -O morsehgp3D_v7/audits/gabriel_full_probe.py --output morsehgp3D_v7/audits/receipts_gabriel_20260905/full_optimized.json
python3 -B morsehgp3D_v7/audits/full_to_reduced_replay.py --output morsehgp3D_v7/audits/receipts_gabriel_20260905/projection_normal.json
python3 -B -O morsehgp3D_v7/audits/full_to_reduced_replay.py --output morsehgp3D_v7/audits/receipts_gabriel_20260905/projection_optimized.json
```

Les [résultats réduits](portal_normal.json), [FULL](full_normal.json) et [de projection](projection_normal.json) portent leurs sources, comptes non vides et rejets exacts. Les fichiers `*_optimized.json` conservent le résultat correspondant sous `-O`. Les quatre ordres non terminaux des deux triangles obtus supplémentaires n’ont pas de journal réduit indépendant ; leurs projections sont seulement confrontées aux coupes FULL. La convention des racines implicites K1 du journal réduit est explicitée à zéro dans le rejeu, sans attribuer cette nouvelle frontière au code de l’ancien oracle réduit.

Le [calcul uniforme exact](uniform_weight_fixture.json) vérifie une distinction de contrat : le triangle obtus possède deux minima topologiques, mais ses trois facettes pondérées ont chacune une masse un. Omettre AB perd une masse un ; renormaliser sur les deux minima donne 3/2 chacun et change le profil. Cette vérification n’exécute ni politique temporelle de masse, ni condensation.

La preuve des cartes verticales FULL reste distincte d’un port exécuté. Catalogue produit, portails industriels, budgets, export, masses et coûts sont à qualifier dans leurs propres jalons. Aucun build, benchmark produit, CTest ou résultat GPU nouveau n’est attribué à ce dossier. GCP non utilisé.

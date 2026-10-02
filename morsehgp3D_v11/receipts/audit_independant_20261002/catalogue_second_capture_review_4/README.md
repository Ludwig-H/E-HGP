# Catalogue2 : qualification conforme, banc incomplet

Source exécutée `f391bf13e1a9a982025bde86fc9219b5b7430afc`. Publication lue à `e6fe34cb0` ; seuls trois outils Git de f391 et les cinq pièces locales catalogue2 sont copiés ici, avec hashes avant/après. Aucun fichier WIP du moteur ni reçu précédent n'est modifié. Cadre `exploration_v11_hors_registre`, `cpu_reference`, `quantized_u18_input_only`, audit de preuve catalogue, `not_claimed`. **GCP non utilisé par cet audit ; aucun build, CTest produit ou binaire natif lancé.**

La matrice est conforme : **948/948 portes sélectionnées et passées** — Release 218 ; ASan+UBSan, TSan, profils 21 et 24 : 143 chacun ; poison 144 ; style 2 ; mutants 12. Clang est facultatif et absent. Les huit mutants catalogue sont effectivement jugés et tués, tous par `code`, sans signal, délai ou refus de construction. Le problème du clone rouge de catalogue1 est donc fermé pour f391, sans effacer cette première capture. [Verdicts détaillés](observations.json), [journal mutant original](excerpts/results/cmd/000_matrice/files/matrix/mutants/LastTest.log).

Le banc conserve **sept tentatives : un succès et six timeouts de 30 s  ; 29 omissions**. Les unités tentées/omises couvrent exactement les 36 unités demandées. À 8k/K5, le succès dépasse 10 s : les répétitions 1/2 sont omises. À 8k/K10, le premier appel expire ; à 16k/32k et sur chacune des trois trames LiDAR entières, le premier K5 expire et K10 est omis. Aucune omission ne devient une réussite. La commande banc rend 1, donc la session globale reste `failed_remote`, alors que la matrice rend 0. `complete=true` signifie que ce calendrier avec omissions est clos ; `full_schedule_completed=false`.

| Seul succès | Valeur observée |
| --- | ---: |
| Entrée entière/profil/K | Uniforme 8 000 sites, u18, K5 |
| API catalogue | 15 478 187 473 ns = 15,478187473 s |
| Processus | 15,625014764 s |
| Boules / niveaux / incidences | 597 998 / 597 987 / 2 895 136 |
| Pic réservé Buffer, Cloud vivant compris | 133 416 208 octets |
| Réservé après construction | 64 427 856 octets |
| Fichier canonique déclaré | 109 592 066 octets |

La durée API couvre deux passes, tri et sorties en mémoire ; le processus comprend lecture et sérialisation. Segmentation du sol et préparation horsligne sont exclues. Le pic Buffer n'est pas le RSS. Les six délais sont des timeouts de processus, pas six chronos API complets. Un seul succès a été obtenu : **aucune paire de répétitions réussies**, aucune estimation de variance ni extrapolation. Les trois trames appartiennent à la même séquence 08. Les fichiers canoniques sont supprimés sur la VM ; SHA `2671f84acd61597300af06e7f164b0c8cd552b726bf7519711c8772d3febf74a` déclaré, pas recalculé ici. Le seul succès dépasse déjà 100 ms pour le catalogue CPU séquentiel ; aucun temps FULL/GPU n'est mesuré.

La provenance est recoupée : brut local égal au hash compact, paquet source identique à un `git archive` indépendant de f391, archive originale et **106 entrées de manifeste valides**, copie matrice/banc exacte, plan rendu conforme au worker. Le hash du binaire mesuré `7b850d52c95a9970bac52c540be293cd66492f6b6bd3e64b2457485ca7a0b054` correspond au manifeste de construction Release. Les textes cache/flags sont hachés et archivés ; les binaires eux-mêmes ne sont pas rejoués ici. [Faits et hashes](observations.json), [sources et captures initiales](sources_before.json), [stabilité finale](capture_after.json).

Fermeture ciblée confirmée dans le reçu brut : génération `2026-10-02T05:53:29.494-07:00` identique au démarrage/arrêt, état `TERMINATED`, deux garde-fous, clé temporaire supprimée, OS Login retiré, verrou libéré. Les deux groupes worker sont fermés ; la fermeture matrice précède le lancement banc. Cette relecture ne recertifie pas l'état GCP actuel et ne transforme pas la limite de quiescence interne de la matrice en correction acquise.

[Lecteur autonome](review.py), [normal/−O](review_runs.json) : passe sur les seules copies, conserve qualification conforme et échec du banc. La publication catalogue2 est encore locale/non versionnée à la capture ; ses cinq fichiers sont stables avant/après. Aucun FULL, points/EOM, HDBSCAN, GPU, LiDAR achevé ni contrat 100 ms n'est qualifié par ce banc.

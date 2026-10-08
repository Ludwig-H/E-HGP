# L1 récupérée et nouvelle prise L1R — provenance distincte

L’archive originale L1 est maintenant disponible localement : **23 176 octets, SHA `3497c745…573b`**, identique au hash distant annoncé avant le [refus initial de rapatriement](../session_l1_diagnostic/README.md). Ses **72 entrées de manifeste** sont réhachées ; seul le manifeste lui-même reste hors de sa liste. Elle conserve le worker **403736300900a709b82316721eeb589fbc4a5350**, son plan `cdc81d10…cf11` et son script `11bf866d…1d`. Commande `mes_b` code0, worker terminé, aucun flux tronqué ni débordement déclaré. L’ancien échec du contrôleur reste historique ; il ne décrit plus la disponibilité actuelle des bruts.

La session de récupération a joué **deux commandes** : `archive_l1`, puis **une nouvelle exécution `mes_b`**. Cette dernière n’est pas une réécriture du rapport original. L’enveloppe locale fait **52 145 octets**, SHA `34eebfbf…030d`, avec **81 entrées de manifeste** vérifiées.

| Preuve | L1 originale | Nouvelle L1R |
| --- | --- | --- |
| Source worker | `40373630` | `ea62cd69` |
| Rapport MES-B | `803aa13e…46d`, 33 368 octets | `d7357b6c…1735`, 39 447 octets |
| Pilote déclaré | `84777f19…65f7` | `457d0e6f…c49d` |
| Sonde déclarée | `5bbd7b02…1524` | `3c2c340e…21a9` |
| État déclaré | non tenu | non tenu |

Chaque série contient 18 cas, dix succès et huit refus ; 18 passes complètes dont huit chaudes. Cela ne constitue pas deux répétitions appariées à agréger aveuglément. La sonde L1R possède notamment le nouveau bloc mémoire du schéma902 ; les lecteurs doivent rester explicitement liés à chaque version. L’admission des mesures est traitée séparément par les contre-lecteurs L.

Les deux rapports déclarent Release/u21/CUDA ON. Les hashes de binaires sont des empreintes consignées par les pilotes ; aucun ELF n’est réexécuté ou qualifié par ce reçu. Les paquets/plans sont réhachés ; les trois sources pertinentes du paquet L1R sont identiques à leurs objets Git `ea62cd69` (pins complets dans [capture.json](capture.json)). `src/` ne change pas entre403 etEA ; cela n’autorise aucune attribution causale d’écarts de temps aux changements d’instrumentation ou de pilote.

La récupération est close : **DONE0, worker0, completed, results_verified=true, zéro erreur**, arrêt ciblé code0 et observation `RUNNING → TERMINATED`, réserve libérée. Ce sont les traces locales existantes, sans nouvelle interrogation cloud. `archives_vm.py` compare le hash de la **source après copie**, pas celui de la destination ; ici notre relecture indépendante du fichier local et des deux manifestes tranche la conservation de l’archive.

Les deux dossiers extraits sont séparés hors Git (`original_worker` et `new_worker`), avec inventaires hachés. Aucun payload XYZ, compte, projet, cible ni commande de récupération n’est copié dans ce reçu. Aucun nouveau calcul FULL ni action GCP par l’audit.

```sh
python check.py --original-session "$L1_SESSION" --recovery-session "$L1R_SESSION"
```

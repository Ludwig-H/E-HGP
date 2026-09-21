# Clôture des lecteurs — tranche32

[`READBACK.json`](READBACK.json) ferme20 commandes de lecture/autotest,
avec sorties brutes, codes de retour et empreintes avant/après identiques :
206 sources,75 fichiers d'entrée et78 artefacts. Aucune exécution native
n'est faite par ce helper. SHA256 du reçu :
`53fa28ce645dad32123b95c560f21ba98501017aa7275045f970199d1082d79e`.

Les trois nouvelles captures contiennent chacune trois portes et douze
mesures avec supports, profondeurs, clés et coquilles complets :

- `qualifications/smoke_2x8dvljr` : Release, census q3 par blocs.
- `qualifications/smoke_nc5osoja` : Release, census q3 scalaire.
- `qualifications/smoke_qbh9yxw5` : Clang ASan/UBSan, census par blocs,
  `detect_leaks=1:halt_on_error=1` maintenu lors de la reprise hors sandbox.

Lectures normal/−O avec `--check-live` identiques ; autotests réfutant
respectivement90/86/90 corruptions dans les deux modes. La comparaison
croisée des36 mesures confirme les mêmes sorties complètes entre modes de
témoins, census et builds. Les comptes géométriques sont aussi comparés
entre workers/builds pour un même algorithme, sans confondre les maxima de
capacité dépendant de leur affectation avec du travail logique.

Deux captures antérieures restent attachées à leur source historique :
`qualifications/regression_ucul7g41` (94 CTests PASS) et
`lidar/lidar_2x8pm0nw` (8k/16k/32k, K5, s8, W4, trois lignes PASS).
Elles sont relues normal/−O sans `--check-live`, sans les réexécuter ni
transférer leurs chronos vers une capture nouvelle. La seule différence
de source est le contrôle de la gate globale dans le nouveau lecteur.
Les deux scripts antérieurs sont archivés byte-à-byte dans
[`preflight/before_empty_calls_fix/`](preflight/before_empty_calls_fix/).

L'échec `qualifications/smoke_5vvacjai` venait de ce contrôle : il attendait
deux appels parallèles vides hérités de31, alors que32 en teste huit. Le
correctif valide désormais le contrat32 explicitement, sans modifier les
données observées. L'autre échec `qualifications/smoke_yclsnvf8` conserve
le refus LeakSanitizer sous ptrace. Les deux captures FAILED sont toujours
refusées par les lecteurs, dans les deux modes. Aucun produit n'a été
modifié pour ces reprises ; les anciennes preuves ne sont pas écrasées.

Cette clôture ne qualifie ni une borne sous-quadratique générale, ni FULL,
ni un contrat de tour sur G4. Les temps des petites qualifications ont une
charge concurrente et ne représentent pas un gain de vitesse stable.

# Reprise d'audit : délais et mesures G4

Audit du 29 septembre 2026, sources v10 au point de reprise `6206d1d11`.
Moteur inchangé, aucune commande GCP et aucun benchmark géométrique exécuté.

## Défaut prospectif reproduit

`bench/scaling/scale_run.py:33–44` lance le binaire sous `/usr/bin/time` avec
`subprocess.run(timeout=...)`. Le délai tue le processus direct, pas son descendant.
L'appel peut rendre le statut `-9` alors que le calcul continue ; les mesures
suivantes peuvent alors partager CPU et mémoire avec un calcul déclaré arrêté.

Le [reproducteur](../../../receipts/audit_continu_20260929/timeout/reproduce.py) importe réellement ce `run_json`, sans remplacement.
Il lance un Python qui ferme ses flux puis dort, dans un groupe de processus
possédé. Après le retour du délai de 1 seconde, l'enfant est encore dans l'état
`S`. Un sous-récepteur Linux permet ensuite de tuer et de récolter cet enfant
explicitement. Aucun processus tiers n'est signalé.

```bash
python3 -B morsehgp3D_v10/receipts/audit_continu_20260929/timeout/reproduce.py
python3 -B -O morsehgp3D_v10/receipts/audit_continu_20260929/timeout/reproduce.py
```

Les [captures normale](../../../receipts/audit_continu_20260929/timeout/receipt_normal.json) et [optimisée](../../../receipts/audit_continu_20260929/timeout/receipt_optimized.json)
confirment le défaut, le signal au groupe possédé, la récolte du descendant et son
absence dans `/proc` après nettoyage. Chaque exécution laisse son reçu dans un
nouveau répertoire `/tmp`. Les PID sont locaux à l'espace de noms de l'exécution.
Le second lancement optimise le processus orchestrateur ; les sous-processus
Python sont lancés avec `-B`, sans `-O`. Ce n'est donc pas une porte distincte
sur `run_json` importé avec `-O`.

Le [premier essai](../../../receipts/audit_continu_20260929/timeout/receipt_initial_inconclusive.json), à 0,2 seconde, est conservé :
il n'avait pas encore obtenu le fichier témoin au retour du délai, donc ne prouve
pas le défaut. Son groupe possédé a été arrêté. Il utilise une révision antérieure
du reproducteur, identifiée par son hash, et n'est pas la qualification finale.

**Portée historique distincte :** la session G4 5 contient 169 statuts `ok` et
7 `skipped_budget`, aucun `timeout`. Ce défaut ne démontre donc pas une
contamination de ses mesures closes. Il faut corriger l'arrêt du groupe et le
tester avant une campagne susceptible de dépasser le délai ; ne pas effacer les
mesures historiques au nom de ce risque futur.

## Autre partie de la revue

Voir [la revue des mesures et des affirmations d'échelle](AUDIT_ECHELLE.md).
Les reçus historiques restent immuables ; les corrections de portée sont ici.

Complément : [cibles statistiques et limites des diagnostics oracle](AUDIT_CIBLES_STATISTIQUES_20260929.md).

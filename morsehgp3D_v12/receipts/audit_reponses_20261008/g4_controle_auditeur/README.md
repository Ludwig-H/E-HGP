# Contrôle final G4 par l’auditeur

8 octobre 2026, **19:11:32 UTC**, sur demande explicite de l’utilisateur : arrêter soi-même
la VM si elle tourne encore à la fin. La cible et sa génération sont celles de T2dB3.

Le verrou commun v10/v11/v12 a été tenu ; le script gardé épinglé `stop_and_verify.sh`
a été exécuté avec cible, projet et génération attendue figés. Lectures GCP avant et après :
**TERMINATED → TERMINATED**. Le script rend zéro. La VM était déjà arrêtée :
**aucun nouvel ordre d’arrêt n’a été envoyé**. Inventaire du projet limité au label
`project=e-hgp` : **zéro instance active**.

Les sorties brutes sont conservées hors dépôt, sans publier cible ni identité de compte.
`capture.json` en ferme les empreintes et les seuls champs publics. Le lecteur ci-dessous
rejoue les métadonnées ; il ne contacte pas GCP. Ce contrôle distinct n’exécute aucun moteur
et ne qualifie aucune mesure de performance. Il décrit cet instant, pas un état futur permanent.
Le premier lancement local du contrôleur d’audit a échoué lors du décodage AST des épingles,
avant tout accès cloud ; le décodage a été corrigé puis vérifié avant l’exécution réussie.

```sh
python -B check.py SNAPSHOT_PRIVE RECEIPT_T2DB3
```

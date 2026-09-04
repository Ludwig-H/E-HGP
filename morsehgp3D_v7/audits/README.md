# Audits courants de MorseHGP3D v7

Entrée maintenue le 4 septembre 2026. Toutes les écritures de l'auditeur
restent dans ce dossier ; le constructeur réalise les changements produit.

Commencer par la [synthèse indépendante](AUDIT_INDEPENDANT_20260904.md),
puis consulter selon le besoin :

- [Mathématiques et hiérarchie](AUDIT_MATHEMATIQUE_20260904.md) : modèle du manuscrit, descente silencieuse, réduction et certificats restant à composer.
- [Interfaces et archives](AUDIT_INTERFACES_20260904.md) : entrée réelle, refus et rejeu des objets publiés.
- [Résidence et parallélisme](AUDIT_RESIDENCE_20260904.md) : allocations du tri, census et pistes d'échelle.
- [Validation courante](receipts_20260904/validation_current.json) : résultats exécutés et identité des sources contrôlées.

Les rapports sont réécrits sur l'état vérifié courant. Les points corrigés
cessent d'être des objections ouvertes ; leurs contre-fixtures demeurent
exécutables. Il n'y a pas d'addendum contradictoire ni de proposition de
correctif périmée à appliquer.

Les fichiers `*_current*` de `receipts_20260904/` portent les preuves actives.
Le sous-dossier `history/` conserve uniquement les preuves brutes nécessaires
à la traçabilité des essais, y compris leurs échecs. Les copies de sources,
binaires et temporaires de travail restent sous `.work*`, ignorés par Git.

Statut public : `not_claimed`. GCP non utilisé par l'auditeur.

Avant de réutiliser ces conclusions, contrôler les sources depuis la racine :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/verify_current.py
```

Un code 1 demande une actualisation des fichiers indiqués ; un code 2 signale
un manifeste invalide. Le code 0 confirme uniquement la fraîcheur des octets
épinglés, sans requalifier les tests ni promouvoir le produit.

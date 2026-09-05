# Supplément first-C — contrôles en lecture seule

5 septembre 2026. CPU de référence, entrée u16 ; public_status=not_claimed.

Ce paquet ne lance aucun moteur. Il contrôle des captures existantes avec le
supplément first-C épinglé et conserve leurs octets, les commandes et sorties.
Le juge FULLv2 reste obligatoire : ce supplément ne le remplace pas.

Pour chaque ordre lazy réussi : inserts=min(C,portals),
skips=max(0,portals-C). Les refus ne reçoivent que des bornes de préfixe.
Les modèles de selftest ne sont pas des captures du moteur.

[Statut et comptages](receipt.json), [sources et entrées](inputs.json),
[sommes des octets](SHA256SUMS). Aucun résultat 50k/1 seconde/100 ms,
aucune complétude géométrique ni qualification de tour inter-K intégrée.
GCP non utilisé.

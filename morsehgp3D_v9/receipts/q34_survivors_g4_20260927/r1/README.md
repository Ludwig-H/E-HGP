# G4 — tri résident des survivants, comparaison S2

État : **failed**. Sources : `5571957ca9231cf5771c053a283b11616d51b889`.
Autorité LIVE : snapshot, originaux privés, sorties VM et arrêt de la même génération.
Aucune tour FULL ni nouveau contrat de temps ne sont qualifiés ici.

Sorties partielles et erreurs conservées ; aucune mesure favorable ne transforme
une session en échec en campagne qualifiée.

Allocation observée : 155.926 s ; montant facturé non estimé.

Relecture LIVE normale puis avec `-O` :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_survivors_g4_readback_20260927/readback.py --readback CHEMIN_DU_RECU
```

Le snapshot et les originaux privés liés dans `PRIVATE_LINKS.json` restent nécessaires.
`SHA256SUMS` couvre toutes les pièces sauf lui-même. Aucune archive, clé, donnée KITTI
nouvelle, réponse OS Login brute ou réponse GCE non expurgée n’est publiée.

# Livraison des juges : G corrigé, admission CUDA ouverte

Complément **CST-0018**, 7 octobre 2026. Pins et résultats dans
[verification.json](verification.json). Aucun nouveau constat, source,
patch ou fixture dupliqué.

**G : résidu `exit.order` clos.** Le commit `af0c2ecd7` livre exactement
le hash `c88be31c…` de la [correction proposée](../g_juge_integration/README.md).
La nouvelle fausse sonde donne nominal→0, `False`→2, `0.0`→2 en normal et
`-O` : six appels Python. Les sources étaient stables pendant le rejeu et
sont identiques aux octets commis.

**CUDA : correction stricte encore absente de la livraison `8ba7d7287`.**
Les deux scripts livrés sont exactement les préimages du
[contre-audit publié](../cuda_juge_prepublication/README.md), et
`bench/g4_catalogue_schema.py` est absent du commit. Rejeu de son lecteur
sur les sources livrées : résultats normal/−O identiques. Étapes absentes,
processus dupliqués, comptes d'identité absents, temps négatifs, un seul
digest CPU pour dix passes et mutant d'identité reclassé en délai restent
indûment **adoptés**. La perte du compteur de réécriture dans la première
passe disparaît encore du résumé. Les contrôles nominal, 45 ms et
45 ms + 1 ns gardent leurs verdicts attendus.

L'admission CUDA reste ouverte et doit être corrigée **avant toute adoption
de temps**. La [proposition stricte publiée](../cuda_juge_proposition/README.md)
n'est pas intégrée dans ce commit. Ce constat ne rejette aucune campagne
GPU réelle : ici, seules les fonctions Python et des JSON synthétiques
ont été exécutés. Aucun moteur, build ou GCP.

Pour reproduire CUDA depuis un checkout épinglé à `af0c2ecd7` :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261007/cuda_juge_prepublication/check.py --source-dir morsehgp3D_v12/bench
python3 -B -S -O morsehgp3D_v12/receipts/audit_reponses_20261007/cuda_juge_prepublication/check.py --source-dir morsehgp3D_v12/bench
```

La commande G et les trois valeurs d'environnement utilisées sont consignées
dans le JSON ; les quinze témoins historiques de sa proposition restent
consultables dans le lecteur lié, sans duplication ni nouveau rejeu inutile.

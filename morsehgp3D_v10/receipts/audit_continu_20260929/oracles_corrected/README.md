# Capture indépendante des juges v10 corrigés

29 septembre 2026. `cpu_reference`, `quantized_u18_input_only`, hors registre,
`public_status=not_claimed`. Aucun GCP. Aucun moteur, juge de production ou
fichier d'un autre acteur modifié.

`evidence/normal.json` et `evidence/optimized.json` sont les sorties nouvelles
du script `scripts/probe_live_original.py`, exécuté en Python normal puis `-O`
dans `/tmp/v10_oracles_counteraudit_bIFSN95O`. Les deux commandes ont terminé
avec code 0 ; leurs JSON sont identiques. Le script importe les juges corrigés
de `build/v10-fixes/oracles/src` sans les changer, mute des copies de deux petits
dumps existants, puis exécute une seule fixture TRIANGLE à K3 par commande pour
conserver un dump neuf avec témoins et son mutant binarisé. Les empreintes
des deux juges et du binaire sont identiques avant/après dans les JSON.

Les juges copiés dans `sources/` ont respectivement les empreintes
`b814428d4c63133441b8e21f921f23fd28bcd7b4d35421ea6050e9e37a9689a3`
(catalogue) et
`ad615bce4ea97dd94f22b2359ffafbf97ea6d844c899104f2b432798f53b1c38`
(tour). Ces deux empreintes sont également celles des copies
`build/v10-fixes/oracles-verif/src`. `SPEC_V10.md`, la référence exacte et les
deux contrats C++ nécessaires sont copiés mécaniquement à côté.

Rejeu autonome, sans build ni binaire, des dumps archivés et des mutations :

```sh
python3 scripts/replay_snapshots.py
python3 -O scripts/replay_snapshots.py
```

Le code 0 signifie ici : fixtures intactes acceptées et contrôles positifs
rejetés. Les verdicts de mutation sont dans le JSON : `null` signifie que le
juge accepte le dump mutant. Ce code n'est pas une qualification du produit.
`evidence/replay_normal.json` et `evidence/replay_optimized.json` conservent ce
rejeu ; `evidence/catalogue_mutants/` conserve les dumps mutants correspondants.

`developer_observed/` contient seulement des copies de résultats déjà terminés
produits par les sessions développeur : leurs 24/24 portes et quatre appels
directs normal/−O. Nous n'avons pas relancé ces campagnes. Ces fichiers ne sont
pas des commandes exécutées par ce contre-audit. Les autres annonces de
`RECU_oracles.md` ne sont pas requalifiées ici.

Note courante :
`audits/audit_continu_20260929/catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md`.
`SHA256SUMS` ferme les fichiers de ce dossier hors lui-même.

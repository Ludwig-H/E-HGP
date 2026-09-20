# Compatibilité du chemin précédent, tranche27 → tranche28

Capture intermédiaire `differential_h79xiwx2` close de20:35:18.925 à20:35:19.812 UTC : **20 paires, 40 commandes PASS**, égalité de tous les champs hors `timings`. Les quatre lectures (Python normal/`-O`, historiques/vivantes) ont passé ; leurs commandes et sorties figurent dans `DEFAULT_DIFFERENTIAL_READBACK.json`. Les177 sources et artefacts sont identiques avant/après cette tentative.

Attention à sa portée : la gate locale venait déjà d'être renforcée après un mutant survivant (`72086faf…8b15` dans ce reçu, contre `d76e0d04…dd6f` au gel r1). Le runner ancien était encore présent (`39dab39d…f523`) et a changé après les quatre lectures. Cet instantané stable de compatibilité du chemin collectif n'est donc **ni le gel r1 coordonné, ni la qualification r2 finale**. La capture sera conservée intacte et une nouvelle tentative suivra le gel r2 ; ses lectures historiques restent pertinentes, ses lectures vivantes n'ont pas vocation à passer après ces changements.

`run_default_differential.py` adapte explicitement le helper homonyme de `receipts/q4_center_map_20260920/mutants/`, sans modifier ce dernier. Il compare `mhgp8_q34_collective_probe` entre les builds `v8_q4_center_map_20260920` et `v8_q4_local_20260920`. Cette sonde demeure celle du chemin collectif précédent : elle ne qualifie pas le nouveau balayage local q4.

Les20 paires sont inchangées : far/cap × K5/10 × n8k/16k/32k, puis adversarial × K5/10 × n32/64/128/256. Chaque appel utilise C64 et `variance_collective`. La totalité du JSON est comparée, sauf le seul objet `timings` ; les compteurs, masses, options, sorties et digests doivent donc tous rester égaux. Il ne s'agit ni d'une mesure de gain, ni d'une preuve générale de complétude ou de croissance sous-quadratique.

L'autorité historique exacte du vieux binaire est `receipts/q4_center_map_20260920/mutants/differential_lkomphpg/`, close à170 sources avec40 commandes et20 paires égales. Son manifeste épingle la sonde collective, l'archive et le cache du build27 ; contrairement à ce reçu, `smoke_errjw8oa` n'épingle pas la sonde collective. Le nouveau reçu archive le manifeste et la clôture de cette autorité et compare les trois hashes historiques avant toute exécution. Les177 sources, les six artefacts des deux builds, le helper et l'autorité sont rehashés à la fermeture.

Chaque tentative reçoit un répertoire `differential_` neuf et conserve commandes, environnement, code de sortie, stdout/stderr exacts, résultats parsés et erreurs. La lecture historique vérifie la fermeture et les sorties archivées sans exiger les builds encore présents ; `--check-live` vérifie en plus tous les fichiers vivants. Les contrôles utilisent `require`, jamais `assert`, et doivent passer en Python normal et `-O`.

```sh
python3 morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py run --build build/v8_q4_local_20260920
python3 morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py read <capture>
python3 -O morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py read <capture>
python3 morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py read <capture> --check-live
python3 -O morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py read <capture> --check-live
```

GCP non utilisé ; contrat de tour FULL/G4 non qualifié.

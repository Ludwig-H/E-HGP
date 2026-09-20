# Compatibilité du chemin précédent, tranche27 → tranche28 r2

La capture finale [differential_lnkgkcjz](differential_lnkgkcjz/MANIFEST.json), close le20 septembre2026 de20:42:06.792 à20:42:07.702 UTC, comporte **40 commandes et20 paires PASS**. Tous les champs JSON concordent, sauf l'objet `timings` explicitement exclu. Sa [fermeture](differential_lnkgkcjz/COMPLETION.json) vérifie que les177 sources, six artefacts (cache/archive/sonde des deux builds), helper et autorité sont identiques avant/après.

Le nouveau build est `build/v8_q4_local_r2_20260920` ; l'ancien est `build/v8_q4_center_map_20260920`. L'autorité de l'ancienne sonde collective est le [différentiel27 clos](../../q4_center_map_20260920/mutants/differential_lkomphpg/MANIFEST.json), dont manifeste et clôture sont archivés dans le nouveau reçu. Il s'agit de la compatibilité du chemin collectif précédent, pas des résultats de la sonde locale q4 ni d'une comparaison de performances.

Les20 cas C64/`variance_collective` restent : far/cap × K5/10 × n8k/16k/32k, puis adversarial × K5/10 × n32/64/128/256. La comparaison porte donc également sur les options, compteurs, masses, sorties et digests, pas seulement sur une signature finale.

L'unique adaptation du helper pour cette reprise, autorisée hors177 sources, accepte les deux chemins exacts `v8_q4_local_20260920` et `v8_q4_local_r2_20260920` aux contrôles `inventory/run`. Elle ne change ni protocole, ni paramètres, ni autorité et préserve les lectures historiques. Nouveau SHA256 du helper : `a6e3a080669c61cd5d687edf2362087b65dfb0f700fbdb99d48911d5f6e93d91`, épinglé avant/après. Le gel final porte la gate `72086faf513f49ad7d1850e9d77b3fd786a58b260ddcd558ba1c38f082eb8b15` et le runner `54d99d83ce22e6f675699d73bd4c2a2b9cb7626311ff6cf5962fab9e496f36bb`.

Les quatre lectures finales — Python normal/`-O`, historiques/vivantes — passent ; commandes, sorties et codes sont conservés dans [DEFAULT_DIFFERENTIAL_R2_READBACK.json](DEFAULT_DIFFERENTIAL_R2_READBACK.json). Ce fichier restaure les résultats d'outil initialement documentés dans le README partagé, avant son remplacement par la note des mutations ; aucune capture ni sortie n'a été réécrite ou rejouée pour cette restauration.

La lecture historique de la capture intermédiaire [differential_h79xiwx2](differential_h79xiwx2/MANIFEST.json) passe aussi avec le helper adapté. Cette capture n'est pas promue en qualificationr2 et reste intacte ; son contexte et son propre readback sont décrits dans [l'ancienne note](DEFAULT_DIFFERENTIAL.md). Les [mutations compilées](README.md) constituent une preuve distincte.

```sh
python3 morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py run --build build/v8_q4_local_r2_20260920
python3 morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py read morsehgp3D_v8/receipts/q4_local_20260920/mutants/differential_lnkgkcjz --check-live
python3 -O morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_default_differential.py read morsehgp3D_v8/receipts/q4_local_20260920/mutants/differential_lnkgkcjz --check-live
```

GCP non utilisé. Aucun contrat FULL/G4 ni borne générale sous-quadratique ne découle de ce différentiel ; `public_status=not_claimed`.

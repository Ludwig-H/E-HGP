# Épingles CPU des trames brutes (avec sol) de R21 (auditeur C)

24 septembre 2026. Calcul **en cours** : les valeurs seront ajoutées ici
et dans le canal dès la fin du calcul.

- Entrées : celles du WIP R21 `61cfba666`, à savoir les trames brutes de
  la séquence 08, versionnées dans
  `morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_0X_…_grid/full.u32le`.
  - `b00` = 08/000000 : 123 389 sites, SHA-256 `233cc4ea…`, FNV
    `4120701a6194c19b` ;
  - `b01` = 08/000100 : 124 479 sites, `de45e8dc…`, `d2bd37fb9befdd7d` ;
  - `b02` = 08/000200 : 125 526 sites, `37a7be39…`, `583db2f3deafe8e9`.
- Calcul : build Release CPU de `origin/main`, `mhgp9_tower_probe` à K5 et
  K10, `s=8`, 8 fils, `--catalogue-digest`, en deux bras :
  - moteur, avec les leviers par défaut ;
  - lots CPU : `q34_batch_filter`, `q34_batch_certificates`,
    `q34_batch_q3` et `q34_batch_q4` sont les jumeaux hôte du chemin GPU.

  Les condensés de tour et de catalogue doivent être égaux entre les
  deux bras.
- Usage : tout cas R21 brut, GPU ou moteur, devra reproduire la paire
  (condensé FULL, condensé du catalogue) de sa trame et de son K.

GCP non utilisé.

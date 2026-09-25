# Épingles CPU des trames brutes (avec sol) de R21 (auditeur C)

24 septembre 2026. Base `093d943ce`, sonde v26, build Release CPU, W8. Les douze exécutions sont `complete_relative` et Euler tient partout. Pour chaque trame et chaque K, les condensés de tour et de catalogue sont **identiques entre le bras moteur et le bras par lots CPU**.

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

## Épingles

| trame | sites | K | boules | condensé FULL | condensé du catalogue | voies reportées (lots CPU) | CPU·s moteur (W8) |
| --- | ---: | ---: | ---: | --- | --- | ---: | ---: |
| b00 (08/000000) | 123 389 | 5 | 2 822 052 | `cfb1634832c0384a` | `11f6a8e1a7f28127` | 16 | 288 |
| b00 (08/000000) | 123 389 | 10 | 11 387 391 | `dd90bda1e6569b79` | `1a315a5241510296` | 48 | 895 |
| b01 (08/000100) | 124 479 | 5 | 2 747 970 | `15015e5a5c29beac` | `7d385c14e5870263` | 0 | 225 |
| b01 (08/000100) | 124 479 | 10 | 10 663 312 | `2816dd6bcdb92ad6` | `dbbfc30b411e11ea` | 0 | 732 |
| b02 (08/000200) | 125 526 | 5 | 3 071 514 | `8096d4c6e269b254` | `f0206be1c838c4bd` | 0 | 268 |
| b02 (08/000200) | 125 526 | 10 | 11 684 593 | `f6e2e996224328f2` | `8c39ed9e85d7c0fd` | 8 | 864 |

Contrôles de cohérence :
- les condensés FULL de b00 (`cfb16348…` à K5, `dd90bda1…` à K10) sont ceux des mesures brutes locales antérieures, obtenues avec d'autres binaires ;
- le bras par lots CPU **reporte** déjà des voies au moteur sur ces trames brutes, avec les capacités par défaut : 16 et 48 pour b00 à K5 et K10, 8 pour b02 à K10. L'objet reste identique. C'est la première exécution du **chemin CPU complet à quatre leviers** (filtre, certificats, voies q3 et q4) sur des trames brutes avec sol. Précision de B : l'ancienne porte sur 08/000000 sans sol n'activait que le filtre CPU par lots, et R20 exécutait q3/q4 sur GPU.

Ordre de grandeur, sans valeur de chronométrage (hôte local W8) : 2,75 à 3,07 M boules à K5 et 10,7 à 11,7 M à K10, soit **2,1 à 2,5 fois** les trames sans sol (b01/K5 atteint 2,507). Le pic de RSS publié par la sonde (`peak_rss_kb`) va de 1,8 à 2,0 Gio à K5 et de 6,6 à 7,3 Gio à K10 ; les rapports GNU time, en Mio, donnent des valeurs voisines. Correction de B (21 h 43) : la première version donnait 2,1–2,4 fois et 6,8–7,5 Gio.

Réserves (B) : les JSON portent `grid=unspecified`, faute d'option `--grid` ; la grille 1 mm est établie par les SHA-256 des entrées v8. Seule l'empreinte du binaire (`probe.sha256`) est archivée, pas le binaire. `DONE` n'est qu'un marqueur de fin ; Euler n'est vérifié que jusqu'à K−2.

Fichiers : `PINS_RAW.json`, les sorties `probes/*.json`, les rapports GNU time `probes/*.time`, le script `pin_raw.sh`, `BASE.txt` et `probe.sha256`.

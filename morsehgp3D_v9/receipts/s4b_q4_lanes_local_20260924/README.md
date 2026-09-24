# Reçu local : voie q4 par lots sans atlas (S4b)

24 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP non utilisé**, aucun GPU : l'hôte exécute
la copie portable (`HostGroup`) du noyau.

- **Commit** : `931a0862` (`out/commit.txt`). Le script refuse des sources
  différentes de HEAD.
- **Machine** : 8 cœurs partagés.
- **Entrée** : trame sans sol 08/000000 de la v8, 39 885 sites. Les
  empreintes de l'entrée et des deux binaires sont dans
  `out/inputs.sha256`.
- **Commande** :
  `bash morsehgp3D_v9/receipts/s4b_q4_lanes_local_20260924/run.sh build/v9-exp morsehgp3D_v9/receipts/s4b_q4_lanes_local_20260924/out`

## Voies de l'hôte contre le moteur, arête par arête

Le mode `--compare` de `mhgp9_gpu_lanes_port_gate` est lancé avec
`--all-asked` et des planchers positifs. Chaque arête certifiée dont une
voie reste ouverte est comparée à `engine_q3_records` / `engine_q4_records`
: clé, support, arité, profondeur, taille et empreintes de coquille.

| K | survivantes | certificats reportés | sans voie | q3 demandées | q4 demandées | voies reportées | q3 émises | graines q4 | tétraèdres |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 2 043 612 | 0 | 1 334 926 | 701 678 | 576 456 | 0 | 691 284 | 7 126 317 | 158 496 |
| 10 | 4 507 278 | 0 | 3 043 916 | 1 437 421 | 1 357 994 | 0 | 2 898 219 | 31 928 291 | 1 732 548 |

Aucune exclusion (`all_asked_compared=1`), toutes égales (`equal=1`). Les
identités du registre q4 du lecteur sont vérifiées sur le travail exact.

Registre q4 :

| K | certifiées (1er paquet) | survivantes | paquets de lentilles | seaux vivants | groupes | `list_steps` | `group_steps` | `max_buffered` |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 6 253 220 (5 338 976) | 873 097 | 17 312 783 | 3 668 184 | 470 724 | 4 976 933 | 4 651 253 | 1 091 |
| 10 | 27 437 764 (20 885 337) | 4 490 527 | 106 636 691 | 20 406 103 | 5 324 855 | 34 256 291 | 52 449 078 | 1 183 |

## Chaîne : S4a, S4a + S4b, S4b jugé (W8, CPU)

Les trois bras reproduisent les épingles de tour et de catalogue de C. Ils
publient aussi le même condensé des présentations et les mêmes comptes q2,
q3 et q4 : `a2aa4b20ca392dfe` à K5, `43ff64fb1c3846d9` à K10. G4 R16
trouve les mêmes valeurs sur l'appareil. Toutes les voies demandées sont
décidées, et jugées dans le bras jugé. Les voies q4 émettent seulement
sous S4b.

Les temps de mur de `out/SUMMARY.json` sont **indicatifs** : l'hôte était
chargé par d'autres travaux. La copie hôte exécute les 32 voies d'un warp
en série. Ils ne mesurent donc pas le noyau ; le temps de l'appareil est
dans le [reçu R16](../g4_tower_r16_20260924/README.md).

## Fichiers

`out/` : `commit.txt`, `nproc.txt`, `inputs.sha256`, `compare_k{5,10}.txt`
et `.time`, `k{5,10}_{s4a,s4b,s4b_judged}.json`, `SUMMARY.json`,
`SHA256SUMS`.

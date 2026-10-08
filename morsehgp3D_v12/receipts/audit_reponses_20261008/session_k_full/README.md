# Session K — première contre-lecture FULL intégrée

8 octobre 2026. `exploration_v12_hors_registre`, `full_pi0`, u21, W48,
catalogue CUDA G4 ou CPU explicitement séparés, `public_status=not_claimed`.
**Les temps bruts sont admis ; le contrat de 100 ms n'est pas tenu.**

Archive reçue `5f8d64c1…` ; le [lecteur indépendant](../mes_full_contrelecture/README.md)
admet les 38 processus, 610 passes dont 392 chaudes, et recalcule exactement
les empreintes, statistiques et verdict du rapport. Aucun désaccord de
métadonnées, durée, cohorte, identité FUL1 ou état GPU avant/après. Rejeux
normal/−O identiques. Aucun moteur ou contrôleur lancé par cet audit.

La [provenance et clôture](../session_k_provenance/README.md) sont séparées :
FULL code externe 0 ; arrêt ciblé certifié, état TERMINATED à
**03:03:03.040 UTC**. Le statut global `failed_remote` vient de D6, code 1,
et n'annule pas le résultat FULL. Paquet `dev_snapshot` au HEAD `c9ac60f20`,
pas une qualification implicite de tous les changements du worktree.
Les compilations internes aux pilotes n'archivent ni empreinte binaire ni
cache CMake/identité du compilateur ; la chaîne source→binaire n'est donc
pas entièrement attestée. Les codes des 38 sondes ne sont pas archivés
séparément : leur zéro est inféré de `refus=[]` sous le pilote épinglé.

## Temps mesurés

Médiane des passes chaudes réunies par trame, en millisecondes. Les trois
trames ng00/01/02 ont 39 885 / 35 551 / 45 845 sites ; une seule séquence.

| FULL | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| GPU K5, 5 processus × 9 chaudes | **159,303** | **127,627** | **163,269** |
| maximum des médianes de processus | 160,760 | 127,804 | 164,636 |
| maximum brut chaud | 164,458 | 132,950 | 173,552 |
| CPU K5, 3 × 4 chaudes | **441,069** | **368,333** | **447,075** |
| GPU K10, 3 × 4 chaudes | **793,439** | **603,341** | **715,308** |

Le jalon descriptif d'une seconde est observé sur ces trois trames à K10,
pas qualifié sur plusieurs séquences à K10. Aucun bras CPU K10 dans K.

Les **37 trames de six séquences**, 33 179 à 99 099 sites, sont jouées dans
cinq processus résidents, deux tours chacun ; seul le second est chaud :
185 passes. Médiane entre les 37 médianes de trame **241,309 ms**, maximum
contractuel **467,920 ms**. Aucune des 37 trames ne passe le maximum
préannoncé de 100 ms. La plus lente est `kitti_ng_00_001896`, 95 586 sites,
médiane 465,778 ms. Le maximum est celui des médianes de processus ; pour
ce bras, un seul passage chaud par trame/processus le rend aussi maximum
brut. Ne pas changer ce juge après observation ni restreindre la cohorte.

## Où agir

Médianes des étages du même bras GPU K5, en ms. Elles ne sont pas additionnées
pour reconstruire le mur : chaque mur FULL est directement mesuré.

| Étage | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| P : Cloud/Morton/index | 1,729 | 1,456 | 1,875 |
| C : catalogue GPU, transferts/finition inclus | 34,941 | 30,798 | 37,417 |
| G : résolution | **57,200** | **43,833** | **51,737** |
| T : noyaux et leur préparation/historique | **33,078** | **25,847** | **37,951** |
| M : contraction | 11,697 | 8,434 | 12,007 |
| V : verticales | 2,208 | 1,949 | 2,359 |
| R : registre des branches | 14,257 | 11,966 | 15,322 |
| enveloppe T/M/V/R | 65,373 | 51,672 | 72,280 |

Le catalogue CPU coûte **326,092 / 274,071 / 329,246 ms** dans le bras FULL.
C'est la priorité de la régression CPU ; G et T sont les principaux postes
restants sur le chemin GPU. La [proposition R](../registre_classe_unique/README.md)
évite des recherches, sans résoudre seule l'écart au budget : calculé
**par passe avant la médiane**, `wall_ns−R_ns` vaut 145,132 / 115,758 /
147,967 ms. Ce diagnostic comptable n'est pas une mesure d'une variante
sans R ; aucun effet indirect de cache/allocation n'est supposé.

La [v11 gelée](../../../../morsehgp3D_v11/docs/AUDIT_GEANT_V11.md#53-où-en-est-on)
publiait environ 251 / 212 / 255 ms GPU K5 et 314 / 255 / 313 ms CPU K5 sur
ces trames. La tendance descriptive est donc un gain GPU de 36–40 % et
une régression CPU de 40–44 %. Sessions, modes de résidence et agrégation
différents : **pas un A/B causal**, ni une qualification du produit CLI v11.

## Frontière et preuve conservée

Le mur va des buffers quantifiés en mémoire à FULL, R et verticales inclus,
via P/C/G/raccord/TMVR. Lecture, segmentation, quantification, ouverture
du contexte/Pool, validation, FUL1, libération et initialisation de
`PassState` sont hors mur. Ce n'est pas le temps depuis le LiDAR brut.
Le pic publié est celui du budget commun hôte/GPU/épinglé, **pas RSS ou
VRAM séparés** ; GPU K5 : 1,159 / 0,965 / 1,192 Go décimaux. Aucun CPU·s
par trame publié. FUL1 n'encode pas les lignes/CSR de R ; les portes R
antérieures restent distinctes de cette égalité CPU/GPU.

`mesures.json` garde toutes les 46 lignes de statistiques sous colonnes
explicites, les deux contrats et les empreintes des 38 bruts. Le rapport
entier avec ses chemins privés n'est pas recopié. `replay.py` relit uniquement
les 39 membres JSON/JSONL FULL de l'archive, vérifie les sources et le plan
épinglés, la stabilité de l'archive et l'égalité complète à la capture.
L'archive extérieure reste nécessaire au rejeu ; aucune donnée XYZ/IDs
sous licence n'est conservée ici.

```sh
python3 -B CHEMIN_DU_RECU/replay.py --repo DEPOT --plan PLAN_K_JSON --archive RESULTS_TAR_GZ
python3 -B -O CHEMIN_DU_RECU/replay.py --repo DEPOT --plan PLAN_K_JSON --archive RESULTS_TAR_GZ
```

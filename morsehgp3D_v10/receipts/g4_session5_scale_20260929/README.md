# Reçu : passage à l'échelle jusqu'à un million de sites, et faits GPU de la VM (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. **Aucun calcul sur GPU** : la VM a seulement été interrogée.

## Session

- Code au commit `882b13286` (J2c, assemblage, pool, tête). Plan [`plan_s5_scale.json`](plan_s5_scale.json) :
  - la sonde `bench/g4/vm_facts.py` ;
  - `ctest -L fast` : 2 sur 2 ;
  - `bench/scaling/scale_run.py`, qui mesure catalogue et tour à K = 5 et 10, à 48 fils, par taille croissante,
    avec un délai de 300 s par appel et un budget total de 1 100 s.
- Entrées : `bench/scaling/scale_inputs.py --factors 1,2,4,8,16,32,64,128` ([`MANIFEST_entrees.json`](MANIFEST_entrees.json)).
  - 67 nuages synthétiques, de 8 000 à 1 024 000 sites, et les 21 secteurs LiDAR.
  - En régime spatial, les facteurs qui sortent du domaine 18 bits sont écartés : l'étendue grandit à pas fixe. Le
    manifeste les liste (`skipped`).
- 169 mesures abouties. 7 sautées par le budget : les plus grosses entrées ×128 restantes.
- VM `g4-standard-48` SPOT. **Arrêt certifié TERMINATED** sur la cible exacte : génération `11:40:08.147-07:00`,
  arrêt `12:06:15.802-07:00`, clé retirée. Le statut `failed_remote` ne vient que de pip.
- `session/receipt.json` et `session/preflight.json` : adresse du compte masquée.

## Faits GPU de la VM ([`vm_facts.txt`](vm_facts.txt))

- GPU : NVIDIA RTX PRO 6000 Blackwell Server Edition, 97 887 Mio, capacité de calcul 12.0, pilote 580.173.02,
  600 W.
- Boîte à outils CUDA 12.9 (`nvcc` V12.9.41) sous `/usr/local/cuda-12.9`, hors du PATH.
- CPU AMD EPYC 9B45 avec AVX-512 (dont `avx512_vp2intersect`) ; g++ 11.4.

## Résultats (exposants par doublement : [`exposants.txt`](exposants.txt))

- **Régime spatial** (densité locale constante) : exposants de 1,00 pour les boules, les tests jugés et les nœuds et
  pas de la tour, jusqu'à ×32. Le travail est linéaire.
- **Régime de densité**, même support :
  - `uniform` et `clusters` : 1,01 à 1,07 jusqu'à ×128 ;
  - `filaments` : l'exposant redescend de 1,33 à 1,06 ;
  - `shells` et `terrain` : il monte jusqu'à 1,3 à 1,4 à K = 10.

  **Boules par site à K = 10** : la hausse vient de la géométrie, pas de l'algorithme.

  | Famille | ×1 | ×8 | ×64 |
  | --- | ---: | ---: | ---: |
  | `uniform` | 390 | 433 | 457 |
  | `shells` | 76 | 127 | 272 |
  | `terrain` | 64 | 76 | 146 |

  Sur une surface, le nombre de boules par site est celui du 2D. Quand la densité résout l'épaisseur de la surface, il
  tend vers la valeur 3D (≈ 460). Le coût par boule reste constant.
- **LiDAR** : les exposants quart → moitié → trame s'étalent de 0,6 à 1,4 selon le contenu des secteurs ; les trames
  ont environ 120 boules par site à K = 10.

**Plus grosses entrées mesurées**, à 48 fils :

| Entrée | K | Sites | Boules | Catalogue (s) | Tour (s) | RSS (Go) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `clusters` densité ×128 | 5 | 1 024 000 | 87,6 M | 7,8 | 13,0 | 23 |
| `clusters` densité ×128 | 10 | 1 024 000 | 478,5 M | 41,3 | 83,0 | 135 |
| `uniform` densité ×64 | 10 | 512 000 | 233,8 M | 19,9 | 41,7 | 67 |
| `terrain` densité ×64 | 10 | 512 000 | 74,5 M | 7,7 | 8,6 | 22 |

## Lecture

- **Le temps est linéaire en nombre de boules.** Le nombre de boules par site est borné par la géométrie : ≈ 460 en
  3D, ≈ 65 à 120 sur une surface, à K = 10.
- **La mémoire est le mur**, à environ 280 octets par boule au pic à K = 10.
  - Un million de sites en 3D prend 135 Go, sur environ 188 Go de VM.
  - Pour du LiDAR (≈ 120 boules par site), environ 1,4 M sites tiennent.
  - Viser 10 M points demandera un catalogue qui ne réside pas tout entier : traitement par blocs spatiaux, ou
    émission directe vers la tour.
- **À grande échelle, la tour coûte deux fois le catalogue** (exposants de 1,1 à 1,3 en temps). C'est le premier
  poste à paralléliser au-delà du régime LiDAR.

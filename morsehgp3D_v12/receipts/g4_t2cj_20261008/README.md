# Session G4 v12.20261008.t2cj : leviers de l'étage G (T2-c), **adoptés**

8 octobre 2026. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à `10050a96e`), cible
`us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, construction par défaut au profil 21.
VM démarrée à 01:12:52 UTC, worker de 01:14:34 à 01:26:15 (code 0), **arrêt certifié `TERMINATED`** par le lanceur
(clôture `stopped`) et relu indépendamment à 01:28:26 UTC (`lastStopTimestamp` 01:27:51 UTC). Reçu sans identité de
compte : [`receipt.json`](receipt.json) ; tableaux, rapport et journaux bruts du pilote sous
`resultats/cmd/001_t2c_pilote/files/t2c/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (étage G de la tour, voie CPU du produit)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `c13812d21c403065…` |
| plan | `bcd75cedec07b32f…` |
| données (trames ng00–02 ; archive du bras « avant » = `main` + correctif 1, `45a69c21…`, identique à `4df326cc8` sur les sources construites) | manifeste `d16be285435bc4a4…` |

## Portes sur la VM

`ctest -LE long` : **675 sur 675** ; `ctest -L lidar` : **6 sur 6** (déterminisme de G sur ng00 à l'empreinte de
l'objet `e5a81154fb1b15f1`) ; campagne des mutants de la tour (`mhgp12_mutants_tower`, 18 mutants) : verte.

## Verdict de `REGLE_T2C` (écrite d'avance, [`pilote_t2c.py`](../../microbancs/mes_t2c_g/pilote_t2c.py))

Cinq bras en processus alternés (avant, avant_bis pour l'A/A, sans_gl7, après, gl5), 48 fils, ng00–02 à K5,
10 tours × 10 passes, chaque bras deux fois à chaque position ; temps de G = médiane des passes 2 à 10 ; rapports
appariés par tour, moyenne géométrique et IC 95 % par bootstrap (10 000 tirages). Auto-test du juge : cinq cas
conformes (adopté, rejeté, refusé).

| Trame | G avant | avant_bis | sans G-L7 | **après** | G-L5 | lot T2-c | G-L7 | index et corrections | G-L5 à la place de G-L7 | A/A | empreinte de l'objet |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- | --- | --- | --- | --- |
| ng00 | 79,14 | 79,51 | 55,53 | **53,06** | 65,03 | 0,671 (0,666–0,676) | 0,957 (0,954–0,961) | 0,701 (0,697–0,705) | 1,223 (1,220–1,227) | 1,005 (0,997–1,012) | `e5a81154fb1b15f1` |
| ng01 | 62,85 | 62,86 | 43,96 | **42,11** | 52,37 | 0,669 (0,665–0,672) | 0,957 (0,955–0,960) | 0,699 (0,695–0,702) | 1,246 (1,242–1,249) | 0,999 (0,989–1,008) | `10bb4c6d14096b2d` |
| ng02 | 76,66 | 76,11 | 51,03 | **48,20** | 66,34 | 0,631 (0,626–0,635) | 0,945 (0,941–0,949) | 0,667 (0,660–0,674) | 1,375 (1,372–1,378) | 0,996 (0,986–1,005) | `d66647a043f682da` |

**Adoptés** : le lot T2-c, `G-L7` (file de sondes préchargées) et l'index des naissances avec les corrections ;
**rejeté** : `G-L5` (jointure triée) à la place de `G-L7`, 22 à 38 % plus lent sur l'hôte. Empreinte de l'objet
identique dans tous les bras.

À K10 (informatif, 3 passes) sur ng00 : G passe d'environ 634 ms (session H) à 443 ms, les tables de 107 à 45 ms ;
la résolution (381 ms, dont l'ordre 10 seul 147 ms) reste le poste dominant.

## Lecture

L'étage G passe à **42 à 53 ms à K5** sur G4 (budget 25 à 30 ms) : encore 1,6 à 1,8 fois trop. Avec l'étage C sur
l'appareil (31 à 38 ms, [session I](../g4_t1bi_20261008/README.md)), catalogue et résolution font 73 à 91 ms à K5, avant
les étages T, M, V et R, qui ne sont pas encore mesurés sur G4. Le census gardé (module index) est désormais le premier
poste de la résolution (31 à 35 % à un fil) ; puis `LEM-T1` et la proposition.

## Ce que cette session n'établit pas

Ni la chaîne complète, ni le budget de 100 ms ; une seule séquence de trois trames. GCP utilisé pour cette seule
session, arrêt certifié.

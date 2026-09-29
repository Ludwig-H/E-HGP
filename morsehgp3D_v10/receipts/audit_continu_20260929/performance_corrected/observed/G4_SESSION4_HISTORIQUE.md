# Reçu : session G4 après l'assemblage, la course du pool, la tête et J2c, CPU seul (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. **Aucun calcul sur GPU.**

## Session

- Code au commit `777406b82` :
  - J2 (`5565f94fb`) ;
  - assemblage (`7eee86c53`) ;
  - course du pool corrigée (`8e3b76245`) ;
  - tête (`b662673b2`) ;
  - J2c (`777406b82`).

  Build Release par défaut.
- Même plan qu'en sessions 2 et 3 ([`plan_s4_j2c.json`](plan_s4_j2c.json)) : 21 commandes, toutes réussies. La porte
  `ctest -L fast` passe 2 sur 2, dont la porte de stress du pool (50 000 travaux enchaînés à 4 et 8 fils).
- Mêmes données : les trois trames LiDAR entières sans sol.
- VM `g4-standard-48` SPOT (AMD EPYC 9B45, 48 fils). **Arrêt certifié TERMINATED** sur la cible exacte : génération
  `09:57:53.518-07:00`, arrêt `10:01:45.821-07:00` ; clé OS Login retirée. Le statut `failed_remote` vient seulement
  de l'absence de pip sur la VM.
- `receipt.json` et `preflight.json` : adresse du compte masquée, sha256 des originaux dans `ORIGINAUX.sha256`.
- Comparaison produite par [`compare_sessions.py`](compare_sessions.py) :
  [`comparaison_s2_s3_s4.txt`](comparaison_s2_s3_s4.txt).

## Résultats (dernière passe chaude, 48 fils sauf mention)

| Mesure | Session 2 | Session 3 (J2) | Session 4 |
| --- | ---: | ---: | ---: |
| Catalogue, trame 02, K = 5, 1 fil (s) | 8,514 | 4,819 | 4,040 |
| Catalogue, trame 02, K = 5 (s) | 0,314 | 0,235 | 0,171 |
| Catalogue, trame 02, K = 10 (s) | 1,098 | 0,840 | 0,628 |
| Catalogue + tour, K = 5, trames 00 / 01 / 02 (s) | 0,373 / 0,305 / 0,374 | 0,283 / 0,231 / 0,285 | 0,252 / 0,204 / 0,254 |
| Catalogue + tour, K = 10, trames 00 / 01 / 02 (s) | 1,541 / 1,234 / 1,498 | 1,291 / 1,015 / 1,237 | 1,125 / 0,861 / 1,024 |

**Chaîne complète jusqu'aux étiquettes** (`mhgp10_cluster`, K = 5, couverture, EOM z = 3, mcs = 200) :

| Trame | Session 2 (s) | Session 3 (s) | Session 4 : catalogue / tour / tête = total (s) | Amas |
| --- | ---: | ---: | --- | ---: |
| 00 | 0,547 | 0,462 | 0,177 / 0,058 / 0,024 = 0,259 | 66 |
| 01 | 0,459 | 0,380 | 0,151 / 0,047 / 0,020 = 0,218 | 45 |
| 02 | 0,558 | 0,475 | 0,178 / 0,061 / 0,024 = 0,263 | 46 |

Les nombres d'amas sont les mêmes dans les trois sessions.

## Lecture

- En une journée, la chaîne complète à K = 5 est passée d'environ 0,55 s à 0,22–0,26 s sur G4, en CPU seul, à
  sorties identiques. La tête ne pèse plus que 0,02 s.
- La frontière initiale remonte avec l'arbre binaire de J2c, comme l'agent l'annonçait. À 48 fils, K = 5, elle
  passe de 0,017 à 0,027 s, pendant que l'étage des boîtes descend de 0,143 à 0,103 s.
- Pour viser 100 ms à K = 5, il reste un facteur 2,2 à 2,6. Les postes restants :
  - l'étage des boîtes (0,10 s) ;
  - la tour (0,05 à 0,06 s) ;
  - la frontière (0,027 s), à développer sur plusieurs niveaux par tour ;
  - l'ordre et l'assemblage.

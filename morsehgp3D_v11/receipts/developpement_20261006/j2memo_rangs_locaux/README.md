# Feuille GPU : J2 mémorisé et rangs locaux directs, gain mesuré sur G4

6 octobre 2026. Session gardée `v11.20261006.claudej2memo`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `34a8a561d`. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.

## Leviers, tirés du profil source de coop2 (noyau de comptage un fil, K10/24, ng00)

- **J2 mémorisé.** `center_line_meets` représentait environ 23 % du comptage, alors que le cache J2 réussit sur 58 %
  (K5) et 71 % (K10) des tests de ng00. L'issue d'une face déjà jugée est désormais relue dans la mémoire de la
  feuille, comme le fait déjà la feuille CPU (`center_line_cache.hpp`). Le prédicat ne dépend que du triplet et de la
  boîte : décisions et compteurs sont identiques. Il n'y a qu'une seule branche, pour ménager la reconvergence (reçu
  coop2).
- **Rangs locaux directs.** `local_rank`, une recherche linéaire par incidence émise, représentait environ 7 % du
  comptage. La feuille transmet désormais aux puits les rangs locaux qu'elle connaît déjà.

## Médianes à chaud (ms) ; référence coop3, même machine

| K, feuilles | Trame | `domain` CPU / GPU | Exécuteur GPU (coop3) | Rapport | Comptage / écriture (coop3) |
| --- | --- | --- | --- | ---: | --- |
| K5, 16 | ng00 | 209 / 232 | 49 (59) | 0,83 | 30 / 13 (35 / 17) |
| K5, 16 | ng01 | 171 / 196 | 45 (54) | 0,82 | 25 / 15 (29 / 20) |
| K5, 16 | ng02 | 203 / 224 | 43 (52) | 0,82 | 28 / 9 (34 / 13) |
| K10, 24 | ng00 | 829 / 659 | 262 (342) | 0,76 | 172 / 77 (217 / 112) |
| K10, 24 | ng01 | 657 / 550 | 229 (302) | 0,76 | 139 / 80 (175 / 116) |
| K10, 24 | ng02 | 776 / 629 | 244 (321) | 0,76 | 159 / 72 (203 / 105) |

**Critère écrit d'avance** (plan) : exécuteur ≤ 1,00 × coop3 à K5 et ≤ 0,90 × coop3 à K10, sur les trois trames, avec
dumps et registres identiques. **Atteint** : 0,82 à 0,83 à K5, et 0,76 à K10. Les bancs sont `conforme`.

**Conséquences.**
- À K10, avec des feuilles de 24, le `domain` GPU bat désormais le CPU de 16 à 21 %, et le mur de 6 à 7 %.
- À K5, le GPU reste derrière le CPU sur `domain` (+11 à +15 %) : le gain de l'exécuteur ne couvre pas les étages fixes
  (contexte, envoi, niveaux).

**Supports.** Les portes `cli_supports*` et `api_supports_route*` sont conformes à ce pin (14/14,
`supports_v2_ctest.txt`), mais avec la sélection par rôle. Cette sélection est remplacée au commit suivant par Kruskal
(défaut de cycle de plateau relevé par l'audit be8085ec1) ; ce résultat ne qualifie donc pas la sortie définitive.

## Pièces

| Fichier | Contenu |
| --- | --- |
| `plan.json` | Plan de session, avec le critère écrit d'avance |
| `launch.json` | Lancement |
| `receipt.json` | Contrôleur |
| `gpu_ab_report_k5.json`, `gpu_ab_report_k10.json` | Bancs |
| `supports_v2_ctest.txt` | Portes supports, en CTest |

`SHA256SUMS` couvre les autres fichiers.

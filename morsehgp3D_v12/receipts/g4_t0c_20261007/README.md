# Session G4 v12.20261007.t0c : MES-M5 (parcours des boîtes en largeur sur le GPU)

7 octobre 2026, 13 h 12 à 13 h 23 UTC. Session gardée (`gcp-migration/v12_session.py`, instantané du worktree à
`320db4a12`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, sans construction
par défaut. VM démarrée à 13:12:47 UTC, **arrêt certifié `TERMINATED` à 13:22:41 UTC**. Reçu sans identité de compte :
[`receipt.json`](receipt.json) ; résultats choisis sous `resultats/` ; empreintes : `SHA256SUMS`. Les journaux bruts du
banc (`logs/*.log`) restent dans le dossier de session local ; leurs résultats sont dans `report.json` et `runs/`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (microbanc)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Épingle | Valeur |
| --- | --- |
| paquet | `50ec1b12ae2db638…` |
| plan | `b6b08e3fbc6488cd…` |
| données (mêmes 14 fichiers que les sessions A et B) | manifeste `fb6b79d2cd45f691…` |
| résultats rapatriés | `8608c9d67f3055b0…` |

| Commande | État | Durée |
| --- | --- | ---: |
| `source_v11` | ok | 5 s |
| `m5` : vidages du parcours de la v11, identité sur l'hôte et l'appareil, fixtures, mutants, sanitizers, 1 + 5 tours | ok | 357 s |

## Verdict : adoptée

Règle écrite avant la mesure ([`microbancs/mes_m5_parcours/README.md`](../../microbancs/mes_m5_parcours/README.md) § 6) :
identité sur l'hôte, sur l'appareil et sur les fixtures, mutants tués, sanitizers propres, et borne haute de l'IC 95 %
au plus 1/4 du temps de la v11 (frontière et passe unique, 48 fils, passes chaudes) sur chaque cas à feuilles de 24.
**Toutes les preuves sont présentes** (identité appareil sur les douze cas, isolation du GPU, aucun refus, aucun rejet).

**Portée du verdict au regard du contre-audit Codex** (`bf70e8b99`, pin `e30000dec`, antérieur à cette session : le
juge de M5 pouvait adopter malgré une identité en échec, des cas ou fixtures manquants, une seule passe de la v11 ou une
médiane déclarée contraire aux durées brutes). Recalcul indépendant depuis `runs/` : les douze cas ont leurs cinq tours,
l'identité sur l'appareil est vraie partout, la v11 a dix passes par tour (la première écartée), et les médianes
recalculées depuis les durées brutes redonnent exactement les rapports publiés (écart nul). L'adoption tient ; le juge
reste à durcir avant tout nouvel usage.

Temps total sur le GPU (copie du nuage, tous les niveaux, rapatriement des feuilles ; médianes, millisecondes) :

| Cas | GPU | v11 (48 fils) | Rapport (moyenne géométrique) | Borne haute | Sans transferts |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 K5/24 | 4,72 | 47,81 | 0,099 | 0,099 | 0,091 |
| ng01 K5/24 | 4,26 | 40,46 | 0,105 | 0,106 | 0,098 |
| ng02 K5/24 | 4,79 | 48,20 | 0,099 | 0,100 | 0,092 |
| ng00 K10/24 | 10,86 | 148,42 | 0,073 | 0,073 | 0,063 |
| ng01 K10/24 | 9,29 | 123,27 | 0,076 | 0,076 | 0,065 |
| ng02 K10/24 | 10,45 | 143,21 | 0,073 | 0,073 | 0,063 |
| ng00 K5/16 (publié) | 7,69 | 100,66 | 0,077 | 0,077 | 0,068 |
| uniformes 8 000 / 16 000 / 32 000, K5/24 (publiés) | 1,76 / 2,28 / 3,25 | 9,46 / 17,15 / 33,01 | 0,185 / 0,133 / 0,099 | — | — |

Prédiction écrite avant G4 : 0,06 à 0,10 à K5 (tenue), 0,03 à 0,06 à K10 (mesuré 0,073 : moins bon que prévu, très
en dessous du seuil). Avec la feuille J3 adoptée en session A (11,6 ms sur ng00 K5/24, noyau seul), le parcours et la
feuille valent ensemble environ 16 ms à K5, pour un budget de 35 à 45 ms de l'étage C qui comprend aussi la fin d'étage.

## Ce que cette session n'établit pas

Ni la fin d'étage, ni le flux feuilles-parcours intégré, ni le chemin produit : c'est un microbanc. GCP utilisé pour cette
seule session, arrêt certifié.

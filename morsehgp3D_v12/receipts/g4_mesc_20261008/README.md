# Session G4 C : la v12 sur les 159 petits nuages (`MES-C`, régime (c))

8 octobre 2026. Session gardée `v12.20261008.mesc` (`gcp-migration/v12_session.py`, commit `83ed7620d`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 06:03:13
à 06:41:53 UTC, **arrêt certifié `TERMINATED`**. Reçu sans identité de compte : [`receipt.json`](receipt.json) ;
sorties sous `resultats/` (lignes brutes de chaque Session comprises) ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (voie CPU complète) ; cuda_g4 (catalogue, voie appareil)
objet=full_pi0 (tour FULL K1..K, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

Paquet `g4_small` en une archive. Pilote [`pilote_c.py`](../../microbancs/mes_c_petits/pilote_c.py) au commit joué. Une
Session résidente par configuration enchaîne les 147 nuages réels et synthétiques sains en trois tours ; les nuages
difficiles sont joués seuls. Valeur chaude d'un nuage : médiane de ses passes des tours 2 et 3. Durée : 2 050 s, au
délai global. GPU vide avant et après.

## Verdict : refusé ; C1, C2 et C3 non tenus

| Critère | Règle écrite d'avance | Mesure | État |
| --- | --- | --- | --- |
| C1 | voie CPU, K5, 48 fils, nuages réels : ordonnée à l'origine des moindres carrés ≤ 2 ms | 20,34 ms | non tenu |
| C2 | même configuration : pente ≤ 3 727,2 ns par site | 11,75 µs par site | non tenu |
| C3 | cohorte complète des nuages difficiles, K5, CPU et appareil, 48 fils : aucun refus, échec ni expiration | quasi-sphère refusée `wide_leaf` à 3 000 et 10 000 sites, sur les deux voies | non tenu |

Le verdict est « refusé », pas seulement « non tenu » : la Session informative `appareil:10:1` (K10, un fil) a expiré au
délai global, et toute Session en échec fait manquer un contrôle. Les deux Sessions suivantes, `appareil:10:4` et
`appareil:10:48`, n'ont pas été jouées. Le pilote joué datait d'avant le
[correctif de cohorte de l'auditeur](../audit_reponses_20261008/mes_c_livraison/README.md) (`24dec7b85`). Rejugés par la
règle corrigée, sur les mêmes lignes, les critères restent les mêmes (C1, C2, C3 non tenus, la cohorte C3 étant
complète : 12 nuages × 2 voies à K5 et 48 fils) et le verdict reste « refusé ».

## Droites (valeurs chaudes, nuages réels, 132 nuages)

| Configuration | a (ms) | b (µs par site) | médiane de t/n | t médian, ≤ 150 sites | t médian, ≥ 5 000 sites |
| --- | ---: | ---: | ---: | ---: | ---: |
| CPU, K5, 1 fil | −26,6 | 195,7 | — | — | — |
| CPU, K5, 4 fils | −0,4 | 54,9 | 54,0 µs | 6,8 ms | 458,5 ms |
| CPU, K5, 48 fils | 20,3 | 11,7 | 40,0 µs | 15,6 ms | 121,3 ms |
| appareil, K5, 4 fils | 3,5 | 10,5 | — | — | — |
| appareil, K5, 48 fils | 9,7 | 3,6 | 16,1 µs | 8,5 ms | 41,5 ms |
| CPU, K10, 48 fils | 28,4 | 40,8 | 84,2 µs | 26,9 ms | 355,5 ms |

Ces droites sont descriptives : l'ordonnée à l'origine est extrapolée à zéro site et la pente mêle des scènes de
tailles et de géométries différentes ([portée des droites](../audit_reponses_20261008/mes_c_statistique/README.md)).
Tableaux complets, familles synthétiques comprises :
[`tableaux_c.md`](resultats/cmd/000_mes_c/files/c/tableaux_c.md).

## Familles difficiles (K5, seules, 48 fils)

- **Réseau entier** (cosphéricité massive) : calculé de 100 à 10 000 sites sur les deux voies (10 000 sites : 312 ms
  sur CPU, 259 ms sur l'appareil). La v11 gelée y passait 8,4 s à 10 000 sites
  ([session G](../g4_t0g_20261007/README.md)).
- **Droite** (100 et 1 000 sites) : calculée.
- **Quasi-sphère** : calculée jusqu'à 1 000 sites, refusée `wide_leaf` à 3 000 et 10 000 sites sur les deux voies
  (voie large T1-c, `CST-0237`, qui sert aussi ETH3D courtyard, [session L2](../g4_mesb2_20261008/README.md)).

Empreintes FUL1 identiques sur les passes de chaque nuage et entre voies et nombres de fils à K égal (aucun contrôle
d'empreinte manquant).

## Lecture

1. **Le coût fixe vient du parallélisme, pas du calcul.** Sur la voie CPU, 48 fils coûtent plus que 4 sur les petits
   nuages (15,6 ms contre 6,8 ms vers 150 sites) : la coordination des fils domine. La Session doit borner le nombre
   de fils engagés par la taille du travail. C'est le premier levier du régime (c).
2. **La voie appareil est déjà plus rapide que la voie CPU** à 48 fils, du plus petit au plus grand nuage (8,5 contre
   15,6 ms vers 150 sites, 41,5 contre 121,3 ms au-delà de 5 000). Le seuil « voie CPU sous une taille » n'est pas
   justifié par cette mesure tant que le coût fixe de la voie CPU n'est pas réduit.
3. Le réseau entier, mur de la v11, ne l'est plus ; la quasi-sphère reste refusée tant que la voie large manque.

## Ce que cette session n'établit pas

Ni K10 sur la voie appareil (expiré ou non joué), ni le budget de 2 ms. GCP utilisé pour cette seule session, arrêt
certifié.

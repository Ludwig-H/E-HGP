# Microbanc MES-E : la v11 gelée sur des scènes LiDAR réelles de 1 à 8 millions de sites

7 octobre 2026. Mesure de la tranche T0 ([`PLAN.md`](../../docs/PLAN.md), table des mesures), **hors produit**, sans
règle d'adoption : elle est **publiée** et fixe les objectifs du régime (b) (décision D7) et la taille des lots de la
v12 ([`ARCHITECTURE.md`](../../docs/ARCHITECTURE.md) § 4.6).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (v11 gelée ac081a06f, sonde mhgp11_full_bench, voie CPU 802811)
quantification=quantized_u21_input_only
public_status=not_claimed
```

## Ce que fait le pilote

[`pilote_e.py`](pilote_e.py) (bibliothèque standard, Python 3.10 nu) construit la cible `mhgp11_full_bench` dans une
construction de la v11 gelée déjà configurée (`microbancs/outils/source_v11.py`), puis joue chaque cas `NOM:K` dans
un processus neuf : entrée `NOM.u32le` et `NOM.ids.u32le` (sites distincts des découpes de `bench/data`, règle de
découpe corrigée `CST-0218`), feuilles de 16 à $K\leq 5$ et de 24 au-delà, vidage FULL vers `/dev/null`, budget
mémoire de la sonde donné en Gio. Il publie par cas : code de sortie (un refus de la sonde ou une expiration est un
résultat, le point de rupture), secondes, pic de mémoire résidente (`wait4`), boules et incidences du catalogue par
site, octets résidents et microsecondes par site ; les lignes brutes de la sonde restent dans le dossier de sortie.

```bash
python3 pilote_e.py --v11-build <construction de la v11> --donnees <dossier des découpes> --sortie <dossier> \
    --cas ign_lyon_0842_6521_sans_sol_c1M:5,ign_lyon_0842_6521_sans_sol_c2M:5 --fils 48 --budget-gio 160
```

## Premier passage local (7 octobre, indicatif)

Codespace partagé, 3 fils, budget 16 Gio : Boreas `f4500_n10_sans_sol_c1M` (1 000 005 sites) à K5, code 0, 268 s,
**pic de 15,1 Gio** (16 Ko par site), 44,3 boules et 208 incidences par site. La résidence de la v11 est donc le
premier mur attendu aux découpes de 8 millions (environ 128 Go extrapolés, pour 177 Gio sur la VM G4), et K10 le
point de rupture à mesurer. Les temps de la VM G4 font foi.

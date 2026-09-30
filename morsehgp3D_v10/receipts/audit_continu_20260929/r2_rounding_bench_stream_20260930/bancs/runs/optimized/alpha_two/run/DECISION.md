# Décision du banc v10 (AUDIT_BANCS_R2_ALPHA_AND_SCHEMA)

Calculée par `decide.py` ; aucun chiffre écrit à la main.

Banc v10 préenregistré AUDIT_BANCS_R2_ALPHA_AND_SCHEMA, 4 scènes de test, comparaison appariée K = min_samples. La tour bat HDBSCAN pour tour_contre_hdb, sans perte significative ailleurs.
- tour_contre_hdb (tour contre hdb) : la tour bat HDBSCAN ; Δ = +0,700 [+0,700 ; +0,700], p_Holm = 0,33, 4 victoires / 0 défaites / 0 égalités ; non-infériorité de la tour à la marge 0,02 établie.
Audit-only fabricated score fixture; no clustering or ground-truth benchmark was run.

## ARI_s moyen pondéré par cellule

| Méthode | ARI_s | Refus |
| --- | ---: | ---: |
| tour | 0,8000 | 0 |
| hdb | 0,1000 | 0 |

## tour_contre_hdb : tour contre hdb

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,700 | [+0,700 ; +0,700] | 0,33 |
| n = 8000 | +0,700 | [+0,700 ; +0,700] | 0,11 |
| spherical | +0,700 | [+0,700 ; +0,700] | 0,33 (Holm) |

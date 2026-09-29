# Décision du banc v10 (AUDIT_DEV_METADATA_ONLY)

Calculée par `decide.py` ; aucun chiffre écrit à la main.

Banc v10 préenregistré AUDIT_DEV_METADATA_ONLY, 32 scènes de test, comparaison appariée K = min_samples. Aucune revendication de supériorité : pas de différence significative au sens préenregistré.
- tour_hdb (tour contre hdb) : pas de différence significative ; Δ = +0,694 [+0,647 ; +0,723], p_Holm = 0,11, 32 victoires / 0 défaites / 0 égalités ; non-infériorité de la tour à la marge 0,02 établie.
Fixture d'audit, aucun score de nuage.

## ARI_s moyen pondéré par cellule

| Méthode | ARI_s | Refus |
| --- | ---: | ---: |
| tour | 1,2500 | 0 |
| hdb | 0,5561 | 0 |
| temoin | 0,3959 | 1 |

## tour_hdb : tour contre hdb

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,694 | [+0,647 ; +0,723] | 0,11 |
| n = 8000 | +0,570 | [+0,440 ; +0,574] | 0,11 |
| n = 16000 | +0,818 | [+0,744 ; +0,916] | 0,11 |
| shells | +0,600 | [+0,539 ; +0,661] | 0,22 (Holm) |
| spherical | +0,788 | [+0,655 ; +0,847] | 0,22 (Holm) |

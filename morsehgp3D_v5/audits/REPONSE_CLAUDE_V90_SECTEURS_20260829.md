# Réponse Claude — V90 : un handle n'atteint que 1,2 à 1,7 secteurs sur 8 (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Sonde `bench/sector_reach_probe.cpp` (jointe), $n = 8000$, 3 000 blocs à pas
constant, quatre familles.

## 1. La mesure

`anchor_sector_kill` recouvre le disque des centres par $K = 8$ secteurs et
exige $h_3$ témoins dans **chacun**. Si $\mathrm{Box}(C)$ n'en atteint qu'un
sous-ensemble, exiger le seuil sur ceux-là seulement est strictement plus
faible, donc tue strictement plus. Combien en atteint-il ?

| famille | secteurs atteints (moyenne) | **1 seul** | $\le 2$ | ouverture angulaire moyenne |
|---|---|---|---|---|
| `scanline` | **1,17** | 83,9 % | **99,4 %** | 10,1° |
| `terrain` | **1,31** | 71,8 % | **97,6 %** | 14,4° |
| `uniform` | **1,71** | 45,6 % | 85,9 % | 34,7° |
| `eight_clusters` | **1,35** | 67,9 % | **97,8 %** | 13,4° |

Histogrammes complets (1 à 8 secteurs) : aucun groupe n'atteint plus de
**6** secteurs, et au-delà de 4 les effectifs sont nuls ou négligeables
(scanline `16165 2989 100 10 0 0 0 0`).

**Au lieu d'exiger le seuil sur 8 secteurs, il faudrait l'exiger sur 1 ou 2.**
C'est un affaiblissement considérable de la condition de mort.

## 2. Ce que la mesure est, et ce qu'elle n'est pas

- Elle compte les secteurs atteints par les **centres exacts**, à **ancre
  fixe**. C'est le **plancher** du bénéfice.
- Un calcul par **boîtes** serait conservateur et en atteindrait davantage. Le
  gain réel sera donc **inférieur** à ce plancher — mais passer de 8 requis à
  environ 2 laisse une marge que même un calcul lâche devrait exploiter.
- L'ouverture au pire vaut 180° sur trois familles. C'est attendu et sans
  gravité : quand $q_x \to 0^{+}$ le centre tend vers $m$ et sa **direction**
  devient instable. Ces groupes atteignent 2 à 4 secteurs, jamais plus de 6.
- `uniform` est le cas le moins favorable (1,71), et c'est **cohérent** : c'est
  aussi la famille au plus grand gain marginal (45,3 %) et au plus grand
  diamètre de centres (0,183). Les trois mesures racontent la même histoire.

## 3. L'état de la chaîne, après cette mesure

| étape | résultat |
|---|---|
| plafond de la fibre | **92 à 98 %** du travail résiduel |
| patches nécessaires | **0,1 à 3,7 %** — la machinerie lourde est superflue |
| certificat par intervalles sur $\Pi$ | **0 %** — inerte, couplage sur $a$ perdu |
| structure des centres | **secteur**, pas bande ; rayon positif ; $v \parallel p_x$ |
| secteurs à exiger | **1 à 2 sur 8**, au lieu de 8 |
| arithmétique nécessaire | **aucune nouvelle** — `sector_kill.hpp` expose déjà `sector_counts` |

Tout ce qui manquait au 29 août au matin est maintenant chiffré, et la seule
pièce à écrire est le calcul, par boîtes, des secteurs atteignables par
$\mathrm{Box}(C)$ — des produits vectoriels entiers du même type que ceux déjà
présents dans `sector_kill.hpp`.

## 4. Questions

- **V91.** Recevez-vous ce plancher, et confirmez-vous que le sens du test est
  bien conservateur — un secteur atteignable **par boîtes** mais sans centre
  réel rend le test plus exigeant, donc jamais faux ?
- **V92.** Le calcul des secteurs atteignables demande de situer les directions
  $\hat p_x$, pour $x \in \mathrm{Box}(C)$, par rapport aux sommets $p_k$ de
  l'octogone. Voyez-vous une difficulté d'exactitude entière que je sous-estime
  — en particulier près de $q_x = 0$, où la direction est instable et où il
  faudrait sans doute déclarer **tous** les secteurs atteignables plutôt que de
  trancher ?
- **V93.** Si vous recevez tout ceci, l'incrément à écrire est-il bien : (1)
  calculer les secteurs atteignables par bloc, (2) restreindre le seuil de
  `anchor_sector_kill` à ceux-là, (3) porte à digest identique et compteur
  d'appels de puissance évités — ou voyez-vous une étape intermédiaire que
  j'omets ?

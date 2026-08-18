# Addendum — garde de capacité transactionnelle du fold (index locaux u32/i32)

Date : 18 août 2026. Exécute le § 7 et le n° 3 de l'« ordre conseillé »
du contre-audit `5d274a1` : les index locaux étroits du fold compact
(`FRec::e` u32, `fid` compatible i32, époques u32 à sentinelle
`UINT32_MAX`) sont le bon choix pour les futures tuiles, mais toute
entrée qui ne tient pas doit être REFUSÉE avant les casts et les
allocations — `resource_exhausted/requires_tiling`, jamais une
troncature.

## La garde (entrée de `build_forest`, avant toute allocation)

Majorants vérifiables à l'entrée : le nombre de facettes internées
vérifie toujours `nfid <= Σ(q_e + d_e)` (l'internement dédoublonne,
jamais n'ajoute) et le nombre de lots vérifie `lots <= evenements` ; le
numéro de lot doit rester STRICTEMENT sous la sentinelle `UINT32_MAX`
des tableaux à époque. Trois refus :

- `evenements > UINT32_MAX` (`FRec::e` et `ev_fid` sont indexés u32) ;
- `Σ(q_e + d_e) > INT32_MAX` (majorant de `nfid`, union-find i32) ;
- `evenements >= UINT32_MAX` pour les lots (collision de sentinelle).

Le refus remplit `ForestResult::refusal` et rend un résultat vide ; le
pipeline du probe le propage en code 2 (refus avant calcul, format des
campagnes transactionnelles). La borne Poisson q2 (~180 n facettes
nées) montre que ces limites sont atteignables bien avant `n = 2^32` —
la garde est la première moitié de la politique « offsets globaux u64,
index locaux u32 » ; les tuiles retireront le refus.

## Porte `--fold-capacity-gate` (124 CTest verts)

Bases FICTIVES `cap_base_*` près des limites — jamais d'allocation
géante (recommandation de l'audit). Pipeline réel eight_clusters
n=120, K=10 (2 141 événements, 23 551 records) :

- quatre cas AU-DESSUS refusés 4/4 : événements à `2^32`, majorant de
  fid à `INT32_MAX + 1` (bord exact) ET à `2^32 + r` (enroulement),
  lots à la sentinelle `UINT32_MAX` ;
- trois cas JUSTE SOUS la limite acceptés avec résultat IDENTIQUE à la
  base (facettes, fusions, deltas, partition) — la garde est une pure
  vérification ;
- trois MUTANTS causaux tués (code 4), chacun dégradant UNE
  comparaison : `fold-u32-event-wrap` (le compte d'événements
  s'enroule en u32 : n'atteint jamais le refus),
  `fold-i32-fid-wrap` (le majorant de fid s'enroule : `2^32 + r`
  accepté à tort), `fold-epoch-sentinel-collision` (`<=` au lieu de
  `<` : le lot sentinelle passe).

Piège d'implémentation gravé : la première écriture du mutant
d'enroulement (`(u32)nev <= UINT32_MAX` en ligne) est une comparaison
toujours-vraie que `-Werror=type-limits` REFUSE à la compilation — le
mutant route l'enroulement par une valeur effective, la vérification
elle-même reste identique dans les deux branches.

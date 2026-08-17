# Addendum — l'aval en parallèle : préfiltre ×3,3, census ×2,4, fold au plancher de son K dominant

Date : 17 août 2026. Base : `5281a20` (cœur aplati) ; code livré dans
le commit portant ce reçu.

## Ce qui est implémenté

Les trois étages d'aval du probe acceptent `threads`, par TRANCHES
CONTIGUËS d'indices dont la fusion en ordre de tranche restitue la
sortie BIT-IDENTIQUE au séquentiel :

- `prefilter_balls` : indépendant par candidat (la passe count-only par
  clé) — survivantes concaténées en ordre d'indices.
- `census_balls` : indépendant par survivante ; un refus/invariant
  arrête sa tranche et le verdict fusionné est celui de la tranche
  d'indices la plus basse (déterministe — la même boule qu'aurait vue
  le séquentiel), message imprimé une seule fois.
- `forests_from_balls` : phase A, expansion des plateaux par tranches
  de boules (listes par K privées, fusion en ordre de tranche = ordre
  séquentiel exact des événements) ; phase B, folds INDÉPENDANTS par K.

## Portes (106 CTest verts)

`--par-gate` étendue à l'aval : survivantes identiques (idx,
profondeur), événements BIT-EXACTS par K (q, d, masque actif, support,
intérieurs, représentation de niveau), résumés de forêt identiques
(fusions, nœuds, lots), 1 fil contre 4, deux familles. Nouveau mutant
`par-drop-ball-chunk` (une tranche de census oubliée à la fusion,
appliquée au seul côté 4 fils) : tué à code 4, comme `par-drop-shard`.

## Mesure (uniform n=8000, smax=11, seed=3, `--threads=4`)

| étage | avant (séq.) | après (4 fils) | speedup |
|---|---|---|---|
| t_prefiltre | 22,8 s | **7,0 s** | ×3,3 |
| t_census | 27,5 s | **11,3 s** | ×2,4 |
| t_fold | 55–69 s | 56,1 s | ×1,0 |

Sorties IDENTIQUES au run gravé de la campagne : 3 134 427 boules,
3 126 158 événements, 19 465 140 fusions, 1 974 086 nœuds. Total du
run : ~343 s (campagne, 1 fil) → **~136 s** (4 fils, cœur aplati).

## Résultat négatif honnête : le fold ne se laisse pas découper par K

La parallélisation par K du fold ne gagne RIEN (56,1 s contre 54,6 s
séquentiel en campagne) : un K dominant concentre l'essentiel des
événements et son `build_forest` séquentiel est le chemin critique —
le découpage par K et par tranches d'expansion n'y peut rien. Le
prochain levier est INTERNE à `build_forest` : le tri des
enregistrements (fid, événement, slot) du fold sort/reduce, qui en est
le poste dominant, se parallélise en tri-fusion déterministe si le
comparateur est un ordre TOTAL (à vérifier avant de coder — sinon le
tri stable par tranches + fusion stable préserve l'ordre). C'est le
dernier étage séquentiel significatif du pipeline.

`public_status=not_claimed` ; rien de tout ceci n'est une promotion.

## Post-scriptum — le tri du fold hors de cause : profil gravé, expérience revertie

Tentative suivante mesurée puis REVERTIE : `parallel_stable_sort`
(tranches + `inplace_merge` stables, bit-identique au séquentiel) sur
les deux tris de `build_forest`, avec folds séquentiels entre K pour
donner tous les fils au K dominant. Résultat : t_fold 56,1 → 60,4 s —
PIRE, car la sérialisation entre K perd le recouvrement des phases de
réduction, que les tris parallèles ne compensent pas. Profil de
`build_forest` à n=8000 (somme des K, `--threads=4`) :

| étage interne | temps |
|---|---|
| tri des événements (U320) | 0,9 s |
| découpe en lots | 0,1 s |
| construction des enregistrements (facet_minus) | 5,3 s |
| tri des enregistrements (déjà parallèle ici) | 5,0 s |
| internement (fid, first_batch) | 9,4 s |
| **boucle de réduction + final_partition (non instrumenté)** | **~40 s** |

Le vrai cœur du fold est la RÉDUCTION : union-find, rôles par lot,
construction des deltas (vecteurs triés par delta), et
`final_partition` en `std::map` (des millions de nœuds alloués par K).
C'est un chantier d'internes du fold — candidat : `final_partition` en
vecteur trié, deltas à tampons réutilisés — qui touche l'ABI auditée
du fold sort/reduce : il mérite sa propre note d'audit avant de coder.
L'état committé reste celui du tableau ci-dessus (folds par K en
parallèle, tris séquentiels).

# Reçus du 13 août 2026 — rampe du cône prolongée et directions fermées

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Ces reçus servent la
[`NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md`](../../audits/NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md).
Ce sont des **diagnostics**, jamais des benchmarks ni des lois asymptotiques.
La machine porte deux vCPU et exécutait des campagnes concurrentes : les temps
sont indicatifs, seuls les compteurs sont retenus.

## 1. `cone_scale_ramp_raw.txt` — la rampe du cône jusqu'à `n=16 000`

ELF Release `mhgp3v_spindle_cone_probe`, SHA-256
`abbc57c5a430e06c63b94631a584b37df83b1dd33280426c5199bfa3d2d5faef` —
c'est **exactement** l'ELF pincé par
[`AUDIT_CONE_CIBLE_LIVE_ROUTE_50K_20260813.md`](../../audits/AUDIT_CONE_CIBLE_LIVE_ROUTE_50K_20260813.md).
Commande, par famille et par taille :

```text
mhgp3v_spindle_cone_probe --points=$n --family=$fam --seed=3 --bank=48 --leaf=8
```

Ce reçu prolonge la rampe de l'audit, qui s'arrêtait à `n=4 000`, jusqu'à
`n=16 000`. Résultat : les pentes **ne décroissent pas**. Paires candidates sur
`uniform` : `1,846`, `1,845`, `1,858`, `1,824`. Tests témoin--nœud : `1,677`,
`1,772`, `1,811`, `1,716`. À `n=16 000`, `1 147 205 897` tests témoin--nœud et
`2 395 210 838` évaluations de coins laissent encore `63 149 448` paires
candidates sur `127 992 000`.

La colonne `target_visits` du brut duplique `knn_node_visits` : le filtre de
lecture a capté le premier `node_visits=` de la ligne. Les visites cibles ne
sont donc **pas** publiées par ce reçu ; celles de l'audit à `n<=4 000` font
autorité.

## 2. `locality_window_grid4_raw.txt` et `locality_open_cells_raw.txt`

Mesure de la portée du théorème de localité par calottes
([`NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md`](../../audits/NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md),
section 2). Le programme est `window_probe_hors_depot.cpp.txt`, conservé ici
pour rejeu : il est **hors dépôt**, sans porte, sans mutant et en flottant. Il
n'a aucune autorité et ne doit pas être promu tel quel.

Il discrétise la sphère en cellules géodésiques d'octaèdre, prend `K=10`, borne
la recherche à une banque de plus proches voisins, et compte par ancre les
cellules qu'aucune famille de dix calottes ne ferme dans cette fenêtre.

À `n=2 000`, grille 512, banque 512 :

| famille | ancres à zéro cellule ouverte | cellules ouvertes par ancre (moy.) | p50 | max |
| --- | ---: | ---: | ---: | ---: |
| `uniform` | `47,1 %` | `43,3 / 512` | 78 | 247 |
| `eight_clusters` | `30,6 %` | `42,6 / 512` | 57 | 209 |
| `terrain` | `0,0 %` | `63,8 / 512` | 52 | 262 |
| `scanline_overlap_multiecho` | `2,6 %` | `57,0 / 512` | 34 | 335 |

Environ neuf dixièmes des directions sont fermées sur **les quatre** familles.
Le `0,0 %` de `terrain` mesure que chaque point d'une surface possède son cône
normal ouvert, pas que la localité y échoue.

Raffinement de la grille sur `uniform`, `n=2 000`, banque 512 : `52,85 %`
d'ancres totalement fermées à 512 cellules, `49,85 %` à 2 048 cellules — la
discrétisation a convergé, le reste est géométrique.

Limites explicites : `n=2 000` seulement, `K=10` seulement, quatre familles
seulement, banque finie, arithmétique flottante, sphère échantillonnée par
cellules. Aucun de ces chiffres ne borne quoi que ce soit. Ils servent à
choisir une route, pas à recevoir un producteur.

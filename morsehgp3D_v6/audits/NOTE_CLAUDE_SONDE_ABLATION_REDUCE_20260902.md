# NOTE — sonde d'ablation du reduce : décomposition de `materialisation_tri_copie` et `post_remplissage`

Date : 2 septembre 2026. Reçu local
`morsehgp3D_v6/receipts/sonde_ablation_reduce_20260902/` (commit de mesure
`81623528`, binaire `mhgp6_profile_sonde` sha256 `74a46046…`, seule
modification du worktree : le bit exécutable du lanceur). Cadre :
`phase=exploration_v6_hors_registre backend=cpu_reference
profile=quantized_u16_input_only mode=audit_independant_math_and_architecture
public_status=not_claimed`. Faits seulement ; aucune décision ; jamais un mur.

## 1. Pourquoi cette sonde avant tout palier

L'arbre pré-enregistré du § 5.10 et le § 5.17 des auditeurs demandent de
« réduire réellement plusieurs passages du reduce » et de mesurer la latence
avant d'écrire ; la critique adversariale du design CompactDelta (workflow
du 2 septembre, deux sceptiques) a établi que le palier 3 pré-enregistré
**déplace** la copie profonde `scratch → r.deltas` vers une passe finale sans
la supprimer, et a demandé une décomposition de la fenêtre par ablation
(vingt lignes sous `MHGP6_TESTING`) avant 750 lignes de palier. C'est cette
sonde.

## 2. Protocole

- Trois mutants d'ablation dans `reduce_fold` (`src/forest/fold.hpp`),
  constante `false` hors `MHGP6_TESTING`, chacun **change l'objet** et est
  tué (code 4) par la conformité `eight_clusters` 400 :
  `ablation-mat-sans-copie` (la copie profonde `r.deltas.push_back(cd)` et
  ses deux allocations par delta sont retirées ; tout le reste de la fenêtre
  est exécuté), `ablation-mat-sans-tris` (les deux `std::sort` de `FacetKey`
  44 o par racine sont retirés), `ablation-post-cle-factice` (la lecture
  aléatoire `keys[fid]` / `keys[pre_canon]` du remplissage est remplacée par
  une copie locale chaude de `keys[0]` ; le prefetch de `keys[]` reste armé).
- Binaire `mhgp6_profile_sonde` (`MHGP6_TESTING` + `MHGP6_PROFILE_REDUCE`),
  seul à accepter `--inject=` ; le produit refuse (porte
  `mhgp6_profile_refuse_inject`). `uniform`, n ∈ {8000, 16000, 32000},
  48 → 8 fils (`taskset -c 0-7`, Codespace 8 vCPU EPYC 9V74 partagé),
  `--fold-inflight=2 --fold-join=1` (étage B isolé), graine 3, trois
  répétitions en ordre aller/retour, médianes par fenêtre et par K.
  `loadavg` avant/après chaque run gravé — première composante entre 1,78
  et 9,88 sur ce reçu (rectification des auditeurs : le reçu ne permet pas
  d'affirmer l'absence d'autre charge ; le second reçu va de 0,55 à 7,67).
- Lecture des colonnes `profil_reduce K=…` seulement : attribution sur
  binaire instrumenté, `join=1` — le mur `temps_mur_ms` de ce binaire
  n'est pas un mur.

## 3. Résultats (Σ sur K = 1..10, médianes, ms ; écart = bras − aucune)

| n | fenêtre | aucune | sans copie | sans tris | clé factice |
|---|---|---|---|---|---|
| 8000 | materialisation_tri_copie | 1816 | −977 (−54 %) | −499 (−27 %) | −280 (−15 %) |
| 8000 | post_remplissage | 1023 | −12 | +8 | −756 (−74 %) |
| 8000 | somme reduce | 4979 | −1108 (−22 %) | −515 (−10 %) | −1057 (−21 %) |
| 16000 | materialisation_tri_copie | 4162 | −2371 (−57 %) | −1072 (−26 %) | −598 (−14 %) |
| 16000 | post_remplissage | 2194 | +7 | +15 | −1612 (−73 %) |
| 16000 | somme reduce | 11263 | −2544 (−23 %) | −1095 (−10 %) | −2340 (−21 %) |
| 32000 | materialisation_tri_copie | 8948 | −5296 (−59 %) | −2273 (−25 %) | −1371 (−15 %) |
| 32000 | post_remplissage | 4550 | −30 | −17 | −3415 (−75 %) |
| 32000 | somme reduce | 24494 | −5882 (−24 %) | −2446 (−10 %) | −5075 (−21 %) |

À K = 10 seul, n = 32000 : `materialisation_tri_copie` 2879 → 1124 sans
copie (−1754), 2042 sans tris (−837), 2327 à clé factice (−552) ;
`post_remplissage` 1351 → 327 à clé factice (−1024). Les fenêtres `touch`,
`pre`, `unite`, `partition` bougent de −8 à +2 % (sans copie : `touch`/`pre`
−5 à −8 %, effet de trafic mémoire, pas un coût de la copie).

## 4. Lecture factuelle

1. **La copie profonde domine** : 54–59 % de `materialisation_tri_copie`
   sur les trois tailles, part **croissante avec n** (8000 → 32000), soit
   22–24 % du reduce séquentiel entier (5,3 s sur 24,5 s Σ_K à 32000, 1,75 s
   sur le seul K = 10). C'est `r.deltas.push_back(cd)` : zéro, une ou deux
   allocations par delta (une par liste non vide — « deux par delta » n'est
   pas un invariant, rectification des auditeurs) plus la copie de 44 o par
   clé (`parents` + `born`, 14,6 M clés à 16000 K = 10).
2. **Les tris de clés 44 o** pèsent 25–27 % de la fenêtre, stables en n.
3. **La lecture aléatoire `keys[]` du remplissage** pèse 73–75 % de
   `post_remplissage` (3,4 s sur 4,5 s à 32000), stable en n. Sa part
   « −15 % » sur `materialisation_tri_copie` est **confondue** (à clés toutes
   égales, les tris deviennent triviaux) : borne haute, pas une mesure.
4. Le reste de `materialisation_tri_copie` (tri de `post_list`, `find`,
   `batch_levels`, passe `seen`) est ≈ 15 % ; le reste de
   `post_remplissage` (`find` + `post_of`) ≈ 25 %.

## 5. Ce que cela fait au palier CompactDelta pré-enregistré (aucune décision)

- Le palier 3 tel que pré-enregistré (snapshots en fids, conversion
  fid → clé et **copie profonde en passe finale**) supprime les tris 44 o
  (remplacés par des tris u32) et la lecture `keys[]` du remplissage, mais
  **conserve la copie profonde et ses allocations** — 54–59 % de la fenêtre —
  et re-paie la lecture de `keys[]` à la finale. Son gain plausible sur la
  fenêtre est donc borné par ≈ 25 % (tris) + une fraction de la lecture :
  ≈ 10 % du reduce Σ_K, en accord avec la borne « ≤ 15 % » du sceptique.
- Le poste réellement dominant est la **représentation de `r.deltas`**
  (un `std::vector<FacetKey>` par delta pour `parents` et pour `born`). Une
  représentation CSR par K (une arène de clés + offsets, une allocation
  amortie, mêmes clés, même ordre) supprime les 2 allocations par delta et
  la copie élément par élément — l'ablation « sans copie » en est la borne
  haute : jusqu'à −59 % de la fenêtre, ≈ −22 % du reduce Σ_K. Cela change
  la **forme** du payload `ForestResult` (le digest et `on_forest` lisent
  `r.deltas` ; le rendu, lui, se reconstruit depuis les événements et n'est
  pas un consommateur — rectification des auditeurs), pas l'objet : c'est
  une question aux
  auditeurs (`QUESTION_CLAUDE_COMPACTDELTA_CSR_20260902.md`), pas un repli
  pré-enregistré.
- La mémoire : `rss_max_kb` est insensible aux ablations (les arènes du
  scratch et de `st` dominent le pic ; la copie n'ajoute pas au pic mesuré
  ici à inflight 2 / join 1).

## 6. Réception par l'auditeur (`ALERTE_SONDE_ABLATION_REDUCE_20260902`)

Le reçu est classé **`exploratory_noncausal_upper_bounds`** : bornes
exploratoires non causales, jamais un benchmark ni un choix de palier. Trois
coutures, toutes intégrées au lanceur avant toute seconde mesure : plan à
trois répétitions non équilibré (→ carré de Williams, différences appariées
par bloc — à 16000 la différence des médianes du bras sans copie sur le mur
instrumenté disait `+669 ms` là où les trois différences appariées valent
`−847`, `+669`, `−3634 ms`) ; le bras clé factice est une **borne
composite** (lecture `keys[]` + tri de clés égales), pas « lecture seule » ;
reçu fail-open (binaire partagé mutable, `REPS=0` publiable, zéros
substitués) → copie privée hachée avant/après chaque tuple, refus des
matrices vides, agrégateur et manifeste fatals. Les parts de fenêtre du § 3
restent des bornes hautes ; la section 5 n'est qu'une hypothèse à
falsifier avec différences appariées et égalité complète du `ForestResult`.

## 7. Seconde mesure sous le lanceur fail-closed (plan de Williams, 4 blocs)

Reçu `receipts/sonde_ablation_reduce_20260902b/` (commit `2aaa4a53`,
worktree propre, copie privée du binaire `74a46046…` hachée avant/après
chacun des 48 runs, `statut=exploratory_noncausal_upper_bounds`).
Différences **appariées par bloc** (bras − témoin, même bloc, même
taille), médiane [min ; max] sur quatre blocs, Σ_K :

| n | fenêtre | sans copie | sans tris | clé factice (borne composite) |
|---|---|---|---|---|
| 8000 | materialisation_tri_copie | −962 [−971 ; −923] | −487 [−501 ; −459] | −308 [−314 ; −260] |
| 8000 | post_remplissage | −6 [−39 ; +8] | −9 [−14 ; +12] | −755 [−772 ; −740] |
| 8000 | somme reduce | −1066 [−1146 ; −978] | −528 [−575 ; −505] | −1112 [−1198 ; −1073] |
| 16000 | materialisation_tri_copie | −2415 [−2462 ; −2319] | −1052 [−1074 ; −1035] | −669 [−704 ; −588] |
| 16000 | post_remplissage | −6 [−95 ; +9] | +3 [−82 ; +69] | −1608 [−1719 ; −1605] |
| 16000 | somme reduce | −2632 [−3031 ; −2580] | −1128 [−1456 ; −936] | −2397 [−2811 ; −2232] |
| 32000 | materialisation_tri_copie | −5314 [−5368 ; −5268] | −2258 [−2374 ; −2195] | −1400 [−1467 ; −1342] |
| 32000 | post_remplissage | −65 [−67 ; +70] | −31 [−58 ; +156] | −3399 [−3415 ; −3365] |
| 32000 | somme reduce | −5941 [−6008 ; −5121] | −2511 [−2810 ; −1218] | −5140 [−5245 ; −4412] |

Parts retirées de la fenêtre du témoin, appariées (médiane [min ; max]) :
sans copie 53,6 [52,3 ; 54,0] / 57,9 [56,8 ; 58,4] / 59,3 [59,2 ; 59,6] % de
`materialisation_tri_copie` ; sans tris 27,0 / 25,4 / 25,2 % ; clé factice
17,1 / 16,1 / 15,7 % de la fenêtre de matérialisation **et** 74,4 / 74,1 /
75,1 % de `post_remplissage`. Les intervalles des fenêtres dominantes sont
étroits (≤ 2 points) ; ceux des fenêtres de trafic (`touch`, `pre`) et du
mur instrumenté restent larges (dérive de charge), ce qui confirme la
lecture de l'auditeur : seules les fenêtres directement ablatées portent un
signal, et ce signal reste une borne haute non causale. Le premier reçu
(trois répétitions, non équilibré) est corroboré à moins d'un point sur les
trois parts.

## 8. Limites

Machine partagée 8 vCPU ; trois répétitions pour le premier reçu, **quatre
blocs** (carré de Williams) pour le second ; la dispersion n'est PAS
uniformément ≤ 3 % (rectification des auditeurs : témoin 8000, sommes `pre`
632,3 / 665,9 / 633,6 ms et `partition` 399,6 / 405,7 / 382,5 ms, étendues
> 3 % de la médiane) — seules les fenêtres ablatées portent un signal net ;
binaire instrumenté ; `join=1` (le mur nominal est `join=0`) ; une seule
famille (`uniform`) ; les ablations changent l'objet, leurs nombres ne
valent que comme décomposition de fenêtres. Ce reçu n'établit ni un gain ni
une pente ; il désigne le poste à attaquer et la borne haute de chaque
retrait.

GCP non utilisé par cette sonde.

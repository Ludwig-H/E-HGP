# Note de Claude — réparations P0 du cône cible, et ce qui reste ouvert

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Réponse aux deux contre-audits du 13 août :
[`AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md`](AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md)
et
[`AUDIT_CONE_CIBLE_LIVE_ROUTE_50K_20260813.md`](AUDIT_CONE_CIBLE_LIVE_ROUTE_50K_20260813.md).
Le NO-GO de l'ordonnance par endpoint est **accepté sans réserve** ; il est
confirmé et étendu par ma propre rampe jusqu'à `n=16 000`, et il motive le
changement de route de la
[`NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md`](NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md).

Cette note ne traite que la dette d'exactitude. Elle ne revendique aucune
réception : les portes ci-dessous sont des portes, pas un verdict.

## 1. Le faux vert `smax` — réparé deux fois, pas une

Le défaut était double, et corriger une seule moitié l'aurait laissé
reproductible sous une autre forme.

**Moitié 1, le domaine.** `--smax` est désormais borné **avant tout cast**, à
`4 <= smax <= 34` — c'est `envelope_depth(smax)=smax-2` contraint par
`kMaxEnvelopeDepth=32`. `strtoll` sature en outre silencieusement à
`LLONG_MAX` et ne le signale que par `errno` : `errno` est maintenant testé.
Cinq refus gravés, tous antérieurs au moindre calcul : `LLONG_MAX`,
`INT_MAX+1`, `35`, `3`, et une valeur à vingt chiffres qui déclenche `ERANGE`.

**Moitié 2, le mode commun.** La vraie faute n'était pas la conversion, c'était
que **le juge partageait la conversion**. Les seuils devenaient négatifs des
deux côtés, sujet et juge fermaient ensemble `380/380` paires sans un seul
témoin, et l'accord était donc parfait sur une contradiction.

Le juge a changé d'unité de traduction : `prototype/spindle_cone_oracle.cpp`
n'inclut ni `spindle_cone.hpp`, ni `anchor_envelope.hpp`, ni `mhgp/mhgp.hpp`.
Le compilateur garantit ainsi qu'aucune ligne du sujet ne s'exécute dans le
juge. Il recalcule ses seuils en `long long` et **refuse** un domaine
inutilisable au lieu de convertir. Son arithmétique 128 bits est réécrite à la
main sur deux limbes `u64` en complément à deux ; la production emploie le
`__int128` du compilateur. Cette arithmétique est elle-même jugée par
`mhgp3v::BigInt` — signe et magnitude, chiffres de 32 bits — sur 40 000 tirages
en petites coordonnées puis en pleine largeur u16, dans le selftest.

Trois représentations, trois écritures, aucune ligne commune.

## 2. Les trois lanes sont jugées séparément

Le sujet ne remplissait qu'un bitset, et seulement lorsque q2, q3 **et** q4
étaient mortes ; le juge faisait le même ET. Une fermeture q3 seule ou q4 seule
pouvait donc être fausse sans être observée, alors que l'aval consomme les
lanes séparément.

Le sujet tient désormais quatre bitsets — `q2`, `q3`, `q4` et leur conjonction
— remplis au nœud exact où chaque lane bascule. Le juge publie trois vérités.
Les trois inclusions `closed_q subset dead_q` sont vérifiées séparément, avec
un compte d'accord et un compte de désaccord par lane, et **un plancher non nul
par lane** sur les quatre portes de juge. La conjonction ne les remplace pas :
elle est vide dès qu'une seule lane ne ferme rien.

Mesure au pin, `uniform n=300 seed=3 bank=48` : `q2=48607/74104`,
`q3=11826/54292`, `q4=9290/51896`, désaccords `0/0/0`, `26 730 600` témoins
évalués par le juge indépendant.

## 3. Cardinalité, digest, compteurs

- `--points=100 --coord=2` rendait huit points, code zéro, et toutes les
  identités étaient vérifiées sur le mauvais univers. La cardinalité produite
  est maintenant comparée à la cardinalité demandée, refus en code 2 avant le
  LBVH, et le reçu publie un digest FNV-1a du nuage effectivement généré.
- `witness_none_q3/q4` comptaient les **hits** : la même réfutation était
  recomptée à chaque visite du sous-arbre. Ils comptent maintenant les
  **transitions**. Conséquence directe et honnête : le plancher
  `--min-none-q4` de la porte `uniform` était calibré sur `200 000` hits ; la
  mesure réelle en transitions vaut `170 160`. Le plancher descend à `150 000`.
  C'est une correction à la baisse d'un plancher, elle est signalée comme telle.
- `none_classifier_calls` est publié. Le coût du classifieur `NONE` — un
  `Hmax`, trois intervalles de produit vectoriel, plusieurs carrés larges —
  n'apparaissait dans aucun compteur. Au même pin il vaut `673 432` appels pour
  `170 160` réfutations q4 : quatre appels par réfutation.
- Les réfutations héritées se consultent selon `floor`, plus selon `want`. Quand
  une lane inférieure avait atteint son seuil, `floor` valait q3 ou q4 alors que
  `want` valait encore q2 : la réfutation de la lane réellement utile était
  ignorée et le classifieur repayé dans tous les descendants.
- Le LCG est en arithmétique non signée. Le débordement signé est indéfini.
- Le message de refus de banque annonce `[1, 256]`, la vraie borne.

## 4. Le mutant d'héritage a enfin une porte

`cone-ignore-inherited` était implémenté et tuait, mais aucun CTest ne
l'exerçait. Un mutant sans porte ne prouve rien. Il est désormais permanent, sur
`uniform n=300 bank=48 --verify`, code 4 attendu.

## 5. Inventaire des portes

`39` CTests `mhgp3v_cone_`, contre `30` au snapshot contre-audité. Les neuf
ajoutées sont : six refus de domaine `smax`, le refus de cardinalité, le
plancher d'accord par lane sans juge, le plancher `q4` non atteint, et le
mutant d'héritage.

## 6. Ce qui reste ouvert, et que cette note ne prétend pas fermer

1. **Le résiduel n'est toujours pas un flux consommable.** `unknown_to_residual`
   et `residual_block_mass` restent deux scalaires. Aucun record
   `(A_node, B_node, lane_mask, credit_receipt, epoch)` n'est émis, et
   `candidate_pairs` omet la masse résiduelle. La partition
   `mass_closed + terminal_alive + residual_alive = n(n-1)` par lane n'est pas
   encore vérifiée par une porte.
2. **Les caps ne mordent dans aucun CTest**, et `visits>cap` intervient après la
   classification du nœud : ce n'est pas un cap absolu de travail.
3. **`pairid_before_terminal` et `bank_restarts` restent structurellement
   nuls.** Leur zéro est vacueux tant qu'aucun scénario ne les incrémente.
4. **Aucune porte ne compare la banque k-NN à un top-`M` exhaustif ordonné**,
   égalités comprises.
5. **Le selftest revendique les réflexions** mais n'exerce que six permutations
   d'axes et des translations.
6. **Aucune cible UBSan.** Le mutant `narrow-i64` dépend encore d'un
   débordement signé indéfini.
7. **`PASS_REGULAR_EXPRESSION` masque encore le code de sortie sur dix-huit
   portes à planchers** — pas seulement `anchor_`, mais aussi `oracle_inject_`,
   `direct_source_`, `pair_selfjoin_`, `pair_anchor_`, `yao48_`, `p1a_` et
   `centre_cell_`. Leurs planchers sont donc inopérants. Je le répare ensuite,
   porte par porte : certaines s'appuient peut-être sur le regex pour masquer un
   code non nul volontaire, et les retirer en bloc les affaiblirait au lieu de
   les renforcer. Le garde-fou de configuration qui interdira la combinaison
   viendra **après** la réparation, pas avant.
8. **L'ABI CUDA `run_anchor_point` ne transporte pas `density_guard`.** Je suis
   la recommandation d'ablation : la garde sort du chemin produit.
9. **Le relèvement collectif `A × B × C` n'est pas écrit.** Il ne le sera pas
   non plus tel quel : la note de route propose de quitter entièrement
   l'élimination de paires. Si vous jugez ce virage prématuré, dites-le — le
   lift bloc reste la meilleure réparation *de cette famille de routes*, et je
   l'écrirai.

GCP non utilisé.

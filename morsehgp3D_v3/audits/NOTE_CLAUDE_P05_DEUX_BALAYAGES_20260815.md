# Note de Claude — P0.5 fait, et la redondance à huit n'existe pas

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé.

Répond au point **P0.5** de
[`AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md`](AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md) :
« Passer ce compte en deux passes, avec budget `n |S|`, paires `D>0` et scan
point--point multi-lane unique. »

## 1. Ce qui est fait

Le balayage du `W`-vivant est désormais en deux passes :

- **passe 1**, `O(1)` par paire : le masque des lanes où `(a,b)` survit, par
  test de budget. Masque vide, la paire est sautée sans être touchée ;
- **passe 2**, seulement si le masque est non vide : **un** balayage de `z`,
  **une** évaluation `pair_lane(a,b,z)`, trois compteurs alimentés, et chaque
  lane éteinte dès qu'elle atteint son `h_q`.

Les paires `D = 0` sont exclues et comptées à part (`degenerees`), `V_q` étant
défini sur `||a-b|| > 0`. Le reçu publie maintenant `paires`, `travail`
(visites de `z`, unité comparable entre modes), `evals` (évaluations du prédicat
`(e,t)`) et `degenerees` : le budget `n |S|` est **vérifiable**, non affirmé.

Contrôle de non-régression : `uniform, n=600, s=8` rend toujours
`17 479 / 42 294 / 45 913`, à l'unité.

## 2. Le point sur lequel je vous corrige, mesure à l'appui

Votre section 6.4 dit que `corner8_lane(a, Box(b), z)` « évalue huit fois le
même coin quand `b` est ponctuel ». C'est exact du **pire cas** et faux en
régime : la fonction rend `0` dès le **premier** coin dont `H <= 0`, et les huit
coins étant identiques, elle sort après une évaluation sur tout `z` qui n'est
pas dans le demi-espace. La redondance vaut donc exactement `1 + 7 P(H > 0)`.

Mesurée à `n=300`, `s=8`, `seed=3` :

| famille | visites | évaluations legacy | redondance | `P(H>0)` |
| --- | ---: | ---: | ---: | ---: |
| `uniform` | `15 136 960` | `20 984 214` | `1,386` | `5,51 %` |
| `terrain` | `7 650 619` | `9 525 079` | `1,245` | `3,50 %` |
| `eight_clusters` | `18 107 872` | `33 416 480` | `1,845` | `12,07 %` |

Jamais huit. **Le gain vient donc de l'autre moitié de votre P0.5** — la fusion
des trois passes — et non de la spécialisation ponctuelle :

| famille | évals legacy | évals fusion | gain |
| --- | ---: | ---: | ---: |
| `uniform` | `20 984 214` | `6 690 404` | `3,136` |
| `terrain` | `9 525 079` | `3 153 925` | `3,020` |
| `eight_clusters` | `33 416 480` | `8 429 105` | `3,964` |

Temps de paroi du seul balayage, `s=8`, obtenu en retranchant le coût du même
run sans `--vrai-vivant` :

| famille | `n` | legacy | fusion | rapport |
| --- | ---: | ---: | ---: | ---: |
| `uniform` | `600` | `412 ms` | `208 ms` | `1,98` |
| `uniform` | `1 500` | `2 891 ms` | `1 518 ms` | `1,90` |
| `terrain` | `600` | `167 ms` | `77 ms` | `2,17` |
| `terrain` | `1 500` | `1 132 ms` | `471 ms` | `2,40` |
| `eight_clusters` | `600` | `533 ms` | `279 ms` | `1,91` |
| `eight_clusters` | `1 500` | `4 274 ms` | `2 295 ms` | `1,86` |

Le temps gagne moins que les évaluations (`1,9`--`2,4` contre `3,0`--`4,0`) :
la boucle est bornée par le parcours mémoire de `sorted_pts`, pas par
l'arithmétique. Je le signale parce que c'est exactement le genre d'écart que
j'aurais pu ne pas publier.

## 3. Le montage de preuve, parce qu'une réécriture de balayage et une
réécriture de ce qu'il compte se ressemblent

L'ancien balayage est **conservé** sous `--vivant=legacy`, et
`audits/check_vivant_balayages.py` confronte les deux. Quatre portes nouvelles :

| porte | ce qu'elle exige | code |
| --- | --- | ---: |
| `mhgp3v_vivant_deux_balayages` | égalité à l'unité des trois lanes, trois familles, planchers `--min-vivantes=500` et `--min-paires=2000` | `0` |
| `mhgp3v_vivant_extinction_neutre` | le mutant **neutre** `vivant-sans-extinction` ne change aucun compte | `0` |
| `mhgp3v_vivant_mutant_lane_unique` | le mutant létal `vivant-lane-unique` est **tué** | `1` |
| `mhgp3v_combined_refus_vivant_mutant_dans_son_juge` | injecter un mutant `vivant-*` dans le balayage legacy est refusé avant calcul | `2` |

Plus deux refus : `--inject=vivant-*` hors du mode `--vrai-vivant`, et
`--cout-instruction --vivant=legacy`, que le balayage à une lane ne peut pas
alimenter.

Le mutant neutre n'est pas décoratif : il chiffre ce que l'extinction gagne
(`11,1 %` des visites sur `uniform`, `13,1 %` sur `terrain`, `24,3 %` sur
`eight_clusters`) **et** atteste que la sortie anticipée ne change aucun compte
— ce qui est la seule chose qui pourrait la rendre fausse.

J'ai dû relâcher deux gardes du probe pour que ce montage existe : ces deux
mutants n'altèrent aucun certificat, donc le juge par force brute ne peut pas
les voir, et les déclarer « survivants » masquerait leur mort réelle derrière un
code `3` sans rapport. Les gardes sont remplacées par deux refus explicites, et
la raison est écrite à l'endroit du relâchement.

## 4. Ce qui reste dû sur votre plan

Non fait, et je ne le prétends pas : **P0.3** (protocole cap-aware complet,
scission récursive des gros endpoints, matérialisation des `PairId` survivants,
comparaison `s=6/8/10` à stratégie de cap identique) ; **P1.7** (ledger par lane
q2/q3/q4 et mutant de masque propre au bulk boule q3/q4) ; **P1.8** (`h_b`
vérifié séparément dans la porte dual-tree, seul `dA` l'est) ; **P1.9**
(égalité boule OFF/ON contre le **même** repli Corner64) ; **P1.10** (reçus
injectifs sur les modes réellement exécutés) ; **P1.11**--**P1.13**, **P1.15**,
**P2.16**--**P2.17**.

De la section 6.3 il reste aussi `masse_non_decide=0` imprimé comme statut
positif, l'égalité `s6/s8` sur le même nuage, et le mutant qui force une paire
de `V_q` hors de `S_q`.

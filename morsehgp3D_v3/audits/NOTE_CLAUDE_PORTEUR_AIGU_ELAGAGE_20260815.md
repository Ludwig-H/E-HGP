# Note de Claude — le porteur aigu tient dans un signe, et deux réfutations en route

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé — `gcloud` est absent du conteneur.

Répond à [`NOTE_AUDIT_Q4_PROPOSITIONS_VS_SORTIE_20260815.md`](NOTE_AUDIT_Q4_PROPOSITIONS_VS_SORTIE_20260815.md).

## 1. Le lemme, et sa stricte

`(a,b)` étant l'arête maximale, avec `H = (x-a).(b-x)` — le `H` du fuseau, pas
un autre :

`x est porteur aigu de (a,b)  <=>  x dans L(a,b)  ET  H < 0`.

Les angles en `a` et `b` sont **gratuits** : si l'angle en `a` valait `>= 90`,
alors `|bx|^2 = |ax|^2 + |ab|^2 - 2 (x-a).(b-a) >= |ax|^2 + |ab|^2 > |ab|^2` et
`x` sortirait de la lentille. Seul l'angle en `x` peut être obtus, et son signe
est `-H`.

C'est la condition **couplée** de l'audit, pas trois bornes indépendantes :

`|ax|^2 + |bx|^2 - |ab|^2 = -2H`,

donc `|ax|^2 + |bx|^2 > |ab|^2` équivaut exactement à `H < 0`.

Vérifié à `331 857` triplets sur quatre régimes — dont une grille `3^3`
volontairement dégénérée — **sans un écart**. Mon premier jet écrivait `H <= 0`
et faisait `185` écarts sur `8 005` ancres, tous du même cas : `H = 0` est
l'angle **droit**, donc un non-porteur.

Corollaire, vérifié lui aussi sans écart : `W_2(a,b) = {H > 0}` est la boule
diamétrale ouverte, donc la lentille se **partitionne** en non-porteurs
(`H >= 0`) et porteurs (`H < 0`). Un témoin q2 est exactement un non-porteur.
Le même parcours rend les deux ; j'en jetais la moitié.

## 2. Les étages, séparés et renommés

Le compteur que j'appelais `q4` ne mesure que des **paires-ancrages**. Renommé.
À `n=400`, `s=8` :

| famille | `V4_pair_walive` | `C4_carrier` | contraction |
| --- | ---: | ---: | ---: |
| `two_lines` | `43 128` | **`0`** | **`1,0000`** |
| `terrain` | `16 536` | `136 926` | `0,0767` |
| `uniform` | `38 070` | `1 172 715` | `0,0263` |
| `eight_clusters` | `56 633` | `3 428 909` | `0,0170` |

`two_lines` : la positivité retire **la totalité** des ancres. C'est le fait
architectural, et il ne dépend d'aucun certificat de témoins.

## 3. Deux réfutations, gardées parce qu'elles coûtent cher à redécouvrir

**Le certificat au niveau du rectangle `A x B` ne sert à rien.** C'était l'objet
que l'audit propose (`NONE_ACUTE` sur `A x B x C`) et la structure de
`h_coeur` — un certificat valable pour toutes les ancres d'un coup. Mesuré :
`gain = 1,005`, `90 324` blocs pour `99 210` points, soit `1,1` point par bloc.
Il n'élaguait que des feuilles. Le quantificateur universel sur `A x B` est trop
fort : il échoue dès qu'une seule paire du rectangle rend un `x` aigu.

**Le certificat de boule seul est plus lent que le balayage complet.**
`gain = 0,33` à `0,59`. La boule diamétrale est petite devant l'emprise du
nuage, donc « boîte incluse dans la boule » ne peut jamais élaguer près de la
racine : la descente parcourait `657` nœuds sur `799`.

## 4. Ce qui marche, et son chiffre honnête

Deux disjoints, chacun exact sur l'enveloppe continue :

- `bloc_hors_lentille` — la boîte est hors d'une des deux boules `B(a,|ab|)`,
  `B(b,|ab|)`. C'est lui qui élague le **lointain**, et qui rend la descente
  logarithmique.
- `bloc_dans_boule_diametrale` — `H >= 0` partout. C'est lui qui tue
  `two_lines`, dont la lentille est pleine mais entièrement non aiguë.

En complétant le carré, `H(x) = R^2 - |x - m|^2` avec `m` le milieu et
`R = |ab|/2` : le second est une **inclusion de boîte dans une boule**, donc
trois `max` et une comparaison, non huit évaluations. C'est ce facteur huit qui
séparait le certificat refusé de sa version utilisable.

| famille | `travail_ref` | `travail_elag` | gain |
| --- | ---: | ---: | ---: |
| `two_lines` | `17 164 944` | `1 291 198` | **`13,29`** |
| `terrain` | `6 581 328` | `1 454 124` | `4,53` |
| `uniform` | `15 151 860` | `9 019 189` | `1,68` |
| `eight_clusters` | `22 539 934` | `18 718 310` | `1,20` |

Le coût est compté en **évaluations du prédicat** des deux côtés — une par nœud
visité, une par point testé. Comparer des tests de feuilles à des tests de
points aurait fabriqué un gain, exactement comme le dual-tree l'avait fait.

Sur `two_lines`, `points_elagues` vaut `17 251 200 = 43 128 x 400` : **tous**
les points, zéro test de feuille, `~30` visites de nœud par ancre.

## 5. Les portes

| porte | ce qu'elle exige |
| --- | --- |
| `mhgp3v_seed_elagage_exact_<fam>` | `ecarts=0` et `certif_desaccords=0`, quatre familles |
| `mhgp3v_two_lines_etages_separes` | `V4_pair_walive=43128`, `C4_carrier=0`, `contraction=1,0000` |
| `mhgp3v_seed_elagage_gain_two_lines` | `points_elagues=17251200`, la totalité |

Le juge du certificat confronte `bloc_dans_boule_diametrale` à la version à huit
coins : même prédicat, deux chemins sans primitive commune — distance maximale à
un centre d'un côté, énumération de coins et produits scalaires de l'autre. Je
l'avais d'abord mal cadré, en confrontant le certificat **complet** aux huit
coins, et j'ai lu `365 234` désaccords en croyant à un défaut : c'était le
second disjoint qui faisait son travail.

## 6. Ce que je ne prétends pas

L'élagage rend la descente logarithmique **par ancre** ; il n'empêche pas
d'énumérer les `Theta(n^2)` ancres. Le verrou que l'audit nomme — ne jamais
matérialiser les propositions de paires, produire directement la source
ternaire factorisée — reste **ouvert**, et ma réfutation du certificat au niveau
rectangle dit que la voie évidente n'y mène pas.

Non fait : la porte mémoire attestant qu'aucun tableau de taille
`V4_pair_walive` n'est alloué ; les étages `M4_apex`, `W4_positive`, `H4_rank`,
que ce probe ne calcule pas ; le protocole cap-aware complet (P0.3) ; les
ledgers q3/q4 et la symétrie `h_a`/`h_b`.

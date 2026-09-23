# D5, tour FULL maigre : analyse du sidecar

```text
phase=exploration_v9_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=lecture_audit_sidecar
public_status=not_claimed
```

GCP non utilisé. Worktree `3dfedcae1` en lecture seule. Je n'ai rien compilé ni exécuté. Tous les comptes ci-dessous sont re-sommés depuis les JSON du sidecar : ce ne sont pas de nouvelles mesures.

Chemins relatifs à `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/` :
- **SC** = `audits/c_alternatives_20260923/experiences/d5/d5_probe.cpp` (version finale) ;
- **RF** = `audits/c_alternatives_20260923/experiences/refute_d5_full/` ;
- **FBT** = `src/tower/forest/full_ball_tower.hpp`.

## 0. Versions de la sonde et provenance des chiffres

| Version | Ce qu'elle ajoute | Sorties |
| --- | --- | --- |
| v1 | Tour produit à 1 fil ; réplique de la voie statique (MEB, clé, fenêtre, échange, graine après échange) ; comptage de ce que couvriraient les index graines et selles ; phase A maigre v1 ; taille compacte | `probe_s01_8k_k5/k10.json` |
| v2 | Chronos CPU ; route graines + selles + mémo + intrus lu dans le catalogue (recoupé à l'arbre, `catalogued_intruder_differs`) ; égalité de **terminal** exigée (`d5_terminal_mismatch`) | `probe2_s01_8k_k5` |
| v3 | Saut au centre (census d'arbre à chaque état, repli par échange) ; **racines pré-lot** comparées facette par facette | `probe3_s01_8k_k5` |
| v4 | Phase A v2 (positions de programme, lots singletons sans allocation, tampon `r[13]`) ; images verticales (lemme C, remontée naïve, naturalité) | `probe4_s01_8k_k5`, `probe4_s01_8k_k10` |
| finale | MEB(G) réutilisée à l'état suivant ; census lu dans le catalogue quand la clé de D y figure ; G haché avant toute MEB | `probe4_s01_16k_k5` (malgré son nom, il contient `catalogued_census`), `probe5_s01_8k_k10_product` |

- Les chiffres de l'audit (MEB ÷5,3–5,7, pas maximal 159 → 10, nœuds 53,4 → 15,6 M, 87 ns/bloc, 2 456 ms) viennent de la **version finale**.
- Les 2 456 ms se décomposent ainsi : programmes 444 + saut 1 748 + phase A 156 + images 108 ms CPU. Le produit fait 8 577 ms.
- Le chiffre de 124 ns/bloc vient de la v4.
- Les 670 642 facettes de 8k/K5 viennent de v3/v4 : même règle d'arrêt, mais sans census catalogué.

## 1. (a) Résolution en trois étages et règle 0

**Contrat.** Soit un bloc de la boule B à l'ordre K, de niveau λ. Chaque facette F reçoit une boule T :
- F est soit `I_B ∪ U_B∖{s}` (SC:227-234), soit une composante stricte de `ShellTable::rank(K)` (SC:236-247) ;
- T est au programme K, c'est-à-dire `p_T+q_min−1 ≤ K ≤ p_T+u_T` (SC:209-213) ;
- T a un niveau `< λ` ;
- l'ancre de T est dans la composante de F dans `L_K(λ⁻)`.

Toute cible de cette composante donne la même racine pré-lot.

**Prédicats exacts utilisés.**
- **P1** : égalité de K-uplets triés.
- **P2** : `anchor_meb_proposed`. Welzl en double ne fait que proposer, la vérification est entière. Le résultat donne la clé (5 i128), l'`ExactLevel` et les `support_slots`.
- **P3** : recherche exacte d'une `BallKey`.
- **P4** : fenêtre entière sur (`n_interior`, `arity` = q_min, `n_shell`).
- **P5** : puissance i128 `a‖z‖²+b·z+c`, avec a > 0. Son signe sépare intérieur et coquille ; son ordre est celui des distances au centre.
- **P6** : `AxisBounds` i128 sur une boîte de l'arbre radix.
- **P7** : `compare_exact_level` (U192×i128 → U320).

**(a) Graines.** Si `p_T+u_T = K` et `F = I_T∪U_T`, la règle 0 s'applique sans MEB, par P1 seul.
- Sidecar : SC:288-293.
- Produit : FBT:1480-1487, 1516-1519 et 1571-1578.

**(b) Lemme A (selles).** Soit C régulière : `U_C = S_C` est son unique support minimal. Si `S_C ⊆ F ⊆ I_C ∪ S_C`, alors `MEB(F) = C`.
- Preuve : C est la MEB de S_C ; tout englobant de F contient S_C ; la MEB est unique.
- La facette n'est admise qu'à `|F| = p_C+q_C−1`, soit `F = S_C ∪ I_C∖{z}`.
- Il faut, par ordre, un index de Σp_C K-ensembles (SC:294-303 ; table plate XOR + P1 dans `d5_refut/hash_probe.cpp`).
- Les coquilles étendues ne sont pas indexées.

**Règle 0**, appliquée à **chaque** état s :
- D = MEB(s) (P2) ;
- si la clé de D est au catalogue (P3) et que la fenêtre de D contient K (P4), la cible est D.

Elle est codée en SC:473-477. Le produit l'applique déjà dans l'échange (FBT:1272-1282).

**(c) Lemme B (saut au centre).** Si D n'est pas terminal :
1. On fait le census de la boule fermée D̄ (puissance ≤ 0) :
   - si D est catalogué, on lit `interior()` (P5) et `shell()` (puissance 0) (SC:483-486) ;
   - sinon, on parcourt l'arbre avec P6 : élagage si lo > 0, nœud entier si hi ≤ 0 (SC:487-503).
2. G = les K sites de plus petite (puissance, rang géométrique) (SC:506-509).
3. **Témoin** : le centre c de D est à distance ≤ r_D de tout site de F ∪ G. Donc `c ∈ C_F(r) ∩ C_G(r) ⊆ L_K(r)` pour tout r ≥ r_D. Comme r_D < λ (P7, FBT:1272), F et G sont dans la même composante à λ⁻.
4. Si G est une graine, une selle ou un état mémorisé, on termine (SC:512-514). Sinon E = MEB(G), et on continue avec E si niveau(E) < niveau(D) (SC:515-520).

**Renforcement (vérification adverse).** Si D n'est pas terminal, alors `K < p_D+q_min−1` :
- hors d'un catalogue complet, `p_D+q_min > Kmax+1 ≥ K+1` ;
- `K > p_D+u_D` est impossible, car s ⊂ D̄.

G contient donc I_D et au plus q_min−2 sites de coquille, donc aucun support de D. D'où MEB(G) < r_D strictement. Trois conséquences :
- le repli par échange (SC:521-529) n'est jamais atteint (0 repli partout) ;
- la terminaison est une descente stricte de niveau ;
- `|D̄∩P| > K` est garanti (SC:505).

**Conséquence absente de l'audit.** À K = Kmax, une D non terminale n'est **jamais** cataloguée : `catalogued_census` vaut 0 à K10 dans probe5, et à K5 à 16k.

## 2. (b) Comment le sidecar prouve l'égalité des racines

1. `dump` : chaîne à 1 fil, catalogue conservé (SC:93-106), sur les coupes s01 de 08/000100. Le cas 8k/K5 est le filtre `p+q_min ≤ 6` du catalogue K10 (SC:116-120).
2. Tour produit sur le même catalogue, à 1 fil statique (SC:137).
3. Réplique de la route produit, par facette distincte (SC:316-375). Ses compteurs égalent ceux du produit à 8k/K10 : MEB 1 778 659, intrus 1 089 587, nœuds 53 432 975.
4. Saut calculé par facette distincte, puis étalé par occurrence (SC:542-544).
5. Phase A v1 construite avec les cibles de la **réplique**. À chaque occurrence de facette, dans l'état pré-lot, on compare `find(anchor[saut])` à `find(anchor[produit])` (SC:602-613). De plus :
   - nœuds et fusions par K sont égaux au produit (SC:821) ;
   - la phase A v2 est égale à la v1 (SC:756).

| Cas | Occurrences K ≥ 2 égales | Écarts | MEB produit → saut | Nœuds d'arbre | Pas max |
| --- | ---: | ---: | --- | --- | --- |
| 8k/K5 (v3/v4) | 670 642 | 0 | 211 374 → 78 952 | 4,05 → 3,13 M | 107 → 4 |
| 8k/K10 (finale) | 3 042 577 | 0 | 1 778 659 → 310 249 (÷5,73) | 53,4 → 15,6 M (v4 : 29,5 M) | 159 → 10 |
| 16k/K5 (finale) | 1 328 737 | 0 | 422 312 → 79 564 (÷5,31) | 8,77 → 4,80 M | 107 → 5 |

- Total : 5 041 956 occurrences.
- 18 % des facettes distinctes changent de terminal : seule la **racine** est invariante.
- S'y ajoutent 221 556 occurrences sur 12 nuages dégénérés (`RF/r_lat*`, `r_plan*`, `r_sq*`), sans aucun écart.

**Limites.**
- Une seule scène ; ni 32k, ni K10 à 16k, ni trame entière.
- Les racines sont prises dans la phase A du sidecar, validée par des comptes et non par `same_payload`.
- Les chiffres du saut incluent l'index des selles, le mémo `jmemo` et le census catalogué. Ce n'est **pas** le saut seul.

## 3. (c) Défauts connus et corrections

**Règle 0 (`fx_cz`).**
- Sites : a=(500,1000,1000), b=(1500,1000,1000), c=(1000,1000,1500), x=(1000,1600,1000).
- On regarde la facette {a,b} de la boule q3 abx, à K2. D = MEB({a,b}) a pour coquille {a,b,c} (angle droit en c), avec p=0, q_min=2 et une fenêtre [1,3].
- Sans règle 0 : G = {a,b} = F, donc MEB(G) = D. Le niveau ne baisse pas, et l'échange ne trouve aucun intrus strict. **Échec.**
- Le code du sidecar avait cette règle (SC:473-477). Le texte ne l'avait pas (`principe_draft.md` § 2(c), item 2 de la complétude de D5).
- La variante « prose » (RF/patch.py:29-120) laisse 1 facette non résolue sur `fx_cz`, et 210 à 2 787 sur les grilles. `fx_czm`, `fx_c34` et `fx_cm34` passent : l'échec dépend de l'ordre de Morton.
- **Correction** : appliquer la règle 0 à chaque état, y compris après un saut, et tuer un mutant « sans règle 0 ».

**Format compact : la Proposition F est fausse.**
- Une continuation contributive prend le rang du premier bloc de son groupe, et ce bloc peut être muet.
- Exemples : lat5_3 K8, `contributions[28]` précède `[29..]` ; lat5_1 K9, `[316]` précède `[317]` (RF/order_check.cpp).
- **Correction** : chaque exception porte la position de programme du premier bloc de son groupe (+4 o).
- Le sidecar ne mesurait que la taille (SC:794) : il ne pouvait pas voir ce défaut.

**Tampon de 13 racines.**
- SC:707 déclare `r[kBallShellMax + 1]`, soit 13 cases (`ball_data.hpp:22`). SC:713 y écrit sans vérifier de borne.
- `fx_ico12` : 12 sites c ± 100·v donnent 32 composantes strictes à K6. Le produit y crée une fusion à 32 parents (RF/comp_check.cpp).
- La sonde passe en comportement indéfini, sans rien signaler.
- **Correction** : petit tampon, puis offsets CSR au-delà.

**Lemme C : la borne basse manque.**
- Il faut d'abord exclure K = p_B+q_min−1. Là, le bloc émet au moins une facette : q_min s'il est régulier, au moins une composante stricte s'il a une coquille étendue.
- Il a donc une racine parente et ne peut pas être une naissance.
- Le sidecar le vérifiait implicitement (SC:771-772).
- Garder le refus produit `full_ball_vertical_birth_anchor`.

**Porte E1 (contre-fixture 0, 1, 10, 11 à K2).**
- Bloc diamètre (0,10) : λ = 25, facette F = {0,1}.
- La boule diamètre (10,11) (niveau 1/4, fenêtre admise) passe tous les contrôles structurels. Pourtant, elle est dans une autre composante juste sous r = 5.
- L'adaptateur externe du produit a la même faille (FBT:1422-1431).
- Le sidecar comparait bien les racines (SC:602-613). La porte doit exiger cette comparaison, ou certifier chaque saut : G ⊂ D̄, |G| = K, niveau(D) < λ, baisse stricte.

## 4. (d) Ce qui a été mesuré négatif, et pourquoi

**Reçu `saddle_index_negative_20260923`** (coupe 16k de 08/000200, K10, W8) :

| Mesure | Index OFF | Index ON |
| --- | ---: | ---: |
| MEB | 2 700 241 | 1 688 262 |
| Intrus | 1 337 551 | 1 337 551 |
| Entrées d'index | 0 | 10 188 238 |
| Phase 0 | 2 582 ms | 2 752 ms |

- Le condensé est identique.
- Réserves de B : le juge par coup n'existe que sous `MHGP9_TESTING`, et la mesure repose sur une seule paire et un seul n.
- Ce qui est fermé : l'index isolé, pas le lemme.

**Pourquoi l'index seul ne paie pas.**
- Le lemme A ne retire que des résolutions de profondeur 0 : une MEB et une recherche de clé, sans arbre (FBT:1279-1281). Les chaînes restent intactes.
- Il construit et trie environ 10 entrées par MEB évitée, soit 0,79–1,02 s nets à 8k/K10.
- Le coût réel est dans les chaînes. À 8k/K10, 281 637 facettes font 1 089 587 échanges et parcourent 53,4 M nœuds. Les chaînes de profondeur ≥ 7 (2,2 % des facettes) portent 58 % des échanges.

**Le saut sans l'index.** Ses gains principaux ne dépendent pas de l'index :
- les facettes-selles ne parcourent jamais l'arbre ;
- la baisse des nœuds vient du saut et du census catalogué.

Estimation sans index ni mémo, tirée des compteurs de probe5 :
- chaque facette non-graine paie MEB(F), comme aujourd'hui (758 840 facettes) ;
- chaque G haché (232 687) ou mémorisé (48 943) coûte au plus une MEB(G), sans arbre.

| Cas | MEB estimées | Nœuds d'arbre | Travail pondéré |
| --- | --- | --- | --- |
| 8k/K10 | 0,84–1,07 M (÷1,7–2,1, contre ÷5,7 avec index) | environ 15,6 M, plus la suite des chaînes mémorisées | ÷1,8–2,6 |
| 16k/K5 | 0,26–0,33 M (÷1,3–1,6) | 4,8 M (÷1,8) | ÷1,45–1,7 |

Le travail pondéré compte 3 680 cycles par MEB et 136 par nœud (coûts du développeur, Zen 3 local). Ce sont des **projections, pas des mesures**.

**Point faible à K5.** À Kmax, tout census passe par l'arbre : les nœuds ne baissent que de ÷1,4 à 16k/K5. Or c'est l'ordre le plus lourd : `static_by_k` y vaut 86–110 ms sur 222–272 ms de phase statique en R13.

**Conclusion.**
- Le saut devrait payer seul à K10. À K5, ce n'est pas acquis.
- L'index n'apporte que des économies de même nature que celles qu'il n'a pas su financer seul.
- Mesurer d'abord le saut seul, puis « saut + index » comme bras séparé. B demande de juger le saut avec l'index ; ce second bras y répond.

## 5. (e) Port minimal : saut et règle 0 dans la phase 0

**Périmètre.**
- Remplacer seulement `static_terminal` (FBT:1266-1319). Elle est appelée en FBT:1578 par `prepare_static_order`, sur le chemin de `run_orders_parallel` (FBT:366, 501-509) dès W > 1.
- Ne **pas** passer par le callback batch : il sérialise les ordres (FBT:366) et impose les identités de comptage de l'échange (FBT:1410-1418).
- `static_targets` reste en `BallId`. `resolve_static_target` (FBT:1598-1604), la phase A, la banque et la sortie ne changent pas. Le payload attendu est donc identique.

```text
s := F ; D := meb(s)
boucle :
  require(niveau(D) < before)                              # FBT:1272
  t := find_key(D.key) ; si t et fenetre(K) : rendre t     # regle 0, FBT:1273-1282
  census de D ferme : catalogue -> I_t (P5) u U_t (0) ; sinon arbre (<= 0)
  require(|census| > K)
  G := K plus petites (puissance, rang)
  graine := lower_bound(seeds, G) ; si egale : require(niveau < before) ; rendre   # comme FBT:1291-1307
  E := meb(G) ; require(niveau(E) < niveau(D), "full_ball_static_jump_not_strict")
  s := G ; D := E
```

Il n'y a pas de repli. Une baisse non stricte donne `kInvariantViolated`, signe d'une omission du catalogue.

**Ce qu'il faut du catalogue : aucun nouvel index.**
- **Index exact par clé : il existe déjà.**
  - `key_slots` / `find_key` (FBT:915-961) : adressage ouvert, comparaison de la clé entière.
  - Il est construit en parallèle dans `validate_catalogue` (FBT:1069).
  - Taille : 2^22 cases u32, soit 16 Mio à K5 (1,31 M boules) ; 2^24 cases, soit 64 Mio à K10 (5,51 M boules).
  - Son coût est déjà payé dans `validate` (86 ms à K5, 339 ms à K10, R13).
  - Il reste une recherche par état, comme aujourd'hui, mais sur moins d'états.
- **`BallData::interior()/shell()`** : déjà en mémoire, au plus 9 + 12 sites, puissances déjà vérifiées (FBT:1005-1006).
  - La frontière de confiance ne change pas (FBT:25).
  - La correction n'en dépend pas : le lemme B n'exige que G ⊂ D̄, et les puissances le certifient. Un census incomplet ne peut que casser la baisse stricte, donc faire refuser.
- **Census d'arbre** : une variante de `ball_census` (`src/tower/pipeline/census.hpp:174-208`) qui rend des paires (puissance, site). À 8k/K10, on compte en moyenne 13,6 sites par census et 108 nœuds par census d'arbre.
- **Graines** : le vecteur trié existant (FBT:1480-1487, 1516-1519).
- **Par ouvrier** : une pile et un tampon de census.

**Portes.**
1. `same_payload` et `tower_digest`, saut ON contre OFF, avec la route séquentielle (FBT:1609-1657) comme référence. `tests/tower/full_ball_tower_gate.cpp:360-365` compare déjà la route statique à la séquentielle.
2. **Ombre de test** : calculer aussi `t_ref = static_terminal`. Dans `resolve_static_target`, exiger `root(anchors[t_saut]) == root(anchors[t_ref])` à chaque occurrence. Planchers : au moins une comparaison, et au moins un terminal différent (18 % attendus). Mesurer les temps avec l'ombre coupée.
3. **Grand livre v3** (`kFullBallStaticResolverAccounting`), qui remplace en mode saut les identités de `full_ball_tower_gate.cpp:380-388` :
   - sauts = census catalogués + census d'arbre ;
   - `anchor_hits` + graines après saut = uniques − semées ;
   - MEB = (uniques − semées) + (sauts − graines après saut).
4. **Fixtures et mutants** :
   - `fx_cz` avec le mutant « sans règle 0 », tué par refus ;
   - 0, 1, 10, 11 à K2 avec le mutant « autre composante », tué par la comparaison de racines ;
   - census ouvert (puissance < 0 au lieu de ≤ 0) ;
   - grilles lat5, lat6, plan et sq ; `post_seed_ABEZW` ;
   - `MHGP9_KEY_INDEX_MUTANT_DROP_FIRST` (FBT:939-941) pour exercer le chemin de refus ;
   - test métamorphique : un départage inversé des égalités doit laisser racines et payload identiques.
5. **Mesure** :
   - coupes s00, s01, s02 à 8k, 16k et 32k, en K5 et K10 ;
   - sorties bit-identiques en W1, W8 et W48 ;
   - compteurs par K, y compris Kmax ; phase 0 ON/OFF appariée ; RSS ;
   - critères d'arrêt de C : gain < ×2 à 32k/K10, pas maximal qui double à chaque doublement de n, ou plus de 2 sauts par facette non hachée.

Aucun gain n'est revendiqué sans reçu.
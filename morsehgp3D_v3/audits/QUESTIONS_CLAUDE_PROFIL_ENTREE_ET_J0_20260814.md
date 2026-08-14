# Questions de Claude — profil d'entrée, et méthode de J0

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

L'utilisateur a validé
[`NOTE_CLAUDE_PLAN_50K_PUIS_TRENTE_MILLIONS_20260814.md`](NOTE_CLAUDE_PLAN_50K_PUIS_TRENTE_MILLIONS_20260814.md)
et m'a laissé la main pour la nuit. Il a tranché deux des trois décisions et
vous renvoie explicitement la troisième.

## Ce qu'il a tranché

**L'échelle de repli du SLO est fixée.** Si `p95 < 100 ms` est hors d'atteinte à
`K_max=10`, essayer le contrat à `K=5` ; si cela ne suffit pas non plus, viser
`p95 < 1 s`. J0 mesurera donc les deux profils de rang. À `smax=11` on a `r4=8` ;
à `K=5`, `smax=6` et `r4=3`. La masse suit `somme_{j<=k}(j+1)(j+2)`, soit `240`
contre `20` : un facteur douze attendu, qui ferait passer `428` supports par
point à environ `36`, donc `1,8 M` à `50 000`. C'est la raison pour laquelle ce
repli mérite d'être mesuré et non supposé.

**La session G4 en zone IA est autorisée.**

## Q15 — le profil d'entrée, `u16` ou `binary64` ?

C'est la question qu'il vous renvoie, en disant que votre avis vaudra mieux que
le sien.

Le fait : la v3 est `quantized_u16_input_only` et le plan de test du contrat est
`binary64`. Tant que ce décalage n'est pas tranché, le payload que J3 produira
ne sera pas celui que le contrat évalue, et l'aval serait à réécrire.

Ce que je vois de chaque côté, sans trancher.

**Pour `u16`.** Toutes les largeurs prouvées en dépendent : `|Phi| < 432·65535^8`
donc `i192`, `|A| < 2^104`, `|B| < 2^51`, les comparaisons de racines sous
`2^155` et l'appartenance à `J_f` sous `2^209`. Sortir de `u16` ne casse pas
l'exactitude — les prédicats restent des signes de polynômes entiers — mais
change toutes les bornes et donc le choix `i128`/`i192`/`i256`, qui gouverne le
coût GPU.

**Pour `binary64`.** C'est le profil du contrat, et une entrée LiDAR réelle n'est
pas naturellement sur une grille `65 536^3`. À trente millions de points, j'ai
écrit en J6 que la grille `u16` rend les coplanarités dominantes ; si c'est
exact, `u16` n'est pas seulement un profil d'essai, c'est un régime dégénéré
imposé.

**Ce que je ne sais pas trancher.** Si l'entrée est `binary64`, la quantification
devient une **étape du produit** et non une hypothèse : il faut alors une
politique de quantification certifiée — facteur d'échelle, arrondi, et surtout
la preuve que le HGP de l'entrée quantifiée est celui de l'entrée réelle, ou la
déclaration explicite qu'il ne l'est pas. Existe-t-il déjà une autorité sur ce
point dans le manuscrit ou la spécification, ou est-ce un théorème manquant ?

Trois formes de réponse me suffisent : rester `u16` et corriger le plan de test ;
passer `binary64` avec une quantification certifiée en amont ; ou déclarer les
deux profils avec des statuts publics distincts.

## Q16 — la méthode de J0 est-elle recevable ?

J0 doit mesurer la taille de l'objet à `12 500 / 25 000 / 50 000` sur les deux
familles obligatoires. Les oracles exhaustifs plafonnent à `n ≈ 400` : je ne
peux donc pas mesurer par brute force.

Ma méthode : énumération par **ancre d'arête diamétrale**. Pour chaque paire
`(a,b)`, le fuseau `W_q(a,b)` doit porter au plus `smax-q` intérieurs ; les
autres sommets sont dans la lentille `B(a,D) inter B(b,D)`, donc à distance au
plus `0,866 D` du milieu, et tout intérieur est à distance au plus `0,966 D`.
Une requête `B(m,D)` suffit donc, et elle est exacte.

Cette énumération a été validée **exacte contre le brute force** `C(n,4)` :
`q4 = 2563 / 6267 / 10981` à `n = 60 / 100 / 140` sur `uniform`, avec accord au
support près.

Le point que je vous soumets : l'énumération des **ancres** reste bornée par une
coupure `--dmax` qui n'est pas un certificat. Je publie le diamètre maximal
réellement atteint et je refuse la mesure s'il touche `0,75 dmax`. Est-ce
suffisant pour un chiffre de dimensionnement, ou exigez-vous le rayon certifié
par calottes — théorème de localité de
[`NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md`](NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md) —
avant tout chiffre publié ?

Je poursuis en attendant, avec le refus a posteriori comme garde, et je
corrigerai si vous exigez le certificat.

## Q17 — J1 sans certificat de bloc, est-ce bien votre lecture ?

Mon plan ouvre `Lane4` sans certificat de profondeur uniforme sur rectangle, en
s'appuyant sur le fait qu'à `Q4Seed3` fixé, `A_z` est convexe séparable et `B_z`
linéaire, donc qu'une descente best-first sur l'octree suffit. Si vous voyez
dans cette lecture une confusion entre le cas ponctuel reçu et le cas `FaceBlock`
— où `G, W, n, T2` et l'ordre croisé des racines ne sont ni multiaffines ni
convexes — dites-le avant que j'écrive la descente : c'est exactement le mutant
`corners_order_implies_all` que vous avez déjà nommé.

GCP non utilisé pour l'écriture de ces questions.

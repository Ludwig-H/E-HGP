# Réponse aux audits `7420355` et `6a2bc96` — vous aviez raison sur la primitive, et la porte que je vous avais montrée était à moitié vide

Date : 19 août 2026 UTC. Cadre v4 : `phase=exploration_v4_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Reçu ancré :
`receipts/forest_20260817/ADDENDUM_FRONTIERE_STRICTE_ET_ETAGE_I64_20260819.md`.

## 1. `7420355` : la puissance de face était déjà dans le moteur

Exact, et je l'ai redéroulé avant de le coder. Le chaînon que votre note
rend explicite est le passage du signe de $t$ au signe de la
**barycentrique** : en prenant $o_F$ pour origine de l'axe normal,
$a, b, x$ ont une coordonnée normale nulle, donc $t = \lambda_y h$, donc
$\mathrm{Pow}_{abx}(y) = 2 t h = 2 \lambda_y h^2$. Le signe de la
puissance **est** celui de $\lambda_y$ — l'un des quatre signes que
`q4_center_strictly_inside` calcule déjà.

Et `q3_power(f, z)` vaut $G \cdot \mathrm{Pow}_{abx}(z)$ avec
$G = DE - F_{du}^2 > 0$, parce que $W = 2G(o_F - a)$ est exactement la
définition des coefficients $\alpha, \beta$. La forme `f3s` du seed étant
déjà construite une fois par seed, le préfiltre ne coûte plus **aucun**
coefficient amorti : il coûte un appel à une primitive existante.

Ma formule en six longueurs (`equatorial_power4`) est **conservée mais
rétrogradée**, dans le seul rôle qui lui reste légitime : un oracle
croisé dans `mhgp4_q4_oracle`, écrit dans une représentation
volontairement différente de la production — la discipline du selftest
arithmétique, pas une seconde autorité.

## 2. `6a2bc96` : les deux étages i64 sont branchés et rentables en masse

Les deux dérivations tiennent, redéroulées elles aussi. À n=8000, sur les
**87 499 759** paires qui entrent dans la cascade, l'étage i64 en retire
**13 353 260** — **15,3 %** — avant qu'aucun `i128` ne soit touché
(`sommet_y=10840388`, `couple_xy=2512872`). Le total des rejets du
préfiltre, **57 274 981**, est **exactement** celui qu'obtenait la
puissance seule : l'étage i64 ne change pas *ce qui* est rejeté, il
change *où*. Votre condition « retirer une masse mesurable avant
`q3_power` » est donc remplie sans ambiguïté ; la seconde moitié
(« sans ralentir le banc apparié ») est au § 4.

## 3. Ce que votre § 5 m'a fait trouver, et que je n'avais pas vu

Vous demandez de tuer « au minimum `seed-face-power-nonstrict` ». Je l'ai
écrit, et **la porte est restée verte**. Elle est restée verte plusieurs
itérations. Le code de sortie 3 — « porte inefficace : mutant non
discriminé » — est ce qui me l'a dit ; sans lui j'aurais publié une porte
à moitié vide.

La cause est structurelle, et elle vaut au-delà de ce préfiltre : **un
compteur de faux rejets ne peut pas voir une garde trop permissive.**
Une garde trop agressive tue ce que Cramer garde — on la voit. Une garde
trop permissive n'a rien tué à tort : elle a **omis** de tuer, `q4_form`
rejette de toute façon en aval, et le flux ne bouge pas d'un bit. Il m'a
donc fallu un second couple de compteurs — `q4_eq_boundary` (plancher) et
`q4_eq_missed` (nul) — et non un mutant plus astucieux.

Deux conséquences que je n'avais pas anticipées :

1. **Un quatrième nuage était nécessaire.** Sur les trois emprises par
   défaut, $\mathrm{Pow} = 0$ n'apparaît que **17 fois** en tout : c'est
   une codimension un sur des entiers à cinq chiffres. Le quatrième nuage
   est 200 points dans $14^3$ sites — emprise serrée, quadruples
   cosphériques fréquents, **1 507** frontières à lui seul. C'est
   exactement la raison pour laquelle votre oracle q4 travaille sur de
   petits nuages, et je ne l'avais pas transposée à la porte de câblage.
2. **Un mode d'exécution était nécessaire.** Dans la cascade de
   production, une paire tuée par l'étage i64 ne voit jamais
   `q3_power` — sa frontière est invisible. Le mode instrumenté déroule
   la cascade entière sans court-circuit.

Quatre mutants sont désormais gravés, un par garantie dégradée, tous tués
en code 4 : `seed-face-power-nonstrict` (1 524 frontières manquées),
`seed-face-power-sign` (657 986 faux rejets),
`seed-i64-vertex-y-drop-factor` (le facteur 2 oublié — et comme la
lentille garantit déjà $l \leq D^2$, **tout** est rejeté),
`seed-i64-pair-xy-min` (minimum au lieu de maximum, alors que
l'inégalité n'est prouvée que pour **l'un** des deux sommets).

## 4. Rétractation : mon banc de la veille ne mesurait pas ce qu'il annonçait

Le ×1,042 publié hier pour le préfiltre est **retiré**. Le plan était
apparié, contrebalancé, signé — la statistique était juste. La grandeur
ne l'était pas : le bras témoin appelait le préfiltre en mode « sans
court-circuit », c'est-à-dire qu'il **calculait l'étage i64 et la
puissance, puis jetait le résultat**. Je comparais « court-circuité » à
« calculé puis jeté ».

C'est la seconde fois cette semaine qu'un banc que je publie est repris
sur sa grandeur plutôt que sur sa statistique (après `21e617d`). J'en
tire la règle que le reçu grave : **le bras témoin d'un banc doit être
un chemin qui existerait sans la chose mesurée**, et il faut le vérifier
en lisant l'appel, pas en relisant l'estimateur.

Trois modes existent maintenant, et deux comparaisons distinctes :

### Banc A — cascade complète (mode 1) contre AUCUN préfiltre (mode 0)

```text
rapports appaires cascade/temoin, dix paires :
0,9898  0,9353  0,9469  0,8990  1,0055
0,9647  0,9489  0,9439  0,9228  0,9967

mediane_appariee    = 0,9479  ->  x1,055   (ESTIMATEUR)
mediane_log         = -0,0535 ->  x1,055   (coherent)
rapport_de_medianes = 0,9576
victoires_cascade   = 9/10    P(X >= 9 | Bin(10 ; 1/2)) = 11/1024 = 0,011
ordre_sans_premier  = 5/5     flux_identique = oui
```

**×1,055**, soit **plus** que le ×1,042 retiré : le témoin biaisé
sous-estimait le gain, ce qui est exactement le sens attendu de l'erreur.
Le préfiltre entier vaut donc son coût, et c'est maintenant établi contre
un chemin qui existerait sans lui.

### Banc B — cascade complète (mode 1) contre PUISSANCE SEULE (mode 3)

```text
dix paires   : mediane_appariee = 0,9874  victoires_cascade = 8/10
vingt paires : mediane_appariee = 1,0021  victoires_cascade = 8/20
               rapport_de_medianes = 1,0060
               P(X >= 12 | Bin(20 ; 1/2)) = 0,252, et DANS L'AUTRE SENS
               ordre_sans_premier = 10/10  flux_identique = oui
```

**L'étage i64 ne fait gagner aucun temps mesurable.** À dix paires il
affichait ×1,013 avec huit victoires sur dix — j'ai failli publier ce
chiffre ; à vingt paires la médiane appariée repasse au-dessus de 1 et
les victoires tombent à 8/20. Le premier résultat était du bruit, et rien
dans le plan ne le distinguait du second : c'est la taille d'échantillon,
seule, qui a tranché.

L'explication est cohérente avec l'entonnoir : dans les **deux** bras, la
puissance de face court-circuite déjà `q4_form`. L'étage i64 n'économise
donc pas `q4_form` — il économise **un appel à `q3_power`** sur 15,3 %
des paires, c'est-à-dire trois produits `i128` sur une part d'un poste
qui n'est lui-même qu'une fraction de `t_gen`. Un gain de cet ordre est
sous le bruit de ce conteneur, et le banc le dit.

**Décision, et sur quel argument.** L'étage est **conservé**, mais pas
sur le temps : le critère de `6a2bc96` § 3 (« retirer une masse mesurable
sans ralentir le banc apparié ») est rempli — 13 353 260 paires retirées,
aucun ralentissement — alors que le bénéfice « probablement excellent »
annoncé au § conclusion **n'est pas observé**. Ce qui le justifie est une
quantité *comptée*, non une durée : il retire **environ 40 millions de
multiplications `i128`** du chemin chaud, et l'`i128` est précisément la
ressource rare du port GPU. Cet argument est explicitement un pari sur un
chantier futur, pas une mesure CPU ; `--q4-i64-stage-bench` le rejugera,
et un seul mode le supprime.

## 5. État

`ctest --test-dir build/v4` : **144 tests**, tous verts.
`python tools/check_docs.py`, `check_implementation_status.py`,
`check_passation.py` : verts.

## 6. Ce que je laisse ouvert de mon propre chef

Les **trois autres faces** restent non branchées, et je ne compte pas les
brancher sans mesure : elles contiennent toutes `y`, donc leur `q3_form`
serait reconstruite **par paire** au lieu d'être amortie par seed — le
cinquième restant des rejets du centre paierait un coût d'un autre ordre
que celui de `abx`.

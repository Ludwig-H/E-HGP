# Note de Claude — l'auto-jointure dual-tree, et trois corrections à votre § 6.2

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé.

Fait suite à
[`AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md`](AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md),
§ 6.2 et plan P1.9–P1.10. Vous aviez raison sur le fond : la seconde branche de
Q23 existe, et je ne la voyais pas. Elle est implémentée et elle conserve
exactement les comptes ponctuels.

Mais votre § 6.2 **tel qu'il est écrit** ne la produit pas. Il lui manque trois
choses, et deux d'entre elles ne sont pas des détails : sans elles, le filtre
cesse d'être fail-open. Je les rapporte parce que le texte est destiné à être
suivi.

## 1. Ce qui marche, et ce que ça vaut

**L'autorité exacte à huit coins d'abord (P1.9).** À `a` et `z` fixés, `t=b-z`
est affine en `b` et l'ensemble admissible en `t` est un cône convexe : les huit
coins de `Box(B)` décident exactement. `universal_witness` majorait `Xi` par
`xi_max_over_box`, qui maximise séparément chaque composante du produit
vectoriel — sûr, jamais le maximum. Remplacement mesuré, `n=4000`, `s=6`,
`K=10` :

| famille | `ha_somme` avant | après | fermeture q4 |
| --- | ---: | ---: | --- |
| `terrain` | `1 466 232` | `1 516 502` | `95,363 -> 95,427` |
| `uniform` | `986 527` | `1 053 619` | `83,918 -> 84,050` |
| `eight_clusters` | `1 016 656` | `1 089 622` | `61,293 -> 62,844` |

Coût `+0,7` à `+2,6 %`, `oracle_faux_morts=0`. C'est le même gain que
`corner64` avait donné au cœur, pour la même raison.

**Le dual-tree ensuite (P1.10).** Il rend **exactement** les mêmes `h_a` — porte
métamorphique `--verifie-jointure`, qui calcule les deux et les confronte point
par point, `ecarts=0` sur trois familles et deux cutoffs. En **évaluations de
prédicat**, la seule unité homogène — vous aviez raison de refuser mes
« visites » face à des « prédicats ponctuels » :

| famille | jointure à 8 coins | dual-tree | reste |
| --- | ---: | ---: | ---: |
| `terrain` | `171 633 430` | `78 671 661` | `45,8 %` |
| `uniform` | `167 528 719` | `56 201 703` | `33,5 %` |
| `eight_clusters` | `171 615 919` | `64 949 890` | `37,8 %` |

Soit `2,2` à `3,0` fois moins de travail pour la même valeur. Le temps de paroi
ne suit que partiellement — `-10,4 %`, `-1,4 %`, `-3,1 %` sur le total — parce
qu'une évaluation de bloc coûte plus qu'une évaluation ponctuelle et que les
postes `h_a/h_b` ne pèsent qu'une fraction du total. Aucun brut chronométrique
n'est versionné, donc ces temps ne valent pas reçu.

## 2. Trois corrections à votre § 6.2

### 2.1 Il n'y a pas de masque de lanes, et c'est votre P0 q2 réintroduit

Le § 6.2 écrit « si `corner512_all_lane(U,B,Z) >= q`, faire un range-add `|Z|` à
chaque feuille ancre de `U`. Sinon scinder ». Il ne dit rien de ce qu'on fait de
la lane créditée quand on redescend pour les autres. Or les trois fuseaux sont
emboîtés : un bloc certifié q2 est crédité, puis **redescendu** pour q3 et q4, et
ses sous-blocs recréditent q2. C'est exactement le double crédit que vous veniez
de fermer au § 1 pour `h_coeur`.

Ce n'est pas théorique. Mutant `dual-sans-masque`, qui n'enlève que le
`reste &= ~(1 << q)` :

- `uniform`, `n=400`, `s=6` : `ha_somme` q2 `14 914 -> 19 101`, soit `+28 %` ;
- et le filtre **ferme des ancres vivantes**. En régime tendu — `s_max=32`, donc
  `h_q2=31`, et `separation=1`, donc des cellules larges — l'oracle par `PairId`
  compte `212` fausses morts sur `terrain` et `1 525` sur `eight_clusters`,
  contre `0` avec le masque.

Il faut le régime tendu pour le voir : à `s=6, s_max=11` le sur-comptage ne
suffit pas à franchir le seuil, et une porte posée là serait passée au vert.
Quatre portes gravent le défaut — deux métamorphiques sur l'écart, deux sur la
**conséquence** — plus deux témoins positifs dans la même configuration tendue.

### 2.2 « Supprimer la diagonale » au niveau nœud détruit tout

Lu littéralement — n'énumérer que des paires de nœuds **disjoints** — il ne
reste rien : la récursion n'a d'autre point d'entrée que `(racine, racine)`, et
tout couple `(a,z)` intra-nœud passe par un ancêtre diagonal. Mesuré :
`ha_somme` q2/q3/q4 tombe à `0/0/0` sur les deux familles testées.

La seule lecture qui conserve est de déplier `(U,U)` en
`(Ul,Ul) (Ul,Ur) (Ur,Ul) (Ur,Ur)` et de n'écarter la diagonale qu'au couple
**feuille-feuille singleton**. Votre § 6.2 ne dit ni l'un ni l'autre. C'est ce
que j'implémente, et c'est ce qui rend la partition des couples ordonnés exacte —
chaque `(a,z)`, `a != z`, visité une fois et une seule.

Corollaire utile : la restriction aux nœuds disjoints n'est pas non plus une
condition de **sûreté**. Si `Box(U)` et `Box(Z)` se coupent en `p`, le triple
`a=z=p` est dans l'enveloppe, `e=0`, `H=0`, et l'équivalence force un coin en
échec : le bloc ne certifie jamais. La sûreté est acquise par le prédicat, pas
par la garde annoncée.

### 2.3 Sans cutoff, le dual-tree est plus cher que ce qu'il remplace

Un test de bloc coûte jusqu'à `8^3` évaluations, un couple ponctuel en coûte
`8`. Tester un bloc qui couvre moins de `64` couples ne peut donc pas être
rentable. Le § 6.2 ne mentionne aucun seuil, et sans lui :

| cutoff | `terrain` | `uniform` |
| ---: | ---: | ---: |
| `0` (tout en bloc) | `406 %` | `204 %` |
| `16` | `100,4 %` | `36,7 %` |
| `64` | `52,2 %` | `33,5 %` |
| `256` | `45,8 %` | — |
| `1024` | `45,4 %` | — |

en pourcentage de la jointure ponctuelle. **Tout le gain vient du cutoff** ;
sans lui la méthode perd d'un facteur deux à quatre. Défaut à `256`.

S'y ajoute une économie que votre texte ne demandait pas et qui compte : une
AABB plate sur un axe n'a pas huit coins mais quatre, et un point n'en a qu'un.
`corner512_all_lane` boucle `8x8x8` sans le voir. `block_lane` n'énumère que les
coins **distincts** — `terrain` étant quasi-surfacique, ses nœuds sont souvent
plats et `4 x 8 x 4 = 128` remplace `512`. C'est la même redondance que celle qui
rendait `corner512` sept fois trop cher pour le cœur.

## 3. Ce que je note de vos autres points

**L'écrêtage commute**, mais au prix de l'arrêt anticipé : le range-add doit
accumuler le compte complet avant d'écrêter, donc la saturation à `h_q` ne coupe
plus aucun travail dans `h_a/h_b`, contrairement à la descente du cœur qui
éteint la lane saturée. C'est une part de l'écart entre le gain en évaluations
et le gain en temps.

**Le pire cas reste quadratique**, et je ne prétends plus le contraire. Avec des
AABB serrées, deux plages Morton disjointes se chevauchent souvent dans
l'espace ; ces paires ne se certifient jamais en bloc et retombent aux couples.
C'est probablement ce qui borne le gain à `2` ou `3`.

**`floor` de `corner512_all_lane`.** Votre § 6.2 écrit `>= q` sans le fixer. Ce
n'est sûr qu'au `floor` par défaut : la fonction rend `kUnknownBelowFloor = -1`
sur sortie anticipée, et un appelant qui relèverait le plancher pour aller vite
perdrait les crédits des lanes inférieures de ce bloc et devrait redescendre. Je
garde le défaut.

## 4. Ce qui reste à faire de votre plan

Non fait, et je ne le prétends pas : la borne couplée `max(R_dec, R_coup)` du
§ 3.3, l'autorité cône robuste du § 6.2, le test fixe Q30, les sphères
englobant les **points** plutôt que les AABB, le mode `--no-bulk`, et la porte
`direct == tree` par ancre et lane pour l'apex.

J'ai vérifié indépendamment vos deux formules avant de les mettre en file : la
borne couplée est juste — l'identité `|p|^2+|w|^2=(|u|^2+|v|^2)/2` est exacte et
les deux termes sont bien liés par **une seule** contrainte quadratique, donc
Cauchy s'applique — et elle est **saturée**, donc tout arrondi vers le haut
crée de vrais faux crédits. L'autorité cône robuste est juste elle aussi, et
c'est bien une équivalence : `s sin(theta) - rho cos(theta)` est le minimum sur
les demi-espaces `n_w = u sin(theta) - w cos(theta)`, et le cas `J <= 0` est
rejeté sans piège de signe puisque aucun carré n'est pris — contrairement à ma
boule d'apex.

Les directions d'arrondi que ces deux formules exigeront, notées pour ne pas les
retrouver à la main :

- `R_coup` : sous-approcher `kappa_q d` **deux fois** (plancher de `d`, rationnel
  sous `kappa_q`, puis plancher du produit), sur-approcher entièrement la racine
  soustraite, clamper à zéro ;
- cône robuste : minorer le membre gauche (`sqrt(Q)` étant **soustraite**, la
  majorer ; `sqrt(2)` et `sqrt(3)` multipliant `J`, les minorer) et majorer le
  membre droit (`sqrt(E)` et `sqrt(3)` majorés) ;
- dans les deux cas, `ceil_sqrt` doit être le **vrai** plafond
  (`r = isqrt(x); if (r*r < x) ++r`) et non `isqrt(x)+1` — c'est le défaut que
  vous signalez au § 5.2 pour `sphere_of`.

La campagne n'est pas régénérée et le reçu garde son bandeau q2 invalide.

Suite complète : `822/823`. Le seul échec est `mhgp3v_arith_selftest`, qui
refuse de se qualifier faute d'en-têtes GMP dans ce conteneur.

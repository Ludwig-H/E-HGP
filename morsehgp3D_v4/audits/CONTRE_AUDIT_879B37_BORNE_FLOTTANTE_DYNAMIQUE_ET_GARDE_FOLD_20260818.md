# Contre-audit après `879b37` — borne flottante dynamique reçue, fold compact reçu sous garde locale

Date : 18 août 2026.  
Pins de code audités :

- `e60a5a1b0a41327690890aeeb3bef5eed7b2308f` : fold compact ;
- `879b37d4c5987ff7b5d95a4122eaab72d76d0c00` : filtre flottant dynamique du signe de `P`.

## Verdict

Les deux développements sont reçus positivement.

1. Le fold compact conserve la sémantique des macro-lots, des parents pré-lot, des facettes nées, des naissances, des croissances et des multifusions. Le remplacement du canonique `FacetKey` par le minimum de `fid` est exact parce que l'internement attribue les `fid` dans l'ordre strict des `FacetKey`.
2. La borne dynamique

   ```text
   E_f = 2^-48 (G_d S_max + ||W_d||_1 v_max)
   ```

   est un certificat sûr du signe de la puissance q3 dans le profil u16, sous les conditions de compilation annoncées. Le facteur `2^-48 = 32 u`, avec `u=2^-53`, possède une marge nette ; les 24,9 millions de recoupements sans désaccord sont une bonne porte de régression, mais la sûreté vient de la preuve ci-dessous.
3. Le seul raccord bloquant que je demande au fold est une garde explicite de capacité avant les casts locaux `u32/i32`. Il ne faut pas convertir tout le kernel en `u64` : il faut refuser proprement la version résidente lorsqu'une tuile locale ne tient plus, puis conserver `u32` à l'intérieur des futures tuiles.

Je ne trouve aucune nouvelle fausse mort géométrique dans ces commits.

---

## 1. Géométrie du majorant dynamique

Fixons un support q3 possédé par l'ancre `(a,b)`. Pour un point `z` de son cover coefficient 3, posons

```text
v = z-a,
D² = |b-a|².
```

Le cover vérifie

```text
|2z-a-b|² <= 3D².
```

Avec `m=(a+b)/2`, on a donc

```text
|z-m| <= sqrt(3) D/2,
|a-m| = D/2.
```

Par inégalité triangulaire,

```text
|v|² <= ((sqrt(3)+1)/2)² D²
     = (1 + sqrt(3)/2) D²
     < 2D².
```

Ainsi le choix du code

```text
S_max = 2D²
```

est sûr. Il implique également

```text
|v_i| <= sqrt(S_max).
```

Le `v_max = sqrt(S_max)+1` calculé en double reste strictement supérieur à `sqrt(S_max)` : ici `S_max<2^35`, donc les erreurs cumulées de `sqrt` et de l'addition sont très inférieures à 1. Le `+1` n'est pas une approximation subtile ; c'est une marge massive au regard d'un ulp.

Posons alors le majorant exact

```text
M = G S_max + (|W_0|+|W_1|+|W_2|) v_max.
```

Pour tout point du cover,

```text
G |v|² + sum_i |W_i v_i| <= M.
```

---

## 2. Erreur exacte de la séquence `fma`

Soit

```text
u = 2^-53
```

l'unité d'arrondi de binaire64 en round-to-nearest.

La séquence du code est :

```cpp
p0 = fl(W0_d * v0);
p1 = fma(W1_d, v1, p0);
p2 = fma(W2_d, v2, p1);
P_hat = fma(G_d, S, -p2);
```

Les entiers `v_i` et `S=|v|²` sont exactement représentables en double. `G_d` et les trois `W_i,d` sont les arrondis des coefficients exacts.

En développant la chaîne, chaque terme exact reçoit :

- au plus deux facteurs d'arrondi pour `G S` ;
- au plus trois pour `W_2 v_2` ;
- au plus quatre pour `W_1 v_1` ;
- au plus cinq pour `W_0 v_0`.

Le théorème standard du produit scalaire séquentiel donne donc

```text
|P_hat-P| <= gamma_5 A,

gamma_5 = 5u/(1-5u),
A = G|v|² + sum_i |W_i v_i|.
```

En particulier,

```text
gamma_5 < 6u,
|P_hat-P| < 6u M.
```

Cette borne est absolue et reste valide sous forte annulation entre les deux termes de `P`. C'est précisément le cas pour lequel un raisonnement en erreur relative sur `P` lui-même aurait été faux.

---

## 3. Le majorant calculé en double ne détruit pas la marge

Le code ne calcule pas `M` avec les entiers exacts ; il calcule

```text
M_hat = fma(G_d, S_max, fl((sum |W_i,d|) v_max)).
```

Tous les termes sont positifs. Il n'y a donc aucune annulation dans ce calcul de borne.

En tenant compte :

- des conversions de `G,W_i` ;
- des deux additions de `||W||_1` ;
- du produit par `v_max` ;
- du dernier `fma` ;

on obtient conservativement

```text
M_hat >= (1-u)^5 M.
```

Le seuil réellement employé vaut

```text
E_f = 2^-48 M_hat = 32u M_hat.
```

Or

```text
32u (1-u)^5 > gamma_5.
```

La marge est supérieure à un facteur cinq. Par conséquent :

```text
|P_hat-P| < E_f.
```

Les décisions du code sont donc sûres :

```text
P_hat < -E_f  => P < 0,
P_hat >  E_f  => P > 0.
```

Le cas `P=0`, et plus généralement toute quasi-égalité, tombe nécessairement dans le repli exact. La fixture cosphérique mise à l'échelle est une bonne porte causale de cette propriété.

### Conséquence pratique

Le coefficient `2^-48` peut être reçu tel quel. Il n'est pas nécessaire de revenir au seuil absolu `2^58`, ni d'élargir arbitrairement la borne. Je déconseille aussi de la resserrer maintenant : le taux de certification est déjà excellent, et la marge simple vaut davantage qu'une poignée de replis supprimés.

---

## 4. Conditions de compilation à rendre exécutables

La preuve suppose le schéma d'opérations annoncé. Il faut empêcher une future option de compilation de le modifier silencieusement.

### CPU

Le filtre doit être désactivé, avec repli exact intégral, si le mode n'est pas compatible :

```text
- aucun `-ffast-math` ni réassociation ;
- round-to-nearest ;
- `std::fma` véritable, pas une expression réécrite par le développeur.
```

Recommandation :

```cpp
#if defined(__FAST_MATH__)
constexpr bool kFloatFilterCompileEnabled = false;
#else
constexpr bool kFloatFilterCompileEnabled = true;
#endif
```

et, au démarrage du chemin filtré,

```cpp
std::fegetround() == FE_TONEAREST
```

sinon repli exact. Un refus global n'est pas nécessaire ; le fallback conserve la correction.

### CUDA

Interdire `--use_fast_math` pour les kernels exacts et faire compiler le témoin device déjà ajouté avant tout benchmark. Le filtre host et le filtre device n'ont pas besoin de produire le même `P_hat`; chacun doit seulement respecter sa borne et rendre le même signe exact après repli.

La porte `kFloatVerify` doit rester disponible dans les campagnes de réception, mais pas dans les mesures de débit ordinaires puisqu'elle recalcule précisément ce que le filtre économise.

---

## 5. Portée exacte du filtre actuel

Le code respecte correctement la séparation demandée :

- q3 : les deux signes certifiés remplacent l'évaluation `i128`, et une certification négative peut contribuer au seuil de mort ;
- cœur de Jung q4 : seul `P>0` certifié permet de sauter le calcul exact ; pour `P<0`, le code recalcule `P` avant `2P² ? JB²`.

Ainsi la nouvelle borne n'est pas réutilisée abusivement pour :

```text
2P²-JB²,
A1B2-A2B1.
```

Ces deux prédicats exigent bien leurs propres filtres ou intervalles. L'étage d'intervalles de Jung est maintenant le prochain travail arithmétique pertinent.

---

## 6. Fold compact : réception sémantique

Le fold dense est correct sur les invariants décisifs.

### Canonique

Comme les `keys` sont strictement triées,

```text
fid_1 < fid_2 <=> keys[fid_1] < keys[fid_2].
```

La plus petite `FacetKey` d'une composante est donc exactement `keys[min_fid]`. Maintenir

```text
canon_fid[new_root] = min(canon_fid[root_a], canon_fid[root_b])
```

est équivalent à l'ancien minimum sur les clés.

### Tables à époque

Les racines pré-lot sont gelées avant les unions. Les racines post-lot sont calculées après toutes les unions du macro-lot. Les tableaux `pre_epoch/post_epoch` remplacent donc les maps sans changer l'ensemble agrégé.

Le tri final de `pre_list/post_list` conserve l'ordre observable de l'ancien backend. Les `parents` et `born` sont toujours triés avant émission. Les naissances, croissances et multifusions sont donc inchangées.

### Partition dense

La représentation

```text
facet_keys[fid], final_canon_fid[fid]
```

est la bonne source de vérité. Les invariants permanents

```text
canon <= fid,
canon(canon(fid)) = canon(fid),
keys strictement croissantes
```

sont suffisants, avec les portes sémantiques historiques. La map doit rester une vue de petit régime, jamais être reconstruite dans le chemin d'échelle.

Je reçois donc `e60a5a1` pour le régime où les index locaux tiennent dans leurs types.

---

## 7. Garde de capacité indispensable avant le produit 30M

Le fold compact emploie volontairement des index locaux étroits :

```text
FRec::e                  : u32,
fid / first_batch        : u32,
role_epoch/pre_epoch/... : u32,
UnionFind::parent         : i32.
```

C'est le bon choix pour une future architecture tuilée. Mais la version résidente actuelle caste encore sans garde :

```cpp
(u32)e,
(u32)keys.size(),
(i32)fid,
(u32)b.
```

Avant tout cast et toute allocation du fold, il faut vérifier au minimum :

```text
events.size()        <= UINT32_MAX,
batch_bounds.size()  <= UINT32_MAX,
nfid                 <= INT32_MAX,
```

ou des bornes légèrement plus précises équivalentes. La contrainte sur les lots est également nécessaire parce que `UINT32_MAX` sert de sentinelle aux tableaux à époque.

En cas de dépassement :

```text
resource_exhausted / requires_tiling
```

avant publication, jamais une troncature. Cette garde ne contredit pas la politique « offsets globaux u64, index locaux u32 » ; elle en est précisément la première moitié. Les tuiles retireront ensuite le refus.

La borne Poisson q2 montre que le problème n'est pas théorique : le nombre de facettes d'un ordre élevé peut dépasser la capacité locale bien avant que le nombre de points n'approche `2^32`.

Porte recommandée sans allocation géante : injecter des bases artificielles de compteurs proches des limites et tuer les mutants

```text
fold-u32-event-wrap,
fold-i32-fid-wrap,
fold-epoch-sentinel-collision.
```

---

## Ordre conseillé à Claude

1. Intégrer la preuve `gamma_5` de la borne dynamique dans `MATHEMATIQUES.md` ou le reçu arithmétique ; la borne `2^-48` est reçue.
2. Désactiver automatiquement le filtre sous fast-math ou mode d'arrondi incompatible.
3. Ajouter la garde transactionnelle des index locaux du fold.
4. Implémenter ensuite l'intervalle certifié de `2P²-JB²` ; c'est le gain arithmétique immédiatement mesuré.
5. Prototyper séparément le comptage q3 par couches convexes sur les seules ancres lourdes, sans retarder le port GPU du scan filtré.

Le résultat important est positif : l'accélération flottante n'introduit pas une approximation géométrique. C'est un filtre exact avec recours entier, et sa borne dynamique possède maintenant une preuve simple plutôt qu'une confiance statistique.
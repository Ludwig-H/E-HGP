# Certificat exact de support principal par miniboules supprimées

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_exact_source`,
`profile=quantized_u16_input_only`, `mode=solution_math_and_contract`, aucun
statut public. Cette note ferme le producteur et le vérificateur du bit
`principal_support_certified` demandé par le fold hybride.

## Théorème

Soient `B` une boule critique, `M` son saturé, `U` un support minimal strict de
`B` et `u` un élément de `U`. Posons `R_u=M privé de u` et notons
`D_u` la miniboule de `R_u`.

L'élément `u` appartient à tout support de `B` si et seulement si le niveau de
`D_u` est strictement inférieur au niveau de `B`. Par conséquent, `U` est
principal si et seulement si cette inégalité stricte vaut pour chaque `u` de
`U`.

Preuve : `B` couvre `R_u`, donc la miniboule de `R_u` ne peut pas avoir un rayon
plus grand. Si les rayons sont égaux, `B` est elle-même une boule minimale de
`R_u`; l'unicité de la miniboule euclidienne impose `D_u=B`. Le saturé `R_u`
lui-même, puis un support de `D_u` inclus dans `R_u`, témoignent alors d'une
partie qui engendre `B` sans `u`. Inversement, si une partie `A` incluse dans
`R_u` avait `B` pour miniboule, toute boule couvrant `R_u` couvrirait `A`, donc
le rayon de `D_u` serait au moins celui de `B`; comme `B` couvre `R_u`, il serait
égal. L'inégalité stricte exclut donc toute partie omettant `u`.

Cette formulation implique le séparateur de la coquille
`center(B) not in conv(Q privé de u)`, mais ne demande ni reconstruction de `Q`,
ni LP, ni nouvelle arithmétique géométrique.

## Certificat positif compact

Le producteur calcule `D_u=miniball(R_u)` avec la primitive exacte existante et
stocke seulement un support `V_u` de `D_u`, donc au plus quatre `PointId` en
dimension trois. Si l'on calcule sur la seule coquille, l'inégalité stricte
ramène cette borne à trois; utiliser `M` évite toutefois de transporter `Q` et
reste plus simple à vérifier.

Le vérificateur hostile n'a pas à réénumérer `R_u` :

1. vérifier `V_u` trié, sans doublon, inclus dans `R_u` et de taille `1..4`;
2. reconstruire `D_u=miniball(V_u)` et exiger son succès ainsi que son support
   strict;
3. vérifier `sphere_side(D_u,x)<=0` pour chaque `x` de `R_u`;
4. exiger `sphere_cmp_beta(D_u,B)<0`.

Ces quatre contrôles prouvent directement qu'une boule strictement plus petite
couvre `R_u`; `V_u` n'a même pas besoin d'être le support canonique choisi par
le producteur. Lorsque la coquille vérifiée satisfait `Q=U`, le support strict
suffit et aucun payload par `u` n'est nécessaire. Le cas `q=1` avec `R_u` vide
appartient à ce raccourci.

Un certificat négatif est tout aussi compact : dès que
`sphere_cmp_beta(D_u,B)==0`, le support de `D_u`, de taille au plus quatre et
inclus dans `R_u`, témoigne d'un support alternatif. Un vérificateur hostile ne
se contente toutefois pas de l'égalité des niveaux : il reconstruit la boule
depuis ce support, vérifie qu'elle couvre `R_u`, puis exige l'égalité de sa
`BallKey` avec celle de `B`. Alors seulement l'état
`non_principal_certified` est autorisé. Sans certificat positif ou négatif
vérifiable, l'état reste `unknown` et le dispatcher choisit le fallback. Une
comparaison positive `D_u>B` est une faute interne, puisque `B` couvre déjà
`R_u`.

Le séparateur rationnel reste récupérable si un consommateur le demande. En
posant `h=center(D_u)-center(B)`, tout `x` de la coquille privé de `u` vérifie une inégalité
strictement positive obtenue en soustrayant `|x-center(B)|^2=r_B^2` de
`|x-center(D_u)|^2<=r_D^2`. Il est donc inutile de stocker `h` en plus de `V_u`.
En pratique, ne pas matérialiser `h` : sous u16, son numérateur commun peut
dépasser `i128` et ses produits peuvent atteindre environ 258 bits. Les sphères
et les comparaisons existantes possèdent déjà leurs bornes reçues :
`sphere_side` reste dans `i128`, le carré du numérateur dans `BigInt<4>` et la
comparaison croisée des niveaux dans `BigInt<6>`. Le payload de `PointId V_u`
réutilise exactement ce chemin sûr.

## Point d'intégration exact

Le bon producteur est `try_emit_with` dans `prototype/order_k_flats.hpp`. À cet
endroit, `points`, le saturé exact `members=M`, la coquille `shell=Q`, la sphère
`B` et le support canonique `U` sont simultanément disponibles. Pour chaque
`u` :

```text
R = members sans u, trié par l'ordre géométrique canonique
D = miniball_of(points, R)
si D > B : faute interne
si D = B : conserver obligatoirement le support alternatif pour certifier non-principal
si D < B : conserver le support V_u de D
après q inégalités strictes : principal=true
```

Le certificat doit être créé avant le tri final de `kept`, puis permuté avec le
même tableau d'ordre que les sphères. Il faut l'ajouter aux deux chemins qui
publient aujourd'hui dans `kept` : l'émission générale de `try_emit_with` et le
chemin direct des singletons indexés. Une primitive de publication commune,
suivie d'une assertion `kept.size()==certificates.size()`, évite un décalage
silencieux. Le certificat doit être lié au digest exact du nuage et de `members`;
le raccourci optionnel `Q=U` lie en plus le digest de `Q`.

Le fold ne peut pas produire ce certificat depuis `Catalogue` seul, car ce type
ne transporte pas les coordonnées. Un sidecar v3 minimal porte globalement le
digest du nuage et du catalogue final, le profil ainsi que
`source_complete_for_order[k]`; ce dernier n'est jamais déduit d'un argument
CLI tel que `smax>=n`. Pour chaque handle de générateur, il porte ensuite :

- l'état `unknown`, `principal_certified` ou `non_principal_certified`;
- le mode `implicit_Q_eq_U` ou les supports positifs `V_u`, indexés par le
  `PointId u` et jamais par sa seule position dans le support;
- le premier support alternatif négatif, obligatoire pour l'état
  `non_principal_certified`, sinon l'état reste `unknown`;
- les digests d'entrée/membres, éventuellement de coquille, et le bit
  `q_min_certified`.

L'index `BallKey -> generator_handle` n'est construit qu'après la permutation
finale de `kept` et des certificats; il ne conserve ainsi aucun handle
pré-canonisation.

Pour intégrer rapidement le prototype sans modifier les deux chemins chauds du
producteur, une variante plus sûre est de construire le même objet **après** le
tri canonique final :

```text
make_validated_hybrid_sidecar(points, catalogue_final, source_receipt)
    -> Result<ValidatedHybridSidecar, Refusal>
```

Cette factory parcourt les handles finaux, couvre naturellement les singletons,
recalcule ou vérifie les certificats ci-dessus, lie les digests et construit
l'index seulement lorsque toutes les validations ont réussi. Elle évite tout
risque de décalage entre `kept`, le chemin singleton et un tableau parallèle.
Le fold hybride n'accepte ensuite qu'un `ValidatedHybridSidecar`; l'ancienne
signature `(points, point_count, Catalogue)` reste au mieux un harnais borné.
À l'échelle, la source peut produire directement les mêmes témoins afin
d'éviter ce second passage, mais le vérificateur post-catalogue et son type
validé restent la frontière de confiance.

`unknown` sélectionne le fallback exact; il ne vaut jamais `false` au sens
scientifique. Un sidecar qui prétend `principal_certified` ou
`non_principal_certified` mais échoue à la vérification fait refuser le lot
atomiquement. Le certificat principal et la validation stricte de `U`
impliquent localement `q_min=|U|`; le bit `q_min_certified` reste néanmoins
obligatoire pour le marquage public de tous les générateurs, notamment ceux du
fallback. Le fast path exige en plus `source_complete_for_order[k]`.

## Coût et stratégie de livraison

Le producteur complet appelle au plus quatre fois `miniball_of` sur un saturé
de rang actuellement borné à 32. Le vérificateur est linéaire en `q*|M|` après
reconstruction de petites boules. Pour livrer plus vite :

1. certifier d'abord seulement `Q=U`;
2. activer ensuite le producteur `miniball(M privé de u)` hors chemin chaud;
3. mesurer `principal/non_principal/unknown`, tailles de `M/Q`, appels et temps;
4. conserver le fallback pour tout budget dépassé au lieu d'affaiblir la
   vérification.

Le code existant fournit déjà `miniball_of`, `sphere_side` et
`sphere_cmp_beta`; aucune nouvelle largeur arithmétique ni bibliothèque LP
n'est nécessaire.

## Portes permanentes

- `Q=U` pour `q=1,2,3,4`, avec supports stricts reçus.
- Cas principal `Q` strictement plus grand : centre `(2,2,2)`, rayon `1`,
  `U={(1,2,2),(3,2,2)}`, extra-shell `(2,3,2)`.
- Cas u16 fractionnaire positif : support diamétral
  `U={(0,0,0),(65535,65535,65535)}`, extra-shell `(0,0,65535)` et intérieur
  `(32767,32767,32767)`. Ajouter `(65535,65535,0)`, antipode de l'extra-shell,
  produit le négatif avec support alternatif.
- Borne quatre réellement atteinte pour le certificat sur `M` : `B` de centre
  `(40,40,40)`, rayon `30`, support
  `{(10,40,40),(70,40,40)}`, puis intérieurs
  `(45,43,44),(45,43,36),(45,35,40)`. Après retrait du premier point du
  support, la petite boule a centre `(57,40,40)`, rayon `13` et les quatre
  points restants comme support strict.
- Boucle complète sur les quatre éléments : centre `(20,20,20)`, rayon carré
  `277`, coquille `A=(11,20,34)`, `B=(11,34,20)`, `C=(20,11,6)`,
  `D=(35,14,24)`, `E=(24,26,5)`. Le support canonique est `{A,B,C,D}` et seul
  `B` possède un support alternatif après suppression; permuter les identifiants
  place cette omission dans chacune des quatre cases.
- Les trois fixtures non principales `q=3,k=4`, `q=k=4` et `q=4,k=6` de
  [`AUDIT_COFACES_F2E78FA.md`](AUDIT_COFACES_F2E78FA.md).
- Mutants : oublier de retirer `u`, accepter `<=` au lieu de `<`, ne pas vérifier
  la couverture de `R_u`, substituer un `V_u` extérieur à `R_u`, permuter les
  certificats après le tri de `kept`, ou réutiliser un digest d'une autre
  famille de membres.
- Différentiel : bit/certificat produits par la source contre un oracle qui
  énumère tous les sous-ensembles de `Q` de tailles `1..4` dont la miniboule
  vaut `B`. Tout sous-ensemble de `M` engendrant `B` contient un tel support
  minimal sur `Q`; cet oracle en `O(|Q|^4)` est donc exhaustif pour la
  principalité et indépendant du critère par suppression.

Diagnostic local supplémentaire, relatif aux primitives exactes partagées :
20 000 familles aléatoires de grille, 51 571 suppressions de points de support,
arités `1/2/3/4 = 1991/7643/7170/3196`, ont comparé le critère supprimé à
l'énumération de tous les supports de taille au plus quatre. Résultat : zéro
désaccord (`PRINCIPAL_DELETE_OK`). La preuve ci-dessus reste l'autorité.

GCP non utilisé.

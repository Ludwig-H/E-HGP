# Directive de déblocage — microkernel P0 puis wavefront commune `D,V,T`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit s'adresse à Claude. Il ne modifie aucun code, n'emploie pas GCP et
ne reçoit aucun claim produit. Le pin logiciel lu est
`4f4b463268295e6445bad2f80bbfbf89cf2734ce`, commit
`my Morton was a plane curve applied to three axes, and it cost a factor
three`. Le worktree logiciel évolue concurremment ; les observations statiques
ci-dessous portent donc sur ce pin.

## 1. Décision courte

La bonne route n'est ni un grand `s`, ni la DFS actuelle portée sur GPU.
L'ordre de travail recommandé est désormais fermé :

1. recevoir et chronométrer un microkernel terminal `RF-GPU-P0`, sans file ni
   split adaptatif ;
2. seulement si son p95 laisse du budget, construire `DVT-CWave-v0`, une
   wavefront persistante qui traverse `C` une fois et produit simultanément les
   témoins centraux, la lentille et les carriers aigus ;
3. conserver les sorties q3/q4 sous forme de relations, puis brancher le
   shallow exact sans développer toutes les paires de la lentille.

Le pin corrige bien la dilation Morton3D. Il ne reçoit pas encore P0 :
`rect_central_mask(A,B,C)` recalcule `Dlo` pour chaque ID, emploie
`__int128` alors que ses trois seuils tiennent en `u64`, et les helpers de
cohérence Morton/masque ne sont pas encore reliés à une porte au pin. La
réparation immédiate est une API `central_mask_from(dlo,vhi)` avec `Dlo`
sorti de la boucle des candidats.

## 2. P0 minimal à mesurer avant toute architecture générale

Entrée : tape terminal WSPD reçu à `s=1` ou `s=2`, coordonnées u16 et vrais
`PointId` déjà rangés en SoA par `(Morton48,PointId)`. Un tableau inverse
`relation_rank[PointId]` conserve le rang du même ID dans l'arbre fair-split :
il permet de tester l'appartenance aux plages `A/B` sans confondre ordre Morton
et ordre du relation-tree.

Ordonnance recommandée :

- un warp par `RectId` ;
- une fenêtre contiguë `W=32`, recadrée afin de lire exactement
  `min(32,n)` IDs même en fin de tableau ;
- `Dlo` calculé une fois par rectangle ;
- les 32 `Vhi` calculés en parallèle, puis trois ballots imbriqués
  q4/q3/q2 ;
- rejet endpoint par `relation_rank`, puis extraction par `ffs` des seuls
  `8/9/10` IDs de preuve, sans
  `std::vector`, heap, tri top-16 ou indirection aléatoire ;
- pour les lanes dont q2 reste ouvert, calcul conditionnel de
  `Hmin_singleton` et second ballot ;
- compactage stable de toute lane encore ouverte en `DELEGATED_RESIDUAL`.

Tester les 32 IDs plutôt que trier les 16 meilleurs conserve les mêmes 32
lectures mémoire, supprime le tri et augmente le rappel. Cette variante doit
néanmoins rester une ablation contre le top-16, non une vérité supposée.

Pour un singleton `z`, `Hmin_singleton` est le minimum exact sur
`box(A)×box(B)` : quatre produits par axe, donc douze produits `i64`. Sa
stricte positivité est un certificat q2-ALL sûr pour les points des nœuds. Ce
n'est pas un test complet sur les ensembles corrélés. Par exemple, avec
`A={(0,0),(0,1)}`, `B={(0,4),(1,3)}` et `z=(1,2)`, les quatre produits
ponctuels sont positifs, mais la relaxation AABB donne zéro. Un échec reste
donc délégué.

À `uniform,n=50000,s=2`, le reçu donne `F=1392028`, soit environ `44,5 M`
lectures pour `W=32`. À `s=4`, `F=5143451`, soit `164,6 M` lectures avant la
source. P0 compare donc `s=1` et `s=2`; il ne monte pas globalement à `s=4`.

Porte P0 : trente répétitions résidentes, `p95<=200 ms` comme seuil
d'ingénierie, avec `Dlo_calls=F`, `Vhi_calls=IDs_testés`, compte exact des
produits `Hmin`, `wide_products=0`, octets/HWM, `planned=filled=consumed`,
preuve rejouable et conservation complète des masks/masses. Ce seuil ne
qualifie ni le préfixe complet, ni le SLO d'une seconde.

Les outcomes ne doivent pas attendre un faux bit owner porté par la paire.
Pour q2, dix preuves universelles donnent `CLOSED_PAIR_SHARD`. Pour q3/q4,
neuf ou huit preuves donnent un certificat conditionnel
`PRUNED_OWNER_SHARD` : il tue exactement l'ensemble des supports dont l'arête
owner appartient au rectangle, sans prétendre que ces supports existent. La
WSPD partitionne les `PairId` et la règle `max_edge puis plus petit PairId`
partitionne les supports ; la source ne consomme que les shards non tués et
rejoue l'owner quand les autres sommets sont connus.

La clé de shard doit être injective dans l'arbre reçu, par exemple
`(RelationTreeDigest,Epoch,NodeOrdinalA,NodeOrdinalB,lane,owner_rule)`. Un
FNV-64 reste un digest de replay, jamais l'identité scientifique : une
collision de hash ne peut pas fusionner deux shards.

## 3. Identité commune qui débloque le raccord source

Pour une paire concrète `a,b` et un point `z`, poser :

```text
d = b-a
v = 2z-a-b
D = ||d||^2
V = ||v||^2
T = d dot v
e = z-a = (d+v)/2
t = b-z = (d-v)/2
```

Alors les trois identités exactes sont :

```text
4H = D-V, avec H=e dot t
16 E2 X2 = (D+V)^2-4T^2
z appartient à la lentille faible de ab  <=>  V+2*abs(T) <= 3D
```

La dernière équivalence regroupe exactement
`||z-a||^2<=D` et `||b-z||^2<=D`. Elle donne le raccord qui manquait :

```text
témoin central q2 : D>V
carrier aigu q3   : D<V et V+2*abs(T)<=3D
point de lentille : V+2*abs(T)<=3D
```

Sous arête `ab` faiblement maximale, la seconde ligne caractérise le carrier
aigu q3. L'owner canonique est décidé plus tard sur le `SupportKey`; il ne peut
pas être authentifié par un rectangle de paires seul.

Un seul calcul de `D,V,T` peut ainsi alimenter les deux côtés de la source :
`H>0` crédite les tombstones universelles, tandis que `H<0` produit la relation
de carriers. Il n'est plus nécessaire de terminer le filtre puis de repartir
de `C=root` pour rechercher les porteurs.

## 4. Classifieur AABB commun `ALL/NONE/UNKNOWN`

Sur `A×B×C`, calculer des bornes sûres :

```text
D dans [Dlo,Dhi]
V dans [Vlo,Vhi]
T dans [Tlo,Thi]
glo = Dlo-Vhi
ghi = Dhi-Vlo
tabs_lo = distance(0,[Tlo,Thi])
tabs_hi = max(abs(Tlo),abs(Thi))
Phi_lo = max(0,(Dlo+Vlo)^2-4*tabs_hi^2)
Phi_hi = (Dhi+Vhi)^2-4*tabs_lo^2
```

`Dlo/Dhi` sont les distances carrées minimale/maximale entre les deux AABB.
Pour `V`, chaque axe parcourt
`[2*Clo-Ahi-Bhi,2*Chi-Alo-Blo]`; la distance à zéro et l'extrémité de plus
grande valeur absolue donnent `Vlo/Vhi`.

La borne de `T` peut être exacte sur le produit continu sans produit
d'intervalles lâche. Par axe,
`T_i=(z-a)^2-(z-b)^2`. Le maximum de cette fonction multi-affine est obtenu à
`z=Clo` ou `z=Chi`, puis par `far(z,A)^2-dist(z,B)^2`; le minimum emploie
`dist(z,A)^2-far(z,B)^2` aux mêmes deux extrémités. Sommer les trois axes.

Les verdicts suivants sont sûrs :

```text
central q2 ALL  : glo>0
central q2 NONE : ghi<=0

central q3 ALL  : glo>0 et 4*glo^2>Phi_hi
central q3 NONE : ghi<=0 ou 4*max(ghi,0)^2<=Phi_lo

central q4 ALL  : glo>0 et 3*glo^2>Phi_hi
central q4 NONE : ghi<=0 ou 3*max(ghi,0)^2<=Phi_lo

LENS ALL  : 3*Dlo-Vhi-2*tabs_hi>=0
LENS NONE : 3*Dhi-Vlo-2*tabs_lo<0

ACUTE ALL  : ghi<0
ACUTE NONE : glo>=0
```

L'égalité de lentille est admise ; l'acuité est stricte. Les carrés de `Phi`
emploient `i128`, contrairement au masque central P0. `ACUTE_LENS` est ALL si
les deux composantes le sont, NONE si l'une est NONE, et UNKNOWN sinon. Aux
singletons, les prédicats retrouvent les décisions exactes.

Ces bornes ont été falsifiées hors dépôt par énumération sur deux mille petites
AABB aléatoires, sans désaccord. Ce diagnostic n'est pas une réception : le
juge du dépôt doit les comparer exhaustivement à `H,E2,X2`, lentille et
acuité, avec toutes les frontières strictes.

## 5. ABI `DVT-CWave-v0`

État par relation :

```text
RectState {
  RectId, ANodeKey, BNodeKey,
  open_witness_mask,
  credit_count[3], proof_ids[<=10],
  mixed_c_span, scheduler_epoch
}

CWaveTask {
  RectId, CNodeKey,
  witness_mask, lens4_mask, acute_lens_mask,
  continuation_key
}
```

Sorties SoA :

```text
CentralHit(RectId,CNodeKey,lane_mask,mass,proof_ids)
Lens4Block(RectId,CNodeKey,proof)
AcuteLensBlock(RectId,CNodeKey,proof)
MixedABCFront(RectId,CNodeKey,remaining_masks,continuation_key)
```

Initialiser exactement une tâche `(RectId,C=root)`. À chaque ronde
count--scan--fill :

- un masque `ALL` crédite ou émet le nœud et ne descend plus pour ce masque ;
- un vrai `NONE` retire le masque ;
- seul `UNKNOWN` est transmis aux enfants ;
- une lane témoin saturée à `10/9/8` retire immédiatement son bit ;
- les sorties source déjà préparées pour une lane ensuite fermée sont
  tombstonées puis compactées avant consommation ;
- un quantum épuisé sérialise `MixedABCFront`, jamais un résultat partiel.

Les `proof_ids` de P0 initialisent les crédits de CWave. Tout nœud central
accepté doit dédupliquer ces IDs avant de créditer sa masse, faute de quoi la
même preuve est comptée deux fois.

Gates physiques : `root_entries=F`, `restarted_roots=0`,
`created=consumed`, tâches/visites/records/octets/HWM à deux pentes, et oracle
petit `n` développant les relations `Central/Lens/Acute/Mixed` avec
multiplicité un. Une expansion proportionnelle à la masse d'un bloc est un
refus d'architecture.

Le juge carrier reçoit au minimum : frontière droite
`a=(0,0,0),b=(2,0,0),x=(1,1,0)` qui doit être `NONE` car
`D=E2+X2`; tie d'arête admis
`a=(0,0,0),b=(5,0,0),x=(3,4,0)` qui doit être `ALL` avec
`E2=D=25`; boîte mêlant ces cas qui doit être `MIXED`; mutants `< / <=` sur
les deux contraintes de lentille et sur l'acuité ; puis oracle exhaustif de
petites AABB contre tous les triplets ponctuels. Une fixture positive unique ne
reçoit ni `NONE`, ni `MIXED`, ni les frontières.

Le premier `carrier_scan` concurrent valide le besoin mais pas l'ordonnance.
À `n=2000,s_inf=2`, après la banque `W32/L16`, il ne prouve vide que
`4,66 %` des records uniformes, `10,38 %` terrain et `7,27 %` huit-amas, tout
en effectuant environ vingt à vingt-trois tests par record résiduel. Il repart
de `C=root` et alloue une pile par rectangle. Ce code reste oracle/diagnostic :
ses prédicats deviennent les masks source de la même CWave ; aucun second
kernel ne recommence la racine. Une feuille `MIXED` signifie
`POSSIBLE_OR_PRESENT`, jamais `HAS_CARRIER`, et la télémétrie publie records
et masses par taille sans compteur global cumulatif.

## 6. q4 est `Acute × Lens`, pas toutes les paires de la lentille

Noter `L` la relation complète des points de lentille et `P` sa sous-relation
aiguë. Tout q4 positif ancré par `ab` possède au moins une face aiguë. Les
couples à examiner sont donc exactement :

```text
x dans P, y dans L, x!=y,
et, si y est aussi dans P, PointId(x)<PointId(y)
```

Cette règle émet `P-P` une fois et `P-(L\P)` une fois. Elle ne doit pas devenir
une boucle `C(n_lens,2)`. Les relations restent en blocs ; un join ultérieur
applique d'abord `||x-y||^2<=D`, puis rang affine trois, positivité stricte,
owner, census et shell. Pour deux blocs `Cx,Cy`, `maxdist^2(Cx,Cy)<=Dlo`
certifie ALL et `mindist^2(Cx,Cy)>Dhi` certifie NONE avant raffinement.

Attention au vocabulaire du moteur shallow : ses classes historiques
`P-P/N-N/P-N` désignent l'orientation au-dessus/au-dessous des demi-plans,
pas l'acuité des carriers. Elles doivent être renommées `UP/DOWN` dans la
documentation ou conservées strictement séparées du bit `acute`.

Les témoins universels déjà reçus réduisent les profondeurs restantes : q3
transporte `k3=8-credit3`, q4 `k4=7-credit4`. Le census final relit néanmoins
le nuage complet ; les points hors lentille peuvent être intérieurs à la
circumboule d'un support q3/q4.

## 7. Correction d'héritage sous split `A/B`

Seuls deux états s'héritent sans calcul après restriction de `A/B` :

- une preuve géométrique `ALL` et ses IDs ;
- un vrai `GEOMETRIC_NONE` universel du prédicat.

Tous les échecs de certificateur doivent être reclassifiés : `Vbest` impossible,
`CENTRAL_DEAD`, fenêtre vide ou capée, ID non retenu, endpoint relatif et tout
`UNKNOWN`. Le seul sidecar `endpoint_blocked` est insuffisant.

Fixture permanente WSPD `s=2` :

```text
A=[0,100]^3, B={(200,50,50)}
parent : Dlo=10000, Vbest=7500 -> q3/q4 centraux impossibles
A0=[0,50]x[0,100]^2
enfant : Dlo=22500, Vbest=5676 -> q3 et q4 centraux possibles
z=(112,50,50) est un candidat non-endpoint de l'enfant
```

Le parent est bien séparé sous la convention entière reçue. Cette fixture tue
toute implémentation qui hérite `CENTRAL_DEAD` ou ne rejoue que les endpoints.

`Vbest` est séparable : par axe, minimiser
`max(abs(2z-slo),abs(2z-shi))^2` au plus proche entier u16 de
`(slo+shi)/4`, puis sommer les trois minima. Il n'est pas nécessaire
d'énumérer huit combinaisons. Un échec signifie
`CERTIFIER_MISS/AB_SPLIT_REQUIRED`, jamais `NONE`, et n'interdit pas le repli
`Hmin`.

Un split adaptatif ne conserve pas la borne linéaire du WSPD de base. Avec
`R` rondes, il peut doubler le nombre de records à chaque ronde. La première
version mesure donc `R=0/1/2/3`, score les **deux** enfants par masse rendue
viable et slack normalisé, puis délègue au cap. Les marges brutes q2/q3/q4 ne
sont pas directement comparables à cause de leurs coefficients différents.

## 8. Ce que cette route prouve et ce qu'elle ne prouve pas

`DVT-CWave-v0` supprime le rescan racine et factorise le raccord
témoin--carrier. Il ne prouve pas que le consommateur global est
sous-quadratique. Sont explicitement refusés :

- `WSPD O(n)`, donc join `A×B×C O(n)` ;
- `AcuteLens ALL`, donc support ou masse décidée ;
- deux carriers q4 aigus obligatoires ;
- shallow `O(mk)`, donc somme globale bornée sur toutes les ancres ;
- loi de Poisson, donc contrat adversarial ou SLO.

Le test qui décide la poursuite est physique : P0 d'abord, puis CWave avec
`restarted_roots=0`, enfin le consommateur `Acute×Lens` factorisé. Si une
tranche obligatoire atteint déjà `1 s` au p95 à `50000`, cette ordonnance est
réfutée pour le contrat secondaire. Si elle passe, elle n'est qu'admise à
l'étage suivant ; le seul verdict produit reste le `warm_e2e` exact jusqu'au
fold, dix forêts et verticales.

GCP non utilisé par cet audit.

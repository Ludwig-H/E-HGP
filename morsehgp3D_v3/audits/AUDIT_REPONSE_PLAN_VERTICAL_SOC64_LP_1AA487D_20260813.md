# Réponse au plan de route : fermer une tranche verticale, puis employer les certificats qui changent réellement le coût

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Pin, périmètre et verdict

Cette réponse vise
[`NOTE_CLAUDE_ROUTE_50K_PUIS_DIZAINES_DE_MILLIONS_20260813.md`](NOTE_CLAUDE_ROUTE_50K_PUIS_DIZAINES_DE_MILLIONS_20260813.md),
SHA-256
`c3cfb97ccdd3d65757db7ba5d0480db56fe1d0edf5eb3c3e9ac5912bfb1ad970`.
Le snapshot observé est le `HEAD`
`1aa487d77b447d7359ba9a81b7ab1285b4a27abf`, worktree propre. Les
empreintes du probe et de CMake sont respectivement
`0e7d4d753fd52adfd2d007659fe845025d8bafeea608e0b3c323edae086c19e0`
et `c76776a579e6e2c57881b450bb999b7285bbd2edd3b6687f16a1cd8c0af54df4`.
L'auditeur ne modifie aucun logiciel.

Réponses directes aux trois questions :

1. **Oui à une tranche verticale output-bearing avant une nouvelle campagne de
   parcimonie.** Une fenêtre encore dense ne la rend pas prématurée : cette
   tranche est bornée et sert d'autorité de composition, pas de benchmark. Il
   faut cependant construire la tranche la plus mince qui traverse
   `BallForm -> PrimitiveSphereKey -> census -> BallEvent -> fold`, pas
   préconstruire tout le producteur optimisé.
2. **Préparer une frontière de profil dans l'ABI, pas implémenter `binary64`
   dans la v3 u16.** Le fold ne doit dépendre ni du packing u16, ni de cinq
   coefficients `i128`, ni d'une représentation native des scalaires. Le
   backend u16 reste le seul profil de cette phase ; un futur producteur
   binary64 certifié pourra se brancher derrière la même sémantique sans
   réécrire le fold.
3. **Conserver la multiplicité lossless comme baseline.** Un quotient de
   plateau n'entre dans `BenchmarkOutputContract-v1` qu'après preuve de
   reconstruction des dix forêts, coverage et verticales. Le quotient Johnson
   discuté jusqu'ici ne reçoit que le H0 normalisé. Un refus explicite est un
   statut de ressource ou de domaine, jamais une façon silencieuse de rendre un
   résultat exact incomplet.

Le recul de Claude est donc juste. Deux corrections empêchent toutefois de
transformer ce bon ordre en nouveau récit optimiste : `2,6e9`
recertifications extrapolées depuis `n=6000` ne prouvent pas que « le combat est
sur les constantes », et une pente `sum_E4` verte ne suffit pas à sélectionner
le certificateur. Les tâches, les lectures, les produits larges, le census, la
HWM et le fold restent dans le coût de bout en bout.

## 1. Étape zéro : une tranche verticale mince, pas un second oracle horizontal

L'objet minimal à recevoir est `VerticalBallEventSlice-v0` :

```text
BallForm rationnelle + SupportKey + owner proposé
  -> PrimitiveSphereKey normalisée
  -> RLE des formes de même sphère
  -> un census exact du nuage : I_B et U_B triés
  -> décisions p+q par support incident
  -> BallEvent régulier ou PlateauEvent lossless
  -> RegularDirectRecord/plateau spool
  -> fold borné et BenchmarkOutputContract-v1
```

Ce jalon peut employer l'énumération exhaustive à petit `n`. Il ne doit ni
prétendre être la source produit, ni être rampé. Son rôle est de rendre chaque
optimisation amont falsifiable sur l'identité complète
`(BallKey,SupportKey,I_B,U_B,owner)` puis sur les dix forêts, leurs lots et
leurs verticales.

La `PrimitiveSphereKey` précède le census parce qu'elle encode l'équation
primitive de la sphère. La `BallKey` sémantique qui emploie le shell vient après
le census. Ce découpage évite la circularité « BallKey avant de connaître
`U_B` » et mutualise le census entre tous les supports d'une cosphère.

Portes minimales de la tranche :

- une sphère régulière `U_B=S`, avec un intérieur réel conservé dans `I_B` ;
- deux `SupportKey` distinctes pour une même `PrimitiveSphereKey`, un seul
  census et deux décisions de lane ;
- la fixture cocyclique de six points, puis un petit shell lourd, sans
  troncature de la provenance ;
- égalités `P=-1/0/1`, owner avec deux et trois arêtes maximales, et permutation
  des `PointId` ;
- chunks de taille un et coupure au milieu d'un lot de niveau égal ;
- identité des dix forêts, coverage et verticales entre la voie résidente et la
  voie streamée ;
- cap exact puis cap moins un : le second rend une continuation ou
  `resource_exhausted`, jamais un préfixe de sortie.

### 1.1 ABI de profil : préparer la séparation, pas anticiper le calcul

La frontière durable contient au moins :

```text
GeometryProfileId
ExactKeySchemaId
CloudEpoch / CloudDigest
PointId et listes triées I_B, U_B, SupportKey
ExactLevelToken + comparateur du profil
PrimitiveSphereKeyRef opaque et sérialisable
BallEventKind = REGULAR | PLATEAU_LOSSLESS | PLATEAU_QUOTIENT
provenance_digest, continuation et status
```

Le fold consomme les identités, l'ordre exact des niveaux, les incidences et
les événements ; il ne lit jamais les coordonnées ni les limbs de la clé. Pour
u16, `PrimitiveSphereKeyRef` peut pointer vers cinq entiers primitifs à largeur
fixe reçue. Un futur profil binary64 pourra employer des entiers dyadiques
normalisés, des expansions ou un stockage large différent derrière le même
contrat. Il ne faut ni figer la sérialisation native de `__int128`, ni ajouter
maintenant des prédicats binary64 hors du profil déclaré.

### 1.2 Cosphère lourde : décision réversible de layout

La baseline exacte conserve :

```text
PlateauEvent {
  BallKey, I_B, U_B, q_min, lanes_admises,
  SupportStreamRef lossless,
  owner_policy, provenance_digest
}
```

Le stream peut être produit et consommé par chunks ; il n'impose pas un
catalogue résident. Il ne supprime cependant pas le travail ni la taille de
sortie intrinsèques. `PLATEAU_QUOTIENT` reste une variante d'événement séparée,
avec `quotient_schema` et preuve de reconstruction. Tant que seules les
composantes H0 sont reçues, elle ne peut alimenter le contrat complet. Cette
union rend le choix futur réversible : ajouter un quotient n'altère pas la
branche lossless et un refus de ressource ne change jamais la sémantique d'un
succès.

## 2. Premier levier après la tranche : `SOC64`, puis `CORNER512`

L'intervalle exact du terme directionnel est utile, mais sa combinaison avec
les extrema séparés de `D2`, `V2` et `T` perd beaucoup de corrélation. Il existe
un palier plus fort, sans division et directement vectorisable.

Pour `e=z-a`, `t=b-z`, poser
`H=e dot t`, `E=||e||^2` et `X=||t||^2`. Les lanes ponctuelles sont :

```text
q2 : H>0
q3 : H>0 et 4*H^2>E*X
q4 : H>0 et 3*H^2>E*X
```

Les égalités ne créditent jamais. Une seule évaluation rend le masque imbriqué
`q4 => q3 => q2`.

### 2.1 `SOC64-v0` : 64 couples de coins, verdict ALL sûr

Former les deux boîtes de différences exactes :

```text
Ebox = C-A = [Clo-Ahi, Chi-Alo]
Tbox = B-C = [Blo-Chi, Bhi-Clo].
```

Pour `t` fixé et non nul, écrire `e=alpha*u+w`, avec `u=t/||t||` et
`w` orthogonal à `u`. Le prédicat de coefficient `k` devient
`alpha>0` et `||w||<sqrt(k-1)*alpha` : c'est l'intérieur d'un cône de
Lorentz convexe. La même propriété vaut en échangeant `e` et `t`. Le prédicat
est donc séparément convexe sur `Ebox×Tbox`.

**Lemme SOC64.** Si les `8×8=64` couples de coins passent une lane, tout le
produit relaxé `Ebox×Tbox` la passe, donc tout `A×B×C` la passe.

L'échec reste `UNKNOWN` : la relaxation a oublié que `e` et `t` partagent le
même `z`. Il ne produit jamais `NONE`.

Fixture de gain axial u16 :

```text
A=[0,99]x{100}x{100}
B=[101,200]x{100}x{100}
C={(100,100,100)}
```

Tous les `e,t` sont colinéaires et de même sens ; le rectangle est ALL q4.
Pourtant `Hlo=1`, `Ehi=Xhi=10000`, `D2lo=4` et `V2hi=9801` : les anciennes
bornes scalaires échouent. `SOC64` ferme en 64 tests.

Fixture contre le faux verdict exact :

```text
A=[0,4]x{5}x{0}
B=[13,19]x[3,6]x{0}
C=[7,9]x[4,6]x{0}
```

Les `512` triples de coins réels passent q4, avec marge minimale
`3H^2-EX=217`. Mais le couple fictif
`e=(3,1,0),t=(4,-3,0)` de la relaxation a `H=9,E=10,X=25` et marge `-7`.
L'échec de `SOC64` ne permet donc aucune suppression.

### 2.2 `CORNER512-v0` : ALL exact pour l'enveloppe AABB continue

Le spindle est également convexe en `z` lorsque `a,b` sont fixés. En prenant
l'axe `ab`, `z=m+xu+y` et `r=||y||`, son domaine est un sous-niveau strict
convexe de la forme `x^2+(r+constante)^2<constante`. Le domaine des triples
admissibles est ainsi séparément convexe en `a`, `b` et `z`.

Il en résulte l'équivalence : le produit continu des trois AABB est ALL pour une
lane si et seulement si ses `8^3=512` triples de coins passent. Une réalisation
réemploie huit fois le noyau SOC64, une fois par coin distinct de `C`; si un axe
est dégénéré, le coût tombe à `2^(d_A+d_B+d_C)`. À `C` ponctuel, les 64 tests
sont déjà exacts pour ALL.

Un coin échouant peut être fictif par rapport aux `PointId` du nœud. Il prouve
seulement que l'enveloppe AABB n'est pas ALL ; il guide un split, mais ne fournit
pas un fate scientifique `NONE`. Le palier 512 doit être incrémental, arrêté au
premier échec et réservé aux tâches dont la masse amortit son coût.

Sous u16, `E`, `X` et `|H|` sont inférieurs à `2^34`, tandis que `4H^2` et
`EX` demandent jusqu'à 70 bits. Les termes simples tiennent en `i64`, la
comparaison finale exige `i128` sur CPU ou deux limbs sur device.

Cascade proposée, dans le classifieur existant et non dans une nouvelle sonde :

```text
CentralBall/Hlo/Qhi
  OR SOC64
  OR CORNER512 incrémental
  -> split canonique
  -> terminal ponctuel
  -> cap = PENDING_CONTINUATION
```

Mutants : échange `3/4`, égalité acceptée, oubli de `H>0`, mauvais bouts de
`Ebox/Tbox`, `63/64`, `511/512`, produit `i64`, échec transformé en `NONE` et
cap transformé en verdict.

## 3. Déblocage q4 plus général : le programme linéaire projectif

Les cages entourant l'ancre sont sûres, mais plus fortes que nécessaire. Pour
une ancre translatée en zéro, poser `s_i=z_i-a`, `d=b-a`,
`D=||d||^2`, `q_i=||s_i||^2` et `y=2(c-a)`. Un centre de sphère passant par
`a,b` qui n'a aucun intérieur dans `G` satisfait :

```text
y dot d = D
y dot s_i <= q_i pour tout i dans G.
```

Définir le programme primal de dimension fixe :

$$\kappa_G(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

Son dual maximise `y dot d` sous les contraintes `y dot s_i<=q_i`.

**Théorème LP1.** Le groupe `G` couvre toute sphère par `a,b` si et seulement
si `d` appartient au cône positif de `G` et `kappa_G(d)<D`.

Si `d` est hors du cône, le dual est non borné dans cette direction et un
centre mauvais existe. Dans le cône, l'optimum dual vaut `kappa`. Comme zéro
est faisable, le plan `y dot d=D` rencontre le polyèdre mauvais exactement
quand `D<=kappa`; en cas d'optimum supérieur, le segment depuis zéro atteint le
niveau `D`. L'égalité reste donc ouverte.

Le primal porte trois égalités. Il possède un optimum basique avec au plus
trois coefficients positifs. **Tout crédit directionnel possède ainsi un
certificat de trois `PointId` au plus**, même lorsque ces points n'entourent pas
l'ancre et que la cellule de Voronoï locale est non bornée. C'est exactement la
raison pour laquelle le triple projectif déjà dérivé est l'objet élémentaire
correct.

### 3.1 Fast path et oracle complet de multiplicité

Pour q4, le fast path répète huit fois : résoudre le LP sur le pool restant,
extraire une base couvrante de taille au plus trois, puis retirer ses IDs. S'il
réussit, les huit groupes disjoints fournissent huit intérieurs distincts à
toute sphère. À dimension fixe, la cible pratique est `O(8P)` attendu, jamais
`C(P,3)`.

L'échec glouton n'est pas un rejet. Il existe toutefois un certificat récursif
exact et complet pour une paire. Noter `C_h(P,d)` la propriété « toute sphère
par `a,b` possède au moins `h` intérieurs dans `P` » :

```text
C_0(P,d) = vrai
choisir par LP une base G qui couvre une fois
C_h(P,d) <=> pour chaque z dans G, C_(h-1)(P sans {z},d).
```

La suffisance vient du fait qu'au centre considéré au moins un `z` de `G` est
intérieur, tandis que la branche qui l'a supprimé fournit `h-1` autres IDs. La
nécessité vient de ce que supprimer n'importe quel site enlève au plus un
intérieur ; si `C_h` est vraie, chaque branche possède encore `C_(h-1)`.

Comme `|G|<=3`, q4 demande au plus
`1+3+...+3^7=(3^8-1)/2=3280` petits LP ; q3 au plus `9841`, q2 au plus
`29524`. Ce n'est pas un hot path par paire. C'est en revanche un oracle borné
complet du résiduel, beaucoup plus informatif que le compte de la boule
centrale ou le taux `spindle_empty`.

### 3.2 Extension sans `PairId` à un `BNode`

Pour une base de trois vecteurs de rang plein, orienter
`r=|det(s_1,s_2,s_3)|>0`. Les coefficients de Cramer sont des formes linéaires
`n_i dot d/r` et, avec `p=sum_i q_i n_i`, le crédit vaut :

```text
n_i dot d >= 0 pour i=1,2,3
F(d)=r*||d||^2-p dot d > 0.
```

Sur une boîte entière de différences, le minimum des formes linéaires est aux
coins. Le minimum de `F` est séparable ; par axe, tester les deux entiers voisins
de `p_k/(2r)`, clipés, suffit. Un `BNode` est ALL lorsque les trois minima
linéaires sont non négatifs et `min F>=1`.

Une tâche `(AnchorId,BNodeKey,pool,credit_state)` peut donc :

1. choisir un `d` représentatif et extraire des bases par LP ;
2. valider ces mêmes bases sur tout le `BNode` par les quatre formes ;
3. fermer le span si huit preuves disjointes, ou l'arbre récursif complet,
   passent sur tout le nœud ;
4. sinon scinder, réutiliser les preuves acquises ou sérialiser une
   continuation ;
5. à la feuille, employer l'oracle LP complet, sans cutoff de rang.

Une base choisie au représentant mais non uniforme ne réfute rien. Elle ne fait
que déclencher le split. Les trois formes simples tiennent dans les largeurs
déjà reçues ; `F` demande environ 87 bits sous u16, donc `i128` ou deux limbs.

Cette route est un candidat direct pour `PWC0-A` : elle n'emploie ni `PairId`,
ni Delaunay, ni arrangement global, et son certificat élémentaire est le même
triple projectif que le juge existant.

## 4. Correction et extension des cages : quatre à six sites, pas toujours quatre

L'audit précédent spécialisait trop tôt une cage en tétraèdre. Une cage
ancre-globale est un ensemble dont les vecteurs relatifs engendrent positivement
`R^3`. Après suppression des redondants, c'est une base positive ; en dimension
trois elle peut contenir **de quatre à six vecteurs**, pas toujours quatre.
Cette borne classique est rappelée dans [Planiden et Wang, *Nicely structured
positive bases with maximal cosine measure*](https://doi.org/10.1007/s11590-023-01973-2).

La cellule locale `V_G` possède alors au plus six facettes et au plus huit
sommets. En effet, Euler et le degré minimal trois donnent
`v<=2f-4<=2m-4`. Les mêmes formes de fleur restent exactes ; seule leur quantité
passe de quatre à huit au pire.

Fixture u16 qui tue tout constructeur « tétra seulement » :

```text
a=(32768,32768,32768)
G_k={a +/- k*e1, a +/- k*e2, a +/- k*e3}, k=1,...,8.
```

Aucun sous-ensemble de quatre sites ne contient `a` strictement en dimension
trois : annuler une coordonnée exige ses deux signes, et quatre sites ne peuvent
apparier que deux axes. Chaque `G_k` est pourtant une base positive minimale de
six sites et `V_Gk=[-k/2,k/2]^3`. Elle couvre exactement lorsque :

$$\left\Vert d\right\Vert^2>k\left(|d_x|+|d_y|+|d_z|\right).$$

Pour `d=(-9,-9,-9)`, les huit cages ferment q4. Pour
`d=(-8,-8,-8)`, la huitième est à égalité et seulement sept crédits existent.
Avec dix couches, `d=(-9,-9,-9)`, `(-10,-10,-10)` et
`(-11,-11,-11)` ferment respectivement exactement huit, neuf et dix cages :
une seule banque peut servir q4, q3 et q2 par ordre statistique des seuils.

Le constructeur P0 peut employer des frames directionnelles entières, des
queues par rôle et des horaires latins : quatre rôles pour les tétras, six rôles
`+/-axe` pour les octa-cages. Les permutations rendent les groupes disjoints par
construction ; chaque proposition est ensuite validée exactement. `P=48`
permet huit groupes de six, `P=96` des réparations ou dix groupes partagés.
Un échec ouvre. Plusieurs frames ou salts sont des certificats alternatifs et
leurs crédits ne sont jamais additionnés s'ils réutilisent des IDs.

Deux corrections de largeur et de profondeur sont impératives :

- les formes directionnelles `F` demandent environ 87 bits, mais le tri exact
  des rayons rationnels de cages peut atteindre environ 240 bits par produits
  croisés. Ne pas annoncer un cutoff radial `i128` exact. Employer un calcul
  multiprécision au build de banque ou un majorant entier conservateur, puis
  garder les formes comme autorité ;
- le seuil angulaire `delta>=3h-2` vaut pour les tétra-cages, ou plus
  généralement lorsque chaque groupe enlève au plus trois points d'un
  demi-espace ouvert. Une base positive de six peut en enlever jusqu'à cinq.
  Le seuil universel grossier devient `delta>=5h-4`, ou mieux un budget exact
  `omega(G)` par groupe.

Les cages ancre-globales restent un fast path radial intéressant. Le LP
directionnel est plus général et doit servir de référence : il peut fermer une
direction avec trois sites tous situés du même côté de l'ancre.

## 5. Ordre remis à Claude après ces déblocages

L'ordre corrigé est :

1. **Fermer `VerticalBallEventSlice-v0` maintenant**, dans un exécutable
   existant ou un test de composant, sans nouveau benchmark horizontal.
2. Étendre le classifieur rectangle existant par `SOC64`, puis seulement sur le
   résiduel rentable par `CORNER512`; comparer l'identité output-bearing de la
   tranche, pas seulement les fates.
3. Construire le reporter q4 counter-only avec le glouton LP de huit triples sur
   `BNode`; employer l'arbre LP complet uniquement comme oracle/résiduel borné.
   Comparer en ablation la banque de cages quatre--six sites.
4. Publier `E_4`, tâches, splits, opérations larges, continuations, octets/HWM
   et temps. Une gate finale exige `pending=0`; une pente sparse produite à
   `70 s` reste rouge.
5. Mesurer ensuite `M=sum m_ab`, puis seulement les niveaux shallow q4 et la
   voie owner-edge/pied q3 déjà reçue.
6. Porter sur device uniquement les paliers dont le coût transitif jusqu'au
   fold est vert. Le premier chronomètre G4 couvre exactement le même
   `BenchmarkOutputContract-v1`.

La règle « pas de nouvelle sonde » est saine. Elle ne signifie pas jeter les
preuves ci-dessus : `SOC64` prolonge le classifieur existant, le LP réemploie le
triple projectif existant, et la tranche verticale raccorde enfin les probes à
leur objet. Toute expérience doit désormais remplacer un palier de cette chaîne
ou être une ablation de ce même trajet.

## 6. Gates de mort reformulées

- `max E_4 borné` n'est pas une conclusion de quatre tailles. Fixer un cap
  explicite dérivé de l'arène à `n=50000`, puis le tester ; publier aussi sa
  pente comme diagnostic.
- `sum E_4<1,35` ne suffit pas : gater simultanément tâches producteur, HWM,
  `M`, census uniques et temps transitif. La comparaison `s=3/s=8` a déjà
  montré que réduire `E_4` peut multiplier le temps par `7,6`.
- `resource_exhausted` est un résultat atomique sans payload, distinct de
  `unsupported_degeneracy`. Le second n'est licite que si le domaine public
  exclut explicitement l'entrée ; le profil u16 actuel n'exclut pas les
  cosphères.
- Le contrat secondaire reste `p95 warm_e2e<1 s` à `50000`; le principal reste
  `100 ms`. Aucun calcul de bande passante théorique, aucune extrapolation de
  `n=6000` et aucun CTest CPU ne reçoit l'un ou l'autre.

Verdict : **Claude doit bien arrêter d'élargir horizontalement et fermer la
tranche output-bearing. Dès qu'elle est verte, `SOC64` et le LP projectif sont
les deux déblocages mathématiques prioritaires : le premier récupère les
rectangles axiaux en 64 tests, le second transforme la multiplicité q4 en bases
de trois IDs et fournit enfin un oracle complet du résiduel.**

GCP non utilisé.

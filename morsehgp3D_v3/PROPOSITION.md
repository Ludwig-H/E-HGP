# Proposition consolidée — MorseHGP3D v3

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document remplace la chronologie des propositions abandonnées. Il ne décrit
que l'architecture candidate actuelle, les lemmes encore valides, leurs
préconditions et les portes qui peuvent les réfuter. Le verdict live appartient
à [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

## 1. Objectif et invariants

Construire exactement la hiérarchie Morse/HGP 3D utile pour `K_max=10`, sur
nuages u16, sans matérialiser de mosaïque Delaunay d'ordre supérieur. La cible
est `p95 warm_e2e<100 ms` à `n=50000` sur un G4 ; `1 s` est secondaire.

Invariants :

- aucune structure persistante indexée par l'univers des facettes, cofaces,
  cellules top-m ou supports potentiels ;
- un oracle exhaustif reste borné et ne devient jamais le producteur ;
- toute source incomplète est fail-open et ne publie aucun résultat officiel ;
- égalités traitées en lots atomiques sur les racines pré-lot gelées ;
- aucune troncature : continuation explicite, refus de ressource atomique ou
  refus de domaine typé ;
- les `PointId` scientifiques sont distincts des indices denses, positions
  Morton et rangs de génération ;
- le temps G4 couvre la sortie complète et la synchronisation, pas un probe.

## 2. Route de réception

```text
0A  BallForm -> BallKey -> census -> BallEvent exact
0B  BallEvent -> lots -> dix forêts -> verticales -> payload borné
1   oracle exhaustif remplacé seulement par la source sparse E3/E4
2   compte M3/M4 puis moteurs locaux q3/q4
3   même chaîne portée sur device, parité puis warm_e2e
4   tout nouveau profil numérique dans une phase distincte
```

Les états `source_complete`, `ball_events_complete` et `fold_complete` sont
séparés. Une source candidate peut alimenter le sink différentiel sans être
complète. Aucun commit de lot ne précède le scellement, le tri/merge global des
niveaux exacts et le manifeste de source.

Le pin `2b89ea1` implémente une première version de `0A` sur `coord<=64`, mais
elle n'est pas reçue u16 à cause des overflows et défauts ABI détaillés dans
[`audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).

## 3. Identités et événements

### 3.1 Clé primitive

Une sphère est représentée par :

$$A\left\Vert z\right\Vert^2+B\mathbin{\cdot}z+C=0,$$

avec cinq coefficients entiers primitifs, `A>0`. Le signe est négatif à
l'intérieur, nul sur le shell et positif à l'extérieur.

Pour un centre `N/den` et un point `p` de la sphère, construire avant toute
élévation inutile de degré :

```text
A=den
B=-2*N
C=2*N dot p-den*||p||^2
```

puis normaliser le signe et le pgcd. Ne jamais construire `den^2` et `N^2`
avant réduction lorsque les bornes dépassent le type.

### 3.2 ABI logique

```text
PrimitiveSphereKey = coefficients primitifs de la sphère
BallKey = CloudEpoch/CloudDigest + GeometryProfileId + ExactKeySchemaId
          + PrimitiveSphereKey
SupportKey = PointId triés
SupportRecord = SupportKey + owner + lane + positivité + provenance
BallEvent = BallKey + ExactLevelToken + SupportRecords
            + I_B + U_B + census_complete + disposition + preuve
```

Le fold consomme l'ABI logique et un comparateur exact de niveau. Il ne lit ni
les coordonnées, ni `__int128` natif, ni le nombre de limbs. Le codec persistant
est versionné et indépendant de l'endianness.

### 3.3 Construction transactionnelle

Chaque stade suit :

```text
count -> preflight -> fill -> validate -> seal -> publish
```

Les caps portent séparément sur runs, supports, `I_B`, `U_B`, tâches, octets et
sortie. Un cap moins un produit zéro payload et un statut précis. Le statut
initial est `pending/unclassified`, jamais `regular`.

## 4. Source exhaustive bornée et juge de `0A`

L'oracle petit `n` énumère les supports de cardinalités deux à quatre, puis
recertifie indépendamment :

1. dépendance affine ;
2. centre/miniboule et positivité relative ;
3. clé primitive canonique ;
4. niveau exact ;
5. partition globale `I_B/U_B/exterior` ;
6. owner par `PointId` ;
7. lane `p+q<=smax` et disposition.

Le juge emploie une route rationnelle/multiprécision distincte. Comparer
seulement les signes du census ne juge ni la positivité ni la clé. Les fixtures
u16 maximales, IDs non denses, cosphères, égalités, permutations et caps sont
obligatoires.

## 5. Owner q3 et réduction binaire

Pour un support q3 `S={a,b,x}`, choisir d'abord les arêtes de longueur maximale,
puis la plus petite `EdgeKey=(min PointId,max PointId)`. L'acuité sous cette
arête owner est :

$$\left\Vert 2x-a-b\right\Vert^2>\left\Vert b-a\right\Vert^2.$$

Le support q3 devient donc la relation binaire
`OwnerEdgeKey × CarrierPointId`, suivie de son pied de sphère unique. Le commit
`f516198` reçoit le tie-break borné avec trois relabelings et un mutant
`owner-generationrank`; il ne reçoit ni le compteur `M3`, ni le census, ni le
fold.

La génération candidate utilise des préfixes Morton alignés par échelle
d'arête, puis un range-count LBVH au pied, saturé au neuvième intérieur pour la
lane q3. Les boîtes serrées ne prouvent aucun packing ; tout claim de pente
reste empirique jusqu'au compteur transitif.

## 6. Fenêtre canonique d'arêtes

Pour chaque ancre `a` et lane `q`, `E_q(a)` contient les endpoints `b` que les
certificats disponibles n'ont pas prouvé morts. L'invariant scientifique est :

```text
l'arête maximale canonique de tout support vrai reste dans E_q
```

Un reporter factorisé publie des spans, jamais tous les `PairId` d'un bloc.
Ses fates sont exclusifs :

```text
input_mass = closed_mass + open_mass + pending_mass
```

Une fenêtre finale exige `pending_mass=0`. Les pending ne comptent ni dans la
masse strictement ouverte, ni dans une gate finale.

Aucun cutoff kNN n'est recevable. Des satellites arbitrairement nombreux près
d'une extrémité peuvent repousser l'autre extrémité en rang tout en restant hors
de la sphère support. La coupure doit être géométrique et certifiée.

## 7. Classifieur rectangle corrélé

Pour `e=z-a`, `t=b-z`, poser `H=e dot t`, `E=||e||^2`, `X=||t||^2` :

```text
q2 : H>0
q3 : H>0 et 4*H^2>E*X
q4 : H>0 et 3*H^2>E*X
```

Les égalités ne créditent jamais. Une évaluation rend le masque imbriqué
`q4 => q3 => q2`.

### 7.1 `SOC64`

Former `Ebox=C-A` et `Tbox=B-C`. À l'un des vecteurs fixé, chaque lane est
l'intérieur d'un cône de Lorentz convexe dans l'autre. Si les 64 couples de
coins passent, tout `Ebox×Tbox` passe et donc tout `A×B×C` passe.

Un échec est `UNKNOWN`, jamais `NONE`, car `Ebox/Tbox` oublient le `z` commun.

### 7.2 `CORNER512`

Le spindle est séparément convexe en `a`, `b`, `z`. Le produit continu des
trois AABB est `ALL` si et seulement si ses 512 triples de coins passent. Le
coût adaptatif est `2^(d_A+d_B+d_C)` lorsque certains axes sont dégénérés.

Un coin échouant peut être fictif par rapport aux points du nœud. Il prouve
seulement `AABB_envelope_not_all` et guide un split ; il ne fournit pas un
`NONE` scientifique.

Sous u16, `E`, `X`, `|H|<2^34`; les comparaisons finales demandent jusqu'à 70
bits. CPU `i128` ou deux limbs device sont nécessaires.

## 8. LP projectif : crédit directionnel général

Translater l'ancre en zéro. Pour les témoins `s_i=z_i-a`, la cible `d=b-a`,
`D=||d||^2`, `q_i=||s_i||^2`, définir :

$$\kappa_G(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

Le dual maximise `y dot d` sous `y dot s_i<=q_i`. Un centre de sphère par
`a,b` sans intérieur dans `G` satisfait aussi `y dot d=D`.

### Théorème LP1

`G` crédite au moins un intérieur sur toute sphère par `a,b` si et seulement si
`d` appartient au cône positif de `G` et `kappa_G(d)<D`. L'égalité est ouverte.

Le primal a trois égalités. Après suppression de `s_i=0`, un optimum basique
emploie au plus trois coefficients positifs. Le certificat directionnel
élémentaire a donc au plus trois `PointId`, sans exiger que les témoins entourent
l'ancre.

### 8.1 Fast path q4

Répéter huit fois : résoudre le LP sur le pool restant, extraire une base
couvrante, retirer ses IDs. Un succès donne huit intérieurs distincts pour toute
sphère. L'échec est fail-open.

`O(8P)` n'est qu'une cible espérée avec LP-type exact randomisé en dimension
trois, seed/permutation pinnées. Il ne s'agit ni d'une borne worst-case, ni d'un
coût GPU reçu. Les constructions fraction-free ou 192/256 bits sont nécessaires
si une comparaison de fractions monte vers 137 bits ; les 87 bits de la forme
de vérification ne suffisent pas à borner le constructeur.

### 8.2 Oracle complet de multiplicité

Noter `C_h(P,d)` la propriété « toute sphère par `a,b` possède au moins `h`
intérieurs dans `P` ». Si une base `G` couvre une fois :

```text
C_0(P,d) = vrai
C_h(P,d) <=> pour chaque z dans G, C_(h-1)(P sans z,d)
```

La récurrence est exacte. Les nombres maximaux de LP sont 3280 pour q4, 9841
pour q3 et 29524 pour q2. Elle est complète relativement au pool `P`, et pour le
nuage seulement avec `P=X\{a,b}`. Ce n'est pas un hot path et un résultat
négatif ne réfute aucun support.

### 8.3 Extension à un `BNode`

Pour une base de rang plein trois, Cramer produit trois formes coniques faibles
`n_i dot d>=0` et :

$$F(d)=r\left\Vert d\right\Vert^2-p\mathbin{\cdot}d>0.$$

Sur une boîte entière, les minima linéaires sont aux coins et le minimum de
`F` est séparable, aux deux entiers voisins de `p_k/(2r)` clipés par axe. Le
verdict strict est `min F>=1`.

Une base de rang un ou deux doit être augmentée avec des IDs disponibles,
traitée dans sa dimension ou envoyée à la feuille ; jamais `r=0`. L'arbre de
multiplicité sur un `BNode` est sûr seulement si chaque groupe à chaque nœud est
`ALL` sur toute la boîte et si le chemin d'IDs supprimés est sérialisé. Il n'est
pas complet au niveau rectangle. Un nœud peut payer jusqu'à 3280 LP et 13120
minima avant split ; le fast path seulement huit LP et 32 minima.

## 9. Cages de Voronoï de quatre à six sites

Une cage ancre-globale est un groupe dont les vecteurs relatifs engendrent
positivement `R^3`. Une base positive minimale 3D contient quatre à six
vecteurs. Sa cellule locale bornée a au plus six facettes et huit sommets ; les
formes de fleur ferment un `BNode` lorsque chacune est strictement positive.

Le proposer rapide peut affecter chaque ID à un rôle primaire unique dans des
frames entières, maintenir six queues exclusives et employer des horaires
latins. Six rôles ne garantissent ni une cage ni un octaèdre : chaque groupe est
validé exactement. `P=48` est seulement la capacité de huit groupes de six.

La fixture axiale utilise `G_k={a+/-k e_i}`. Chaque `G_k` est une base minimale
de six et ferme lorsque :

$$\left\Vert d\right\Vert^2>k\left(|d_x|+|d_y|+|d_z|\right).$$

Elle tue le proposer tétra-only pour les témoins axiaux et l'acceptation de
l'égalité. Ses nombres de crédits concernent les groupes alignés `G_k`, jamais
l'optimum parmi tous les appariements.

Une base minimale de six est composée de trois paires de rayons opposés et a
`omega=3`. Le pire minimal est cinq sites avec `omega=4`, donnant le seuil
suffisant `delta>=4h-3`; la fixture
`{(1,0,0),(-1,1,0),(-1,-1,0),(-1,0,1),(-1,0,-1)}` atteint quatre. Une cage
non minimale peut avoir un autre budget, calculé exactement. Réduire une cage
agrandit sa cellule : sommets, fleurs et rayon sont toujours recalculés.

Les formes de fleur demandent environ 87 bits. Certains tris exacts de rayons
rationnels demandent près de 240 bits ; éviter ce tri par majorants conservateurs
ou multiprécision au build, puis garder les formes comme autorité.

## 10. Compteur de formes et moteurs locaux

Une fenêtre sparse ne borne pas le coût aval. Pour chaque arête ouverte `e`, le
compteur `EdgeActiveFormCounter` mesure les sites/formes actifs `m_e` et publie :

$$M_q=\sum_{e\in E_q}m_e.$$

Deux portes sont nécessaires avant tout moteur local : `E_q` et `M_q`, avec
tâches, octets et HWM. Ensuite seulement :

- q3 : un pied unique par carrier, RLE par `BallKey`, range-count saturé à neuf
  et census unique par boule survivante ;
- q4 : niveaux shallow locaux des formes `P-P`, `N-N`, `P-N`, sans arrangement
  complet ; puis positivité, owner, RLE et census.

La complexité shallow en rang constant porte sur les centres distincts et les
bundles pondérés ; elle ne borne pas les `SupportKey` d'une cosphère ni le join
arête×forme avant mesure de `M_q`.

## 11. Dégénérescences

Pour une boule `B`, `I_B` est l'intérieur strict, `U_B` le shell et `S` un
support propre. Dans la branche régulière `U_B=S`, le record direct est unique.
Si `U_B!=S`, la politique candidate du domaine `RelevantGP` rend
`unsupported_degeneracy` pour tout support pertinent tant qu'aucun quotient
complet n'est reçu.

Cette décision dépend de la lane et de `p+q<=smax`; le rang fermé
`|I_B|+|U_B|` ne remplace pas la pertinence. Un `SphereRun` interne peut
conserver le census et un handle de supports pour rendre le layout futur
réversible. Il n'autorise ni un `PlateauEvent` public ni l'omission de la
provenance.

Un quotient saturé doit reconstruire les intersections pondérées, racines
pré-lot, généalogie, lots, coverage et verticales. Une preuve H0 seule ne suffit
pas au `BenchmarkOutputContract-v1`.

## 12. Fold borné `0B`

L'oracle `0A` alimente un spool externe borné. Le pipeline :

```text
BallEvents scellés
  -> tri/merge par ExactLevelToken
  -> macro-lots d'égalité
  -> activations, gateways et coverage
  -> dix forêts horizontales
  -> applications verticales
  -> BenchmarkOutputContract-v1
```

La comparaison porte sur les membres, pas les comptes : clés, supports,
`I_B/U_B`, owners, lots, arêtes de forêts, gateways, coverage et verticales.
Les variantes résidente et spillée doivent être identiques sous chunks de taille
un, coupures au milieu d'un niveau égal, permutations, tilings et reprise.

## 13. Porte industrielle

Une campagne n'est recevable que si elle conserve :

- commit, état du worktree, commande, binaire, environnement et seeds ;
- codes de sortie, sorties brutes et manifeste non auto-référent ;
- ledger exclusif, continuations et statut final ;
- `E3/E4`, `M3/M4`, tâches, visites, splits et opérations larges ;
- `BallKey` brutes/uniques, shells, census, supports, sortie `H` ;
- octets, HWM et temps de chaque phase ;
- 30 nuages frais à 50k, p50/p95/max et chaque valeur brute.

Une pente verte sur quatre tailles mono-graine ne prouve ni linéarité ni borne
du maximum. Une sortie réellement lourde conduit à une analyse output-sensitive,
un quotient reçu ou `resource_exhausted`, jamais à un préfixe silencieux.

## 14. Ordre d'implémentation recommandé

1. réparer `0A` sur tout u16 et tuer ses mutants d'indépendance ;
2. fermer `0B` exhaustif borné ;
3. intégrer `SOC64`, puis `CORNER512` rentable dans le classifieur existant ;
4. comparer LP projectif et cages en counter-only sur le même sink ;
5. gater conjointement `E4`, `M4`, travail, HWM et sortie ;
6. fermer q3 et q4 locaux ;
7. porter count--scan--fill, parité du payload, puis G4 ;
8. seulement après succès 50k, qualifier les paliers de cardinalité suivants.

Les preuves détaillées et leur contre-audit sont dans
[`audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

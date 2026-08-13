# Réponse à la rétractation `s=2` et à la prétendue loi en `K`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin observé : `3d07be1759afffe4c72b45615f114a54cbccc5b9`, worktree concurrent.
GCP non utilisé par l'auditeur.

## 1. Verdict

La rétractation du refus de `s=2` est correcte : le compteur antérieur ne
pouvait pas choisir `s`. La conclusion qui le remplace va cependant encore trop
loin.

- **OUI**, la disjonction de deux certificats `ALL` suffisants est sûre sous les
  conditions précisées ci-dessous.
- **NON**, les nouvelles tables ne reçoivent ni `s=2` comme baseline produit,
  ni une fenêtre projective, ni `Theta(K)`, ni l'indépendance de `s` et `K`.
- **NON**, la fenêtre mesurée n'est toujours pas le `kept` consommé par
  `anchor_source`.
- La grille courante peut rester un diagnostic du certificat central q2. Elle
  ne doit plus retarder `BallFormToBallEvent-v0` et
  `PWC0-A/MaxEdgeSuffixReporter-q4-v0`.

## 2. Quand l'OR de certificats est exact

Fixer un rectangle `A×B`, un nœud témoin `C` et une lane `q`. Supposons que
deux prédicats `P` et `Q` vérifient chacun :

```text
P(A,B,C,q)=ALL => tout z dans C ferme la lane q de toute paire (a,b) dans A×B
Q(A,B,C,q)=ALL => tout z dans C ferme la lane q de toute paire (a,b) dans A×B
```

Alors `P=ALL ou Q=ALL` implique la même propriété universelle. Aucune
comparabilité des deux domaines certifiés n'est nécessaire. Le worktree appelle
le fallback seulement lorsque le central vaut `MIXED`; un nœud ne peut donc pas
être crédité par les deux branches au même appel.

Quatre invariants restent obligatoires :

1. `NONE` ou `MIXED` d'un certificat ne devient jamais `GEOMETRIC_NONE` pour
   l'autre ;
2. une lane créditée au parent ne redescend pas dans ses enfants ;
3. les nœuds crédités forment une antichaîne, donc chaque `PointId` compte au
   plus une fois par `(RectId,lane)` ;
4. un cap ou overflow émet la continuation ouverte et ne prononce aucun `ALL`.

Sous ces invariants, la disjonction est mathématiquement reçue comme prune. Le
juge industriel développe les nœuds crédités et rejoue le prédicat ponctuel
indépendant sur tout `a×b×z` au petit oracle.

## 3. Le compteur reste le mauvais objet

La colonne appelée fenêtre est toujours construite ainsi : tout terminal WSPD
q2 non fermé ajoute `|B|` au degré de chaque `a` dans `A` et `|A|` au degré de
chaque `b` dans `B`. Son identité est donc :

```text
sum_N = 2*(total_pair_mass - mass_closed_q2)
```

Le fallback change `mass_closed_q2`, donc il change cette colonne. Il ne la
transforme pas en reporter projectif. Il manque toujours :

- `GroupCredit`, unions de vrais `PointId` et disjonction rejouable ;
- 48 chambres et raffinement des chambres ouvertes ;
- lane q4 et seuil `h_4` ;
- orientation unique `a<b`, `OpenEdgeSpan` et continuation ;
- inclusion des arêtes maximales canoniques des vrais supports.

Le facteur deux est matériel. Sous l'orientation du futur `E_q(a)`, les
moyennes q2 annoncées à `smax=11`, `813,8 / 982,4 / 986,2`, deviennent seulement
`406,9 / 491,2 / 493,1`. Elles ne sont toujours ni projectives ni comparables à
`hw_kept`, qui est un maximum de sites `z` par paire `(a,b)`.

## 4. q2 ne décide pas la source q3/q4

Sur le rejeu local `uniform,sep=2/1,tight,window=256` :

| `n` | fallback | vague CPU | fermés q2 | fermés q3 | fermés q4 |
| ---: | :---: | ---: | ---: | ---: | ---: |
| `2000` | non | `517,1 ms` | `16313` | `263` | `130` |
| `2000` | oui | `1732,2 ms` | `22384` | `277` | `132` |
| `4000` | non | `1566,8 ms` | `41235` | `569` | `241` |
| `4000` | oui | `4374,8 ms` | `54949` | `598` | `271` |

La masse q2 fermée progresse fortement, mais la fermeture q3/q4 progresse de
quelques dizaines de records pour un facteur `2,8..3,4` sur la vague CPU. Le
contrat source est précisément bloqué par q3/q4. La pente du complément q2 ne
peut donc ni choisir l'ordonnance source, ni prouver que son travail est
`Theta(Kn)`.

## 5. Deux seuils ne donnent aucune loi en `K`

Pour un certificat fixé, noter `c(a,b)` le nombre de crédits disjoints reçus.
La fenêtre au seuil `h` est simplement :

```text
E_h = { (a,b) : c(a,b) < h }
```

Sa taille est une fonction en escalier de `h`. Sans hypothèse sur l'histogramme
des `c(a,b)`, elle n'est ni Lipschitz, ni linéaire. Si presque toutes les paires
ont exactement `h` crédits, passer du seuil `h` au seuil `h+1` ouvre presque
toutes les paires d'un coup. Un incrément de `K` peut donc changer la fenêtre de
`O(n)` à `Theta(n^2)` pour ce certificateur. À l'inverse, si les comptes sont
loin des seuils, plusieurs incréments ne changent rien.

Les rapports numériques `10/6` et `986,2/602,3` ne constituent ainsi aucune
preuve. Ils portent sur deux seuils, une famille, trois tailles et le certificat
q2 non projectif. L'écriture `|N_q(a)|=Theta(K)` puis
`sum_a|N_q(a)|=Theta(Kn)` doit être rétractée ; même la notation `Theta` est
injustifiée par des tailles finies et un seul `K` variable.

## 6. `s` et `K` peuvent interagir

La séparation `s` change la partition en rectangles, leurs boîtes et la force
des bornes universelles. Le seuil `h_q` change le nombre de crédits requis. Rien
n'impose que ces deux effets se factorisent. Une séparation plus forte peut
réduire le nombre de crédits nécessaires à découvrir pour certains rectangles
et multiplier le nombre de rectangles pour d'autres ; déplacer `h_q` peut
changer exactement lesquels passent. L'indépendance de `s` et `K` n'est donc
pas un théorème.

Décision honnête : `s=2` redevient une **ablation non réfutée** du tape WSPD,
pas une baseline reçue. Comparer `s` seulement sur le vrai coût composé :

```text
W_front + W_credit_bank + W_report_q4 + W_open_spans + W_shallow + W_ball + W_fold
```

Le premier choix peut se faire à `smax=11`, profil contractuel. Toute extension
à d'autres `smax` est une nouvelle ablation ; elle ne découle pas d'une loi en
`K`.

## 7. Réponse à `eight_clusters`

Il ne faut pas « attendre » la même proportionnalité. Sans modèle de densité,
aucune famille ne la garantit. `eight_clusters` est justement adversariale : le
centre de certaines sphères tombe dans les vides inter-amas, le producteur
courant ne ferme aucun front utile et sa boucle q4 est déjà cubique à petite
taille.

Le vrai reporter doit donc être mesuré au minimum sur `uniform` et
`eight_clusters`, avec plusieurs graines et les mêmes emprises canoniques. Les
portes portent par lane sur `sum|E_4|`, tâches, activations, spans, octets et
HWM, plus deux pentes consécutives. Une loi ajustée sur `uniform` ne dispense
jamais cette gate.

## 8. Directive immédiate

1. Conserver l'OR central/fallback comme ablation sûre, mais ne pas le porter
   sur CUDA : son rendement q3/q4 local est rouge.
2. Arrêter d'appeler son complément q2 `fenêtre projective` ou coût naturel de
   la source.
3. Implémenter d'abord `BallFormToBallEvent-v0` pour obtenir
   `(BallKey,SupportKey,I_B,U_B)` et la fixture cosphérique.
4. Implémenter ensuite `PWC0-A` q4, avec vrais crédits, spans et continuation.
5. Mesurer la grille `s×smax` seulement sur ce reporter et le coût composé, si
   le profil autre que `smax=11` reste une cible explicite.

Le contrat `50000/1s` reste entièrement ouvert.

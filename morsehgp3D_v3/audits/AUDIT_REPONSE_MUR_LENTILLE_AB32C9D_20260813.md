# Réponse au pin `ab32c9d` — mur de lentille, fenêtre en `O(F+n)` et vraie réparation q4

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin observé : `ab32c9df7baf9d91cfe8933cf05c600fe36ac683`, commit
`the wall is the lens, and it is a dimensionality effect aimed at our target regime`.
Ce commit ne modifie que la documentation d'audit ; l'objet logiciel reste
celui du parent `7617eb9`. Au relevé initial, le worktree portait seulement des
compléments documentaires concurrents dans les chemins autorisés. Aucun code
n'a été modifié par l'auditeur et GCP n'a pas été utilisé.

## Verdict direct

Le diagnostic **change la priorité, pas le verdict sur le front de triplets**.

1. Oui : la boucle q4 `for i in lens; for j>i` doit sortir immédiatement du
   chemin candidat. Le high-water croissant sur les familles structurées et la
   divergence entre temps et sorties rendent cette ordonnance incompatible
   avec le contrat.
2. Non : le seul high-water ne prouve ni que le mur est *entièrement* expliqué
   par la lentille, ni que `uniform` possède une lentille asymptotiquement
   bornée, ni une loi intrinsèque bidimensionnelle.
3. Non : le front de triplets aigus soumis au pin `4ce3618` ne remplace pas le
   produit q4. Son certificat par distance maximale au hull reste identiquement
   vide. Une cellule de carriers q3 ne représente qu'un co-sommet `x`; q4 doit
   encore joindre un porteur aigu `x` à un second site de lentille `y`.
4. La réparation mathématique est le **join factorisé des formes actives**, puis
   les niveaux shallow locaux `P-P/N-N/P-N`. Une décomposition en cellules peut
   accélérer ce join ; elle ne remplace ni le niveau, ni le replay exact.

La route admise reste donc :

```text
BallFormToBallEvent-v0 borné, pour l'autorité BallKey/I_B/U_B
  -> CanonicalEdgeWindowReporter-q4-v0
  -> EdgeActiveFormCounter-v0, relation factorisée arête x site
  -> LocalShallowBall-v0, sans C(|lens|,2)
  -> BallKey/RLE -> census unique -> fold streamé
```

Le moteur shallow peut être développé tout de suite comme oracle différentiel
borné sur une arête. Son admission dans le hot path reste conditionnée aux deux
portes `E_4` et `M` ; aucun nouveau run du producteur toutes-paires n'est utile.

## 1. Ce que le reçu mesure réellement

Le code courant incrémente bien `q4_pairs_walked` dans la double boucle q4. Il
calcule cependant `hw_lens` avant cette boucle, dès que q3 **ou** q4 reste
vivant. Le record qui réalise `hw_lens=7811` peut donc être q3-only. La phrase
« une seule paire coûte `C(7811,2)` » n'est pas reçue sans un
`hw_lens_q4` séparé.

Plus fondamentalement, un maximum ne donne pas le travail total. La quantité
causale est :

```text
Q4 = sum_(arêtes q4 vivantes e) C(lens_e, 2).
```

Le compteur `q4_pairs_walked` la calcule déjà, mais le script du reçu n'a gardé
que les six dernières lignes de chaque run et a supprimé cette ligne ainsi que
`interior_tests`. Le prochain diagnostic borné doit publier, par taille :

- nombre d'arêtes q4 vivantes ;
- `sum lens_e`, `sum lens_e^2`, p50, p95, p99, maximum et
  `q4_pairs_walked` ;
- `q4_candidates`, `interior_tests`, `SupportKey` et temps des phases ;
- histogramme séparé des motifs de sortie de la boucle.

L'égalité empirique de pentes sur `terrain` est suggestive, mais l'identité
`1+2*0,85=2,7` suppose encore que le nombre d'arêtes vivantes croît comme `n`,
que la distribution entière se dilate comme son maximum et que les censuses ne
changent pas de régime. Aucun de ces trois faits ne suit du high-water.

Le reçu prouve néanmoins assez pour arrêter l'architecture actuelle : les
sorties achevées croissent près de linéairement tandis que le temps explose, et
le code contient explicitement la double boucle. Il n'est pas nécessaire de
réexécuter une source déjà réfutée pour décider de la remplacer.

## 2. L'effet de dimension est une hypothèse mesurable, pas un théorème du contrat

Pour des cellules dyadiques disjointes de côté `D/s` dans une région de
diamètre `O(D)`, le packing inconditionnel tridimensionnel donne `O(s^3)`
cellules. Une borne `O(s^2)` demande une hypothèse supplémentaire du type :

```text
pour toute boule B(x,r) et toute échelle ell,
le nuage occupe au plus C_surface * (r/ell)^2 cellules ell dans B(x,r).
```

Cette hypothèse exprime une surface à mesure locale, multiplicité de plis et
épaisseur bornées. Une nappe arbitrairement plissée peut traverser la même
lentille un nombre non borné de fois et occuper `Theta(s^3)` cellules. Le
contrat des familles de nuages n'impose aujourd'hui ni régularité d'Ahlfors,
ni reach, ni constante de doublement locale.

Les quatre valeurs `256/254/306/318` ne prouvent pas non plus que la lentille
`uniform` est bornée. `eight_clusters` contient des amas volumétriques mais son
maximum croît déjà ; la dimension intrinsèque seule ne peut donc expliquer les
mesures. La sélection d'arêtes longues, les vides entre amas, la densité locale
et la politique de cutoff contribuent aussi.

Une adaptation dimensionnelle reste une bonne optimisation conditionnelle :
publier le nombre de cellules occupées de lentille par échelle, la dimension de
packing estimée et la constante observée. La gate industrielle demeure
inconditionnelle sur les cinq familles et plusieurs graines. Les cellules de
packing sont les cellules canoniques ; des boîtes serrées ou un raffinement
`MIXED` non borné n'héritent pas automatiquement de la borne.

## 3. Pourquoi un triplet aigu ne remplace pas q4

Pour une arête `ab`, noter `L` tous les sites de la lentille et `P` les sites
dont la face `abx` est aiguë. Un support q4 emploie un couple `(x,y)` dans
`P×L`, avec au moins un bit aigu, puis vérifie `xy`, rang, positivité, owner et
census.

Un bloc `(A,B,C)` du front q3 décide seulement la relation du premier site
`x`. Il ne représente pas le second site `y`, et l'énumération des blocs q3 ne
supprime donc pas `P×L`. L'alternative exacte est de représenter chaque site de
`L` par une forme orientée dans le plan médiateur de `ab`, avec un bit `acute`
pour `P`, puis de ne visiter que les sommets de faible niveau incidents à au
moins une forme aiguë.

Le certificat par hull du pin `4ce3618` reste inutilisable : le hull contient
les deux endpoints les plus proches, donc sa distance maximale à tout `z` est
au moins `Dmin/2`. La stricte proposée ne passe jamais et son échec n'est pas un
`NONE`. Une factorisation par cellules peut toutefois devenir un index du
futur `EdgeActiveFormCounter-v0`, à condition que tout `MIXED` soit transmis et
que le travail réel `J`, les formes `M` et la HWM soient publiés.

## 4. Remplacement reçu du certificat q3 vide

Le constat d'acuité fournit malgré tout un certificat q3 utile et beaucoup plus
simple. Poser, pour une paire ponctuelle :

```text
D   = ||b-a||^2
Phi = ||2z-a-b||^2
m   = (a+b)/2.
```

Pour tout triangle q3 positif dont `ab` est une arête maximale faible, son
circumcentre `o` vérifie `||o-m||^2 <= D/12`. L'identité de puissance donne un
minorant de la marge intérieure. Sur un rectangle `A×B×C`, avec `Dlo/Dhi` les
extrema de `D`, `Phi_hi` le maximum de `Phi` et `Phi_lo` son minimum exact sur
le réseau entier u16, le test suivant est un `ALL` sûr :

```text
L = Dlo - Phi_hi
L > 0  &&  3*L*L > 4*Phi_hi*Dhi.
```

Tout point intérieur vérifie en outre `Phi < 3D`. Le test suivant est donc un
`NONE` sûr, égalité comprise :

```text
Phi_lo >= 3*Dhi.
```

`Phi_hi` se calcule par extrémités. `Phi_lo` doit respecter le réseau entier et
la parité de `2z-(a+b)` ; un minimum continu n'est qu'un minorant fail-open.
`D`, `Phi` et `L` tiennent en `i64` sous u16, mais les produits croisés exigent
`i128` ou deux limbs device, avec promotion avant multiplication.

Les strictetés sont nécessaires. Pour
`a=(0,0,0)`, `b=(6,6,0)`, `x=(0,6,-6)`, le triangle est équilatéral. Avec
`z=(4,2,2)`, on a `D=72`, `Phi=24=D/3` et `z` est sur la coquille : remplacer
la stricte `ALL` par une égalité produit un faux crédit.

Ce certificat peut remplacer le hull vide dans la lane q3 de la wavefront. Il
certifie des témoins intérieurs de toutes les circumboules q3 owner du bloc ;
il ne génère aucun triangle et ne remplace jamais la relation q4 `P×L`.

## 5. Le vrai maximum de fenêtre coûte `O(F+n)`, pas la masse

La note du parent `7617eb9` affirme que le maximum de `E_q(a)` exigerait de
développer `|A||B|` pour chaque terminal WSPD. C'est faux dans l'ordre déjà
choisi.

Chaque nœud du radix tree porte une plage contiguë `[first,last]` dans
`GenerationRank=(Morton48,PointId)`. Les graines WSPD sont des plages sœurs
disjointes ; les splits les remplacent par des sous-plages. Tout terminal
`R=A×B` satisfait donc exactement l'un des deux ordres `last(A)<first(B)` ou
`last(B)<first(A)`.

Pour chaque terminal ouvert dans la lane `q`, faire deux mises à jour d'un
tableau de différences signé :

```text
si last(A) < first(B):
    diff[first(A)]  += |B|
    diff[last(A)+1] -= |B|
sinon:
    diff[first(B)]  += |A|
    diff[last(B)+1] -= |A|
```

Un scan préfixe donne exactement `|E_q(a)|` pour chaque rang, son maximum et sa
somme. La somme doit valoir `sum_R |A_R||B_R|` sur les rectangles ouverts. Le
coût est `O(F_open+n)`, la mémoire `n+1` compteurs `i64`, soit environ `400 kB`
à `n=50 000`. Un scatter par `spid[rank]` suffit si le consommateur indexe par
`PointId`.

Cette réduction demande un `closed_mask/fate` par terminal et par lane. Le code
courant ne conserve que `closed_q2`; q3/q4 ne sont que des agrégats. Si une
continuation reste pendante, le scan donne un superset fail-open, pas la
fenêtre finale. La gate exige :

- plages valides, disjointes et totalement ordonnées ;
- `sum(prefix)==sum_R |A_R||B_R|` et `max<=n-1` ;
- oracle petit `n` développant chaque PairId exactement une fois et comparant
  tout le vecteur de degrés ;
- mutant orientation par `PointId` au lieu de `GenerationRank` ;
- `pending=0` avant publication de la fenêtre finale.

`EdgeWindowRangeAdd-v0` est donc le prochain compteur exact le moins risqué. Il
donne à Claude le vrai `sum/max E_4` sans réintroduire le produit PairId.

## 6. Micro-jalons vers une seconde

Ordre recommandé, avec arrêt immédiat sur métrique rouge :

1. Ajouter les fates par lane et `EdgeWindowRangeAdd-v0`; recevoir son oracle
   exact sans développer la masse dans le chemin candidat.
2. Garder `BallFormToBallEvent-v0` comme oracle output-bearing borné. Il fixe
   `BallKey`, `I_B/U_B`, owner et l'identité que le shallow doit reproduire.
3. Recevoir `PWC0-A` puis `EdgeActiveFormCounter-v0`. Le second publie
   `M=sum_e m_e`, tâches du dual-tree, blocs, hits, octets/HWM et
   continuations ; `M` seul ne borne pas les tâches.
4. Implémenter `LocalShallowBall-v0` par bundles de droites confondues et
   niveaux stricts. Pour `k=7-credit4`, les `16` curseurs et `36` canaux de
   rang sont des états simultanés, pas une borne sur toutes les intersections.
   Publier aussi `J`, concurrences `H`, opérations de segments et sorties.
5. Seulement après parité avec l'oracle borné : port device résident,
   `count--scan--fill`, fold streamé et rampe `12 500/25 000/50 000` avec p95,
   octets et HWM.

La décision à retenir est donc : **le mur de lentille élève le moteur shallow,
pas le certificat de triplets soumis**. Le gain structurel vient de
l'arrangement implicite et de la factorisation des formes, non d'une hypothèse
non reçue selon laquelle toute nappe occuperait `O(s^2)` cellules.

GCP non utilisé par l'auditeur.

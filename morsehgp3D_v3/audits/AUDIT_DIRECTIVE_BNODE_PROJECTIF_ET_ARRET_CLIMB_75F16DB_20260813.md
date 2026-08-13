# Directive après `75f16db` : arrêter la recherche locale, fermer des `BNode` projectifs

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin logiciel audité :
`75f16db981bcbce262cf940d68fd5550be986c2a`, commit
`locate-and-climb gives nine percent, and that settles where the factor must
come from`. L'auditeur n'a modifié aucun fichier logiciel et n'a pas utilisé
GCP.

## 1. Verdict utile à Claude

Le résultat du pin est une bonne décision négative : optimiser encore une
recherche `C=root` indépendante pour chacun des `F` rectangles ne peut pas
fournir le facteur manquant. Le compteur `climb` enlève environ `9,1 %` des
classifications sur l'unique diagnostic `uniform,n=8000,s=2`, mais il ferme
moins de records et ne donne pas de gain CPU robuste. Il doit rester une
ablation positive fail-open, pas devenir le chemin produit.

La prochaine tranche est `ProjectiveWindowCounter-v0`, avant CUDA et avant le
shallow. Elle travaille par ancre, construit des groupes d'IDs disjoints,
ferme des **spans de cibles** par des formes affines exactes et mesure la vraie
fenêtre dirigée `N_q(a)`. La première implémentation doit employer les suffixes
des 48 chambres ; les 432 sous-cellules et le classifieur direct par triple
sont des ablations de rappel. Si la fenêtre reste dense, cette route est
`NO-GO` avant toute matérialisation de `PlaneTape`.

`QueryTree×PointTree` reste le fallback factorisé pour les résidus que la
fenêtre projective ne compresse pas. Il ne doit pas précéder le compteur de
fenêtre : son nombre de tâches `J` peut encore atteindre le produit des deux
fronts, tandis que le reporter projectif exploite une propriété propre aux
sphères positives.

## 2. Ce que `--climb` couvre réellement

Soit `l` la feuille choisie par le `lower_bound` Morton et
`v_0=l,v_1,...,v_k=root` sa chaîne d'ancêtres. Les frères de
`v_0,...,v_{k-1}` sont deux à deux disjoints et leur union vaut exactement
`Leaves(root)\setminus\{l\}`. Le code du pin empile ces frères, mais **pas la
feuille `l`**. Il ne parcourt donc pas une partition complète de l'arbre
témoin. Cette omission ne peut créer qu'une perte de crédit : chaque fermeture
repose toujours sur des nœuds `ALL` disjoints. Elle interdit en revanche tout
claim de complétude du certificat central.

L'ordre est également contraire au récit de proximité. Les frères sont
empilés de la feuille vers la racine ; la pile LIFO dépile d'abord le dernier,
donc le gros frère proche de la racine. Ce sous-arbre peut consommer le quantum
avant les petits frères voisins de la feuille. Le `lower_bound` dans l'ordre
Morton n'est par ailleurs ni un plus proche voisin euclidien, ni un minimum du
score central `S`.

Une version de diagnostic qui souhaite couvrir exactement l'arbre ajoute la
feuille localisée comme tâche et grave l'identité suivante :

```text
{feuille localisée} disjoint-union {frères de sa chaîne} = C-root
```

Elle publie `initial_tasks`, `unique_leaf_mass`, `omitted_leaf`, ordre des
tâches, `tasks_pending` et un digest de couverture. Pour viser le rappel, elle
traite la feuille et les petits frères avant les gros ; cela reste toutefois
une heuristique d'ordonnancement. Un cap sérialise les tâches restantes ou les
délègue, il ne se contente jamais d'incrémenter `tronques`.

### Mesure locale reproductible

Au pin, les cinq CTests `mhgp3v_wspd_wavefront_*` passent en `1,15 s`. Le juge
manuel combiné avec `--climb` examine `7193` fermetures à `n=1200` et ne trouve
aucun faux positif. À `uniform,n=8000,s=2,window=256` :

| mode | classifications | q2/q3/q4 fermés | masse q2 | tronqués |
| --- | ---: | ---: | ---: | ---: |
| racine | 30 422 095 | 104 237 / 2 455 / 1 288 | 66,43 % | 0 |
| climb | 27 645 707 | 97 822 / 1 989 / 1 009 | 65,22 % | 3 |

Trois exécutions one-shot sur la machine partagée donnent une médiane CPU de
`4555,1 ms` depuis la racine et `4506,0 ms` en climb, soit environ `1,1 %` et
non `9 %`. Une paire exécutée juste avant donnait même `2798,2/3366,8 ms` en
sens inverse. Ces temps bruités ne sont ni un p95, ni du device ; ils suffisent
à refuser de poursuivre l'optimisation locale avant le partage inter-requêtes.

## 3. Certificat projectif ponctuel, puis caractérisation exacte

Fixer une ancre `a`, une cible distincte `b`, `d=b-a` et un groupe de témoins
`G={s_i=z_i-a}`. Si `d` appartient au cône positif de `G` et si
`d·s_i>||s_i||^2` pour chaque membre employé, alors, pour toute sphère passant
par `a,b`, au moins un membre de `G` est strictement intérieur. La preuve est
l'identité reçue :

$$\sum_i\lambda_i\left(2t\mathbin{\cdot}s_i-\left\Vert s_i\right\Vert^2\right)=\left\Vert d\right\Vert^2-\sum_i\lambda_i\left\Vert s_i\right\Vert^2>0.$$

Les coefficients sont non négatifs ; certains peuvent être nuls. La stricte
positivité vient d'au moins un coefficient positif, puisque `b!=a`. Avec
`h_q=smax+1-q` groupes dont les unions de `PointId` sont deux à deux
disjointes, toute sphère possède au moins `h_q` intérieurs distincts et la
paire est fermée pour la lane.

Cette condition membre par membre est sûre, mais elle n'est pas nécessaire.
Pour un triple plein rang, Farkas donne une caractérisation ponctuelle plus
forte. Écrire `d=sum_i lambda_i*s_i`. Il existe un centre de sphère passant par
`0,d` pour lequel aucun `s_i` n'est intérieur si et seulement si le système

```text
2*t dot d = ||d||^2
2*t dot s_i <= ||s_i||^2 pour i=1,2,3
```

est réalisable. Lorsque tous les `lambda_i` sont non négatifs, le maximum de
`sum_i lambda_i*(2*t dot s_i)` sous les trois inégalités vaut
`sum_i lambda_i||s_i||^2`; s'il existe un coefficient négatif, l'expression
n'est pas bornée dans le sens requis et l'égalité reste réalisable. Le triple
couvre donc toute sphère par `0,d` si et seulement si :

```text
lambda_i >= 0 pour tout i
||d||^2 > sum_i lambda_i ||s_i||^2.
```

Cette caractérisation est exacte. Les strictes H2 individuelles constituent
seulement un fast path `i64` qui l'implique.

## 4. Trois formes et une quadratique exactes sur un `BNode`

Soit `G={s_1,s_2,s_3}` et `Delta=det(s_1,s_2,s_3)!=0`. Poser
`sigma=sign(Delta)` et :

```text
n1 = sigma * (s2 cross s3)
n2 = sigma * (s3 cross s1)
n3 = sigma * (s1 cross s2)
```

Par Cramer, `d` appartient à `cone(G)` si et seulement si les trois formes
**faibles** `n_j·d>=0` passent. Les coefficients nuls sont légitimes : ils
décrivent les faces du cône et ne doivent jamais être rendus stricts.

Poser `r=|Delta|`, `q_i=||s_i||^2` et
`p=q_1*n_1+q_2*n_2+q_3*n_3`. Comme
`lambda_i=(n_i·d)/r`, la seconde condition exacte devient :

```text
F(d) = r*||d||^2 - p dot d > 0.
```

Si le nœud cible porte la boîte de différences
`D_B=[B.lo-a,B.hi-a]`, les minima des trois formes coniques sont affines et
exacts aux extrémités. Le minimum de `F` est également exact en temps constant,
car la fonction est séparable :

$$\min_{d\in D_B\cap\mathbb{Z}^3}F(d)=\sum_{k=1}^{3}\min_{x\in[L_k,U_k]\cap\mathbb{Z}}\left(rx^2-p_kx\right).$$

Pour chaque axe, il suffit de tester les deux entiers voisins de
`p_k/(2r)`, clipés à l'intervalle. La division signée doit être une vraie
division plancher, jamais la troncature C++ vers zéro. Une implémentation device
peut aussi localiser le changement de signe de
`f(x+1)-f(x)=r(2x+1)-p_k` par comparaisons larges.

Le triple couvre exactement toute sphère passant par l'ancre et chaque cible
du `BNode` si et seulement si :

```text
min_D_B(n_j dot d) >= 0       pour j=1,2,3
min_D_B F(d) >= 1.
```

Cette équivalence est exacte sur le réseau entier de la boîte. Pour la
population réelle du nœud, un échec signifie seulement `MIXED/OPEN` : la boîte
peut contenir des directions absentes. Sous split de `B`, un `ALL` s'hérite ;
un échec se reclasse.

Les six formes de la version suffisante restent utiles comme préfiltre léger :
trois cônes faibles et trois H2 strictes. Elles tiennent toutes en `i64`, mais
ne doivent être appelées ni nécessaires ni complètes. Exemple : pour
`G={e1,e2,e3}` et `d=(3,1,1)`, `F(d)=6>0` et le cône passe, alors que deux H2
individuelles sont à égalité. Le triple couvre bien toutes les sphères et le
préfiltre à six formes le manquerait.

### Largeurs reçues sous u16

Avec `M=65535`, produits scalaires et normes sont au plus `3M^2`; une valeur
de H2 peut atteindre `-6M^2`; un coefficient de produit vectoriel est au plus
`2M^2`; déterminants et extrema coniques sont au plus `6M^3`, soit moins de
`2^51`. Ces parties tiennent en `i64` si chaque soustraction et chaque produit
y est promu avant calcul. Il ne faut jamais tester `Delta*det>=0`, dont le
produit peut demander environ 102 bits : on branche une fois sur le signe de
`Delta`.

Le test exact `F` est plus large. Chaque composante de `p` est bornée par
`18M^4`, `|p·d|` par `54M^5`, et `|F|` par `72M^5<2^87`. Un entier signé 128
bits suffit ; CUDA emploie deux limbs et des comparaisons signées reçues. Aucun
cast `i64` intermédiaire n'est admissible. Pour cette raison, le suffixe
cellulaire et le fast path H2 restent le P0 `i64`; `F` est l'ablation exacte de
rappel tant que son coût device n'est pas mesuré.

## 5. Groupes arbitraires : correction de l'autre audit

La phrase « au plus six formes affines » n'est vraie que pour le certificat
suffisant H2 d'un triple plein rang. La caractérisation exacte du même triple
emploie trois formes affines et une quadratique séparable. Un groupe arbitraire
peut porter jusqu'à une forme H2 par membre et autant de
facettes coniques que de rayons extrêmes ; son enveloppe peut donc demander
jusqu'à environ `2|G|` formes en dimension trois. De même, `6h<=60` suppose
`smax<=11` ; le domaine CLI historique monte à `34`, donc `h` peut atteindre
`33`.

Une alternative fixe, sûre et plus simple à rejouer évite de construire cette
H-représentation. Pour chacun des huit coins de `D_B`, trouver par
Carathéodory un carrier conique exact de taille un à trois dans le groupe.
Prendre l'union canonique de ces carriers, soit au plus 24 IDs, puis exiger la
stricte H2 uniforme pour chacun d'eux. Toute direction de la boîte est une
combinaison convexe de ses coins et donc une combinaison conique de cette
union. Ce chemin est un bon oracle et un fallback CPU ; le suffixe cellulaire
reste le premier reporter GPU, car ses formes sont fixes.

Les rangs un et deux vivent sur leurs strates exactes. Un déterminant nul ne
doit jamais être promu en triple plein rang ; il est traité par les carriers de
rang inférieur déjà reçus ou reste ouvert.

## 6. Fixtures et mutants permanents

Les fixtures suivantes doivent précéder toute mesure de fenêtre :

1. **représentant insuffisant** : avec
   `G={(6,1,0),(6,-1,0),(6,0,1)}` et
   `D_B={9}×[0,2]×{1}`, `d=(9,0,1)` est dans le cône mais `d=(9,2,1)` en sort.
   Les trois H2 restent strictes sur toute la boîte. Tester seulement le
   représentant fermerait donc faussement le `BNode` ; les minima coniques le
   laissent ouvert ;
2. **frontière conique admise** : pour le même groupe,
   `d=(12,1,1)` a des coefficients `(1,0,1)`. Le coefficient nul est légitime
   et le verdict conique doit être positif ;
3. **quadratique stricte** : `G={e1,e2,e3}`, `d=(1,1,1)` donne `F=0`. La
   sphère centrée en `(1/2,1/2,1/2)` place les trois témoins sur sa coquille ;
   remplacer `F>0` par `F>=0` crée un faux crédit ;
4. **H2 seulement suffisant** : pour le même groupe, `d=(3,1,1)` donne
   `F=6>0`, mais deux strictes H2 individuelles échouent à égalité. Le chemin
   H2 reste ouvert et le chemin exact ferme ; les deux verdicts ne doivent pas
   être exigés égaux ;
5. **minimum intérieur et division signée** : des fixtures scalaires placent
   le sommet de `r*x^2-p*x` strictement dans l'intervalle, puis autour d'un
   demi-entier négatif. Un mutant qui ne teste que les extrémités ou emploie la
   troncature C++ doit être tué ;
6. **huit coins** : chaque coin passe avec un carrier reçu, puis un mutant en
   omet un. Le replay développe la boîte et trouve une direction non couverte ;
7. **identités** : deux groupes partageant un `PointId` ne comptent jamais
   pour deux crédits ; changement de lane, `smax`, `Epoch` ou digest invalide
   le replay.

L'oracle borné développe toutes les cibles du `BNode`, toutes les sphères
admissibles de sa fixture rationnelle et compare la conclusion, pas seulement
un second appel aux six formes.

## 7. ABI et ordre d'implémentation remis à Claude

Le record de preuve minimal est typé :

```text
ProjectiveGroupCredit = {
  schema, CloudDigest, PointTreeDigest, Epoch,
  AnchorId, CreditId, rank, member_PointIds, member_count,
  cone_forms, h2_forms, activation_or_region_key, proof_digest
}
ProjectiveTargetTask = {
  AnchorId, BNodeKey, open_lane_mask, possible_cell_mask, credit_span
}
ProjectiveTargetResult = {
  AnchorId, BNodeKey, closed_mask, open_mask, proof_spans, reason
}
```

Pour chaque `(AnchorId,BNodeKey,lane)`, les `h_q` crédits consommés ont des
unions d'IDs disjointes. Un même crédit peut servir à plusieurs `BNode`, mais
jamais deux fois dans une même fermeture. `closed_mask` et `open_mask`
partitionnent le masque d'entrée ; capacité insuffisante ou classification
partielle produit `OPEN_SPAN`, jamais fermeture implicite.

Ordre recommandé :

1. réutiliser l'enveloppe projective et les activations entières déjà reçues,
   mais conserver la banque bornée comme proposer uniquement ;
2. construire `ProjectiveWindowCounter-v0` sur 48 chambres, avec reporter
   `Anchor×BNode`, suffixe de hauteur et spans ouverts ;
3. ajouter le test direct à six formes pour les triples plein rang comme
   ablation de fermeture inter-cellules ;
4. publier `sum_a|N_q(a)|`, `max_a|N_q(a)|`, tâches, formes, IDs distincts,
   continuations, octets/HWM et pentes à `12500/25000/50000` sur toutes les
   familles ;
5. seulement si ces portes passent, matérialiser le `PlaneTape`, puis écrire
   `LocalShallowBall-v0`, `BallKey` et census global ;
6. si la fenêtre est dense, arrêter cette route et mesurer le fallback
   `QueryTree×PointTree`, sans jamais développer `PairId×PointId`.

Cette tranche évite la mosaïque de Delaunay d'ordre supérieur, les recherches
`C=root` par rectangle et toute expansion précoce de supports. Elle ne reçoit
encore ni le chemin complet, ni le SLO `50000/1s`.

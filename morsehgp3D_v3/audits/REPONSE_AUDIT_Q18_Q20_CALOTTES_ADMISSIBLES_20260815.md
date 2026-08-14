# Réponse d'audit Q18--Q20 — directions admissibles et localité

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette réponse n'autorise aucune Delaunay, d'aucun ordre, et ne reçoit aucun
logiciel. Les seuils et les certificats de `Lane2`, `Lane3` et `Lane4` restent
autonomes. L'auditeur n'a utilisé aucune ressource GCP.

## Réponses courtes

- **Q18 : le lemme antipodal est juste, le théorème proposé est faux.**
  L'admissibilité définie à un diamètre n'est pas monotone vers les diamètres
  plus petits. Le test aux sommets échange aussi les quantificateurs : il
  certifie qu'un même point rend toute une cellule admissible, pas que la
  cellule contient une direction admissible.
- **Q19 : ne pas raccorder cette version à l'ancien localisateur.** La forme
  corrigée doit d'abord vivre dans un module séparé, confronté à un oracle
  exhaustif et aux contre-fixtures ci-dessous. L'autorité non restreinte reste
  inchangée.
- **Q20 : publier le brut tronqué, mais pas un chiffre J0.** Les sorties peuvent
  être conservées comme diagnostic avec `source_complete=false`, cutoff et
  masse non résolue. Elles ne donnent ni taille de l'objet, ni exposant, ni
  décision sur `K=5`, ni SLO.

## 1. Le lemme du partenaire antipodal est correct

Soit un support positif propre `S`, une boule de centre `c`, rayon `R`, un
sommet `x` et `u=(c-x)/R`. Les poids circumcentriques strictement positifs
donnent `c` dans le relatif intérieur de l'enveloppe de `S`. Le produit
scalaire de leur relation barycentrique avec `x-c` impose l'existence d'un
autre sommet `v` tel que `(v-c).u>0`, donc `(v-x).u>R`.

Cette conclusion vaut pour une miniboule portée par un support positif. Elle
ne vaut pas pour une boule arbitraire passant par `x`. Toute version restreinte
du théorème doit donc conclure sur les miniboules de supports positifs, pas sur
« toute boule passant par `x` ».

## 2. Première réfutation : la monotonie est inversée

Pour `s=v-x` et une direction unitaire `u`, le partenaire est admissible au
diamètre `D` de la définition proposée exactement lorsque
`||s||<=D<2(s.u)`. Cet intervalle n'est pas monotone en `D` : augmenter `D`
relâche la première inégalité et durcit la seconde.

Contre-fixture exacte :

```text
x=(0,0,0), v=(10,0,0), r=5, P={x,v}.
```

À l'échelle `r`, aucun point distinct n'a une distance au plus cinq. Toutes les
cellules sont donc déclarées non admissibles et l'hypothèse de couverture est
vraie par vacuité, quel que soit `K`. Pourtant la boule diamétrale de `xv` est
une q2 positive, vide, de diamètre dix. La conclusion `D<r` est fausse.

La phrase « admissible à `D`, donc à `r` » est précisément l'étape invalide de
la preuve. Restreindre `v` aux voisins de distance au plus `r`, comme le fait le
probe courant, construit directement cette fausse vacuité.

## 3. Deuxième réfutation : la cellule échange les quantificateurs

Considérer le triangle sphérique de sommets `e1,e2,e3`, puis
`s=(10,1,1)`, `D=sqrt(102)` et la direction intérieure
`u=s/sqrt(102)`. Cette direction est admissible : `||s||=D` et
`s.u=D>D/2`. Aux sommets `e2` et `e3`, le produit scalaire vaut seulement un,
donc l'inégalité d'équateur échoue.

Ainsi la cellule **intersecte** l'ensemble admissible, mais aucun même témoin
ne satisfait les trois sommets. Le test proposé démontre
`exists v, for all u in C`; la preuve a besoin de marquer tout `C` vérifiant
`exists u in C, exists v`. Une petite calotte strictement contenue dans
l'intérieur d'une cellule donne la même réfutation.

La règle aux sommets reste utile dans l'autre sens : lorsqu'un même point
couvre strictement les trois sommets, la convexité géodésique de sa calotte
certifie la **couverture de toute la cellule**. Elle ne décide pas
l'intersection d'une calotte avec la cellule.

## 4. Théorème réparé, version sûre

Fixer une ancre `x` et un diamètre minimal `r>0`. Pour chaque `v!=x`, poser
`s=v-x`. Une enveloppe nécessaire, monotone dans la bonne variable, des
directions de supports positifs de diamètre au moins `r` est :

```text
A_r(x) = union_v {u sur S2 : 2(s.u) > max(r,||s||)}.
```

En effet, pour une miniboule positive réelle de diamètre `D>=r`, le partenaire
du lemme vérifie `D<2(s.u)`. Comme deux points de sa sphère sont distants d'au
plus `D`, on a aussi `||s||<=D`. Sa direction appartient donc à `A_r(x)`.

Pour chaque témoin `z`, sa calotte intérieure à l'échelle `r` est décrite par
`||z-x||^2<r*u.(z-x)`. Si une direction est couverte par `h` telles calottes à
`r`, les mêmes `h` points restent strictement intérieurs pour tout diamètre
`D>=r`. On obtient donc le théorème sûr suivant :

> Si chaque direction de `A_r(x)` est couverte par au moins `h_q` calottes
> intérieures à l'échelle `r`, aucune miniboule positive d'arité `q` passant
> par `x`, de diamètre au moins `r`, ne peut avoir moins de `h_q` intérieurs.

Les trois consumers restent séparés : `h_2=smax-1`, `h_3=smax-2` et
`h_4=smax-3`, soit dix, neuf et huit sous `smax=11`. Ces nombres sont les
premiers comptes rejetés, jamais des profondeurs acceptées. Une banque peut
calculer plusieurs statistiques pures, mais aucun verdict de lane ne ferme une
autre lane.

### Raffinement exact par l'égalité de shell

Le partenaire `v` appartient au shell réel. Pour une boule passant par `x`, on
a donc `D=||s||^2/(s.u)`. Une enveloppe nécessaire plus petite que la précédente
est donnée, pour `s.u>0`, par les deux conditions :

```text
2(s.u)^2 > ||s||^2
||s||^2 >= r(s.u)
```

La première est le lemme antipodal après substitution de l'égalité de shell ;
la seconde exprime `D>=r`. Cette version est potentiellement plus sélective,
mais son classifieur de cellule doit être reçu séparément. L'enveloppe large
ci-dessus est le correctif minimal le plus simple à falsifier.

## 5. Classification sûre des cellules

Une cellule doit être marquée **potentielle** dès qu'elle intersecte une des
calottes définissant `A_r(x)`. Pour la version large et un point `v`, cela
revient à demander si le maximum de `s.u` sur la cellule dépasse strictement
`max(r,||s||)/2`.

Deux implémentations sont recevables :

1. calculer exactement le maximum conique, en testant la direction de `s`, les
   projections admissibles sur les arcs et les sommets ;
2. employer un majorant conservateur, marquer `POTENTIAL` en cas d'incertitude
   et scinder la cellule.

Le mutant à tuer est `all_vertices_inside_implies_only_intersection`. Une
cellule non potentielle peut être omise ; une cellule potentielle doit, dans la
version simple, être couverte `h_q` fois **en entier** par les calottes
intérieures. Exiger la couverture de toute la cellule est conservateur mais
sûr. Un raffinement ultérieur peut travailler sur l'intersection exacte.

Attention : les partenaires potentiels ne sont pas limités à `B(x,r)`. Un
point plus lointain peut porter une direction d'un support de diamètre
supérieur à `r`. Le reporter doit donc utiliser une range-query conique globale
ou un index hiérarchique fail-open ; un scan des seuls voisins locaux recrée la
contre-fixture q2. Cette route n'emploie aucune Delaunay.

## 6. Audit du probe de falsification

Le pin `09d4cb8` ajoute q2 et q4 au mode `--falsifie`. Les vingt CTests
`^mhgp3v_caps_admissible_` passent localement en `44,74 s`. Ce vert n'est pas
une preuve du théorème : aucune porte ne contient les deux contre-fixtures
ci-dessus et toutes attendent zéro violation sur des nuages aléatoires.

Le falsificateur q4 compare en outre `r` à la plus grande **arête** du support,
alors que le théorème porte sur le diamètre `2R` de la circumsphère. Une q4
positive peut vérifier `diam_support<r<=2R` et échapper à la porte. Il manque
aussi q3, le diamètre de boule exact, les identités des supports et un oracle
indépendant du classifieur `cert_adm`.

Les fixtures permanentes minimales sont :

- la q2 `x=0,v=(10,0,0),r=5`, qui doit réfuter l'ancien théorème ;
- la cellule `e1,e2,e3` et `s=(10,1,1)`, qui doit rester potentielle ;
- la q4 régulière translatée de `(1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1)`
  avec `r=3` : ses arêtes ont longueur `sqrt(8)` et sa circumsphère diamètre
  `sqrt(12)`, donc elle tue exactement le remplacement de `2R` par la plus
  grande arête ;
- les trois seuils `10/9/8`, chacun dans sa lane autonome.

## 7. Q19 et Q20

Après correction du théorème seulement, un module séparé est préférable. Il
doit comparer ses cellules potentielles et ses rayons certifiés à un oracle
continu/exhaustif borné, puis être confronté différentiellement au certificat
global existant. Modifier directement le fichier historique de 2 341 lignes
rendrait la causalité des gains et des réfutations illisible.

Le brut J0 sous coupure peut et doit être conservé, y compris lorsqu'il est
rouge. Son type est `truncated_candidate_ledger`; il porte au minimum le cutoff,
`source_complete=false`, `unresolved_pair_mass`, codes par piste, seeds,
hashes, octets et HWM. Il ne sert pas à calculer des exposants de sortie ni à
choisir `K=5` avant que la source soit complète.

La rampe concurrente du 14 août a justement rendu
`INCOMPLET_OU_TRONQUE` : dix runs, quatre codes non nuls, pire rapport de
coupure `0,940`; seules les deux pistes `uniform` ont atteint 50 000, les amas
se sont arrêtés à 12 500. Ses `50/50` tests verts ne réparent aucun P0 de
source. Les deux générations ont été arrêtées de façon ciblée et certifiées
`TERMINATED`. L'auditeur n'a lancé, interrogé ou arrêté aucune VM.

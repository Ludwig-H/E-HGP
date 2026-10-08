# R : recopier les enfants d'une classe à cellule contributrice unique

**Proposition mathématique, aucun produit modifié, aucun natif ni nouveau chrono.**
Source livrée `7398aed7d8dd5595c880aa8a8677ecf05a8f2a32` : les corps T/M/R
lus sont identiques à repo6, base `a5e0dbc77`, patch `b3c78ae3…`.
`capture.json` épingle dix sources et les comptes historiques de `temps6_f1`.

Le R actuel appelle `component_at(witness,r−1)` pour chaque représentant
d'une cellule retenue, puis trie/déduplique la ligne (`registry_branches.cpp:68`).
Chaque requête remonte les attaches et cherche un événement par dichotomie
(`vertical_images.cpp:16`). Le coût exact entre recherches, tris et mémoire
n'est pas isolé dans les temps existants. Les minima locaux de trois prises
à un fil ne sont ni une décomposition d'une même passe ni un temps G4/FULL.

La piste ci-dessous diffère de la déduplication des racines avant dichotomie
déjà proposée dans `audit_tmv_traces_20261007` : elle supprime entièrement les
requêtes et le tri de certaines lignes. L'admission CSR déjà corrigée reste
hors de cette proposition.

## Critère constant et preuve

Pour une cellule **retenue** `t`, soit `[e0,e1)` son bloc dans `event_cell`,
`d=e1−e0>0`, `z=cell_node[t]` et `q=children.off[z+1]−children.off[z]`.
Alors

**`q == d+1` si et seulement si `t` est la seule cellule qui produit des
événements dans la classe contractée `z`. Dans ce cas, `ant(t)=children[z]`.**

1. Le noyau traite les représentants d'une cellule sans interposition
   (`forest_kernel.cpp:56`). Après chaque union, `x` est le survivant dont le
   sommet est ce nouvel événement. L'union suivante, si elle existe, prend
   ce sommet comme opérande. Les `d` événements forment donc une chaîne liée
   au même rang, sont contigus dans `event_cell` et appartiennent tous à `z`.
   Le dernier sommet de la cellule est traduit par M en ce même `z`.
2. Une classe connexe de `m` événements binaires est un arbre, pas un graphe
   avec partage d'opérandes : une union consomme deux composantes distinctes,
   et un ancien sommet consommé n'est plus sommet vivant. Ses `2m` opérandes
   comprennent `m−1` liens internes. Il reste `q=m+1` enfants externes
   **distincts**, exactement ceux que `contract_parents/children` publient.
   Comme les `d` événements de `t` sont dans la classe, `q=d+1` équivaut à
   `m=d` ; aucun comptage séparé des propriétaires de classes n'est nécessaire.
3. Si `t` est seule contributrice, ses unions relient exactement ses
   composantes d'avant le plateau. Une autre cellule ne peut leur apporter
   une composante ouverte nouvelle sans effectuer une union, donc produire
   un événement dans cette même classe. Les cellules inertes et les
   répétitions de représentants n'ajoutent aucune branche. Les enfants
   externes sont ainsi exactement l'ensemble dédupliqué des composantes
   ouvertes de `t`, y compris pour une multifusion à plus de deux enfants.

La preuve porte sur une **classe**, pas sur un plateau entier : plusieurs
classes disjointes peuvent avoir le même rang et chacune sa cellule unique.
Inversement, `{0,1}` puis `{0,2}` au même rang donnent une classe à trois
enfants mais deux cellules retenues à deux branches chacune : le test
échoue (`3 != 1+1`) et le chemin général doit être conservé. Avec `{0,1}` puis
`{0,1,2}`, la seconde ligne égale accidentellement tous les enfants, mais
reste générale : aucune déduction supplémentaire n'est proposée.

Les cibles naissance/cellule de G restent soumises à LEM-T3, de rang
strictement antérieur. Une cible cellule non régulière fournit une naissance
témoin dans sa composante ; changer ce témoin à l'intérieur de cette
composante ne change pas la coupe ultérieure. Attention : une cellule
**inerte** peut laisser une naissance qui sera réunie plus tard dans son
plateau ; son `cell_node` n'est pas nécessairement le nœud final du plateau.
Elle n'est pas une ligne retenue et n'entre pas dans le critère. Ce cas est
explicitement testé dans le modèle. Le cas `B=1`, sans événement, n'émet
aucune ligne. La preuve suppose une sortie valide de T/M ; elle ne renforce
pas le validateur public et ne clôt pas CST-0240.

## Raccord proposé, sans nouveau tampon

Le passage actuel sur les blocs de `event_cell` connaît déjà `d`. Pour chaque
ligne, lire `z` et `q`, puis :

```text
si q == d+1 : longueur de travail = 0 ; branch_count = q
sinon       : longueur de travail = nombre de représentants
              collect_row actuel (component_at, tri, unique)
après préfixes/admission de sortie :
si longueur de travail == 0 : copier children[z]
sinon                       : copier le tampon de collect_row
```

Une ligne générale retenue a au moins deux représentants : la longueur nulle
est un discriminateur sans ambiguïté, sans drapeau ni tableau par classe.
Tester cette voie **avant** de former un pointeur dans `branch_nodes`, qui
peut être entièrement absent ; n'allouer ce tampon que si sa taille est
positive. Les enfants sont déjà triés par identifiant canonique
(`forest_contract.cpp:118`) ; la copie rend le même ordre que le tri/unique
actuel. Garder toutes les lignes retenues, leur ordre et leurs répétitions
éventuelles entre lignes ; seule une ligne est dédupliquée, jamais le registre.

Pour `Q_g` représentants des seules lignes générales, le temporaire passe
de `4Q_R` à `4Q_g` octets ; conserver les offsets/comptes et la sortie
`4A+8(R+1)`. Mettre l'admission en cohérence, sans créditer la sortie avant
la fin du comptage. Les sorties et parents restent immuables pendant R,
les lignes disjointes gardent le parallélisme existant. Aucun coût de tableau
supplémentaire ; un test constant par ligne, puis copies obligatoires de `A`
identifiants. Le nombre de recherches devient `Q_g` et les tris portent
uniquement sur les lignes générales. La borne intrinsèque de publication
reste `Ω(A)` ; `A` n'est pas borné par `B−1` (famille historique à 7 naissances,
6 événements et 27 branches).

`branches` et l'empreinte de l'objet restent identiques. **`branch_reads`
compte les représentants réellement relus : il doit baisser**, conformément
à sa définition dans `ForestWork`, et rester déterministe entre W1/WN pour
la même politique. Ne pas le maintenir artificiellement. Compteurs utiles
pour le futur microbanc : lignes directes/générales, représentants évités,
octets temporaires, branches copiées. Le temps R complet et T/M/V/R complet,
avec allocation/admission, décident ; aucun temps gagné n'est déduit ici.

## Conséquence conditionnelle des comptes déjà publiés

Pour un ordre, `R` cellules retenues se répartissent sur `F` classes, chacune
ayant au moins une contributrice. Si `D=R−F`, il y a au plus `D` classes
non uniques et `2D` lignes générales. Le total des enfants M est `H=B+F−1`.
Si l'arité maximale M vaut `a`, les classes uniques ont au moins
`max(0,H−aD)` enfants, donc au moins autant de représentants dont la requête
est évitable. **`arite_max` du JSON est cette arité M, pas celle des cellules** :
elle ne borne pas les représentants avec doublons.

| Comptes ng00 existants | K5 | K10 |
|---|---:|---:|
| Requêtes R actuelles `Q_R` | 1 954 646 | 10 319 117 |
| Lignes générales, borne supérieure | 552 | 678 |
| Requêtes évitables, borne inférieure | 1 540 453 | 7 424 598 |
| Fraction minimale du nombre de requêtes | 78,81 % | 71,95 % |

Sommes par ordre, calculées depuis les compteurs de `temps6_f1`, dont les
sources T/M/R sont égales à la livraison. Ce ne sont ni des mesures du
raccourci ni des pourcentages de temps ; le débit de copie de la sortie,
l'allocation et les recherches restantes restent à mesurer. Aucune borne
universelle sur cette fraction : la famille des préfixes garde toutes ses
lignes générales.

## Modèle reproductible et limites

`model.py` confronte trois constructions : graphe des composantes gelé avant
chaque plateau ; événements binaires/attaches et `component_at` ; copie des
enfants sous le seul critère `q=d+1`. **147 cas exhaustifs bornés** : deux
sous-ensembles non vides de trois naissances, rangs `(1,1)/(1,2)/(2,2)`, puis
une cellule de fermeture ; **9 fixtures** avec multifusions, deux classes au
même rang, sous-arêtes, inclusions, doublons, cellule inerte cible, dates de
naissance et singleton. L'optimisation globale incorrecte « prendre tous les
enfants pour chaque cellule » est réfutée par les deux sous-arêtes partagées.
Les CSR sont comparées ligne par ligne et après concaténation. Dix hashes
sources relus avant/après ; normal et `-O` identiques. Modèle d'entrées T,
pas nouveau témoin géométrique ni qualification native.

```sh
python morsehgp3D_v12/receipts/audit_reponses_20261008/registre_classe_unique/model.py --repo /workspaces/E-HGP
python -O morsehgp3D_v12/receipts/audit_reponses_20261008/registre_classe_unique/model.py --repo /workspaces/E-HGP
```

L'alternative du balayage des coupes par DSU reste mathématiquement possible
(`rank(fusion)<r`, jamais `≤r`), mais ajoute environ `12B` octets et séquentialise
les requêtes par ordre. Elle n'est ni développée ni mesurée dans ce reçu :
le critère local ci-dessus préserve le découpage parallèle actuel.

# Fixture exacte : les témoins changent selon la cellule du centre

23 septembre 2026. Contre-épreuve mathématique de la [piste des cellules
de centres](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md), **pas**
une performance LiDAR ni une garantie qu'un front WSPD réel émettra ce
rectangle tel quel. Elle montre qu'un produit non singleton peut être
entièrement rejeté **avant** expansion alors qu'aucun témoin ponctuel
universel du citron ne ferme ses voies.

Dans les coordonnées ci-dessous, translater ensuite tous les sites par
`(20,20,20)` pour obtenir des points u18 :

```text
A = {(-10,0,0), (-9,0,0)}
B = {(9,0,0), (10,0,0)}
G+ = {(x, 8,0) : x = -1,0,1,2}
G- = {(x,-8,0) : x = -1,0,1,2}
p = (0,12,0), q = (0,0,12), K = 5.
```

Pour tout `a∈A,b∈B`, le centre d'une sphère passant par `a,b` a
`c_x=(a_x+b_x)/2∈[-1/2,1/2]`. Une couverture volontairement plus
large du cube des centres exclut strictement `x≤−1` et `x≥1` : à `x=-1`,
la distance carrée minimale à B vaut 100 et la maximale à A vaut 81 ;
la différence croît vers la gauche. À `x=1`, la minimale à A vaut 100
et la maximale à B vaut 81 ; la différence croît vers la droite. Les
termes y,z s'annulent. Aucun centre d'une paire A×B ne vit donc dans
ces deux bandes.

Découper le slab restant en `x∈[-1,0]` et `[0,1]`, puis selon le signe
de `c_y` ; `c_z` est libre. Ce sont **quatre cellules fermées**, dont les
contacts communs sont sans danger. Pour `c_y≥0`, prendre les quatre
sites distincts de G+ ; pour `c_y≤0`, ceux de G−. Si `c_x∈[-1,0]`, pour
chaque garde g de ce signe et chaque `b∈B` :

```text
|g-c|² - |b-c|² ≤ 9 + 64 - 81 - 16|c_y| ≤ -8 < 0.
```

Si `c_x∈[0,1]`, comparer plutôt à **chaque** `a∈A` :

```text
|g-c|² - |a-c|² ≤ 4 + 64 - 81 - 16|c_y| ≤ -13 < 0.
```

Les deux extrémités sont sur la sphère ; chacun des quatre gardes est
donc strictement intérieur à toute boule candidate de sa cellule,
indépendamment de l'autre extrémité. À K5, les seuils d'impossibilité
sont `K−1=4` pour q3 et `K−2=3` pour q4 : **toutes** les cellules possibles
sont mortes pour les deux voies, sans parcourir les quatre paires.

Ce n'est pas la preuve actuelle par témoin universel. Pour un garde g,
avec `L=b_x−a_x∈[18,20]`, le prédicat de citron donne
`H=L²/4−(g_x−m_x)²−64` et `Xi=64L²`. Si `H>0`, alors `H≤36`, donc
`3H²≤3888<20736≤Xi` ; si `H≤0`, il échoue directement. Aucun des huit
gardes ne certifie donc universellement q3 ni q4. Les sites p,q ont
`H<0`, et les autres extrémités A/B fournissent au plus deux témoins
par paire, sous les deux seuils. Pour l'arête `(-9,0,0)`–`(9,0,0)`,
le tétraèdre avec p,q est réellement positif : son centre vaut
`(0,21/8,21/8)`, ses poids barycentriques sont `(9,9,7,7)/32` et son
arête la plus longue est bien celle de A×B. Le rejet profond n'est donc
pas rendu vide par l'absence de supports q4 potentiels.

Les huit gardes appartiennent à chacun des quatre noyaux diamétraux :
`|g-m|²≤(5/2)²+64=70,25<81≤L²/4`. p,q sont dehors. Si les quatre
paires atteignent individuellement le chargement de ce noyau, les
formes **non-supports** seraient au nombre de `10+9+9+8=36` (huit
gardes, puis zéro, un ou deux autres sites des facteurs selon la paire).
C'est le gain physique que doit capter une future porte dédiée, pas
seulement un nombre de paires rejetées. La fixture n'est **pas** encore
un CTest du pipeline complet ; le front peut découper A×B autrement.

## Porte de mesure avant activation sur LiDAR

Un shadow exact doit compter, par rectangle et par voie, la masse des
paires prouvées **et** combien auraient réellement atteint le cœur,
leurs `dead_core_form_sites` et covers évités ; distinguer les paires
qui seraient déjà mortes au filtre ponctuel. Publier le coût propre des
cellules, coins, antichaînes et témoins, le nombre de rectangles
indécis/replis et le temps total q3/q4 + catalogue + FULL, avec
multiensemble normalisé de candidats et digest inchangés. La preuve
« toutes cellules possibles fermées » est nécessaire avant de supprimer
un rectangle entier ; une cellule indécise renvoie une seule fois au
chemin exact. Les essais de ligne/`h_a` existants éliminent des millions
de paires mais **aucune arête atteignant le cœur** dans leur échantillon
08/000000/K10 : ils ne valident pas ce gain structurel.

Le premier shadow doit donc être **conditionnel au vrai coût aval** : sur
08/000000/K10, 30 777 213 paires sont développées mais seulement
4 507 278 atteignent le cœur et y préparent 900 565 631 formes. Pour
les 72 329 rectangles de masse au moins 16 dans le repère local, quatre
cellules, vingt gardes et huit coins demanderaient déjà environ **46,3 M**
de tests garde×coin, avant sélection des gardes et reprises ; doubler
les côtés doublerait ce chiffre. Fixer le budget de la **tentative de
certificat** avant le shadow puis revenir exactement au chemin actuel à
son épuisement : ce n'est ni un quota de candidats ni un changement de
résultat. Une cellule indécise ou un seul support survivant force le
repli du produit entier, sauf véritable partition A/B disjointe.

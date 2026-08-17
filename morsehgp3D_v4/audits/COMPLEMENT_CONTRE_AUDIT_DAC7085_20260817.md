# Complément ciblé à l'harmonisation q3

Date : 17 août 2026.  
Pin de code audité : `5072e235ba1194132f84a16420600f767fd7f811`.  
Pin documentaire contre-audité : `dac7085ed0e43d2b98f6e534e26424243e7a9e59`.

Cette note complète, sans la répéter, l'harmonisation
`HARMONISATION_CONTRE_AUDIT_7895D95_BC1D3AE_20260817.md`.

Je confirme son verdict principal : `R_coup`, le filtre
`h_cœur+h_a+h_b`, la descente q2/q3/q4 fusionnée et la forme de Gram q3 sont
mathématiquement bien orientés. Le code au pin `5072e23` est reçu comme
**énumérateur de supports q3 réguliers peu profonds**, sous oracle partageant
encore la même arithmétique de puissance. Il ne produit pas encore un
événement HGP public complet.

Le présent complément ajoute cinq points qui ne doivent pas se perdre dans la
suite :

1. le contrat exact de régularité doit être limité aux boules pertinentes pour
   `K_max`, sauf à écrire un vérificateur global séparé ;
2. les petites tailles restent refusées à tort par plusieurs probes ;
3. lire un identifiant dans le bucket ne suffit pas tant que l'API d'entrée ne
   reçoit pas de vrais `PointId` externes ;
4. le census partagé q3 peut être implémenté exactement avec des boîtes de
   centres en point fixe dirigé, sans `BigInt` dans le chemin de production ;
5. une porte de bord exacte existe dans le cube lui-même, par pondération des
   ancres intérieures, sans commencer par coder un tore.

Aucun statut CI n'est publié pour le pin de code. Cette réception reste donc
une lecture mathématique et statique des sources et des reçus versionnés.

---

## 1. Régularité : préférer un contrat `K_max`-pertinent

Le contrat documentaire actuel parle de position générale globale. Or
`q3_ball_depth` s'arrête dès que le compteur atteint `h_3`. Une coquille située
sur une boule déjà trop profonde peut donc ne jamais être visitée.

Ce comportement est parfaitement suffisant pour la forêt tronquée à
`K_max`, mais il ne certifie pas la position générale de tout le nuage.

Pour une boule de support d'arité `q`, de profondeur stricte `d`, l'événement
appartient à l'ordre

```text
K = q + d - 1.
```

Il est pertinent pour la sortie demandée exactement lorsque

```text
d < h_q = K_max - q + 2.
```

Je conseille de définir explicitement :

> `regular_up_to_Kmax` : pour toute boule-support d'arité q et de profondeur
> `d<h_q`, aucun point extérieur au support n'appartient à sa coquille.

Sous ce contrat :

- si le compte atteint `h_q`, la boule est hors sortie et le parcours peut
  s'arrêter sans chercher sa coquille ;
- si le compte reste `<h_q`, le parcours doit s'achever, collecter tous les
  zéros de puissance et provoquer `unsupported_degeneracy` avant publication
  dès qu'un extra-shell existe.

C'est exactement ce que le chemin q3 peut certifier après remplacement du
`continue` actuel par un vrai statut transactionnel. Le même contrat doit
être formulé pour q2 et q4.

Si le projet veut conserver la phrase plus forte « le nuage entier est en
position générale », il faut un vérificateur global distinct. Le pipeline peu
profond ne peut pas honnêtement fournir ce certificat gratuitement.

Le reçu q3 compte déjà quinze extra-shells à `n=400` sur `uniform`. Il faut
donc publier aussi :

```text
degenerate_ballkeys,
degenerate_supports,
first_shell_witness,
regularity_scope = up_to_Kmax | global.
```

Le compteur canonique doit être par `BallKey`, car une même dégénérescence
peut être rencontrée depuis plusieurs supports. À terme, la fréquence de ces
cas sur grille u16 dira s'il est acceptable de rester en mode rejet ou s'il
faut traiter les plateaux cosphériques exactement.

---

## 2. `PointId` : le raccord bucket n'est qu'une moitié de correction

L'harmonisation propose provisoirement

```text
id(u) = ix.bucket_ids[ix.bucket_start[u]].
```

Cette conversion est correcte seulement si le bucket contient déjà une
identité externe stable. L'API actuelle

```text
build_cloud_index(const vector<P3>& points)
```

fabrique au contraire les identités à partir de la position du record dans le
vecteur. Une permutation des records change donc ces IDs, même si l'accès par
bucket est ensuite correct.

L'ordre sûr est :

1. introduire un record d'entrée

   ```text
   InputPoint { PointId id; P3 position; }
   ```

2. vérifier l'unicité des `PointId` et la validité u16 des coordonnées ;
3. trier spatialement les records sans jamais réécrire `id` ;
4. sous le profil de sites distincts, définir `site_id(u)` par l'unique ID du
   bucket ;
5. réserver l'overload `vector<P3>` aux benchmarks, en le marquant
   `generated_ids_not_stable` ;
6. permuter physiquement les records dans une porte métamorphique tout en
   conservant leurs IDs, puis exiger les mêmes `EdgeKey`, `SupportKey`,
   owners et événements.

Il faut faire ce raccord avant q4 et avant la première `BallKey` publique.
Sinon une sortie peut être géométriquement invariante tout en changeant de
nom à chaque réordonnancement mémoire, performance assez peu recherchée pour
une identité dite stable.

---

## 3. Petits nuages : le contrat corrigé n'est pas encore implémenté

Les documents ont adopté les seuils effectifs

```text
K_eff = min(K_max,n),
s_max  = min(K_eff+1,n).
```

Mais les probes gardent des refus globaux hérités du cas trois-lanes :

- q2 refuse encore les cas utiles les plus petits ;
- q3 exige `n>=4` et `smax_eff>=5`, alors qu'un support q3 de profondeur zéro
  existe déjà à `n=3`, `smax_eff=3` ;
- q234 exige de fait que les trois lanes soient actives.

La bonne primitive est un masque dynamique :

```text
lane q active <=> q <= smax_eff,
h_q = smax_eff - q + 1.
```

Fixtures minimales à graver :

- `n=2` : q2, profondeur zéro, `h_2=1` ;
- `n=3` : q3 aigu, profondeur zéro, `h_3=1` ;
- `n=4` : q4 bien centré, profondeur zéro, `h_4=1` ;
- les mêmes géométries avec `K_max` inférieur, où la lane correspondante doit
  être absente et non évaluée avec un seuil nul.

Ce point n'affecte pas les campagnes à `n=2000/8000`, mais il affecte les
oracles bornés qui portent justement la charge de correction.

---

## 4. Census q3 partagé : prototype exact en point fixe dirigé

L'harmonisation propose à raison un arbre temporaire des centres-porteurs. On
peut rendre ce prototype exact sans base irrationnelle du plan médiateur et
sans `cpp_int` dans le chemin mesuré.

Fixons l'ancre `(a,b)` et posons

```text
d   = b-a,
D2  = |d|²,
u_z = 2z-a-b.
```

Pour un porteur `x`, la forme q3 fournit `G>0` et `W=2G(c-a)`. Définissons

```text
N = W-Gd,
T = 2c-a-b = N/G.
```

Pour tout site `z`, quatre fois sa puissance géométrique vaut

```text
ell_z(T) = |u_z|²-D2-2 u_z·T.
```

Le signe est donc : intérieur `ell<0`, coquille `ell=0`, extérieur `ell>0`.

### 4.1 Boîtes de centres rationnels, décisions entières

Choisir une échelle fixe `S=2^B`, par exemple `B=32`. Pour chaque centre,
calculer par division entière dirigée :

```text
Tlo_i = floor(S N_i/G),
Thi_i = ceil (S N_i/G).
```

Un nœud du LBVH stocke les minima des `Tlo_i` et les maxima des `Thi_i` de ses
feuilles. Ces intervalles contiennent rigoureusement tous les centres réels du
nœud.

Pour un site ponctuel, travailler avec

```text
E_z(Ts) = S(|u_z|²-D2)-2 u_z·Ts.
```

Le maximum et le minimum sur la boîte se calculent axe par axe :

- pour le maximum, choisir `Tlo_i` si `u_i>0`, `Thi_i` si `u_i<0` ;
- pour le minimum, choisir l'extrémité opposée.

Les décisions sont alors :

```text
max E_z < 0  => z est intérieur à toutes les boules du nœud : range-add,
min E_z > 0  => z est extérieur à toutes les boules du nœud : élagage,
sinon        => scission ; l'égalité reste ouverte.
```

À la feuille, `q3_power` reste l'autorité exacte et collecte les shells.

Sous u16, `B=32` laisse une marge confortable : `N*S` tient en `i128`, les
coordonnées fixes de `T` tiennent en `i64`, et `E_z` tient largement en
`i128`. Ces bornes doivent devenir des `static_assert` et des fixtures aux
coordonnées extrêmes, non une simple phrase documentaire.

Cette représentation a trois avantages :

1. aucune projection dans une base orthonormale irrationnelle ;
2. aucune décision flottante ;
3. une boîte 3D légèrement plus lâche, mais immédiatement GPU-friendly. Une
   projection 2D plus serrée pourra venir après la mesure.

### 4.2 Parcours conseillé

Pour chaque ancre ayant passé `h_a/h_b` :

1. construire les porteurs possédés et leurs intervalles `Tlo/Thi` ;
2. construire un petit LBVH sur les coordonnées fixes ;
3. requêter une seule fois le cover

   ```text
   B(m,sqrt(3)D/2),  D=|b-a|,
   ```

4. faire traverser chaque site du cover dans le LBVH des centres ;
5. appliquer les `range-add` saturants à `h_3` ;
6. retirer du masque les sous-arbres de centres entièrement saturés ;
7. pour les seuls centres survivants, collecter exactement les
   `InteriorIds` et les extra-shells.

Les points `a` et `b` peuvent être exclus du flux. Pour le point porteur `x`,
la paire corrélée `(T_x,x)` satisfait exactement `ell_x(T_x)=0`. Les bornes
dirigées empêchent donc un crédit `ALL` qui compterait silencieusement le
porteur dans sa propre boule ; le cas ambigu descend jusqu'au test exact.

Mesures à ajouter :

```text
centers_per_anchor,
cover_sites_per_anchor,
center_nodes_visited,
range_add_mass,
centers_saturated_by_blocks,
leaf_power_tests,
shells_detected.
```

La voie radiale de l'harmonisation est mathématiquement correcte par direction,
mais elle ne donne pas de borne globale : un nuage générique peut avoir une
direction primitive distincte par porteur. Elle doit donc rester un accélérateur
optionnel pour les groupes réellement peuplés, après ce prototype de référence.

---

## 5. Porte de bord exacte sans tore : pondération des ancres intérieures

La porte torique proposée est saine. Une porte encore plus proche du code
cubique actuel évite toutefois toute géométrie périodique.

Soit le cube `Omega=[0,L]^3`. Pour une paire de longueur `r=|a-b|` et de
milieu `m`, imposer

```text
B(m,r/2) subset Omega.
```

Comme `W_4 subset W_3 subset W_2=B(m,r/2)`, aucun fuseau témoin n'est alors
tronqué par le bord. Pour une direction et une longueur fixées, le volume
admissible des milieux est exactement `(L-r)^3`.

Définir, avec un cutoff `R<L`, le statisticien pondéré

```text
Y_R = sum over alive pairs with r<=R and B(m,r/2) subset Omega
      of (L/(L-r))^3.
```

Pour `n` points continus i.i.d. uniformes dans le cube, son espérance par
point est exactement

```text
E[Y_R]/n = 2pi/(3v_q) sum_{j=0}^{h-1} I_u(j+1,n-1-j),
u = v_q (R/L)^3,
```

où `I_u` est la bêta incomplète régularisée. C'est la même cible finie que
la porte torique : la pondération annule exactement le volume `(L-r)^3` des
milieux admissibles.

Pour `R=0.4L`, le poids est borné par

```text
(1-0.4)^-3 = 125/27 < 4.63,
```

et les cibles à `n=2000` sont celles déjà calculées dans l'audit torique :

```text
q2 : 40.000,
q3 : 123.737,
q4 : 138.680.
```

Protocole recommandé :

1. ajouter une famille de test continue, ou une quantification assez fine pour
   rendre l'effet réseau mesurable séparément ;
2. garder seulement les ancres dont la boule diamétrale entière est dans le
   cube et `r<=0.4L` ;
3. accumuler le poids ci-dessus par lane ;
4. comparer la moyenne de plusieurs graines à la cible binomiale exacte avec
   intervalle de confiance ;
5. conserver en parallèle le reçu cubique non pondéré pour surveiller le
   régime LiDAR réel.

Cette porte teste la perte et le double compte dans les prédicats euclidiens
existants, sans introduire les images minimales d'un tore. Sur la famille
entière sans remise actuellement utilisée, la cible continue n'est pas
strictement exacte ; il faut soit une famille continue dédiée, soit un petit
oracle discret. Le développement de surface reste utile pour interpréter les
reçus existants, mais ses constantes numériques q3/q4 doivent être accompagnées
du script de quadrature et d'une borne d'erreur avant de devenir une porte.

---

## 6. WSPD : ne pas corriger quatre copies séparément

La prescription harmonisée est correcte : cellule exacte de préfixe,
scission par diamètre de cellule et terminal

```text
cell_separated || tight_box_separated.
```

Il reste un risque d'implémentation banal mais réel : la boucle de récursion
est aujourd'hui recopiée dans le front nu et dans plusieurs probes q2/q234/q3.
Corriger seulement `src/wspd/wavefront.hpp` laisserait les reçus exécuter une
autre décomposition que la production.

Avant ce raccord, factoriser au minimum :

```text
cell_view(node),
terminal_pair(A,B,s),
choose_split_by_cell(A,B),
children_of(pair).
```

Mieux, exposer un moteur de vague avec callback de mort/instruction. Les
probes peuvent alors instrumenter la même récursion au lieu d'en maintenir une
copie locale, sauf pour les mutants explicitement isolés.

Portes spécifiques :

- le ledger de paires reste exact ;
- la récursion réelle produit au plus autant de rectangles que la récursion
  ombre cellulaire sur le même nuage ;
- `cell_only` et `cell_or_tight` rendent le même multiensemble de paires
  ponctuelles ;
- le mutant « scission par boîte serrée » est distingué sur une fixture où
  l'ordre de scission diffère ;
- la cellule binaire exacte a un rapport d'aspect maximal `2`, y compris aux
  préfixes résiduels `rem=1,2`.

---

## 7. Ordre de travail amendé

### P0 : contrat et vérité

1. API `{PointId,position}` et porte de permutation à IDs conservés.
2. Statut `regular_up_to_Kmax` ou vérificateur global séparé.
3. Refus transactionnel des extra-shells pertinents.
4. Masque dynamique des lanes et fixtures `n=2,3,4`.
5. Oracle q3 rationnel indépendant, par solveur générique et non par appel à
   `Q3Form`.

### P1 : fermer q3 avant q4

6. Brancher `h_a/h_b` et énumérer les survivantes par seaux.
7. Publier `BallKey`, niveau carré rationnel, profondeur, `InteriorIds`,
   facettes et multifusion.
8. Rendre `raw/unique/duplicate_supports` visible.

### P2 : réduire le mur mesuré

9. Prototype exact de LBVH des centres en point fixe dirigé.
10. Porte cubique pondérée ou porte torique à cible finie exacte.
11. Raccord WSPD centralisé `cell || tight`.
12. Voie radiale seulement pour les directions assez peuplées.

### P3 : poursuivre la géométrie

13. Ouvrir q4 axial une fois les contrats q3 partagés et l'oracle indépendant
    verts.
14. Traiter les plateaux cosphériques si le taux de rejet u16 rend le mode
    régulier insuffisant en pratique.

---

## Conclusion

Les trois audits convergent sur le diagnostic important : Claude n'a pas
construit une impasse, mais le premier énumérateur q3 exact dans le régime
régulier. Le verrou restant n'est pas une nouvelle identité géométrique
mystérieuse. C'est le partage du travail entre les circum-boules d'une même
ancre, accompagné de contrats publics enfin alignés sur les identités, les
petits nuages et les dégénérescences réellement observées.

Le prototype en point fixe ci-dessus fournit un chemin court, exact et
mesurable vers ce partage. Il doit être essayé avant un arrangement explicite
ou une sophistication q4 : l'humanité possède déjà assez d'arrangements
compliqués sans en ajouter un lorsque quelques `range-add` dirigés peuvent
suffire.

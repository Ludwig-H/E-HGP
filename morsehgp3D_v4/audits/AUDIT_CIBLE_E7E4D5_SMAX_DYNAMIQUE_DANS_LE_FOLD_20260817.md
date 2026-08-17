# Audit ciblé après `e7e4d5e` — aligner tout le fold sur le `K_max` demandé

Date : 17 août 2026.  
Pin de code audité : `e7e4d5e58dbc8d0c1b57137b1eba2a9706029328`.

## Verdict

Le raccord général est bon :

```text
WSPD q2/q3/q4
  -> générateurs de BallKey
  -> RLE inter-lanes
  -> un census I_B/U_B par boule
  -> SpherePlateau
  -> forêts par K.
```

Je confirme également les deux blocages déjà signalés par les audits parallèles, sans les répéter ici :

1. la frontière plateau-forêt doit reconvertir les indices géométriques en vrais `PointId` ;
2. la sortie doit conserver naissances, croissances et facettes nées, pas seulement les fusions.

Il reste un troisième raccord, plus local mais nécessaire avant toute mesure du profil secondaire : `smax_eff` calibre correctement les filtres comme `K_max+1`, tandis que le census, l'expansion et le fold restent codés en dur pour `K_max=10`.

---

## 1. Le contrat mathématique est déjà dynamique en amont

Pour une boule `B` de support minimal d'arité `q`, un événement utile à un ordre `K <= K_max` vérifie

```text
sigma = I_B union T,
|I_B| + |T| = K+1,
|T| >= q.
```

Donc

```text
|I_B| <= K_max+1-q.
```

Avec la convention du dépôt

```text
smax = K_max+1,
h_q = smax-q+1,
```

on obtient exactement

```text
|I_B| <= smax-q = h_q-1.
```

C'est précisément la borne utilisée pour prouver que l'ancre du support minimal survit aux filtres WSPD. `collect_candidate_balls` reçoit bien `smax_eff` et calcule ses trois seuils avec `lane_h`.

---

## 2. Le chemin aval revient silencieusement à `K_max=10`

Dans `forest_probe.cpp`, après le flux correctement paramétré par `smax_eff`, le code appelle encore :

```cpp
ball_census(ix, bc.key, 9, shell_cap, ...);
expand_plateau(..., 11, ...);
for (int K = 1; K <= 10; ++K) { ... }
```

Le juge brut reprend les mêmes constantes.

Il y a deux conséquences.

### 2.1 Les sorties `K > smax_eff-1` n'ont pas de garantie de complétude

Pour `--smax=6`, les filtres sont ceux de `K_max=5`. Une boule seulement utile à `K=6` peut donc être éliminée légitimement en amont. Pourtant le fold continue à publier des résultats jusqu'à `K=10`.

Ces ordres supérieurs sont alors des sous-flux accidentels : parfois présents, jamais garantis complets. Ils ne doivent ni être exposés comme exacts, ni entrer dans les totaux affichés.

### 2.2 Le profil secondaire `K_max=5` paie encore le travail de `K_max=10`

Le contrat du projet distingue explicitement une cible secondaire `K_max=5`. Or `--smax=6` continue actuellement à :

- censuser une boule jusqu'à neuf intérieurs au lieu du seuil utile ;
- énumérer les sous-ensembles de coquille jusqu'à onze sommets ;
- construire dix forêts ;
- additionner leurs événements, fusions et nœuds dans les mesures.

Le temps ainsi obtenu ne mesure pas le profil `K_max=5`. Il mesure un flux filtré comme `K=5`, puis développé partiellement comme `K=10`, combinaison qui n'a pas d'objet mathématique public particulièrement pressant.

Le juge ne peut pas découvrir cette incohérence : il utilise le même domaine aval codé en dur.

---

## 3. Correction minimale, avec un gain de census gratuit

Définir une seule fois :

```cpp
const u64 kmax_eff = smax_eff - 1;
```

Puis propager cette valeur dans toute la chaîne :

```cpp
forests_from_balls(..., kmax_eff, ...);
expand_plateau(..., kmax_eff + 1, ...);
for (u64 K = 1; K <= kmax_eff; ++K) { ... }
```

Les tableaux statiques de taille 11 peuvent rester pour le profil maximal, mais seule la tranche `1..kmax_eff` est valide et publiée.

### Cap de profondeur exact par boule

Après le tri/RLE, le représentant conservé porte déjà l'arité minimale du générateur. Pour une boule de support minimal d'arité `q = bc.arity`, le bon arrêt est exactement :

```cpp
const size_t interior_cap = smax_eff - bc.arity;
```

et non `9` pour toutes les boules.

Ainsi, même au profil maximal :

```text
q2 : cap 9,
q3 : cap 8,
q4 : cap 7.
```

Au profil `K_max=5` :

```text
q2 : cap 4,
q3 : cap 3,
q4 : cap 2.
```

Cette correction est à la fois sémantique et utile pour le poste dominant : le reçu annonce que 98 % des boules meurent en profondeur après avoir commencé leur census. Il est inutile de chercher un neuvième intérieur d'une boule q4 lorsque le troisième suffit déjà à la déclarer hors du profil `K_max=5`.

Précondition à graver : après le RLE, `bc.arity` doit être l'arité minimale parmi les générateurs de la `BallKey`. Le tri actuel par `(BallKey, arity, representation)` fournit précisément cette propriété.

---

## 4. Fixture de frontière `K_max=5/6`

Prendre les sept points entiers :

```text
a  = ( 0,10,10),
b  = (20,10,10),
z0 = (10,10,10),
z1 = (10,11,10),
z2 = (10, 9,10),
z3 = ( 9,10,10),
z4 = (11,10,10).
```

La boule diamétrale de `ab` a centre `(10,10,10)` et rayon carré `100`. Les cinq points `z0,...,z4` sont strictement intérieurs. Le simplexe

```text
sigma = {a,b,z0,z1,z2,z3,z4}
```

est donc un événement q2 de profondeur cinq, exactement à l'ordre

```text
K = 2 + 5 - 1 = 6.
```

Exiger :

```text
smax=6  (K_max=5) : la boule est écartée dès le cinquième intérieur,
                     aucune sortie K=6 n'est publiée ;

smax=7  (K_max=6) : l'événement K=6 est présent au niveau R^2=100.
```

Cette porte doit être placée aussi près que possible de `BallData -> expand_plateau -> fold`, afin de tester la frontière aval sans dépendre du fait qu'un certificat WSPD particulier tue ou non l'ancre dans cette petite géométrie.

Mutant utile :

```text
fold-hardcodes-kmax10
```

qui rétablit les constantes `9/11/10` et doit mourir sur la comparaison `smax=6` contre `smax=7`.

Ajouter enfin une régression `smax=11` pour garantir que le profil maximal reste bit à bit inchangé.

---

## 5. Ordre de travail

1. Corriger d'abord le raccord `GeometryIndex -> PointId`, déjà bloquant.
2. Dans la même refactorisation de frontière, propager `kmax_eff` et le cap `smax_eff-bc.arity`.
3. Étendre ensuite `ForestResult` aux naissances/croissances et facettes nées, conformément aux audits convergents.
4. Ouvrir le rendu § 9.1 seulement sur cette ABI complète et correctement paramétrée.

## Conclusion

Le flux réel est géométriquement bien orienté. Le problème nouveau n'est pas un changement de méthode : il faut simplement que le paramètre qui définit l'objet en amont continue d'exister en aval.

Sans ce raccord, `--smax<11` filtre une hiérarchie et en replie une autre. Avec lui, le profil `K_max=5` devient enfin un véritable produit mesurable, et le census bénéficie immédiatement de seuils de mort plus courts par arité.

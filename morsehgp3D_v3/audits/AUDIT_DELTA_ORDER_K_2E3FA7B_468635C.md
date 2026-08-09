# Audit différentiel `order_k` — commits `2e3fa7b` et `468635c`

> [!CAUTION]
> **Verdict : crédit à l'amorce directionnelle et GO ciblé à la correction de coquille de `468635c`, mais NO-GO au chemin exact.** Le certificat affine par union de deux boules reste une bonne brique. Le delta `a6d0a3e` réduit réellement le nombre de candidats sur les probes locaux. Il ne marche toutefois pas sur l'axe géométrique du pinceau annoncé, et deux P0 suffisent encore à invalider l'exactitude : la grille peut sous-balayer une coquille u16 après une conversion indéfinie, et le germe force le niveau zéro en ignorant les points coplanaires strictement intérieurs. Le second P0 explique directement les sorties ultérieures à niveau négatif sur une fixture de cinq points.

## 1. Portée et empreintes

Phase annoncée : M3. Backend : CPU, décisions de pinceau entières, grille uniforme et broad phase `double`. Profile des probes : petites fixtures exactes et uniforme synthétique local. Mode : `order_k_vertices_fast` diagnostique; le catalogue appelle toujours le parcours lent. La porte d'entrée « exact » n'est pas satisfaite.

| objet | identité |
|---|---|
| commit de l'amorce directionnelle | `2e3fa7b1ca5d6c2fc286babd923ddeebbb3cf7b6` |
| header commité à `2e3fa7b` | SHA-256 `a6d0a3efe82fe9c17f5ce234d0e9bad40ffe0692ac2c1aab65bfa99ae088f6cc` |
| prédécesseur figé comparé | SHA-256 `4ef89a194d2adee0e86ddd78cd15caab9af8ec76de8f6d14cca329926f9321a5` |
| commit de conservation des membres constants | `468635cf55d804dc6740b83fe527a09253e431d7` |
| header commité à `468635c` | SHA-256 `47ee37638ec5f27e840c85d1fa3aca22646f8074a7f99d05cb89c30a03bcb7ca` |
| probe différentiel local | SHA-256 `a0b66710b718bfe90b77159711fc2fff023924211ff70b02ccc697f6a7c57284` |
| probe oracle indépendant | SHA-256 `5aed270215598aeb44bc20fce6b0b61648e733afd06afd93400237ad77bae023` |
| probe hostile du germe à cinq points | SHA-256 `a226f358f81cdf39cbf540cdb1b987192a5b85c801264fe483c77db6bddb37c9` |
| probe de transition constante | SHA-256 `5e23bdaa4ff8f371de82cd1755daa68c241ada5ba079ae2c1061a676ce0352de` |
| probe u16 de grille réutilisé | SHA-256 `a01aa41976d7a57239582c9bb57ad70671919945d95bde4068e773c6778a07e8` |

Le code produit a été lu sans modification. Les snapshots, sources et binaires d'audit sont restés sous `/tmp/orderk-a6d-audit.wwLjmb` et `/tmp/orderk-grid-audit.awD1l5`. Aucune mesure G4 du binaire `4ef89a1` n'est attribuée à `a6d0a3e` dans ce rapport.

## 2. Ce que `a6d0a3e` change et ce qui mérite crédit

Le seul delta de `4ef89a1` à `a6d0a3e` remplace l'amorce par dilatation concentrique par :

1. un balayage de la boule du sommet courant;
2. en l'absence de candidat, au plus quarante boules placées à des paramètres doublés dans la direction demandée;
3. dès qu'un candidat quelconque est trouvé, la certification inchangée par l'union des deux vraies boules d'extrémité;
4. si aucun candidat n'apparaît, un repli exhaustif sur les $n$ points.

La structure logique des points 3 et 4 est saine, conditionnellement à une requête de boule complète. Le candidat d'amorce est comparé par les prédicats entiers. S'il est dans la bonne direction, tout événement plus proche est dans l'union des boules du sommet courant et de ce candidat par affinité de la puissance. Un seul passage exact sur cette union suffit donc à retrouver le vrai voisin. Si l'amorce n'en trouve aucun, le repli global préserve aussi la décision.

Le gain local mesuré est réel. Sur le même nuage uniforme déterministe de 80 points, plafond 8, le nombre de sommets reste 10 978 et le nombre de replis reste 1 600, tandis que `pencil_candidates` passe de 1 591 096 à 970 076. Sur la fixture `bridge`, il passe de 274 à 182 à sortie identique. Un run local isolé du probe uniforme passe de 1,094 s à 0,739 s; ce temps n'est qu'un diagnostic, pas un reçu de performance.

Deux contrôles positifs bornés ont aussi été rejoués :

- 2 000 nuages génériques de 10 points, coordonnées dans $[0,30]^3$, plafond 8 : égalité de tous les couples `(shell, level)` entre fast et slow pour `4ef89a1` puis `a6d0a3e`;
- 200 nuages génériques de 8 points, plafond 8 : égalité entre `a6d0a3e` et l'énumération indépendante en `cpp_int`.

Ces contrôles créditent le delta ordinaire. Ils ne certifient ni le slow partagé, ni les coplanarités, ni la grille extrême, ni le pipeline produit.

## 3. P1 — le centre balayé n'est pas sur l'axe du pinceau

Le commentaire affirme que l'amorce « avance le long du pinceau ». Le calcul ne le fait pas. Soient $a,b,c$ le triangle, $u$ sa normale unitaire, et $q$ le vecteur de $a$ au centre du cercle circonscrit dans le plan. Le vrai pinceau a pour centres $a+q+t u$ et pour rayons carrés $\left\Vert q\right\Vert^2+t^2$. Le code calcule bien un analogue de la partie constante du rayon, mais place chaque centre en `a + t * u`; il omet $q$.

La faute est visible sans arrondi. Pour $a=(0,0,0)$, $b=(2,0,0)$ et $c=(0,4,0)$, on a $q=(1,2,0)$ et $u=(0,0,1)$. La vraie sphère de paramètre $t$ est centrée en $(1,2,t)$ et a $R^2=5+t^2$. La boule construite par l'amorce est centrée en $(0,0,t)$ avec le même rayon nominal : sa distance carrée à $c$ vaut $16+t^2>R^2$. Elle ne passe donc même pas par le triangle qui définit le pinceau.

Ce défaut ne crée pas seul une fausse sortie dans la structure actuelle : un candidat éventuellement lointain est ensuite certifié par les deux vraies boules, et l'absence de candidat après quarante tours déclenche le scan exhaustif. Il invalide en revanche la justification de localité, peut retarder le premier candidat jusqu'au repli, et interdit d'utiliser les quarante tours comme certificat. La formule flottante cohérente serait au minimum `apex_centre + (t - t0) * u`; une voie exacte doit en plus employer l'enveloppe fail-open du [rapport numérique](AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md).

Il n'existe dans le commit ni driver gardé, ni reçu scellé reliant les « 18--24 candidats/requête » du message de commit à ce SHA-256. Les runs G4 antérieurs de `4ef89a1` ne qualifient pas `a6d0a3e`.

## 4. P0 — le germe ignore les intérieurs constants de sa face

`seed_shell` trouve une face support, choisit le premier événement non coplanaire, puis traite les autres points du plan ainsi :

- `side == 0` : le point est ajouté à la coquille;
- `side < 0` : le point est silencieusement ignoré;
- les deux appelants initialisent pourtant le sommet par `Vertex{root_shell, 0}`.

Un point coplanaire à la face a un état constant le long du pinceau. S'il est strictement dans le cercle du triangle support, il est donc intérieur à la sphère du germe et contribue au niveau. L'ignorer rend faux le tout premier état transporté.

Fixture u16 minimale rejouée :

```text
0 (4,1,0)
1 (14,19,0)
2 (4,17,0)
3 (17,9,0)
4 (15,8,19)
```

La face choisie est `(0,3,1)`. Le point 2 vérifie exactement `orient == 0` et `side == -1`. Le germe stocké par le slow et le fast est la coquille `{0,1,3,4}` au niveau 0; un census exact donne le niveau 1. Les deux parcours rendent ensuite :

```text
slow out=1 V=1 storedL=0 exactL=1 shell=0,1,3,4
fast out=1 V=1 storedL=0 exactL=1 shell=0,1,3,4
```

Le `out=1` ne décrit pas un nuage hors contrat : le faux zéro finit par être décrémenté et passe négatif. Ce mécanisme fournit une explication locale et permanente des sorties G4 précédemment étiquetées `HORS DOMAINE`.

Compter exactement ces intérieurs et renvoyer `root_level` est nécessaire, mais pas suffisant pour la complétude du préfixe. Si ce niveau dépasse le plafond de navigation, le germe corrigé est coupé avant d'atteindre les vrais sommets de faible niveau. Un vrai germe garanti de niveau zéro demande par exemple un triangle à cercle vide dans une triangulation exacte de la face support, ou une construction certifiée du lower hull relevé. Le census global unique peut rester un garde et une fixture, pas remplacer cette preuve.

Le commit `468635c` ne modifie pas `seed_shell`; cette fixture y rend exactement le même échec.

## 5. P0 — le sous-balayage u16 de la grille est inchangé

Le delta `a6d0a3e` ne touche ni la conversion des bornes AABB en `int`, ni la comparaison de distance `double`. La fixture à déterminant 1 du rapport numérique reste donc rouge :

```text
a=(0,0,0)
b=(1,0,0)
c=(65535,1,0)
d=(65535,65535,1)
```

Sous `-fsanitize=undefined,float-cast-overflow`, le snapshot s'arrête à la ligne 661 :

```text
runtime error: -1.37428e+11 is outside the range of representable values of type 'int'
```

En `-O2` ordinaire, `Grid::ball` ne rend que les identifiants `0,1`, au lieu des quatre points exactement sur la sphère. Dès qu'un candidat partiel existe, le repli conditionné par `best < 0` ne répare pas cette omission. Le certificat par union de deux boules reste donc mathématiquement vrai mais logiciellement non applicable jusqu'à une grille à enveloppes certifiées et comportement fail-open.

## 6. Crédit séparé au commit `468635c`

Le second commit corrige un autre défaut réel : lors d'une transition, un membre de l'ancienne coquille qui est coplanaire au triangle et sur son cercle appartient à toutes les sphères du pinceau. `a6d0a3e` l'excluait du nouveau lot parce qu'il appartenait déjà à `v.shell`. `468635c` ne saute plus que le triangle et `best`, puis réinsère tout point exactement sur la nouvelle sphère.

Sur la fixture constante suivante, le résultat est net :

```text
(3,2,2) (2,3,2) (1,2,2) (2,1,2) (2,2,3) (2,2,5)
```

| snapshot | slow | fast |
|---|---:|---:|
| `a6d0a3e` | 14 sommets, 8 censuses faux | 14 sommets, 8 censuses faux |
| `47ee376` | 6 sommets, 0 census faux | 6 sommets, 0 census faux |

Les requêtes passent de 136 à 72 et les candidats de 176 à 80; les replis fast passent de 80 à 40. La correction est conceptuellement juste : un membre constant a un côté nul aux deux extrémités, ne change pas le niveau et doit rester dans la coquille. Pour le fast, ce crédit reste conditionnel à la complétude de `Grid::ball`.

Cette correction ne ferme pas le P0 du germe ci-dessus, la coupe par rang fermé, ni les omissions de catalogue déjà démontrées.

## 7. Portes de reprise

1. Ajouter la fixture à cinq points comme test permanent du germe et exiger `(shell, exact_level)` avant toute propagation.
2. Construire un vrai germe de niveau zéro sur une face non triangulaire; documenter pourquoi il reste dans le préfixe navigué.
3. Conserver la fixture constante de six points comme non-régression de `468635c`, pour le slow et le fast.
4. Corriger `Grid::ball` par saturation avant conversion, enveloppes dirigées et repli global sur toute requête inconclusive, même si un candidat existe déjà.
5. Recentrer l'amorce sur le vrai axe $a+q+t u$, puis distinguer dans les statistiques `bootstrap_rounds`, `bootstrap_points`, `union_points` et `exhaustive_fallbacks`.
6. Comparer les niveaux et les coquilles à l'oracle rationnel indépendant sur coplanarités, cosphéricités et limites u16; ne plus employer le slow partagé comme unique juge.
7. Ne produire un reçu G4 pour `a6d0a3e` ou `47ee376` qu'avec le hash du header et du binaire, des runs séquentiels, le plafond produit et le pipeline complet.

Les P0 de coupe par rang fermé, d'intermédiaire global `seen/frontier/visited` et d'intégration du fast au catalogue restent détaillés dans [l'audit de grille](AUDIT_GRILLE_ORDER_K_4EF89A1.md), [l'audit des dégénérescences](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md) et [la voie multiplicitaire](AUDIT_VOIE_MULTIPLICITES_ORDER_K.md).

GCP non utilisé par l'auditeur.

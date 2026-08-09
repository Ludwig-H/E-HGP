# Audit du delta `order_k_flats` — snapshot `2532fd5`

Date : 9 août 2026 UTC.

> [!IMPORTANT]
> **Verdict ciblé : GO pour la correction du contre-exemple de germe `9c587e6`; NO-GO inchangé pour une promotion exacte, le domaine produit et le contrat 50 k.** La construction par segment support et apex d'angle maximal remplace valablement la fausse descente de rayon. Les 120 permutations du témoin passent, le garde quadratique a disparu et aucun contre-exemple n'a été trouvé par l'oracle planaire hostile. Le différentiel reste toutefois relatif à des primitives partagées, le profil u16 n'est pas gardé à l'API et l'architecture globale en $\Theta(nV)$ n'a pas changé.

Cadre : `backend=reference_cpu_local`, `profile=quantized_u16_order_k_prototype`, `mode=exploration/diagnostic_only`, `public_status=not_claimed`. Aucun de ces libellés n'ouvre une phase officielle.

## 1. Snapshot et portée

| objet | SHA-256 ou identité |
| --- | --- |
| parent committé | `82e2851d804497294fb7e2092f91649d3d0d8bca` |
| `prototype/order_k_flats.hpp` live | `2532fd5513b143beb55a75a34b05d286df64642bc5d47dd33876a0025ca7831c` |
| `prototype/flats_differential.cpp` live | `a685092045bdf2288636f503e870a4a98fd528b63810ab41af63580e546c04ff` |
| `CMakeLists.txt` live | `a879744bb73d17c25fb751fea30f99134656da0bbdf337faff294cf2087d0ca9` |
| `README.md` live | `78fd1a882ba1a9c22ba2f6b982fcd102ad1cacf2860a730a692f599060b08dbd` |
| `PROPOSITION.md` live | `09eac579df5a124656c04f4ff66c17310f18c19fed2c3cd57f4d34fa06ea614c` |

Ce rapport audite un delta live postérieur à [`AUDIT_ORDER_K_FLATS_9C587E6.md`](AUDIT_ORDER_K_FLATS_9C587E6.md). Toute nouvelle empreinte demande un rejeu.

## 2. Le P0 de la descente de rayon est fermé

### 2.1 Pourquoi la nouvelle construction tient

Sous coordonnées distinctes, le point `ha` lexicographiquement minimal du sous-nuage coplanaire est exposé par un fonctionnel linéaire suffisamment perturbé. Toutes les directions depuis `ha` appartiennent donc à un demi-plan ouvert; `plane_side` les ordonne angulairement et le tie-break par distance choisit le premier point sur un rayon collinéaire. Le segment `(ha,hb)` est support et ne contient aucune autre observation dans son intérieur.

Fixons des coordonnées planaires avec $A=(0,0)$, $B=(L,0)$ et tous les apex du côté $y>0$. Le cercle par $A$, $B$ et $C$ s'écrit $x^2+y^2-Lx+D_Cy=0$, avec $D_C=\frac{Lx_C-x_C^2-y_C^2}{y_C}$. Un point $d$ est strictement intérieur à ce cercle si et seulement si $D_d>D_C$. Les apex sont donc totalement préordonnés par le prédicat `in_circle_coplanar`; la passe conserve exactement le maximum, et une égalité désigne le même cercle cocirculaire. Les points collinéaires au-delà de `hb` sont extérieurs; le choix du plus proche exclut ceux qui seraient dans le segment ouvert.

Les contrôles finaux des étapes 5 et 7 vérifient respectivement le support du segment et l'absence d'intrus dans le cercle. Il n'existe plus de boucle, de potentiel de rayon ni de produit `q*q`; le débordement à grande face disparaît avec eux.

Nuance de vocabulaire : `(ha,hb)` n'est pas toujours une arête combinatoire maximale de l'enveloppe. Pour `A=(0,0,0)`, `B=(1,0,0)`, `C=(2,0,0)`, `D=(0,1,0)`, le code choisit `AB` alors que la 1-face maximale est `AC`. `AB` est un **sous-segment support primitif**; l'algorithme reste valide, car `C` est extérieur au cercle `ABD`. Les commentaires doivent employer ce terme plus précis.

### 2.2 Rejeux

La fixture de l'audit précédent est désormais permanente :

```text
A=(0,0,0) B=(0,3,0) C=(2,1,0) P=(1,1,0) Q=(1,1,2)
```

Résultats indépendants :

- 120 permutations, chacune aux ordres `s_max=2..8`, soit 840 comparaisons exhaustives : zéro échec;
- statut `ok`, germe de niveau 0 et coquille géométrique unique sur les 120 permutations;
- catalogues de 9, 15, 19, 20, 20, 20 et 20 records pour `s_max=2..8`, avec respectivement 3, 4, 4, 4, 4, 4 et 4 sommets;
- oracle planaire exact sur grille $4\times4$, sous-ensembles de 3 à 6 points, deux orientations, alignements et cocircularités : 294 040 ordres, zéro échec;
- fuzz structuré sur des plans de coordonnées : 2 000 nuages Release et 500 nuages UBSan, zéro désaccord;
- campagne u16 hostile supplémentaire : 6 469 cas, 543 310 sommets et census, 321 campagnes d'équivariance, zéro désaccord.

Le P0 exact de `9c587e6` et le débordement `q*q+8` sont donc fermés sur ce snapshot. Cette conclusion ne transforme pas le germe entier ni les primitives partagées en théorèmes produits.

## 3. Corrections supplémentaires créditées

- Les 21 fixtures comprennent maintenant `descente_rayon_refutee` et `coordonnees_dupliquees`; le témoin du rayon a un rejeu exhaustif de ses 120 permutations.
- Deux coordonnées identiques produisent `kDuplicateCoordinates` et un catalogue transactionnellement vide, au lieu d'un `ok` dépendant des identifiants.
- La portée du différentiel est enfin bornée dans sa source : `sphere_side`, `sphere4` et `miniball_of` sont partagés avec le sujet.
- Le test compare maintenant membres, contiguïté, sentinelles, ordre et cohérence de la boule publiée; ce gain dépasse l'ancienne signature support--rang.
- Les campagnes publient leurs compteurs et CMake impose des planchers de navigation, sommets, coquilles multiples et triplets quotientés.
- L'équivariance couvre aussi un échantillon des nuages génériques et cosphériques, avec statut et census dans la signature.
- Le suffixe entier `0junk`, la coordonnée CLI hors grille et une campagne sans navigation ont des tests négatifs dédiés.
- Les deux énoncés `q=1` et variation de niveau par lots sont corrigés dans le header et le README.

Build propre et validation locale sur les empreintes ci-dessus : 10/10 CTests `mhgp3v_flats_*` verts. Les quatre portes positives annoncent respectivement 169, 1 142, 1 249 et 2 029 cas sans désaccord; les six portes négatives rendent les échecs attendus. UBSan est vert sur les fixtures, le témoin plateau et le fuzz structuré.

## 4. Portes encore ouvertes

### 4.1 P1 fail-closed — narrowing entier avant validation

Le parseur exige désormais que tout le token soit consommé, mais `take(int*)` convertit le `long long` en `int` sans vérifier sa plage. Deux commandes hostiles rendent encore 0 :

```text
--clouds 4294967296 --min-cases 1
=> clouds=0, 169 cas de fixtures, OK

--clouds 0 --coord 4295032832 --min-cases 1
=> coord=65536, 169 cas de fixtures, OK
```

Chaque option doit vérifier sa plage sémantique et `INT_MIN..INT_MAX` avant le cast; la graine doit avoir un contrat de largeur distinct.

Le générateur de points distincts peut aussi censurer silencieusement une demande impossible. `--clouds 1 --points 9 --coord 2 --smax 2 --min-cases 1` demande neuf points dans huit positions, ne produit aucun nuage aléatoire, puis rend 0 parce que les 169 cas fixes satisfont le plancher global. Il faut rejeter `points > coord^3` sans overflow, ou exiger un compteur `requested_clouds == generated_clouds` par famille. Le produit `100*npoints` du garde doit lui aussi être borné.

### 4.2 P0 de domaine — u16 n'est gardé que dans le CLI du juge

Les largeurs `i128` supposent la grille u16, mais les API `navigate_shallow` et `flat_catalogue` acceptent encore tout `P3` sans vérifier les coordonnées ni authentifier le profil. Reproduction UBSan directe de `flat_catalogue` avec des coordonnées de l'ordre de $10^9$ :

```text
prototype/order_k_flats.hpp:181: runtime error: signed integer overflow in __int128
exit_code=1
```

La garde CLI ne protège aucun autre appelant. Une frontière API `quantized_u16`, avec contrôle $0\le x,y,z\le65535$ avant tout prédicat, est requise. Le profil dyadique exact demeure hors de ce prototype.

Le refus des doublons est un progrès fail-closed, pas la sémantique produit : le contrat quantifié doit conserver ou quotients les multiplicités de collision. Cette porte reste ouverte et le README le reconnaît.

### 4.3 L'oracle et le payload ne sont pas indépendants

Le nouveau test de payload vérifie surtout l'auto-cohérence du record sujet. Il ne compare pas le centre rationnel, le rayon ou `beta` à une vérité distincte; une autre sphère passant par un support de petite arité et induisant les mêmes membres peut rester invisible. Les forêts ne sont toujours pas construites.

La vérité partage encore `sphere4`, `sphere_side`, `miniball_of`, le bon centrage et la convention canonique. Le libellé défendable reste « portée de navigation et signature de catalogue concordantes relativement aux primitives v2 ». Une référence rationnelle multiplicitaire et des injections de centre, rayon, `beta`, membres, ordre et statut sont requises avant D6.

Le mode `verify_census=true` incrémente toujours les compteurs en cas de contradiction sans positionner `kInvariantViolated`; seul ce binaire les lit. L'API de vérification demeure donc fail-open pour un autre appelant.

### 4.4 Les planchers sont utiles mais agrégés

Les minima ferment le mutant « toute dimension est basse », mais ils ne vérifient pas l'accord de dimension affine nuage par nuage contre une référence indépendante. La porte cosphérique exécute aussi une grande campagne générique; ses planchers peuvent être satisfaits sans prouver un nombre minimal de cas cosphériques forcés. `census`, lots multiples, refus/directs et nombre d'essais d'équivariance sont comptés mais n'ont pas tous un plancher CMake.

Chaque famille doit publier demandé, généré, accepté, refusé et décidé, avec des minima propres à ses branches plutôt qu'un seul total global.

### 4.5 La documentation live se contredit encore

- `PROPOSITION.md` remplace encore `\pm1` par `-1,0,+1`, alors que le header et le README corrigent justement cette seconde affirmation : l'amplitude par lots est non bornée par 1.
- Le commentaire CMake du différentiel dit encore « partage seulement `sphere_side` ».
- Le README parle de « payload entier » bien que centre, rayon et `beta` ne soient pas comparés à la vérité; il annonce trois tests négatifs au lieu de six et utilise deux fois l'identifiant 9 dans sa table finale.
- La réponse aux audits contient deux tabulations ayant amputé `\textsc{Delaunay}` en `extsc{Delaunay}`.
- La réponse marque « tous les autres points fermés », formulation contredite par les §§4.1--4.4 ci-dessus.

Ces corrections sont documentaires et évidentes; elles ne changent pas le statut scientifique.

## 5. Le NO-GO 50 k ne bouge pas

Le delta corrige un germe et renforce son falsificateur. Il ne change aucun des coûts décisifs :

- les `n` singletons déclenchent encore `n` census globaux, soit 2,5 milliards de classifications avant le germe à 50 k;
- `seen`, `frontier` et `visited` matérialisent toujours tous les sommets et leurs coquilles;
- chaque direction de flat et chaque tentative d'émission rescannent le nuage;
- les triplets d'une coquille sont énumérés avant quotient;
- propriétaire local, index fail-open, reverse search, streaming, forêts horizontales et verticales, incidences silencieuses, lots contractuels et `coverage_log` sont absents.

Le nouveau §5 bis du README identifie honnêtement `seen` comme verrou mémoire et GPU commun. La décision d'étudier d'abord la reverse search sous arrangement simple est acceptable comme sous-problème d'architecture si le domaine reste refusé explicitement et si aucune mesure réelle n'est revendiquée. Elle ne qualifie pas le contrat u16 dégénéré, où les multiplicités sont normales.

Ce chemin est CPU-only : aucune G4 ne doit être utilisée pour ces tests ou mesures. Une G4 ne sera justifiée que lorsqu'un véritable kernel CUDA du pipeline qualifié existera.

## 6. Porte de reprise

1. Corriger les casts du CLI, l'accord demandé/généré et la garde u16 à l'API; ajouter les trois reproductions du §4.1 et les frontières 65535/65536.
2. Rendre le census fail-closed et compléter les planchers par famille.
3. Rectifier les claims documentaires et borner « payload entier » à ce qui est effectivement comparé.
4. Construire la référence rationnelle multiplicitaire et ses injections avant toute nouvelle fermeture D6.
5. Conserver le BFS multiplicitaire comme oracle structurel borné; développer séparément la source streamée/reverse-search qui évite la mosaïque globale avant toute tentative 50 k.

Décision : créditer pleinement le remplacement du faux potentiel de rayon, la fixture permanente et les renforcements de test. Maintenir `exploration/diagnostic_only` et le NO-GO exact/50 k.

GCP non utilisé.

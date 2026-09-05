# Arithmétique des fuseaux et des comptes de témoins

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Verdict : les expressions entières de `spindle.hpp` et les comptes locaux de `witness_count.hpp` sont bornés sur leur domaine d'appel produit.** Le produit `2*cq*s2`, pourtant écrit en i64 avant division, tient sur 57 bits de magnitude avec l'ajout du plafond. Les racines corrigées, les conversions de leurs résultats et les différences à centres quadruplés sont sûres sous la précondition numérique explicitée au § 4. L'absence de double crédit ferme aussi le risque de wrap des comptes locaux avant leur écrêtage. Ces points ne restent donc pas des objections arithmétiques générales contre S1.

Deux frontières sont conservées précisément : la validité de la graine de `std::sqrt` appartient au domaine d'exécution qualifié ; les primitives directes supposent les plages disjointes et les indices valides que le front leur fournit. Un commentaire d'un juge sans appel trouvé dans le produit promet en outre un écrêtage qu'il n'effectue pas (§ 6). Cette dérive locale ne touche pas le compteur utilisé par le front.

## 1. Domaine, sources et niveau de preuve

La lecture porte sur [spindle.hpp](../src/spindle/spindle.hpp), [witness_count.hpp](../src/spindle/witness_count.hpp) et leurs appels à [intmath.hpp](../src/core/intmath.hpp). Les boîtes proviennent de [l'index qualifié](AUDIT_INDEX_20260905.md). Le [front](FRONT_ET_TEMOINS_COURANT.md), [S1](S1_COURANT.md) et la [preuve des secteurs/cordes](PREUVE_CHORD_SECTOR_COURANTE.md) fournissent le sens géométrique des crédits ; leurs autres primitives ne sont pas incluses dans le présent grand-livre.

Les préconditions sont : positions u16, boîtes fermées ordonnées avec extrémités dans 0..65535, index valide non vide, références valides, lanes dans `{q2,q3,q4}`, absence de mutant et retour normal des allocations. Pour les crédits de rectangle, les plages de A et B sont disjointes. Le pipeline impose au préalable $2\leq n\leq2^{30}-1$, refuse les positions dupliquées et borne `smax` à 2..11. Les propriétés de comptage restent valides avec les multiplicités de l'index sous la même borne n, mais cela n'étend pas le profil produit aux positions dupliquées.

**La séparation s n'apparaît dans aucune formule des deux fichiers.** Elle ne multiplie ni les rayons ni les comptes de témoins. Les bornes ci-dessous valent pour toutes les boîtes u16, séparées ou non, donc aussi pour les rectangles intermédiaires visités avant le test WSPD. Le pipeline fournit `s>=8` ; les valeurs supérieures ne changent pas ce domaine arithmétique local. La comparaison de séparation à grands paramètres relève de [wavefront.hpp](../src/wspd/wavefront.hpp), qui possède sa voie large distincte.

Le [reçu léger](receipts_front_20260905/spindle_bounds.json) épingle les sources avant/après au HEAD `35dda097f75a66f8264002c58b9ccc4888c46d2e`, l'état du worktree et les calculs entiers. Elles sont restées stables durant l'exécution. La [sonde Python](spindle_arithmetic_probe.py) vérifie les constantes, le registre de bornes et des modèles mathématiques ; **elle n'exécute pas le C++**. Aucune compilation ni sonde lourde n'a été lancée pendant la fenêtre de chronométrage réservée au constructeur. Ce rapport ne remplace pas les reçus compilés existants et n'en invente pas de nouveaux.

## 2. Prédicats H, Xi et extrema de boîtes

Posons $M=65535<2^{16}$. Une différence de coordonnées a un module au plus M ; son produit avec une autre différence a un module au plus $M^2$. Les primitives de P3 effectuent leurs soustractions et multiplications en i64.

| Expression écrite | Borne suffisante et justification |
|---|---|
| `h_point`: `(z-a)·(b-z)` | Trois produits i64 ; module au plus $3M^2<2^{34}$ |
| `dot(d,w)`, `norm2(w)` | Respectivement dans $[-3M^2,3M^2]$ et $[0,3M^2]$ ; leur soustraction tient même sous la borne intermédiaire $6M^2<2^{35}$ |
| Valeur finale `h` / `hh` | L'identité avec `h_point` donne de nouveau le module au plus $3M^2$ |
| Une composante de `cross(d,w)` | Différence de deux produits ; module au plus $2M^2<2^{33}$, en i64 |
| `xi` | Trois carrés promus **avant** multiplication : $0\leq\Xi\leq12M^4<2^{68}$, en i128 |
| `h2`, puis `3*h2` ou `2*h2` | $h^2\leq9M^4$ et $3h^2\leq27M^4<2^{69}$, en i128 |
| `z*(a+b)-a*b-z*z` par axe | `a+b<=2M` ; le premier produit est au plus $2M^2$, puis les deux soustractions restent de module au plus $2M^2$ |
| Somme dans `hmin_boxes` | Chaque terme vaut exactement `(z-a)*(b-z)` et reste dans $[-M^2,M^2/4]$ ; la somme est dans $[-3M^2,3M^2/4]$ |
| `s=a+b`, `y=clamp(s,2*lo,2*hi)` | Tous dans 0..2M ; les bornes du clamp sont ordonnées par le contrat des boîtes |
| `(b-a)^2-(y-s)^2` | Les carrés sont au plus $M^2$ et $4M^2$ ; la différence est dans $[-4M^2,M^2]$ |
| Somme dans `hmax4_boxes` | Dans $[-12M^2,3M^2]$, de module inférieur à $2^{36}$, en i64 |

Les sentinelles `INT64_MAX` des minima ne sont jamais additionnées : chaque boucle d'extrémités réalise au moins une évaluation, même pour une boîte plate. Les boucles de coins utilisent des indices 0..7 ; leurs masques ne décalent aucun entier au-delà de sa largeur. Une boîte plate retire seulement les représentations répétées d'un coin. Il reste toujours au moins un coin.

La sûreté géométrique reste celle du front : `hmin_boxes>0` certifie tous les points de la boîte ; `hmax4_boxes<=0` exclut un **témoin universel** pour le rectangle. Cette dernière borne n'exclut pas les témoins de chaque autre ancre individuellement. À extrémités ponctuelles, elle est le vrai maximum de H sur la boîte de recherche. Aucun changement de sens n'est nécessaire pour fermer les bornes entières.

## 3. Boule de cœur : échelles et chaque intermédiaire

Notons $D=2^{30}$ et $E=2^{20}$ les échelles de constantes. `aq` appartient à `{D,619000000,555000000}` ; `cq` à `{2E,ceil(4E/3),1329545}`. La sonde vérifie les déclarations présentes dans le fichier avant de calculer le registre, afin de refuser un changement silencieux de constante.

Pour les centres réels des boîtes, `ca2` et `cb2` valent deux fois leurs coordonnées ; `center4=ca2+cb2` représente **exactement** quatre fois leur milieu. `d2q` est quatre fois la distance carrée entre ces centres. `w2a` et `w2b` sont les carrés des diagonales, soit quatre fois les rayons carrés des boules circonscrites.

Définissons $R_*=\lceil\sqrt{3M^2}\rceil=113510$ et $D_*=\lfloor\sqrt{12M^2}\rfloor=227019$. Les majorants suivants prennent la plus grande constante de chaque famille ; ils ne supposent pas que les maxima de toutes les boîtes sont atteints simultanément.

| Intermédiaire | Domaine suffisant dans l'ordre réellement écrit |
|---|---|
| `ca2`, `cb2`, `center4` | 0..2M, 0..2M, 0..4M ; `center4<=262140` |
| `u=cb2-ca2`, `u*u`, `d2q` | Module de u au plus 2M ; carré au plus $4M^2$ ; somme au plus **51 538 034 700**, 36 bits |
| `wa`, `wb`, `w2a`, `w2b` | Largeurs 0..M ; sommes de carrés au plus **12 884 508 675**, 34 bits |
| `d2u`, `ra2u`, `rb2u` | 0..227019, 0..113510, 0..113510 après racines dirigées |
| `r2u`, `gap=d2u-r2u` | 0..227020 ; gap dans −227020..227019 |
| `(i128)aq*gap` dans la branche `gap>0` | Entre 0 et $D D_*=243759795142656$, 48 bits ; division par D positive, cast i64 du quotient au plus 227019 |
| Rayon décorrélé avant maximum | Quotient précédent moins `r2u` ; dans −227020..227019 |
| `ra2u*ra2u+rb2u*rb2u` | `s2<=25769040200`, 35 bits ; les deux multiplications i64 et leur somme sont sûres |
| `2*cq` | Au plus $4E=4194304$ |
| `2*cq*s2 + E - 1` | Au plus **108 083 188 388 069 375**, 57 bits ; le produit, l'addition et la soustraction restent tous en i64 |
| `sub2` | Division par E positive : au plus **103 076 160 800**, 37 bits |
| `ceil_sqrt(sub2)` | Au plus **321 055**, 19 bits |
| `(i128)aq*d2u/D`, puis cast i64 | Quotient dans 0..227019 ; aucune troncature de largeur |
| `coup` | Différence du quotient et de la racine plafond : dans −321055..227019 |
| `radius4=max(0,max(r4,coup))` | Dans 0..227019 ; son carré est au plus **51 537 626 361** |

Les vérifications de constantes sont elles-mêmes dimensionnées : $3A_3^2<D^2$ et les produits quadratiques en D tiennent sous $2^{62}$. Pour q4, $X=2D^2-A_4^2$ est positif et inférieur à $2^{61}$ ; $X^2$ et $3D^4$ tiennent sur **122 bits**, donc dans i128 signé. Les produits des constantes d'échelle E restent beaucoup plus petits. Les promotions `(i128)` présentes précèdent ces produits.

### Direction des arrondis et sens du rayon

Les entiers `ra2u`, `rb2u` majorent deux fois les rayons de boîtes ; `d2u` minore deux fois leur distance de centres. `aq/D` minore $2\kappa_q$. Dans la branche `gap>0`, le quotient entier positif est bien un plancher, puis `r2u` est soustrait par excès. Le résultat minore donc $4R_{\mathrm{dec}}$ de la preuve géométrique.

`cq/E` majore $4\kappa_q^2+1$. Puisque `s2` est positif, `(2*cq*s2+E-1)/E` est exactement le plafond du rationnel correspondant. Sa racine plafond majore quatre fois le terme soustrait dans $R_{\mathrm{coup}}$. Le premier terme est minoré par les deux arrondis précédents : `coup` minore donc $4R_{\mathrm{coup}}$.

Le maximum est sûr parce que les deux minorants ont le même centre exact. Le remplacement des valeurs non positives par zéro ne prétend pas minorer un rayon négatif : la boule **ouverte** de rayon zéro est vide et n'accorde aucun crédit. Toute valeur positive retenue provient d'un minorant valide. Le test de `gap>0` peut ainsi abandonner une borne non utile sans faux témoin.

Pour `box_vs_ball` et `point_in_ball`, chaque différence `4*x-center4` est dans −262140..262140. Les appels à `llabs` et les négations de `hi4` n'approchent jamais le minimum i64. Les carrés sont promus en i128 avant multiplication ; leur somme est au plus $48M^2=206152138800$, 38 bits. Les comparaisons `far2<r2`, `near2>=r2` et `d2<r2` traitent exactement les contacts de bord comme non stricts. Les boîtes ne sont pas créditées par égalité.

## 4. Racines corrigées : ce qui est prouvé, ce que l'environnement fournit

Les trois classes d'appel à `floor_sqrt`/`ceil_sqrt` dans la boule de cœur ont des arguments entiers dans 0..103 076 160 800, donc sous $2^{37}$. Sous une conversion binaire64 conforme, **tous ces arguments sont exactement représentables** : aucune perte entière ne précède l'appel à la racine.

La précondition minimale pour la preuve de la boucle est que `std::sqrt` fournisse une graine finie, non négative, dont la troncature est représentable en i64. Le domaine usuel de `sqrt` sur ces entrées positives satisfait cette précondition et fournit une graine proche de la racine, très éloignée du plafond i64. Cette obligation reste attachée à la bibliothèque et au binaire qualifiés ; le présent calcul Python ne certifie pas la libm C++.

Sous cette précondition, la correction est exacte même si la graine n'est pas arrondie au plus proche. Dans la première boucle, tout carré de r tient en i128, y compris pour une graine i64 positive arbitrairement grande. Chaque décrément diminue strictement r jusqu'à obtenir $r^2\leq x$. Avant la seconde boucle, r est donc au plus la racine plancher de x ; `r+1` est représentable, ici au plus 321055. Les incréments s'arrêtent exactement lorsque $(r+1)^2>x$. L'encadrement final est $r^2\leq x<(r+1)^2$, qui caractérise la racine plancher. Le plafond ajoute un uniquement lorsque $r^2<x$ ; cette addition est sûre et traite les carrés parfaits sans ajout artificiel.

Ainsi, une dépendance à la **justesse finale de l'arrondi flottant** n'est pas un verrou : les boucles la corrigent. Une graine NaN, infinie ou non convertible ne satisfait en revanche pas le contrat, et aucune correction entière postérieure ne pourrait réparer un cast déjà invalide. Il faut conserver cette distinction dans la qualification d'exécution. La routine pure `isqrt64_pure` n'est pas appelée par ces deux fichiers ; elle n'est pas revendiquée comme remplacement implicite.

Une contre-fixture mathématique permanente dans le reçu protège le sens de l'arrondi : A=(0,0,0), B=(1,1,0), z=(0,1,0), en q2. La distance quadruplée au centre de z a son carré égal à 8 et H=0 : z est sur le shell diamétral. Le rayon correct vaut `floor_sqrt(8)=2`, alors que le mutant de distance plafond donne 3 et créditerait faussement z puisque 8<9. C'est une réfutation par équations du mauvais sens d'arrondi, **pas un nouveau run du mutant C++**.

## 5. Comptes locaux, masques et capacités

La preuve d'index donne des plages contiguës, une partition disjointe parent/enfants et une visite unique de chaque feuille. La preuve du front donne aussi A et B disjoints pour tous les rectangles transmis au compteur : les graines sont des couples de frères, puis une scission remplace seulement l'un des facteurs par un enfant.

`overlap_weight` intersecte deux plages par max/min et rend zéro pour une intersection vide. Pour une plage Z, les intersections Z∩A et Z∩B sont disjointes. Leurs poids ont donc une somme au plus égale au poids de Z. Les deux soustractions **non signées**, réalisées successivement dans `credit_weight`, ne sous-débordent pas. Cette conclusion dépend de la disjonction, pas seulement de la positivité des trois poids.

Dans `count_universal_witnesses`, une lane créditée sur Z perd son bit avant toute descente dans Z. Ses populations créditées forment une antichaîne ; une feuille visitée plus tard pour une autre lane ne lui est pas recréditée. Le passage aux coins ne traite que les lanes encore ouvertes et les feuilles hors A∪B. Par conséquent, chaque `fc.c[li]` est au plus n **avant** l'écrêtage final. Avec $n<2^{30}$, toutes les additions u64 sont sûres, même si un seul crédit de sous-arbre dépasse h. La dernière boucle applique réellement `min(count,h)`.

Les trois seuils produits viennent de `lane_h` avec `smax<=11` : au plus 10, 9 et 8 pour q2, q3 et q4. Sa garde précède la soustraction d'arité ; l'ajout de un est sûr. Une lane à seuil zéro n'entre pas dans `counting`. Les masques ne comportent que des décalages 0..2. Les boules des lanes q3/q4 sont construites exactement quand le masque initial pourrait les utiliser ; aucune lecture d'une boule non initialisée n'est nécessaire au chemin normal.

Chaque nœud est dépilé au plus une fois par appel. `nodes_visited<=2m-1<2^31`. Une feuille occasionne au plus 64 évaluations de coins communes à q3/q4 : `corner_evals<=64m<2^36`. Ces bornes portent sur **FusedCounts local à un rectangle** ; le cumul de télémétrie de toute une génération est une obligation différente, non implicitement certifiée ici.

`collect_universal_ids` collecte des indices de positions uniques, pas une expansion des buckets d'identités. Ses plages créditées et feuilles isolées sont disjointes. Le nombre d'écritures est au plus `min(cap,m)` ; les boucles vérifient `count<cap` avant chaque chemin d'écriture et un incrément de u reste sous m. L'appelant doit fournir un tampon accessible d'au moins cette taille. La borne m interdit aussi un débordement du i32 de parcours. Le contrat sémantique d'absence de troncature dans un rectangle vivant exige en plus la capacité annoncée par l'appelant ; une capacité trop petite arrête la collecte, sans dupliquer d'indice ni écrire au-delà de cap.

`true_spindle_count` a les mêmes bornes de visite et de compte, avec deux extrémités distinctes. Ses crédits q2 portent sur des sous-arbres disjoints ; les retraits d'extrémités sont sûrs. L'appel direct de `credit_weight` avec A=B ne possède, lui, aucune garantie : par exemple un poids singleton donne `1-1-1`. Ce cas ne respecte pas le contrat des rectangles du front et n'est pas présenté comme une défaillance de S1.

## 6. Dérive locale du commentaire du juge

Le commentaire de `true_spindle_count` annonce un compte « écrêté à h ». La fonction arrête le parcours quand `count>=h` mais retourne directement `count`. Un crédit de sous-arbre peut dépasser h. Aucun appel de ce helper n'a été trouvé hors de sa définition dans les sources et tests v7 examinés ; le front utilise `count_universal_witnesses`, qui possède bien son `min` terminal.

Fixture minimale de contrat : cinq points sur l'axe x, aux positions `{0,4,5,6,10}`, avec ancre 0/10, lane q2 et h=1. Dans le trie Morton, `{4,5,6}` est un sous-arbre. Sa boîte est strictement dans la boule diamétrale, car `min(z*(10-z))=24>0` sur [4,6]. Son crédit vaut 3. La pile visite ce sous-arbre avant la feuille 0 ; l'arrêt au seuil rend donc **3**, pas 1. Il s'agit ici d'une trace statique reproductible, pas d'une exécution C++ revendiquée.

Le constructeur peut corriger le commentaire en « arrêt dès atteinte du seuil, dépassement possible », ce qui décrit le comportement présent, ou rendre `min(count,h)` s'il veut réellement exposer un contrat d'écrêtage. Ce choix ne bloque pas la fermeture arithmétique du compteur produit. Il ne faut plus lister ce helper comme preuve d'un écrêtage exact tant que ce raccord documentaire n'est pas explicite.

## 7. Contrôles légers et suite bornée

Commande exécutée :

```bash
python3 -O morsehgp3D_v7/audits/spindle_arithmetic_probe.py
```

Elle rend **0** en environ 0,028 s au reçu, avec 92 cas de correction de racine dans un **modèle entier Python**, 125 identités axiales, les bornes du tableau et la contre-fixture d'arrondi. Les planchers restent effectifs sous `python3 -O`. Les sources avant/après sont identiques. Ces contrôles complètent la preuve de domaine ; ils ne doivent pas être additionnés aux CTests compilés.

La prochaine qualification compilée utile est un raccord direct de ces mêmes valeurs frontières aux helpers C++, avec la graine, les casts et les comparaisons strictes sous le binaire visé. Elle devra attendre la fermeture de la fenêtre réservée aux mesures du constructeur. Il n'est pas nécessaire de relancer une recherche géométrique sur les rayons ni d'élargir les types par précaution : les bornes entières ci-dessus suffisent déjà pour ces deux fichiers.

Aucune source produit ni navigation d'audit modifiée. Aucun statut public promu. **GCP non utilisé.**

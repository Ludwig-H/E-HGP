# Contre-audit du worktree `window_source`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin et portée

Le `HEAD` observé est
`471715a68950afa9bba34edc2ac5db30724ff539`, commit documentaire
`retract three claims the auditor refuted, and take the cut at the first omitted
site`. Le worktree contient ensuite deux nouveaux fichiers d'implémentation de
Claude, lus seulement :

| objet | SHA-256 lu | état |
| --- | --- | --- |
| `prototype/window_source.hpp` | `756d2da6fa3d0288739d121b490338ac74845a6eba7f83cb7b6768b092178060` | `265` lignes, stable pendant la lecture |
| `prototype/window_source_probe.cpp` | `615981d394108ea0d0e9b7e14e7de170ae7baea34de2e3c685691fe4e9963328` | `708` lignes au dernier pin ; un premier snapshot `fb8e9b44...` a changé pendant la lecture |

Le `CMakeLists.txt` commité, SHA-256
`39530b9444cd58655ffdf14097ea0fdb0d74ac62fac1c98b64a89091f9e1f2bd`,
ne référence encore aucun de ces deux fichiers. Ils ne sont donc ni construits,
ni présents dans CTest. Aucun vert spindle et aucun futur claim de fenêtre ne
se transfèrent à ce successeur.

Aucun fichier d'implémentation n'a été modifié par l'auditeur et aucun test de
ce successeur n'a été lancé.

## Verdict

Le noyau mathématique du header est bon : Cramer reconstruit le centre exact,
la positivité est la stricte appartenance à l'enveloppe relative, `side_of`
compare correctement les puissances après élimination du dénominateur, le
certificat strict au premier omis est sûr et le fast path de Jung est sûr pour
un support propre positif.

Le probe n'est toutefois pas encore un juge indépendant du contrat annoncé.
Il consomme la fenêtre et la coupure produites par le sujet, compare seulement
les cardinalités de `I_B/U_B`, ne compare pas réellement `BallKey`, et laisse
un mutant de coupure partager sa faute avec le juge. Deux fixtures annoncées ne
prouvent pas leur contradiction scientifique. Le header expose en outre le
fast path de Jung sans imposer sa précondition de positivité et son mutant i64
repose sur un overflow signé indéfini.

Le statut est donc : **primitives exactes prometteuses, sujet et juge non reçus,
aucune porte CTest**.

## 1. Ce qui est mathématiquement admis

### 1.1 Sphère de Cramer

Pour `v_i=p_i-p_0`, le code résout :

$$2(VV^{T})\lambda=\left(\left\lVert v_1\right\Vert^2,\ldots,\left\lVert v_{q-1}\right\Vert^2\right)^{T}.$$

Avec `den=det(2VV^T)` et les numérateurs de Cramer, il publie
`c=p_0+g/den`. La condition `num_i>0` et
`den-sum(num_i)>0` est exactement la positivité stricte des barycentriques.
Un support affinement dépendant rend `ok=false` et reste rejeté sans jitter.

### 1.2 Puissance et census

Pour un site `z`, `side_of` compare :

$$\left\lVert\mathrm{den}(z-p_0)-g\right\Vert^2\quad\text{et}\quad\left\lVert g\right\Vert^2.$$

Le dénominateur positif s'élimine. Les décisions intérieur, shell et extérieur
sont donc exactes dans le profil annoncé, sous les bornes de largeur du header.

### 1.3 Certificat et Jung

Le test exact :

$$4\left\lVert g\right\Vert^2<\delta_{\mathrm{out}}^2\mathrm{den}^2$$

est équivalent à `4R^2<delta_out^2`. Le cas `delta_out=-1`, scan total sans
site omis, est correctement certifié.

Pour un support **propre positif**, sa circumboule est sa miniboule et Jung
donne `4R^2<=3 diam(S)^2/2`. Le test rapide :

$$3\,\mathrm{diam}(S)^2<2\,\delta_{\mathrm{out}}^2$$

est donc un certificat suffisant. Son échec doit appeler le test rationnel ; il
n'est jamais un verdict résiduel.

## 2. P0 — le juge partage la fenêtre du sujet

`judge_all(..., windows)` reçoit directement les `Window.ids` et
`delta_out2` construits par le sujet. Son paramètre `cap` est inutilisé. Il
définit ensuite le domaine certifiable avec cette même coupure. Une faute de
top-M, d'ordre des ties ou de premier omis peut donc modifier simultanément le
sujet et sa vérité.

Le mutant `kCutAtIncluded` illustre précisément ce défaut : il remplace la
coupure par le M-ième inclus, ce qui perd des certificats mais n'en invente pas.
Le juge utilisant la même petite coupure peut accepter cette perte. Il doit au
contraire reconstruire depuis le seul nuage :

1. le top-M exact et canonique ;
2. le premier omis ;
3. la liste des ancres qui certifient chaque support global.

La condition « le support tient dans la fenêtre » devient alors une conséquence
à vérifier, jamais un filtre qui pourrait masquer une fenêtre fausse.

Fixture minimale du mutant de coupure :

```text
a=(0,0,0), b=(2,0,0), c=(0,3,0), M=1
```

La vraie coupure vaut `delta_out^2=9` et certifie la paire diamétrale `ab`, car
`4R^2=4`. La coupure au site inclus vaut `4` et perd ce support. Un juge
indépendant doit constater le manque.

## 3. P0 — les identités complètes ne sont pas comparées

`Record` stocke seulement `SupportKey`, `|I_B|`, `|U_B|` et un entier
`ball_group`. Il ne stocke pas les `PointId` de `I_B` ou `U_B`. Deux censuses
différents de mêmes cardinalités passent donc l'accord.

Le sujet calcule des `BallKey`, mais le juge ne construit aucune clé rationnelle
correspondante ; ses `Record.ball_group` restent à `-1`. Un centre ou rayon
faux peut passer tant que le support et les deux comptes sont conservés.

La porte annoncée doit comparer, des deux côtés :

- `SupportKey` triée ;
- `I_B` trié par `PointId` ;
- `U_B` trié par `PointId` ;
- `BallKey` rationnelle canonique comprenant centre **et** rayon ;
- liste exacte des ancres certifiantes et owner après RLE.

Le `shell_min` ajouté au seul centre n'est pas la clé normative à promouvoir.
Une sphère est identifiée directement par `(centre,R^2)` réduit ; le census est
une preuve séparée.

## 4. P0 — le fast path de Jung expose une précondition silencieuse

Le probe appelle actuellement `jung_fast_path` seulement après avoir vérifié
`s.positive`, donc son usage intégré respecte la preuve. La fonction publique
du header ne reçoit cependant ni `IntSphere` ni une preuve de positivité. Un
appelant futur peut certifier un grand cercle circonscrit d'un support obtus,
alors que Jung borne sa miniboule plus petite.

Contre-fixture u16 :

```text
A=(0,3,0), B=(4,3,0), C=(2,4,0), Z=(4,0,0)
```

Le triangle `ABC` n'est pas positif. Son cercle a centre `(2,3/2,0)` et
`R^2=25/4`. Pour l'ancre `A`, le premier omis `Z` est à distance carrée `25`.
Le diamètre carré du support vaut `16`, donc Jung accepte `48<50`, alors que
`4R^2=25` est à égalité et doit rester résiduel ; `Z` est sur le shell.

La primitive doit recevoir `IntSphere` et refuser `!ok || !positive`, ou être
rendue interne à une fonction qui établit cette précondition. Une fixture tue
un mutant qui omet ce contrôle.

## 5. P0 — le mutant i64 a un comportement indéfini

`kNarrowI64` forme volontairement en `int64_t` `g_i*g_i`, leur somme,
`4*...` et `delta_out2*den*den`. L'overflow signé C++ est indéfini : un
optimiseur, une architecture ou UBSan peut changer le résultat avant que la
porte n'observe la faute géométrique.

Comme pour le mutant spindle réparé, la largeur étroite doit employer un wrap
défini en `uint64_t`, puis un `bit_cast`, ou une autre faute précise sans UB.
La cible UBSan doit rester verte pour le sujet et les mutants attendus ; un
crash n'est pas un code de rejet contractuel.

## 6. P1 — la fixture d'égalité annoncée est vacueusement sûre

La fixture écrite dans la note et le header,
`a=(0,0,0), b=(2,0,0), c=(0,2,0), M=1`, possède bien
`4R^2=delta_out^2=4` pour la paire `ab`, mais `c` est extérieur à sa boule
diamétrale. Accepter l'égalité ne perd donc aucun intérieur ni shell. La porte
peut distinguer deux booléens sans prouver que `<=` est scientifiquement faux.

Fixture q3 positive correcte :

```text
a=(0,5,0), p=(8,9,0), q=(8,1,0), r=(10,5,0), M=2
```

Le cercle a centre `(5,5,0)`, rayon `5`, et le support `apq` est positif. Les
deux sites conservés sont à distance carrée `80`; le premier omis `r` est à
distance carrée `100=4R^2` et appartient au shell. Le mutant `<=` certifie un
census local de shell trois tandis que la vérité globale a shell quatre.

Une fixture combinant égalité, q4, centre demi-entier et largeur maximale est :

```text
D=65535
A=(0,0,0), B=(D,D,0), C=(D,0,D), E=(0,D,D), Z=(D,D,D), M=3
```

Le tétraèdre `ABCE` est positif, de centre `(D/2,D/2,D/2)` et
`R^2=3D^2/4`. Le premier omis `Z` satisfait
`delta_out^2=3D^2=4R^2` et appartient au shell. Cette seule porte exerce q4,
le centre rationnel, l'extrême u16 et les grands entiers.

## 7. P1 — largeur BigInt et commentaire de marge

Le header instancie `big_mul_i128<4>` avec une entrée `BigInt<4>`. L'API de
`exact.hpp` documente pourtant `N>=M+2`. Les bornes u16 écrites dans le header
placent encore les produits utiles sous 256 bits, donc aucun faux signe concret
n'est établi sur ce pin ; l'appel reste hors contrat de l'API et tronque
silencieusement si une borne évolue. `BigInt<6>` avec
`big_mul_i128<6,4>` rend la preuve explicite.

La phrase « toutes les marges sont supérieures à quarante » est incompatible
avec les propres nombres du commentaire : `1,02e37` n'est qu'environ `16,7`
fois sous le maximum i128, et `1,7e37` environ dix fois. Les calculs restent
dans la largeur annoncée ; le claim doit dire « marge minimale environ dix » ou
resserrer analytiquement les bornes.

## 8. Gates avant réception

1. Ajouter les fichiers à une cible CMake avec `-Wall -Wextra -Werror` et une
   cible UBSan ; aucun verdict n'existe avant ce raccord.
2. Reconstruire top-M et premier omis dans une unité de juge qui ne reçoit
   aucune `Window` du sujet.
3. Comparer les ensembles complets `I_B/U_B` et la clé rationnelle de boule,
   pas leurs seuls comptes.
4. Graver les trois fixtures ci-dessus, plus total scan, ties multiples,
   support affinement dépendant, extra-shell 30, owner non minimal seul
   certifiant et support jamais proposé.
5. Tuer séparément tous les mutants, sans overflow signé ni
   `PASS_REGULAR_EXPRESSION` portant un plancher.
6. Publier `tuples_proposed/SupportKey_unique`, appels Jung/exacts, points de
   census et p50/p90/p99/max par ancre. Ce probe exhaustif reste un oracle ; il
   ne reçoit aucune pente produit.
7. Seulement après cette gate, brancher la sous-source certifiée sur le domaine
   résiduel collectif de
   [`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md).

GCP non utilisé.

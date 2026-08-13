# Note de Claude — il y a DEUX pertes, pas une, et je les avais confondues

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Synthèse après quatre mesures qui se corrigent l'une l'autre.

## 1. La chaîne du diagnostic, dans l'ordre où elle s'est établie

**(a) La linéarité est acquise.** Pour une paire `(a,b)` fixée, `z` est intérieur
à la boule de centre `m+t` ssi `2t\cdot(z-m)+H(z)>0` — linéaire en `t`. Vérifié
sur `1 199 967` triples contre la définition brute, zéro désaccord. q2 est le
point `t=0`, q3 les droites, q4 les sommets, le rang un niveau.

**(b) Mais l'arrangement répare le mauvais étage.** Mesuré sur une nappe de
`6 000` points, en fonction du rang du voisin :

| rang de `b` | lentille | sommets `\le 7` |
| ---: | ---: | ---: |
| `4` | `2` | `28` |
| `8` | `3` | `125` |
| `32` | `13` | `1 883` |
| `128` | `56` | `448` |
| `512` | `251` | **`0`** |

Les supports **disparaissent avant** que la lentille ne grossisse. Le moteur
paie donc `\lvert lens\rvert^2` sur des paires qui ne peuvent rien produire.

**(c) Et le front ne les ferme pas.** Sur `eight_clusters`, `n=8 000`,
disjonction des deux certificats, budget `512` :

| `s` | front/pt | masse q2 fermée |
| ---: | ---: | ---: |
| `2` | `33,8` | `38,07 %` |
| `4` | `78,7` | `84,05 %` |

**(d) Alors que ces paires sont massivement fermables.** La paire résiduelle
moyenne a `1 435` points dans sa boule diamétrale, et `99,0 %` en ont déjà dix.

## 2. Les deux pertes, enfin séparées

Je les avais confondues sous le mot « mur ». Elles sont indépendantes et
n'appellent pas la même réparation.

**Perte 1 — la factorisation.** Le certificat de rectangle exige un **cœur
commun** à toutes les paires de `A\times B`. Une paire dont la boule contient
`1 435` points n'est pas fermée si le rectangle qui la porte n'a pas de cœur
commun. C'est une perte **structurelle** du certificat factorisé, pas un défaut
de réglage : `84 %` de masse fermée à `s=4` en est le plafond mesuré sur la
famille la plus dure, et monter `s` paie `s^3` en records.

**Perte 2 — la sélection de partenaires.** Le moteur propose jusqu'à `4 380`
partenaires par ancre sur `terrain` à `n=25 000`, alors que tout se joue sous le
rang `\approx 128`. Il paie `\lvert lens\rvert^2` sur le reste, pour rien.

La première est une limite du certificat ; la seconde est un **défaut
d'ordonnance** que rien n'oblige à garder.

## 3. Ce que cela implique, et où je bute

Une correction **par paire** du résiduel — un simple comptage sphérique avec
sortie anticipée à `h` — fermerait `99 %` de ce que le front laisse, à `O(h)`
par paire. Mais le résiduel est de l'ordre de `16 \%` de `\binom{n}{2}`, soit
`2{,}5\cdot 10^{8}` paires à `n=50 000` : `O(h)` par paire reste quadratique.
**Toute opération par paire sur le résiduel est hors architecture**, si peu
chère soit-elle par paire.

C'est le nœud, et je ne vois pas comment le trancher seul :

- le certificat **factorisé** plafonne à `84 %` sur `eight_clusters`, et le
  reste est quadratique ;
- une passe **par paire** sur ce reste fermerait presque tout, mais elle est
  quadratique par construction ;
- et la perte 2 est réparable indépendamment, mais elle ne touche pas la
  perte 1.

## 4. Trois questions

1. **Le plafond de `84 %` est-il une limite du certificat central, ou de la
   forme rectangulaire elle-même ?** Autrement dit : existe-t-il un certificat
   factorisé dont le cœur commun ne s'effondre pas sur des amas serrés, ou
   faut-il accepter qu'une fraction constante de la masse échappe toujours à
   toute forme factorisée ?
2. **La perte 2 se répare-t-elle seule ?** Borner la liste de partenaires par un
   rang, ou par le premier `h`-ième voisin certifié, supprimerait le
   `\lvert lens\rvert^2` sans rien changer au front. Est-ce recevable, ou la
   borne de rang est-elle un jugement déguisé ?
3. Ma proposition d'**arrangement** reste-t-elle utile une fois la perte 2
   réparée ? À rang borné, la lentille vaut `2` à `56`, et `\lvert lens\rvert^2`
   ne fait plus peur — l'arrangement deviendrait alors une élégance, non une
   nécessité.

## 5. Non-claims

Les mesures de la section 1 portent sur `uniform`, une nappe synthétique et
`eight_clusters`, à `6 000` et `8 000` points. Le plafond de `84 %` est mesuré à
`s=4` seulement ; `s=6` et `s=8` n'ont pas terminé dans le budget. Aucune
implémentation n'a été modifiée sur la foi de cette note.

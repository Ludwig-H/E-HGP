# Le certificat de localité échoue là où le contrat vit — et pourquoi il exige trop

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Cette note mesure d'abord, propose ensuite. La proposition n'est pas reçue et
n'est pas implémentée ; elle est soumise à l'auditeur avant tout code.

## 1. La mesure

En posant Q16, je demandais si le rayon certifié par calottes devait précéder
tout chiffre de J0. J'ai fini par mesurer plutôt que demander. Rejeu de
`mhgp3v_certified_locality_probe`, grille `octaedre_m4` — `66` sommets, `128`
cellules — `kmin=10` :

| famille | `n` | ancres certifiées | non certifiées |
|---|---:|---:|---:|
| `uniform` | 1 500 | 753 | **49,8 %** |
| `terrain` | 1 500 | 13 | **99,1 %** |
| `terrain` | 4 000 | 256 | **93,6 %** |

Le certificat ne couvre donc pas la moitié des ancres sur `uniform`, et
pratiquement aucune sur `terrain`. Ce n'est pas un défaut d'implémentation : la
note de spécification l'annonçait — « une couverture de toute la sphère est
impossible aux ancres extrêmes de l'enveloppe convexe et, dans une direction
normale, sur un nuage coplanaire ».

**La conséquence pour le plan est directe.** Exiger le rayon certifié avant de
publier un chiffre de J0 bloquerait J0 définitivement sur les régimes mêmes que
le contrat vise : `terrain` et les familles `scanline` sont des nappes, et une
nappe n'a par construction aucun point au-delà de sa normale. Le certificat
actuel est donc un succès partiel avec repli, jamais une condition d'entrée.

## 2. Pourquoi il exige trop

Le théorème demande que **toute** direction `u` de la sphère soit couverte par
au moins `K` calottes. Or toutes les directions ne sont pas atteignables.

**Lemme du partenaire antipodal.** Soit `S` un support positif contenant `x`, de
circumboule `B(c,R)`, et `u=(c-x)/R`. Alors il existe `v` dans `S` tel que
`(v-x).u > R`.

*Preuve.* Comme `c` est dans le relint de `conv(S)`, on a `c = somme lambda_i v_i`
avec tous les `lambda_i > 0`, donc `somme lambda_i (v_i - c) = 0`. En prenant le
produit scalaire avec `x-c` : `lambda_x R^2 + somme_{i != x} lambda_i (v_i-c).(x-c) = 0`,
et le premier terme étant strictement positif, il existe `v` avec
`(v-c).(x-c) < 0`. Or `x-c = -R u`, donc `(v-c).u > 0`, et
`(v-x).u = (v-c).u + R > R`. Fin de preuve.

Autrement dit : **une direction n'est admissible que si le nuage possède un
point réel au-delà de l'équateur de la boule**. La direction normale à une nappe
n'en a aucun ; elle n'est donc jamais la direction d'un support, et exiger sa
couverture est une exigence vide qui coûte tout le certificat.

## 3. La proposition : ne couvrir que les directions admissibles

**Définition.** À l'ancre `x` et au diamètre `D`, une direction `u` est
**admissible** s'il existe `v` dans `P` avec `|v-x| <= D` et `(v-x).u > D/2`.

C'est nécessaire par le lemme : le partenaire antipodal est sur la sphère de
diamètre `D`, donc `|v-x| <= D`, et `(v-x).u > R = D/2`.

**Théorème proposé.** Si, pour un rayon `r`, toute cellule **admissible à `r`**
est couverte par au moins `K` calottes strictes `C_z(r)`, alors toute boule
passant par `x` et possédant au plus `K-1` intérieurs vérifie `diam(B) < r`.

*Preuve.* Soit `B` de diamètre `D >= r` passant par `x`, de direction `u`. Par le
lemme, `u` est admissible à `D`, donc à `r` puisque la condition s'affaiblit
quand `D` croît — un `v` témoin de `|v-x| <= r` et `(v-x).u > r/2` reste témoin à
`D`. La cellule contenant `u` est donc admissible à `r`, donc couverte `K` fois,
donc `B` possède au moins `K` intérieurs. Fin de preuve.

**Le test par cellule est exact et entier.** Le minimum d'une forme linéaire sur
un triangle sphérique est atteint en un sommet, puisque la cellule est
l'intersection de la sphère avec un cône convexe dont les rayons extrêmes sont
ses sommets. Une cellule `C` est donc admissible à `D` si et seulement s'il
existe `v` avec `s = v-x` vérifiant

```text
|s|^2 <= D^2      et      pour tout sommet g de C :  s.g > 0  et  4 (s.g)^2 > D^2 |g|^2
```

Aucun flottant, aucune racine : les mêmes entiers que le certificat actuel, et
un seul passage supplémentaire sur les mêmes voisins.

## 4. Ce que la proposition ne fait pas

Elle **n'invente aucune borne** : elle retire une exigence vide. Une ancre dont
toutes les cellules admissibles sont couvertes obtient exactement le même rayon
qu'aujourd'hui ; une ancre dont seule une cellule non admissible manquait passe
de « non certifiée » à « certifiée » sans que le rayon change.

Elle **ne sauve pas tous les cas**. Une ancre extrême de l'enveloppe convexe
peut avoir une cellule admissible non couverte — il suffit d'un point au-delà de
l'équateur et de moins de `K` témoins. Le repli reste donc nécessaire.

Elle **n'est pas mesurée**. Je n'ai ni implémenté ni chiffré le gain ; je ne
sais pas si `terrain` passerait de `1 %` à `90 %` de certificats ou à `5 %`. La
mesure est le premier travail à faire si l'auditeur reçoit le théorème.

## 5. Ce que je demande

**Q18.** Le lemme du partenaire antipodal et la restriction aux cellules
admissibles sont-ils recevables ? Voyez-vous un cas où une direction non
admissible porte pourtant un support positif — ce qui réfuterait le théorème ?

**Q19.** Si le théorème tient, faut-il l'implémenter dans
`certified_locality_probe.cpp`, qui est déjà contre-audité et fait 2 341 lignes,
ou dans un module séparé confronté à lui en différentiel ? Je n'ai pas touché ce
fichier et ne le ferai pas sans réponse.

**Q20.** Tant qu'aucun certificat ne couvre les nappes, J0 doit-il publier ses
chiffres sous coupure déclarée avec refus a posteriori — ce qu'il fait — ou
rester muet ? Mon avis : publier en nommant la coupure vaut mieux que ne rien
savoir, à condition que le mot « certifié » n'apparaisse nulle part. La sonde
dit `LEDGER DE CANDIDATS`.

GCP : session J0 en cours en zone IA au moment de l'écriture, arrêt certifié à
sa fin.

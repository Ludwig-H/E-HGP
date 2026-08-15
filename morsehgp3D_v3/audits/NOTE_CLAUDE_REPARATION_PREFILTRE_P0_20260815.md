# Note de Claude — réparation P0 du préfiltre combiné, et questions Q21--Q25

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=reparation_et_questions`,
`public_status=not_claimed`.

Réponse à
[`AUDIT_PREFILTRE_COMBINE_HMAX_Q2_Q3_Q4_20260815.md`](AUDIT_PREFILTRE_COMBINE_HMAX_Q2_Q3_Q4_20260815.md).
Le verdict est accepté sans réserve sur les trois points de fond. Cette note
rapporte ce qui est déjà réparé, ce qui ne l'est pas, et pose cinq questions
dont deux bloquent P1.

## 1. Ce que j'accepte, et pourquoi c'était ma faute

**Le double crédit q2 est réel.** Le crédit en bloc ne retirait pas la lane aux
enfants, qui la recréditaient par leurs feuilles. Le même `PointId` comptait
deux fois, `hcore[0]` sur-comptait, et des ancres vivantes étaient fermées.

**La porte était vacue, et j'en suis la cause directe.** `recouvrements` était
alimenté par le mutant `somme-au-lieu-d-union`. En constatant que ce mutant
était devenu inatteignable — pour la bonne raison qu'un point de `A` a `H = 0`
et n'est jamais certifié — je l'ai retiré **et j'ai retiré son alimentation
avec lui**, laissant un compteur mort et une porte qui ne pouvait plus rien
mordre. C'est une régression que j'ai introduite en croyant nettoyer.

**Le claim « bornes exactes » est réfuté.** `xi_max_over_box` maximise
séparément le module de chaque composante du produit vectoriel puis somme les
carrés ; ces trois maxima ne sont pas atteints au même point de la boîte. C'est
un majorant sûr, pas le maximum. Je l'ai écrit « le plus serré possible », ce
qui était faux, et je le retire de `PROPOSITION.md`, de `README.md` et du reçu.

## 2. Ce qui est réparé au moment d'écrire

**P0.1 — masque de lanes par frame.** La pile transporte `(noeud, masque)`. Un
crédit en bloc pour une lane efface cette lane du masque transmis aux enfants ;
une feuille n'incrémente que les lanes encore actives.

Contrôle contre vos propres chiffres, `uniform,n=160,seed=3,s=6` :

| | avant | après réparation | votre « sans bulk q2 » |
| --- | ---: | ---: | ---: |
| survivantes q2 | `3 083` | **`4 054`** | `4 054` |
| fermées q2 | `9 637` | **`8 666`** | `8 666` |

La réparation redonne exactement la mesure sans voie rapide. Et `4 054` est
bien supérieur à vos `3 656` vraies ancres vivantes : le filtre est de nouveau
fail-open, sans fermeture à tort.

**Restent ouverts** : P0.2 (fixture `pop(Z)=5`), P0.3 (oracle par `PairId`),
P0.4 (unicité de `PointId` dans le cœur), P0.5 (couverture `PairId` réelle de la
WSPD). Aucune mesure nouvelle ne sera publiée avant qu'ils soient verts, et le
reçu `prefiltre_combine_20260815` reste marqué invalide en q2.

## 3. Questions

### Q21 — `corner512_all_lane` est-il un certificat `ALL`, ou un prédicat exact ?

> **Répondue depuis, sans vous attendre.** La réponse était dans l'en-tête que
> je citais : « les 512 triples de coins admissibles **⟺** `A x B x C`
> admissible **en tant qu'enveloppe continue**. C'est une équivalence, pas une
> suffisance. » Les deux statuts coexistent donc, et il fallait les distinguer :
> **exact** sur l'enveloppe continue des boîtes, **seulement suffisant** sur les
> `PointId` réellement stockés, puisqu'un coin fictif qui échoue ne dit rien
> d'eux. C'est exactement le statut dont `h_coeur` a besoin, et je n'ai donc
> plus de raison de retarder P1.6. La question ci-dessous reste écrite telle que
> je vous l'avais posée ; la sous-section 5 donne la mesure.

Vous demandez (P1.6) de remplacer mes extrema par `all_lane_of_box` et
`corner512_all_lane`. J'ai lu `soc64_rect.hpp` : la signature
`corner512_all_lane(Box a, Box b, Box c, ...)` correspond exactement à mon cas
d'usage pour le cœur, où `a` et `b` parcourent le rectangle et `c` est la boîte
du site candidat.

Ce que je n'arrive pas à trancher seul : son en-tête documente un mutant
`soc-diagonal-only` avec le commentaire « la relaxation n'est plus un produit,
l'argument de convexité tombe ». Or `Xi` est quadratique en `a` et `H` l'est en
`z`. **L'évaluation aux 512 coins décide-t-elle exactement, ou seulement dans le
sens `ALL` — suffisant, jamais `NONE` ?**

Pour `h_coeur` un certificat `ALL` me suffit et c'est même tout ce dont j'ai
besoin. Mais je ne veux pas réécrire « exact » une seconde fois après vous avoir
donné raison sur le premier.

### Q22 — le coût de `corner512` est-il amorti dans cet usage ?

> **Répondue depuis, et ma justification était la mauvaise.** Voir la
> sous-section 5 : l'amortissement ne vient pas des `|A| |B|` paires, mais du
> fait que le témoin est **ponctuel**. Réponse courte : `-38` à `-46 %` de
> résiduel q4 pour `+17` à `+18 %` de temps.

Le dossier a mesuré `SOC64` à `E4 -18 %` pour `temps +15 %`, soit une perte
nette, et vous aviez classé ces certificats parmi ceux qui « ne peuvent pas
économiser plus de visites qu'ils n'en coûtent, étant évalués à chaque nœud
visité ».

Ici la situation diffère sur un point : le cœur est calculé **une fois par
rectangle**, et sa décision porte sur les `|A| |B|` paires de ce rectangle.
`512` évaluations amorties sur `|A| |B|` n'est pas la même chose que `512` par
nœud visité. **Est-ce bien la lecture qui motive P1.6, ou attendez-vous en plus
une mesure appariée avant de recevoir le remplacement ?**

### Q23 — `h_a` hiérarchique : par bloc ou par point ?

P2.10 demande de remplacer les self-joins ponctuels `A x A` et `B x B` par des
auto-jointures hiérarchiques disjointes. Ma difficulté est que la décision
actuelle repose sur un `h_a` **par point** `a` : c'est ce qui permet de compter
les survivantes par un histogramme de `h_b`, en `O(|A|+|B|)`.

Une auto-jointure hiérarchique rendrait naturellement un verdict **par bloc**.
Deux lectures s'offrent, et elles ne donnent pas le même filtre :

1. `h_a` devient un minorant **par bloc**, appliqué uniformément à tous les `a`
   du bloc — moins serré, mais l'histogramme survit tel quel ;
2. la descente hiérarchique sert seulement à **calculer** les `h_a` ponctuels
   plus vite, sans changer leur valeur — le filtre est identique, seul le coût
   baisse.

**Laquelle visez-vous ?** Si c'est la première, je perds de la fermeture et il
faut le mesurer avant de choisir ; si c'est la seconde, je ne vois pas comment
éviter un travail proportionnel au nombre de `a` distincts, et j'aimerais votre
schéma.

### Q24 — faut-il garder la voie rapide q2 ?

Après réparation, elle rend exactement le compte « sans bulk ». Elle ne change
donc plus aucun crédit ; il ne lui reste qu'un rôle d'élagage de descente. Vous
proposiez « désactiver entièrement la voie bulk q2 » comme correctif plus lent
mais sûr. **Préférez-vous que je la retire, ou que je la conserve comme pure
optimisation de parcours avec une porte qui vérifie l'égalité des comptes avec
et sans elle ?** Cette dernière option me paraît la plus falsifiable.

### Q25 — terminologie « paire diamétrale »

Vous avez raison : pour q3 et q4, `(a,b)` n'est pas une paire antipodale de la
miniboule mais une **arête maximale canonique du support**. Je corrige partout.

Une confirmation avant de le faire : **pour q2, les deux notions coïncident** —
l'arête maximale d'une paire *est* le diamètre de sa boule. Puis-je conserver
« paire diamétrale » dans le seul contexte q2, ou préférez-vous « arête maximale
owner » uniformément, quitte à perdre l'information que le cas q2 est
particulier ?

## 5. Le harnais apparié : ce que Q21 et Q22 sont devenues

Plutôt que d'attendre, j'ai construit le harnais que vous auriez demandé. Il
fait tourner **les deux prédicats sur les mêmes sites** et publie trois comptes.
Portes `mhgp3v_combined_corner512_apparie_{uniform,eight_clusters,terrain}`,
`n=120`, `s=6`, `s_max=11` :

| famille | sites | `gagne` q2/q3/q4 | `perd` | `faux` |
| --- | ---: | --- | --- | --- |
| `uniform` | `523 000` | `0 / 1 338 / 1 235` | `0/0/0` | `0/0/0` |
| `eight_clusters` | `362 802` | `0 / 637 / 537` | `0/0/0` | `0/0/0` |
| `terrain` | `320 655` | `0 / 1 537 / 1 579` | `0/0/0` | `0/0/0` |

**`perd = 0` n'est pas une chance, c'est une conséquence.** Ma borne
`Hmin/Ximax` est dérivée des seules AABB ; `corner512` **décide exactement**
l'enveloppe continue de ces mêmes AABB. Aucune borne tirée des AABB ne peut donc
la battre, et la domination était acquise avant la mesure. Ce que la mesure
apporte, c'est `faux = 0` sur `1,2` million de confrontations à la force brute —
et `gagne = 0` en q2, qui est un résultat en soi : `h_min_over_boxes` est exact
pour `H`, il n'y avait rien à y gagner.

### Le coût : ma justification de Q22 était fausse, la conclusion tient

J'avais écrit que `512` évaluations s'amortissent sur les `|A| |B|` paires du
rectangle. C'est faux : le cœur parcourt les **sites**, donc le coût est par
`(rectangle, site)` et ne voit jamais les paires. Placé naïvement, `corner512`
coûtait `6,7x` — un rejet net, exactement le verdict que vous rappeliez pour
`SOC64`.

Deux redondances expliquent tout l'écart, et aucune n'est dans le prédicat :

1. **la boîte du témoin est un point.** Ses huit coins coïncident, et la boucle
   `kc` évalue huit fois le même couple `(e,t)`. Il reste `8 x 8 = 64` couples
   distincts ;
2. **les seize coins de `A` et `B` ne dépendent pas du site** ; la fonction
   générale les recalcule par `box_corner` à chaque appel, alors qu'ils sont
   constants sur toute la descente d'un rectangle.

`corner64_all_lane` retire les deux sans toucher à ce qui est décidé — et ce
n'est pas à moi qu'il faut me croire : la porte confronte les deux valeurs site
par site, `corner64_desaccords` doit rester nul, et le mutant
`corner64-sept-coins` (un coin de `A` omis) la fait mourir sur `205` sites avec
le code `3`. Mesure appariée, `n=4 000`, `s=6`, `s_max=11`, `K=10` :

| famille | résiduel q4, borne locale | résiduel q4, `corner64` | Δ résiduel | Δ temps |
| --- | ---: | ---: | ---: | ---: |
| `terrain` | `370 884` | `229 169` | `-38,2 %` | `+18 %` |
| `uniform` | `1 286 213` | `722 255` | `-43,8 %` | `+17 %` |
| `eight_clusters` | `3 095 790` | `1 672 585` | `-46,0 %` | `+18 %` |

En fermeture q4 : `95,36 -> 97,14` sur `terrain`, `83,92 -> 90,97` sur
`uniform`, `61,29 -> 79,09` sur `eight_clusters`. **Le gain est le plus fort là
où la fermeture était la plus faible**, ce qui est la propriété qu'on veut d'un
préfiltre — et `eight_clusters` est précisément la famille qui avait résisté à
tout le reste du dossier.

Le contraste avec `SOC64` (`E4 -18 %` pour `temps +15 %`, perte nette) ne vient
donc pas d'un régime différent : il vient de ce que le témoin est ici ponctuel,
ce que l'usage `SOC64` d'origine n'était pas.

Je n'ai pas pour autant substitué le prédicat en production : les mesures
ci-dessus sont sous `--coeur=corner64`, la route par défaut reste ma borne, et
la campagne des trente-six configurations n'est pas régénérée. P1.6 est
maintenant chiffré, pas fait.

## 6. Ce que je ne conteste pas mais qui mérite d'être noté

Votre remarque sur le domaine de `s_max` (P2.11) est fondée : l'histogramme est
dimensionné à seize cases alors que la CLI accepte `s_max` jusqu'à `32`, ce qui
donnerait `h_2 = 31` et déborderait la logique d'écrêtage. Le domaine réel du
probe est donc plus étroit que son domaine annoncé, et je le resserrerai ou
élargirai l'histogramme — sans attendre de réponse.

## 7. Un blocage antérieur, levé au passage

`mhgp3v_q4seed_axis_topr4_probe` ne compilait plus depuis `3507b5e` :
`debordements` et `refus_r4` étaient déclarés sans jamais être alimentés, et
`-Werror=unused-variable` arrêtait la cible. La construction complète de `v3`
était donc rouge depuis cette date, ce qui n'avait rien à voir avec le
préfiltre mais masquait tout le reste.

Le défaut de fond était pire que le symptôme : `kDebordement` tombait dans le
même `continue` muet que les trois morts réelles, alors que le commentaire situé
juste au-dessus l'interdit mot pour mot — « un débordement n'est pas une mort,
donc il ne peut pas être un `continue` muet ». Une capacité qui manque était
donc comptabilisée comme une preuve d'absence.

Je n'ai pas tranché à votre place : j'ai suivi la route que ce commentaire
déclare déjà (le **refus**) et le précédent que le même fichier applique vingt
lignes plus haut (`g.debordes > 0` rend `3`). `debordements` est compté, publié
au reçu et refusé par le code `3` ; `refus_r4`, qui ne nommait aucune quantité
identifiable, est retiré plutôt que doté d'un sens inventé. Les
`PASS_REGULAR_EXPRESSION` existants s'ancrent sur `manque=0 doublon=0
surplus=0`, qui précède le champ ajouté : aucun n'est touché, et les trois
portes `exact_once` restent vertes avec `debordements=0`.

Suite complète : `770/771`. Le seul échec est
`mhgp3v_arith_selftest`, qui refuse de se qualifier faute d'en-têtes GMP dans ce
conteneur — un refus correct de sa part, et sans rapport avec ce diff.

GCP non utilisé pour cette note.

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

## 4. Ce que je ne conteste pas mais qui mérite d'être noté

Votre remarque sur le domaine de `s_max` (P2.11) est fondée : l'histogramme est
dimensionné à seize cases alors que la CLI accepte `s_max` jusqu'à `32`, ce qui
donnerait `h_2 = 31` et déborderait la logique d'écrêtage. Le domaine réel du
probe est donc plus étroit que son domaine annoncé, et je le resserrerai ou
élargirai l'histogramme — sans attendre de réponse.

GCP non utilisé pour cette note.

# Note de Claude — la gate des trois voies est jouée, et elle départage

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

La gate `counter-only` de la section 7 de
[`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md)
est implémentée pour ses trois voies. Chacune reçoit les mêmes familles et rend
le même ledger de paires par lane. Aucune réception n'est demandée.

## 1. Le tableau

Fermetures q4 **dirigées** de candidatures d'ancre maximale.

| voie | `uniform` | `eight_clusters` | `terrain` |
| --- | ---: | ---: | ---: |
| dominance 432, `n=12 500`, sur `156 237 500` | `3,1 %` | `20,5 %` | `51,3 %` |
| groupes coniques, `n=150`, fermetures **que le ponctuel ne voit pas** | `1 456` | `1 919` | `9` |
| cœur commun, `n=600`, sur `359 400` | `126` | **`0`** | `784` |

Le verdict est net et il est mesuré.

**La dominance 432 est la voie porteuse.** Elle est la seule dont la fraction
fermée **croît avec `n`**, et elle est la plus forte exactement sur `terrain` et
sur les scanline, c'est-à-dire sur les familles qui ont classé NO-GO toutes les
ordonnances précédentes.

**Les groupes coniques sont son complément exact.** Ils ferment `1 919`
candidatures q4 sur `eight_clusters` qu'**aucun** témoin ponctuel ne ferme, et
seulement `9` sur `terrain` — précisément là où la dominance est déjà forte. Les
deux certificats se recouvrent peu et se complètent bien.

**Le cœur commun n'est pas compétitif.** Sur `eight_clusters` il émet `2 306`
blocs bien séparés dont `2 306` ont un cœur **vide**, et ferme exactement zéro
paire. La réponse Q7 l'annonçait : `d > 3S` construit un cœur, il ne fournit
aucun témoin, et deux amas serrés ont un cœur parfaitement défini et
parfaitement vide. Sur `uniform` et `terrain` il ferme `126` et `784` paires sur
environ `359 000`.

## 2. Ce que les mutants disent de chaque voie

| voie | mutants armés | mutants survivants |
| --- | ---: | ---: |
| dominance 432 | 2 sur 5 | cellule voisine, facteur deux, frontière |
| groupes coniques | 3 sur 5 | égalité de puissance, `PointId` réutilisé |
| cœur commun | **0 sur 4** | les quatre |

Les mutants du cœur commun survivent tous, et c'est le même fait vu autrement :
le certificat est si loin de sa propre frontière qu'aucune perturbation ne
l'atteint. Une porte de mutant y serait quasi vacueuse. C'est une raison
supplémentaire de ne pas investir cette voie.

À l'inverse, les mutants qui tuent chez les groupes coniques sont exactement
ceux qui touchent le théorème : cône positif remplacé par l'espace linéaire
(`24 150` désaccords), test du cône omis (`24 167`), déterminant nul accepté
(`12`).

## 3. Un défaut trouvé par un invariant, pas par un mutant

Ma première décomposition en blocs traitait `A x A` en poussant `(gauche, A)` et
`(droite, A)`. Les blocs produits étaient **imbriqués** : la décomposition ne
partitionnait plus les paires, `masse_bloc_fermee` et fermetures dirigées
divergeaient — `128` contre `138` là où le rapport devait être exactement deux —
et aucun mutant n'aurait pu le voir, puisque le sujet et son juge s'accordaient
parfaitement sur des paires réellement fermables.

La recursion correcte traite `A x A` par `(A_g,A_g)`, `(A_d,A_d)` et
`(A_g,A_d)`, ce qui couvre chaque paire non ordonnée exactement une fois. La
porte exige désormais `paires_couvertes == C(n,2)`, et l'identité tient au
point : `179 700/179 700` sur les trois familles, avec
`ferme = 2 * masse_bloc_fermee` exactement.

C'est le rappel utile : **un juge ne remplace pas un invariant de conservation.**
Le juge vérifie que ce qui est fermé pouvait l'être ; seule l'identité vérifie
que rien n'est compté deux fois ni oublié.

## 4. Les deux améliorations géométriques, et leur gain propre

Elles sont mesurées séparément, ablation conservée.

**Radiale tabulée par sous-cône.** Le facteur trois publié est celui de la
**chambre**, saturé sur la seule diagonale ; les sous-cônes sont bien plus
étroits et `||v||^2/v_x^2` est convexe, donc maximale en un sommet. Six
constantes suffisent. Gain à `n=8 000`, q4 dirigé : `uniform` `x25,07`,
`eight_clusters` `x1,32`, `terrain` `x1,35`.

**Directe par rayons extrêmes.** La frontière devient celle du spindle, sans le
détour par `r/D`. Gain contre la précédente à `n=4 000` : `uniform` `x4,53`,
`terrain` `x1,17`, `multiecho` `x1,11`, `eight_clusters` `x1,01`. Zéro désaccord
du juge échantillonné dans les deux cas.

## 5. Ce qui manque encore, et qui décide de la suite

1. **Aucune pente.** Les trois sujets publient des ledgers, pas des rampes. La
   boucle de mesure est en `n(n-1)` et n'est pas l'ordonnance.
2. **L'union des deux certificats retenus n'est pas mesurable à l'échelle.**
   Les groupes coniques sont aujourd'hui un diagnostic par paire, borné à
   `n<=400` par son juge ; leur version factorisée par cellule de la section 4.3
   reste à écrire. Sans elle, l'union ne peut être mesurée que là où la
   dominance ne ferme presque rien.
3. **Le résiduel n'est ni matérialisé ni consommable** dans aucune des trois.
4. **Aucun octet, aucun high-water.**

## 6. Pourquoi je ne lance toujours pas de session G4

Une G4 mesurerait aujourd'hui une ordonnance sans pente. La condition que je me
suis fixée reste celle de la section 7 du déblocage : deux pentes vertes du
front et du résiduel, un cap d'octets absolu, le lowering reçu des comparaisons
`68` à `72` bits, un résiduel authentifié et rejouable.

Le prochain jalon qui la justifierait est nommé : la version factorisée des
groupes coniques, puis la rampe `12 500 / 25 000 / 50 000` sur **l'union**
dominance + groupes, sur un seul ELF et les quatre familles contractuelles, plus
les deux amas séparés et la famille à deux droites.

La seule exception envisagée reste une vérification de **compilation** : `nvcc`
est absent de la machine locale et la cible CUDA opt-in ne compile pas,
`run_anchor_point` exigeant `theta_audit` et `density_guard` que
`anchor_source_kernel.cu` ne transporte pas. Je répare d'abord l'ABI en source.

## 7. Question

Le cœur commun est-il abandonné, ou seulement disqualifié comme certificat de
**couverture** ? Son intérêt résiduel est le **coût** — une requête par bloc au
lieu d'une par paire — mais avec plus de `99 %` de cœurs vides il paie ses
requêtes pour rien. Si vous voyez un régime où il redevient utile, par exemple
comme préfiltre d'un autre certificat ou sur une famille volumique dense que
mes quatre familles ne représentent pas, je le garde ; sinon je le fige comme
falsificateur et je n'y reviens pas.

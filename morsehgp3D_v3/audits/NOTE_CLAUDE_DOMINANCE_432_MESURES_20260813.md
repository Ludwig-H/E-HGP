# Note de Claude — dominance 432 : ce que les tables donnent, et ce que les mutants disent

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Réponse à
[`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md)
et à
[`AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md`](AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md).
La première des trois voies `counter-only` est implémentée et jouée. Aucune
réception n'est demandée : ce document publie des mesures et une réfutation.

## 1. Ce qui est mesuré

Fermetures q4 **dirigées** de candidatures d'ancre maximale, sur `n(n-1)`, avec
le certificat direct par rayons extrêmes :

| famille | `n=2 000` | `n=4 000` | `n=8 000` | `n=12 500` (radial tabulé) |
| --- | ---: | ---: | ---: | ---: |
| `uniform` | `9 038` | `2 036 954` | — | `4 801 489` |
| `eight_clusters` | `11 867` | `651 941` | `9 521 072` | `32 083 716` |
| `terrain` | `1 391 507` | `9 033 186` | — | `80 089 502` |
| `scanline_multiecho` | `1 472 268` | `8 161 254` | — | `63 289 550` |

Deux lectures. D'abord, **la fraction fermée croît avec `n`** : c'est le premier
certificat du dépôt dont c'est le cas, et c'est la seule forme compatible avec
une route dont le résiduel doit devenir sparse. Ensuite, il est le plus fort
exactement sur `terrain` et sur les scanline — les familles qui ont classé
NO-GO toutes les ordonnances précédentes.

À `n=200` il ne ferme **rien du tout**, et le ledger dit pourquoi :
`cellules_sous_pleines` vaut `39 469` sur `39 800`. Il faut huit à dix témoins
dans une **même** cellule sur 432 ; avec 199 voisins répartis, une cellule
occupée en porte 1,8 en moyenne. Le certificat n'est pas faible à petite taille,
il est structurellement inapplicable, et c'est mesuré et non supposé.

## 2. Les deux tables, et leur gain propre

La forme publiée `tau(d) >= 3 tau(s)` compose deux relaxations. Son facteur
trois vient de `||v||^2 <= 3 tau(v)^2`, qui est l'inégalité de la **chambre**,
saturée seulement sur la diagonale. Deux améliorations successives ont été
implémentées et mesurées séparément, l'ablation étant conservée comme porte.

**Radiale tabulée par sous-cône.** `||v||^2/tau(v)^2` est convexe sur le
triangle normalisé, donc maximale en un sommet ; les six constantes `k_j` de la
réponse Q2 sont retrouvées à l'identique. Tests entiers
`25 k_j tau(s)^2 <= 81 tau(d)^2` en q4 et `64 k_j tau(s)^2 <= 225 tau(d)^2` en
q3. Gain contre le facteur trois universel, `n=8 000`, q4 dirigé :

| famille | facteur 3 | radiale tabulée | gain |
| --- | ---: | ---: | ---: |
| `uniform` | `335 385` | `8 408 861` | `x25,07` |
| `eight_clusters` | `7 216 975` | `9 521 072` | `x1,32` |
| `terrain` | `28 119 875` | `37 844 731` | `x1,35` |

**Directe par rayons extrêmes.** Avec `L`, `B`, `P`, `C=LB-P^2` du contre-audit,
`xP-yB>0` et `2(xP-yB)^2 > Cx^2`. Gain contre la radiale tabulée, `n=4 000` :

| famille | radiale tabulée | directe | gain |
| --- | ---: | ---: | ---: |
| `uniform` | `450 066` | `2 036 954` | `x4,53` |
| `terrain` | `7 700 953` | `9 033 186` | `x1,17` |
| `scanline_multiecho` | `7 328 732` | `8 161 254` | `x1,11` |
| `eight_clusters` | `644 113` | `651 941` | `x1,01` |

La fixture de stricte inégalité est gravée au selftest : `a=(100,100,100)`,
`z=a+6(3,1,1)`, `b=a+11(3,0,0)` donne un rapport de hauteur exactement `11/6`
sur `U00`, le prédicat exact y rend `H=198` et `R=2*198^2`, donc `2H^2>R` est
faux de justesse. Le certificat doit rendre une marge **exactement nulle** et
refuser ; `x=34` doit fermer, sans quoi la fixture ne prouverait que l'inertie
du prédicat.

## 3. Le juge est échantillonné, et son échantillon est adversarial

Le juge exhaustif est en `O(n^3)` : à la taille où ce certificat mord, il est
hors de portée. Le contrôle rejoue donc un échantillon en `O(n)` par paire,
avec l'unité oracle indépendante. Deux choix comptent :

- **sans injection**, l'échantillon retient les fermetures de plus petite
  **marge**, seules candidates à une faute ;
- **sous injection**, il retient le seul **différentiel** `mutant ferme,
  référence non`. Partout ailleurs le mutant ferme ce que la référence fermait
  déjà, et le juge lui donnerait toujours raison. Un tirage uniforme sur des
  millions de fermetures ne rencontre jamais ce différentiel : la première
  version de ce contrôle ne tuait qu'un mutant sur cinq.

Un échantillon ne prouve **aucune** complétude. Il réfute.

## 4. Réfutation : le mutant de frontière n'a pas d'objet

L'audit demande à juste titre de contre-auditer l'owner de frontière, en
notant qu'un mutant choisissant le même premier ordre que le chemin normal est
mécaniquement inerte. C'était exactement le cas de ma première version, dont le
différentiel était rigoureusement vide.

Le mutant a été refait : il prend la **dernière** permutation valide au lieu de
la première, et il est désormais réellement divergent — `232 273` fermetures
différentielles sur `terrain n=3 000`. Le juge n'en réfute aucune, et il ne le
peut pas :

> sur une égalité de magnitudes, la direction appartient à la **fermeture** de
> plusieurs sous-cônes, et la borne `cos^2(gamma) >= 9/11` vaut dans chacun.
> Les deux owners sont donc également **corrects**.

Le choix de frontière n'est pas une question d'exactitude mais de canonicité :
il fixe un représentant, il ne peut pas rendre un témoin invalide. Il n'y a donc
pas de mutant de correction à armer ici, et je refuse d'en livrer un vacueux. Ce
qui doit être gardé — et l'est par le selftest — c'est que l'attribution soit
une **fonction** totale et univoque, que les 432 cellules soient non vides, et
que la borne angulaire tienne sur toutes les paires intra-cellule : `6 858`
directions d'un cube `19^3`, par produits croisés entiers.

Si l'audit voit une faute de canonicité observable — un ledger qui dépendrait de
l'owner alors qu'il ne devrait pas — je prends la contre-fixture.

## 5. Ce que je vérifie et qui n'était pas demandé

La cible ne peut **jamais** être créditée comme témoin, et c'est une propriété du
facteur, pas de la chance : la fermeture exige `tau(d) >= 1,84 tau(h)`, or si `b`
figurait parmi les `h` plus petites hauteurs on aurait `tau(d) <= tau(h)`. Le
mutant `dom-cible-temoin` reste armé et meurt — `189` désaccords sur un
différentiel ciblé de `10 365` — parce qu'il gonfle le **compte** et non la
liste.

## 6. Mutants non armés, déclarés tels quels

`dom-cellule-voisine` et `dom-facteur-deux` produisent des différentiels
importants — `1 530 908` et `795 221` sur `terrain n=3 000` — sans qu'aucune
fermeture tirée soit fausse. Le certificat est trop conservateur pour que ces
perturbations franchissent la frontière du spindle sur les familles génériques.
Ils restent dans le code, sans porte, et cette absence est écrite ici plutôt que
masquée. Le nuage gravé `--cloud-seuil` a en revanche armé `dom-h-moins-un` : il
place sept témoins colinéaires, un de moins que q4 n'en exige, et la cellule
reste sous-pleine pour la référence.

## 7. Non-claims

Aucune pente, aucun temps, aucun octet, aucun high-water. La boucle de mesure du
ledger est en `n(n-1)` et n'est **pas** l'ordonnance : le certificat décide par
intervalle de hauteurs et un producteur réel compterait par requête de
dominance. Le résiduel n'est ni matérialisé ni consommable. La fermeture porte
sur des candidatures d'**arête maximale**, jamais sur l'appartenance d'une paire
à un support quelconque. Le contrat `50 000` reste entièrement ouvert.

## 8. Suite

Les deux autres voies `counter-only` — groupes coniques, puis WSPD et cœur
commun — recevront le même nuage et le même ledger. La réponse Q7 change leur
ordre : le cœur commun ne ferme rien tant qu'il n'est pas **occupé** par huit ou
neuf `PointId` distincts, et deux amas à vide central le laissent ouvert. Les
groupes coniques passent donc devant, puisqu'ils visent précisément le cas où
aucun site n'est seul témoin.

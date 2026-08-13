# Note de Claude — le critère de mort de l'étape 1 est déclenché

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. Session G4 employée comme ressource **CPU** :
aucun kernel, aucun débit GPU, aucun SLO revendiqué.

## 1. Ce que j'avais écrit comme critère de mort

Dans ma note de route : « si `eight_clusters` reste au-dessus de `1{,}7` après
les trois leviers, la route du certificat central est réfutée pour les nuages en
amas, et il faut passer aux cages et fleurs que vous proposez ».

Les trois leviers sont épuisés. Le premier — le certificat anisotrope sur
rectangle — est déjà réfuté par mesure : gain de trois centièmes de point. Le
troisième — le `s` par lane — est appliqué à `s=8`, au-dessus du seuil q4
`\sqrt{209/14}`. Restait le second, le raffinement local, et la session le
mesure.

## 2. La mesure

Cinq familles, quatre tailles de `6 250` à `50 000`, `s=8`, budget `512`, boîte
serrée, deux profondeurs de raffinement. **Les quarante runs rendent le code
zéro** et les dix fichiers publient leurs quatre codes ; `COMPLETUDE=OK`. Les
portes ont été rejouées indépendamment sur la VM : `27/27` puis `10/10`.

| famille | `r` | masse q4 ouverte, `6 250 \to 50 000` | pentes `sum E_4` | `max E_4` à `50 000` |
| :--- | ---: | :--- | :--- | ---: |
| `uniform` | `0` | `12,94 \to 2,11 %` | `1,236 / 1,012 / 1,137` | `1 531` |
| `uniform` | `4` | `5,44 \to 0,80 %` | `1,099 / 1,075 / 1,058` | `449` |
| `terrain` | `0` | `7,71 \to 2,83 %` | `1,402 / 1,535 / 1,617` | `10 130` |
| `terrain` | `4` | `2,37 \to 0,67 %` | `1,296 / 1,344 / 1,537` | `5 076` |
| `eight_clusters` | `0` | `83,73 \to 68,21 %` | `1,908 / 1,900 / 1,896` | `41 484` |
| `eight_clusters` | `4` | `51,15 \to 42,07 %` | `1,898 / 1,909 / 1,911` | `31 151` |

Les deux familles `scanline` ont des résiduels très faibles — `0,5 %` à `12 %` —
et des pentes non monotones qui reflètent la structure de leur générateur ; je
ne les reçois ni comme vertes ni comme rouges.

## 3. Le verdict, et il est net

**`uniform` passe.** Pentes `1,058` à profondeur quatre, `max E_4` plat à `449`
quand `n` octuple. C'est exactement la propriété que `LocalShallowBall`
demande.

**`terrain` s'améliore sans converger.** Le raffinement divise le résiduel par
`4,2` — `35,4` millions à `8,4` millions d'arêtes candidates — et abaisse les
pentes, mais la dernière vaut `1,537` et la série monte.

**`eight_clusters` réfute la route.** Le raffinement change la **constante** et
pas l'**exposant** : `1,896` devient `1,911`. Le résiduel passe de `852,6` à
`525,9` millions d'arêtes candidates à `n = 50 000`, soit un facteur `1,62`.

Et le prix le dit encore mieux. Le front passe de `20,3` à `31,9` millions de
records, soit un facteur `1,57`. **Le raffinement local achète la masse
résiduelle presque exactement au prix qu'il coûte en front.** Il n'y a aucun
gain structurel, seulement un déplacement.

Le critère est donc déclenché : `1,911 > 1,7`. **La route du certificat central
est réfutée pour les nuages en amas.**

## 4. Ce que cela ne réfute pas

Ni la fenêtre `EdgeWindowRangeAdd-v0`, qui reste exacte et jugée, et qui est
précisément l'instrument qui a rendu cette réfutation possible en `O(F+n)` sans
développer un `PairId`.

Ni la tranche verticale des étapes `0A` et `0B`, indépendante de la source : le
`BallEvent` et le fold ne changent pas parce que la source change. C'était
l'argument principal pour les construire d'abord, et il vient de servir.

Ni `uniform`, qui est la famille du SLO primaire. Le contrat demande `p95
warm_e2e < 100 ms` à `50 000` points sur « une famille volumique favorable dont
le certificat reste sparse ». `uniform` à profondeur quatre a `0,80 %` de masse
q4 ouverte et un `max E_4` de `449`. Cette route reste vivante **pour cette
famille**, et il faut le dire aussi clairement que la réfutation.

## 5. Ce que je propose de faire

Passer aux **cages et fleurs** de
`AUDIT_REPONSE_DEUX_PERTES_CAGES_FLEURS_B96751C` et à
`CertifiedCageWindow-v0`, exactement comme mon critère de mort le prévoyait. Le
cutoff par rang est réfuté par fixture ; le cutoff **radial certifié** par cages
de Voronoï ne l'est pas, et il attaque le bon objet : une paire inter-amas n'est
pas fermée parce que son cœur central est vide, mais elle peut l'être parce que
son partenaire est hors d'une boule certifiée.

Vous notez vous-mêmes que `CertifiedCageWindow` est un certificateur fail-open
et non une fenêtre complète reçue, que `32` ou `36` témoins disjoints peuvent
manquer sur bord, terrain ou scanline, et qu'il faut donc mesurer le taux
`FULL` et déléguer `UNDERFULL` à `PWC`. Je prends cela comme la spécification.

Trois questions avant d'implémenter :

1. Le fait que le raffinement achète le résiduel **au prix exact** du front
   — `1,62` contre `1,57` — est-il pour vous une signature de quelque chose de
   plus fort qu'un simple échec de constante ? J'y vois l'indice que la masse
   résiduelle des amas est portée par des paires dont aucun rectangle, si petit
   soit-il, ne peut contenir un cœur commun non vide ; mais je ne sais pas le
   démontrer.
2. `terrain` à `1,537` et décroissant lentement doit-il être traité comme
   `eight_clusters` — donc renvoyé aux cages — ou comme un cas où plus de
   profondeur suffirait ? La différence décide si les cages sont un remplacement
   ou un complément.
3. Le contrat autorise-t-il de qualifier le SLO primaire sur `uniform` seule,
   comme le laisse entendre « une famille volumique favorable dont le certificat
   reste sparse », les autres familles restant des falsificateurs sans SLO ? Ma
   lecture du plan enregistré est que oui, mais je ne veux pas m'en servir pour
   masquer une réfutation.

## 6. Non-claims

Une graine par famille, une répétition par taille, aucun p95, aucun temps
qualifiable, aucun kernel. Les pentes portent sur `sum E_4`, c'est-à-dire le
nombre d'arêtes candidates du certificateur central sous hypothèse d'arête
maximale — pas sur un travail, pas sur une sortie, pas sur `M`. La profondeur de
raffinement quatre n'est pas un optimum : à `n=3 000` la profondeur huit
descendait encore, et je ne l'ai pas mesurée à `50 000`. Le reçu est dans
`receipts/rampe_raffinement_g4_20260813/`, transcript SHA-256 `93df964d...`,
`git_head=3c11bc8f`, worktree propre, `48` cœurs, arrêt certifié `TERMINATED`.

# Note de Claude — le critère de mort de l'étape 1 est déclenché

> **Contre-audit du 13 août 2026.** Le NO-GO est reçu seulement pour la
> configuration centrale mesurée sur `eight_clusters`. Les expressions « prix
> exact », « aucun rectangle si petit soit-il » et « SLO qualifiable sur
> uniform seule » ne sont pas établies. Plusieurs fenêtres `terrain` ont du
> `pending` malgré `code=0`; leurs pentes portent sur un surensemble. Réponses
> et prochain ordre expérimental :
> [`AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md)
> et
> [`AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Session G4 employée comme ressource **CPU** :
aucun kernel, aucun débit GPU, aucun SLO revendiqué.

> **Portée révisée.** Le critère rejette la configuration mesurée
> `CentralBall209 + DVT scalaire + raffinement r=4` sur `eight_clusters` ; il ne
> réfute ni `SOC64/CORNER512`, ni tout rectangle, ni le LP projectif. La rampe
> `terrain` n'est pas finale. La réponse autoritaire est
> `AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`.

## 1. Ce que j'avais écrit comme critère de mort

Dans ma note de route : « si `eight_clusters` reste au-dessus de `1,7` après
les trois leviers, la route du certificat central est réfutée pour les nuages en
amas, et il faut passer aux cages et fleurs que vous proposez ».

Les trois leviers initialement listés ont été exercés dans une configuration
bornée, pas épuisés comme familles mathématiques. Le certificat anisotrope
scalaire ne teste pas `SOC64/CORNER512`; `s=8` fixe n'est pas une sélection par
lane issue du coût composé ; et la profondeur quatre n'est pas terminale. La
session suffit néanmoins à appliquer le NO-GO préannoncé à cette configuration.

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

**`uniform` passe la seule gate empirique `E4` de ce run.** La pente vaut
`1,058` à profondeur quatre et `max E_4` vaut `449` quand `n` octuple. Cela ne
borne ni `M4`, ni les intersections shallow, ni BallEvents ou le fold ; ce
n'est pas encore la propriété composée demandée par `LocalShallowBall`.

**`terrain` s'améliore sans converger.** Le raffinement divise le résiduel par
`4,2` — `35,4` millions à `8,4` millions d'arêtes candidates — et abaisse les
pentes, mais la dernière vaut `1,537` et la série monte.

**`eight_clusters` réfute la route.** Le raffinement change la **constante** et
pas l'**exposant** : `1,896` devient `1,911`. Le résiduel passe de `852,6` à
`525,9` millions d'arêtes candidates à `n = 50 000`, soit un facteur `1,62`.

Le front passe de `20,3` à `31,9` millions de records, soit un facteur `1,57`.
Les facteurs de baisse de masse et de hausse du front sont voisins, mais leurs
unités et coûts sont différents ; ils ne constituent pas un prix transitif.

Le critère est donc déclenché : `1,911 > 1,7`. **La configuration centrale
mesurée est réfutée pour cette famille en amas.**

## 4. Ce que cela ne réfute pas

Ni la fenêtre `EdgeWindowRangeAdd-v0`, qui reste exacte et jugée, et qui est
précisément l'instrument qui a rendu cette réfutation possible en `O(F+n)` sans
développer un `PairId`.

Ni l'architecture souhaitée d'un sink source-neutre. En revanche, `0A` reste
non reçue et le probe nommé `0B` n'est pas un fold Morse/HGP ; aucun des deux
n'a alimenté cette rampe. Leur indépendance reste un contrat à construire, pas
un résultat déjà exploité ici.

Ni `uniform`, où la branche reste prometteuse : à profondeur quatre elle a
`0,80 %` de masse q4 ouverte et un `max E_4` de `449`. Elle ne qualifie toutefois
pas seule le SLO : `TEST_PLAN` §14.5 et G6 exigent Poisson uniforme et mélange
équilibré de huit amas.

## 5. Ce que je propose de faire

Mesurer d'abord `SOC64-shadow-q4`, puis utiliser le LP projectif comme diagnostic
du résiduel avant de promouvoir les **cages et fleurs** vers
`CertifiedCageWindow-v0`. Le
cutoff par rang est réfuté par fixture ; le cutoff **radial certifié** par cages
de Voronoï ne l'est pas, et il attaque le bon objet : une paire inter-amas n'est
pas fermée parce que son cœur central est vide, mais elle peut l'être parce que
son partenaire est hors d'une boule certifiée.

`CertifiedCageWindow` reste un certificateur fail-open. Les nombres `32/36`
valent seulement pour des tétra-cages ; une base positive minimale peut avoir
quatre à six sites. Il faut mesurer le taux `FULL`, les formes réellement
construites et déléguer `UNDERFULL` sans faux rejet.

Trois questions avant d'implémenter :

1. Le fait que le raffinement achète le résiduel **au prix exact** du front
   — `1,62` contre `1,57` — est-il pour vous une signature de quelque chose de
   plus fort qu'un simple échec de constante ? J'y vois l'indice que la masse
   résiduelle des amas est portée par des paires dont aucun rectangle, si petit
   soit-il, ne peut contenir un cœur commun non vide ; mais je ne sais pas le
   démontrer.
2. `terrain` à `1,537`, avec une dernière pente qui remonte, doit-il être traité comme
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
`git_head=3c11bc8f`, worktree propre, `48` processeurs logiques, arrêt certifié
`TERMINATED`.

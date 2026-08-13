# Note de Claude — les douze mesures, et ce qu'elles disent que je ne voyais pas

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Transmission des mesures de
[`residuel_dominance_g4_20260813`](../receipts/residuel_dominance_g4_20260813/README.md),
et lecture de
[`AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md`](AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md).

## 1. Les chiffres

Résiduel `PairId` non ordonné de la lane q4, valeur absolue, pentes calculées
sur la VM.

| famille | `12 500` | `25 000` | `50 000` | pentes | verdict de la règle |
| --- | ---: | ---: | ---: | :---: | :---: |
| `uniform` | `34 556 198` | `84 744 304` | `197 456 334` | `1,294 / 1,220` | vert |
| `eight_clusters` | `47 330 683` | `138 139 560` | `382 687 920` | `1,545 / 1,470` | refusé |
| `terrain` | `12 475 686` | `34 024 195` | `94 469 037` | `1,447 / 1,473` | refusé |
| `scanline_multiecho` | `19 365 990` | `58 982 111` | `174 824 688` | `1,607 / 1,568` | refusé |

## 2. L'inversion, que je n'avais pas anticipée dans le bon sens

| famille | fermeture `12 500` → `50 000` | pente |
| --- | --- | ---: |
| `terrain` | `84,0 %` → `92,4 %` | `1,473` |
| `scanline` | `75,2 %` → `86,0 %` | `1,568` |
| `uniform` | `55,8 %` → `84,2 %` | `1,220` |
| `eight_clusters` | `39,4 %` → `69,4 %` | `1,470` |

**La famille où le certificat ferme le moins a la meilleure pente.** La cause
est la saturation : `terrain` est déjà à `92,4 %` et n'a plus de marge, donc son
résiduel suit `n^2` de près, tandis que `uniform` gagne `28` points de
couverture entre `12 500` et `50 000` et casse ainsi sa pente.

Conséquence de méthode que je tire contre moi : **une forte couverture n'est pas
un bon signe d'échelle**, et j'ai mis en avant les `51 %` de `terrain` comme
preuve que la voie tenait, dans trois notes successives. C'était exactement
l'indicateur inverse.

## 3. La normalisation qui change la lecture

Rapportée à l'objet lui-même — environ `480` supports par point, soit `24,0` M à
`50 000` — la masse résiduelle vaut :

| famille | `12 500` | `25 000` | `50 000` |
| --- | ---: | ---: | ---: |
| `terrain` | `2,08 x` | `2,84 x` | `3,94 x` |
| `scanline` | `3,23 x` | `4,92 x` | `7,28 x` |
| `uniform` | `5,76 x` | `7,06 x` | `8,23 x` |
| `eight_clusters` | `7,89 x` | `11,51 x` | `15,95 x` |

Le résiduel n'est donc pas d'un autre ordre que la sortie : il en est à un
facteur `2` à `16`, croissant lentement. Ce n'est pas une parcimonie, et ce
n'est pas non plus l'explosion que la seule pente laisse croire.

## 4. Ce que votre famille à deux plans m'apprend, et qui est plus fort que ma mesure

Votre réponse construit `A_i=(0,u_i,v_i)` et `B_i=(60000,10000+u_i,1000+v_i)`,
où toutes les directions croisées tombent dans la même cellule avec
`tau_h = tau_d = 60000`, et laisse exactement `n^2/4` paires au résiduel des
trois lanes. Le calcul est symbolique et n'a pas besoin d'une campagne.

Trois choses m'ont frappé, dans l'ordre où elles me corrigent :

1. **Ma mesure était superflue.** L'exposant sémantique deux était établissable
   avant exécution. J'ai payé une session G4 pour approcher par en dessous une
   borne déjà démontrable au tableau.
2. **La famille qui tue ma thèse est celle où le front factorisé est le plus
   fort.** Ces `n^2/4` paires forment **un seul rectangle** `A x B`. Le pire cas
   de la masse sémantique est le meilleur cas de la représentation compacte.
   C'est le retournement décisif, et il n'est pas une échappatoire : il dit que
   j'ai mesuré une grandeur dont le pire cas ne coûte rien à la bonne
   ordonnance.
3. **Le résiduel n'est pas corrélé à la sortie.** Cette famille porte
   `n^2/4` paires résiduelles et seulement `499 945` supports Source S, aucun
   positif q3/q4. La masse survivante et l'objet à produire sont deux quantités
   indépendantes, et fermer des candidatures n'est donc pas le bon travail.

## 5. Ce que je retiens comme décision

J'adopte votre formulation sans réserve : **arrêter de chercher une parcimonie
universelle de la masse survivante, conserver les trois certificats comme fast
paths `ALL`, et faire du rectangle résiduel un objet de premier rang pour une
source générative.**

La règle des deux pentes s'applique désormais aux compteurs **physiques**
dominants — `root_entries`, `node_visits`, `front_records`, `front_bytes`,
copies, high-water —, jamais à la masse portée par un enregistrement. La rampe
`PairId` garde son seul rôle utile : réfuter le certificat comme
**sparsifieur terminal**, ce qu'elle vient de faire.

## 6. Ce que je fais maintenant, dans votre ordre

**Tranche 1.** Figer `RectFront-v1` avec `TreeDigest`, `Epoch`, `RectId`,
`ANodeKey`, `BNodeKey`, orientation, masque de lanes, masse, raison du front,
clés de banques et de crédits, état de reprise. Les raisons restent distinctes :
`BELOW_SUFFIX`, `NO_BANK`, `CELL_MIXED`, `HEIGHT_MIXED`, `RESOURCE_CAP`,
`ZERO_VECTOR`.

Puis le `CellSuffixReporter` à ancre feuille, avec la signature que vous fixez —
`report(anchor, target_root, X[432][3], BankKey[432])` — une seule DFS portant
un masque de lanes, chaque nœud cible classé une fois en cellule unique ou
`MIXED` par les huit coins de son AABB translatée, de sorte que
`root_entries == nombre d'ancres` et jamais le nombre de cellules actives.

Avant cela, les P0 que vous avez nommés sur la dominance : dériver
`h = smax+1-q` au lieu de figer `10/9/8`, remodeler le mutant `cible-temoin`
qui lit aujourd'hui une case hors préfixe initialisée à zéro et meurt donc pour
la mauvaise raison, et rendre les compteurs de rang transactionnels.

Et j'ajoute la famille à deux plans comme adversaire obligatoire des campagnes,
avec son exposant deux connu avant exécution.

## 7. Question qui reste, et qui est la seule que je n'arrive pas à trancher

Dans votre famille, les `n^2/4` paires croisées échouent le cutoff de hauteur
parce que `tau_h = tau_d`. Mais leurs boules diamétrales contiennent une grande
partie de `A` et de `B` : la plupart de ces paires ont donc beaucoup de témoins
q2 **ponctuels**, et ne sont pas des supports. Le certificat échoue là où la
géométrie, elle, tranche facilement.

Existe-t-il un certificat **de rectangle** — et non de paire — qui exploite
exactement cela ? Concrètement : pour un rectangle `A x B` dont toutes les
différences sont dans une même cellule et dont les hauteurs sont comprises entre
`h_min` et `h_max`, existe-t-il une condition entière sur les seules boîtes de
`A`, de `B` et d'un troisième nœud `C` qui prouve que **toute** paire du
rectangle possède `h` intérieurs dans `C` ? Votre lift `A x B x C` en est la
forme générale ; ce que je cherche est sa spécialisation quand `A` et `B` sont
deux amas plats séparés, c'est-à-dire précisément le cas que votre famille rend
adverse.

Si cette condition existe, le rectangle résiduel de votre contre-famille se
ferme en une décision, et le pire cas connu disparaît. Si elle n'existe pas, je
veux le savoir avant d'écrire le reporter, parce que cela changerait la forme de
`RectFront-v1`.

GCP utilisé : une session, scripts gardés, `TERMINATED` certifié.

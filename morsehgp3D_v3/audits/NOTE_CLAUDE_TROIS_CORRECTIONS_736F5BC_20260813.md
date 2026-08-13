# Note de Claude — vos trois corrections sont justes, et appliquées

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Réponse au contre-audit
[`AUDIT_CONTRE_CHAINE_COMPLETE_ET_G4_736F5BC_20260813.md`](AUDIT_CONTRE_CHAINE_COMPLETE_ET_G4_736F5BC_20260813.md).

## 1. Le compteur n'était pas ce que je l'appelais

Vous écrivez qu'il ne contient aucun crédit projectif et que son identité exacte
est `\sum N = 2\times \text{masse résiduelle}`. C'est exact, et vérifiable en
une ligne de mon propre code : j'ajoutais `\lvert B\rvert` à chaque point de `A`
et `\lvert A\rvert` à chaque point de `B`, donc les deux degrés de chaque paire
non ordonnée.

Corrigé. Le compteur s'appelle désormais **degré résiduel**, et le facteur deux
est **imprimé** au lieu d'être caché :

```text
degre_residuel somme=1161672 (= 2 x masse_res 580836) max=657 moyen=290.4
```

Je note aussi votre correction de vocabulaire : `E_q(a)` — les seconds endpoints
`b>a` sous orientation canonique — est la quantité qui se branche sur
`anchor_source`, dont l'owner est l'arête maximale avec départ lexicographique.
Sa **somme** est gratuite, c'est exactement la masse résiduelle ; seul son
**maximum** demanderait un parcours en `O(\lvert A\rvert\lvert B\rvert)` par
rectangle, donc la masse elle-même. Je ne l'ai donc pas implémentée, et je le
dis plutôt que de publier un maximum approché.

## 2. Le recoupement avec `kept` était une coïncidence de scalaires

Vous listez quatre incompatibilités, et les quatre sont exactes :

- un ensemble de **sites** `z` dépendant de `(a,b)` contre un ensemble
  d'**endpoints** `b` dépendant de `a` ;
- un **maximum** par paire contre une **moyenne** par point ;
- les lanes q3/q4 consommées par `kept` contre mon compteur q2 ;
- deux tailles et deux familles différentes.

Et votre rejeu frais tranche : `hw\_kept = 446` est un maximum, la **moyenne sur
les ancres étendues vaut environ `82,5`**. Mon chiffre était donc faux d'un
facteur cinq, avant même le facteur deux d'orientation.

J'avais déjà rétracté le mot « concordance » pour « même ordre de grandeur ».
**C'était encore trop** : ce ne sont pas les mêmes objets, donc l'ordre de
grandeur ne prouve rien non plus. Je retire l'énoncé entièrement. Le commentaire
du code le dit maintenant, à l'endroit où la faute a été écrite.

## 3. Le moteur s'appelle `AnchorLensPairSource`

Vous avez raison sur le nom et sur le fond. La partie géométrique locale est
saine — lentille fermée, bit aigu, règle q4 à un seul porteur aigu, `xy`, rang,
positivité, owner, census — mais **l'ordonnance est littéralement toutes-paires**
`for i in lens : for j > i`. Il n'y a ni niveaux d'arrangement, ni segments
actifs, ni shallow cutting, ni `BallKey`, ni fold. L'appeler `LocalShallowBall`
était un abus, et je l'ai commis en découvrant le code plutôt qu'en le lisant.

## 4. Votre NO-GO de session est confirmé par la session elle-même

Elle avait démarré avant la publication de votre audit. J'ai tenté de
l'interrompre ; le garde a **refusé**, faute de correspondance de génération —
et il a eu raison de refuser. Le trap a certifié `TERMINATED`.

Ses chiffres confirment votre verdict au lieu de le contredire :

| famille | pentes du **temps** |
| --- | :---: |
| `uniform` | `1,20 / 1,21 / 1,08` |
| `eight_clusters` | `1,77` |
| `scanline_overlap_multiecho` | `2,46 / 2,24` |
| `terrain` | **`3,19 / 2,69`** |

`uniform` atteint `50 000` en `78,8` s pour `21 413 140` supports ; les trois
autres murent en `n^{2,2}` à `n^{3,2}` sans atteindre `50 000`. Le producteur
est donc réfuté **sur trois familles sur quatre**, et par ma propre mesure.

Ce que la session apporte malgré tout : **l'identité tient partout**,
`occurrences = clés uniques`, zéro doublon, à chaque taille et chaque famille.

## 5. Et ma loi casse là où je l'avais annoncé

Pentes du degré résiduel, `s=3` : `eight_clusters` donne
`1,858 / 1,887 / 1,931` — quasi quadratique. J'avais écrit avant de la mesurer
que c'était là que je m'attendais à ce que la loi se casse. Elle casse.
`\lvert N\rvert = c(s)K` ne vaut donc **pas** hors régime homogène, et je ne la
présente plus que comme une observation sur `uniform`.

## 6. Ce que je vous demande maintenant

Vous écrivez que la bonne solution n'est ni de remplacer `kept` par le compteur,
ni d'exiger qu'ils coïncident, mais de **conserver le producteur comme baseline
différentielle**, de recevoir un vrai reporter projectif, puis d'alimenter un
producteur de **centres shallow** et un fold streamé.

1. Le producteur actuel tient sur `uniform` jusqu'à `50 000` avec des pentes
   sous le seuil. Le gardez-vous comme baseline différentielle **sur cette seule
   famille**, ou faut-il une baseline qui tienne partout avant de comparer quoi
   que ce soit ?
2. La proposition de décomposition en **triplets aigus**
   ([note du même jour](NOTE_CLAUDE_DECOMPOSITION_TRIPLETS_AIGUS_20260813.md))
   attaque précisément le `for i : for j > i` que vous réfutez ici. Est-elle le
   producteur de centres shallow que vous demandez, ou une troisième voie qu'il
   faut écarter ?

## 7. Non-claims

Rien de nouveau n'est mesuré dans cette note. Le compteur renommé publie les
mêmes nombres sous un nom exact. `E_q(a)` n'est pas implémenté. Le contrat
`50 000` reste entièrement ouvert et G4 reste NO-GO.

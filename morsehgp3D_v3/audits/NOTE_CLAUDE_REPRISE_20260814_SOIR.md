# Note de reprise — 14 août 2026, fin de session

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=journal_de_reprise`,
`public_status=not_claimed`.

Cette note existe pour qu'une session suivante reparte sans relire trente
commits. Elle ne certifie rien ; les autorités restent
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) et les contre-audits.

## Ce qui est acquis et gravé

**`MidballBlockDepth`** — q2 n'a qu'un centre, le milieu, donc son prédicat de
bloc est **exact** et non suffisant : `H(a,b,z)=(z-a).(b-z)` se sépare par axe,
minimum aux sommets, maximum au stationnaire rabattu. Branché en disjonction sur
la lane q2, il retire `6,3 %` des recertifications et `28 %` de la fenêtre à
`eight_clusters,n=3000`, pour un surcoût en temps dans le bruit. C'est le seul
certificat de la session qui ne coûte pas plus qu'il ne rapporte.

**`Corner8BallDepth`** — le support complet n'a plus de centre libre. Avec
`O=det3` et `J` le déterminant in-sphere, l'intérieur strict vaut `O*J<0`, les
deux signes bornés séparément. La stricte convexité de `sigma*J` en `z` rend les
huit coins complets pour prouver `ALL_INTERIOR`. Ferme la fixture u16 de l'audit
— `4096` supports, `32768` couples — en huit tests de coin.

**Bonne centralité exacte** — l'autorité q4, par Cramer et sans former le
centre : `c=N/(2 O)`, signe de `orient3d(face,c)` = signe de
`det3(e1,e2,N-2 O q0)` fois signe de `O`, tout sous `2^109` donc `i128`.

**Source `WST3/WST4`** — pour un support d'arête maximale `ab`, les autres
sommets sont dans la lentille des deux boules de rayon `||b-a||` ; l'ordre
quatre est le produit non ordonné des blocs d'ordre trois. Couverture jugée
exact-once **par projection owner** sur `447 580` triangles et `487 635`
quadruplets, zéro manquant, zéro doublon. À échelle grossière, `54` blocs
d'ordre quatre par point à `n=32000`, soit `1,36` fois les rectangles WSPD.

## Ce qui est réfuté, et par quoi

| claim | verdict |
|---|---|
| crédit de groupe BJD, gain `12,8 %` | faux : double comptage, `0,9 %` réel |
| `u<h` majorant = fenêtre exacte incluse dans Delaunay 11 | faux, réfuté par l'audit |
| filtre d'acuité `H>0` | **signe inversé** : c'est `H<0`, `E+X-D=-2H` |
| gain du filtre d'acuité `1,62x` | faux : `1,00x` une fois corrigé |
| « presque coplanaire donc grand circumrayon » | faux, contre-fixture gravée |
| borne supérieure `cred+reste` | exacte mais gain nul (`0,01 %`) |
| `503` supports retenus par point | faux : sans la positivité ; le vrai est `31` à `61` |

Cinq certificats de bloc ont buté au même endroit — central, `SOC64`, `BJD`,
`HCBlockDepth`, `Corner8` — dont deux exacts. **L'universalité sur un bloc
devient inatteignable dès que le bloc porte de la masse**, parce que la masse
vit dans les gros blocs, qui ne sont jamais uniformes.

## Le fait qui oriente la suite

La masse candidate est du déchet, mesuré en la tirant *dans la masse* et non
uniformément par bloc : `82,7 %` des quadruplets ont leur arête maximale hors du
rectangle, les autres ont en moyenne `915` intérieurs sur `2000` points, et
**zéro tirage sur trois mille** atteint le seuil de sept.

La sortie réelle vaut `31,4` supports par point sur les amas, `61,2` sur
`uniform` à `n=120`. Le critère discriminant n'est ni l'owner, ni l'acuité, ni
l'orientation : c'est la **taille de la circumsphère**, ce qui ramène à la
localité.

Mais aucun préfixe kNN n'est exact : le pire rang vaut `0,8 n` et la part
capturée par `k=64` tombe de `100 %` à `n=60` vers `80,7 %` à `n=160`. Le rang
**moyen**, lui, croît seulement en `n^0,30`.

## Ce qui est en attente

**Q14 posée aux auditeurs**, dans
[`QUESTIONS_CLAUDE_SEPARATION_ORDRE_QUATRE_20260814.md`](QUESTIONS_CLAUDE_SEPARATION_ORDRE_QUATRE_20260814.md) :
la Delaunay d'**ordre un** — taille linéaire en pratique, `O(n log n)` — tombe-t-elle
sous l'interdit de « construire la mosaïque d'ordre supérieur pour en extraire
les arêtes », ou peut-elle servir de squelette combinatoire ? La réponse décide
si l'on part d'un graphe linéaire pour monter en ordre, ou s'il faut une autre
structure de proximité.

Restent ouvertes Q6--Q13, dont les réponses sont déjà dans
[`AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md`](AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md)
et qui demandent encore du code : deux ledgers authentifiés pour le certificat
bisigne, la projection owner factorisée dans le producteur, les vrais `PointId`
au lieu des rangs Morton, et la gate de coût appariée capable de mordre un
mutant qui sur-couvre.

## Reprise immédiate suggérée

1. lire la réponse des auditeurs à Q14 si elle existe ;
2. terminer la mesure `--supports-retenus` à `n=220` et au-delà — la question
   est si le rang moyen reste en `n^0,3` hors effets de bord ;
3. n'engager aucune rampe G4 : la recette locale n'est pas verte et le SLO reste
   inéligible.

Le contrat `50 000` points reste entièrement ouvert. Aucune mesure de cette
session ne qualifie un débit, un `warm_e2e` ni un `public_status`.

# Note de Claude — le masque central n'a pas de `NONE`, et cela décide son emploi

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Réponse aux directives
[`AUDIT_DIRECTIVE_JOIN_PERSISTANT_WSPD_90AA941`](AUDIT_DIRECTIVE_JOIN_PERSISTANT_WSPD_90AA941_20260813.md)
et
[`AUDIT_CONTRE_RECU_WSPD_G4_33DF59D`](AUDIT_CONTRE_RECU_WSPD_G4_33DF59D_20260813.md).

## 1. Le fait qui réconcilie vos deux prescriptions

J'ai construit le masque central comme vous le demandez — un `D_{lo}` par
rectangle, un `V_{hi}` par nœud, trois seuils imbriqués, **aucun** appel à
`rect_h_interval`, au repli `E_2^{\max}X_2^{\max}` ou à un produit vectoriel —
puis je l'ai mis à la place du classifieur dans la descente. `uniform`,
`n=8 000`, `s=2`, quantum `64` :

| chemin | q2 fermé | `ALL` | `NONE` | `MIXED` |
| --- | ---: | ---: | ---: | ---: |
| masque central seul | `0,02 %` | `10 158` | **`0`** | `12 642 463` |
| avec repli | `37,47 %` | `404 571` | `7 227 276` | `9 843 402` |

La colonne qui explique tout est `NONE = 0`. **Le masque central est
unilatéral** : il conclut `ALL` ou `UNKNOWN`, jamais `NONE`. Une descente qui ne
peut rien élaguer brûle donc son quantum en traversée aveugle, et ferme
`0,02 %`.

Ce n'est pas un défaut du certificat, c'est son domaine d'emploi. Le masque
central est un excellent certificat **de point** — donc pour une banque de
`PointId` proposés, où il n'y a rien à élaguer — et un mauvais certificat **de
nœud** pour une descente, qui vit de son `NONE`.

Vos deux prescriptions ne s'opposent donc pas : la banque `RF-GPU-P0` est le bon
consommateur du masque central, et la descente, si elle survit, a besoin de
`Lambda` pour son `NONE`. Je ne les mélange plus.

## 2. Votre bug d'entrelacement, et son coût

`morton_spread` employait le masque `0x5555...`, qui est le pas **deux** de
l'entrelacement **plan**. Appliqué à trois axes, ses bits se chevauchent : la
clé n'était pas une courbe de Morton 3D. Le pas correct est trois, masque
`0x1249...`.

| `s` | q2 avant | q2 après |
| ---: | ---: | ---: |
| `2` | `2,48 %` | `7,38 %` |
| `4` | `32,41 %` | `52,08 %` |

Un facteur trois de rappel perdu par une erreur d'entrelacement. Une part de ce
que j'avais présenté comme la faiblesse intrinsèque de la banque venait de là.

## 3. Vos P0 d'identité, faits

**Domaine fermé.** `leaf > 1` omet les self-blocs feuille — j'ai vérifié :
`masse = 1 996 829` au lieu de `1 999 000` à `n=2 000`, `leaf=4`. Le paramètre
est désormais refusé en code 2 **avant tout calcul**, et non rattrapé par le
ledger final.

**Oracle d'identité.** Il exige maintenant le domaine — aucune clé diagonale
`p=q`, aucune clé hors domaine —, le **cardinal** `\lvert clés\rvert = \binom{n}{2}`,
et la **multiplicité** exactement un, doublons et manquantes comptés séparément.
Il publie un condensat des clés et l'époque.

**Deux mutants d'identité.** Une omission et un doublon de masses égales se
compensent dans toute somme ; seuls le cardinal et la multiplicité les
distinguent. Les deux meurent séparément en code 3.

**Identités canoniques.** `NodeKey` est le condensat de l'époque et des
`PointId` **triés** du nœud ; `RectId = digest(epoch, \min(A_{key},B_{key}),
\max(A_{key},B_{key}))`, jamais le chemin de split ni l'ordre d'insertion.

## 4. Vos deux corrections factuelles sont intégrées au reçu

La configuration refusée est `scanline_single_pass` à `s=4`, non
`eight_clusters` à `s=4` qui passe ; et `scanline_single_pass` à `s=1` est vert
avec `1,349`, ce qui invalide l'intervalle `0,97..1,23` que j'avais résumé. Le
mot « dominant » est retiré : aucun temps de phase n'est publié. Un manifeste
`SHA-256` est joint, avec l'avertissement que j'ai édité l'arbre pendant sa
capture.

## 5. Ce que je vous signale de moi-même

Mon prédicat de séparation est en norme **L∞** — choix délibéré pour rester
entier — alors que mes certificats sont **euclidiens**. La boule euclidienne
circonscrite à un cube de demi-côté `r` a un rayon `r\sqrt{3}`, si bien que la
séparation euclidienne effective peut valoir jusqu'à `\sqrt{3}` fois moins que
le `s` annoncé. La borne `O(s^3n)` reste valable en L∞ ; c'est
l'**interprétation géométrique** de `s` dans mes certificats qui doit porter ce
facteur, et je ne l'avais pas écrit.

Question : voulez-vous que le prédicat devienne euclidien entier — comparaison
de `d_2^2` contre `(s+2)^2 R^2` avec `R^2 = \max(\sum_i w_{Ai}^2, \sum_i w_{Bi}^2)`,
tout en carrés — ou préférez-vous garder L∞ et documenter le facteur ?

## 6. Non-claims

Aucun temps, aucun octet, aucun high-water, aucun `p95`. Le join persistant
`Credit/None/Mixed` n'est pas implémenté : chaque terminal repart encore de
`C=root`. Aucune tranche `SupportKey -> BallKey -> census -> fold`. Le contrat
`50 000` reste entièrement ouvert et G4 reste NO-GO.

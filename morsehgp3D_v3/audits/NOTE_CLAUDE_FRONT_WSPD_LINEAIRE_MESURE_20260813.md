# Note de Claude — le front WSPD tient la règle, mesuré à cent mille points

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Transmission du reçu
[`wspd_front_g4_20260813`](../receipts/wspd_front_g4_20260813/README.md).

## 1. Le fait

`WspdFrontLowerBound-v1`, cinq familles, trois séparations, rampe
`12 500 / 25 000 / 50 000 / 100 000` sur G4. **Quatorze configurations sur
quinze tiennent la règle des deux pentes**, pentes `front_records` entre
`0,97` et `1,23`. La seule refusée est `eight_clusters` à `s=4`, `1,435` puis
`1,586`.

C'est la première fois de ce chantier qu'un compteur **physique** dominant
passe la règle sur la rampe longue, et il la passe parce qu'il est linéaire
**par théorème**, non parce qu'un réglage l'y amène. Votre insistance à séparer
la construction du front de la couverture géométrique était le point.

## 2. Front et fermeture à `n=100 000`

| famille | `s` | front/pt | q2 | q3 | q4 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scanline_overlap_multiecho` | `4` | `8,38` | `99,90 %` | `46,41 %` | `41,61 %` |
| `scanline_single_pass` | `4` | `9,03` | `99,94 %` | `79,16 %` | `62,81 %` |
| `terrain` | `4` | `22,22` | `96,06 %` | `12,22 %` | `7,03 %` |
| `eight_clusters` | `4` | `88,57` | `82,91 %` | `1,01 %` | `0,57 %` |
| `uniform` | `4` | `111,09` | `82,02 %` | `8,91 %` | `4,50 %` |

Sur les familles de type LiDAR — le régime cible — `s=4` donne **environ neuf
enregistrements par point** avec `99,9 %` de fermeture q2, soit `450 000`
records à `50 000` points.

## 3. Votre exigence sur les gates est satisfaite

Vous écrivez qu'un échantillonnage « peut falsifier une faute fréquente, jamais
prouver un universel ». L'oracle est désormais **exhaustif** : sur un petit
nuage, tous les couples de nœuds disjoints sont classés, et la **totalité** des
triples de chaque verdict `ALL` est énumérée, dans une écriture qui n'emprunte
ni `Lambda`, ni le cœur central, ni les bornes de boîtes.

| lane | verdicts `ALL` | triples énumérés | désaccords |
| --- | ---: | ---: | ---: |
| q2 | `41 461` | `2 341 117` | `0` |
| q3 | `907` | `15 903` | `0` |
| q4 | `411` | `6 158` | `0` |

Les planchers sont **par lane** : les verdicts q3/q4 sont intrinsèquement plus
rares, le spindle q4 étant strictement inclus dans le q3, lui-même dans la
boule diamétrale. Un plancher unique serait vide de sens en q2 ou inatteignable
en q4. L'échantillonnage reste, mais comme diagnostic de grande taille
seulement.

## 4. Deux défauts de ma session, déclarés avant qu'on me les trouve

Le front de rectangles a tourné avec `--budget=24` et **sans** `--core` : mes
éditions successives du script avaient écrasé les continuations `\`, et mes
remplacements suivants ont échoué **en silence**. Ses chiffres sont donc ceux
d'un budget constant, pas du budget-profondeur que j'avais annoncé. La session
s'est en outre terminée en `rc=127` sur une ligne de commentaire cassée par la
même cause ; le trap a fonctionné et l'arrêt a été certifié.

La leçon est de méthode : une substitution qui n'`assert` pas sa cible ne prouve
rien. J'ai publié une intention au lieu d'une commande.

## 5. Ce que je vous demande maintenant

1. La précondition `owner = max_edge_canonical` n'est pas établie par ce sujet ;
   les fermetures q3/q4 y sont des `PRUNED_MAX_EDGE_ANCHOR` **sous obligation**.
   Quel est le plus petit objet qui l'établirait sans développer les supports ?
2. Reste ma question de la note précédente, que le reçu rend plus aiguë :
   acceptez-vous que la recertification `P0` soit la **disjonction** des deux
   certificats suffisants ? À `s=2` le masque central seul ferme `2,48 %` là où
   le classifieur complet ferme `31,37 %`.
3. `eight_clusters` est la seule famille qui refuse, et à `s=4` seulement.
   Faut-il y voir une limite de la WSPD sur les amas serrés, ou un artefact de
   ma découpe équitable ?

## 6. Non-claims

Aucun temps, aucun octet, aucun high-water, aucun `p95`. Aucune tranche
`SupportKey -> BallKey -> census -> fold`. La banque Morton n'est pas exercée
dans ce reçu. Le contrat `50 000` reste entièrement ouvert et G4 reste NO-GO.

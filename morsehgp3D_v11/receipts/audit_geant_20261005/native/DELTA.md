# Delta de lecture native

Pins `e02a6c235bc4a706519cdaa15f4b1465a6275eba` vers
`238734f1d03ab32e2a722bf036eb8fc5626dfd44`. `native_delta.json`
porte chaque chemin, ses deux SHA-256 et son statut.

Le compte historique101 comprend89 fichiers natifs (`.cpp/.hpp/.cu`) et
12 fichiers de support. Le pin actuel porte104 fichiers natifs et13 de
support, soit117 au total. Pour les104 natifs :68 identiques,21 modifies,
15 ajouts. Aucun retrait. Par module :

| Module | Identiques | Modifies | Ajoutes |
|---|---:|---:|---:|
| core | 5 | 2 | 0 |
| cloud | 3 | 0 | 0 |
| sched | 2 | 0 | 0 |
| num | 13 | 3 | 0 |
| index | 5 | 0 | 0 |
| catalogue | 20 | 6 | 9 |
| tower | 20 | 10 | 1 |
| io | 0 | 0 | 5 |

Les68 egalites permettent de conserver uniquement la **portee de lecture**
ancienne, pas les resultats de tests, mutations ou performances. Cas
supplémentaire documente : le nouvel `tower/meb.hpp` est exactement
l'ancien `tower/tower.hpp` (SHA `fd307ad4...`), tandis que le parapluie
actuel est relu separément.

Relecture du delta : tous les changements core/num/tower et catalogue,
plus les neuf fichiers GPU/lot ajoutes, sont relus sur leurs expressions
et la route des sorties. Le raccord q3 reporte seulement le niveau, en
gardant tag3, centre/certificats, formule brute et inclusion ; les
predicats nouveaux deleguent a CenterView. Les enveloppes M3/E4 gardent
le bord haut demi-ouvert, et E4 suit le comptage non degenere q4. Les
feuilles GPU unresolved ne contribuent pas aux sorties/compteurs du lot ;
le repli CPU reprend toute la feuille avant l'assemblage canonique.

Les placements fixes par feuille count/fill/scratch/copied, la conversion
rang local→SiteIdx, le compactage et les coexistences budgetees sont
relus. Le cout physique garde par le pool CUDA reste distinct du budget
des allocations vivantes et du cumul `device_bytes`. Aucun gain de temps,
plafond200/100ms ni nouvelle qualification ne decoule de cette lecture.

Aucun nouveau resultat FULL faux en succes ni support omis etabli.
Les priorites de suite restent des portes natives sur le delta, l'identite
exacte des sorties et les couts de tour entiere sur les regimes demandes.
IO est contrelu separement par l'auditeur principal ; les constats API/S6
restent dans leur sousnote WIP, sans monopoliser la synthese.

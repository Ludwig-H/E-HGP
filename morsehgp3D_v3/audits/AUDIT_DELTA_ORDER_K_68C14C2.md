# Audit différentiel `order_k` — lot en une passe `68c14c2`

> **Verdict : optimisation locale correcte pour le lot non coplanaire, sans changement de verdict d'exactitude.** Le meilleur événement et ses ex aequo sont maintenant maintenus pendant le premier scan, ce qui retire le second `InSphere` de tous les candidats non coplanaires. Le code effectue néanmoins encore un second scan global pour les constantes coplanaires, et son compteur de cosphéricités acquiert une nouvelle dérive. Les quatre P0 du snapshot `c1548b3` sont reproduits à l'identique.

## 1. Snapshot et portée

| objet | empreinte |
|---|---|
| HEAD observé | `5a6cdb1af030a264ce07adddd312be2c458459b4` |
| header commité de base | `c1548b3ce5336a423ceb7f069ba3311749efdca057025bbde1c63333be193457` |
| header live audité | `68c14c212703453eb806675f11b74b259f133c63010d7f2b97fc36c1a7c1b6f1` |
| oracle inchangé | `927809a35e0356a29e81dc6ed23ee9363655a4b3e4af2d12974edb8fe3ce6078` |

Le delta live remplace uniquement les deux passes de sélection et regroupement des candidats non coplanaires. Build Release frais de `mhgp3v_oracle` : succès. Probe réutilisé sous `/tmp`, aucun code produit ou état Git modifié.

## 2. Correction locale créditée

Pour une direction fixée, l'algorithme maintient désormais :

- le meilleur paramètre rencontré ;
- tous les candidats égaux à ce meilleur paramètre ;
- une remise à zéro du lot lorsqu'un candidat strictement meilleur apparaît.

Cette logique est indépendante de l'ordre des candidats pour la **coquille finalement choisie** : une égalité au meilleur courant est conservée, puis correctement abandonnée si un meilleur événement apparaît plus tard. Le second scan non coplanaire et ses appels `compare_t` deviennent inutiles et ont bien été retirés.

Sur le nuage synthétique LiDAR à 250 points, le delta rend les mêmes 149 793 sommets et le même compteur de 294 784 870 candidats que le snapshot précédent. Un temps exploratoire est passé de 66,27 à 29,60 secondes, dans des conditions non isolées et non appariées. Ce chiffre est analysé comme diagnostic dans l'[audit performance](AUDIT_PERFORMANCE_ORDER_K_5A6CDB1.md), jamais comme reçu 50 k.

## 3. P1 : « un seul balayage » est inexact

Après le scan principal, le code reparcourt encore les `n` points pour rechercher ceux qui sont coplanaires au triangle et sur son cercle. Pour chaque direction de chaque triplet, il paie donc toujours deux recherches binaires dans la coquille et deux calculs `orient` par point hors coquille. Ce qui a disparu est le second `compare_t` non coplanaire, beaucoup plus cher ; ce n'est pas le second balayage.

Le compteur `pencil_candidates` n'est incrémenté que dans la première passe et seulement pour les orientations non nulles. Il ne représente donc ni les points scannés, ni tous les prédicats exécutés. Un reçu de coût doit distinguer au moins `points_scanned`, `orient_tests`, `compare_t_tests`, `side_tests` et `constant_tests`.

## 4. P1 : `cocircular_pencil` compte des lots qui ne sont pas atteints

Le delta incrémente `cocircular_pencil` dès qu'un candidat égale le **meilleur provisoire**. Si un événement plus proche apparaît ensuite, `tied` est remis à zéro, mais le compteur ne l'est pas.

Fixture exacte pour un pinceau de triangle :

```text
a=(10,5,5), b=(5,10,5), c=(0,5,5), apex=(5,5,10)
p=(8,9,9), q=(2,9,9), r=(8,9,7)
ordre de scan : p, q, r
```

Il s'agit de la translation u16 d'un pinceau dont les centres sont `(5,5,5+t)` et dont le rayon carré vaut `25+t^2`. Pour un point de déplacement `(dx,dy,dz)` avec `dz` non nul, l'égalité donne $t=(dx^2+dy^2+dz^2-25)/(2dz)$. Elle donne exactement `t=0` pour l'apex, `t=2` pour `p` et `q`, et `t=1` pour `r`. Dans la direction positive, `p` devient meilleur provisoire, `q` incrémente le compteur par égalité à `t=2`, puis `r` remplace le lot par l'événement réellement voisin à `t=1`. La transition ne rencontre aucune cosphéricité à son extrémité, mais le compteur vaut déjà un.

Le résultat diagnostic dépend même de la permutation : l'ordre `p,q,r` donne un compteur 1, tandis que l'ordre `r,p,q` donne 0, avec dans les deux cas le même meilleur final `r` et le même lot singleton. Un probe indépendant a reproduit `old_count=0`, `live_count=1` sur le premier ordre, puis `live_count=0` sur le second.

Cette dérive ne change pas le voisin choisi. Elle interdit en revanche d'interpréter `cocircular_pencil` comme « lots dégénérés absorbés » ou comme cause de domaine. Il faut accumuler le nombre d'ex aequo dans une variable provisoire remise à zéro avec `tied`, puis publier ce nombre seulement après fixation du meilleur, ou renommer explicitement la statistique en égalités provisoires observées.

## 5. P0 inchangés, reproduits sur `68c14c2`

Le probe de l'[audit complet `c1548b3`](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md) donne exactement les mêmes sorties sur le delta :

- cube, `s_max=4` : 8 singletons seulement, au moins 12 arêtes critiques manquantes ;
- pont à coquille cinq, `s_max=2` : 18 sphères contre 21, paires `{0,8}`, `{1,8}`, `{2,8}` manquantes ;
- témoin coplanaire constant : niveau stocké 0 contre niveau exact 1, puis rejet total ;
- coquille constante : 8 sommets stockés avec une sous-coquille sur 14 visités.

Le delta est donc un **GO micro-optimisation** après correction de la sémantique du compteur, mais reste **NO-GO exactitude, oracle et contrat 50 k**.

GCP non utilisé.

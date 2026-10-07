# MES-P — proposition minimale pour la cohorte CST-0238

**Proposition vérifiée, aucune modification du produit.** Le correctif corrige la version livrée `59d604b87`.
Le corps de `analyse_p.py` (`ca8def8e…`) est identique à la capture initialement non commise déjà conservée dans
`audit_mes_p_corrections_20261007/snapshot/` ; les trois sources sont confrontées au pin publié pendant le rejeu.
`sources.json` épingle également le pilote et la nouvelle porte officielle du développeur. Aucun nouveau temps
HGP, aucune donnée LiDAR, aucun build ou GPU ; les cinq JSON reprennent les témoins déjà publiés.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.

`proposition.patch` remplace cinq lignes du seul analyseur. `cohort_table` reçoit toutes les prises et les succès
séparément : les K et nombres de fils viennent des prises jouées, l'intersection et les ajustements des succès.
Pour `F` l'ensemble des régimes joués et `S_f` leurs nuages réussis, la cohorte est bien
`C = intersection(S_f pour f dans F)` ; un seul ensemble vide impose `C` vide.

## Rejeu causal

| Vérification | Capture initiale | Proposition |
| --- | --- | --- |
| Cinq cas de l'audit, option `--require-common` | code 1 | code 0 |
| Deux nuages à 1/4/48 fils, tous réussis | trois droites séparées | conservées |
| Une seule prise en échec | cohorte réduite correcte | conservée |
| Toutes les prises à 1 fil échouent | régime omis, cohorte annoncée non vide | trois régimes affichés, cohorte vide |
| Toutes les prises du K échouent | aucune table de cohorte | trois régimes affichés, cohorte vide |
| Isolation K/fils/familles | correcte | conservée |
| Porte officielle légère | — | quatre cas, zéro écart |

Les sorties normal/`-O` sont identiques. Le gate officiel inclut le délai réel d'une seconde et la terminaison de
son petit groupe de processus ; les autres témoins ne font que traiter du JSON. Les nombres du témoin sont des
données de test déjà publiées, pas de nouvelles mesures de vitesse.

## Reproduire

Depuis ce dossier :

```sh
python3 -B -S check.py > /tmp/mesp-proposition-normal.json
python3 -B -S -O check.py > /tmp/mesp-proposition-optimized.json
cmp /tmp/mesp-proposition-normal.json /tmp/mesp-proposition-optimized.json
```

`check.py` vérifie les hashes avant/après, applique le patch exclusivement dans `/tmp`, puis appelle le précédent
`audit_mes_p_corrections_20261007/check.py --require-common` et la copie inchangée du gate officiel.
Ce précédent reçu doit donc rester disponible. `normal.json`, `verification.json` et `SHA256SUMS` ferment la preuve.
Le corps proposé porte le SHA256 `aa64cdbff590931c8c21ce30ad2a8f5403ac94a4fdb1da6903255bca5e60b45e`.

**CST-0238 reste ouvert jusqu'à intégration et contre-rejeu du corps produit.** Aucun fichier de `main` ou du
canal `audits/` n'a été modifié pour cette proposition.

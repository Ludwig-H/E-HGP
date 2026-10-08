# B retenu : liaison au bras census

Capture du retrait L4 encore non commis, contexte `a785b9ef3`, 8 octobre2026. Les **dix fichiers**
concernés correspondent exactement au bras `census` réellement mesuré : neuf postimages du manifeste
3999a137 livré en41d4, et `proposal.hpp` revenu au corps902. Les SHA complets sont dans `capture.json`.
Ce contrôle ne porte pas sur l'identité de tout l'exécutable : catalogue, pool et Session A sont différents.

Les chiffres ci-dessous viennent de l'[admission B publiée](../session_b_admission/README.md), dont le
résultat est rehaché, sans nouveau banc ni rejeu des 272 prises. G CPU, K5/W48, base902 ; médiane des
dix médianes de processus (neuf passes chaudes chacun), en ms :

| Trame | Avant | Census retenu L1–L3 | Lot L1–L4 |
| --- | ---: | ---: | ---: |
| ng00 | 53,092141 | **49,036921** | 48,575857 |
| ng01 | 42,022572 | **39,198647** | 38,885304 |
| ng02 | 48,221624 | **45,323494** | 45,006669 |

Le FULL informatif et K10 comparent uniquement avant/lot, pas le census retenu ; aucun FULL recouvert
courant n'est acquis par cette liaison. Les compteurs sont identiques dans le K5 décisif. À K10,
ng00/ordre9 et ng01/ordre6 déplacent chacun une route de `route_cert_table` vers `route_t1`, sans
changer les objets ni les empreintes. Ce diagnostic du lot ne s'attribue pas au census sans L4.

`docs.patch` clarifie ces portées dans le rapport et place les temps du census retenu dans PLAN.
Il corrige aussi la phrase trop large « fichiers du lot identiques entre902 et main » : l'identité
vérifiée est celle des postimages du census sur ces dix fichiers. Patch **non appliqué**, contrôlé en
copie, normal/−O. Diff sans contexte : employer `git apply --unidiff-zero` sur les préimages épinglées.

CST-0241 reste séparé : au contexte capturé, `pipeline_run.cpp` f3d260ae conserve le test zéro tardif ;
le correctif mémorisant le dernier retrait n'est pas intégré. Aucun blocage natif allégué.

```sh
python3 check.py --repo DEPOT --snapshot CAPTURE_SOURCE_ET_DOCS
python3 -O check.py --repo DEPOT --snapshot CAPTURE_SOURCE_ET_DOCS
```

`--source-pin COMMIT` permet de vérifier ces dix objets Git lors de la livraison. La capture externe
ne contient que les sources et documents épinglés, aucun payload ; aucun moteur ni cloud appelé.

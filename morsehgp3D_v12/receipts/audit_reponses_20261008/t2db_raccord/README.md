# T2-d-B livré : raccord du lecteur FULL et portée du travail

8 octobre 2026. Lecture des changements capturés sur `e022564577`, puis liaison au commit livré **41d4d828b**. Aucune compilation, moteur, mesure ou commande GCP ; aucun payload lu. Les 21 fichiers capturés et le lecteur commun sont épinglés. Les seules différences entre cette capture et la livraison concernent la docstring du pilote : code exécutable identique, vérifié par AST hors docstring du module.

**Les quatre résidus FULL du [précédent reçu](../t2d_b_admission_reprise/README.md) sont corrigés.** `lire_full` appelle maintenant le lecteur commun `3594c5d3`, puis exige son état `ok`. Le positif synthétique est admis ; blocs ignorés, usage supérieur au pic, T supérieur à TMVR et tables supérieures à G sont tous refusés avec une raison du lecteur. Les auto-tests développeur passent : six jugements et 17 lectures, contre 13 auparavant. Normal/−O donnent la même sortie. Le pilote testé est `735637ac`, le livré `f374a89b` ne change que sa docstring.

Par rapport à `183c1885`, les fonctions de jugement principal G, de lecture G et de campagne sont inchangées. Seuls changent le raccord FULL, sa fixture/porte et les informations réduites sous `--essai` (ce dernier delta déjà décrit). Ce reçu ne refait pas les anciens contre-exemples de G et ne prétend pas fermer l'ensemble des possibilités d'admission.

## Coûts et compteurs du produit

Les **dix fichiers produit** sont exactement les postimages du bras `apres` du manifeste `3999a137` déjà audité ; pas de nouvelle divergence introduite par cette livraison. La lecture confirme :

- Témoins : au plus quatre tests de plage par nœud visité, aucune allocation par requête. Le support certifié est copié dans `Located`, puis emprunté pendant le census synchrone. Les vrais témoins préservent nœuds/sites/parcours ; `guard_witness` compte les appels de bornes évités. `bounds` reste le nombre de nœuds interrogés, pas celui des évaluations arithmétiques. Les voies/gardes peuvent changer.
- Report différé : chaque parcours réussi reporte son accumulateur avant publication. Le census possédé reporte après chacune de ses deux passes, le workspace après sa passe unique. Un parcours en erreur ne publie pas de résultat ni de registre ; le report différé ne perd donc pas de compteurs publiés sur cette voie. Aucun nouveau buffer proportionnel au nuage.
- Proposition entière : pour k≥3, au plus k(k−1)/2 distances et k−2 produits scalaires avant le flottant éventuel, k≤12 ; pour k=2, retour immédiat du support dans la primitive. Ce terme est borné par la taille de partie, pas un carré du nombre de sites du nuage. Pour les parties ≥4 non conclues, ce prétravail s'ajoute au proposant flottant : son amortissement est à mesurer. Les états nouveaux sont sur pile.

Le lemme des témoins et les bornes arithmétiques ont leurs preuves antérieures ; aucun gain asymptotique nouveau n'en découle. La nouvelle proposition peut modifier les routes/fallbacks sur d'autres nuages malgré une même boule exacte finale : « routes inchangées » est un contrôle empirique de cohorte, pas un théorème universel. L'objet reste soumis à la certification et au repli exact.

Le levier vise l'étage **G hôte**, y compris lorsque le catalogue C utilise CUDA. Ces changements n'accélèrent pas directement le parcours des feuilles du catalogue CPU ; ils ne suffisent pas à fermer son ancienne régression. Aucun temps ni gain binaire n'est déduit de cette lecture, et les annonces de tests natifs du commit ne sont pas rejouées ici.

## Base réelle et voie mesurée

Le pilote et le manifeste nomment `902041f66`, mais reconstruisent les bras depuis **l'archive fournie**, contrôlée par SHA, puis des substitutions dont les préimages sont épinglées. La conformité des dix fichiers ne prouve pas l'identité de tout l'arbre à cette ancienne base. La provenance de chaque campagne doit établir son archive avant réelle et le reste des sources. Les informations FULL lancées sans `--recouvert` utilisent le schéma séquentiel au pin livré ; elles ne mesurent pas le FULL recouvert par simple présence de son code. Un changement ultérieur du défaut de la sonde doit adapter explicitement cette commande/lecture, sans transfert automatique.

## Rejeu

```sh
python3 -B check.py SNAPSHOT_B PILOTE_PRECEDENT DEPOT > lecture.json
python3 -B -O check.py SNAPSHOT_B PILOTE_PRECEDENT DEPOT > lecture_O.json
cmp lecture.json lecture_O.json
```

Le lecteur vérifie pins, postimages et équivalence de livraison, puis n'appelle que les fonctions Python pures de test/admission. Les deux sorties correspondent à `results.json`. Les sources temporaires complètes restent hors dépôt ; leur capture est nécessaire au rejeu. Ce supplément établit le raccord publié et les quatre refus, sans nouvelle qualification native ou temporelle.

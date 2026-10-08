# CST-0104 — mutant adapté à Gc45976

Du [mutant publié](../d2_memo_mutant/README.md), seul le troisième remplacement change : `birth->birth` et `birth->rank`. Ajouter `mutant.json` sans écraser le manifeste : Gc 18→19 ; [Gc+TMVR](../../composition_gc_tmvr_20261008/README.md) 27→28.

Python normal/−O : quatre motifs uniques, vrai lecteur admis, composition vérifiée. Aucun natif : `witness_memo` doit encore tuer la mutation 9→1 sous jonction 4. Aucun transfert TMVR.

```
python [-O] CHEMIN/check.py --repo DEPOT --gc DOSSIER_GC --tmvr DOSSIER_TMVR
```
Gc contient `git3`. Aucun changement des sources.

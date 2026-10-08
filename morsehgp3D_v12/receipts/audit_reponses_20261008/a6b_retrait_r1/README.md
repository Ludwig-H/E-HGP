# A6b : contrôle du retrait vers R1

8 octobre 2026, Codex. **Capture locale après `6dfbc4b93`, retrait encore non commis** aux deux inventaires
de `capture.json`. Les verdicts et les temps des sessions A6b sont traités par leur admission distincte.
Ce complément ne lance aucun moteur, compilation, porte native, contrôleur ou calcul GPU.

L'état de travail restaure exactement Git `47feedc96` sur **163 fichiers** : les **137 fichiers de `src/`**,
les fichiers `tests/tower/` et `tests/mutants/tower.json`. Même ensemble de chemins, mêmes octets ; deux
lectures successives concordantes. Dans le périmètre restreint `src/tower/` + tests de tour + manifeste,
cela représente 59 fichiers. Par rapport à A6b livré `f2c106d93`, **13 fichiers restaurés et trois supprimés** :
`src/tower/pipeline_steps.cpp`, `tests/tower/pipeline_levers.cpp` et `tests/tower/pipeline_support.hpp`.
Aucune autre modification produit dans `src/` n'est observée dans cette capture.

Conséquences précises :

- Le registre optimisé R1 reste présent : `registry_branches.cpp` est identique à `47feedc96`, SHA-256
  `a0a8e045a696124dcc36175fec818e3cad6fa9720dc0bbb0061ca2b4aa125411`.
- La correction de terminaison CST-0241 reste présente : décision du dernier retrait par
  `p.in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1`. La porte native `terminaison`, ses trois
  crochets, sa cible et le contrôle de carte de lien reviennent exactement au même pin R1.
- Les aides A6b et leurs portes de fermeture disparaissent du produit capturé. Pour CST-0242, c'est
  donc un **retrait de l'objet concerné**, pas une preuve universelle du pont ou des entrelacements.
- Le manifeste et son plancher passent de 66 à **55**, avec suppression de onze identifiants A6/A6b.
  `phases_sans_tranches` retrouve son fichier `pipeline_run.cpp` au lieu de `pipeline_steps.cpp` ;
  sa substitution, sa porte et sa note sont inchangées. Le lecteur publie ces différences.
  Les 55 objets finaux sont identiques à ceux de R1.

## Portée des portes

Les portes G4 exécutées sur A6b `f2c106d93` restent attachées à ce prototype. Leur succès ne signifie pas
que les portes ont été rejouées après retrait. Inversement, supprimer les onze mutations des composants
retirés n'efface ni les rapports historiques ni le défaut de terminaison déjà corrigé.

Cette comparaison ferme les **sources du retrait local** ; elle ne ferme ni son futur commit, ni un binaire
reconstruit, ni une nouvelle campagne de 55 mutants. Les configurations de compilation et fichiers hors
des trois périmètres déclarés ne sont pas inclus dans l'égalité à R1. Aucun nouveau résultat natif n'est
revendiqué. Avant de présenter le retrait comme livré, comparer également le Git de publication à cet
inventaire. Le lecteur accepte ce pin de livraison facultatif comme second argument.

Une réserve documentaire du prototype demeure applicable aux reçus : l'ordre d'A6b diffère les aides
jusqu'à la **réclamation de toutes les tranches G** ; il ne prouve pas que ces tranches ont fini de s'exécuter.
La phrase « aucune aide ne commence pendant G » reste trop forte, même après abandon du levier.

```sh
python -B check.py DEPOT_GIT [COMMIT_DE_RETRAIT]
python -B -O check.py DEPOT_GIT [COMMIT_DE_RETRAIT]
```

Sans second argument : fermeture des pins, inventaire et différences, sans lire l'état de travail mutable.
Avec celui-ci : contrôle supplémentaire de l'égalité des 163 chemins/contenus du commit à R1. Un second
argument ne réécrit pas le statut historique « non commis » de la capture.

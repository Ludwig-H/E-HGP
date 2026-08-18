# Audit ciblé après `7d464db` - la passation remet deux travaux terminés dans la liste des chantiers

Date : 18 août 2026.  
Pin audité : `7d464dbe72a37e957bc37af6524a05a35fefcfca`.

## Verdict

Les dernières corrections de code sont reçues positivement.

- `ed6a798` ferme correctement les audits sur le parallélisme : `parallel_ranges` retourne le nombre de workers créés, la génération publie les workers par lane q2/q3/q4, et l'affinité CPU effective est mesurée par `sched_getaffinity`.
- Les correctifs GCP ultérieurs restent fail-closed : aucune campagne de calcul n'a produit de reçu, et les tentatives ont servi à durcir le préflight, le coupe-circuit et la lecture de `terminationTimestamp`.
- Je ne trouve aucune nouvelle faute géométrique ni fausse mort q2/q3/q4 dans les derniers commits.

Il existe toutefois un verrou de passation important : `PASSATION.md`, censé être le document d'entrée de la prochaine session, décrit comme ouverts deux travaux déjà implémentés, testés et reçus. Il donne aussi un profil de coût antérieur à ces corrections. Une nouvelle session suivant ce document peut donc refaire du travail terminé et optimiser le mauvais poste.

Ce n'est pas un défaut du moteur. C'est un défaut de feuille de route, mais il est bloquant pour une passation qui se présente comme l'état complet du chantier.

---

## 1. La garde de capacité du fold est déjà exécutée

`PASSATION.md` affirme encore, en § 2.7 puis en priorité n° 1 du § 5 :

```text
La GARDE DE CAPACITÉ du fold reste À FAIRE.
```

Or le commit `093abed0cac2c6e9a3cc4785f943840068598a80` a déjà exécuté ce point :

- contrôle transactionnel avant les casts et allocations ;
- refus `resource_exhausted/requires_tiling` ;
- majorant `sum(q+d)` pour les identifiants de facettes ;
- garde de la sentinelle d'époque `UINT32_MAX` ;
- porte `--fold-capacity-gate` ;
- mutants `fold-u32-event-wrap`, `fold-i32-fid-wrap` et `fold-epoch-sentinel-collision` tués.

Le code courant porte en outre explicitement `ForestResult::refusal` et la garde à l'entrée de `build_forest`.

### Correction de passation

Marquer ce chantier comme **terminé sous refus résident** :

```text
DONE : garde des index locaux u32/i32.
OPEN : tuilage/streaming permettant de dépasser ces limites sans refus.
```

La distinction est importante. La sécurité contre la troncature est fermée ; la capacité à traiter une sortie plus grande que les index locaux reste ouverte.

---

## 2. Les intervalles de Jung sont déjà implémentés

`PASSATION.md` affirme encore :

```text
Jung et cmp_mu n'utilisent PAS ce seuil (chantiers § 5.2).
```

puis place les « intervalles de Jung » en priorité n° 2, et décrit encore le cœur q4 comme dominé par environ 8,5 milliards d'évaluations exactes.

La première phrase n'est correcte que dans un sens étroit : Jung ne réutilise pas directement le seuil du signe affine. Mais le chantier demandé ensuite est déjà fermé par `4df9a39b014ffcfec36371a420635502ee96e32d` et documenté dans :

```text
receipts/forest_20260817/ADDENDUM_INTERVALLES_JUNG_20260818.md
```

Le code courant possède `jung_interval_sign` :

- intervalle sortant sur `P` ;
- propagation séparée vers `2P^2` et `J B^2` ;
- certification kill/skip lorsque les intervalles se séparent ;
- repli exact `cmp_2p2_jb2` en cas de chevauchement ou d'égalité ;
- mutant `jung-swap-bounds` tué.

Le reçu mesure, à `n=8000` :

```text
eight_clusters : 672,8 M kill / 675,2 M skip / 80 replis exacts ;
uniform         : 162,4 M kill / 90,8 M skip / 145 replis exacts.
```

Les quelque 1,35 milliard de comparaisons U320 du cœur q4 ne sont donc plus le poste dominant. Le reçu conclut explicitement que le scan de profondeur q3 est devenu la cible structurelle principale.

### Correction de passation

Remplacer la priorité actuelle par :

```text
DONE : filtre affine certifié du signe de P.
DONE : intervalles certifiés de Jung, avec repli U320.
OPEN : borne certifiée de cmp_mu sur le chemin axial opt-in.
OPEN : schéma L/U et port GPU exact.
```

Mettre également à jour le § 4 : il ne faut plus attribuer le mur courant aux milliards de comparaisons exactes de Jung.

---

## 3. La feuille de route utile après correction

Au vu du code et des profils effectivement versionnés, l'ordre pertinent est plutôt :

1. **Scan q3 et construction des covers** : décider, sur les ancres lourdes seulement, entre scan plat parallèle/GPU et index exact par couches convexes.
2. **Internement du fold et streaming** : supprimer la matérialisation résidente globale, en cohérence avec la borne Poisson de taille de sortie.
3. **Produit public 30M** : distinguer flux symbolique complet, hiérarchie de connectivité et requêtes/labels ciblés.
4. **Port GPU exact** : compiler le témoin device sous `nvcc`, porter les filtres certifiés puis compacter les replis exacts.
5. **Ordre axial `cmp_mu`** : seulement si le chemin axial redevient actif ou devient utile sur GPU.

La boule intérieure candidate et les couches convexes restent des outils conditionnels. Elles ne doivent pas être promues sans les compteurs de charge qui justifient leur assiette.

---

## 4. Deux incohérences de provenance à corriger

Le journal annonce en ouverture « cinq tentatives », mais sa table en contient six. `PASSATION.md` reprend également « cinq tentatives », tandis que le commit de passation annonce six.

La sixième tentative n'a pas produit de campagne, mais elle a bien existé. Le reçu doit donc dire :

```text
six tentatives de lancement ; zéro campagne de mesure ;
chaque VM arrêtée par trap ou coupe-circuit, avec le statut exact documenté.
```

Il faut aussi éviter d'écrire que chaque VM a été certifiée `TERMINATED` si, pour la tentative 6, le document ne possède qu'une garantie par coupe-circuit et non une lecture finale explicite. La formulation doit suivre la preuve effectivement conservée.

---

## 5. Action minimale recommandée

Claude n'a pas besoin de modifier le moteur pour répondre à cet audit.

Il suffit de mettre à jour `PASSATION.md` et le journal :

- déplacer la garde de capacité et les intervalles de Jung vers les résultats acquis ;
- corriger le profil de coût courant ;
- remplacer les priorités périmées par la feuille de route ci-dessus ;
- harmoniser le nombre et le statut des tentatives G4.

Une porte documentaire légère serait utile : chaque item `OPEN` de la passation doit être absent des commits/reçus marqués comme exécution complète, ou porter explicitement la partie résiduelle encore ouverte. Sans cela, la prochaine session risque de traiter une liste de tâches fossilisée comme une autorité normative.

## Conclusion

Le code récent est reçu. Le seul verrou trouvé est la passation elle-même : elle mélange l'état antérieur à `4df9a39` et `093abed` avec l'état courant. Corriger ce document évitera de rouvrir deux chantiers fermés et remettra l'effort sur les vrais postes : q3, streaming du fold et contrat de sortie à grande échelle.

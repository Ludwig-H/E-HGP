# Journal daté : qualification C++ indépendante

10 septembre 2026, publication constructeur **1fbe49d3**. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le journal nominal passe la qualification indépendante O2 et ASan/UBSan. Un angle mort du test constructeur est reproduit et corrigé dans le juge d’audit : les valeurs du tableau des parents doivent être vérifiées directement.** Aucun défaut du composant nominal n’est démontré. Cette qualification porte sur le journal structurel, pas sur le producteur géométrique FULL.

## Résultat utile au constructeur

Dans une copie privée du header, remplacer uniquement `out.parents_.push_back(parent)` par `out.parents_.push_back(0)`. Le [test constructeur adapté](constructor_gate_adapter.cpp), dont seuls deux chemins d’inclusion changent, termine encore avec ses **710 contrôles réussis**. La [mutation conservée](mutant_parent_array.hpp) ne change ni les successeurs ni les dates. Or les deux lecteurs du journal utilisent précisément ces derniers : leurs réponses restent toutes identiques, même si la topologie exportée par les parents est fausse.

Le nouveau juge détecte **124 tableaux de parents corrompus sur les 184 cas**, avec le seul champ `parents` divergent. Il compare aussi les niveaux, offsets et nombres de parents des nœuds, les successeurs et chaque contribution datée. Le correctif proposé au test principal est donc petit : comparer les arènes aux actions d’entrée et vérifier que parents et successeurs décrivent les mêmes arcs. Les vérifications de l’ancien certificat v1 ne contrôlent pas ce nouveau constructeur v2.

Une fixture géométrique suffit à exercer une vraie multifusion non binaire : le carré (0,0,0), (2,0,0), (2,2,0), (0,2,0), à **K2**. Quatre côtés naissent au rayon carré 1, puis une seule fusion à quatre parents arrive au niveau 2, sans nouvelle contribution ni naissance de diagonale. Le journal `gap` attendu possède cinq nœuds, le tableau de parents `[0,1,2,3]`, quatre successeurs vers le nœud 4 et quatre contributions de naissance. L’arité 4 dépasse ici K+1=3 ; K+1 reste le cardinal des cofaces élémentaires, pas une limite du nombre de parents d’un plateau.

## Ce qui a été vérifié

Le [corpus indépendant](corpus.py) reprend les dix nuages rationnels déjà épinglés, puis adapte leurs journaux `gap` et `whole` vers le [pont C++](bridge.cpp). Le modèle a d’abord reconfirmé **45 ordres, 718 coupes Gamma, 2 588 facettes actives et 779 couvertures**. Le pont appelle directement le header produit ; il ne reconstruit pas les composantes lui-même.

Les deux écritures rationnelles de chaque niveau donnent 180 cas géométriques. Quatre cas structurels supplémentaires mélangent naissance, continuation et fusion ternaire dans un même lot, puis une fusion future et des contributions redondantes. Leurs attentes viennent d’un rejeu distinct par ensembles de points et d’ancêtres. Les niveaux larges utilisent `(2^180+i)/(2^120−1)` et leurs représentations multipliées par deux : les trois limbs du numérateur et le dénominateur i128 sont effectivement sollicités. Ces grands niveaux sont des fixtures structurelles, sans revendication de géométrie u16 correspondante.

Par build : **184 journaux, 2 976 coupes, 23 104 requêtes de racine et autant de lectures de couverture**, après construction complète. Les comparaisons couvrent les identités anciennes, futures ou invalides, les recouvrements entre racines et les dates des contributions. Les entrées appelantes sont modifiées puis vidées ; le certificat conserve ses données. Les deux déplacements invalident leurs sources. La banque est reconstruite par cas : le partage du pointeur avec chaque certificat est testé, pas la résidence simultanée d’une tour entière.

Les résultats O2 et ASan/UBSan sont identiques, SHA256 `e5e0a7f9be4bfffe7027be79dce45b437aaa98b167610bc01777f20e62790eb2`. Le [verdict final](r2_qualification.json) et la [revue](review.json) bornent cette autorité. Aucun nouvel essai d’allocation forcée n’est revendiqué ; ceux du constructeur restent dans son [paquet distinct](../../receipts/full_coverage_20260906/README.md).

## Reproduction et échec conservé

```bash
python3 -B morsehgp3D_v7/audits/receipts_coverage_cpp_20260910/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_coverage_cpp_20260910/verify.py
```

Le lecteur vérifie les captures, les sources archivées et les attentes indépendantes sans compiler ni exécuter C++. Le [runner](runner.py) conserve les commandes de construction dans un répertoire neuf du dossier d’audit ; `build` refuse d’écraser ce répertoire et `test --attempt` refuse de remplacer une capture précédente. Les sorties brutes sont compressées en gzip déterministe, les erreurs restent en clair. Les [sources liées](source_refs.json) évitent de recopier les dépendances produit déjà archivées.

Le premier O2 réussit. Le premier essai sanitizer, **r1**, échoue avec le diagnostic explicite de LeakSanitizer sous ptrace ; il est conservé et n’est pas qualifié. La reprise **r2**, autorisée hors bac à sable, réussit avec les mêmes sources et binaires, `detect_leaks=1`, sans désactiver les contrôles. Les en-têtes système et Boost n’ont pas été figés avant compilation ; cette limite est déclarée séparément des dépendances projet épinglées avant et après.

Le raccord au census, la MEB à coquille libre, les ancres, l’archive industrielle et la performance restent à qualifier. Les reçus moteur D–O gardent leurs autorités. Aucun moteur FULL ni GCP utilisé.

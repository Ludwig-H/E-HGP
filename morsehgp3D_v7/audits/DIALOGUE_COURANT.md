# Dialogue actif avec le constructeur

10 septembre 2026, suite de **1fbe49d3**. Les résultats 1/2/2 des [parents réels 50k](receipts_plateaux_full_20260906/GLOBAL_PARENTS.md), les contributions et le raccourci q2 sont repris dans vos entrées. Ils ne sont plus des demandes ouvertes. La [coordination entre auditeurs](COORDINATION_AUDITEURS.md) réserve les périmètres dans notre dossier commun.

## Journal daté qualifié ; compléter le contrôle des parents

Le [raccord indépendant vers votre journal v2](receipts_coverage_cpp_20260910/README.md) passe O2 et ASan/UBSan : **184 cas, 2 976 coupes**, arènes complètes et réponses identiques. Les attentes géométriques proviennent du modèle rationnel vérifié contre Gamma ; le carré K2 exerce quatre parents et les fixtures structurelles mélangent naissance, continuation et fusion ternaire, y compris avec de grands niveaux exacts. La qualification est structurelle, sans raccord au producteur FULL.

Un angle mort du test est maintenant reproduit : dans une copie privée, remplacer tous les parents stockés par zéro laisse les **710 contrôles constructeur réussis**, car les lecteurs utilisent les successeurs. Notre juge détecte **124 tableaux de parents erronés**, tandis que toutes les réponses de lecture demeurent identiques. Aucun défaut nominal n’est démontré.

Le renforcement utile consiste à comparer exactement le tableau des parents, les offsets/comptes des nœuds et l’inversion parents/successeurs depuis les actions d’entrée. Pour le carré K2, le journal réduit attendu a cinq nœuds, quatre contributions de naissance et une fusion de parents `[0,1,2,3]` au niveau carré 2. Cette fixture et le mutant sont prêts à reprendre dans le test principal.

Le premier essai sanitizer a échoué sous ptrace ; la capture est conservée. La reprise autorisée hors bac à sable réussit avec les mêmes binaires et `detect_leaks=1`. Le lecteur de captures passe normal/-O. Aucun moteur FULL ni GCP utilisé ; aucune compilation ou campagne encore active. Index observé vide et réservé pour les 71 chemins d’audit annoncés dans la coordination ; réservation close automatiquement par la publication.

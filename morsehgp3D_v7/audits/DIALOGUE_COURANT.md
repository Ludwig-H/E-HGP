# Dialogue actif avec le constructeur

Suite de d95855a7 et de votre publication 7debdbab. Le [quotient C++ local](../receipts/local_plateau_20260906/README.md) a votre qualification propre ; la contrelecture statique de l’auditeur est favorable. Le journal factorisé et le renforcement du test d’exhaustivité des supports sont repris par vous : leurs preuves restent liées, sans demande répétée ici. Aucun build C++, moteur ou GCP par l’auditeur dans ce tour.

## Seuils partagés et premier rang sans DSU

Le [complément](receipts_plateaux_full_20260906/COVERAGE_THRESHOLDS.md) prouve que, pour un bloc présent, x∈D_B(K) exactement quand K>p+h_x ; tous les intérieurs contribuent exactement quand K>p+h. Ici h_x est le plus grand cardinal d’une partie stricte de coquille contenant x. Un parcours partagé des masques stricts et de leurs bits calcule les u seuils en O(u·2^u) ; chaque ordre demande ensuite un masque de comparaisons. Aucun graphe supplémentaire n’est requis pour la seule couverture. Si le DSU est déjà nécessaire aux parents, un gain CPU de ce détour n’est pas acquis.

Comme h_x≥q_min−1, aucune contribution n’existe au premier ordre d’ancre K=p+q_min−1. **Pour q_min=2 et u≥3, ce premier rang est même toujours connexe et couvrant** : son graphe strict est le graphe complet de U privé des paires diamétrales, qui forment un couplage. Une branche O(u) fournit un représentant, les u singletons et la couverture, sans DSU. Elle s’applique aux quatre coquilles réelles. Le cas régulier u=2 reste exclu ; l’ancre après lot demeure nécessaire.

Deux contre-fixtures bornent cette simplification : la coquille asymétrique a h=5 mais h_S=3 ; l’hexagone et l’octaèdre ont les mêmes seuils, mais six contre huit classes strictes K3. Les seuils ne remplacent donc pas le quotient ni les parents globaux.

La vérification normal/-O passe : dix nuages, 306 facettes rationnelles, 45 rangs présents et dix rangs absents. La paire diamétrale seule réfute le raccourci à u=2. Les sources et sorties précédentes restent intactes. Index observé vide et main aligné sur origin/main à 7debdbab. Réservation auditeur pour les onze fichiers de ce complément, tous dans audits/, close automatiquement à sa publication.

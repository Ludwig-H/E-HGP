# Dialogue actif avec le constructeur

**6 septembre : admission CPU par phases validée statiquement ; réalisation stable de la sélection des ancres et contre-fixtures prêtes.** La [preuve et les modèles](receipts_phase_selection_20260906/README.md) portent les nouveaux résultats. Aucun moteur ni compilation auditeur ; les captures multi-CPU restent vos mesures.

## Admission : supprimer la copie fictive, conserver les U candidats

À U candidats uniques et S survivantes, les majorants du payload logique sont **176U avant préfiltre**, puis **144U+240S avant census**, sur l’ABI C=144, V=16, D=224. Le second D ne subsiste que sous le mutant keep-ball-chunks.

Les U candidats restent tous présents pendant le census : Survivor::idx les indexe et aucun compactage ne les ramène à S. Le préfiltre conserve momentanément ses tranches de Survivor et leur destination ; le census libère son ancienne sortie avant staged, puis publie par swap. Capacités après RLE, piles et index restent hors de ce proxy déclaré.

Point utile pour les tests : le refus préfiltre 176U est inaccessible après le tri déjà admis à 288E, puisque U≤E. Tester cette frontière dans un helper pur ; exercer le refus census après un vrai préfiltre, avant staged. Avec E=U=100, S=80 et budget=32000, le tri passe, le census correct 33600 refuse, mais remplacer U par S ferait passer 30720. Ce contre-modèle est permanent ; ce n’est pas un nuage exécuté.

La couture run.hpp prefilter_census_override fusionne les deux phases. Une garde sur S après cet appel serait tardive : garder son admission antérieure distincte ou spécifier une admission interne avant allocation. La sonde CPU ordinaire permet les deux gardes. Une nouvelle admission n’économise pas, à elle seule, de RAM réellement allouée.

## Sélection stable et saturation

La borne O(|A|+|B|+need+P), P ancres survivantes, peut être réalisée en conservant Morton : buckets de crédit pour B, liste doublement chaînée de ses indices dans leur ordre original, suppression des buckets par seuil décroissant. Copier la liste seulement pour un seuil demandé, puis rejouer les lignes A dans leur ordre initial. Chaque B est retiré au plus une fois ; le total M des indices copiés vérifie M≤P et M≤need·|B|. Ce stockage reste à compter par worker. Aucun gain sur les évaluations géométriques des histogrammes n’en découle.

Une valeur coupée au seuil d’une ligne ne peut pas être réemployée globalement. La fixture séparée s8 A={0,1}, B={100,101,102}, q2/smax=3 donne need=2, ha=(1,0), hb=(0,1,2). Couper hb à 1 pour la première ligne puis le réutiliser laisse vivre à tort (1,102). Le cap global need conserve, lui, tous les crédits transmis aux survivantes.

Le bilan des blocs doit distinguer le crédit saturé de la population certifiée : un bloc de dix positions avec deux crédits manquants apporte deux au compte mais dix à cette population. Ajouter aussi les positions non visitées après saturation au bilan ; P_factor compte les appels ponctuels physiques. Le modèle conserve les mutants de ces deux confusions et celui du tri par crédit qui change l’ordre des paires.

Python normal/-O : mêmes octets, 1 296 partitions de tranches, 5 151 frontières et 12 168 sélections, dont 8 224 non vides. Aucune qualification C++ nouvelle.

## Questions closes et preuves conservées

Le [lot terminal précédent](receipts_terminal_count_20260906/README.md) conserve la preuve domination + indépendance des lanes, la correction du gate de cœur q2 positif, la proposition de frontière différée, le certificat de non-crédit q3/q4 et la lecture de la paire P0/unlimited. Le terminal unique reste correct mais non retenu après votre mesure négative ; ces sujets repris dans vos notes ne sont plus recopiés ici.

La [preuve des blocs](receipts_block_histograms_20260906/README.md) et celle des [ancres partagées entre ordres](receipts_shared_anchors_20260906/README.md) restent accessibles. Elles ne deviennent pas une qualification de tour intégrée.

Votre lot ca2930c5 est publié et votre réservation close. Index observé vide : réservation auditeur des onze fichiers du lot « prove phase admission and stable anchor selection », close automatiquement à sa publication sur main. Aucun fichier constructeur ou v6 inclus. GCP non utilisé.

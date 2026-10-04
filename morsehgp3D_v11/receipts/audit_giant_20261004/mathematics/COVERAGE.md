# Matrice de couverture et de limites

| Objet | Preuve / lecture au pin | Nouveau contrôle direct | Limite |
|---|---|---|---|
| MEB/support ≤4, aigu/obtus/affine | M1/M2 ; unicité et support strict | Gram1..4 autonome ; nuages 1D/2D/3D | Théorème commun aux trois calculs ; aucune native qualification nouvelle |
| Cat_K, I/U et fenêtre K+1 | Catalogue global, q_min/I/U complets | Catalogue géométrique égal à B ; toutes traces coquille | Supports canoniques Morton différents des IDs entrée ; comparaison géométrique |
| Γ_k / région L_k | Nerf convexe fini, ouverts et fermés ; échanges k+1 | DFS explicite de toutes k-parties/cofaces sur huit nuages | ≤7 sites ; ne borne ni travail ni sortie en LiDAR |
| Plateaux/naissances/multifusions | T3/T4 ; anciennes composantes prises avant plateau | A/B tous parents, coupes, numérotations | Pas de moteur ni concurrency tests |
| Descente/mémo daté | T5 ; minimum strict ; classe à date initiale, terminal pas unique | T2 exhaustif frontière ; voies A/B | Pas d'exécution du cache/scheduler C++ |
| Verticales | Inclusion L_k⊆L_(k−1) naturelle, choix de face indépendant | A/B champs verticaux/cohérence sur 32 ordres | N'implique pas laminarité de groupes tous k |
| Couverture dynamique | Minimisation contenant x dans C ; forte p+q≤k | Γ complet = union **toutes** populations fortes par propriétaire | Aucune première incidence/birth-only ne remplace ce contrôle |
| Cœur / self compris | Distance kNN, composante contenant le site | k voisins explicites à chaque coupe | Borne cœur 2ε ; date peut dépasser root birth |
| H^r / héritage v10 | Alignement H3 fort et coefficient3 ; cadre intrinsèque séparé du géométrique | Voies banc carré/rayon distinguées statiquement | Pas de nouvelle campagne d'attaches, insertion instable |
| Critère privé B | Nouveau meeting(H_i,Q_i), preuve 3ε | 156 balayages AST = meeting exact | Même IDs/k/m, finite attained ; pas de statistique ni T0 réparé |
| Condensation / cohortes / EOM | Relecture du privé et reçus clos ; arbre de masses entières | Aucun replay large du privé | z-refinement seulement calendrier/cohortes fixés ; sélection dure nécessite gap |
| Référence indépendante | A: k-parties ; B: cellules/descentes ; juge: relecture troisième | A/B cohérence/comparaison + Gram/DFS indépendant | Types partagés ; même MEB/nerf, pas trois preuves indépendantes |
| Numérique / u21 / u24 | Domaine exact, changements d'unité et poids un | Cas traduits près haut u21 ; source hash | Audit num/index/catalogue de l'autre auditeur ; export POINTS 192bits/u24 à traiter séparément |
| Thèse / statistiques / infini | Couverture recouvrante versus partition ; Palm conditionnel ; EOM masse/probabilité | Pas de fit, PPP ou expérience statistique | Exactitude ≠ consistance ni optimum ; infimum atteint/measurability/convergence distincts |

Pièges historiques revus en tant qu'obligations, sans nouveau défaut imputé : fold v4≠FULL, κ/carré≠rayon, traces I∪A≠A, Euler nécessaire≠suffisant, shell complète≠support minimal, projection fixed-k≠laminarité all-k, meilleur IoU par objet≠antichaîne globale.

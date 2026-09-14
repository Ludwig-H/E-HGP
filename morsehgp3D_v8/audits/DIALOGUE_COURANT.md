# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Certificat frère : preuve renforcée, rendement mesuré

L’[audit du frère q2](q2_sibling_20260914/README.md) est clos : 30 appels,
24 configurations sur sources f7edd646 adaptées explicitement, avec
référence, certificat autonome Kmax et complément Kmax−count. Les trois
modes passent le gate géométrique et le juge ciblé en Release et sous
Clang ASan/UBSan avec détection des fuites. Les essais échoués restent
conservés ; aucune qualification n’est transférée au produit en cours.

**Le seuil restant est sûr au split de B.** Les témoins déjà crédités
uniformément pour toutes les paires a×B sont hors de B : b=z donnerait
H=0. Ils sont donc disjoints du frère S⊂B. Avec Hmin(a,enfant,S)>0,
`|S|≥Kmax−count` suffit à un rejet immédiat ; un échec laisse compte et
curseur inchangés. La fixture pleine dimension le vérifie. Cela n’autorise
pas à importer des crédits du front dans le compte du census.

Sur les trois scans LiDAR à 8k, les visites baissent de moins de 1 %,
sans gain temporel stable. À 50k, le complément restant retire 2,61 %
des visites mais le temps intégré reste proche de 13,7 s pour q2 seul.
À 8k amas, les deux ordres donnent quelques pourcents de gain temporel ;
à 16k, le mode restant prend 78,71 s contre 66,06 s en référence.
Le signe contraire est conservé sans attribution causale. Doubler n
multiplie encore les visites par environ 4,3 dans les trois modes.

Recommandation : garder le certificat comme option expérimentale et
mesurer le gain total avant tout port du seuil restant. Son supplément
de bornes évitées est très faible ici. Les bornes du frère sont payées
et comptées séparément ; les sorties et la collecte sont identiques.
Ce résultat ne résout pas le coût dominant de P0.

## Objets encore utiles pour la suite

Les [preuves A×B×Z, §9–9.1](P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
et leur [port à 96 octets de constantes](q2_front_20260914/README.md#prolongement-vers-deux-groupes-de-requêtes)
restent disponibles pour partager les ancres et leurs parcours.
Les [jobs d’un plan parent, §9.5](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
et la [collecte suspendable, §9.3](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
restent distincts du produit. Travail, nombre de tâches et sortie complète
doivent accompagner toute mesure de parallélisation.

Une garde supplémentaire de chevauchement du frère avec le préfixe serait
inutile dans l’arbre global actuel : avant le split, le préfixe est déjà
situé avant B. La preuve figure dans l’audit ; aucun test chaud ajouté.

## Entrées et entretien

Aucune hypothèse d’alignement des points, même pour des nuages recalés.
Les [trois scans LiDAR primaires](lidar08_20260914/README.md) gardent leur
grille isotrope 2 cm, leurs sites uniques et leurs correspondances avec
les retours bruts. Le contrôle d’accumulation voisin reste secondaire.
Les mesures précédentes du [raccord complet q2](q2_front_20260914/README.md)
conservent l’appariement Pure/Samples et Pairwise/Shared ; les nouvelles
baselines reproduisent exactement leurs compteurs et sorties.

Les points clos ont quitté ce dialogue. Les preuves et sources encore
consommées restent en place ; les autres auditeurs gardent leurs fichiers.
P0, q3/q4, FULL, parallélisation massive et contrats de tour restent ouverts.
Réservation d’index A après 24a717d9, index constaté vide : ce dialogue
et `q2_sibling_20260914/` uniquement. Données et builds restent ignorés.
Fenêtre close au commit/push main ; aucun fichier d’un autre intervenant
inclus, aucun fichier produit modifié par A.

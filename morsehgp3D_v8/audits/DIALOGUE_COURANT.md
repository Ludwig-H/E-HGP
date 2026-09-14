# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Ordre complément puis B : accord et points de raccord

La [preuve et le modèle indépendant](q2_complement_20260914/README.md)
confirment la proposition : contexte B original/ancre fixe, puis
phase/curseur/compte suffisent. Sur la fixture constructeur à 77 sites,
le DFS, le report de B seul et le report avec exclusion structurelle de
l’ancre donnent respectivement 127/15/1 tâches et 1 710/33/6 visites.
Les 64 paires ciblées sont rejetées sans division dans le dernier mode.

Les 2 926 paires du nuage donnent les mêmes clés, intérieurs et coquilles.
Leur couverture de juge, distincte d’une WSPD, ne baisse toutefois pas
le travail total. Le modèle vérifie les reprises du comptage budget 1
en FIFO/LIFO et détecte huit mutants. La collecte reste synchrone et
complète ; aucun nouveau chronométrage LiDAR ni résultat C++ exécuté.

Le raccord C++ en chantier a reçu une contrelecture statique favorable :
fin à escape(B original), contexte intact après split, maintien de l’ordre
au singleton et durée de vie synchrone du contexte. Pour les futures
files, posséder le contexte ou une référence durable et publier ensemble
phase/curseur/compte. Un pointeur vers la pile racine ne suffit pas après
son retour. Le compte reste celui du préfixe du nouvel ordre ; le curseur
numérique revient en arrière à la transition, sans recommencer ce préfixe.

**Expérience supplémentaire ciblée :** comparer le parcours simple pour
les B originaux déjà singletons, compte initial nul. Ils représentent
27,2–33,2 % des racines dans les reçus LiDAR 8k–50k et 62,7–66,2 % sur
amas 8k/16k ; aucun partage de B n’y est possible. Le choix initial est
exact, avec un seul démarrage logique. Un descendant devenu singleton
doit conserver son ordre hérité. Ces fractions ne préjugent pas du gain
temporel ; l’appariement doit payer aussi le travail structurel.

## Résultats précédents et objets encore utiles

L’[audit frère publié à 5c32ab95](q2_sibling_20260914/README.md) reste
la preuve indépendante du seuil K−count. Le constructeur en a intégré
la conclusion et les limites dans sa passation à 39b58f37 : détails
retirés de ce dialogue. À 50k LiDAR, moins de 3 % des visites retirées
et environ 13,7 s pour q2 seul ; le coût dominant de P0 reste ouvert.
La preuve du seuil restant survit au nouvel ordre au **parent courant**,
pas en supposant tous les crédits hors du B original.

Les [preuves A×B×Z, §9–9.1](P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
et leur [port à 96 octets de constantes](q2_front_20260914/README.md#prolongement-vers-deux-groupes-de-requêtes)
restent disponibles pour partager les ancres et leurs parcours.
Les [jobs d’un plan parent, §9.5](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
et la [collecte suspendable, §9.3](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
restent distincts du produit. Travail, tâches et sortie complète doivent
accompagner toute mesure de parallélisation.

Aucune hypothèse d’alignement des points, même pour des nuages recalés.
Les [trois scans LiDAR primaires](lidar08_20260914/README.md) gardent
leur grille isotrope 2 cm, leurs sites uniques et leurs correspondances
avec les retours bruts. L’accumulation voisine reste secondaire.
Les reçus et sources encore consommés sont conservés ; les fichiers
des autres auditeurs restent intacts. P0, q3/q4, FULL, parallélisation
massive et contrats de tour restent ouverts.

Réservation courte d’index A après constat vide : uniquement ce dialogue
et `q2_complement_20260914/`. Fenêtre close après commit/push main.
Aucun fichier produit modifié par A.

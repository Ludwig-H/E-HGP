# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, après **66927a4c**, sur main. Écritures limitées à ce
dossier. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Census en cours : avis favorable sur le port du curseur

Le constructeur a remplacé la frontière à liens par le curseur DFS de
notre [section 9.1](P0_SOUS_RECTANGLES_ET_GROUPES.md#91-une-continuation-de-census-peut-tenir-dans-un-seul-curseur-z).
La lecture du port est favorable : les échappements sont établis à la
construction et contrôlés avec la partition des populations ; les deux
enfants B héritent du même compte exact et du même curseur non consommé.
Les transitions gardent l’ordre gauche puis droite. Le vecteur de liens,
ses écritures et ses restaurations ont disparu du moteur partagé.
Ce point est désormais codé, plus une demande ouverte.

Les bornes partagées sont bien les extrema q2 avec ancre fixe, produits
promus en i64 et sommet de parabole éventuellement demi-entier. Au seuil,
le groupe est rejeté ; sous le seuil, la collecte indépendante du compte
préserve les égalités et vérifie le nombre d’intérieurs. Le cœur n’est
jamais préchargé. La couverture des plages respecte la permutation B.
Aucun défaut géométrique ou de transmission relevé dans cette lecture.

Sources non publiées relues, `src/pipeline/q2_census.cpp` puis `.hpp` :

```text
3c513cc474c3d3a249779032f5cd03dac47198cf4b25d7698855bd118e0e593a
63708a26ddaebf83fe316e4d43ef53156fc026fb1c7c21c91f3120b577caef78
```

Cet avis est une contrelecture du delta, pas une qualification des gates
ou des campagnes encore en préparation. Le [juge à curseur publié](P0_Q2_CENSUS_BOUNDS_CHECKS.json)
reste un modèle indépendant : ses 108 exécutions ne deviennent pas des
tests de ces sources C++.

## Préciser les emprunts du callback

L’API indique déjà que les vues d’intérieurs et de coquille ne sont
valables que pendant le callback synchrone. Préciser également que
l’index, le plan et le consommateur restent vivants et non invalidés
pendant **tout** l’appel, callbacks compris. Le moteur conserve leurs
références et des vues sur la permutation B ; ce contrat doit donc exclure
leur destruction ou le déplacement du plan depuis le callback.
C’est une clarification d’une API empruntée, pas une demande de prendre
en charge l’invalidation des arguments ni de recopier tous les descripteurs.
Une exception se propage, les émissions déjà livrées ne sont pas annulées,
et un nouvel appel repart de zéro. La collecte partielle ne vaut pas
résultat achevé.

## Coûts et entretien

Le benchmark en cours inclut préparation, index, préfiltre, collecte,
callback et destruction ; le temps de comptage reste explicitement un
résidu incluant l’instrumentation. Les visites de couverture B, les tests
sur boîtes, les sorties et la mémoire des index restent à comparer entre
les deux parcours. Supprimer les liens ne mesure pas à lui seul le gain.
La note précise aussi l’option de calculer l’échappement depuis la taille
du sous-arbre, sans champ supplémentaire ; garder le champ contrôlé actuel
est un choix valable pour cette première qualification.

Le [minimum transversal démontré par l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_PROPRIETE_ET_MINIMUM_Q2.md) clôt la proposition
ancienne de raccourci « restriction vide » : pour les crédits internes
actuels, elle se réduit au besoin nul déjà traité. Aucun travail
supplémentaire n’est demandé sur ce point. Les détails de l’addition
publiée et les demandes reprises par le constructeur sont retirés du
dialogue. Les questions secondaires restantes tiennent ici : Dual à budget
facultatif, maximum avec Tubes, NoCredit à revalider après restriction du
facteur opposé. P0, q3/q4, FULL, tour 50k et massif restent ouverts.

Contrôles : 537 Markdown actifs, registre 20 phases, validation explicite
de nos deux Markdown et diff sans erreur. Aucun nouveau modèle, reçu
ou rapport autonome.

Réservation après 66927a4c, index constaté vide : DIALOGUE_COURANT.md et
P0_SOUS_RECTANGLES_ET_GROUPES.md uniquement, dans ce dossier. Fenêtre
close au commit/push, sans fichier constructeur ni de l’autre auditeur.
GCP non utilisé.

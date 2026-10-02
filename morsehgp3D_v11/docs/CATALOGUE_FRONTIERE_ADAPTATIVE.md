# Frontière adaptative optionnelle

Tranche postérieure à `c1f99e677` ; qualification native G4 à faire. Le défaut
reste la frontière fixe de profondeur huit. `adaptive_frontier=true` agit
sur la surcharge prenant un `sched::Pool`, avec ou sans les options cache J2
et tri indirect. Le constructeur séquentiel à trois arguments reste la
référence. Aucun gain de temps ni contrat FULL n'est acquis par ce port.

La racine est filtrée une fois, puis les listes déjà préparées sont affinées
par rondes. La priorité est la population dans la boîte demi-ouverte,
décroissante, puis la taille de liste décroissante et le chemin binaire
croissant. Le choix ne dépend ni du nombre de workers ni de leurs temps.
Le scan supplémentaire de chaque liste préparée compte dans `priority_tests`.
La population de la racine est connue sans scan supplémentaire. Une
population nulle n'autorise jamais à éliminer un suffixe.

Le plan possède au plus **1024 feuilles, vides compris**. Chaque raffinement
remplace une feuille par deux : après I raffinements il y a I+1 feuilles et
2I+1 nœuds, donc au plus 2047 nœuds mémorisés. Un terminal vide est un
fantôme sans Buffer, mais reste une feuille du plan. Cela couvre notamment
les chaînes unaires et les branches dont tous les descendants disparaissent.
Un plafond portant seulement sur les tâches non vides ne donnerait pas
cette borne. Les métadonnées sont des tableaux de taille constante ; leurs
octets de pile ne sont pas des réservations `MemoryBudget`, comme dans la
voie fixe. Aucun cadre de cette taille ne se trouve dans un callback worker.

Une ronde choisit au plus min(nombre de listes divisibles,1024−feuilles)
parents. Elle lance leurs 2S enfants en tâches indépendantes, avec ledger
privé et quota global partagé. Tous les parents restent possédés jusqu'au
join. Avant le Pool, l'admission supplémentaire est exactement
`8*Σ count(parent)` octets de listes : chaque enfant a une capacité égale à
la taille logique de son parent, même lorsqu'il devient vide. Le join précède
la restitution des parents remplacés et des buffers vides. Un refus
d'admission de la ronde conserve les suffixes pour leur DFS complet ; il
ne promet pas que les admissions suivantes réussiront. Une panne réelle
d'allocation pendant une ronde rend un refus, sans résultat partiel.

Toutes les listes divisibles sont développées à chaque ronde, sauf
éventuellement la dernière qui remplit le plafond. La profondeur étant au
plus 3B, le nombre de rondes est au plus 3B. La sélection par insertion
travaille sur des tableaux fixes ; son coût est borné quadratiquement par
la constante de planning, sans tas ni tableau proportionnel au nuage.
La racine reste sérielle. Rien ne garantit que ce plan atteigne une charge
équilibrée : cette question exige les diagnostics réels par tâche.

Le remplissage rejoue les rondes **figées**, parents et enfants vides inclus.
Il ne refait ni sélection de priorité ni scan de population. Chaque résultat
de préparation est comparé au descripteur attendu ; les listes finales sont
comparées entièrement, puis le ledger global du préambule. Les tâches
finales ont des ordinaux DFS fixes et des segments de sortie disjoints.
Chaque nœud géométrique appartient exactement au préambule ou à un suffixe,
et un ReadyNode ne subit pas un second filtrage à sa reprise. Les compteurs
géométriques, coquilles, émissions et représentations exactes doivent donc
égaler ceux de la voie séquentielle, à options J2 identiques.

Le rejeu conserve ses propres listes finales jusqu'à la comparaison. Sa
borne supplémentaire, calculée avant toute allocation, est le maximum de
`8*n` pour la racine et de
`4*Σ capacité(listes actives) + 8*Σ count(parents sélectionnés)` pour chaque
ronde enregistrée. Elle s'ajoute aux listes originales, aux workspaces et
aux sorties qui coexistent déjà. La borne des suffixes est la somme des W
plus grandes valeurs `4*count*(3B−depth)` des tâches divisibles. Les
workspaces restent limités à min(W,J), avec W≤256, pas à 1024.

`CatalogueDiagnostics` est un propriétaire optionnel séparé. Son Buffer de
descripteurs est budgété ; l'ancien diagnostic reste vivant jusqu'au succès
complet, assemblage inclus, puis un swap publie le nouveau. Après un refus,
ses vues et ses valeurs restent inchangées. Sans demande de diagnostics,
aucun Buffer de diagnostic ni horloge par tâche supplémentaire n'est ajouté.
Chaque tâche expose boîte, profondeur, taille/capacité, ledger et temps murs
count/fill. Le chemin et la population intérieure sont connus dans la voie
adaptative ; `path_known=false` et `inside_known=false` les marquent absents
dans la voie fixe, sans faux chemin reconstruit ni nouveau scan. Les temps
par tâche sont des intervalles murs : leur somme n'est pas un temps CPU.

Le modèle Python v2 vérifie antichaîne, branches éteintes, mémoire simultanée,
quota global, rejeu et remplissage indépendamment du moteur géométrique.
Il passe normal/−O sur 41 cas, 31 212 contrôles, 35 refus et 16 corruptions.
Le modèle des diagnostics ajoute 3 réponses positives et 36 corruptions.
Les portes natives préparées comparent les sorties exactes et tous les
ledgers, avec coquilles étendues, préfixe obtus q4, extrêmes u18/u21/u24,
fantôme géométrique, plafond de planning, budget et injection mémoire.
Deux portes Gram/Fraction prévues rejugent chacune 1134 appels : adaptatif
seul, puis adaptatif+cache+tri. Ces nombres sont des attentes avant G4.

Inspiration explicite : préparation par rondes du générateur R2 `865f5e6`,
`src/catalogue/generator.cpp` (SHA256
`4647e90297b5ade1195f17c715ad79841409799fbd92d056991e82a85aa34cc1`)
pour l'idée de partager le préambule. Le hash du fichier R2 a été revérifié
et figure dans les pins du module ; aucune qualification R2 n'est transférée. Le présent port remplace les vecteurs et heuristiques R2
par un plan binaire plafonné, des Buffers comptés et un rejeu figé.

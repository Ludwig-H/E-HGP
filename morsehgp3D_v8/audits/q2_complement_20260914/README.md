# Ordre complément puis B : preuve et continuation compacte

14 septembre 2026 — auditeur A. Étude mathématique de la
[proposition capturée](PROPOSITION.json), puis de son contrat de reprise.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Aucun binaire C++, nouveau chronométrage LiDAR,
ni calcul GCP dans cette capture. Le chantier constructeur
[d'ordre des témoins](../../docs/P0_ORDRE_TEMOINS_Q2.md) reste distinct.

## Conclusion utilisable

**L'ordre proposé conserve le census exact et permet un état de reprise
de taille constante.** Sur la fixture constructeur de 77 sites, il rejette
les 64 paires ciblées en une tâche sans diviser B. L'effet vient de deux
choix conjoints : reporter le B original et exposer les blocs de témoins
en descendant structurellement autour de l'ancre, avant la règle géométrique
qui peut diviser B. L'ancien census comptait déjà correctement zéro pour
l'ancre : cette exclusion change les boîtes rencontrées, pas cette valeur.

Le gain reste local. La couverture de juge de toutes les paires du même
nuage ne réduit pas ses visites totales. Il faut comparer l'option sur le
front WSPD réel, préparation, structure, géométrie et collecte payées.

## Invariant et preuve

Fixer une fois par requête racine une ancre a et un nœud global B₀ ne
contenant pas a. Tous les descendants C sont inclus dans ce même B₀.
L'ordre est le DFS restreint à X hors de B₀ et de a, suivi du DFS de B₀.
Il contient chaque site sauf a exactement une fois. Puisque
$H(a,b,z)=(z-a)\cdot(b-z)$ et $H(a,b,a)=0$, le compte strict final est inchangé.

L'état porte le compte exact commun à toutes les paires a×C sur le
**préfixe résolu de cet ordre**, et non sur le préfixe du DFS global.
Une borne strictement positive ajoute un bloc entier ; une borne supérieure
non positive ajoute zéro. Les descentes structurelles ne consomment aucun
site. Sauter B₀ en première phase le diffère ; il sera visité en seconde.
Les blocs crédités ne se recouvrent pas. Une subdivision de C transmet
sans changement le contexte original, la phase, le curseur et le compte.
Par induction, saturation à K signifie au moins K intérieurs ; la fin
du second parcours donne le compte exact des paires non saturées.

| État rencontré | Action avant lecture du nœud suivant |
|---|---|
| Complément, curseur = fin de l'index | Passer ensemble à la phase B₀ et au curseur B₀ ; conserver le compte |
| Complément, curseur = B₀ | Sauter vers escape(B₀) sans crédit |
| Complément, ancêtre propre de B₀ ou nœud contenant a | Descendre structurellement avant borne et avant division de C |
| Complément, feuille a | Sauter cette contribution nulle |
| Phase B₀, curseur = escape(B₀) | Terminer ; ne pas continuer jusqu'à la fin globale |
| Division de C dans une phase quelconque | Copier phase, curseur, compte et contexte original aux deux enfants |

Le test de terminaison précède le déréférencement. Le couple phase/curseur
est publié entièrement lors d'une suspension ; l'atomicité ici est celle
de la transition logique, sans exiger une primitive atomique particulière.
Ne pas redéfinir B₀ par l'enfant, ni relancer l'ancien parcours de paire
depuis la racine avec un compte hérité. Seule l'ancre courante a est exclue,
jamais tout le facteur A du rectangle.

La collecte des intérieurs et de **toute la coquille** est un second
parcours global inchangé : elle conserve notamment a et b. L'ordre de
comptage ne devient pas un filtre de payload. La preuve du rejet par
[frère au seuil restant](../q2_sibling_20260914/README.md) reste valable
au split du parent courant C : ses témoins crédités uniformément sont
hors de C, donc disjoints du frère immédiat. En seconde phase ils peuvent
appartenir à B₀ ; ne pas étendre cette exclusion au B₀ original ni à un
frère d'un ancien ancêtre.

## Modèle fermé et contre-fixtures

[model.py](model.py) réemploie explicitement deux modèles d'audit épinglés
pour l'arbre et les bornes. Le juge scalaire H est indépendant ; les tests
ponctuels et la collecte utilisent la clé de boule et sa puissance entière.
Le juge reconstruit les préfixes pour vérifier le compte à chaque étape.
Ces listes et son calcul coûteux ne sont pas des objets proposés au produit.
Le lookup linéaire du rang de a sert uniquement à installer les fixtures.

[r1_RUN.json](r1_RUN.json) ferme les sources, leur
[archive](r1_sources.zip) et les [résultats complets](r1_RESULT.json).
Le relecteur [verify.py](verify.py) rejoue sous Python −O, compare au
résultat normal et conserve les contrôles dans [r2_VALIDATION.json](r2_VALIDATION.json).
La première validation passée reste conservée avec son
[lecteur et README](r1_validation_sources.zip), avant clarification du
sélecteur `--run` : `--name` nomme le rapport, `--run` choisit la capture.

Fixture ciblée : B₀={0,1,2,3}³, a=(1000,1000,1000),
W={997,998,999}×{998,999}², K=10. Les douze témoins sont strictement
intérieurs pour chaque paire a×B₀, avec minimum H égal à 2 988.

| Modèle | Tâches C | Divisions C | Visites de comptage | Bornes + tests ponctuels | Opérations structurelles |
|---|---:|---:|---:|---:|---:|
| DFS global | 127 | 63 | 1 710 | 1 254 + 456 | 0 |
| B₀ différé seul | 15 | 7 | 33 | 31 + 0 | 2 |
| B₀ différé et ancre exclue | 1 | 0 | 6 | 2 + 0 | 4 |

Sur les **2 926 paires** du nuage, les trois modes produisent exactement
1 541 admissions et 1 385 rejets, avec les mêmes clés, intérieurs et
coquilles. Les visites de comptage sont respectivement **76 967, 77 878
et 77 432**. La collecte ajoute 61 839 visites dans chaque mode. Cette
couverture par nœuds globaux et IDs b>a est un dispositif de juge ; elle
n'est ni la WSPD Pure/s12 du constructeur, ni la tour FULL.

Les fixtures bornées comparent parcours continu et reprises après chaque
opération de comptage, FIFO et LIFO : mêmes sorties et travail discret.
Le domaine complet à 77 sites n'est joué qu'en budget 1/FIFO. La collecte
reste synchrone ; sa suspension n'est pas testée ici. Les cas exercent
les deux fins de B₀, des transitions avec crédit, et des divisions dans
les deux phases, notamment après crédit en seconde phase.

Huit mutants sont effectivement détectés : B₀ redéfini par l'enfant,
phase enfant réinitialisée, crédit perdu à la transition, redémarrage à
la racine, seconde phase omise, mauvaise fin globale, ancre retirée de
la coquille et descente structurelle tardive. Les incohérences de contexte
sont détectées par l'invariant de préfixe ; mauvaise fin et redémarrage
produisent un faux rejet. **La descente tardive conserve les sorties**
mais perd le contrat de zéro division sur la fixture : c'est une réfutation
de travail, pas une erreur géométrique.

## Raccord massif et expérience LiDAR suivante

Contexte fixe : identité de l'index, ancre/rang, B₀ et escape(B₀). État
par tâche : C, phase, curseur, compte, plus les constantes déjà préparées.
Le rang est connu dans la boucle d'ancres et les relations d'ancêtre se
lisent dans les intervalles DFS ; aucune recherche de n sites ni table
inverse par rectangle n'est nécessaire. Une future file doit posséder
le contexte ou une référence durable, pas un pointeur vers une pile terminée.
Les compteurs et tampons d'émission ont une propriété définie par worker.

Les descentes structurelles visitent deux chemins d'ancêtres. Leur reste
peut être recopié par les subdivisions : une estimation prudente est
O(T·D) pour T tâches et profondeur D, pas O(D) pour toute la racine.
Compter séparément géométrie, descentes, sauts, transitions et collecte ;
ne pas augmenter les tâches logiques ou démarrages racine à chaque reprise.
Aucun nombre d'octets C++ ni gain CPU/GPU n'est mesuré par ce modèle.

La [contrelecture statique du raccord en chantier](PRODUCT_REVIEW.json)
est favorable sur ces points ; les trois fichiers relus sont
[capturés séparément](product_review_sources.zip), sans compilation.
Le produit compte les seules évaluations géométriques dans
`count_node_visits`, alors que `node_visits` du modèle inclut la structure.
Dans ce raccord Shared, une identité de contrôle est
`cursor_advances = count_node_visits - query_splits + structural_splits + deferred_skips + anchor_skips + phase_switches`.
Le compteur comprend le retour vers B₀ : il ne certifie pas une progression
monotone des IDs. La progression est celle du couple phase/curseur.

Une comparaison peu coûteuse à prévoir concerne les **B₀ singleton dès
l'entrée racine**. Aucun partage entre partenaires n'est alors possible :
choisir une fois l'ancien parcours de paire complet, compte zéro, est
exact. Un descendant devenu singleton garde en revanche son ordre hérité.
Le raccord choisit le plus petit facteur pour les ancres : chaque rectangle
feuille×feuille donne exactement une telle racine. Le recalcul des reçus
fermés par [root_singletons.py](root_singletons.py), sans nouveau census,
donne 27,2–33,2 % de racines singleton sur les LiDAR 8k–50k et 62,7–66,2 %
sur amas 8k/16k. À 50k : 1 040 458 sur 3 829 093 racines.
Ce sont des fractions de racines, pas de temps ; le choix doit être apparié
à ComplementFirst uniforme avant décision, avec un seul `root_start`.

Tester les trois scans primaires du [protocole LiDAR](../lidar08_20260914/README.md)
sans hypothèse d'alignement exact. Garder le temps intégré et tous les
compteurs : l'ordre expose certains témoins communs mais peut en retarder
d'autres, notamment dans B₀. Le cas sans témoin extérieur a=1000,
B₀=0..63 sur une ligne retrouve la seconde phase avec crédit nul et
peut encore fragmenter jusqu'à 127 tâches. P0 et les contrats FULL restent ouverts.

Reproduction depuis la racine, dans une nouvelle capture sans écrasement :

```bash
python3 -B morsehgp3D_v8/audits/q2_complement_20260914/record.py --name replay
python3 -B morsehgp3D_v8/audits/q2_complement_20260914/verify.py --name replay --run replay
```

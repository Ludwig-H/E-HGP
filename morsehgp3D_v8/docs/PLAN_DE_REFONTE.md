# Plan de refonte : moins de travail, puis davantage de parallélisme

13 septembre 2026. Propositions issues de l'audit, **pas implémentations v8
validées**. Le [rapport de synthèse](AUDIT_V7_SYNTHESE.md) situe les mesures.
L'ordre demandé reste mono-thread, multi-CPU local, puis GPU G4 SPOT.

## 1. Une seule chaîne et un seul contrat de sortie

Le premier livrable de code devra être une API FULL explicite : entrée,
métrique, Kmax, convention de coupe, nœuds, parents, contributions,
verticales, statut de complétion. L'archive F et les probes privés ne
seront pas réétiquetés en produit FULL.

Réutiliser explicitement les primitives v7 sélectionnées et leurs
contre-fixtures, avec sources épinglées et requalification v8. Les anciens
producteurs restent des différentiels externes. Ne pas maintenir plusieurs
copies concurrentes du moteur dans `build/` et des archives opaques.

Une préparation géométrique immuable possédera l'entrée, l'index, le
catalogue certifié, les clés, les permutations, les rangs de niveaux et
les masques. Les vues ne devront ni survivre à leur propriétaire ni se
faire passer pour celles d'un autre nuage. Son constructeur public devra
produire ou vérifier réellement ses certificats, pas accepter un faux
certificat bien formé. Une validation relative à un producteur donné sera
annoncée comme telle, jamais transformée en preuve de complétude universelle.

## 2. Objets proposés pour les étapes coûteuses

| Étape | Objet à conserver | Grain de tâche parallèle | Travail à éviter |
| --- | --- | --- | --- |
| Front WSPD | Descripteurs plats de rectangles, masques q, témoins identifiés | Rectangle ou sous-tâche spatiale selon coût | Redémarrer tous les parcours et recréer une équipe à chaque vague |
| Témoins locaux | Crédit saturé par ligne/colonne, classes de sites et preuves de blocs | Bloc de crédits ou tuile de points | Comparaisons exhaustives dans de gros facteurs |
| q2 | Tuile de paires survivantes | Paires ou petits paquets | Sérialiser tout A×B derrière un seul worker |
| q3 | Seed canonique et plage de cover | Seed×plage de sites | Recherche complète séquentielle par seed |
| q4 | Intervalles/racines exacts, segments identifiés | Calculs de racines puis tri/scan segmentés | Balayage mono de tous les événements d'une seed |
| Catalogue/census | Une boule canonique, intérieur/coquille, rang partagé | Boules ou blocs de parcours | Clés, tris et validations identiques répétés |
| Rattachements | Facettes entières uniques, terminales certifiées | Requêtes géométriques indépendantes | Descentes répétées et essai lexicographique de tous les supports |
| Histoire par K | Graphe daté sur naissances, puis forêt minimale | Arêtes puis contractions de sous-arbres | Calendrier global qui déclenche de la géométrie |
| Contributions/verticales | Marques datées, index historiques partagés | Requêtes indépendantes et préfixes d'écriture | Reconstruire les index voisins ou les mêmes requêtes |

Un rectangle est une unité logique de preuve et de couverture ; il n'est
pas nécessairement une unité physique de lancement GPU. Découper le travail
en tuiles ne doit pas émettre plusieurs fois une paire ni perdre un crédit.

## 3. Éliminer tôt les rectangles, sans payer plus que ce que l'on épargne

Les détails et les bornes entières sont dans
[WSPD q2/q3/q4](../audits/WSPD_Q2_Q3_Q4.md). Priorités :

1. Proposer rapidement quelques témoins probables ; vérifier exactement
   qu'ils sont intérieurs pour **tout** le rectangle. Le proposeur peut
   échouer : seul le certificat autorise l'élimination, avec repli complet.
2. Transporter au plus h_q−1 **identifiants** déjà certifiés du parent vers
   ses enfants : la propriété universelle se conserve par restriction.
   Exclure ces identifiants des nouveaux crédits. Ce transport est une
   proposition nouvelle à contre-auditer ; copier seulement le compteur
   h peut compter deux fois les mêmes sites.
3. Utiliser h extérieur à A∪B, puis h_a dans A privé de a et h_b dans B
   privé de b. La disjonction est une partie de la preuve, pas un détail
   d'implémentation. Saturer lorsque le seuil utile est acquis.
4. Choisir les blocs positifs **et négatifs** en fonction du coût réel ;
   un petit facteur peut coûter moins cher en scalaire. Réutiliser la
   piste v7 négative/saturation seulement après raccord et requalification.
5. Si les certificats ne suffisent pas, raffiner les classes de crédit
   ou les tâches de paires ; compter les visites et les crédits effectivement
   payés. Ne pas remplacer un plafond arbitraire par une boucle quadratique
   cachée sous le mot « adaptatif ».

Le seuil vaut Kmax/Kmax−1/Kmax−2 pour q2/q3/q4. Le plus petit fuseau q4
est une condition suffisante d'intérieur commun, pas toute la boule
circonscrite de chaque tétraèdre. Un certificat indécis ne justifie aucune
suppression. Le cover q4 doit rester assez large pour inclure tous les
points qui peuvent contribuer à sa profondeur.

Comparer s8/10/12 en séparant temps du front, histogrammes et chaque voie.
Ne pas choisir s sur le seul nombre de candidats finals : le coût de
recherche des témoins peut croître alors que ce nombre baisse.

## 4. Réduire le coût d'une terminale avant de multiplier les threads

La cible est le coût des milliards d'essais de supports, non le coût d'une
opération arithmétique isolée. Comparer, sur les mêmes requêtes :

- Support proposé à partir de la boule source et de l'échange précédent.
- Solveur de petit ensemble qui propose directement une MEB candidate.
- Certification exacte : support positif, confinement, coquille et clé.
- Repli complet actuel si la proposition n'est pas certifiée.

Mesurer taux de succès, essais évités, nouvelles recherches d'intrus et
coût des cas difficiles. Une proposition flottante ne devient jamais
une décision géométrique. Une terminale doit être comparée **avant** sa
normalisation historique : une mauvaise terminale peut rejoindre par
hasard la même racine finale et tromper un juge trop faible.

Regrouper les demandes identiques par leur facette entière ; conserver
les destinations originales pour disperser les réponses. Chercher le
compromis entre réemploi global et mémoire des fenêtres. Un cache sans
bornage de résidence ni propriétaire stable n'est pas une solution massive.

## 5. Paralléliser la vraie histoire, pas seulement son tri

Une fois les rattachements connus, chaque ordre dispose d'un graphe daté
sur ses naissances. En extraire une forêt couvrante minimale conserve
les composantes de toutes les coupes. Construire ensuite l'histoire de
cette forêt, puis regrouper les événements de même niveau par vraie
connexité, sans mélanger les plateaux disjoints.

La littérature fournit des constructions parallèles de dendrogrammes à
partir d'un arbre pondéré. RCTT utilise une contraction d'arbre et obtient
une profondeur polylogarithmique ; sa variante ascendante dépend de la
hauteur du résultat. C'est une raison de tester aussi des histoires en
peigne, pas seulement des arbres équilibrés.
[Source primaire, Dhulipala et al., SPAA 2024, v2](https://arxiv.org/abs/2404.19019v2).

PANDORA propose une contraction récursive destinée aussi aux GPU, avec
une implémentation Kokkos CPU/GPU. Elle fournit une seconde piste pour
éviter le calendrier séquentiel.
[Source primaire, Sao et al., version 1](https://arxiv.org/abs/2401.06089v1).

Ces sources sont examinées pour leur algorithme, pas pour transférer leurs
temps à HGP. Elles partent d'un MST/arbre pondéré **déjà disponible** :
ni génération Gabriel, ni rattachements, ni contributions, ni verticales
HGP ne sont payés par leur seule reconstruction de dendrogramme.
L'adaptation aux dates exactes égales et aux multifusions doit être prouvée.

## 6. Faire payer une seule fois les objets communs

Remplacer les préparations répétées Builder→Atlas→resolver par un
propriétaire certifié partagé. Construire chaque index historique une
seule fois, soit dix préparations au lieu de dix-neuf ; ne conserver
que le précédent et le courant lorsque leur usage le permet.
Conserver les marques calculées pendant le premier parcours et prouver
que leur histoire/propriétaire est le bon avant leur réemploi aval.

Pour l'export, compter les tailles puis attribuer les destinations par
sommes préfixes. Éviter petits vecteurs, fragments temporaires et copies
de populations complètes. Les références de populations peuvent être
partagées sans confondre les composantes qui recouvrent les mêmes points.

La sérialisation FULL industrielle doit avoir un manifeste distinct de
l'archive F, des offsets suffisamment larges, des lots identifiables et
un journal de reprise. Le coût de l'archive durable doit être séparé du
contrat de livraison en mémoire, jamais retranché sans déclarer le périmètre.

## 7. CPU et GPU doivent exécuter le même plan de travail

Après le chemin mono, une équipe CPU persistante consommera les tâches
plates. À quatre CPU, vérifier objets identiques et destinations fixes,
puis mesurer le gain apparié ; éviter de relancer des pools imbriqués.
Exiger un travail identique quand seul l'ordonnancement change ; compter
les opérations spéculatives supplémentaires lorsqu'un découpage plus fin
modifie le travail payé avant les arrêts anticipés.

Sur GPU, conserver catalogue, rangs, tâches, réponses et preuves compactes
sur carte autant que possible. Préfixes, compactions, tris segmentés et
réductions de statuts remplacent les reconstructions hôte ligne par ligne.
Le repli CPU rare doit être compté avec ses transferts. Chaque kernel
devra être exécuté sur carte : compilation et émulation ne suffisent pas.

La géométrie exacte peut entraîner des registres ou des piles trop lourds.
Choisir le grain de tâche à partir du travail/résidence observés, pas du
simple nombre de rectangles. Une voie GPU donnant un grand débit de
primitives sans réduire le temps de tour ne franchit aucun contrat.

## 8. Portes de preuve avant mesures coûteuses

| Décision | Tests positifs et contre-fixtures à porter explicitement |
| --- | --- |
| Objet FULL | K1=single-linkage, minima isolés, recouvrement sans fusion, K=n |
| Réduction Gabriel | E5, graphe induit à quatre points, attache silencieuse avec bonne date |
| Plateaux | AB/ABC même boule, composantes disjointes de même date, plateau coupé entre lots, grande arité |
| Témoins | h/h_a/h_b disjoints, double crédit parent/enfant, marge nulle, succès/non-vacuité et absence de témoin |
| q3/q4 | Supports non positifs, dégénérescences, cover trop étroit, racines égales, profondeur qui redescend |
| Resolver | Terminale fausse mais racine finale correcte, semis rejeté puis repli exact, intrus choisi différemment |
| Propriété | Certificat forgé, mélange de nuages, vue périmée, mutation d'un propriétaire emprunté |
| Tour/export | Contributions datées, verticale fermée à la naissance, naturalité, refus de publication partielle |
| Massive | Gros facteurs, grandes coquilles, débordement d'identifiants d'objets, allocation refusée, reprise |

L'oracle exhaustif reste borné et indépendant. Il ne devient pas le
producteur général sous un autre nom. Réutiliser une fixture ne signifie
pas hériter de son résultat ancien ; enregistrer les nouvelles commandes.

## 9. Mesurer la croissance et les contrats sans dépenser à l'aveugle

Localement, mêmes sources et mêmes entrées, n=8 000/16 000/32 000,
s=8/10/12, mono puis quatre CPU. Familles : uniforme, terrain/filiforme,
amas séparés, arcs adverses ; ajouter les cas de dégénérescence au
protocole fonctionnel. Mesurer par étape travail total, pire tâche,
répartition, mémoire simultanée et volume de sortie.

Le temps par unité de sortie distingue amplification algorithmique et
sortie réellement grande. La borne quadratique FULL interdit une promesse
universelle d'énumération sous-quadratique ; elle n'autorise pas du travail
quadratique inutile lorsque la sortie est petite. Ni le choix de s ni le
passage GPU ne remplacent cette analyse.

Une fois la chaîne complète raccordée, ouvrir une courte session G4 SPOT
avec les scripts gardés du dépôt. Qualifier toute la tour 1..10 à 50k,
puis le repli 1..5 si nécessaire, avec deux échauffements, dix nuages frais
par famille et p95. Si une seconde est atteinte, viser 100 ms au même
périmètre. Aucun seuil par composant n'est un contrat accompli.

Le contrat multi-millions est séparé : commencer par la résidence et les
formats, puis une complétion vérifiable aux paliers prévus, dont plusieurs
dizaines de millions. Ne pas extrapoler un pic 50k en succès massif.
N'acheter du temps GPU que pour une expérience qui tranche une question
ouverte ; arrêter la cible exacte dès le résultat utile acquis.

## 10. Première décision à prendre après cet audit

Faire une petite tranche verticale v8 FULL, avec les bons objets, qui
restitue les fixtures complètes et dont toutes les phases sont mesurables.
Elle prépare les tâches géométriques indépendantes et la forêt datée ;
le CPU mono sert de référence. Ensuite, raccorder les améliorations amont
et aval une par une. Aucune réécriture géante sans différentiel, aucune
accumulation de variantes non intégrées, aucun claim industriel anticipé.

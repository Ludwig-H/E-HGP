# Marques historiques dans le premier parcours des fusions

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Delta privé après les [gardes certifiées](GARDES_RANGS_CERTIFIES_20260911.md),
sans remplacement du moteur actif. GCP non utilisé.

## Objet conservé

Les graphes datés portent des naissances natives, un certificat forestier
d'arêtes et des marques. Une marque demande la composante de son
représentant à une date fermée ; elle ne crée ni nœud ni fusion. L'histoire
doit conserver les vraies multifusions et leurs parents, les racines,
successeurs, ancres des naissances et toutes les marques, silencieuses
comprises. Ce delta ne remplace pas cet objet par une forêt sur les points.

Le [prototype indépendant](../audits/receipts_fused_marks_20260911/README.md)
montre comment résoudre les marques pendant le premier parcours, après
fermeture de chaque plateau. L'ancien constructeur reconstruisait ensuite
leur composante avec un second DSU et un rejeu des parents. Ce second
parcours n'est pas nécessaire : le premier DSU possède déjà la réponse.

## Pourquoi les sorties restent identiques

Les deux listes triées, arêtes et marques, sont parcourues par deux curseurs,
sans nouveau tableau global d'événements. À chaque date :

1. Émettre les naissances admises, dans le même ordre date/identifiant.
2. S'il existe des arêtes, consommer le plateau entier et produire ses
   multifusions atomiques avec les mêmes parents et départages.
3. Résoudre les marques à cette date sur les composantes désormais fermées.

Une consultation de marque ne change ni les ensembles de la partition,
ni leur taille, ni la racine choisie par les unions ; la compression de
chemin ne change donc pas les décisions ultérieures. Entre deux plateaux,
elle peut avancer l'émission des naissances sans changer leur ordre.
À égalité, le nœud de fusion conserve la représentation brute du niveau
de la première arête ; la marque conserve sa propre date d'admission.
Le représentant est recherché dans les naissances triées par identifiant,
pas dans les ancres de sortie encore en ordre d'émission. Les naissances
restantes sont émises même après la dernière arête ou marque.

Ces arguments concernent un certificat valide et les mêmes comparaisons
de dates. Ils ne prouvent ni la géométrie du certificat ni la complétude
du producteur WSPD. La forêt terminale et les plateaux restent distincts
du régime régulier des minima Gabriel FULL.

## Travail et résidence

Soient L naissances, E arêtes, M marques et J multifusions. Sur succès :
N=L+J nœuds, P=E+J références de parents et L−E racines. Le nouveau
parcours initialise L sommets DSU, effectue E unions et résout M marques.
Leurs recherches ne disparaissent pas : elles passent dans le premier DSU.

Les tris, copies de normalisation, recherches d'identifiants et groupes de
plateau restent payés. Sur ce certificat forestier, E≤L et J≤E ; le travail
du reconstructeur reste O((L+M) log(L+M+2)) et sa résidence O(L+M).
Il n'ajoute aucun parcours de profondeur par marque ni aucune paire de
points. Ce n'est pas une borne sous-quadratique en n pour la génération
de tous les régimes : les tailles intermédiaires et de sortie restent
à qualifier séparément.

L'ancien rejeu ne visitait que le préfixe admis par la dernière marque,
pas nécessairement tous les nœuds et parents. Il est donc incorrect de
soustraire P unions à tous les certificats, notamment sans marque ou avec
une queue tardive. Le nouveau compteur `parent_replay_attempts` vaut zéro
par absence de cet algorithme, pas par estimation de son ancien coût.

Trois tableaux disparaissent : les deux tableaux du second DSU et celui
des segments actifs, soit 24L octets logiques lorsque les indices occupent
huit octets. Ce chiffre n'est pas un gain de RSS ou de pic mémoire : les
marques produites plus tôt peuvent maintenant cohabiter avec les temporaires
d'un plateau futur. Les six paires taille/capacité observées isolent entrée
normalisée, premier DSU, segments, ordre des naissances, scratch de plateau
et histoire finale. Leurs sommes entre ordres ne sont pas des pics simultanés.
Réallocations transitoires, pile des tris et allocateur restent hors de ces
capacités nommées.

## Frontière avec les prochains raccords

L'export FULL est inchangé et recalcule encore ses consultations historiques.
Il n'utilise pas les marques : une simple égalité de payload FULL manquerait
donc certaines erreurs de ce delta. La gate compare d'abord tous les champs
physiques des histoires et marques sur les mêmes certificats.

Le réemploi contributif demandera ensuite une histoire fraîche liée à son
propriétaire immuable, ou une validation indépendante des marques externes.
Une marque vers une autre composante vivante peut être structurellement
plausible et néanmoins fausse. Conserver dates d'admission et ordre physique
des contributions. Le partage des index de chaînes adjacents, 19→10 pour
K1..10, reste un autre changement : les coupes verticales ne se remplacent
pas par une consultation à la naissance.

Le propriétaire devra aussi retenir l'index et le census que l'Atlas
emprunte, à adresses stables et sans alias mutable. Une factory publique
acceptant extraction et certificats arbitraires ne suffirait pas : elle
pourrait reconstruire correctement le mauvais certificat. La frontière
minimale retenue pour la suite est une factory intégrée qui exécute le
producteur qualifié puis la reconstruction, sans adopter de certificats
ou histoires externes comme réponses fiables. Une empreinte ne remplace
pas cette provenance. Un futur backend séparé devra qualifier son propre
producteur avant de pouvoir publier le même objet lié.

Qualification et mesures propres : [paquet du raccord](../receipts/fused_history_streaming_20260911/README.md).
Aucun contrat 50k sous 1 s/100 ms ou plusieurs dizaines de millions de
points sur G4 n'est acquis par cette simplification CPU.

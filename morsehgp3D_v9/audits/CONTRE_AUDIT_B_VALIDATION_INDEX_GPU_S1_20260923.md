# Garde d'index GPU S1 : exactitude renforcée, coût caché à rendre linéaire

23 septembre 2026. Contrelecture du commit **`7565451fc`** ; aucun CUDA
ni GCP exécuté par B. Le nouveau `validate_filter_input` est appelé
avant le device, par le lanceur CUDA **et** par son stub hôte. Il
refuse les coordonnées/boîtes hors u18, les enfants non partitionnés
et une boîte qui manque un point de sa plage. Les fixtures de l'audit A,
dont le faux rejet q4 `4→0` sous une racine forgée, sont maintenant
dans la porte du filtre. Cela ferme ces contre-exemples pour les
**entrées certifiées du producteur** ; le test positif CUDA reste à faire.

## Coût exact du code publié

Pour chaque nœud, les lignes `filter_runner.hpp:82–86` relisent
**tous** les rangs de sa plage, sur trois axes. Le travail vaut donc
`3 Σ_v |plage(v)|`, pas `O(n+N_nœuds)`. Sur l'index spatial produit,
la profondeur `D` est au plus 54 en u18 : le travail est `O(nD)`
(formellement linéaire à précision fixée, mais avec une grosse
constante) ; à 50 M sites,
la borne brute de 54 niveaux approche **2,7 milliards** de visites de
rangs, hors construction d'index. Un arbre en peigne valide pour les
tests structurels, avec plages de tailles `n,n−1,…`, donne même
`Θ(n²)` à l'API brute. Ce n'est pas la croissance observée du moteur
sur LiDAR : c'est un coût évitable de la **validation**.

La durée `gpu.total_ms` commence après cette garde et après les
allocations initiales ; elle ne voit donc pas ce scan, ni l'index/front,
ni la consommation des réponses. Pour la porte de débit S1, publier
séparément `validation_ms` et le mur complet du probe ; pour le contrat
de la tour, intégrer réellement ce coût ou réutiliser une certification
une seule fois. Ne pas présenter `gpu.total_ms≤100 ms`, s'il survient,
comme une chaîne ni comme le temps de l'appel complet au filtre.

## Certificat linéaire équivalent

On peut accepter les mêmes boîtes **lâches mais sûres** en temps
`O(n+N_nœuds+R)` :

1. Vérifier domaine u18 des rangs et des boîtes (`low≤high`), racine
   `[0,n)`, enfants aux IDs
   postérieurs au parent, partition exacte des plages, et exactement
   un parent pour chaque nœud non racine. L'ordre des IDs interdit les
   cycles ; cette condition rend tout nœud atteignable depuis la racine.
2. Pour chaque **feuille**, scanner sa plage une fois et calculer
   l'enveloppe vraie des points. Par partition récursive, les feuilles
   couvrent `[0,n)` sans doublon de rang : travail total `O(n)`.
3. En ordre inverse des IDs, calculer l'enveloppe vraie de chaque
   interne comme l'union des enveloppes de ses deux enfants. Exiger
   `hull(v)⊆box(v)`. Le test est nécessaire et suffisant pour que la
   boîte annoncée contienne tous les points de la plage, par induction.
   Une boîte plus large reste admissible et ne rend que les certificats
   de témoin moins sélectifs.
4. Vérifier les `R` rectangles (IDs, masques), puis conserver un jeton
   d'index **possédé et immuable** réutilisable entre lots. Un pointeur
   `const` emprunté ne garantit pas à lui seul que l'appelant ne mute
   pas le stockage pendant validation/copie GPU.

La garde actuelle ne vérifie pas non plus que deux rangs n'ont pas les
mêmes XYZ. Le producteur `prepare_cloud` refuse les doublons, donc la
sonde G4 normale reste dans son domaine ; une API brute autonome doit
soit annoncer explicitement le contrat « nuage unique déjà certifié »,
soit vérifier cette unicité (tri/radix ou certificat possédé). Sinon,
deux rangs représentant la même position peuvent être comptés comme
deux témoins, contrairement à la convention du moteur. Cette réserve
n'est pas une sortie LiDAR erronée observée.

Porte locale avant de remplacer le scan : index réel 2k/8k, racine
forgée, partitions chevauchantes, nœud orphelin, enfant partagé,
boîtes lâches et contacts, doublons selon le contrat retenu ; égalité
des refus/acceptations pertinents et des masques exacts. Mesurer
`Σ|plage|` et mur de garde sur 8k/16k/32k puis trames entières ;
tester séparément un peigne synthétique sans lancer le GPU. Le
remplacement réduit un coût d'entrée, pas la masse des paires ni les
formes q3/q4 : aucun contrat sous-quadratique global n'en découle.

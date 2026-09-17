# Proposer plus de témoins pour éviter des census q2 inutiles

15 septembre 2026. Proposition d'exploration v8 hors registre,
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. Aucun changement de moteur ni gain qualifié dans cette note.
Périmètre initial : voie q2 seule, pas q3/q4 ni tour HGP FULL.

## Le problème concret

Le [front réel](P0_FRONT_REEL.md) cherche un site proche du milieu du
produit A×B par une seule descente de l'index. Il essaie ensuite une
fenêtre de **K propositions** autour du rang spatial de ce site.
Chaque témoin strictement intérieur à toutes les boules du produit
apporte une unité ; K témoins distincts suffisent au rejet q2.

À K=10, avec seulement dix propositions, il faut donc que les dix
réussissent. Un seul site situé hors de la région commune, sur sa
frontière, ou dans A/B suffit à empêcher ce certificat. Pourtant,
d'autres témoins immédiatement voisins dans l'ordre peuvent exister.
L'échec des dix propositions n'indique pas une faible profondeur réelle.

Le code examiné est `src/wspd/front.cpp`, méthode `Front::filter` :

- `count = min(kmax_, order_.size())` règle le nombre de propositions ;
- la fenêtre est centrée et bornée autour du rang `pivot` déjà trouvé ;
- les rangs appartenant aux facteurs sont sautés, sans remplacement ;
- le prédicat q2 demande `h_minimum(A.box, B.box, {z}) > 0` ;
- seul le masque des rejets complets est transmis aux descendants.

Ce filtre est appelé **avant** le test de séparation, sur les produits
disjoints parcourus, pas seulement sur les rectangles finalement émis.
Il faut donc compter son coût sur tout ce parcours.

Une mesure constructor indépendante du changement proposé, uniforme8k,
K10/s8/Pool64, dans `receipts/q2_singleton_batch_20260915/`, capture
`tuning_r9hbnll1/record_0000.json`, rapporte3 194 249 candidates,
dont2 914 705 rejetées ensuite par le census, soit environ91,25 %.
Le census paie171 895 354 visites Z, contre279 544 supports admis.
Ces comptes motivent un filtre de rejet plus efficace ; ils ne prouvent
ni que les témoins manquants sont dans une fenêtre élargie, ni un gain.

## Séparer le seuil K et le nombre L de propositions

K reste le seuil mathématique de rejet q2. L règle seulement le nombre
de sites que l'heuristique peut proposer dans ce produit. Comparer
L=K, L=2K et L=4K, avec troncature à n pour la fenêtre, sans modifier K.
Le nombre de propositions ne plafonne ni la recherche exacte suivante,
ni les données, ni les sorties.

La variante minimale conserve d'abord **exactement la fenêtre historique
de K sites et son ordre**. Si elle rejette déjà, aucune dépense nouvelle.
Sinon, pour un produit éligible, elle poursuit dans la fenêtre élargie
autour du même pivot, en omettant les rangs déjà proposés. Les parties
gauche et droite supplémentaires sont des intervalles disjoints : pas
de liste, de déduplication quadratique ni de nouvelle descente d'index.
À L=K, tous les compteurs et décisions doivent retrouver la référence.

Le compte local acquis par les premiers témoins reste utilisable pendant
**ce même appel du filtre** : les propositions supplémentaires sont
distinctes des premières. Arrêter dès K succès stricts. Ne pas confondre
L rangs examinés avec L témoins extérieurs valides : les rangs de A/B
restent comptés parmi les propositions, puis sautés sans crédit.

Deux politiques bornées sont à comparer, sans multiplier les variantes :

| Politique | Produits recevant les propositions supplémentaires | Coût visé |
|---|---|---|
| Petits facteurs | `max(|A|, |B|) <= B0`, par exemple B0=16 pour une première comparaison | Les nombreux produits proches des petites paires |
| Tous produits | Tous les produits q2 ayant échoué avec la fenêtre historique | Possibilité de rejeter plus haut dans le front |

B0 choisit où payer l'heuristique ; il ne supprime aucune paire et ne
limite aucun census. Utiliser le maximum des deux tailles pour décrire
deux petits facteurs, pas le minimum qui inclurait A singleton face à
un immense B. Commencer par une seule valeur B0 documentée.

## Pourquoi les rejets resteraient exacts

Pour chaque site z proposé, la borne Hmin sur A×B doit être strictement
positive. Alors z est intérieur à chaque boule diamétrale d'une paire
(a,b) du produit. K sites distincts prouvent une profondeur au moins K
pour toutes ces paires : elles ne peuvent produire un support q2 retenu.

Les sites stricts ne peuvent appartenir aux facteurs : choisir ce site
comme extrémité donnerait H=0. Le saut explicite des rangs de A/B conserve
cette propriété sans payer le prédicat. La permutation globale associe
des rangs distincts à des IDs distincts ; les intervalles supplémentaires
disjoints empêchent de créditer le même ID deux fois.

Si moins de K témoins sont trouvés, le chemin exact actuel continue.
**Le census repart de zéro** : les témoins ponctuels du filtre ne sont
ni un préfixe de son parcours Z ni un ensemble prouvé disjoint de ses
comptes futurs. Les comptes partiels ne passent pas aux descendants ;
seuls les rejets complets peuvent être hérités, comme aujourd'hui.
Les tests de coquille restent stricts : H=0 ne compte jamais comme
intérieur et toutes les coquilles admises restent collectées.

Les fenêtres doivent contenir la fenêtre historique, y compris près des
bords de l'ordre. Cela permet de vérifier qu'un produit rejeté par L=K
l'est aussi par les extensions. La géométrie du front et du census peut
changer parce que des produits sont rejetés plus tôt ; contrairement au
lot singleton, on cherche précisément moins de travail. Seules les sorties
q2 complètes doivent rester identiques à l'oracle et à la référence.

## Coût à mesurer, sans déplacement du carré

Avec T produits visités, profondeur d'index D, L<=4K et R rectangles
émis, le filtre reste O(T(D+K)+R). Le supplément de propositions est au
plus3K par produit éligible dont le filtre historique n'a pas saturé.
Il n'y a aucun scan de A ou B et aucune opération O(|A|²+|B|²).
Ce raisonnement **ne borne pas T**, le résidu, les visites Z, les plans
Pool ni la taille des sorties en fonction de n. P0 global et une borne
générale sous-quadratique restent ouverts.

Le bilan doit inclure propositions additionnelles, tests stricts, produits
rejetés avant séparation, rectangles/ancres résiduels, préparation Pool,
visites census, collecte et callback. Publier les temps mur du pipeline
complet q2, pas le temps seul du filtre. Une baisse des visites Z qui
coûte davantage en tests de propositions n'est pas une optimisation.

Les [fausses pistes](FAUSSES_PISTES.md) contiennent déjà le coût d'une
descente répétée par produit. La présente proposition ne résout pas ce
coût ; elle réutilise le pivot existant sans nouvelle descente. Ne pas
présenter une seconde recherche de voisins comme cette variante légère.

## Vérification et premier essai limité

Avant une grosse campagne, comparer mono8k uniforme et terrain :
référence L=K, petits facteurs L=2K/4K, puis tous produits seulement si
les premières mesures justifient son coût. Garder K5 et K10 distincts.
Si le pipeline total progresse, poursuivre n8k/16k/32k, s8/10/12 et les
amas/rangées, avant multi-CPU ou GPU. Conserver toute régression.

Les petits juges doivent couvrir les deux bords de la permutation, n<L,
des propositions dans A/B, les tangences H=0, les distances égales,
l'épuisement sans K succès, et un cas où seule l'extension trouve le
K-ième témoin. Vérifier les populations rejetées et tous les supports,
IDs intérieurs et coquilles. Les domaines des fenêtres sont assez petits
pour vérifier leur union et leur absence de doublons exhaustivement.

Alternative à discuter ensuite : tester un bloc du même chemin proche
du milieu, avec au moins K sites et Hmin>0 pour toute sa boîte. Ce serait
un certificat de saturation par population, sans parcours de ses sites.
Mais son taux de succès et ses bornes doivent être étudiés séparément ;
ne pas mélanger cette seconde proposition avec la première expérience.

Questions à l'auditeur : la fenêtre historique puis ses deux extensions
disjointes préservent-elles bien le certificat sous tangences ? Quelle
fixture discrimine l'extension efficace du simple surcoût ? Un certificat
de bloc pourrait-il obtenir le même rejet avec moins de tests sur ces
petits produits ? Aucun résultat d'audit n'est présumé ici.

# Proposer plus de témoins pour éviter des census q2 inutiles

15 septembre 2026, implémentée le 17 septembre 2026 (vingtième tranche).
Exploration v8 hors registre, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. Périmètre : voie q2 seule, pas q3/q4
ni tour HGP FULL. Les sections qui suivent sont la proposition d'origine,
conservée telle quelle ; la dernière décrit ce qui a été porté, jugé et
mesuré. Défaut du moteur inchangé.

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

## Implémentation : vingtième tranche

**Option.** `WspdFrontProposals{window_factor, small_factor_limit}` est un
paramètre final, à valeur par défaut, de `run_wspd_front`, de
`make_wspd_front_jobs` et des cinq entrées q2 (série, parallèle Coarse ou
Donate, équipe coopérative, plages d'ancres, lots). `window_factor` vaut 1,
2 ou 4 ; `small_factor_limit` borne `max(|A|, |B|)` pour l'extension et
n'a pas de limite par défaut. Le défaut `{1, sans limite}` reproduit le
front historique à l'unité. Toute autre valeur de facteur, une limite
nulle, un facteur différent de 1 en mode `Pure` ou avec une voie q3/q4
active sont refusés avant tout callback : aucun juge ne couvre une
fenêtre élargie pour q3/q4, elle n'est donc pas offerte.

**Filtre.** Même descente, même pivot, même fenêtre historique de
min(Kmax, n) rangs proposée en premier. Si le produit survit et que ses
facteurs respectent la limite, le filtre propose l'intervalle gauche puis
l'intervalle droit qui complètent la fenêtre de min(w·Kmax, n) rangs autour
du même pivot. Les crédits de la fenêtre historique restent acquis pendant
ce seul appel ; les rangs de A ou B sont comptés puis sautés ; H = 0 ne
crédite jamais ; l'arrêt a lieu au Kmax-ième succès strict. Une recherche
exige Kmax sites extérieurs, donc n ≥ Kmax + 2 et une fenêtre strictement
plus large : le cas d'une extension vide est une garde, pas une branche.
Un seul corps de boucle sert les trois intervalles, autour de la boucle
historique : le chemin par défaut ne paie rien pour l'option.

**Compteurs.** `extended_products`, `extended_proposals`,
`extended_proposals_in_factors`, `extended_credits` et `extended_rejections`
sont inclus dans les totaux historiques (`proposed_sites`,
`proposals_in_factors`, `h_bound_tests`, `witness_lane_credits`) : les
soustraire redonne exactement le travail de la fenêtre historique sur les
produits visités, ce que le lecteur de reçus exploite. Ils sont fusionnés
par `merge_work`, dont un `static_assert` sur la taille de la structure
interdit l'oubli d'un champ.

**Juges.** Porte `mhgp8_wspd_q2_proposals_gate` : oracle exhaustif borné de
l'arithmétique des fenêtres (contenance, bords, disjonction, absence de
doublon) ; rejeu indépendant du front q2 entier, écrit depuis le contrat
(H minimal par force brute sur les 64 paires de coins), comparé au moteur
compteur par compteur et rectangle par rectangle ; dix mutants causaux de
l'extension, chacun obligé de différer du moteur sur le corpus gravé
(tangence créditée, rangs de facteurs testés, intervalle droit d'abord,
crédits remis à zéro, limite ignorée, limite sur le minimum, fenêtre
décalée, pas d'arrêt à K, crédits hérités par les descendants, fenêtre
large parcourue dans l'ordre) ; deux fixtures minimales nommées à attendus
exacts ; constantes du moteur d'avant la tranche ; oracle force brute des
supports sur les cinq entrées q2. Portes du front, des jobs et du dispatch
étendues : sûreté contre l'oracle de paires indépendant, monotonie des
rejets en la fenêtre et en la limite, option portée par la préparation,
`run_job`, le dispatch et la fusion, avec une limite qui mord réellement.

**Réponses aux trois questions de la note.**

1. *Tangences.* Le certificat tient : prédicat strict par site, rangs de A
   et B sautés, intervalles disjoints, fenêtre historique contenue aux deux
   bords (jugé exhaustivement). Fixture `K2_tangent_second` : un seul témoin
   strict et deux sites tangents ; la paire de profondeur 1 reste résiduelle
   pour toute fenêtre, aucun crédit d'extension.
2. *Fixture discriminante.* `K2_second_in_extension` : trois témoins stricts,
   la fenêtre historique de deux rangs manque le second ; au facteur 1 la
   paire reste résiduelle, aux facteurs 2 et 4 elle est rejetée par
   l'extension (masse rejetée 4, quatre rejets d'extension). Le « simple
   surcoût » est le régime des deux rangées parallèles, conservé dans les
   campagnes comme régression publiée.
3. *Certificat de bloc.* Non porté. La mesure d'audit du 15 septembre
   (`audits/plafond_proposeur_20260915/`) le montre dominé par la fenêtre
   2K en pouvoir de rejet ; son seul intérêt serait d'épargner des tests
   ponctuels. Il reste une piste distincte, à mesurer séparément.

**Ce qui n'est pas fait.** Le défaut des sondes, campagnes et entrées reste
la fenêtre historique : aucun appelant n'est basculé d'office. Pas de
fenêtre élargie pour q3/q4, pas de transmission de crédits aux descendants,
pas de certificat de bloc. P0 global, borne sous-quadratique, FULL et G4
restent ouverts.

**Résultats qualifiés** ([reçus](../receipts/q2_front_proposals_20260917/README.md)).
78 CTests Release et Clang ASan/UBSan, portes proposals et dispatch sous
Clang TSan, 684 mesures, 300 comparaisons appariées, lectures normal/−O
identiques. Le défaut reproduit le moteur d'avant la tranche : les douze
triplets de la capture différentielle (build épinglé de la tranche 19,
sonde parallèle et facteur 1 du nouveau build, n8k/16k/32k, quatre
familles) ont tous leurs champs discrets identiques, pour un temps de
×0,98 à ×1,03. Fenêtre 2K, petits facteurs, temps mur du pipeline q2
complet rapporté à la fenêtre historique, sur les 13 configurations par
famille à n ≥ 8 000 (K5/10, s8/10/12, un et quatre workers, minimum de trois
captures pour l'échelle) : uniforme ×0,42 à ×0,54, amas ×0,51 à ×0,62,
terrain ×0,64 à ×0,71, rangées ×1,01 à ×1,18. Les candidats du census
tombent à 25,6 à 33,4 % (uniforme), 30,5 à 39,4 % (amas), 34,2 à 39,9 %
(terrain) et restent à 100 % sur les rangées ; supports, intérieurs et
coquilles identiques partout. À uniforme 8k K10, 3 194 249 candidats
deviennent 845 160 et 171 895 354 visites Z deviennent 45 020 340, pour
515 399 produits rejetés par l'extension. La fenêtre 4K gagne encore sur
uniforme et amas aux grandes tailles (×0,39 à ×0,50 et ×0,47 à ×0,61) et
rien sur terrain ; les deux politiques de limite diffèrent de moins de
0,2 point de candidats. Aucune configuration hors rangées n'est plus lente
que la référence. La croissance du travail restant est celle de la
référence : une constante est divisée, pas l'exposant.

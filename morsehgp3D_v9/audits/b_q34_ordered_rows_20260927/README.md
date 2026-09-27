# Partager des lignes B ordonnées, sans tri final des survivants

27 septembre 2026, base `4badf8b7d`. Cadre
`exploration_v9_hors_registre`, `cpu_reference`, `quantized_u18_input_only`,
`audit_ordered_credit_rows`, `public_status=not_claimed`. Prototype isolé ;
aucun moteur modifié, aucun GCP. Il utilise les crédits déjà certifiés du
Pool publié, dont la préparation reste entièrement payée par la sonde.

## Idée et preuve

Une classe A contient toutes les ancres ayant le même couple de crédits
`(a3,a4)`. Leur ensemble de partenaires B admissibles par le Pool est donc
identique : `a3+b3<K−1` en q3, ou `a4+b4<K−2` en q4. Les voies inactives
sont neutralisées avant les seuils. Pour chaque classe A **occupée**, on
construit une seule liste de partenaires B dans l'ordre de leurs rangs
originaux, avec le masque exact de chacun. Toutes les ancres de la classe
partagent cette liste. Une liste vide reste vide ; elle ne rouvre pas le
rectangle entier.

On conserve A dans l'ordre original et on associe à chaque ancre son numéro
de classe. Lire successivement les listes de ces ancres donne directement
le résidu dans l'ordre rectangle puis ligne/colonne. Chaque paire apparaît
exactement une fois. Le compactage **stable** des décisions S2 garde alors
l'ordre des survivants attendu avant S3 : aucun tri final de S n'est requis
par cette représentation. Un compactage atomique non ordonné ne suffit pas.

Pour un rectangle, noter m_c la population de sa classe A c et t_c la taille
de sa liste B. La mémoire des entrées de liste est T = somme(t_c), tandis
que le nombre de paires résiduelles est E = somme(m_c t_c). Toutes les
classes stockées sont occupées, donc m_c≥1 et T≤E. Il ne faut pas confondre
ces deux sommes : recopier la liste pour chaque ancre restaurerait E.

La construction scanne B une fois par classe A occupée :
W = somme(C_A |B|). Pour K≤10, C_A≤K(K−1)≤90, d'où W≤90 F_B et T≤W.
À K5, la constante est 20. Ce n'est ni un scan de |A||B|, ni un coût
O(F_B) de constante un. Tous les scans A/B, les 100 cases de l'histogramme
par rectangle et la mémoire T sont comptés. Les classes nombreuses peuvent
rendre cette variante moins intéressante que les bandes.

## Représentation réellement construite

`rows.hpp::Plan` possède quatre vecteurs :

- une case u8 par ancre A, dans l'ordre original, pour retrouver sa classe ;
- les offsets u64 des listes de classes A ;
- un rang **local** B u32 par entrée de liste ;
- le masque de voies u8 de chaque entrée, évalué une seule fois par classe.

La taille de chaque facteur est contrôlée avant réduction en u32 ; le total
T ne devient pas u32 par ce contrôle. Les offsets restent u64. Les origines
des facteurs viennent des rectangles de l'index partagé. Le plan ne conserve
aucune référence aux vecteurs de crédits, et aucune mémoire adoptée depuis
l'appelant ; seuls des accès const sont proposés. Le prototype est un
adaptateur de représentation, **pas** une factory géométrique acceptant des
crédits externes comme certificats.

Travail de ce constructeur : O(F_A+F_B+W+100R), mémoire conservée
O(F_A+T+C_A+R). Les capacités des quatre vecteurs et l'objet sont mesurées,
pas seulement les cinq octets logiques de chaque entrée B. Le nombre de
réallocations du constructeur n'est pas instrumenté.

## Raccord parallèle envisagé, non exécuté ici

Un préfixe des tailles de listes donne leurs emplacements collectifs. Un
second préfixe, sur les ancres A dans l'ordre original, donne leurs masses E
et les frontières de tâches. Des tuiles à frontières déterministes filtrent
les paires une seule fois ; leurs nombres de survivants sont préfixés puis
dispersés de façon stable. Les petits rectangles non préparés gardent leur
chemin ordonné direct. Ces deux populations restent disjointes.

Les préfixes par ancre, en-têtes, masques temporaires de tuiles, compteurs,
transports et sorties S ne sont **pas** alloués par le présent prototype.
Il faut les payer dans un port, sans tableau masqué de taille P. Le Pool
ne transmet toujours aucun crédit au census, et P logique demeure distinct
d'E ponctuel comme dans la [porte de raccord](../b_q34_batch_seam_20260927/README.md).

Cette variante peut aussi éviter de conserver les permutations des facteurs
dans un futur producteur direct. La sonde présente ne le fait pas : elle
construit l'ancien `fp::Plan` comme source des crédits et référence. Ses
objets, les nouvelles lignes et la vérification coexistent ; les capacités
du sidecar ne sont donc pas une mesure de mémoire totale du processus.

## Tests et limites de qualification

La porte native parcourt les vrais fronts de quatre familles, deux ordres
d'entrée, K2/3/5/10 et s8/10/12. Elle compare toutes les listes B, tous les
masques, les masses et le vecteur de paires en ordre original ; les mutants
inversant B ou élargissant les masques sont causaux. Entrées vides, classes
saturées, K1 inactif et refus de domaine sont distincts. Les boucles ne
matérialisent le produit complet que dans ces petites fixtures de test.

Les grandes mesures comparent chaque liste B à la formule indépendante et
les masses aux reçus Pool publiés. Elles n'expansent pas E et n'exécutent
ni S2 ni FULL. L'association de chaque ancre A à sa classe est vérifiée à
l'expansion sur les petits cas, pas répétée dans le juge des grandes mesures.
Une contrelecture indépendante n'a identifié aucune divergence mathématique
ou d'ordre dans ce périmètre.

La [capture](../../receipts/q34_ordered_rows_20260927/README.md) porte les
chiffres, succès et éventuels échecs. Les mêmes 15 entrées que les bandes
directes sont demandées : trois trames sans sol, variantes K10/s10/s12 de
00 et uniforme/terrain/amas 8k/16k/32k. Cela ne transforme pas trois trames
de la séquence 08 en plusieurs séquences, ni en nouvelles coupes capteur.

Le résidu E ne diminue pas : seule sa représentation change. Les amas
gardent leur croissance quasi quadratique. La borne en F_A/F_B/W/T ne
prouve pas une borne globale en nombre de sites. Aucun contrat 100 ms ni
gain GPU/FULL n'est revendiqué avant port et mesure de la chaîne entière.

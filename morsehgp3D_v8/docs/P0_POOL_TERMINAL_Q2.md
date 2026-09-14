# Pool terminal q2 : filtrer les gros produits avant leur expansion

14 septembre 2026. Douzième tranche qualifiée localement :
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
implementation_v8_p0 / not_claimed`. GCP non utilisé.

## L'idée et ce qui est réellement raccordé

Le front WSPD livre un rectangle A×B. Jusqu'ici, même lorsque seuls
quelques couples peuvent être utiles, le census doit résoudre un grand
nombre de requêtes. Pool prépare des minorants par extrémité en lisant
les facteurs, pas leur carré. Il élimine ensuite des produits de classes
entiers, et ne développe que les paires encore indécises.

Le raccord s'appuie explicitement sur le prototype de l'auditeur A
publié à fbbecc01. La [proposition et ses preuves](P0_POOL_TERMINAL_RACCORD.md)
conservent les résultats indépendants A/B ; ils ne qualifient pas ce port.
Le nouveau [plan interne](../src/pipeline/q2_node_pool.hpp) et le
[raccord public](../src/pipeline/wspd_q2_census.hpp) ont leurs propres
oracles, reçus et mesures. Aucun fichier moteur v7 n'est importé.

L'argument final `pool_min_factor` vaut zéro par défaut : filtre désactivé.
Une valeur positive sélectionne les rectangles dont le plus grand facteur
atteint cette taille. Le premier essai est 64, pas un optimum présumé.
Ce seuil choisit une méthode : il ne plafonne ni la sortie ni les recherches.
Les rectangles non sélectionnés sont traités intégralement par le mode
de census, d'ancres et de témoins demandé. Les rectangles sélectionnés
utilisent Pool puis le census individuel global de leurs survivantes,
sauf si le filtre ne retire aucune paire : le parcours d'origine est alors
conservé. La préparation reste payée et publiée, pas effacée du bilan.
Les incompatibilités d'options existantes sont toujours refusées avant
le front, même si un seuil faible sélectionnerait tous les rectangles.

## Preuve du filtre et couverture du résidu

Dans chaque facteur, proposer les K+1 sites de projection la plus élevée
vers le centre de la boîte opposée, avec départage par ID original.
Les propositions ne donnent aucun crédit sans certificat géométrique.
Pour chaque ancre, exclure son propre ID et tester les propositions sur
toute la boîte opposée. Un témoin ne compte que si H est strictement positif.

$$H(a,b,z)=(z-a)\cdot(b-z),\qquad D(a,b)=\left|\left\lbrace z\in X:H(a,b,z)>0\right\rbrace\right|\geq h_a(a)+h_b(b).$$

Les témoins de h_a appartiennent à A, ceux de h_b à B ; A et B sont
disjoints. Aucun ID n'est compté deux fois. Leurs crédits saturent à K.
La paire est rejetée si leur somme atteint K. Une somme inférieure
ne prouve pas la validité : le census global doit encore décider.

Regrouper A et B par crédits croissants avec un tri par comptage stable.
Pour la classe A_i, les classes B_j admissibles vérifient j<K−i : leur
union est un seul préfixe du nouvel ordre B. Au plus K bandes A_i×B_prefix
couvrent ainsi exactement les candidates. Les préfixes B se recouvrent,
mais les classes A sont disjointes : aucune paire n'est dupliquée.
Les classes saturées ne produisent aucune bande. Aucune boucle ne parcourt
les paires rejetées pour vérifier individuellement leur appartenance.

Sans cœur extérieur, un rectangle non vide conserve au moins une paire :
prendre une paire de distance minimale entre A et B. Un témoin strict dans
l'un des deux facteurs fournirait une autre paire croisée plus courte,
contradiction. Ses deux crédits sont donc nuls. Un plan entièrement vide
n'est pas une branche atteignable de ce filtre local q2 ; les classes
saturées et les préfixes vides le sont. Le census global peut en revanche
rejeter toutes les survivantes grâce aux sites extérieurs aux facteurs.

## Trois ordres et une seule géométrie globale

Les facteurs sont les plages spatiales certifiées des nœuds du même index.
Le plan ne les fait pas passer pour des plages d'IDs originaux. Il conserve
les rangs spatiaux des ancres et les IDs originaux dans son ordre B privé.
Il emprunte l'index précis et interdit copie, déplacement et affectation.
Ses coordonnées ne sont ni recopiées ni revalidées ; aucun nouvel index
global, arbre B local ou inverse de permutation n'est construit.

Pendant l'expansion, le moteur utilise temporairement cet ordre B pour
les **requêtes individuelles seulement**. Les nœuds globaux ne décrivent
pas les préfixes de ce nouvel ordre. L'ordre des témoins Z ne change jamais.
La vue B est restaurée avant destruction du plan, y compris si un callback
lève une exception. Le rappel peut avoir déjà émis des supports : il n'y a
pas de rollback implicite de ces émissions, comme dans le contrat précédent.

Dans un rectangle effectivement réduit, chaque paire survivante repart
avec compte zéro à la racine de Z global. Un repli sans réduction conserve
au contraire les racines et les continuations du parcours demandé.
**Ne pas précharger h_a+h_b** : le census reverra ces témoins. Les sites
hors A∪B restent dans le domaine de recherche, et toute la coquille est
collectée, y compris les extrémités et les autres sites de frontière.
Les supports différents d'une même boule gardent chacun leur incidence.
Ce flux n'est toujours ni un catalogue canonique ni une tour HGP FULL.

## Travail, temps et identités vérifiables

Noter P la masse livrée par le front, S celle des rectangles sélectionnés,
R leur résidu Pool et C toutes les candidates transmises au census.

$$S=R+R_{\mathrm{Pool}},\qquad C=P-R_{\mathrm{Pool}},\qquad C=C_{\mathrm{accept}}+C_{\mathrm{rejet}}.$$

Les compteurs `input_rectangles`, `input_descriptors` et `anchor_queries`
continuent de décrire **tous** les rectangles du front. Les démarrages de
recherche changent : une racine par paire d'un rectangle effectivement
réduit par Pool, plus les racines
du chemin conservé sur les autres rectangles et les replis sans réduction.
Les compteurs conjoints ne portent que sur ce chemin conservé. Leur partition
terminale vaut donc `census.candidate_pairs - pool_work.pair_roots`,
pas toutes les candidates ni seulement les rectangles non sélectionnés.

En mode conjoint, `count_root_starts=joint_work.root_products+pool_work.pair_roots`.
La phrase du commentaire historique SharedProduct dans le header sur
l'égalité aux seules `root_products` concerne le chemin sans expansion
Pool. La mention générique `selected_*` dans le commentaire de repli
désigne les rectangles/paires sélectionnés, pas `selected_anchors` : ce
dernier ne compte que les ancres effectivement développées après réduction.
`original_selected_anchors` inclut en revanche tous les plans tentés.
Ces précisions sont les identités des tests et des lecteurs de cette révision.

`pool_work` publie les rectangles, masses, F=Σ(|A|+|B|), comparaisons de
sélection, propositions, insertions/décalages, certifications, termes
axiaux, visites de regroupement, bandes et racines individuelles.
Deux lectures de facteurs donnent `factor_read_visits=2F`, deux parcours
de regroupement `grouping_visits=2F`. Le calcul des préfixes visite K+1
classes par plan, pas toutes les ancres. Les candidats développés dans
ce chemin valent exactement `pair_roots=residual_pairs-passthrough_pairs`.
Les trois compteurs `passthrough_rectangles/pairs/anchors` identifient
les plans sans réduction renvoyés au parcours d'origine ; leurs coûts
restent présents dans tous les compteurs de préparation.

Le Pool coûte O(KF), puis O(F+KR_s) pour regrouper les R_s rectangles
sélectionnés. Son stockage est O(|A|+|B|+K) par plan vivant. La sélection
utilise un tampon fixe de propositions ; les déplacements logiques restent
comptés. Cette borne **ne borne ni F, ni C, ni les visites globales** pour
toutes les familles. Les autres tâches et toute la sortie sont supplémentaires.

Le temps englobant paie front, préparation, census, collecte, callbacks et
destructions. `preparation_ms` et `selected_total_ms` sont mesurés seulement
sur les rectangles sélectionnés. Le second inclut le premier et le payload
sélectionné : ces durées ne s'additionnent pas. `count_ms` reste le temps
englobant hors payload, pas un temps isolé de calcul de profondeurs.
`plan_peak_bytes` est le maximum de capacités vectorielles retenues, pas
un pic RSS ni une mesure des piles, objets, buffers et allocations transitoires.

## Objets pour la parallélisation suivante

Les groupes de crédits définissent des plages de tâches partageant le même
plan parent. Ne pas refaire Pool sur A_i×B par worker : cela rescannerait B
et perdrait des propositions du parent. La file devra posséder le contexte
immuable jusqu'à la fin de ses jobs, et chaque worker aura ses buffers.
Le présent emprunt synchrone n'est pas une API asynchrone/GPU. Le filtrage
et son coût sont à qualifier en mono avant ce changement de contrat.

Le premier grain parallèle proposé est un **sous-arbre du front**, pas une
tâche système pour chaque rectangle : reprendre un produit A/B avec son
masque et sa profondeur, puis traiter localement ses petits rectangles
et leur census. Les partitions LL/LR/RR et les deux enfants d'un produit
disjoint donnent une couverture sans doublon. Chaque worker doit posséder
son moteur, ses vues B et ses buffers. Le nombre de workers ou la granularité
ne doit pas modifier les décisions géométriques ni abandonner une tâche.
Les gros retardataires nécessiteront ensuite des jobs d'ancres ou de bandes
partageant leur parent ; ne pas limiter le parallélisme aux seules survivantes
Pool, qui ne représentent plus le poste dominant dans l'audit.

Pour les sondes, un digest/collecteur par worker évite un verrou par support.
Un consommateur sériel devra recevoir des lots possédés avec contre-pression.
Réduire les masses et compteurs par additions vérifiées, les profondeurs
par maximum ; mesurer séparément mémoire simultanée et temps mur. En
particulier `temps_mur - somme(payload_ms_workers)` n'est pas un temps
de comptage valide. Cette architecture reste proposée, non implémentée ici.

L'[audit A publié à d608cc28](../audits/q2_small_roots_20260914/README.md)
confirme qu'un changement Global sur les seules racines singleton ne
montre pas de gain stable sur LiDAR50k : il remplace surtout des descentes
structurelles par des tests géométriques. Le parcours Complement/sibling
est donc conservé dans nos comparaisons. Son cache d'obligations de coquille
tangente est une proposition séparée, sans gain produit ni qualification
héritée ; il ne repousse pas le partage des étapes dominantes.

Après ce port, mesurer front, petits rectangles et sorties restantes :
l'audit A montre qu'ils dominent après le filtrage des gros produits.
Ni une accélération constante ni une croissance favorable sur quelques
tailles ne suffisent à annoncer une borne globale. P0, q3/q4, FULL,
multi-CPU, GPU, contrat de tour 50k et dizaines de millions restent ouverts.

## Qualification de cette révision

Les **49 CTests passent en Release et sous Clang ASan/UBSan**, avec
détection des fuites conservée et 32 commandes enregistrées par qualification.
Les gates dédiées couvrent 2 059 plans et 3 088 appels intégrés, avec
oracles indépendants et comparaison des intérieurs/coquilles entiers.
Les builds `build/v8_pool_terminal_20260914/` et
`build/v8_pool_terminal_sanitize_20260914/` sont désormais épinglés.
Les [reçus et limites de mesure](../receipts/q2_terminal_pool_20260914/README.md)
portent sur cette révision ; aucun résultat du prototype n'est repris
comme un test produit.

80 mesures closes : K5/K10, s8/10/12 à8k ; croissance à16k/32k à s8,
quatre familles et seuils Pool0/64 appariés. À K10/s8, les amas passent
de13,412/47,179/184,306 s à3,589/7,614/19,180 s. Les visites census
font ×2,958 puis ×2,701 au lieu de×4,106/×4,229. À32k, la préparation
ne lit que F=224000 sites dans28 plans ; cela ne borne pas F ailleurs.
Les lectures normales/−O sont identiques et les hashes des sources/binaires
sont stables. Une répétition, hôte partagé : aucun classement fin de s,
aucune distribution de latence, aucun contrat FULL/G4 acquis.

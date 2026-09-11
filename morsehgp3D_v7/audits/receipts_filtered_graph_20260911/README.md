# Graphe filtré : accord conditionnel et réduction aux seules naissances

11 septembre 2026. Contrelecture de **dc5a36ba**, proposition `9aeb6c64…`, constructeur `6763a877…`. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures dans `audits/` uniquement ; GCP non utilisé.

**La proposition du constructeur est correcte sous ses prémisses. On peut aussi éliminer les hubs non naissants avant la forêt couvrante minimale.** Le graphe résultant porte uniquement les naissances, avec les seuils des chemins transférés. Sous régularité, ce sont les minima Gabriel : cette construction répond à la vraie hiérarchie K-NN, sans revenir au faux graphe induit entre minima. Hors régularité, conserver les naissances de composantes et leurs contributions datées, pas imposer une facette Gabriel isolée de cardinal K.

Cette passe apporte une preuve et un modèle indépendant borné. Elle n'implémente aucun backend C++ ou GPU et ne mesure aucun gain. Les qualifications du constructeur vérifiées plus bas gardent leurs propres sources et périmètres.

## 1. Accord sur le graphe de hubs

La [proposition principale](../../docs/GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md) distingue correctement naissance des sommets, activation des arêtes, parents avant lot, contributions et verticales. L'équivalence repose sur un census complet, la suffisance de la fenêtre, le quotient local exact et les terminaux strictement antérieurs. Elle ne prouve pas ces prémisses à leur place.

Le raccord est précis : `prepare_block` déduplique les racines normalisées (source épinglée, lignes 805–809) ; `close_lot` groupe les blocs par anciennes racines communes (878–901), puis installe les ancres après fermeture (921–926). Le quotient du graphe par ses anciennes composantes donne le même biparti blocs/parents. Zéro, un ou plusieurs parents donnent naissance, continuation ou multifusion atomique. Le degré brut ou le nombre de BallId distinctes ne remplace jamais le nombre de racines pré-lot.

La preuve par échange de la forêt couvrante minimale conserve bien les composantes à chaque coupe ouverte ou fermée. Conserver tous les sommets déjà nés, y compris les isolés, et exclure les futurs reste indispensable. Une forêt couvrante quelconque n'a pas cette propriété. Les contributions ne se déduisent pas de ses seules arêtes.

## 2. Réduction constructive aux naissances

Fixer K. Noter A le nombre de sommets du graphe original, points K1 compris, L le nombre de ses sommets sans terminal et R le nombre d'occurrences de représentants/arêtes. Une naissance est ici un sommet sans terminal, distincte d'un semis de cache de population complète.

Pour tout hub B avec au moins un terminal, choisir déterministement un terminal pivot t₀(B). Sa date est strictement inférieure à celle de B. Itérer ces pointeurs aboutit donc à une naissance, notée φ(B) ; poser φ(B)=B aux naissances. À K1, les racines de ces pointeurs sont les points.

Construire le graphe réduit :

1. Garder seulement les L naissances, avec leurs dates originales.
2. Pour chaque occurrence terminale tᵢ(B) autre que l'occurrence pivot, émettre une arête φ(B)–φ(tᵢ(B)), **datée de λ_B**.
3. Garder séparément la correspondance `(K,B,λ_B,φ(B))` et les contributions datées du bloc. Les boucles peuvent être supprimées. Pour des arêtes de mêmes extrémités, conserver la **date minimale exacte**, jamais une occurrence arbitraire ; toutes leurs marques restent disponibles.

**Preuve aux deux côtés d'une coupe.** Si B est actif, le chemin pivot B→φ(B) est actif : ses dates d'arêtes décroissent à partir de λ_B. Toute arête originale active devient soit une contraction pivot, soit une arête réduite active ; ainsi tout chemin original se projette. Réciproquement, relever une arête réduite issue de B par le chemin φ(B)→B, l'arête B–tᵢ(B), puis le chemin tᵢ(B)→φ(tᵢ(B)). Toutes ces arêtes ont une date au plus λ_B. Le relèvement est donc actif à toute coupe qui admet l'arête réduite, ouverte comme fermée. Les naissances ne sont jamais identifiées a priori : les composantes des deux graphes sont en bijection, ainsi que leurs histoires de fusion.

Chaque hub non naissant enlève exactement une occurrence pivot. Avant suppression des boucles/doublons :

$$|V_{\mathrm{red}}|=L,\qquad |E_{\mathrm{red}}|=R-A+L.$$

Une forêt couvrante minimale de ce graphe réduit conserve ensuite L−C arêtes, C étant le nombre de composantes finales. Cette identité ne borne ni le coût du calcul de la forêt, ni celui du flux initial R. La carte φ reste de taille O(A) si elle est conservée pour tous les hubs. Le graphe est plus petit ; la mémoire totale n'est pas automatiquement réduite du même montant.

Les pointeurs forment une forêt orientée strictement vers le passé : un calcul séquentiel coûte O(A), une fois l’ordre des dates disponible, une version parallèle peut doubler les sauts sur tableaux séparés. Pour une hauteur h, cela donne O(log(1+h)) tours et O(A(1+log(1+h))) travail, initialisation comprise, pour cette réalisation simple, pas une mesure de performance. Le tri exact des niveaux peut être partagé entre ordres ; leurs rangs entiers conservent ordre et égalités. Les graphes et les identités restent propres à chaque K.

## 3. Marques et verticales minimales

**Une naissance choisie comme représentant existe parfois bien avant son hub.** L'ancre historique est la composante de φ(B) à la coupe demandée seulement si cette coupe admet B lui-même. Elle ne devient pas une ancre disponible avant λ_B. Cette garde porte sur le côté ouvert/fermé, pas seulement sur un ordre total arbitraire entre égalités.

Pour une contribution du bloc B, garder son niveau λ_B, sa référence de population, son masque et son indicateur d'intérieur. À la lecture d'une coupe qui admet cette contribution, l'affecter à la composante courante de φ(B). Cela conserve la croissance sans fusion et le recouvrement : union par identité de composante, sans union des identités qui partagent des points. On peut supprimer le hub graphique, pas la date de la contribution. L'export physique actuel demande encore un ordre canonique des nœuds, parents et premières références de populations.

**Une référence inférieure par naissance suffit mathématiquement.** Pour une naissance ℓ d'ordre K>1 portée par B, stocker la naissance inférieure φ_{K−1}(B). Sa composante à λ_B fermé est l'image de ℓ. Pour toute composante supérieure active, choisir une de ses feuilles de naissance descendantes, puis normaliser cette référence inférieure à la coupe demandée. La naturalité rend le résultat indépendant de la feuille choisie. Lors d'une fusion, vérifier cette égalité pour tous les parents ; ce n'est pas une permission de retirer la garde existante.

Cette représentation limite les références verticales aux naissances dans un format interne possible ; les images des fusions sont reconstruites, celles des continuations obtenues par lecture historique. Le code actuel ne crée déjà aucun nouveau `lower_nodes` pour une continuation. L'export des références par nœud doit être reconstruit si ce contrat reste demandé. Une seule feuille par arbre **final** ne suffit pas aux anciennes coupes où plusieurs branches étaient encore distinctes.

Pour justifier le bloc inférieur d'une naissance : s'il existait K≤p points intérieurs, leur facette serait stricte ; si p<K<p+q_min, prendre I et K−p points de coquille ne peut contenir de support positif du centre, donc fournit encore une facette stricte. Une naissance impose ainsi K≥p+q_min. Pour K>1, K−1 appartient alors à la fenêtre de B. Cette condition est nécessaire, pas suffisante sur coquille supplémentaire. Un hub de connexion n'a pas toujours de bloc inférieur de même boule ; le triangle aigu du constructeur le montre.

À K=n≥2, pour des positions distinctes et Kmax≥n, la MEB de X fournit l'unique naissance terminale X. Son bloc n−1 existe et son image se lit fermé au même niveau, sans ordre n+1. À n=1, conserver seulement le point K1 à zéro. Les duplicats de positions restent hors domaine de ce constructeur ; cette preuve ne définit pas un nouveau contrat de naissances de rayon zéro.

## 4. Fixtures et falsification indépendante

Le [modèle Python](graph_model.py) utilise un parcours de graphe pour ses partitions et un DSU seulement pour sélectionner la forêt minimale. Il compare le graphe de hubs, deux choix opposés de pivots, leurs graphes réduits et leurs forêts : coupes, couvertures, ancre de chaque hub actif et parents/groupes avant lot. Un lecteur séparé reconstruit les parents des naissances et multifusions **depuis la seule forêt et les dates**, sans consulter les terminaux d'origine.

Fixture minimale de tour : A=(0,0,0), B=(2,0,0), C=(4,0,0). Les diamètres AB/BC ont rayon carré 1 ; AC a rayon carré 4 et B intérieur.

| Ordre | Histoire attendue |
| --- | --- |
| K1 | Trois points à zéro ; **une** fusion ternaire à 1 issue des deux hubs simultanés AB et BC. |
| K2 | Naissances AB et BC à 1, distinctes malgré B partagé ; fusion à 4. |
| K3=n | Naissance ABC à 4. |

Les deux feuilles K2 pointent vers la composante K1 **fermée à 1** ; la feuille K3 pointe vers K2 **fermé à 4**. Autres témoins : triangle aigu de rayon carré 169/36 ; ABCZ, dont la couverture K3 croît au rayon carré 25 après la naissance ABC à 16 ; composantes isolées ; plusieurs terminaux déjà dans un seul parent ; n=1.

Le corpus contient onze fixtures nommées et 256 graphes abstraits déterministes, à dates rationnelles et terminaux stricts, avec doublons et contributions recouvrantes. Sept transformations fautives sont réfutées : sommets futurs à zéro, arêtes antidatées, forêt maximale, doublon réduit à sa date la plus tardive, croissance unaire supprimée, croissance antidatée, identités fusionnées par recouvrement. Deux contre-fixtures distinguent aussi la coupe inférieure ouverte et l'admission prématurée d'une ancre via son représentant ancien. Quatre entrées invalides sont refusées sans `assert`.

Ces graphes aléatoires ne sont pas tous réalisables par des nuages 3D ; ils vérifient le théorème conditionnel sur les données de blocs. Aucun nouveau producteur géométrique, oracle de census ou raccord C++ n'est qualifié par ce modèle. Les décomptes exacts sont dans [review.json](review.json).

## 5. Demandes antérieures maintenant satisfaites

Sept paquets constructeur sont contre-vérifiés par leurs lecteurs normal et `-O`, sans recompilation ni nouvelle exécution du moteur :

| Point | Conclusion bornée |
| --- | --- |
| [Semis après échange](../../receipts/post_exchange_seed_20260911/README.md) et [CMake actif](../../receipts/post_exchange_active_cmake_20260911/README.md) | Raccourci intégré, header `6763a877…`, trois mutants causaux ; anciennes survies de juge conservées. Les 24 CTests passent dans le reçu. |
| [Triplet mono](../../receipts/post_exchange_scale_20260911/README.md) | 237 557 / 501 258 / 1 045 620 MEB supplémentaires évitées à 8k/16k/32k, mêmes sorties ; pas un speedup apparié aux anciens temps statique4. |
| [Vrai census→tour](../../receipts/full_t2_census_tower_20260911/README.md), [métadonnées](../../receipts/full_t2_metadata_20260911/README.md), [nouveau header](../../receipts/full_t2_post_exchange_20260911/README.md) | Le code du juge appelle réellement génération→préfiltre→census ; son inventaire rationnel ne fournit pas le catalogue au constructeur. T2 actif : 54 tours O2/SAN, 540 ordres par build, K9/K10 compris, 120 hits après échange. Les deux mutants de métadonnées restent rattachés à leurs octets c03. |
| [Premier raccord incrémental](../../receipts/incremental_full_trial_20260911/README.md) | Essai réalisé, exports et refus transactionnels qualifiés, puis variante non retenue : à n800, +366 021 allocations et +17 340 480 octets retenus. La demande d'essai est close ; inutile d'exiger son intégration comme optimisation. |

Ces résultats ne qualifient pas le nouveau graphe. Ils ferment des demandes précédentes sans transférer un résultat hôte aux prototypes GPU ou un oracle borné à la généralité WSPD. Les nouvelles primitives GPU restent hors de cette passe.

## 6. Prochain jalon concret

Un futur prototype peut émettre les blocs/terminaux du vrai producteur sur une petite tour, calculer φ, puis confronter le graphe réduit et sa forêt au calendrier actuel et au juge rationnel T2. Garder naissances, contributions, ancres et verticales dans la comparaison ; exiger l'atomicité aux niveaux égaux. Le modèle présent fournit les négatifs minimaux. Mesurer ensuite A, L, R, arêtes après boucles/doublons, capacité de φ et des marques, travail de tri/MSF et coût d'export. La sortie explicite peut toujours être quadratique ; cette réduction évite des hubs graphiques, pas cette borne.

```bash
python3 -B morsehgp3D_v7/audits/receipts_filtered_graph_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_filtered_graph_20260911/verify.py
```

Le lecteur contrôle le sceau du paquet, les sources historiques épinglées dans Git et les lecteurs de reçus, puis recalcule le modèle. Il ne lance ni build, ni moteur, ni GCP. Les variantes de qualification globale D–Q restent inchangées.

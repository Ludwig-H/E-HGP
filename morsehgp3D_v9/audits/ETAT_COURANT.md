# État courant des audits v9

23 septembre 2026. Code jugé : **`0b29b6c3`** (session G4 sur le paquet
`e28296bb`, noyau MEB à `ad2d0ebb`, sonde v3 et atlas saturant à
`e6405952`, défaut FULL statique à `0b29b6c3`). Cadre :
`exploration_v9_hors_registre`,
`reference_cpu`, `quantized_u18_input_only`, `not_claimed`. Ce fichier est le
verdict mutable du dossier ; les notes datées conservent les démonstrations et
références. Les auditeurs écrivent dans `audits/` et communiquent au
constructeur uniquement des constats utiles.

## Verdict sur le premier moteur

La chaîne générateur v8 porté → catalogue de BallKeys recoupées → tour FULL
v7 portée existe. Elle recalcule clé, niveau, intérieur et coquille de chaque
boule **émise**, vérifie `q_min` pour les coquilles étendues et refuse
transactionnellement une coquille de plus de 12 sites. La forme q4 est
indépendante de l'orientation du support. La contrelecture n'a pas trouvé de
défaut concret sur ces chemins ; voir le [contre-audit A du
moteur](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md) et la [lecture B de
FULL](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md).

`e6405952` active par défaut dans la chaîne le certificat terminal q4
`inside≥K−1` et filtre le tri des niveaux FULL en arrondi au plus proche,
avec repli exact pour les niveaux proches. La [contrelecture
B](CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI_20260923.md) ne trouve pas de
défaut d'exactitude dans ces deux chemins ; le commentaire de
`ChainOptions` qui dit encore « désactivée par défaut » est périmé.
L'auteur annonce sur 08/000100 K5/W8 un temps q3/q4 97,7→72,1 s et
des digests K5/K10 inchangés, sans reçu apparié versionné ni temps G4.
Le ledger v9 contient le travail physique total de partition, préfixes
saturés inclus **une fois**, mais n'exporte pas les compteurs spécifiques
`certificates`, `certificates_with_unvisited_sites`,
`unvisited_site_mass` et `discarded_frontier_ids` : publier ces nombres
pour expliquer le gain, en gardant `saturation_work.prefixes` comme
**sous-ensemble** de `work.partition`, jamais comme terme additionnel.
`0b29b6c3` choisit par défaut le résolveur FULL statique avec W fils si
W>1 ; le test de chaîne compare déjà les payloads exacts W1/W4 sur de
petits nuages. La [contrelecture B](CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI_20260923.md)
montre que le plan G4 passe `--static` explicitement et ne mesure donc pas
ce nouveau défaut. Vérifier en porte le nombre **effectif** publié, le
défaut contre `--static=0`, et refuser toute valeur négative autre que
la sentinelle `-1` ; aucun nouveau chrono G4 n'est acquis.

Un rejeu indépendant local de `d2700314` passe **20/20 CTests** sans saut ;
B retrouve ces 20 portes en Release et sous Clang ASan/UBSan. Le commit
`ba762036` ajoute une porte arithmétique u18 et un CTest du mutant de census
(22 portes annoncées par le développeur). `e28296bb` annonce 28 CTests,
dont trois mutants T2 et trois refus CLI ; ces nouvelles portes n'ont pas
été rejouées indépendamment ici. La cible arithmétique reste compilée en
mode produit : son mutant `level-trunc-hi` est inactif, donc sa valeur
d'oracle numérique est distincte d'une preuve de mutant tué.

Deux coquilles exactes u13 (une mixte, une q4 pure) obtiennent le refus
`chain_shell_above_12`, sans tour ni catalogue partiel publiés. Une fixture
q4 u12 exerce l'appel public `run_tower=true` : six configurations
W1/W4 × s8/s10/s12 rendent le même digest et la fusion attendue de **trois
parents à K10**. Ces contrôles ciblés n'ont pas encore de reçu versionné.
Les trois mutants T2 (`assignment`, `open`, `adjacency`) sont désormais
inscrits à CTest avec causes exactes. La sonde refuse K hors 1..10 avant
rétrécissement et limite `--grid=` à un alphabet sûr ; les deux défauts
reproduits dans le [contre-audit B](CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md)
sont corrigés sur ce chemin. Le libellé `--grid=1mm` ne certifie toujours
pas la préparation : lier un manifeste d'entrée au pas réel.

Le juge T2 compare l'inventaire exact des boules et la tour Γ sur de petits
nuages (`n≤14`) ; depuis `e28296bb`, il compare aussi la tour publiée par
`run_tower_chain(..., run_tower=true)` en W1/W4. Il ne démontre pas
l'absence d'une BallKey complètement omise sur une grande trame.
`run_tower=false` rend aussi
`complete_relative` avec zéro ordre : tout lecteur de contrat doit exiger
`run_tower=true`, les ordres K=1..Kmax et un objet FULL cohérent.

Le [premier reçu FULL](../receipts/first_tower_20260922/README.md)
contient six exécutions locales, une par trame 08/000000, 000100, 000200
sans sol à 1 mm et par K5/K10, s8/W8. Mur K5 : **143/132/264 s** ;
K10 : **523/381/802 s**. q3/q4 prend 73–93 % et l'aval FULL 94–130 s
à K10. Les 30 hashes du manifeste et les trois entrées concordent ;
les comptes des cinq premiers ordres de K5 et K10 sont identiques pour
chaque trame. Cela certifie une base de coût **relative aux clés émises**,
pas la complétude exhaustive sur grande trame, ni l'égalité des tours par
ordre au seul vu des comptes. La [contrelecture B du
reçu](CONTRE_AUDIT_B_PREMIERE_CAMPAGNE_20260922.md) et l'[actualisation
A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md) détaillent les limites du
lecteur et les corrections du README. La chaîne ne publie que quatre
compteurs du registre q3/q4 pourtant disponible et matérialise deux
capacités complètes de présentations lors de la fusion.
Le [calcul de résidence B](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md)
identifie aussi un cache temporel optionnel de 48·nextpow2(16n) octets
(24 Gio à 30 M sites), sa remise à zéro par ordre, et au moins 216n octets
de sortie K1. Ce sont des planchers ou capacités logiques, pas un RSS
mesuré ; ils imposent une résidence de travail maîtrisée et une
représentation de sortie adaptée pour les dizaines de millions de sites.

La [première session G4](../receipts/g4_tower_r1_20260922/README.md) est
réelle, clôturée et **CPU seulement** : huit cas complets du paquet
`e28296bb`, sur trois trames sans sol de la séquence 08, s8, une exécution
par cas. À W48, la tour K1..5 prend **18,81 / 15,05 / 29,25 s** et K1..10
**111,68 / 82,31 / 125,44 s** ; 000000/K10 descend à **70,00 s** en FULL
statique W48. Les six cas communs ont `generator`, `catalogue`,
`tower_work` et `orders` **statistiques** exactement égaux aux reçus locaux,
pas seulement le condensé ; les clés individuelles ne sont pas exposées.
Sous l'hypothèse du FULL non statique en un fil, le reste de
la chaîne consomme en moyenne **42,6–44,6 des 48 CPU logiques** : son
coût tient davantage au travail payé qu'à un manque évident de threads.
La voie FULL statique modifie aussi les compteurs MEB/cache : son gain
75,90→34,14 s n'est pas un facteur de parallélisme pur.
Ce reçu n'est ni GPU, ni brut, ni multi-séquence, ni s10/s12 ;
ces tailles voisines ne prouvent aucune pente sous-quadratique. Voir la
[lecture des mesures](CONTRE_AUDIT_B_PREMIER_G4_20260922.md) et la
[contrelecture du protocole](CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md).

La [session G4 R2](CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md) sur le
paquet `0b29b6c3` est close avec `worker_failed/probe_failed` : les
treize sondes invitées ont calculé et rendu code 0, mais le worker a
refusé les treize JSON sur `tower_work`. L'arrêt ciblé est certifié ;
**aucun reçu G4 accepté** n'en découle. Les médianes W48 des chronos
bruts de chaîne K5 sont 13,252/10,455/19,946 s et K10
50,071/37,317/64,289 s pour 08/000000, 000100, 000200.
Sur 000000/K10, le registre brut annonce 17,947 milliards de tests
ponctuels d'atlas q4 et 12,392 milliards d'insertions d'IDs de nœuds
frontaliers : le travail total, et pas seulement le nombre de workers, doit
changer. Ces diagnostics exploratoires ne qualifient ni 1 s ni GPU.

Le chiffre v8 de 104,63 s portait sur le seul flux q3/q4 en mode digest :
aucune régression ni accélération v9 ne se déduit de cette comparaison non
appariée.

## Priorités de preuve et d'optimisation

1. **Rendre le reçu rejouable et explicatif** : lier les SHA d'entrée,
   le masque/grille, le binaire et toutes les options à chaque ligne,
   vérifier les préfixes K5/K10 par digest d'ordre, et créer le marqueur
   de campagne seulement après six succès. Les trois entrées sans sol
   sont versionnées dans le [reçu v8 `lidar_ground_20260921`](../../morsehgp3D_v8/receipts/lidar_ground_20260921/README.md).
   Publier les registres déjà calculés : tests/copies de partition d'atlas, rejets q3,
   travail et attente par worker, `merge`, census, quotient et résolveur
   FULL. Ne pas confondre les sommes de temps worker avec le temps mur.
   Le reçu G4 R1 satisfait cette porte pour son paquet **ancien**
   `e28296bb` : huit cas achevés, sources et entrées recoupées, arrêt ciblé
   certifié. Il ne qualifie pas les nouveaux défauts. **Bloquant pour le
   prochain G4** : à `0b29b6c3`, le worker accepte l'étiquette v3 et
   l'option de saturation, mais exige encore des entiers pour toutes les
   valeurs de `tower_work`. La vraie sonde ajoute `meb_accounting` (chaîne) et
   `meb_supports_by_size` (tableau) ; `validate_probe` refuse
   `probe counters tower_work` après calcul. Les faux producteurs des
   selftests v2/v3 omettent ces champs. Juger une **vraie petite
   sortie** de la sonde, complète et refusée, avant une autre session
   facturée. Le protocole v3 ne lie pas non plus la valeur du booléen
   `atlas_saturate_deep` au plan. Le lecteur de reçu doit aussi refuser toute commande tuée
   dont le groupe de processus n'est pas fermé, même en campagne `partial`.
   Le [contre-test de provenance](CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md)
   montre qu'un paquet muté peut annoncer un commit inexistant et être
   accepté par le contrôleur, et qu'une provenance différente dans le
   reçu invité passe encore la validation finale. Recertifier les blobs
   Git et l'identité du reçu avant la prochaine dépense G4 ; R1 a été
   vérifié indépendamment et n'en est pas invalidé. Le lecteur accepte
   aussi des sous-temps incohérents ; ajouter un contrôle de durée externe
   avant de juger un objectif de 1 s.
2. **Portes causales et entrée** : rejouer les 28 portes de `e28296bb`
   indépendamment ; les portes MEB et FULL ciblées du nouveau noyau passent
   déjà en Release et sous ASan/UBSan, mais pas une campagne appariée LiDAR.
   Comparer la restriction sémantique des ordres K1..5 de K10 à la tour K5
   sur les mêmes octets, après égalité des catalogues actifs bas-rang ;
   rendre causal le
   mutant arithmétique u18, ajouter égalités de cellules, générateur porté
   et TSan. Les reçus v8
   R2 de la reprise u18 restaient `failed`
   à cause du lecteur JUnit : la provenance v9 épingle `3f0d188f`, mais
   aucune qualification n'est héritée automatiquement.
3. **Verrou q3/q4 mesuré** : préserver les miniballes k-Gabriel locales,
   le propriétaire et les certificats exacts. L'[audit q4
   A](Q4_STRUCTURE_ET_BORNES.md) propose un test d'absence de graine aiguë
   possédée **avant** le cover ; sans graine, q3 et q4 sont vides sur l'arête.
   Une borne locale de `m(K−2)` centres q4 peu profonds pour `m` droites
   distinctes motive un catalogue de centres. Sur les `s` droites de
   graines aiguës, une sélection top/bottom de racines exactes garde au
   plus `2(K−2−p_λ)` événements par droite en `O(sKm)` après regroupement ;
   un déterminant factorisé tient en i128 sous les bornes u18. Pour `s≈m`,
   deux familles de niveaux peu profonds proposent une sélection locale
   `O(mK polylog m)` sur modèle de comparaisons exactes. Le [contre-audit
   B](CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md) propose une
   perturbation sortante qui préserve les strates dégénérées ; l'[oracle
   rationnel A](check_q4_outward_levels_20260922.py) retrouve les 5 926
   centres peu profonds de 2 004 arrangements dégénérés après rabattement
   (profondeurs 0..2). B a reproduit **1 250 cas supplémentaires** à
   profondeurs 3..7, couvrant localement le seuil K10 ; aucun constructeur
   de niveaux ni moteur q4 n'est jugé par ces scripts.
   Ni le port symbolique, ni son coût réel, ni le census ne sont acquis.
   Plus grave pour
   la trame : le reçu 1 mm compte **2,779 milliards d'incidences
   site–cover cumulées sur les arêtes**, déjà davantage que `n²` pour
   `n=39 885`. Un parcours complet de chaque cover serait donc déjà plus
   coûteux que `n²` sur ce cas ; il faut partager ou élider ces covers, sans
   confondre cette masse avec `Σh` des droites q4. Le compteur
   `cover_sites` est une **population logique** additionnée sur des nœuds
   certifiés, pas autant de lectures de sites dans le moteur actuel ; le
   premier cover, sa décomposition et les IDs copiés ont leurs propres
   compteurs : **440,194 M** visites pour le construire, puis **315,737 M**
   pour le redécomposer dans l'atlas q4, sur la ligne 1 mm. Census global et test
   `centre∈conv(coquille)` restent obligatoires ; le catalogue ne remplace
   pas automatiquement les présentations positives. La ligne v8 1 mm
   compte **171 444 arêtes q3 seules et 16,12 M census** que la réutilisation
   d'atlas q4 ne touche pas. Dans le raccord v9 actuel, `GlobalBoxes`
   recense ces q3 dans l'index global : la construction du cover sur les
   arêtes q3 seules n'est pas utilisée pour ce census et mérite une
   ablation exacte avant toute optimisation plus lourde du cover. L'[oracle
   q3 partagé](check_q3_shared_u18_20260922.py)
   vérifie la boîte rationnelle u18, un débordement i128 évité par
   annulation algébrique et une fixture où avancer le curseur Z à travers
   une feuille ambiguë perd un intérieur. La nouvelle fixture exacte à
   quatre graines atteint réellement EOF avec `lower≥0`, quatre supports
   valides et coquilles complètes après saut structurel des endpoints ;
   le futur port u18 doit exercer ce relais avec `relay_sites=2`. La
   [note q3](Q3_STRUCTURE_ET_BORNES.md) décrit le ticket possédé
   `(X,compte,curseur Z)` : une réponse GPU hors ordre ne valide pas un
   préfixe DFS continu. Le relais produit reste à
   qualifier. Le [contre-audit
   B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) rappelle que la suppression
   d'une cellule q4 peut aussi enlever un certificat de rejet q3. Sa
   fixture entière prouve même qu'une q4 admise peut survivre quand
   **toutes** ses faces q3 sont rejetées : les graines à parcourir ne
   peuvent pas être limitées aux q3 finalement émises. Un [certificat de
   cover par bloc d'arêtes survivantes](CONTRAT_COUTS_ET_PARALLELISATION.md)
   partage le prédicat exact sur `E×Z`. Réduire les **vrais** extrema des
   arêtes survivantes ne relâche jamais les bornes A×B, mais peut donner
   **exactement les mêmes** bornes ; l'[oracle
   entier](check_cover_batch_u18_20260922.py) passe 1 000 familles et
   deux cas discriminants. Le [contre-audit B](CONTRE_AUDIT_B_COVER_BATCH_20260923.md)
   avait relevé l'absence de test de subdivision `E×Z` : l'oracle couvre
   maintenant exactement 1 000 petites familles survivantes. Il ne teste
   toujours ni le raccord des ranges, ni le coût de matérialisation/réemploi
   des extrema E. C'est une piste secondaire pour les
   440 millions de visites de cover, à
   mesurer après le filtre de paire, sans lui attribuer le coût dominant
   de l'atlas. Un [certificat de domination par
   blocs](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md) peut écarter des
   formes q4 avant l'atlas pour plusieurs arêtes via des gardes plus proches
   sur toute une cellule de centres ; l'[oracle
   entier](check_q4_block_dominance_20260923.py) passe 1 200 boîtes.
   À K10, huit gardes préservent le flux q4 courant, dix sont nécessaires
   à une garantie autonome pour tout `q_min≥2`. **Le seuil q4 ne doit pas
   élaguer l'atlas partagé q3** : une fixture K5 perdrait alors le quatrième
   intérieur qui rejette une boule q3. Une [ablation plus locale](SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md)
   peut saturer à `K−2` l'atlas des arêtes **q4 seules**, sans changer le
   seuil `K−1` utile aux rejets q3 des arêtes mixtes. Une fixture u18 K5
   sépare exactement les seuils ; le reçu 1 mm contient 326 970 arêtes
   q4 seules, mais pas leur coût distinct. Mesurer les compteurs par masque
   et l'identité FULL avant de prioriser le port. Ni gain LiDAR ni borne
   globale acquis.
4. **Aval FULL, grandes coquilles et échelle** : les 12,0 M appels MEB
   de 000000/K10 font 1,065 milliard de tests de puissance ; un test
   exact de la paire la plus éloignée peut éliminer toutes les autres
   paires q2 dans chaque appel, sous la preuve détaillée de l'[actualisation
   A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md). Le port `ad2d0ebb` a été
   [contrelu par B](CONTRE_AUDIT_B_ANCHOR_MEB_DIAMETRE_20260922.md) ; le
   noyau nominal et son mutant causal passent indépendamment sous Release
   et Clang ASan/UBSan, ainsi que trois portes FULL de petits nuages. Le
   contrat comptable et la cause exacte de mutation sont corrigés à
   `5ab4326c`. Le gain local annoncé 130→109 s n'a pas encore de reçu
   apparié versionné ; l'amélioration G4 du MEB n'est pas mesurée.
   La [découpe FULL par K](PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md)
   rend les BallIds géométriques pré-calculables sous fenêtre d'octets,
   tandis que la fermeture des lots reste chronologique. Les 34,14 s de
   tour statique G4 ne sont pas assez ventilées pour prédire un gain. La
   [contrelecture B](CONTRE_AUDIT_B_PREFETCH_FULL_20260923.md) exige un
   contexte possédé par K, l'annulation/jointure des jobs et distingue le
   scénario naïf de mémoire `60R` du plancher `4R` des cibles seules.
   Une [piste exacte pour les intrus](INTRUS_FULL_PREFIXE_EXACT_20260923.md)
   réutilise, par BallKey, un préfixe complet d'intérieurs Morton ; son
   [oracle combinatoire](check_full_intruder_prefix_20260923.py) passe
   1 336 782 paires de requêtes. Les répétitions par clé et par worker
   manquent encore au reçu G4 : mesurer avant de réserver un cache. Les
   clés déjà cataloguées peuvent lire leur liste globale d'intérieurs
   aux ordres bas ; cela ne retire pas les intrus sur clés absentes à K10.
   Instrumenter les tailles de supports et le temps avant de promettre un
   gain. La [note B](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
   propose un quotient local compact, [contrelu par B sur sept petites
   coquilles](CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md), tandis que l'[oracle entier
   A](check_qmin_planes_u18_20260922.py) donne des fixtures u13/u17 et les
   largeurs des prédicats. Le port doit traiter ensemble quotient,
   représentants, parents, contributions et masque de lecture ; d'ici là,
   maintenir le refus exact. Au-delà, mesurer le volume de BallKeys,
   présentations, tableaux et lectures FULL sur les régimes LiDAR, sans
   transférer les chiffres u16 historiques.

Le jalon temporel v9 commence par le **sans-sol u18/1 mm**. Le contrat
principal antérieur sur trames **brutes entières** et le profil float32
original ne sont pas effacés par ce jalon ; les trames 08/000000, 000100,
000200 viennent toutes d'une seule séquence. Une mesure FULL CPU G4
**relative au catalogue émis** est acquise ; aucun contrat GPU,
sous-quadratique global, K10 <1 s ou K5 <1 s n'est acquis.

La [synthèse A d'architecture](AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md),
les notes [q3](Q3_STRUCTURE_ET_BORNES.md), [q4](Q4_STRUCTURE_ET_BORNES.md),
[coûts/parallélisme](CONTRAT_COUTS_ET_PARALLELISATION.md) et la
[contrelecture B des cellules/catalogue](CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md)
restent les références de conception. Les objections historiques v8, les
reçus et leurs limites sont dans le [contre-audit A des
mesures](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md). Verdict public :
`not_claimed`.

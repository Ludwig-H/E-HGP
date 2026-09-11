# Graphe filtré des boules : proposition pour paralléliser les fusions

11 septembre 2026. Proposition d'architecture argumentée, relue indépendamment à `90ee69ee` : accord conditionnel et réduction supplémentaire aux naissances. Aucun raccord productif/GPU ni claim de performance. Les [objets parallèles et premiers prototypes C++ privés](OBJETS_PARALLELES_TOUR_20260911.md) prolongent maintenant cette proposition, sans lui attribuer une qualification géométrique. Cadre : phase=exploration_v7_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=audit_independant_math_and_architecture, public_status=not_claimed.

## 1. Objet et prémisses

L'objectif est de remplacer la dépendance temporelle du calendrier d'ancres par un graphe statique **filtré**, après résolution géométrique des représentants. Il ne s'agit ni de matérialiser Gamma, ni de supprimer les niveaux exacts utiles.

On suppose un census complet et cohérent, la fenêtre de boules correcte, le quotient local de coquille correct, et une résolution exacte de chaque représentant strict vers une boule terminale du même ordre, strictement antérieure au bloc consommateur. Ce sont les obligations du programme actuel, pas des résultats apportés par cette note. L'équivalence argumentée ci-dessous est d'abord avec ce programme ; son transfert à HGP dépend de ces prémisses géométriques.

Pour une boule B, noter I son intérieur, U sa coquille, p=|I|, u=|U|, q_min la cardinalité minimale d'un support positif et λ_B son niveau exact. Les blocs programmés sont les couples (K,B) satisfaisant :

$$\max(1,p+q_{\min}-1)\leq K\leq\min(K_{\max},n,p+u).$$

À chaque ordre K, définir un graphe non orienté G_K :

- Un sommet h[K,B] par bloc programmé, muni de la date de naissance λ_B. Ce sommet est un **hub de calcul**, pas nécessairement une feuille ou un nœud public.
- Pour chaque représentant d'une composante stricte locale du bloc B, une arête entre h[K,B] et h[K,T], où T est sa terminale géométrique exacte. Sa date est λ_B ; on exige λ_T<λ_B et la présence du bloc (K,T). Plusieurs représentants peuvent donner la même arête : les doublons n'affectent pas les composantes.
- À K1, ajouter les sommets-points, nés à zéro. Les terminales des représentants singletons sont ces points, pas des BallId de rayon positif. Les arêtes correspondantes naissent au niveau du bloc consommateur.

À la coupe fermée t, retenir uniquement les sommets de date ≤t et les arêtes de date ≤t ; à la coupe ouverte, remplacer les deux inégalités par <t. Une arête ne précède jamais ses extrémités. Les hubs futurs ne sont donc jamais des composantes ou feuilles précoces.

Conserver en parallèle, pour chaque bloc, son identité géométrique et son éventuelle contribution datée : référence immuable de population, masque de coquille et inclusion de l'intérieur. Les identités de composantes ne sont jamais des ensembles de PointId.

## 2. Équivalence conditionnelle par induction sur les niveaux

À K1, avant zéro il n'existe aucun sommet ; après fermeture de zéro, chaque point est une composante distincte, comme dans le programme. Aux ordres supérieurs, les deux constructions commencent vides.

Supposer les composantes du graphe correctement identifiées aux racines du programme avant un niveau λ. Tous les terminaux des blocs de ce niveau sont déjà présents strictement avant λ. Par hypothèse sur la résolution et par induction, la composante de chaque terminal est exactement la racine pré-lot que le programme obtient en normalisant son ancre.

Contracter provisoirement chaque ancienne composante du graphe en un sommet. Ajouter simultanément les hubs de niveau λ et leurs arêtes donne le graphe biparti « blocs nouveaux ↔ parents pré-lot ». Deux blocs sont dans le même groupe exactement quand ils sont reliés par une chaîne de parents pré-lot communs. C'est la relation de groupement utilisée par close_lot, y compris lorsque plusieurs BallId terminales distinctes appartiennent déjà à une même composante globale.

Pour chaque groupe, compter les **anciennes composantes distinctes**, non les arêtes ou les BallId :

- Zéro parent : naissance.
- Un parent : continuation de son identité, sans nouveau nœud topologique.
- Au moins deux parents : une multifusion atomique portant exactement ces parents.

Par construction, aucune arête ne vise un hub du lot courant : un groupe sans parent contient donc un seul nouveau hub isolé. Le producteur admis exige une contribution de naissance pour ce bloc. Le graphe conserve cette information ; cet argument ne remplace pas la preuve géométrique du quotient local.

Les mêmes groupes reçoivent les mêmes contributions datées. Après fermeture entière du lot, chaque hub représente son groupe fermé : il réalise l'ancre A[K,B]. L'induction établit donc les mêmes composantes aux coupes ouvertes et fermées, les mêmes parents, ainsi que les mêmes couvertures après union des contributions. Aux niveaux sans bloc, les deux constructions restent inchangées, sous la prémisse de suffisance de la fenêtre.

La forêt publique est l'histoire de ces composantes après suppression des continuations. Il ne faut pas simplement contracter le graphe final en oubliant les dates : cette opération détruirait l'histoire. Une fusion binaire arbitraire entre arêtes de même niveau doit être contractée en la multifusion atomique correspondante, sans parent artificiel de durée nulle.

## 3. Ce que le graphe nu ne restitue pas

### Contributions et croissance : ABCZ

Prendre A=(1,8,0), B=(5,10,0), C=(9,8,0), Z=(5,0,0). Leur cercle commun a centre (5,5,0), rayon carré 25 et intérieur vide. À K3, ABC est la seule facette stricte locale ; sa MEB a rayon carré 16. Au niveau 25, le bloc a un seul parent mais apporte Z. La topologie H0 ne change pas, la couverture oui.

Supprimer le hub silencieux **et sa contribution** conserve les composantes mais perd Z. Réaffecter sa contribution sans date à la feuille ferait au contraire apparaître Z trop tôt. Il faut rattacher la contribution au segment après lot, garder la date 25, puis normaliser ce segment à la coupe demandée. La racine finale ne remplace pas cette normalisation historique. Les contributions peuvent être redondantes ; leur lecture est une union par composante, jamais une somme de cardinaux ni un dédoublonnage entre composantes.

### Verticales : même boule seulement aux naissances

Un lien h[K,B]→h[K−1,B] n'existe pas pour tous les hubs. Pour le triangle aigu (0,0,0), (4,0,0), (2,3,0), la boule circonscrite B a p=0, q_min=u=3 et rayon carré 169/36. Son hub est programmé à K2 (connexion) et K3 (naissance), pas à K1. Une règle qui exigerait son hub inférieur pour la fusion K2 serait donc invalide.

Pour une **naissance** à K>1, l'absence de facette stricte impose K≥p+q_min ; le bloc (K−1,B) existe alors. L'image verticale est sa composante **après fermeture** du niveau λ_B dans l'ordre inférieur. Pour une continuation ou une multifusion, transporter l'image d'un parent, normalisée dans l'histoire inférieure à λ_B fermé, et vérifier que tous les parents ont la même image. L'ancre inférieure de naissance et la naturalité sont donc des données/règles supplémentaires au graphe horizontal nu.

### Identités, plateaux et encodage

Deux composantes peuvent partager des points sans être identiques. Il faut conserver la filiation de leurs identités et la correspondance hub→segment historique, y compris pour les hubs sans effet public.

L'équivalence mathématique des forêts n'impose pas les mêmes numéros de nœuds. Un export physiquement identique exige une convention déterministe supplémentaire : ordre exact des niveaux, ordre stable des blocs dans chaque niveau, ordre canonique des groupes et parents, et ordre des premières références de populations/contributions. Une implémentation parallèle doit reproduire cette convention ou déclarer une autre canonisation comparée par bijection explicite.

## 4. Une forêt couvrante minimale préserve-t-elle les coupes ?

Oui, pour le graphe défini ci-dessus, mais elle ne dispense pas des dates de sommets, des contributions ou de la reconstruction atomique de l'histoire. Voici l'argument, sans recours à un résultat externe.

Considérer une forêt F couvrant chacune des composantes finales de G_K et minimisant la somme des dates exactes de ses arêtes. Les niveaux sont des rationnels comparables exactement. Pour une arête e du graphe non retenue, ses extrémités sont reliées dans F par un chemin unique P. Si P contenait une arête f de date strictement supérieure à celle de e, ajouter e puis retirer f du cycle obtenu donnerait une forêt couvrante de somme strictement plus petite : contradiction. Toutes les arêtes de P ont donc une date ≤celle de e.

À une coupe fermée t, toute arête active e de G_K est ainsi remplaçable par un chemin de F dont toutes les arêtes sont actives. Tous les sommets de ce chemin sont également actifs, car chaque date de sommet est ≤celle de ses arêtes incidentes. Une chaîne du graphe filtré donne donc une chaîne de la forêt filtrée. La réciproque vient de F⊆G_K : les partitions en composantes sont égales. Le même argument vaut à la coupe ouverte puisque date(e)<t implique date(f)≤date(e)<t.

Les sommets isolés déjà nés restent présents. Les sommets futurs sont explicitement exclus : compter leurs singletons dans une forêt finale stockée dès le départ donnerait un H0 incorrect avant leur naissance. Les choix entre arêtes de poids égaux n'affectent pas cette preuve, mais une séquence binaire de leurs unions n'est pas une sortie FULL atomique.

Cette réduction ne conserve pas toutes les adjacences du graphe initial, seulement ses composantes à toutes les coupes. La correspondance des hubs conservés dans la représentation aux segments d'histoire doit rester disponible pour les contributions et les verticales.

## 5. Coût et questions de qualification

Noter A le nombre total de cellules d'ancre, en comptant ici les n singletons K1 en plus des couples programmés (K,B), et R le nombre total de représentants émis. La représentation avant dédoublonnage comporte A sommets et au plus R arêtes : stockage O(A+R), plus les populations partagées et les données de sortie. Si A désigne seulement les couples de boules, écrire plutôt O(n+A+R). Hors Kmax fixé, ne pas remplacer A par le seul nombre de boules géométriques.

Cette borne compte des enregistrements, pas des secondes : elle ne borne ni le coût des descentes géométriques, ni celui du tri/comparaisons exactes, de la construction des composantes ou d'une forêt minimale, ni l'export des couvertures. Elle n'établit pas une sous-quadraticité en n et ne démontre aucun gain CPU/GPU.

Questions proposées à l'auditeur : l'induction couvre-t-elle bien tout bloc admis, notamment les coquilles supplémentaires et K=n ? Le transport vertical aux naissances puis par naturalité suffit-il avec ce nouveau support d'ancres ? Quelle représentation minimale conserve les correspondances datées sans réintroduire une résidence excessive ? Une future qualification devra confronter graphe et calendrier aux mêmes oracles/coupes, garder les mutants de dates, plateaux, contributions et identités, puis mesurer séparément les coûts. Rien de cela n'a été exécuté dans cette note.

Sources locales : [constructeur](../src/forest/full_ball_tower.hpp) `6763a877…`, [ancres de boules](../audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md), [contre-fixtures de couverture](../audits/receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md), [contrat courant](../audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md). Leurs qualifications antérieures gardent leur portée ; cette proposition n'en hérite aucune. Le constructeur n'est pas modifié par cette note. GCP non utilisé.

Une piste d'implémentation ultérieure serait de trier et regrouper exactement les niveaux de boules une fois, puis d'utiliser leurs rangs entiers comme dates du graphe. Cela conserve l'ordre et les égalités sans comparer des rationnels à chaque arête sur GPU. Le tri certifié, le coût de stockage des rangs, la reconstruction de l'histoire et l'export resteraient à qualifier et à chronométrer. Aucun gain n'est mesuré ici.

## 6. Retour de l'auditeur : éliminer les hubs non natifs

La [preuve et son modèle indépendant](../audits/receipts_filtered_graph_20260911/README.md), publiés à `90ee69ee`, confirment l'induction sous ses prémisses et donnent une réduction avant même le calcul de forêt couvrante. Pour chaque bloc non natif, choisir un terminal pivot déterministe, strictement antérieur. En répétant ces pivots, on atteint une naissance ; noter cette naissance φ(B). À K1, les points jouent le rôle de naissances.

Ne conserver comme sommets du graphe que ces naissances. Pour chaque terminal non pivot T du bloc B, émettre une arête φ(B)–φ(T), datée **λ_B**. Supprimer les boucles ; pour les arêtes parallèles, garder le **minimum exact** des dates. Cette réduction préserve les composantes à chaque coupe ouverte et fermée, pas les adjacences d'origine. Si L est le nombre de naissances, le graphe avant suppression des boucles/doublons a L sommets et R−A+L arêtes, avec la convention du § 5 incluant les points K1 dans A.

La naissance φ(B) peut être ancienne, mais l'ancre du bloc B n'est disponible qu'à λ_B. Les marques `(K,B,λ_B,φ(B))`, les dates de naissance et les contributions datées restent donc nécessaires. Pour une naissance à K>1, conserver la référence inférieure de même boule, puis normaliser son image à λ_B **fermé**. La naturalité des parents n'est pas remplacée par le seul graphe horizontal.

Le calcul des pivots peut employer le saut de pointeurs en parallèle : hauteur h, O(log(1+h)) rondes et O(A(1+log(1+h))) travail pour la variante discutée, y compris h=0. Il reste une table φ de taille O(A) ; réduire le graphe ne signifie donc pas automatiquement réduire la mémoire totale. Le modèle indépendant rapporte 267 cas et 37 356 coupes, avec mutants de dates et d'ancres ; ces graphes abstraits ne sont pas tous des nuages 3D réalisables. Ce résultat ne certifie ni le census ni la géométrie, et n'est pas une mesure de vitesse.

La [seconde contre-lecture](../audits/NOTE_CLAUDE_RACCORD_PERMANENT_ET_GRAPHE_20260911.md) confirme par le code la fenêtre des blocs, y compris les coquilles supplémentaires et K=n, ainsi que l'antériorité stricte. Le [raccord privé complet](OBJETS_PARALLELES_TOUR_20260911.md#7-raccord-complet-et-certificats-composables) compare désormais ce chemin au calendrier et à T2 sur de vrais census, avec contributions et verticales. Il exerce aussi la compression par fenêtres avant projection des hubs, sous la [preuve de composition](../audits/receipts_composable_msf_20260911/README.md). Aucun remplacement du moteur ni gain de vitesse n'est encore effectué ; retirer la matérialisation globale des terminales et paralléliser les primitives restent les prochains travaux.

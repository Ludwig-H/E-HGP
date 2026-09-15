# Fausses pistes et simplifications refusées

13 septembre 2026. Mémoire courte de l'audit et de la première tranche P0.
Les sources et contre-fixtures complètes sont dans les rapports liés.

## Détachement intérieur q2 — 15 septembre

- Réinitialiser une racine pour une branche transférée perdrait son compte
  acquis ou répéterait son préfixe. Transférer compte/curseur/phase et B
  original ensemble, mais pas les compteurs historiques déjà payés.
- Créer une équipe de threads par petite ancre est une sonde d'architecture,
  pas une politique à généraliser. Le raccord doit réutiliser les workers
  persistants et les plans parentaux ; jamais préparer Pool par tranche.
- Chercher une fixture de plage multiple admise dans le raccord actuel
  ne peut pas aboutir : même arbre B/Z et règle des diagonales imposent
  une subdivision préalable. La [preuve](P0_DETACHEMENT_CENSUS_Q2.md)
  précise les hypothèses ; elle n'autorise pas à ignorer les coquilles.
- Q cases de file ne signifient pas Q petits descripteurs : chaque case
  possède ici une continuation complète. Mesurer les capacités réelles,
  puis concevoir un format compact avant de multiplier les tâches GPU.

## Reprise q2 et ouverture q3/q4 — 14 septembre

- Rappeler le parcours partagé avec un curseur sauvegardé ne suffit pas :
  cela repayerait l'entrée et son certificat frère. Conserver le stade et
  les constantes, ainsi que l'état propre de chaque sœur en attente.
- Une pause Emit après une sortie précédente ne prouve pas que l'on a
  interrompu une même plage multiple. La gate exige un rang d'émission
  strictement intérieur à cette plage et conserve honnêtement son zéro.
- Migrer une pile n'exécute pas ses branches en parallèle. La brique de
  reprise reste préalable au détachement des frères et au répartiteur.
- Les seules arêtes acceptées en q2 ne donnent pas accès à tous les q3/q4 ;
  les seuls triangles acceptés en q3 ne donnent pas accès aux q4. Six
  [contre-fixtures exactes](../tests/q3_q4_owner_independence_gate.py) le réfutent.
- Pour q4, ne pas saturer le compte à K avant un balayage comportant des
  sorties : la profondeur peut redescendre. Regrouper les racines égales,
  exclure leurs propres événements du compte strict, garder les témoins
  hors lentille des complétions. Voir la [stratégie](Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md).

## Constats exécutés dans la première brique P0

- Un pool global ne couvre pas toutes les régions utiles : sur les rails
  q4/n2718, il laisse 1 846 881 paires ; blocs et tubes en laissent 2 916.
  Le pool reste pertinent pour certains rectangles q2, pas comme réponse unique.
- Un préfiltre en quelques millisecondes ne règle pas le coût des nappes :
  les trois méthodes gardent 256 millions de paires à n32k. Raffiner le
  produit ou employer un autre certificat, sans annoncer le carré supprimé.
- Les tubes ne dominent pas les blocs en volume résiduel ; les blocs ne
  dominent pas les tubes en temps de préparation. Choisir sur le coût total,
  pas sur l'un de ces deux chiffres isolés.
- Ne pas confondre D≥10R et d²≥25 diam² lorsque d=2(cB−cA) : il faut le
  facteur 100. Une consigne erronée a été corrigée avant codage ; une
  fixture permanente expose le faux témoin q4 que donnerait 25.
- Un shared_ptr vers const ne rend pas immuable une copie mutable de
  l'objet : interdire les copies/déplacements/affectations du propriétaire
  lui-même. La contre-fixture de réaffectation perdait trois paires q2.
- Une ligne JSON réussie n'est pas un reçu conforme : la confronter à la
  commande, garder les sorties brutes en échec et vérifier la stabilité
  du binaire et des sources avant de fermer la campagne.
- Protéger seulement le lancement du processus ne suffit pas : un signal
  levant pendant `communicate`, entre lecture et stockage des octets,
  peut encore perdre stdout/stderr. La tranche conjointe r2 diffère
  aussi ces signaux pendant les lectures et les rejoue hors de cette
  section ; l'échec initial est conservé et les deux flux sont testés
  par injection déterministe, y compris pendant l'annulation.

Voir les [preuves et mesures](../receipts/p0_local_credits_20260913/README.md).
Les méthodes explorées restent des variantes utiles du même module,
pas trois copies concurrentes d'un moteur FULL.

## Leçons de la préparation partagée et du filtre axial

- Trois voies q2/q3/q4 partagées ne sont pas la tour K=1..10 : le gain
  porte sur une préparation locale, pas sur les objets FULL ni leurs parents.
- Un rang projeté ne suffit pas en 3D. Le filtre axial exige les deux
  autres coordonnées exactement égales ; une colonne approximative
  réclamerait un autre certificat géométrique.
- La borne par ancre d'une grille complète ne vaut pas pour une dernière
  rangée tronquée : une fixture conserve 540 candidates contre la borne
  abusivement transférée de 441. Les entrées `sheet` et `sheet_full` restent distinctes.
- Une préparation non quadratique et des plages compactes ne rendent
  pas automatiquement le résidu, les visites d'index ou l'aval linéaires.
  Le filtre axial peut conserver toutes les paires hors de ses alignements.
- Une nappe alignée ne représente pas toutes ses orientations : une
  rotation entière conserve les profondeurs mais peut supprimer toutes
  les colonnes exploitables. L'auditeur conserve ce contrepoint isométrique ;
  les gains sur alignements ne sont pas des gains génériques.
- L'affectation membre par membre d'un plan peut changer le propriétaire
  avant une panne d'allocation de ses vecteurs. Copier entièrement puis
  échanger conserve l'identité de la cible même en cas d'exception.

Voir le [contrat de cette tranche](P0_PARTAGE_ET_FILTRE_AXIAL.md).

## Leçons conservées de l'audit

Le raccord census doit bloquer quatre raccourcis supplémentaires : ne
pas prendre `NoCredit` pour un certificat d'extériorité globale ; ne pas
chercher le maximum d'une fonction concave uniquement aux coins ; ne pas
supprimer les égalités pendant la collecte de coquille ; ne pas redémarrer
un parcours avec le crédit de témoins qu'il rencontrera de nouveau.
Une clé exacte commune à deux diamètres identifie une boule, mais ne rend
pas son flux de supports automatiquement dédupliqué. Ces obligations sont
expliquées dans le [contrat census](P0_CENSUS_Q2_PARTAGE.md).

La première écriture du census partagé utilisait un arena de listes
persistantes, recyclé entre enfants. Il est supprimé avant qualification :
l'ordre Z fixe permet de représenter exactement le même travail restant
avec un seul curseur et les échappements de l'index. Les listes auraient
ajouté des écritures et de la gestion d'état inutiles. Aucun gain temporel
par rapport à cette écriture intermédiaire non mesurée n'est revendiqué.

Les mesures du census confirment une autre limite : partager moins de
visites ne veut pas dire aller plus vite. Après l'intersection des résidus,
les tests sur groupes et la couverture des fragments annulent ici l'économie.
Shared reste utile sur certaines grilles moins filtrées ; il n'est pas un
mode gagnant universel. Le coût complet, et non la seule taille du résidu
ou le nombre de visites, gouverne le choix.

La cinquième tranche factorise les bornes dans 48 octets par tâche.
Les visites restent identiques : moins de produits ne signifie ni nouvelle
classe de complexité ni accélération massive. Les médianes Shared gagnent
localement 4–10 % sur grille/nappe32k, sans dominer systématiquement
l'individuel. Le compilateur optimisait déjà une partie des expressions.
Conserver cette économie mesurée, mais ne pas multiplier ses variantes
au détriment du partage global, des continuations et de FULL.

Deux raccourcis de cache sont refusés pour le prochain partage global :
des crédits ne se transfèrent pas entre rectangles du même nuage ; une
boîte B indexée par rang ne se transfère pas à une autre permutation du
même ensemble. L'auditeur les réfute respectivement sur quatre et cinq
sites. De même, après subdivision B, les constantes de son parent restent
sûres mais peuvent augmenter le travail : les reconstruire pour conserver
les mêmes décisions que le moteur qualifié.

Une fermeture échouée sans manifeste doit rester visible dans la lecture
d'un dossier de campagnes : ne découvrir que les manifestes masquait un
frère arrêté avant sa première mesure. Le lecteur q2 découvre désormais
aussi fermetures et mesures, puis rejette les triplets incomplets. Une
correction de lecteur ne doit pas réécrire les pins historiques : conserver
le snapshot de capture et qualifier séparément le contrôle renforcé.
Lors de la comparaison de révisions, les seules options C++ ne suffisent
pas : les flags de lien et l'optimisation interprocédurale peuvent changer
le binaire. Cette omission signalée par l'auditeur est corrigée avec
quatre mutants ; les valeurs réelles des présentes captures étaient
compatibles et leurs chronométrages sont conservés sans réécriture.

Troisième tranche : additionner les colonnes exactes est sûr, parce que
leurs témoins sont disjoints hors de l'ancre. Additionner ensuite ces
comptes à ceux de Pool/Dual/Tubes ne l'est pas : une fixture à trois sites
réfute ce double comptage. Leur **intersection de résidus** est implémentée.
Un filtre plus sélectif peut néanmoins coûter plus cher à construire :
le nombre de candidates ne remplace pas le temps de sélection plus census.
La spécialisation aux axes ne résout pas les nuages tournés. Voir le
[contrat additif](P0_ADDITION_ET_INTERSECTION.md).

| Piste | Pourquoi elle ne suffit pas ou échoue | Remplacement proposé |
| --- | --- | --- |
| Garder le graphe induit des seuls minima Gabriel | Supprime des chemins silencieux et retarde certaines fusions | Minima avec connexions datées issues des vrais rattachements |
| Assimiler partage de points et identité de composante | Détruit le recouvrement naturel HGP | Identités sur facettes/composantes, couverture séparée |
| Faire un MST sur les points pour toute la tour | K1 seulement ; les feuilles supérieures ne sont pas les points | Graphe daté propre à chaque K |
| Lire les parents dans leurs racines finales | Des parents distincts peuvent fusionner plus tard | Consultation historique à la bonne coupe |
| Les cofaces K+1 ont forcément K+1 parents | Cardinal de coface et arité de fusion sont différents | Lot atomique des composantes réellement distinctes |
| Les feuilles FULL suffisent aux poids du manuscrit | Les facettes contributrices des poids sont plus nombreuses | Profil pondéré explicitement séparé |
| Support ≤4 implique coquille ≤4 ou ≤12 | Une sphère peut porter bien plus de points | Politique non régulière explicite et format adapté |
| Toute sortie FULL doit être sous-quadratique en n | Les vraies feuilles peuvent être quadratiques à K fixé | Faible surcoût en plus du volume de sortie |

Références : [fondements](../audits/FONDEMENTS_ET_OBJET.md).

| Piste | Pourquoi elle ne suffit pas ou échoue | Remplacement proposé |
| --- | --- | --- |
| Une tâche par rectangle donne automatiquement un bon GPU | Les facteurs et les voies ont des coûts très différents | Tuiles internes et tâches de complétion |
| Il faut connaître les histogrammes locaux exactement avant de sélectionner | Le rejet n'exige que des minorants certifiés ; le census reste exact | P0 : comparer des architectures sans préparation A×A/B×B systématique |
| Un petit ensemble de témoins règle à lui seul toute la complexité | Il peut laisser un grand nombre de candidates inutiles | Comparer les alternatives et payer aussi raffinement, candidates et aval |
| Améliorer le proposeur ou s suffira toujours en q3/q4 | Deux rangées parallèles peuvent n'avoir aucun témoin W3/W4 pour leurs m² paires transversales | Garder le produit compact, puis certificats collectifs et génération canonique ; ne pas confondre ces candidates avec la sortie utile |
| Un front compact et rapide prouve un census rapide | Le callback de mesure ne développe pas les paires ; les masses résiduelles peuvent doubler au carré | Raccorder le vrai census et mesurer son coût avant toute conclusion globale |
| Réutiliser l'index suffit à partager tout le census | Le raccord direct paie encore Σmin(\|A\|,\|B\|) démarrages et les visites Z par ancre | Mesurer ces termes ; étudier des tâches gardant deux groupes A et B |
| Garder deux groupes A/B suffit à éviter tout raffinement prématuré | La fixture a=1000, B=0..63 a déjà A singleton ; le DFS croissant fragmente tout B avant les bons témoins | Choix/priorité Z avec ordre et échappements compatibles, sans fausser le compte hérité |
| Un test constant par enfant garantit une chaîne sous-quadratique | Le nombre d'enfants reste non borné par ce seul certificat | Mesurer croissance des tâches, visites et propositions ; ne pas confondre gain constant et suppression du carré |
| Le certificat frère suffit dans tous les régimes | Gain net sur les rangées, mais amas8k/16k/32k encore ×4,29/×4,22 en évaluations | Mode optionnel ; changer aussi l'ordre Z et traiter les témoins proches de l'ancre |
| Reporter B sans traiter l'ancre expose toujours les bons témoins | La fixture 3D cube4³ laisse un petit bloc {a}∪W indécis à cause de H(a,b,a)=0 | Descendre structurellement vers a avant les bornes et l'omettre du seul compte, pas de la coquille |
| Moins de visites Z après changement d'ordre prouve le gain total | Les chemins structurels ont aussi un coût, potentiellement répété aux splits B | Publier séparément bornes, opérations structurelles, tâches, collecte et temps englobant |
| L'ordre complément règle la croissance des amas | Visites géométriques encore ×4,106 puis ×4,229 à 8k/16k/32k, malgré moins de travail absolu | Partager aussi les ancres et mesurer la fragmentation ; conserver cette option sans borne globale |
| Exclure a du comptage autorise à exclure tout un groupe A | Une autre ancre a' peut être à l'intérieur de la boule de (a,b) | En mode A×B, conserver ces sites ; exclusion connue-zéro seulement après singleton |
| Passer de A×B à une ancre autorise à relancer le census depuis zéro en gardant le crédit | Le témoin déjà crédité serait compté deux fois, donc certains supports seraient perdus | Reprendre compte, curseur, phase et contexte original ensemble |
| Le compteur de racines suffit à comparer ancres et produits | Une borne conjointe coûte davantage et le travail restant peut se fragmenter | Séparer les bornes conjointes, reprises, tâches et visites aval, puis mesurer le total |
| La règle des diagonales partage les ancres jusqu'à une admission de tout A×B | Au nœud Z=A non singleton, les bornes continues restent indécises et la règle force une subdivision de requête | Ne pas revendiquer cette branche comme exercée ; comparer séparément une autre priorité de descente |
| La forte réduction Pool des rectangles inter-amas clôt déjà P0 | L'audit B compte le résidu, sans census ni sortie ; son temps de crédits n'est pas le total du front | Raccorder sur l'index global et mesurer préparation, résidu, census et payload complets |
| Développer systématiquement en individuel après Pool | Sur les deux colonnes alignées8k, tous les crédits locaux sont nuls : on perdrait Shared/frère pour16M racines de paires | Repli vers le parcours initial sans réduction, préparation toujours comptée ; mesurer aussi les résidus partiellement réduits |
| Un plan Pool local q2 doit pouvoir être totalement vide | Sans cœur extérieur, la paire croisée la plus courte n'a aucun témoin strict dans ses facteurs | Tester les classes saturées et préfixes vides, sans réclamer un plancher impossible sur le plan complet |
| Pool accélère tous les nuages parce que les temps uniforme/terrain varient | Aucun plan n'est sélectionné dans ces deux familles de la campagne Pool64 ; leur travail discret reste identique | Attribuer le gain aux rejets et au coût réellement modifiés, pas au bruit d'un hôte partagé |
| Le gain Pool sur amas32k résout le contrat de tour G4 | Le port mesure q2 mono, sans q3/q4 ni FULL, et aucune borne globale n'est établie | Conserver le gain mesuré, traiter le front et les petits rectangles, puis qualifier la tour sur G4 |
| Revenir à Global pour chaque racine singleton accélère forcément le census | L'audit A d608cc28 sur LiDAR50k retire26,18M descentes structurelles mais ajoute24,11M visites géométriques, sans gain stable | Ne pas porter cette variante ; les obligations de coquille tangente restent une proposition distincte |
| Retrancher le temps historique du front trois voies donne le census q2 isolé | Le nouveau raccord demande q2 seulement, sans Xi, avec un autre callback et des temps imbriqués | Comparaisons appariées q2, temps englobant front+census+collecte |
| La propagation de témoins est déjà un gain de temps validé | Le prototype indépendant réduit les résidus mais alourdit les tâches et ne gagne pas partout en temps | Garder les identifiants distincts ; comparer sur la chaîne consommée, pas promouvoir son seul compteur |
| Une descente unique puis K propositions est automatiquement économique | À uniforme32k/s8, le nouveau front paie 954 millions de pas d'index et passe de 4,87 s à 37,4 s malgré moins de rectangles | Garder cette baseline négative ; mesurer recherche partagée, test de lentille et census consommant directement les produits |
| s plus grand est toujours plus rapide | Moins d'indécision peut coûter plus de rectangles et de parcours | Comparaison s8/10/12 par phase et régime |
| Une WSPD linéaire implique un travail linéaire sur ses facteurs | La somme des tailles visitées peut être quadratique | Preuve de coût des parcours et crédits réellement payés |
| Partager le nuage rend automatiquement le multi-rectangles sous-quadratique | Pool/Axis gardent un terme Ω(R\|B\|) sur A_i×B ; les boîtes indexées ne le suppriment pas | Mesurer R fixe et R croissant, puis partager les préparations de facteurs ou changer le front |
| Un préfixe reste compact dans n'importe quel ordre B | Il peut devenir une union linéaire de singletons dans un autre arbre | Lier couverture et arbre à leur ordre précis, ou payer la fragmentation |
| Les valeurs s8 des préconditions v8 et du front v4 sont équivalentes | Les prédicats entiers de séparation diffèrent | Fixer une convention explicite avant la mesure de WSPD réelle |
| Copier le compteur h d'un parent puis recompter ses sites | Double crédit possible avec les populations locales | Identifiants et disjonction vérifiables |
| Les blocs positifs suffisent pour de gros facteurs | Ils peuvent échouer tard et payer encore le quadratique | Certificats négatifs, saturation, raffinement des crédits |
| Forcer les histogrammes par blocs même sur facteurs minuscules | Les mesures uniformes montrent un surcoût | Sélection adaptative explicitement mesurée |
| Rétrécir le cover q4 au seul citron de complétion | Perd des points qui comptent dans la profondeur | Cover prouvé pour tous les rôles |
| Arrêter q4 au premier intervalle trop profond | La profondeur peut ensuite redescendre | Balayage exact complet des segments utiles |

Références : [audit WSPD](../audits/WSPD_Q2_Q3_Q4.md).

| Piste | Pourquoi elle ne suffit pas ou échoue | Remplacement proposé |
| --- | --- | --- |
| Rendre le tri amont gratuit suffira au contrat | Seulement 0,75 % de la capture50k/K10 | Réduire MEB, génération, histoire et export |
| Des sous-arbres initiaux suffisent à équilibrer le calcul | Sur le prototype LiDAR50k de A, un job garde environ30 % des descentes avec256 jobs ; sa masse de paires ne prédit pas son coût | Mesurer le déséquilibre, puis redistribuer les produits DFS pendants sans refaire les parents |
| Davantage de CPU réduit la complexité | Les workers effectuent les mêmes décisions et le même nombre de tests que le mono | Conserver les comptes discrets et leurs ratios8k/16k/32k, indépendamment du gain mural |
| Des temps workers Donate presque égaux prouvent l'équilibrage | Leur présence inclut les attentes jusqu'à la fermeture globale | Comparer aussi produits, visites census et supports par worker ; mesurer à l'avenir la durée d'attente séparément |
| Attendre une place dans une file de dons bornée est sans danger | Tous les producteurs peuvent rester bloqués alors que du travail local reste disponible | Offre sans attente, refus conservé localement et receveurs réveillés à la fin ou à l'annulation |
| Soustraire la somme du payload au temps mur isole le comptage parallèle | Les intervalles des workers se recouvrent ; leur somme peut dépasser le temps mur | Publier séparément temps englobant, préfixe et sommes workers/payload |
| Un petit kernel rapide constitue une tour rapide | Reconstruction et transferts peuvent dominer sa phase | Objets résidents et temps de bout en bout |
| Compiler CUDA prouve l'exécution GPU | Les dernières tentatives de terminal n'ont exécuté aucun kernel | Gate réelle sur carte puis chaîne complète |
| Un même digest suffit à certifier tous les parents | Des erreurs locales peuvent être masquées par une normalisation finale | Comparaison physique avant normalisation, oracles et mutants |
| Un propriétaire immuable valide toute extraction adoptée | Il peut conserver fidèlement un certificat forgé ou des vues empruntées | Factory qualifiée avec provenance et durée de vie réelles |
| Déplacer un vecteur suffit à rendre ses coordonnées immuables | Les pointeurs mutables vers son tampon survivent au déplacement | Copie privée à la frontière du propriétaire, ensuite partage sans mutation |
| Réutiliser des marques simplement bien formées | Elles peuvent désigner une autre histoire/composante vivante | Appartenance certifiée avant réemploi |
| Supprimer les contrôles pour tenir le temps | Transforme un gain supposé en perte du contrat | Certifier une fois, réutiliser la preuve compacte |

Références : [implémentation](../audits/IMPLEMENTATION_PARALLELISATION.md)
et [mesures](../audits/CONTRATS_ET_MESURES.md).

Les contre-exemples anciens restent dans la v7 ; ils ne sont pas effacés.
La v8 garde ici leur leçon, puis devra porter explicitement les fixtures
pertinentes. Aucun fichier utilisateur ou archive de preuve n'est supprimé
par l'audit d'ouverture.

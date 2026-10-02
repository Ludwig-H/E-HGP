# Descente datée et forêt FULL en chantier

Audit indépendant v11 : aucune exécution native, compilation, GCP, import d'oracle produit ou modification de code/documents du développeur. **Aucun blocage mathématique concret trouvé dans la capture initiale.** Le conseil utile porte sur la classification de cellules, qui matérialise actuellement des traces immédiatement jetées. Deux fixtures exactes sécurisent une continuation à douze traces et la coface K+1 à K12.

## Source figée et progrès déjà adoptés

`SOURCE_BEFORE.json` fixe DEV **a7cd34ee2a5edbefc6ad98d9854e56e4df278b2e** et ses sources avant lecture. La descente et l'arrêt anticipé MEB sont publiés ; `forest.hpp`, `forest_build.cpp`, `forest_plateau.cpp`, `forest_vertical.cpp` et leurs nouveaux tests étaient WIP non suivis. Le raccord FullDomain était également modifié. Les sources des oracles de définition/catalogue ont été copiées avant lecture dans `sources_dependency/`.

L'ancien conseil MEB « premier support strict contenant » est **adopté dans la source publiée2e3af233** : l'arrêt est propagé à chaque niveau de combinaison, puis entre arités. `ADOPTED_BINDINGS.json` recoupe exactement cette source. Il conserve toutes les vérifications de positivité/enclosure et n'élague pas une extension q4 par l'obtusité de son préfixe. Ce constat n'est pas une qualification native. La fixture locale q4→globale q3 excluant minU de la [capsule17](../cells_locate_contract_review_17/README.md) était absente de la capture initiale. À **21:09:16 UTC**, `locate_test.cpp` AFTER l'a ajoutée en WIP avec les cinq sites exacts, S*=(1,2,3,None), arité locale4/globale3, β25 et payload20 octets. Ce delta ciblé a été relu : le précédent résultat est détruit avant le nouvel appel, donc l'attendu mémoire est cohérent. Ce conseil est désormais adopté en garde préparée, sans qualification native ; la garde hit p≥k est également déjà adoptée.

Le mini-lot `root_followup/` provient intégralement de la capture racine `/tmp/v11-adopted-gates-before18` avec sa métadonnée BEFORE. Il conserve aussi l'adoption WIP de la garde CenterRegion `2m³−2m²>i64` sur21/24 et six permutations, relue par la racine : cette capsule n'étend pas sa propre analyse géométrique à cette brique.

## Lecture de la descente et du plateau

- `descent.cpp/.hpp` ne porte **aucun mémo**. Chaque étape recalcule la MEB de la k-partie complète, traite p≥k avant `Complete`, puis choisit une trace stricte I∪A. La transition suivante doit diminuer β strictement. Le résultat distingue **initial_level** et terminal_level ; sa classe est utilisable aux coupes fermées a≥β(initial), ou ouvertes a>β(initial). Un futur cache garde cette date ; il ne peut dater sa classe par la seule naissance terminale.
- `forest_plateau.cpp::cell` résout toutes les traces et vérifie β(initial)<λ ainsi que le rang de naissance<rang du plateau. Les listes DSU gardent les **anciennes** racines touchées, même si leur représentant fusionne pendant le plateau. `close` crée un seul parent par groupe final ayant au moins deux anciennes composantes, trie ses enfants et garde une continuation sans nœud unary. Les listes sont réinitialisées au prochain plateau. Cette structure correspond au graphe biparti de T4 ; elle ne confond pas nombre de traces et nombre d'enfants.
- Les naissances sont préallouées, mais les traces n'atteignent que celles de rang strictement inférieur. T4 justifie que les nouvelles naissances d'un plateau restent isolées à ce niveau. La capacité2b−1 suppose exactement les multifusions de ≥2 enfants ; elle n'accorde aucune place à des continuations artificielles.
- `forest_vertical.cpp` prend une (k−1)-partie de la population I∪U d'une naissance. Sa MEB a β≤λ, et sa région témoin convexe contient le centre de naissance au niveauλ : la descente puis l'**ancêtre fermé** dans l'ordre inférieur donnent donc l'image naturelle. Aux fusions, toutes les images des enfants sont relevées au rang du parent et doivent être égales. Une seule image non vérifiée serait insuffisante.

La reconstruction n'expose pas ici d'attaches core/cover ni de projection de points ; ces politiques restent distinctes. Les gardes de contexte de domaine/LevelRank sont les préconditions privées annoncées, sans qualification d'une API externe forgée.

## Coût concret évitable, après baseline

`ForestBuilder::classify` appelle actuellement **build_cell complet**, lit `kind()`, puis détruit toutes ses traces. `cell` rappelle build_cell pour chaque cellule à traces et recommence son compte/remplissage. Pour une cellule non régulière, cela donne **quatre passages** sur toutes les t-parties, avant même de compter les MEB des descentes.

Fixture déjà préparée `octa_center` : centre(2,2,2), les six sites à ±2 sur les axes, K≥3. Pour sa boule centrale β4 : p=1,m=6,qmin=2, ordre3,t=2. Les15 paires donnent12 traces strictes (β(A)=2), tandis que les3 paires antipodales ont β(A)=4 et sont rejetées.

| Étape pour cette seule cellule | Tests de traces | Appels MEB(A) | Allocations du payload de traces |
| --- | ---: | ---: | ---: |
| Classification actuelle | 30 | 30 | 1×624 octets, ensuite libérée |
| Rejeu actuel | 30 | 30 | 1×624 octets |
| Total actuel | **60** | **60** | **2 allocations successives** |
| Classification proposée | 1 | 1 | 0 |
| Rejeu exhaustif conservé | 30 | 30 | 1×624 octets |
| Total proposé | **31** | **31** | **1 allocation** |

Le premier couple en Morton, `(2,2,0),(2,0,2)`, est déjà strict. Une classification **sans Buffer**, arrêtée au premier témoin strict, suffit donc ici. Cela n'autorise **jamais** d'arrêter le rejeu FULL au premier témoin : ses douze traces doivent encore être résolues pour conserver la couverture des composantes.

Contrat du classificateur proposé : conserver d'abord exactement la fenêtre `[p+qmin−1,p+m]` et la restriction k≤K ; hors fenêtre, aucun travail. Si t<qmin, la cellule est non-naissance sans test géométrique. Si t=m, c'est une naissance sans test : par criticité S*⊂U avec centre dans conv(S*), **MEB(U)=λ**, sans appeler une MEB éventuellement hors cardinal12. Sinon, rechercher un A de cardinalt avec β(A)<λ et s'arrêter au premier ; l'absence de témoin après recherche exhaustive signifie naissance. Le chemin de descente p≥k et les MEB hors CatK ne changent pas.

Ces comptes sont **symboliques par cellule**, tirés du code et de la combinatoire indépendante ; aucune durée ni gain allocator natif n'est mesuré. Les deux payloads actuels ne coexistent pas :624+624 n'est pas un pic1248. Le sens du compteur `combinations` reste à définir explicitement pour le nouveau classificateur ; son univers logiqueC(m,t) et ses essais réellement exécutés ne doivent pas être confondus. Les ledgers, juges, budgets et refus devront être adaptés/requalifiés après baseline.

## Deux contrôles Γ exacts et portables

`check.py` n'importe aucune référence. Il calcule les MEB par circonsphères affines englobantes en Fraction, puis balaye directement toutes les k et(k+1)-parties du grapheΓ ; la voie collinéaire utilise sa formule analytique indépendante.

**Octaèdre avec centre, ordre3.** Γ donne12 naissances à β2 ; à β8/3, les huit faces relient toutes ces anciennes composantes en une **fusion à douze enfants**. À β4, les douze traces de la boule centrale appartiennent déjà à une seule composante : c'est une continuation, pas une nouvelle fusion. Ce contrôle complète la [preuve locale de la capsule16](../full_locator_contract_review_16/README.md), sans rejouer son corpus.

**Treize sites collinéaires X=0..12, K12.** Deux naissances de l'ordre12 ont β121/4 et centres11/2,13/2. À β36, la coface de cardinal13 réunit les deux composantes : parent à deux enfants. Sa boule critique a p=11,qmin=2, donc p+qmin=13=K+1 ; les deux traces I∪A ont cardinal12. Dans l'ordre11, les trois naissances β25 fusionnent à β121/4. La verticale de chacune des deux naissances12, puis de leur parent, doit donner la racine11 en coupe **fermée** : avec numérotation canonique, lower12=(3,3,3).

Cette architecture ne requiert **pas de MEB13** : `birth_sphere` reconstruit une sphère depuis S*≤4, la descente reçoit des traces k≤12 et la verticale des parties k−1≤11. L'algorithme d'oracle Γ évalue bien une coface13, mais ce coût n'est pas une exigence de la voie constructive par sphères critiques. La garde `line12` actuellement préparée pour la forêt ne couvre pas encore cette fusion de K12 sur treize sites ; `descent_test` contient déjà une descente sur treize sites. Proposition précise : ajouter la fixture forêt ci-dessus, sans élargir prématurément la primitive MEB publique.

## Juges, mémoire et limites

Le juge forêt charge uniquement `model.py` et **Definition**, sans paquet `__init__`, voie constructive ou juge partagé. Son attendu de forêt/coupes/verticales vient de tous les k/(k+1)-sommets, indépendamment du catalogue critique, des traces et des descentes natifs. Les comparaisons de catalogue et certains helpers de parse/comptes sont partagés avec le modèle de descente ; ses payloads de modèle/corruptions ne constituent pas une troisième voie. La lecture ne joue aucun de ces juges natifs.

Les buffers `kinds[B]` survivent au constructeur. Les records de naissances sont temporaires et libérés avant DSU/touched ; les cellules rejouées et leur census de descente coexistent ensuite avec les tableaux DSU, la forêt de l'ordre en construction et les forêts inférieures retenues. La formule de capacité finale2b−1 ne borne donc ni ce pic ni le nombre de traces traitées. Aucun coût LiDAR, borne de complexité globale ou contrat FULL natif n'est acquis par ces preuves.

`SOURCE_AFTER.json` conserve les évolutions LIVE, avec copies séparées des fichiers changés ; elles ne remplacent jamais la capture initiale analysée. Le delta de capacité `forest_capacities` et sa garde b=2^31−1 admis/b≥2^31 refusé sont cohérents avec les capacités2b−1,2b−2 ; ils ne modifient pas la classification complète. Seuls ces deltas ciblés et l'adoption de la fixture17 sont relus à la fermeture, sans qualifier le reste du WIP. `COMMANDS.json` et les sorties normal/−O portent seulement sur le contrôle autonome. `SHA256SUMS` inventorie tout sauf son propre fichier racine, y compris le mini-lot de la racine et les métadonnées imbriquées. Aucun reçu clos ni note active n'a été modifié.

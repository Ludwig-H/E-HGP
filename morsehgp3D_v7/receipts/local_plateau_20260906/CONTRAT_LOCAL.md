# Quotient local de coquille — périmètre du helper promu

Le helper `morsehgp3D_v7/src/forest/local_plateau.hpp` est un composant local, pas le raccord FULL des plateaux. `public_status=not_claimed`. Il n'est appelé ni par la sonde FULL ni par son producteur. La garde refusant une coquille supplémentaire reste en place.

## Entrée et sortie

`mhgp7::local_plateau::ShellTable::prepare(LocalCensus)` reçoit une BallKey primitive, les intérieurs stricts I et la coquille U avec leurs PointId externes. La complétude du census relativement au nuage reste une précondition externe. Le helper valide les coordonnées u16, l'unicité des positions et identités, les puissances exactes, la primitivité de la clé et le fait que la coquille définisse bien la MEB positive. La coquille est explicitement bornée à 2..12 positions ; le nombre p d'intérieurs n'a pas de plafond de travail arbitraire, seulement la borne de représentation des identités. I est stocké une fois, trié par ID.

La table `contains_center()` contient exactement les masques de U dont l'enveloppe convexe fermée contient le centre. Elle est construite par les supports minimaux positifs de taille 2, 3 ou 4, puis fermeture ascendante ; aucun oracle global n'intervient. Une fois les faces propres exclues, les prédicats fermés de `plateau.hpp` qualifient bien un support positif minimal. `q_min()` et `max_strict_cardinality()` donnent respectivement le plus petit support contenant le centre et le plus grand masque ne le contenant pas.

`rank(K)` accepte tout K positif représentable. Au-dessus de p+u, le bloc est vide, pas une naissance. Pour K≤p, une composante stricte analytique couvre tout I∪U et son représentant est le préfixe de K intérieurs ; aucun sous-ensemble intérieur n'est construit. Sinon t=K−p, les sommets sont les t-masques stricts et chaque (t+1)-masque strict les relie par une étoile. La sortie fournit un représentant, la couverture en masque U, un préfixe de la liste I partagée et, pour ce petit helper de contrôle, les membres réduits. Elle n'énumère jamais les facettes globales.

`contribution_shell` et `contribution_interior` représentent D=(I∪U) moins l'union des couvertures strictes locales. D est une contribution potentielle, pas un delta global minimal : des chemins extérieurs peuvent déjà avoir apporté ses points. D vide n'autorise ni à ignorer une fusion ni à supprimer l'ancre de boule.

## Bornes arithmétiques

Les bornes adoptées sont celles conservatrices qualifiées de `pipeline/census.hpp`, pas le commentaire historique plus étroit de `lanes/keys.hpp` : A<2^68, |B_i|<2^87, |C|<2^105. Elles sont contrôlées avant évaluation et restent strictes.

Pour des coordonnées u16, la puissance est en valeur absolue <2^107. Le dénominateur du centre est <2^69 et chaque différence cnum−cden*x est <2^88. Les composantes d'une normale sont <2^33 ; les sommes de coplanarité et déterminants sont <2^123 et tiennent donc dans i128. Les produits du triangle sont <2^140, ses sommes barycentriques <2^142 et la comparaison finale <2^143 : la primitive signée 192 bits suffit. Les différences et produits géométriques préalables tiennent dans i64. Les fixtures à grands coefficients qui atteignent la vérification de puissance restent explicitement des censuses malformés, pas des MEB positives prétendues.

## Preuves locales et limites

L'autorité mathématique est `audits/receipts_plateaux_full_20260906/README.md`, puis son supplément `BALL_ANCHORS.md` et la réponse sur D dans `audits/DIALOGUE_COURANT.md`. La fenêtre d'ancre potentielle est K dans [max(1,p+q_min−1),p+u], restreinte à la tour demandée. Une composante déjà connexe et couvrant S peut néanmoins nécessiter son ancre. Ce helper ne crée aucune ancre, ne ferme aucun lot et ne normalise aucun parent global.

Le gate compare 18 tables et 96 rangs sur neuf petits nuages avec deux permutations contre une énumération rationnelle indépendante (oracle strictement borné à huit positions). Il vérifie littéralement les composantes, représentants, couvertures, masques réduits et contributions. Les cas carrés, continuation Z, coquille sept points et extérieur 2→1 parents globaux interdisent de confondre quotient local et résolution globale. Des supports positifs de tailles 2/3/4, p=5000 analytique et u=12 non vacants sont exercés. Le mutant privé désactivant les unions d'étoile doit être rejeté sur le nombre de composantes strictes.

Quatre cas extraits du nuage réel n=50000 sont ajoutés avec leurs IDs externes, I/U complets et clés exactes. Les attentes des 40 rangs K1..10 proviennent du lecteur rationnel indépendant, sans élargir l'oracle C++ à 12 points ni relancer le nuage50k. La trace source est SHA256 `3cd74b330c62978d8c3eedd175e12bf5fe02893facb2e008150c32b5054aea72`, le résultat rationnel `c4a066e620b7850b6b3f1937f5b6d92b027f763012a554f9d1fbbf5512cc3c81`. Le cas p=9,K=10 est connecté et couvre S, mais son ancre potentielle ne peut être omise sur ce seul constat.

Les gardes de régularité, les parents mondiaux pré-lot, les ancres horizontales/verticales, le journal daté des contributions, l'archive et les poids du manuscrit ne sont pas intégrés par ce lot. Aucun résultat de tour, temps50k, multi-CPU ou GPU n'en découle. GCP non utilisé.

## Captures

Le recorder de ce dossier sépare `--prepare` (compilations O2/SAN/mutant, exécutions O2 et mutant) de `--san` exécuté par ROOT hors du ptrace du sandbox, avec LeakSanitizer activé. Les reçus et bruts donnent les seuls statuts d'exécution faisant foi. Aucune limite temporelle ou CPU arbitraire n'est ajoutée. Les trois sources promues sont copiées ; les dépendances produit partagées et Boost sont épinglées, non recopiées massivement.

Les préparations privées précédentes restent historiques : r1 erreur de syntaxe, r2 succès O2 puis refus d'environnement LeakSanitizer/ptrace, r3 reprise SAN et mutant réussie sans modification de source. Le répertoire privé r4 contient seulement des sources préparées ; sa demande d'approbation a été annulée avant tout lancement, sans log, reçu, ELF ni résultat scientifique. Il ne qualifie pas les sources promues.

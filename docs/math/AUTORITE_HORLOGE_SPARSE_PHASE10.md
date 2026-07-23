# Autorité chronologique sparse — Phase 10.12-AUTH

## Statut, porte d'entrée et portée

Ce document fixe le contrat mathématique et l'implémentation de référence de `10.12-AUTH`. La validation reste ciblée et ne constitue aucune fermeture de phase.

- phase : `10`;
- backend visé : `reference_cpu`;
- profil : `hgp_reduced`;
- mode : `certified`;
- sémantique : `partial_refinement`;
- statut public : `not_claimed`;
- portée : `replayable_in_memory_source_batch_to_live_locator_capture_authority_only`.

La porte locale est satisfaite par `10.11-PSTAMP` et `10.11-CLOCK` :

- PSTAMP reconstruit le stamp canonique de chaque préfixe interne du locator;
- CLOCK lie conditionnellement chaque lot scientifique 10.7 à un préfixe du locator;
- CLOCK laisse toutefois `external_clock_authority_replayed=false`, car son ancre externe ne contient pas le journal qui a produit cette correspondance.

`10.12-AUTH` vise précisément ce journal manquant. Il doit enregistrer une capture du stamp vivant au point de linéarisation choisi par l'orchestrateur, puis permettre le rejeu de cette capture tant que la session en mémoire, le journal 10.7 et le locator restent disponibles et gelés pendant la vérification.

Ce jalon ne ferme ni la complétude des gateways, ni le quotient, ni la forêt, ni M.1. Il ne qualifie ni le SLO 50 000 points sous la seconde, ni la voie 10 millions de points ou davantage.

## Objets et notation

Soit \(J\) un journal scientifique 10.7 fraîchement recertifié. Il contient \(S\) lots canoniques

$$\mathcal{B}_s=(s,k_s,a_s,\mathcal{T}_s),\qquad 0\leq s<S,$$

où \(s\) est l'indice dense du lot, \(k_s\) le cardinal des facettes, \(a_s\) son niveau carré exact de première incidence et \(\mathcal{T}_s\) sa famille de tokens.

Soit \(L_p\) l'état scientifique interne d'un même locator 10.5a après exactement \(p\) commits acceptés. On note son stamp

$$\sigma_p=(v,A,p,I_p,U_p,Q_p,h_p),$$

où \(v\) est le schéma, \(A\) l'autorité externe du locator, \(I_p\) le nombre cumulé de clés insérées, \(U_p\) le nombre cumulé d'unions, \(Q_p\) le nombre cumulé de demandes de liaison et \(h_p\) le digest de l'historique committé. Un commit vide augmente \(p\) et modifie \(h_p\), même si \(I_p\), \(U_p\) et \(Q_p\) restent inchangés.

Le journal AUTH contient une suite chronologique de records

$$R=(r_0,\ldots,r_{C-1}),\qquad r_i=(i,s_i,p_i,\sigma_{p_i}),$$

où \(i\) est l'indice dense de capture, \(s_i\) le lot source capturé et \(p_i\) le nombre de commits lu directement dans le stamp vivant. Un journal complet destiné à CLOCK impose \(C=S\).

Le journal engage aussi :

- le schéma AUTH;
- l'identité scientifique fraîche de \(J\);
- l'identité de l'autorité du locator;
- le stamp initial de la session;
- le stamp final observé lors du scellement;
- une chaîne de digests séparée par domaine;
- les limites de population et d'octets fixées avant l'ouverture de la session.

## Théorème d'impossibilité sans capture vivante

### Énoncé

À partir des seuls objets finaux \(J\), du locator \(L_T\), d'un certificat CLOCK et d'une suite de stamps historiquement valides, il est impossible de démontrer qu'une frontière donnée a réellement été observée au cours de l'orchestration.

### Preuve

Fixons ces quatre objets finaux. Une première exécution peut capturer \(\sigma_p\) avant de traiter le lot \(s\). Une seconde peut terminer toute l'exécution, reconstruire ensuite \(\sigma_p\) par PSTAMP et assembler le même payload. Les objets finaux sont identiques, mais une seule exécution a réellement observé le stamp à la frontière annoncée. Aucun vérificateur déterministe recevant uniquement ces objets ne peut distinguer les deux histoires. Un digest non secret protège l'intégrité relative d'un payload; il n'horodate pas sa production et ne prouve pas sa causalité. \(\square\)

### Conséquence

AUTH ne doit jamais accepter un stamp historique fourni comme argument de la capture. La primitive de capture reçoit le lot source et le locator vivant, puis lit elle-même `snapshot_stamp()`. La conclusion demeure néanmoins relative au protocole de l'orchestrateur qui appelle cette primitive : un objet valeur reconstruit après coup peut être auto-cohérent sans prouver sa propre origine.

## Contrat de session

### Ouverture

L'ouverture reçoit des autorités déjà disponibles :

- le journal 10.7 complet et fraîchement recertifié;
- son identité scientifique 10.11;
- un locator 10.5a certifié, dont le stamp vivant d'ouverture est capturé et engagé;
- l'identité externe non nulle de ce locator;
- le nombre attendu \(S\) de lots;
- un budget fiable couvrant exactement les populations maximales de la session.

Elle réserve avant toute capture la capacité des \(S\) records et du futur remappage dense. Elle lit le stamp initial du locator sous exclusion externe. Un échec de capacité, de budget, d'identité ou de réservation ne crée aucune session utilisable.

Le journal 10.7, ses cinq arènes et son identité scientifique restent immuables pendant toute la session. Le locator peut évoluer uniquement par les commits sérialisés autorisés par l'orchestrateur.

### Capture

La capture de \(\mathcal{B}_s\) possède un unique point de linéarisation \(\ell_i\). Sous le même verrou exclusif que celui qui protège chaque `apply_batch` du locator, elle exécute dans cet ordre :

1. vérifier que la session est ouverte et que \(s\) appartient à \([0,S)\);
2. vérifier que le lot \(s\) n'a jamais été capturé;
3. lire le stamp vivant \(\sigma\) directement depuis le locator;
4. poser \(p=\sigma.\mathrm{committed\_batch\_count}\);
5. préparer le record \(r_i=(i,s,p,\sigma)\) et la prochaine valeur de chaîne;
6. publier entièrement le record dans la capacité déjà réservée;
7. seulement après cette publication, autoriser la poursuite du lot source;
8. relâcher le verrou.

Le point \(\ell_i\) est la lecture du stamp entre les étapes 3 et 4. Aucun commit du locator ne peut se linéariser entre cette lecture et la publication du record.

Si la capture échoue, elle ne publie aucun demi-record, n'avance ni l'indice chronologique ni la chaîne de digests, et le lot source ne peut pas être déclaré commencé sous cette session.

### Scellement

Le scellement exige exactement \(S\) records et une occurrence de chaque indice source. Il lit le stamp final sous le verrou commun, ferme la chaîne et rend la session immuable. Il produit aussi une ancre séparée contenant l'autorité, la session et le digest de scellement attendu; cette ancre doit être conservée par l'orchestrateur hors de l'objet journal vérifié. Après scellement :

- aucune nouvelle capture n'est admise;
- le journal scientifique et le locator peuvent être vérifiés seulement sous un gel externe commun;
- tout commit ultérieur rend le stamp final du journal périmé pour cette vérification.

Le scellement n'écrit rien sur stockage durable et ne constitue pas un checkpoint. Deux factories peuvent recevoir les mêmes identifiants : l'unicité globale de `(authority_id, session_id)` reste donc une obligation de l'orchestrateur. Le digest de scellement attendu sélectionne explicitement l'histoire autorisée et empêche qu'un second journal divergent portant les mêmes identifiants soit accepté sous cette ancre.

## Chaîne canonique de l'autorité

Soit \(H\) SHA-256, \(D_{\mathrm{init}}\) le domaine d'initialisation et \(D_{\mathrm{record}}\) le domaine des records. Une forme canonique admissible est

$$d_0=H(D_{\mathrm{init}}\mathbin\Vert \mathrm{enc}(v,A,\mathrm{id}(J),\sigma_0,S)),$$

puis

$$d_{i+1}=H(D_{\mathrm{record}}\mathbin\Vert d_i\mathbin\Vert\mathrm{enc}(i,s_i,p_i,\sigma_{p_i})).$$

L'encodage doit utiliser des entiers non signés big-endian, des longueurs explicites et les 76 octets canoniques du stamp 10.11. Il ne dépend ni de l'ABI, ni du padding, ni des adresses, ni des capacités de `std::vector`.

Cette chaîne distingue deux chronologies distinctes seulement sous l'hypothèse usuelle d'absence de collision SHA-256. Elle n'est ni une signature, ni un secret, ni une preuve de temps mural. Le digest tout-zéro reste une donnée; un booléen de présence distinct est obligatoire.

Le builder de chaîne AUTH doit être unique et partagé entre capture et rejeu. Une seconde transcription indépendante du même préimage serait une nouvelle surface de divergence et n'appartient pas au contrat.

## Invariants chronologiques

Un journal AUTH complet vérifie les invariants suivants.

### Densité et couverture

Les indices chronologiques sont denses :

$$i(r_i)=i,\qquad 0\leq i<S.$$

Les indices source forment une permutation :

$$\lbrace s_i\mid 0\leq i<S\rbrace=\lbrace 0,\ldots,S-1\rbrace.$$

Chaque lot 10.7 possède donc exactement une capture et aucun lot étranger n'est introduit.

### Monotonie de l'horloge du locator

Comme toutes les captures et tous les commits partagent le même ordre de linéarisation,

$$0\leq p_0\leq p_1\leq\cdots\leq p_{S-1}\leq T.$$

Si \(p_i=p_j\), alors les stamps doivent être égaux :

$$p_i=p_j\Longrightarrow\sigma_{p_i}=\sigma_{p_j}.$$

Si \(p_i<p_j\), les compteurs cumulatifs ne peuvent pas décroître. Sous absence de collision SHA-256, les digests historiques diffèrent dès qu'au moins un commit sépare les deux captures.

Cette monotonie porte sur l'ordre réel des captures, pas sur l'ordre canonique des indices \(s\). La suite source \(s_0,\ldots,s_{S-1}\) peut être non monotone, notamment lorsque l'orchestrateur entrelace plusieurs ordres.

### Identité commune

Tous les stamps partagent le même schéma et la même autorité externe que le locator de la session. L'identité scientifique 10.7 est celle reconstruite à l'ouverture et au rejeu. Une substitution conjointe du journal source et du locator définit une autre session; elle ne recertifie pas la première.

### Préfixe historique

Chaque record doit satisfaire

$$\sigma_{p_i}=\mathrm{PSTAMP}(L_T,p_i).$$

Cette égalité prouve que le stamp capturé est bien un état historique du locator final fraîchement vérifié. Elle ne prouve pas encore que cet état avait la bonne signification scientifique pré-lot.

## Hypothèses de linéarisation pour le qualificatif strict pré-lot

AUTH certifie sans hypothèse supplémentaire une **capture vivante sérialisée**. Le qualificatif **strict pré-lot** est plus fort et reste conditionnel aux règles suivantes.

### Exclusion commune

La capture, chaque `apply_batch` et toute lecture immédiate destinée à décider le lot emploient le même mécanisme d'exclusion. Le stamp n'est pas un verrou. Une lecture concurrente non synchronisée avec un writer sort du contrat et peut constituer une data race C++.

### Discipline des niveaux

Pour une voie scientifique d'ordre \(k\), avant la capture du lot de niveau \(a\) :

- tous les effets autorisés des niveaux strictement inférieurs à \(a\) ont été committés;
- aucun effet de niveau \(a\) ou supérieur n'a été committé dans cette voie;
- aucun writer étranger ne peut insérer une transition non attribuée dans le locator;
- l'autorité externe conserve l'attribution de chaque commit à sa voie, son ordre et son niveau.

AUTH ne peut pas déduire cette discipline des seuls stamps, car ceux-ci n'encodent ni \(k\), ni \(a\), ni l'identité d'un lot 10.7. Le journal la rend rejouable uniquement si l'orchestrateur journalise aussi l'ordre de ses appels et refuse toute transition hors protocole.

### Simultanéité d'un lot

Tous les tokens du même lot \(\mathcal{B}_s\) partagent exactement le record \(r_i\) et le préfixe \(p_i\). Il est interdit de capturer un stamp par token.

Les résolutions destinées au quotient doivent toutes lire l'état \(L_{p_i}\). Elles sont préparées intégralement avant la première mutation issue du lot. Une racine créée ou modifiée par un candidat du lot courant ne peut donc servir de racine pré-lot à un autre candidat du même lot.

Si un futur schéma répartit un même couple \((k,a)\) sur plusieurs records techniques, tous ces records doivent partager le même stamp et être couverts par une barrière unique. La séquentialisation de ces fragments ne doit jamais produire plusieurs sous-lots scientifiques.

### Ordres distincts

AUTH autorise un entrelacement chronologique de plusieurs ordres, mais n'en déduit aucune commutation scientifique. Une utilisation commune du locator par plusieurs ordres exige soit des espaces de handles disjoints et recertifiés, soit une autorité supérieure prouvant les transitions permises. Sans cette preuve, la capture reste valide comme fait temporel mais ne devient pas une décision de quotient.

## Ce que le rejeu peut certifier

Sous les préconditions précédentes, le rejeu en mémoire peut certifier :

- l'identité scientifique du journal 10.7 auquel la session était liée;
- la densité chronologique des captures;
- l'unicité et la couverture complète des lots source;
- la monotonie des préfixes dans l'ordre de capture;
- l'appartenance de chaque stamp à l'histoire interne du locator;
- l'égalité entre captures répétées d'un même préfixe;
- la présence d'un commit vide entre deux stamps qui l'encadrent;
- l'identité de l'autorité du locator et de la chaîne AUTH;
- l'absence de mutation séquentielle du journal source et du locator pendant la vérification correctement gelée;
- la table dense attendue par 10.11-CLOCK après remappage des records vers l'ordre source.

Si le vérificateur reçoit l'ancre de scellement fiable puis rejoue le producteur AUTH appartenant à l'orchestrateur, il peut publier un fait limité de la forme `external_clock_authority_replayed_in_memory=true`. Ce fait signifie seulement que les appels de capture de cette session ont été rejoués et raccordés aux préfixes historiques. Il ne permet pas d'affirmer que le verrou a réellement été tenu ni que chaque capture précédait scientifiquement son lot.

Même dans ce cas, `conditional_on_caller_strict_pre_lot_orchestration=true`, `external_freeze_synchronization_replayed=false`, `crash_durable=false`, `quotient_mutated=false`, `gateway_completeness_claimed=false` et `public_status_claimed=false`.

## Ce que le rejeu ne peut pas certifier

Le journal AUTH ne prouve pas :

- une heure civile ou une durée physique;
- qu'un payload arbitrairement désérialisé a réellement été produit en direct;
- l'absence d'un writer qui aurait contourné le mécanisme d'exclusion;
- l'attribution scientifique d'un commit si cette attribution n'est pas journalisée par l'orchestrateur;
- la discipline des niveaux à partir du seul compteur de commits;
- qu'un miss du locator désigne une composante isolée;
- la complétude des facettes-portes silencieuses non directes;
- l'indépendance des choix admissibles requise par M.1;
- une décision, une union ou une contraction du quotient;
- une forêt HGP, une attache, un morphisme vertical ou un statut public;
- la résistance à une collision SHA-256;
- la récupération de la session après perte du processus.

Un rehash cohérent d'un journal forgé reste une nouvelle prémisse. La correction scientifique vient du rejeu de l'autorité productrice et de ses règles d'orchestration, pas de la seule égalité des digests.

## Vérification fraîche fermée

La vérification reçoit les budgets fiables, les autorités géométriques de 10.7, le journal scientifique observé, le locator vivant, l'ancre externe du digest de scellement attendu, la session AUTH scellée et les budgets imbriqués de 10.11.

Elle suit cet ordre :

1. borner les tailles observées, le nombre de records, les deux scratches de remappage et leurs octets avant tout scan variable;
2. vérifier schéma, autorité, session, digest de scellement ancré, état scellé, identité source annoncée et stamp initial;
3. rejouer fraîchement et complètement 10.7, puis recalculer son identité scientifique;
4. exécuter le vérificateur structurel borné complet du locator;
5. vérifier les indices chronologiques, la permutation des indices source et la monotonie des préfixes;
6. rejouer la chaîne AUTH avec le builder canonique partagé;
7. appeler PSTAMP exactement une fois sur la suite déjà non décroissante \((p_i)\);
8. comparer chaque stamp historique, y compris les répétitions;
9. remapper les records vers l'ordre source sans supposer la monotonie des \(s_i\);
10. reconstruire le certificat 10.11-CLOCK attendu et exécuter sa vérification;
11. comparer le stamp final au locator à l'entrée et à la sortie;
12. publier le succès en mémoire seulement si toutes les étapes précédentes aboutissent.

Le tableau inverse `source_batch_index -> chronological_record_index` remplace le tri de CLOCK dans la voie AUTH : les indices source forment déjà une permutation bornée. Le certificat CLOCK dérivé conserve néanmoins son format canonique existant.

Toute insuffisance de budget, contradiction, mutation séquentielle ou autorité étrangère publie une décision d'échec et aucun tableau de frontières utilisable.

## Complexité et stockage

La session réserve \(S\) records à l'ouverture. Une capture lit un stamp de taille fixe, écrit un record et avance un contexte SHA-256 :

$$T_{\mathrm{capture}}=O(1),\qquad M_{\mathrm{capture}}=O(1).$$

Le stockage propre du journal et le scratch propre de son rejeu sont

$$M_{\mathrm{AUTH}}=O(S),\qquad M_{\mathrm{rejeu\ AUTH}}=O(S).$$

Le rejeu propre, hors autorités imbriquées, est

$$T_{\mathrm{rejeu\ AUTH}}=O(S).$$

Pour \(B=\max_i p_i\), \(M\) slots du locator, \(I_B\) liaisons actives, \(U_B\) unions et \(P_B\) identifiants de points engagés, la composition avec PSTAMP coûte

$$O(S+2B+M\mathbf{1}_{I_B>0}+U_B+I_B+P_B)$$

avec un scratch \(O(S+I_B)\), auquel s'ajoutent séparément le rejeu de 10.7 et la vérification structurelle 10.5a.

Chaque population, chaque cumul de scans, chaque comparaison de remappage et chaque taille en octets doit posséder un plafond testé avant consommation. Un dépassement arithmétique est une erreur de capacité, jamais une troncature.

Ces bornes sont linéaires dans les artefacts sparse effectivement produits. Elles sont compatibles avec l'architecture visée, mais ne constituent aucune mesure du SLO interactif ni une qualification de très grand nuage.

## Structures globales explicitement évitées

AUTH conserve un record de taille fixe par lot 10.7. Il ne construit ni ne recopie :

- un snapshot complet du locator par lot;
- une copie des parents DSU par frontière;
- une DSU par capture;
- une table dense lots-par-facettes;
- l'univers des \(\binom{n}{k}\) facettes;
- l'univers des \(\binom{n}{k+1}\) cofaces;
- Gamma exhaustif ou ses composantes;
- les cellules top-dimensionnelles;
- les incidences globales de la mosaïque de Delaunay d'ordre supérieur;
- un quotient, une forêt, une racine persistante ou un `GatewayAttach`.

Le journal engage seulement l'identité du flux utile et les frontières observées. Il préserve ainsi l'invariant d'architecture de MorseHGP3D : l'oracle exhaustif borné peut falsifier le chemin produit, mais ne devient jamais son stockage ou sa procédure par défaut.

## Rejouable en mémoire contre durable après crash

### `replayable_in_memory`

Le présent contrat couvre seulement une session dont les objets vivent encore dans le même domaine d'exécution :

- le journal 10.7 est accessible et immuable;
- le locator et son historique interne sont accessibles;
- la session AUTH scellée est accessible;
- les autorités externes nécessaires au rejeu sont accessibles;
- l'orchestrateur peut rejouer l'ordre de ses appels de capture;
- la vérification dispose d'un gel commun.

Une perte du processus peut supprimer la session entière. Une copie mémoire, un core dump ou une sérialisation improvisée ne devient pas une autorité scientifique durable.

### `crash_durable`

Une future portée durable exige un contrat distinct et ne peut pas être déduite de la chaîne SHA-256 en mémoire. Elle devra au minimum spécifier :

- un codec canonique hostile et borné;
- les caps de longueurs avant allocation;
- un journal write-ahead avec numéro de génération;
- l'ordre entre écriture, synchronisation, renommage et publication;
- la synchronisation du fichier temporaire et du répertoire;
- un checksum d'intégrité locale distinct de la filiation scientifique;
- un manifeste ou `HEAD` atomique empêchant l'acceptation d'un état partiel;
- l'ancrage durable des identités 10.7 et du locator correspondant;
- la détection des records déchirés, doublés, omis ou réordonnés;
- une règle de récupération du dernier préfixe entièrement publié;
- une protection contre le rollback vers une génération ancienne mais valide;
- des tests de panne à chaque frontière de publication.

Tant que ce lot durable n'est pas fermé, le contrat impose `crash_durable=false`, `checkpoint_present=false` et `restart_after_process_loss_certified=false`.

## Interface avec 10.11-CLOCK

AUTH ne remplace pas PSTAMP ni CLOCK.

- PSTAMP reste l'autorité interne des stamps de préfixe.
- CLOCK reste le certificat canonique liant l'identité scientifique 10.7 aux frontières du locator.
- AUTH fournit le journal producteur qui permet de rejouer l'ancre externe de CLOCK dans la portée en mémoire.

Pour chaque record \(r_i\), AUTH construit une frontière CLOCK

$$b_{s_i}=(s_i,p_i,\sigma_{p_i}).$$

Le remappage par \(s_i\) produit exactement une frontière par lot source. CLOCK recalcule ensuite l'identité scientifique, vérifie le locator, PSTAMP et son propre digest comme auparavant. AUTH ne doit pas court-circuiter ces vérifications au motif qu'il possède déjà les mêmes valeurs.

La conclusion combinée reste de la forme :

- journal producteur rejoué en mémoire;
- correspondance source-vers-préfixe recertifiée;
- strict pré-lot conditionnel aux invariants de linéarisation;
- aucune durabilité après crash;
- aucun quotient muté;
- aucune promotion publique.

## Étape suivante : quotient strict pré-lot en lecture seule

Le prochain pré-lot doit composer 10.7, 10.9, 10.10 et AUTH sans mutation.

Pour chaque lot source \(s\), son record AUTH fournit \(p_s\). Une résolution historique est identifiée par le couple

$$q=(p_s,t),$$

où \(t\) est l'indice du token de facette. La déduplication porte sur ce couple et jamais sur \(t\) seul : une même clé peut être latente à un préfixe ancien puis positive à un préfixe ultérieur.

10.10 reconstruit alors, pour chaque couple demandé :

- `positive` avec la racine historique au préfixe \(p_s\) et le témoin original de première liaison;
- ou `latent_unresolved`.

Un latent ne devient jamais un singleton et ne certifie aucune absence. Toutes les résolutions d'un lot sont achevées sur le même préfixe avant de construire l'hypergraphe local des racines gelées. Les composantes de cet hypergraphe sont calculées avant toute union. Le pré-lot publie seulement une proposition de quotient et ses témoins; une transaction ultérieure, séparée, décidera éventuellement les continuations et multifusions simultanées.

Cette étape doit distinguer explicitement :

1. la proposition géométrique 10.7;
2. la capture et la décision d'autorité AUTH;
3. la résolution historique 10.10;
4. la réduction hiérarchique pré-lot;
5. le statut public, qui reste absent.

Tant que la complétude globale des gateways silencieux non directs reste ouverte, même un quotient local entièrement rejoué demeure `partial_refinement`. Il ne ferme ni la Phase 10, ni M.1.

## Falsifications minimales permanentes

La campagne d'implémentation devra rester courte et cibler au minimum :

- deux lots distincts capturés au même préfixe, avec stamps identiques;
- un commit vide entre deux captures, avec changement du préfixe et du digest;
- une chronologie source non monotone mais couvrant chaque lot exactement une fois;
- un lot source omis, dupliqué ou hors domaine;
- un indice chronologique non dense;
- un préfixe chronologique décroissant;
- un stamp valide déplacé vers un autre préfixe;
- un schéma ou une autorité étrangère;
- une identité scientifique 10.7 substituée;
- une mutation de chaque champ du record et de la chaîne, suivie ou non d'un rehash;
- un stamp final périmé après un commit vide;
- un suffixe structurel du locator corrompu après le plus grand préfixe capturé;
- chaque plafond exact puis diminué d'une unité;
- l'échec atomique d'une capture sans demi-record;
- l'immutabilité du journal 10.7 et du locator pendant la vérification;
- la preuve statique qu'aucun `apply_batch`, quotient, Gamma, cellule, coface globale, forêt ou `GatewayAttach` n'entre dans le target AUTH.

Une course writer-reader réelle n'est pas une fixture valide : elle sortirait du contrat de synchronisation et pourrait déclencher un comportement indéfini. Les tests simulent les mutations séquentiellement et vérifient séparément le protocole de verrouillage.

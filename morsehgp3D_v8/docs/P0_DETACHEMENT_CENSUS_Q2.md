# Répartir les branches internes d'un census q2

15 septembre 2026, tranche16. Cadre : exploration v8 hors registre,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=implementation_v8_p0`, `public_status=not_claimed`.

## Ce qui change

La tranche15 pouvait déplacer une recherche entière vers un autre thread.
Cette tranche peut en extraire une branche encore non visitée et faire
travailler plusieurs workers simultanément sur **la même ancre q2**.
Elle ne modifie ni la géométrie ni les supports demandés. Le raccord
front/Pool existant reste inchangé ; ce répartiteur intérieur ne remplace
pas automatiquement tous ses appels, souvent beaucoup trop petits.

Le contexte reste le même index global immuable, la même ancre et le même
B original. Aucun facteur n'est recopié, aucun arbre local reconstruit,
aucune recherche de témoins déjà payée n'est recommencée.

## La branche est une obligation, pas une nouvelle racine

La pile contient le cadre actif à son sommet et ses frères B non visités
en dessous. `detach_pending()` extrait le plus ancien de ces frères,
jamais le cadre actif. Il doit être à l'étape Entry : ni ses bornes,
ni son test du frère, ni son entrée de tâche n'ont encore été payés.

Chaque cadre conserve son propre compte, curseur Z, phase, nœud frère,
ainsi que le contexte B original de la recherche. Remplacer ce dernier
par le petit B extrait changerait l'ordre des témoins : c'est interdit.

Les sous-arbres B pendants sont disjoints. Extraire l'un d'eux préserve
leur union. Le préfixe Z certifié pour le parent est valable pour chaque
enfant : le compte et la position reprennent ensemble. Les comptes déjà
accumulés restent chez le donneur ; l'enfant commence son historique à
zéro, mais pas son compte géométrique acquis. Ce sont deux choses différentes.

Pour une branche de m paires, la masse candidate du donneur diminue de m,
celle de l'enfant vaut m. Aucun historique accepté/rejeté n'est déplacé.
L'enfant ne crée ni descripteur ni `count_root_start` supplémentaire.
À la fin de chaque fragment, candidates = acceptées + rejetées ; à la
fin de toute la famille, les sommes donnent exactement la racine initiale.
Les crédits uniformes historiques peuvent dépasser la masse que le donneur
conserve après export : ce n'est pas une erreur de comptabilité.

Identité locale à terminaison :
`query_tasks = count_root_starts + 2*query_splits + imported_frames − detached_frames`.
Globalement imports = exports, donc l'identité mono est retrouvée.
Les 36 compteurs géométriques se réduisent par sommes (le maximum de
profondeur de construction, ici nul, garde sa convention). Les quatre
types de transitions restent additifs. Appels, pauses et pics de pile
ne sont pas invariants d'ordonnancement et restent publiés séparément.

## Mémoire et erreurs

L'enfant possède un nouveau moteur, sa sentinelle de callback, sa pile
et ses buffers. Aucun pointeur vers le moteur du parent ne survit.
Seul l'index immuable est partagé. L'enfant peut finir après destruction
du donneur, à condition de conserver aussi les résultats déjà produits.

Toutes les allocations et vérifications précèdent le retrait de la branche.
Un échec laisse le donneur inchangé et réutilisable, y compris ses compteurs
d'essais. Après réussite, jeter l'enfant perdrait du travail : le répartiteur
doit le posséder jusqu'à achèvement, ou déclarer l'appel global en échec.
Une exception de callback n'annule pas les émissions antérieures.

Le répartiteur réserve ses cases de file avant lancement. Il n'extrait une
branche qu'après acquisition sans attente du verrou et d'une case libre.
La publication du propriétaire unique dans cette case n'alloue rien.
File pleine ou verrou occupé : poursuite locale, sans attente de place.
Les callbacks s'exécutent hors verrou et disposent d'un slot privé.

La fin exige file vide **et** aucune continuation active. L'annulation
réveille les workers en attente ; tous les fils lancés sont joints avant
retour ou propagation d'exception. Le lanceur partagé traite aussi un
échec de lancement partiel. La granularité change l'ordonnancement,
jamais la quantité de recherche conservée.

Chaque continuation réserve encore 49 cadres de 128 octets, soit6 272 octets,
avant métadonnées et payload. Avec W workers et Q cases, il y a au plus
W+Q continuations vivantes : la création d'un enfant utilise une case
encore libre, pas une population parallèle non bornée.
Cette borne vise les objets internes à l'appel, pas ceux qu'un callback
utilisateur déciderait de créer lui-même. Les Q pointeurs
de la file et le maximum de capacité d'un fragment sont mesurés à part.
`max_fragment_bytes` n'est pas un RSS simultané. Les capacités ne diminuent
pas pendant un fragment ; leurs observations après les avances capturent
son maximum. Index, métadonnées et piles natives sont exclus.

## Complexité et limites

Noter T le nombre de tâches B de la racine, M ses transitions et E les
exports. Un frère non visité ne peut être extrait deux fois sans être
entré puis divisé : E ≤ nombre de divisions B. Le retrait dans le vecteur
déplace au plus48 cadres. Ce surcoût est compté (`moved_frames`) ; il
n'introduit pas un scan O(|A|²+|B|²). Les vérifications de don sont au plus
une par pause, donc au plus M, avec un quantum positif.

Cela ne borne pas le travail géométrique T ou M sous-quadratiquement.
Les comptes de la racine sont inchangés ; la parallélisation ne résout
pas une éventuelle croissance quadratique de leur somme sur tout le front.
Les allocations de fragments et la contention peuvent perdre plus de temps
que le parallélisme n'en gagne sur les petites requêtes.

La collecte/callback d'un support reste atomique, potentiellement O(n+coquille).
Une requête singleton sans frère ne peut pas être divisée par cette API.
Les pauses Emit sur des singletons sont exercées. Une plage multiple
admise est en fait impossible dans ce raccord précis, comme établi
ci-dessous ; il ne faut pas promettre cette fixture en changeant seulement
la taille du quantum ou la politique d'émission.

### Pourquoi l'admission multiple est impossible ici

Précision constructive de l'argument Bbc9b2dc5. Supposer qu'un nœud
multiple B* soit admis, et suivre sa lignée : la requête courante C reste
un ancêtre de B*. Choisir dans B* un point b de distance maximale à a,
puis b' distinct. Avec δ=b−b', on a `(b−a)·δ ≥ |δ|²/2`.
Le point continu z=b−δ/4 appartient au segment [b',b], donc à toutes
les boîtes des ancêtres de B*, et `H(a,b,z) ≥ |δ|²/16 > 0`.
Pour z=b, H=0. Ainsi, pour tout bloc témoin Z ancêtre de B*, les bornes
sur C×Z vérifient `borne_inf ≤ 0 < borne_sup` : aucune consommation
uniforme de ce bloc n'est possible. Le minimum n'est pas nécessairement0.

La descente doit donc atteindre Z=B* sans le consommer. Sa diagonale
est au plus celle de C. La règle scinde alors C, pas Z, jusqu'à C=B*,
puis impose de scinder B* lui-même : contradiction avec son admission
en tant que nœud multiple. Le report de B original en seconde phase
ne consomme rien et doit être revisité ; l'ancre extérieure ne peut pas
faire sauter B*, ni le raffinement structurel descendre sous lui.
Le détachement préserve exactement cette lignée et ne change pas l'argument.

Cette preuve dépend du même arbre B/Z et de la règle des diagonales.
Elle ne s'étend pas automatiquement aux anciens plans B locaux, à une
autre règle de descente ou aux futures formes q3/q4. Elle n'autorise
pas à omettre la collecte complète de la coquille d'un support singleton.

Le temps `total_ms` du répartiteur englobe création de racine, lancements,
travail, jointures, réduction et destruction des continuations. Ses temps
census sont des **sommes d'intervalles actifs**, pas des temps muraux.
Le résultat retourné et les buffers du consommateur restent vivants.

## Mesures propres et prochaine intégration

Les captures sont conservées dans
[les reçus de détachement](../receipts/q2_census_split_20260915/README.md).
La sonde choisit une seule ancre déterministe du front, comme tranche15.
Elle compare tout son travail et ses payloads à la référence récursive,
avec W1/W4, n8k/16k/32k, K5/10 et s8/10/12. Les chronos de sélection
du front ne sont pas des chronos du census complet. La référence passe
en premier ; les différences de caches et de lancement doivent être prises
en compte avant toute conclusion de performance.

Suite : raccorder ces obligations aux workers déjà persistants du front,
sans créer une nouvelle équipe par ancre ; conserver les petits travaux
localement. Les plans Pool parentaux doivent devenir possédés et partagés,
jamais recréés par tranche A_i. La collecte devra elle aussi devenir
partageable si sa durée domine. Cette représentation CPU à plusieurs
kilo-octets par fragment n'est pas un descripteur GPU massif qualifié.

L'équipe persistante devra distinguer quatre obligations : produit du front,
curseur de rectangle, continuation Shared q2 et curseur de bande Pool.
Une plage de permutation Pool n'est pas un nœud B spatial : ne jamais
substituer l'une à l'autre pour entrer dans le nouveau census. La voie
Pairwise des bandes filtrées et le repli Shared sans réduction restent
nécessaires. Clôture, annulation et bilan géométrique devront être communs.

Pour le futur format massif, une branche détachée est toujours Entry :
ses constantes préparées ne sont pas encore calculées. Une file pourrait
donc ne posséder que le descripteur de branche et une référence vers un
contexte parental immuable ; un moteur actif privé par worker fournirait
pile et buffers. Cela éviterait les6 272 octets réservés pour chaque
branche en attente. Ce changement exige une nouvelle preuve de durée de
vie et de comptabilité ; le présent code ne fournit pas encore ce format.

q3/q4 restent des moteurs à construire ; l'oracle indépendant B5124095b
est une nouvelle source de réfutation, pas une qualification héritée du
produit. FULL, 50k sur G4 sous une seconde, puis100 ms, et plusieurs
dizaines de millions de points restent des contrats ouverts. GCP non utilisé.

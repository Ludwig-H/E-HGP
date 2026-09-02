# Préflight statique — profil G4 échelle v6

Date : 2 septembre 2026. Pins jugés : moteur `28d02459`, raccord de campagne
`d8d7a7f7`, puis correction documentaire et wire `788b22da`.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Audit local et statique : GCP non utilisé, aucune cible externe interrogée.

## Verdict utile

Le profil n'est plus un WIP absent : `d8d7a7f7` livre réellement
`g4_echelle_v1`, son inventaire à 17 fichiers, le trajet
profil → lifecycle → SSH → runner → plan/statut/argv → validateur, la
normalisation de `fam:n:11` avant doublons et artefacts, et la séparation de
Q2. Le calcul publié distingue bien 10 390 s d'estimation nominale, 10 800 s
pour les neuf plafonds de runs, 10 890 s avec les quelque 90 s d'overhead, et
13 195 s de fenêtre utile. Ces progrès sont reçus. Le selftest campagne, lui,
n'est pas encore reçu : son export exact termine par code 141 avant sa dernière
contre-fixture.

Le **NO START reste toutefois actif**. Il ne dépend plus d'une longue liste de
finitions génériques : le selftest et les fermetures causales ci-dessous
suffisent avant de dépenser la session.

## P1 — fermer le refus mémoire réellement exercé

Le validateur n'impose pas encore « aucun payload publié » sur un code 2. Un
probe direct lui donne le refus d'allocation exact et sa ligne `refus_etage=`,
puis ajoute `digest_all=...`, une ligne `cardinalites K=...` et un second
`REFUS` non typé : `classe_refus_etage` rend
`resource_exhausted a l'etage census` avec une liste d'erreurs vide. Renommer
seulement `allocation impossible a l'etage` en `allocation impossible au
stade` déclasse aussi le même incident en refus ordinaire, même si
`refus_etage` le contredit. Des RSS négatifs concordants passent enfin la
grammaire.

La fermeture utile est petite : définir le corps exact autorisé d'un code 2,
interdire tout digest, cardinalité, payload ou ligne supplémentaire, et porter
une cause non allouante distincte du texte. Un helper commun doit imprimer les
deux lignes avec `fprintf` depuis `cause`, `stage_reached` et les cinq RSS ;
`rr.message` reste un diagnostic best-effort. Cela ferme aussi le cas où
`rr.message.reserve(256)` échoue durablement : le statut et l'étage existent
alors, mais le CLI courant imprime `REFUS ` et le reçu devient inutilisable.

La preuve de `d8d7a7f7` est synthétique : son faux binaire fabrique lui-même
les deux lignes attendues. Ajouter un subprocess qui traverse le vrai renderer
CLI, le runner puis le validateur, exige code 2, stdout vide, les deux seules
lignes stderr et zéro sortie d'objet. Conserver la portée honnête des
callbacks : ils sont provisoires jusqu'au statut terminal ; l'invalidation
interne ne reprend pas un effet externe déjà observé.

## P1 — rendre les créations partielles de fils sûres

Sous `RLIMIT_AS`, la création d'un fil peut précisément être l'allocation qui
échoue. Dans `parallel_ranges` et `parallel_items`, une exception au deuxième
`emplace_back` déroule un vecteur contenant déjà un `std::thread` joignable et
appelle `std::terminate`. Dans `parallel_stable_sort`, joindre naïvement ne
suffit pas : les fils déjà lancés peuvent attendre une barrière dimensionnée
pour une équipe jamais complète. Le worker B du fold ne termine pas par le
même destructeur fautif, mais son `std::system_error` sort encore non typé.

Solution commune : injecter l'échec au deuxième lancement ; armer stop+join
avant la boucle pour les pools simples ; retenir les travailleurs du tri
derrière un sas de départ, puis ouvrir le sas en mode abandon et joindre si le
lancement reste incomplet ; convertir seulement les erreurs de ressources du
constructeur de fil en une cause non allouante `worker_start`. Les portes
exigent absence de hang/terminate, équipe jointe et, pour le tri, entrée
inchangée.

## P1 — lier le reçu au binaire et exercer le layout non vide

Le trajet fonctionnel du layout est présent, mais le selftest lifecycle ne
transporte encore que `FRONTIER_LAYOUT=''`; la scène `classic` appelle le
runner directement. Ajouter une scène lifecycle canonique `classic`, puis une
mutation cohérente plan/statut/argv en `csr` alors que le canon reste
`classic`, donne la preuve causale manquante.

La commande frontière accepte encore n'importe quel binaire `\S+`. Sur un
reçu rehashé, remplacer tous les exécutables frontière par `/tmp/rogue` laisse
le verdict inchangé. Le chemin attendu est déjà autoritaire dans
`BIN_MATRICE`; le regex doit exiger exactement cette valeur dérivée du canon.

## P1 — annoncer seulement la grille réellement mesurée

La grille contient quatre points `uniform/K5`, trois `terrain/K5`, deux
`uniform/K10` et aucun `terrain/K10`. Elle ne mesure donc ni « quatre tailles
par famille et par K », ni le plus grand `n` tenant en mémoire. Elle peut
rapporter les sécantes échantillonnées, le plus grand `n` **testé** qui
complète, et un bracket uniquement si succès et échec existent sous le même
pin, le même layout et le même plafond. Corriger cette pré-inscription est
préférable à ajouter des runs qui ne tiendraient plus dans la fenêtre.

## Contre-lecture critique des exigences précédentes

Deux demandes de l'audit antérieur étaient trop catégoriques :

- interdire tout code 134 en plan v2 n'est pas nécessaire pour une simple
  frontière de **complétion**. Un signal 6 avec diagnostic `std::bad_alloc`
  et `RLIMIT_AS` attesté peut rester une borne haute non attribuée au processus,
  à condition de ne jamais le présenter comme un refus transactionnel ni comme
  l'étage coupable. Si la question devient l'attribution par étage, il faut le
  censurer ; cette distinction de claim suffit ;
- la fermeture totale de toutes les clés/tokens et la dérivation générale de
  v1/v2 sont souhaitables, mais ne falsifient pas à elles seules ce profil :
  son canon demande `layout=classic`, donc impose déjà v2. Elles passent en P2
  après les quatre fermetures ci-dessus.

Les faux verts génériques restent reproduits et doivent donc rester au backlog :
un plan rehashé accepte une clé inconnue, `threads` dupliqué et la ligne fixe
`s=999 smax=2 seed=666`; un reçu legacy sans axe peut s'auto-promouvoir en v2
en ajoutant des champs cohérents. Fermer `read_plan` par ensembles exacts de
clés et tokens, unicité et ligne fixe littérale évitera leur retour. Le refus
pré-run d'un pilote non vide sans inventaire de portes est aussi utile au
protocole générique, mais Q2 est désarmée ici et ne bloque pas cette session.

## P1 — rendre le selftest total et stable sous `pipefail`

Sur un export exact de `d8d7a7f7`, le selftest campagne s'arrête par code 141
après le témoin `decision_complete`. La commande
`onest=$(ls "${DVIDE}"/*.status | head -1)` reçoit le SIGPIPE de `head` sous
`set -o pipefail` ; la contre-fixture `time_bin` vide qui suit n'est jamais
exécutée. Un rejeu peut passer selon le volume et le buffering de `ls`, ce qui
en fait une porte instable, pas une preuve 171/171. Sélectionner le premier
fichier par une expansion shell ou une boucle sans pipeline, puis exiger le
code terminal 0 et le témoin final, ferme ce défaut sans toucher au protocole.

Le lifecycle, rejoué depuis le même export initialisé en dépôt Git local,
termine en revanche par code 0 ; les deux échecs D9/D9bis vus dans le worktree
partagé ne sont pas attribués au pin.

## Documentation à remettre en phase

`tests/bad_alloc_gate.cpp` documente encore l'ancien texte
`bad_alloc a l'etage` et affirme que le fold serait le seul étage à workers ;
génération, RLE/tri, préfiltre, census et expansion utilisent eux aussi les
pools. Corriger ces commentaires avec les portes ci-dessus évite de conserver
un contrat historique faux.

## Fermeture minimale avant nouvelle dépense

1. Supprimer le pipeline `ls | head` et prouver que la dernière contre-fixture
   du selftest campagne est effectivement atteinte.
2. Cause non allouante + renderer commun, grammaire totale du code 2 et vraie
   scène CLI → runner → validateur sans payload.
3. Lancements partiels sûrs pour ranges/items, tri à sas et worker B, avec
   injections ciblées.
4. `BIN_MATRICE` exact et fixture lifecycle `FRONTIER_LAYOUT=classic` non vide.
5. Pré-inscription réduite aux points et brackets réellement présents, puis
   rejeu des selftests sur le commit source propre.

## Contre-vérification locale

- export exact `d8d7a7f7`, `selftest_campagne_v6.sh` : code 141 après le
  témoin positif ; la dernière contre-fixture n'est pas atteinte ;
- même export initialisé en dépôt Git local, `selftest_cycle_vie_v6.sh` :
  code 0, sans ligne d'échec ;
- intégration Python du lifecycle : 2/2 verte lors du rejeu indépendant ;
- syntaxe shell/Python et `git diff --check` : propres au snapshot contrôlé.

Aucun résultat G4, aucun claim produit et aucun GO GCP ne sont créés par ce
préflight.

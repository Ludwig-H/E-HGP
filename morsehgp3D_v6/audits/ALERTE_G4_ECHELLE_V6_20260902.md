# Préflight statique — profil G4 échelle v6

Date : 2 septembre 2026. Pins jugés : réponse documentaire `fec58e1f`, capture
moteur `9243d69f`, puis correctif moteur `28d02459`. Le profil et le protocole
encore décrits comme WIP sont postérieurs et non attribuables à ces pins.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Audit strictement statique : GCP non utilisé, aucune cible externe interrogée
ou certifiée. Toute session éventuellement ouverte par Claude reste sa cible ;
elle n'est ni adoptée ni arrêtée ici.

## Verdict utile

Le **NO START reste actif**. `28d02459` ferme réellement trois défauts du
premier moteur : texte compatible avec les classes du validateur, réservation
sous la garde avec une quatrième porte, et portée exacte des callbacks
provisoires. Il ne livre toujours aucun profil ni fichier `gcp-migration/` ; le
raccord de campagne reste un WIP non épinglé.

La direction est bonne : timeout classé comme observation censurée, layout
`classic` annoncé, plan v2 pour les axes nouveaux, `RLIMIT_AS` ramené à
168 Gio et refus mémoire destiné à être typé. Le WIP postérieur porte déjà
`smax` dans le plan, le nom, l'argv et le statut, et aligne les inventaires sur
17 fichiers. Il faut maintenant fermer les raccords ci-dessous avant de
demander un GO.

## P1 — rendre le profil canonique effectivement exécutable

1. Aux pins jugés, le profil déclare `FRONTIER_LAYOUT=classic` sans raccord
   lifecycle. La copie WIP observée après `4d79dbd3` ferme maintenant le trajet
   fonctionnel profil→lifecycle→SSH→runner→plan/statut/argv→validateur et lie le
   plan v2 au profil épinglé. Cette correction reste non attribuable tant
   qu'elle n'est pas commitée. Il lui manque encore sa preuve causale : le
   selftest lifecycle transporte seulement `FRONTIER_LAYOUT=''`, tandis que la
   scène à valeur non vide appelle directement le runner. Ajouter une scène
   `classic` de bout en bout, puis muter ensemble plan, statut et argv en `csr`
   tout en gardant le canon `classic` ; elle doit refuser avant toute dépense.

Le WIP a en revanche correctement simplifié la question :
`GPUV6_GATE_NAMES=aucun` **et** `GPUV6_PILOT_SPECS=aucun`. Q2 est désarmée et
renvoyée à une session distincte ; Q1 ne peut plus être consommée par une
phase optionnelle antérieure. Conserver dans le protocole générique le refus
pré-run `pilot_specs != aucun && gate_names == aucun` évitera de réintroduire
silencieusement ce cas dans un prochain profil.

## P1 — joindre le refus mémoire au reçu

Au pin `9243d69f`, le moteur rend code 2 avec
`REFUS resource_exhausted : bad_alloc a l'etage ...`, tandis que le validateur
inchangé classe explicitement tout `bad_alloc` sous code 2 comme contradiction.
Le cas recherché invalide donc le reçu. De plus, `rr.message.reserve(256)` est
encore avant le `try` : son propre `bad_alloc` échappe à la promesse « jamais
un abort ». Les 188 portes rapportées prouvent le moteur, pas le trajet
CLI→runner→validateur. `28d02459` corrige les deux défauts moteur et porte les
portes dédiées à 4/4.

Le WIP protocolaire va aussi dans le bon sens : sa sous-classe reconnaît le
texte exact et recoupe l'étage et les cinq RSS entre les deux lignes. Elle ne
recoupe toutefois pas encore une cause machine-readable : changer seulement
`a l'etage` en `au stade` déclasse le même corps en refus de capacité ordinaire,
même avec une ligne `refus_etage` contradictoire, et le validateur rend vert.
Fermer la liste des refus ordinaires ou publier une cause séparée, puis exercer
le trajet CLI→runner→validateur. Enfin, versionner la politique : si la doctrine
est désormais « un abort n'est pas une donnée », le plan v2 doit refuser ou
censurer le code 134 ; le rejeu des reçus v1 peut conserver sa règle historique.

Le résidu moteur est borné. Après un `reserve(256)` réussi, l'assignation du
diagnostic tient dans la capacité provisionnée. En revanche, une interposition
qui fait échouer durablement cette réservation produit bien statut 2 et
`stage=entree`, mais le message reste vide ; le vrai CLI imprime `REFUS ` et le
validateur le refuse. Le mutant qui lève une fois avant la réservation ne simule
pas ce cas. Porter aussi la cause dans un champ sans allocation, puis faire
formater la ligne exacte par le CLI, ferme ce dernier secours. La fabrication
du nuage, située avant `run_pipeline`, reste par ailleurs hors capture : une
panne injectée là donne encore l'abort 134. Borner la promesse au pipeline ou
ajouter une garde CLI explicite.

Une seconde ouverture peut encore produire exactement l'abort que la session
cherche à remplacer. Dans `parallel/pool.hpp` et `parallel/sort.hpp`, si une
création intermédiaire de `std::thread` échoue, les fils déjà construits restent
joignables pendant le déroulage et le destructeur appelle `std::terminate`.
La recommandation précédente « garde stop+join » était incomplète pour le tri :
ses travailleurs déjà lancés peuvent attendre une barrière dimensionnée pour
une équipe qui ne sera jamais complète, et le join bloquerait. Pour
`parallel_ranges/items`, une garde RAII armée avant la boucle, puis stop+join,
suffit. Pour `parallel_stable_sort`, retenir d'abord les travailleurs derrière
un sas de départ ; sur échec de lancement, poser abort, ouvrir le sas et joindre,
sans laisser aucun travail atteindre la barrière. Convertir seulement les
`std::system_error` des constructeurs de fils en une cause non allouante
`worker_start`, puis la rendre comme `resource_exhausted` à l'étage courant.
Le `BJoiner` du fold rend déjà sa destruction sûre, mais son constructeur de fil
doit employer la même cause typée.

La fermeture minimale commune est un champ non allouant de `RunResult`, par
exemple `none | heap_allocation | worker_start`, et un helper CLI qui formate
les deux refus depuis cette cause, l'étage et les RSS avec `fprintf`, sans
dépendre de `rr.message`. Ce dernier reste un diagnostic best-effort. Les portes
utiles injectent l'échec au deuxième fil de chaque pool, exigent absence de
hang/terminate et équipe jointe ; pour le tri, elles exigent aussi entrée
inchangée. Une porte subprocess doit vérifier code 2, stdout vide et stderr
exact : `run_expect.cmake` ne contrôle actuellement que stdout.

Enfin, conserver le contrat historique précis : les callbacks déjà appelés
sont **provisoires jusqu'au statut terminal** ; l'invalidation interne ne peut
pas reprendre un effet externe. La porte K=1 prouve zéro callback seulement
pour une panne antérieure au premier callback.

Pour l'API bibliothèque, un `std::bad_alloc` lancé par `on_fold_phase`,
`on_forest` ou `prefilter_census_override` est aussi capturé globalement et
attribué à l'étage interne courant. Cela ne bloque pas la CLI de la campagne,
qui n'installe pas ces hooks, mais l'origine devra rester distincte au prochain
jalon du contrat d'exceptions.

## Portée et budget à dire exactement

La grille WIP contient quatre points `uniform/K5`, trois `terrain/K5`, deux
`uniform/K10` et aucun `terrain/K10`. Elle ne mesure donc pas une pente sur
« quatre tailles par famille et par K ». Elle peut rapporter les sécantes
effectivement échantillonnées et le plus grand `n` **testé** qui complète sous
ce pin, ce layout et ce plafond. Sans bracket same-pin, ce n'est pas le plus
grand `n` tenant en mémoire.

Après désarmement de Q2 et réduction à cinq heures, le lifecycle WIP estime
10 390 s : frontière 10 300 et 9×10 s d'overhead. L'enveloppe de plafonds
vaut 10 800 s, ou 10 890 s avec cet overhead. La fenêtre calculée par le
lifecycle vaut bien 13 195 s : `18000 - 3905` de marge de rapatriement
effective, puis `-900` de build source. Le commentaire est donc juste sur la
fenêtre, mais appelle à tort 10 890 s la sortie de l'estimateur nominal.
Publier séparément **estimateur nominal**, **enveloppe** et **fenêtre** depuis
ce même calcul.

La copie WIP ferme désormais une des coutures de protocole : elle normalise
`fam:n` et `fam:n:11` avant le contrôle des doublons et avant l'émission, prouve
l'identité octet par octet des plans, puis refuse leur coexistence avant tout
artefact. Cette fermeture n'est reçue qu'après un pin propre.

Deux fermetures de protocole restent utiles :

- fermer réellement la grammaire v1/v2 : `read_plan` accepte encore clés et
  jetons inconnus ou dupliqués, ne juge pas la ligne fixe `s=8 smax=11 seed=3`,
  et la commande frontière accepte tout binaire `\S+` au lieu du
  `BIN_MATRICE` canonique. Une seule fixture combinée peut exiger l'unicité des
  clés/tokens et l'identité de la commande ;
- refuser avant tout run un pilote non vide sans inventaire de portes ; Q2
  étant désormais absente du profil d'échelle, aucun digest historique 50k
  n'est revendiqué par cette session.

## Rejeu indépendant du pin moteur

Sur le pin moteur courant `28d02459`, la contre-vérification locale donne 4/4
portes `mhgp6_bad_alloc_*`. Sur le WIP protocolaire, syntaxe shell/Python
propre, selftest campagne complet et selftest lifecycle complet sont verts ;
ce dernier n'appelle que des gardes factices et ne touche aucune ressource GCP.
La copie WIP ajoute la normalisation `:11` et le raccord fonctionnel du layout.
Elle ne contient encore ni trajet lifecycle causal avec layout non vide, ni
vraie porte de la sous-classe d'allocation. Grammaire, binaire, renommage du
refus et code 134 v2 restent des dents utiles du prochain lot, sans ouvrir un
nouvel audit.

## Fermeture minimale avant nouvelle dépense

1. Épingler le raccord `FRONTIER_LAYOUT` déjà présent dans le WIP et lui ajouter
   la fixture causale non vide sur tout le trajet lifecycle.
2. Fermer la cause d'allocation de bout en bout, son secours sans tas, la
   création partielle des pools — sas obligatoire pour le tri — et la politique v2 du code 134 ; conserver Q2
   désarmée.
3. Recevoir la normalisation `:11` déjà présente ; fermer le parseur de plan et
   lier le binaire, avec les contre-fixtures correspondantes avant tout run.
4. Requalifier la portée et le budget, puis rejouer les selftests locaux sur
   un commit d'implémentation propre.

Aucun résultat G4, aucun claim produit et aucun GO ne sont créés par ce
préflight.

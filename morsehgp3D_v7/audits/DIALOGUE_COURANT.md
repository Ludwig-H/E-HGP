# Échanges actifs avec le constructeur v7

État du 4 septembre 2026. Cadre : `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce fichier conserve les actions présentes. Une correction intégrée reçoit
son rejeu et son reçu avant de cesser d’être une objection. Les rapports
courants remplacent les commentaires périmés ; les contre-fixtures restent.

## M1 — Géométrie démontrée conditionnellement, qualification des primitives

La [réponse de composition](REPONSE_AUDITEUR_COMPOSITION.md) justifie sous S
l’ancrage direct, l’application par inclusion des facettes, les contacts
hors cœur et l’inertie des blocs hors fenêtre. La régularité globale n’est
pas une prémisse supplémentaire. La [preuve S1](S1_COURANT.md) compose le
propriétaire, le seed, les covers, le front, les témoins, les secteurs, la
corde, les cellules et les marges numériques jusqu’au RLE d’arité minimale.
Aucun de ces raccords géométriques ne reste ouvert dans cette composition.

Le constructeur a intégré ces résultats dans sa
[preuve horizontale](../docs/PREUVE_HORIZONTALE_COMPOSITION.md) et publié la
[cartographie des primitives](../docs/QUALIFICATION_S1_PRIMITIVES.md).
La contre-lecture de cardinal/Karras et du PGCD confirme les bornes de types
annoncées. La qualification restante porte sur trois contrats concrets :

- index, parcours, tris et clés conformes, avec leurs invariants et préconditions ;
- largeurs, signes, Cramer, PGCD et comparaisons larges sur tout le domaine u16 ;
- binaire64, conversions, FMA, arrondi au plus proche et graphe de calcul stable sur la commande livrée.

Les preuves conditionnelles et les exécutions v7 doivent décharger ces
obligations nommées. Une nouvelle campagne géométrique ne les remplace pas.
La verticale, le rendu et les coûts conservent leurs contrats distincts.
Le [retour mathématique](RETOUR_MATH_COURANT.md) fournit les fixtures de
frontière de rang et leurs mutants, sans assimiler arité minimale et shell.

La précision rédactionnelle sur le PGCD est intégrée à la cartographie :
il produit le représentant entier primitif de la demi-droite positive des
coefficients (a,b,c). Aucun correctif supplémentaire n’est demandé sur ce point.

## A1 — Correction d’archive levée sur le produit intégré

Le constructeur a remplacé le nettoyage par suppression bornée sans
allocation ; `archive.hpp` porte le SHA
`cc2243aaa1bdbe63b69f165d65152cf62d7fac32ff6c641343542c247d989430`.
La probe indépendante inchangée revient en code 0 sous panne persistante,
sans terminaison ni résidu. Les quatre portes archive/API/cleanup passent,
avec dix ordres, les callbacks réels K1/K2 et les erreurs OS exercés.
Les 26 scènes CLI et six corruptions rescellées passent sur le nouveau CLI.

Autorité : [retour d’archive](RETOUR_ARCHIVE_COURANT.md),
[revue du nettoyage](REVUE_NETTOYAGE_ARCHIVE_COURANT.md) et
[rejeu des interfaces](AUDIT_INTERFACES_20260904.md).
La seule adaptation de test déplace son littéral `/tmp` sous `audits/` ;
le code d’archive, la probe indépendante et le CLI sont inchangés dans la
copie. Aucune correction A1 supplémentaire n’est demandée.

## C1 — Classification et enregistrement CTest levés

La [classification courante](CAMPAGNE_INCIDENCES_COURANTE.md) passe ses sept
tests normaux et optimisés, plus un vrai refus K=2 par budget de supports.
Une observation achevée avec refus conserve zéro succès moteur ; les motifs
inconnus, invariants et sorties incohérentes restent invalides.

Les deux invocations sont maintenant enregistrées avec le label `gate`
et un timeout de 120 s. Configuration Release indépendante puis **2/2 CTests
réussis**, code 0 : [reçu d’enregistrement](receipts_20260904/campaign_registration_current.json).
Aucune modification CMake supplémentaire n’est demandée pour C1.
Le [lanceur apparié courant](CAMPAGNE_APPARIEE_20260904.md) passe aussi ses
portes normales et optimisées pour K=5/K=10, les rôles v6/v7 ou v7/v7 et
les séparations 8/10/12. Ces tests de protocole emploient des exécutables
factices ; ils ne mesurent pas le moteur ni le strict mono-thread.

## R1 — Admission mémoire et priorité mono-thread

Le [retour mémoire](RETOUR_MEMOIRE_COURANT.md) fournit deux refus évitables,
les digests et la coexistence mesurée des tampons. Proposition au
constructeur : borner une fois la concurrence par
`min(fold_inflight, kmax_eff)` dans les deux gardes. Précontrôler séparément
les ajouts silencieux avant leur construction. Les coefficients de marge
restent conservés ; proxy logique, capacités et RSS restent distincts.

Le [contrat produit actuel](../docs/CONTRAT_PERFORMANCE.md) vise 50k points,
toute la tour K=1..10 sous une seconde, avec repli K=1..5, puis 100 ms sur
le même périmètre. L’ordre est mono-thread, multi-CPU, puis GPU ; l’échelle
massive cible GCP G4. Ces seuils ne sont pas des résultats acquis.
Le constructeur vient d’intégrer le fold inline pour `threads=1, fold_join=1`.
La [qualification indépendante du delta](MONO_COURANT.md) passe ses quatre
portes : absence de création de threads dans les scènes mono, conservation
des deux sémantiques et des refus tardifs. Les 26 scènes et six corruptions
d’interface repassent sur le nouveau CLI.
L’ancienne création systématique du thread B n’est plus le code courant.
Point de diagnostic pour le constructeur : `peak_fold_inflight` et
`pic_workers_b` comptent l’activité du corps B, y compris inline ; ils ne
sont donc pas un compte de threads natifs. Le compteur interposé de
`pthread_create` est le témoin adapté à cette qualification. Les effets
thread locaux des callbacks, désormais sur l’appelant, restent à déclarer.
Le constructeur a intégré la mise en cache de l’argmin entier dans
`AxisBounds::axis_min`. Sa [revue indépendante](CENSUS_AXIS_COURANT.md) est
statique ; les tests et les gains de ce dernier delta restent à qualifier
séparément du CLI mono déjà exécuté. Aucune compilation supplémentaire
n’est lancée par l’auditeur pendant la préparation de la campagne.

## Preuves exécutées et coordination

La reconstruction indépendante complète passe **279/279 portes CPU Release**
du snapshot initial, avec 203 fichiers et 31 binaires stables :
[validation](receipts_20260904/iteration2/validation.json).
Les changements de banc et d’archive ont leurs reçus ciblés ; ils ne
réécrivent pas ces résultats. Les 46 tests d’échelle sont hors de cette suite.
Les durées sur l’hôte partagé ne sont pas des performances industrielles.

Le constructeur annonce sa campagne appariée terminée : 36 tentatives,
18 paires, 15 paires achevées identiques et cinq censures dans les trois
paires restantes ; statut global `invalid`, sans score de performance.
Cette déclaration reste celle de son reçu, pas une mesure indépendante
refaite par l’auditeur. Aucun autre run lourd n’est engagé ici.

Les écritures de l’auditeur restent dans `morsehgp3D_v7/audits/`, sur `main`.
GCP non utilisé par l’auditeur ; aucun résultat GPU ne provient de ses tests.

## Constructeur — delta mono C en cours de qualification

Le constructeur prend acte des levées indépendantes A1 et C1 ci-dessus.
La précision PGCD a été intégrée dans la cartographie : canonisation de
la demi-droite des coefficients, et non du rayon géométrique.
Le reçu `../receipts/release_delta2_20260904/summary.json` rattache B aux
21 portes fraîches et aux 261 portes réutilisées sur binaires et
dépendances inchangés ; ce n'est pas une suite complète fraîche B.

Le prochain delta C conserve B séparément et combine deux changements
bornés : fold exécuté dans le thread appelant lorsque `threads=1` et
`fold_join=1`, puis argmin entier quadratique clippé dans AxisBounds.
Une porte interpose réellement `pthread_create` et conserve les refus
tardifs, callbacks provisoires et digests ; l'oracle AxisBounds énumère
les intervalles avec OBig indépendant. Leur intégration est en cours,
sans revendication de qualification C tant que les reçus manquent.

Le banc est prêt pour B/C mono, tour 1..10, n=8000, seed=3, s=8/10/12.
Merci de ne pas lancer de compilation ou de test lourd pendant cette
première campagne ; les heures et SHA seront ajoutés à son lancement.
Les documents et audits peuvent continuer, hors modification des sources,
CMake ou du banc consommés. Une qualification complète C suivra les
mesures. Aucun nouveau démarrage GCP n'est engagé à cette étape.

Coordination auditeur : la reconstruction du CLI mono et les replays sont
terminés. Aucun build ni test lourd n’est encore engagé par l’auditeur.
Le delta AxisBounds intégré ensuite est relu séparément et n’est pas
assimilé à la qualification exécutée du CLI mono ; le manifeste distingue
explicitement les sources relues et les binaires testés.

Campagne B/C démarrée le 4 septembre à 22:01 UTC, affinité CPU 6 :
`../receipts/mono_bc_20260904/metadata.json`. B conservé :
`fa917eefd8198d8ee676585dd99401f74594dd33a4bf77e1265ef397f439e200` ;
C : `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`.
La construction de C a ses sources/flags/reçu sous
`../receipts/mono_c_build_20260904/`. Les dix portes mono + AxisBounds
combinées passent avant ce lancement. Six processus, timeout 600 s chacun,
n=8000 uniforme seed=3, tour entière 1..10, s=8/10/12. Attendre le statut
terminal avant toute compilation ou mesure lourde sur cet hôte.

Le premier couple s=8 est achevé : B 127,997 s, C 105,932 s, digests et
cardinalités égaux. Ce ratio isolé ne qualifie pas un gain statistique ni
le contrat 50k ; s=10/12 sont encore en cours. Le constructeur prépare un
commit de jalon sur `main` avec le code C, les portes et les reçus déjà
clos. Les fichiers de campagne ouverts seront exclus de ce commit ; le
statut restera `not_claimed` et la suite Release C fraîche restera à
exécuter après la campagne. Merci de laisser l'index Git au constructeur
pendant cette publication ; aucune source consommée ne sera modifiée.

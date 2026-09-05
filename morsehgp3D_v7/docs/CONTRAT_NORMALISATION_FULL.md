# FULL : normalisation des successeurs, calendrier v2

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Delta et preuve d'état

Le helper interne `full_gabriel_detail::normalize_successor` est consommé
par les deux politiques du [producteur horizontal](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md).
Il ne change ni les catalogues, ni les demandes de facettes, ni first-C,
ni les lots atomiques, ni les identités de composantes. Il ne construit
aucune cellule de Delaunay, coface silencieuse ou structure globale nouvelle.
Le tableau de successeurs appartient au constructeur : ses IDs et son
acyclicité ne sont pas un format externe non fiable.

La [preuve indépendante](../audits/CACHE_FULL_COURANT.md#normalisation--supprimer-la-dernière-paire-redondante)
précède ce delta. Lors de la recherche de racine, retenir le dernier nœud
non racine. Sa case pointe déjà vers la racine finale ; la deuxième passe
s'arrête donc avant cette case. Toutes les autres écritures sont identiques,
y compris une écriture idempotente plus tôt dans le chemin. Après un appel
réussi, **tout le tableau** est identique à celui de l'ancien algorithme.
Par induction, les appels suivants rencontrent les mêmes chemins tant
que les deux exécutions réussissent.

À profondeur zéro, la lecture de la racine reste obligatoire. À profondeur
un, aucune compression n'est nécessaire, mais `normalized_anchors` augmente
tout de même après la lecture terminale réussie. La racine publiée, les
forêts et les 32 compteurs autres que `successor_steps` sont conservés
sur les ordres appariés complets. Ceci n'est pas une preuve de complétude
géométrique des catalogues fournis.

## Unité de travail et refus

`FullGabrielResult::successor_accounting` porte
`full_successor_reads_writes_no_last_pair_v2`, aussi en cas d'échec.
Ce marqueur ne renomme ni l'autorité FULL ni les politiques eager/lazy.
Le calendrier historique implicite est `full_successor_reads_writes_v1`.

| Profondeur initiale d | Ancien travail v1 | Nouveau travail v2 |
| --- | --- | --- |
| 0 | 1 | 1 |
| 1 | 4 | 2 |
| 2 | 7 | 5 |
| 3 | 10 | 8 |
| d>0 | 3d+1 | 3d−1 |

Chaque lecture et chaque écriture restante est chargée **avant** l'opération.
Aucune dépense n'est soustraite après coup. Les plafonds numériques sont
inchangés ; leur frontière d'admission change explicitement. Les unions
locales, installations de nœuds et opérations hors normalisation ne sont
toujours pas facturées comme accès de successeurs.

Pour deux ordres tous deux réussis aux mêmes chemins, le nouveau compteur
est l'ancien moins deux fois `normalized_anchors`. Cette identité n'est
pas appliquée aux préfixes refusés. Un refus après la lecture terminale
peut déjà avoir incrémenté `normalized_anchors`, sans avoir terminé la
compression. À profondeur deux et plafond quatre, la première lecture
de compression réussit, mais son écriture refuse sans mutation de la case.
Tout refus du constructeur reste transactionnel : aucune forêt partielle
valide n'est rendue.

La sonde nouvelle utilise le schéma `mhgp7-full-gabriel-probe-v3` et porte
le calendrier dans sa configuration, ses ordres et son terminal. Les
anciens reçus v2 restent immuables et se lisent avec leur calendrier v1.
Le digest de forêt ne change pas de domaine : il identifie un objet,
pas la comptabilité de sa construction.
La sonde historique `bench/full_gabriel_probe.cpp` est explicitement
non recompilable dans le chantier courant : un `#error` nommé empêche
son ancien schéma implicite de recevoir des charges v2. Ses octets
historiques restent récupérables dans Git, avec leurs pins, binaires
et reçus inchangés. Cette fermeture répond à la contrelecture de l'auditeur.

## Portes du delta

Le header `85c27ab9…` et le gate `68815ac2…` passent la
[qualification fraîche](../receipts/full_gabriel_successor_20260905/README.md) :
20/20 CTests Release et 20/20 ASan/UBSan, LeakSanitizer actif. Huit binaires
par build, carte stable de 585 pins, dont les 521 headers Boost de l'oracle
du digest. Les 17 portes antérieures sont réexécutées, pas la suite F complète.

La primitive est jugée par un oracle indépendant des traces d'accès :
560 cas de préfixe aux profondeurs 0/1/2/3/7/31, puis ancres inconnues,
rappels et budgets cumulatifs, soit 1 242 appels des deux calendriers.
Le tableau entier, la racine partielle, les charges et l'incrément des
ancres sont contrôlés. Parmi les refus v2, 156 surviennent entre la
lecture de compression et son écriture, qui ne doit pas être effectuée.

Le différentiel FULL conserve l'ancien calendrier **uniquement** sous
`MHGP7_TESTING` ; il n'est pas une option produit et ses retours portent
explicitement v1. Sur neuf nuages, 39 ordres et quatre politiques, 180 paires
réussies conservent la forêt et les 32 autres compteurs. L'oracle Gamma
indépendant juge 3 320 coupes. Les 120 ordres nominaux de profondeur
positive évitent la vacuité de l'identité de travail ; 3 000 opérations
sont économisées sur ces seuls témoins, sans conversion en temps CPU.

Le mode budgets ferme 668 paires, 640 appels refusés, 192 plafonds exacts
hors successeurs et 280 plafonds courts. Les deux calendriers passent
32 plafonds de successeurs exacts et refusent 64 plafonds courts. Dans
16 sentinelles, v2 réussit à son coût exact et v1 refuse au même plafond.
Les balayages frais injectent 49 pannes d'allocation eager et 209 lazy,
toutes refusées sans échappement ni forêt partielle valide.

La première tentative a échoué à la compilation du nouveau test, sur une
ambiguïté de namespace dans `main`, avant tout CTest. Son reçu `70714475…`
est conservé tel quel sous `failed_qualification/`. Après correction du
seul nom qualifié, la tentative R2 repart de deux répertoires neufs et
ferme le reçu `49be3d72…`. Le paquet de 280 fichiers porte les sommes
`0e6c84ba…`. Aucun résultat de l'essai échoué n'est promu.

## Qualification indépendante du même header

Le [rejeu Gamma indépendant](../audits/receipts_full_successor_20260905/judgment_normal.json),
identique sous Python `-O`, vise le header
`85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad`.
Deux builds neufs O2 et ASan/UBSan produisent chacun 114 ordres,
912 sorties et 69 120 coupes. Les forêts littérales et les 32 autres
compteurs restent identiques au témoin singleton ; 744 sorties de
profondeur positive exercent l'identité de travail. Les plafonds v2
donnent 16 succès exacts et 180 refus cap−1, dont 16 sur les successeurs,
plus douze conflits d'API. Aucune identité de succès n'est appliquée
aux préfixes refusés. Le jugement est épinglé `b317f20c…`.

La [porte primitive indépendante](../audits/receipts_full_successor_20260905/primitive/README.md)
appelle le helper sans `MHGP7_TESTING`. Ses 3 851 appels par build couvrent
les tableaux monotones de taille 1 à 4, des chemins non contigus, une
profondeur 16, les rappels et des compteurs proches de UINT64_MAX.
Le tableau entier et les préfixes sont jugés, notamment 466 refus entre
lecture et écriture et 930 refus après incrément de `normalized_anchors`.
Les deux mutations causales — ancienne dernière paire et écriture avant
sa charge — sont réfutées sur 813 et 569 sorties respectivement.
Les jugements nominaux O2/SAN et normal/`-O` sont identiques
(`5487787b…`), sur les fixtures `281ec460…` ; l'oracle porte `495069f5…`.

La [contre-vérification des captures constructeur](../audits/receipts_full_successor_20260905/constructor_review_normal.json)
(`2ef078dd…`, identique sous `-O`) contrôle séparément les 20+20 CTests,
les pins, fautes d'allocation et l'échec initial conservé ; elle ne les
relance pas. Ces preuves qualifient le delta sur leurs domaines bornés,
relativement aux catalogues exacts, complets et réguliers fournis.
Elles ne qualifient ni les temps mono, ni le fournisseur géométrique,
ni la verticale, l'archive FULL ou la tour intégrée.

Les économies calculées sur d'anciens ordres clos ne sont ni une nouvelle
mesure de temps ni une prédiction de réussite du K9/32k précédemment refusé.
La [campagne mono propre à ce delta](RESULTATS_MONO_FULL_SUCCESSOR_20260905.md)
est maintenant close : 204 ordres réussis comparés aux témoins épinglés,
et un rejeu s8 séparé après exclusion du premier chrono concurrent.
Le 32k refuse encore K9, désormais sur les quatre millions d'appels MEB.
L'économie d'accès aux successeurs est établie, pas un gain de temps robuste.
Les contrats 50k, export/verticale FULL et G4 restent ouverts. GCP non utilisé.

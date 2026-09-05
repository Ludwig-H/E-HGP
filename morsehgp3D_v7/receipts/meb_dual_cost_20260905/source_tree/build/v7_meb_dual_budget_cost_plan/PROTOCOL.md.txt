# Coût local F contre MEB à double budget — préparation privée

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=protocole_cout_local_prepare`, `public_status=not_claimed`.
Note seulement : aucun C++, build, test, chrono, export public, Git ou GCP exécuté ici.

## 1. Autorités et porte avant mesure

- Lire puis épingler le reçu géométrique **réellement clos et conforme**, ses sources et ses planchers ; la préparation géométrique n'est pas une qualification.
- Le seul reçu de budget a7dc0020 ne suffit pas : triangle/cumul/MAX, mais aucune scène q4 exécutée dans ce reçu.
- Plan géométrique : `build/v7_meb_dual_budget_prototype/geometry_plan/PROTOCOL.md`, SHA256 `3e21a2066934923732375a65329b61d2f3bde73dd0ac5b546f4becb516de6f03`.
- Ses dépendances : `geometry_plan/pins.json`, SHA256 `713c0ff3298ad01236834be641cee3a3f412f004d66b36d4b33f1d8536430c67` ; `additional_scenes.json`, SHA256 `f66008d3a2dc80532d3380cf490d8e87412b66ca2be8ae68066fb88cf604eed4`.
- Proposition nominale : `build/v7_meb_dual_budget_prototype/pivot.hpp`, SHA256 `0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d` ; dépendance historique de formes d6dbba19 conservée, sans appeler son ancien proposeur.
- F : `silent_detail::Builder::miniball` effectivement inclus depuis `silent_incidence.hpp` f75a136a ; fermeture complète du reçu `build/v7_f_build_20260905/build_D.json`, SHA256 `522c950c70b60ca58759c4fa9b9a24ff995fe829b9aa1adf5b2f51b7b2177ac4`.
- Protéger sans lancer le CLI F `build/v7_f_qualification/mhgp7`, SHA256 `ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85` ; ne pas chronométrer la référence C copiée de `meb_lazy_gate.cpp`.
- La note mathématique `morsehgp3D_v7/docs/PROPOSITION_MEB_ET_BUDGETS.md` 365e7a5d distingue preuve conditionnelle, compteur ordinal et travail physique ; ses noms D historiques ne remplacent pas F.
- Tout changement de ces autorités exige une nouvelle revue ; pas de repin silencieux. Futurs harness/runner, options, fermeture compilée et binaire doivent être scellés avant un GO d'exécution distinct.

## 2. Entrées et comparaison fermées

Réemployer les données du probe 42f5cde4 et du gate 242513ee, jamais leurs résultats historiques :
8 scènes explicites + les 160 premières scènes du LCG fixé + les 8 compléments JSON = 176 scènes.
Ordre initial et inverse, puis les 32 permutations nommées q2/q3/q4 : exactement 384 ordres, n2..11.
Les deux compléments q2 du lazy gate sont déjà dans les huit JSON ; ne pas les ajouter deux fois.
Index admis, coordonnées/PointId/slots, doublons explicites et ordre du corpus restent identiques au juge géométrique ; aucun resampling, point ajouté ou support optimal injecté.
Hors chrono : un appel F frais par ordre détermine R au cap 551 ; il ne renseigne pas la proposition.
Matrice principale inchangée : P dans {0,1,4,5,15,16,25,401}, L dans {R−1,R,R+1}, pivot_cap=16, soit 9 216 comparaisons.
Conserver également le bloc géométrique fermé de frontières (au plus 128) ; total différentiel au plus 9 344.
Les 1 507 ordinaux et les mutants sont des juges hors chrono, pas des appels MEB à additionner au coût.
Avant et après les lots, juger chaque cas : booléen, statut/raison, **toutes** statistiques legacy, événements, BallKey entière, q/support complet, niveau trois limbs et dénominateur, sentinelles sur refus.
Exiger aussi identité des nouveaux compteurs entre rejeu observé et rejeu nominal ; une divergence interdit toute comparaison de vitesse.

## 3. Deux cohortes : appel frais et compteur persistant

Appel frais : chaque répétition représente une tentative indépendante, même état initial F/dual ; remettre en place cet état dans les deux bras et inclure ce coût symétrique dans le lot mesuré.
Un `Work` neuf est permis seulement à cette frontière de tentative, jamais à l'intérieur du helper ou lors du repli.
Cumul : conserver distinctement la séquence géométrique de quatre appels triangle P7/L12, et son contrôle P0 ; `Work` et statistiques persistent entre les quatre appels, réinitialisés uniquement entre séquences.
Chronométrer la séquence entière, jamais quatre appels faussement frais ; les frontières MAX/non nulles restent identifiées à part.
Ne pas répéter un appel avec des compteurs oubliés puis mesurer essentiellement le garde « budget déjà épuisé ».
Le contrôle P0 mesure explicitement l'enveloppe de repli ; il ne vaut pas neutralité automatique de cette enveloppe.

## 4. Comptabilité physique et catégories

Déclarer `meb_work_accounting=reference_ordinal_plus_proposal_v1` ; noter L/P les plafonds, Δc/Δp leurs charges effectives par appel.
Dans le wrapper actuel, Δfallback vaut 0 ou 1 : si 1, A=Δc compte les candidats F réellement essayés en repli ; sinon A=0, même lorsque Δc est un ordinal positif.
Le coût physique en candidats de ce bras est **A+Δp**, avec Δp incluant les formes proposées rejetées. Le F comparateur consomme séparément Δc_F candidats.
Vérifier cette attribution contre l'observation géométrique ; ne pas y inclure les appels F de calibration R ou de comparaison, ni assimiler certified à un succès.
La borne en incréments A+Δp<=Δc+Δp reste une borne de candidats ; les compteurs MAX injectés ne sont pas du travail passé mesuré.
Sélection des distances, tests de puissance, copies de Candidate, ordinal et finalisation restent du temps réel non décrit par ce seul nombre de formes ; ne pas annoncer une borne en instructions/RAM/latence.
Chaque ligne garde scene_id, ordre, n, R, P/L, compteurs initiaux, terminal F, terminal dual, Δc/Δp/A, pivots, certified/fallback et nombre d'appels réellement mesurés.
Stratifier par n, q de référence, P/L, succès/refus et route : certificat accepté, certificat puis refus legacy, garde legacy immédiat, repli initial P épuisé, autre repli.
q de calibration est un champ distinct ; sur refus laissant une sentinelle, le q de résultat est non applicable, jamais le q artificiel de cette sentinelle.
Pour les replis milieu de proposition, coquille, géométrie ou plafond de pivots : publier une cause seulement si le passage observé la distingue réellement ; sinon « repli non attribué ».
Le passage causal/tracé est hors chrono. Le nominal mesuré reste `ChargeAfter=false`, `NoObserver`, sans MHGP7_TESTING, sanitizer ni mutant ; conserver les compteurs Work normaux.

## 5. Lot q2 immédiat obligatoire

Utiliser la paire extrême déjà fixée [(0,0,0),(65535,65535,65535)] : n=2, R=1, ses deux permutations, P={0,1,401}, L={1,2} ; 12 cas, sans nouveau point.
Ce lot est une répétition explicitement supplémentaire de cas existants, pas une extension cachée du corpus.
Comparer chaque cas directement à F : temps absolu par appel et ratio apparié. La paire F immédiatement acceptée peut subir un surcoût de recherche/copie/certification ; c'est une hypothèse, pas un résultat.
Garder séparément le q2 tardif n11/R55 du JSON : la seule arité q2 ne décrit pas le travail économisable.

## 6. Chronométrage futur borné et observable

Fenêtre locale mono dédiée à faire confirmer par root : CPU6, un thread, aucun moteur/build concurrent ; relever CPU, compilateur, environnement et affinité réelle avant/après.
Compiler les deux bras dans un même harness avec les mêmes options explicites C++20/O2 strictes, sans LTO ni instrumentation ; ce coût local O2 ne remplace pas une comparaison de CLI Release.
Construire l'index et charger les tableaux à l'exécution avant mesure ; ne pas fournir aux bras les résultats/certificats de calibration.
Appels via une frontière non inlinée, pas de spécialisation par scène ; barrière compilateur entrée/sortie et consommation observable **de chaque résultat** empêchent suppression ou remontée hors boucle.
Un accumulateur dépendant des champs produits est consommé après le lot ; le juge complet hors chrono reste obligatoire et un checksum seul ne le remplace pas.
Relire le code et le désassemblage des boucles avant GO : ni dernier résultat seul, ni checksum prédit depuis F, ni boucle constante remplacée par une multiplication.
Mesurer par lots à l'horloge monotone, pas un appel chronométré isolément ; noter coût de résolution/lecture et signaler les lots trop courts pour un ratio interprétable.
Les strates sont déterminées par les règles du § 4 après qualification, avant temps ; chaque cas de la matrice apparaît une fois par passage, sans pondération par gain observé.
Deux passages de chauffe fixes puis **sept passages mesurés** ; bras F→dual aux passages impairs, dual→F aux pairs, y compris chauffe comptée séparément.
La matrice et les frontières font chacun un appel par cas/passage ; les quatre appels cumulatifs restent une unité séquentielle. Le lot q2 fait exactement 4 096 répétitions par cas/passage.
Fixer au plus **2 000 000 appels MEB** pour toute l'invocation, calibrations/juges/chauffe compris ; contrôler l'inventaire avant lancement, sans augmenter ce cap automatiquement.
Proposer une échéance de mesure cumulée de **120 s**, groupe enfant et fermeture compris, distincte d'un futur build borné et relu ; si insuffisante, conserver la campagne incomplète, sans sous-corpus ni rallonge implicite.
Aucune adaptation du nombre de répétitions au ratio ni sélection du meilleur P. Reporter les deux bras bruts, médiane/dispersion des sept différences et ratios appariés par strate, pas seulement le meilleur temps.
Le coût commun d'état/barrière/checksum est inclus et décrit ; un contrôle de harnais peut le documenter, pas justifier une soustraction magique des temps.

## 7. Reçu et portée

Future destination create-only : sources et fermeture, GO/pins avant-après, commandes, options, depfiles, binaire, configurations, manifestes de cas, compteurs et temps bruts, stdout/stderr complets, statut terminal et sommes byte-exact.
Conserver refus, dépassements et interruptions ; toute différence de terminal, budget, inventaire ou hash ferme un échec sans claim de gain. Aucune réutilisation d'un temps antérieur comme bras frais.
Une baisse de candidats et une baisse de durée sont deux résultats distincts ; une moyenne de ce corpus synthétique ne donne pas la distribution des appels de la tour.
Pas d'extrapolation des répétitions chaudes vers 8k/16k/32k, 50k/1 s ou 100 ms, multi-CPU/GPU ou dizaines de millions ; pas de promotion de l'exactitude globale par accord local à F.
Ce protocole n'ajoute aucune structure globale et ne matérialise aucun catalogue/coface : il mesure uniquement le helper local et son enveloppe de coût déclarée.

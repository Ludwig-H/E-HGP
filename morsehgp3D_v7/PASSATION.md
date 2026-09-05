# État de livraison v7 — 5 septembre 2026

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Le chantier est sur `main`, sans branche supplémentaire. Ce document
sépare le code livré, les preuves exécutées et les contrats encore ouverts.

## Ce que contient la v7

La v6 a été lue et portée explicitement après lecture intégrale des deux
premières parties du manuscrit. Les
[octets consommés](docs/V6_SOURCE_SNAPSHOT.json) sont épinglés ; aucun
changement du worktree v6 n'est inclus dans le chantier v7.

La v7 ajoute une entrée u16 réelle avec identités conservées, une archive
transactionnelle à destination neuve, la complétion silencieuse candidate
avec descentes locales et plafonds explicites, et le fold horizontal
normalisé associé. Elle retire une copie globale de BallData au census,
utilise des permutations pour trier les gros objets, et sécurise l'admission
des threads avant leur travail. Elle ne construit ni Gamma exhaustif ni
la mosaïque de Delaunay d'ordre supérieur dans le chemin produit.

Le delta mono C ajoute un fold inline dans le mode sérialisé et le minimum
entier précalculé des paraboles du census. Le premier ne change pas les
options par défaut ; les callbacks du mode mono arrivent désormais sur le
thread appelant. Le second conserve les signes stricts du census et n'ajoute
que trois valeurs locales par requête, pas un cache global par point ou boule.

Le delta D ajoute la [matérialisation différée des MEB q3/q4](docs/OPTIMISATION_MEB_DIFFEREE.md) :
les candidats déjà rejetés par une puissance entière strictement positive
ne construisent plus leur clé primitive et leur niveau. Supports, charges,
caps et contrôles finaux restent identiques ; aucune structure globale
n'est ajoutée. Les [32 portes ciblées](receipts/meb_lazy_integrated_20260905/README.md)
passent en Release et sous ASan/UBSan. La
[paire mono complète](docs/RESULTATS_MONO_MEB_20260905.md) conserve toutes
les sorties et comptes, avec 225,75 s pour C et 172,67 s pour D sur
n=8000 étendu ; cela reste une observation, pas une qualification SLO.
La [qualification Release D complète](receipts/meb_full_release_20260905/README.md)
réexécute ensuite les 323 portes CPU : toutes passent, aucun saut,
sources et binaires stables. CTest prend 574,05 s après un build
incrémental explicitement déclaré de 232,82 s. Le binaire D qualifié
est `build/v7_meb_qualification/mhgp7` ; les anciens CLI C dans
`build/v7/` et `build/v7_c_qualification/` sont conservés comme témoins
historiques, pas présentés comme le binaire du nouveau delta.

Le delta E étend le [prétest entier à q2](docs/OPTIMISATION_MEB_Q2.md),
sans modifier l'ordre, les charges ni les contrôles finaux. La
[contrelecture rationnelle indépendante](audits/ADDENDUM_MEB_Q2_E_20260905.md)
confirme sa conservation locale. Les
[trois paires mono fraîches D/E](docs/RESULTATS_MONO_Q2_20260905.md)
à s=8,10,12 retrouvent les mêmes objets et comptes, avec une baisse totale
observée de 2,55 %, 0,04 % et 2,98 % respectivement. Une paire par s sur
hôte partagé ne qualifie ni un gain statistique ni un meilleur s ; le
défaut 8 reste inchangé. Le CLI E est
`build/v7_next_q2_qualification/mhgp7`, SHA `df751533…` ; ni les témoins
C/D ni leurs qualifications historiques ne lui sont réattribués.

La [qualification E intégrée](receipts/meb_q2_integrated_20260905/README.md)
ferme les 33 portes ciblées Release, les 33 portes ASan/UBSan et les
324 portes Release complètes, toutes exécutées fraîchement, sans échec
ni saut. Le build complet est incrémental (235,36 s), suivi de 627,84 s
CTest ; ses 140 sources et 37 binaires restent stables. Les inventaires,
JUnit et journaux intégraux sont contrôlés ; la fraîcheur de LastTest est
ancrée dans le même système de fichiers, pas supposée à partir du seul
horodatage UTC. Les premiers faux rejets du harnais restent publiés.

La [proposition MEB par pivots et ses budgets](docs/PROPOSITION_MEB_ET_BUDGETS.md)
reste hors produit. Un contre-exemple compilé montre que l'ordinal de la
référence ne borne pas le travail physique d'une proposition. Le nouveau
prototype possède une charge séparée et prospective, qualifiée localement
dans [deux reçus archivés](receipts/meb_dual_geometry_20260905/README.md) :
triangle causal puis 9 339 comparaisons géométriques à F et 1 507 ordinaux.
La [note de résultats](docs/RESULTATS_MEB_DOUBLE_BUDGET_20260905.md) distingue
ces observations Trace de la preuve et de l'oracle rationnel indépendants
de l'auditeur. Le raccord par ordre et ses schémas publics ne sont pas
encore livrés ; aucun gain de tour ne découle de cette qualification locale.

La [première mesure native](docs/RESULTATS_COUT_MEB_20260905.md) qualifie
séparément 9 351 états F/Trace/NoObserver avant et après sept passages.
Les 1 325 812 entrées MEB restent sous le plafond déclaré ; l'échec du
premier build et toutes les mesures sont archivés. Le raccourci réduit
le compte de candidats de la matrice P401, mais le q2 immédiat répété
ralentit d'environ 39–40 %, et le contrôle P0 des petits lots dépend de
l'ordre des bras. Aucun gain général ni seuil produit n'est établi.
Un suivi distinct à 64 répétitions et dix paires équilibrées est préparé,
sans mesure attribuée à ce stade ; les budgets y repartent à zéro par
appel, pas par ordre de tour. F et son format public restent inchangés.

Le delta F porte maintenant cette [pile locale de témoins](docs/OPTIMISATION_PILE_TEMOINS.md) :
64 entrées inline, puis un vector de secours sans troncature, dans le seul
`count_universal_witnesses`. LIFO, masques, seuils et statistiques restent
inchangés ; aucun catalogue global n'est ajouté. Le calendrier de
`bad_alloc` n'est pas promis identique à E, dont la pile alloue.
Le CLI F neuf est `build/v7_f_qualification/mhgp7`, SHA `ee29d3d5…`.
La [qualification intégrée F](receipts/witness_stack_integrated_20260905/README.md)
ferme **48/48 portes Release ciblées, 48/48 ASan/UBSan et 339/339 Release
complètes**, sans échec ni saut. Le build complet est incrémental (257,16 s),
suivi de 620,68 s CTest ; 143 sources et 39 binaires restent stables.
Les deux cibles de la nouvelle porte distinguent l'observateur d'allocations
et la vérification sémantique sous allocateur natif. Les
[trois paires mono fraîches E/F](docs/RESULTATS_MONO_F_20260905.md)
sont closes : s=8 187,677/188,969 s ; s=10 190,077/185,660 s ;
s=12 184,878/190,039 s. Les six comparaisons d'objets et de comptes
passent dans chaque paire, ainsi que les objets entre s ; deux régressions
et une amélioration ne démontrent aucun gain robuste. Le défaut s=8 reste
inchangé. Aucun ancien temps E n'est utilisé dans ces ratios.

Les [observations F séparées à 16k/32k](receipts/witness_stack_scale_20260905/README.md)
sont également closes, s=8 et tour candidate 1..10 demandée. 16k termine
en 413,816 s, pic RSS 5 361 880 KiB. 32k refuse à K=9 avec
`resource_exhausted`, raison `silent_core_record_budget`, code 2 après
569,876 s de tentative ; aucun timeout, digest de succès ni temps de tour
achevée ne lui est attribué. Les plafonds initiaux restent inchangés.
Les 55 397 230 facettes de la tour 16k sont un cumul entre ordres, pas
une mesure de résidence simultanée. Aucun de ces résultats ne ferme les
contrats 50k/1 s, 100 ms ou plusieurs dizaines de millions de points.

`verified_events_only` reste le payload par défaut. La route
`--complete-incidences` porte `normalized_horizontal_h0_candidate` et refuse
les dégénérescences pertinentes non prises en charge. `--require-exact`
refuse explicitement : il n'existe pas encore de produit globalement
qualifié exact derrière cette option.

## Autorités de qualification distinctes

| Snapshot ou campagne | Résultat exécuté | Portée et limite |
| --- | --- | --- |
| [Release A](receipts/release_20260904/summary.json) | 279/279 portes CPU | Sources et 31 binaires stables ; pas la qualification C |
| [Préparation B1](receipts/release_delta_20260904/summary.json) | Échec du contrôle de réutilisation avant CTest | Les variantes profil consommaient aussi l'archive ; échec conservé |
| [Delta B2](receipts/release_delta2_20260904/summary.json) | 21 portes fraîches vertes, 261 portes réutilisées | 26 binaires et dépendances inchangés ; pas 282 exécutions fraîches |
| [Construction C](receipts/mono_c_build_20260904/summary.json) | CLI Release construit, sources stables, B conservé | Construction seule, pas fermeture Release |
| [Release C fraîche](receipts/release_c_20260904/summary.json) | 292/292 portes CPU exécutées, zéro échec/skip | Build isolé ; sources et binaires stables ; CLI identique à C mesuré, aucune réutilisation A/B |
| [Correction du harnais CI](receipts/ci_sonde_environment_20260904/README.md) | Avant : trois erreurs reproduites ; après : quatre exécutions de 23 scènes vertes | Python normal/-O, environnement propre/hérité ; ce correctif ne change que le harnais Python, pas le moteur |
| [Portes arithmétiques intégrées](receipts/arithmetic_gates_20260904/README.md) | 24/24 ciblées Release et 24/24 ASAN/UBSAN ; puis 316/316 portes CPU fraîches, zéro échec/skip | Build complet incrémental déclaré, CTest 558,50 s ; sources et binaires stables ; CLI C inchangé |
| [Autorité Boost indépendante](receipts/arithmetic_boost_20260904/README.md) | 8/8 portes entières avec Boost 1.83 réellement compilé, plus 16/16 lanes | En-têtes privés extraits sans installation système ; OBig + littéraux restent l'autorité des lanes ; pas un second pipeline qualifié |
| [MEB D ciblée](receipts/meb_lazy_integrated_20260905/README.md) | 32/32 portes fraîches Release et 32/32 ASan/UBSan | Deux builds isolés ; Γ/API/archive/mono/refus et sept nouvelles portes ; aucune erreur sanitizer masquée |
| [Release D complète](receipts/meb_full_release_20260905/README.md) | 323/323 portes CPU fraîches, zéro échec/skip | Build incrémental distinct de C ; D, sources et 37 binaires stables ; pas de résultat C réutilisé |
| [Mono C/D complétée](docs/RESULTATS_MONO_MEB_20260905.md) | Deux runs achevés : 225,75 s puis 172,67 s, mêmes objets et comptes | n=8000 uniforme étendu, s=8, tour candidate entière 1..10 ; baisse observée 23,51 %, pas de SLO |
| [Mono D/E q2 s=8/10/12](receipts/meb_q2_mono_20260905/README.md) | Six runs achevés, trois paires égales ; digests et cardinalités identiques entre s | n=8000 étendu, tour candidate 1..10 ; baisse totale observée 0,04–2,98 %, pas de gain statistique ni de SLO |
| [Qualification E intégrée](receipts/meb_q2_integrated_20260905/README.md) | 33/33 ciblées Release, 33/33 ASan/UBSan, 324/324 complètes ; zéro échec/skip | Tests frais, build complet incrémental, 140 sources et 37 binaires stables ; aucun résultat D réattribué |
| [Qualification F intégrée](receipts/witness_stack_integrated_20260905/README.md) | 48/48 ciblées Release, 48/48 ASan/UBSan, 339/339 complètes ; zéro échec/skip | Tests frais, build complet incrémental, 143 sources et 39 binaires stables ; aucun résultat E réattribué |
| [Mono E/F s=8/10/12](receipts/witness_stack_mono_20260905/README.md) | Six runs achevés, trois paires égales ; objets identiques entre s | n=8000 étendu, tour candidate 1..10 ; aucun gain robuste, ni meilleur s qualifié |
| [Paliers F 16k/32k](receipts/witness_stack_scale_20260905/README.md) | 16k : tour 1..10 achevée en 413,816 s ; 32k : refus de ressources à K=9 | s=8, mono, sources stables ; tentative 32k de 569,876 s, pas un temps de complétion |
| [Mono B/C s=8/10/12](docs/RESULTATS_MONO_20260904.md) | Six runs achevés, mêmes objets ; C 105,1–105,9 s contre B 125,5–128,0 s | n=8000 uniforme, tour entière 1..10, objet Gabriel ; un seul couple par s, pas de SLO |
| [Campagne locale v6/v7 A](receipts/local_paired_20260904/summary.json) | 15 paires achevées identiques ; cinq censures dans trois autres paires | 36 tentatives à 8 threads, n=8k/16k/32k ; campagne globale `invalid`, aucun SLO |
| [Complétion locale](receipts/incidence_local_20260904/summary.json) | Six refus de domaine, zéro succès moteur | Observations achevées, pas capacité exacte à 8k/16k/32k |
| [Petite complétion](receipts/incidence_mini_20260904/summary.json) | Un succès candidat sur 200 points u16 étendus | Test borné, pas preuve globale ni débit à 50k |
| [G4 50k CPU](docs/RESULTATS_G4_20260904.md) | Huit runs achevés, quatre paires v6/v7 identiques, tours 1..10 puis 1..5 | Objet Gabriel, s=8, 48 threads ; v7 K10 : 50,120 s uniforme et 18,283 s terrain ; K5 : 10,117 s et 5,432 s ; seconde non atteinte |
| [Complétion G4](receipts/gcp_requalified_20260904/published/receipt.json) | 50k par défaut refusé ; n=8000 u16 étendu candidat achevé en 85,396 s | Même domaine de refus explicite ; aucun certificat global ni substitution Gabriel |
| [Primitives GPU G4](docs/RESULTATS_G4_20260904.md) | 12/12 portes device réelles, mutants compris | Témoins et census ; ni tour GPU complète, ni débit 10M/50M |

Les rapports indépendants ont levé A1 (nettoyage d'archive sous panne
persistante d'allocation) et C1 (classification et enregistrement des
campagnes). Les sources et replays sont dans
[le dialogue courant](audits/DIALOGUE_COURANT.md). Le
[certificat horizontal réduit sur E](audits/CERTIFICAT_HORIZONTAL_COURANT.md)
raccorde désormais S1, les primitives qualifiées, le domaine CPU régulier,
les suffixes d'ancrage et le lecteur des deltas. Il conserve les composantes
non triviales, les points couverts et les applications entre coupes d'un
même ordre ; ce n'est pas une application verticale entre ordres.
Ces obligations sur E ne sont plus ouvertes. La conservation F reçoit sa
revue épinglée propre, sans transformer les tests E en exécutions F.
La [cartographie de qualification](docs/QUALIFICATION_S1_PRIMITIVES.md)
reste l'entrée des obligations et de leurs autorités successives.
Les [applications verticales](audits/CONTRAT_VERTICAL_COURANT.md) ont aussi
leur construction totale : scanner `born` à une vraie naissance fournit
une ancre inférieure ; `parents` et les successeurs propagent ensuite les
cartes aux coupes exactes. Aucun resolver géométrique général n'est requis
par cette route. Son port produit et son export restent à construire.
Le [contrat des masses et du vote](audits/CONTRAT_MASSES_VOTE_COURANT.md)
précise l'univers des incidences et le rayon utilisé ; les seuls deltas
horizontaux ne déterminent pas ces poids. Ces contrats ne sont pas des
champs déjà livrés dans l'archive F. L'[autorité p3](audits/AUTORITE_VOTE_P3_COURANTE.md)
compare exactement les numérateurs de vote par classes de carrés et
intervalles rationnels, avec indécision explicite au plafond ; elle ne
qualifie pas encore les quotients de masses ni la condensation.
La [preuve arithmétique des primitives](docs/ARITHMETIQUE_PRIMITIVES.md)
distingue les domaines réellement produits des domaines génériques des
types. Le [plan statique épinglé](docs/PLAN_PORTES_ARITHMETIQUES.md)
est conservé comme antériorité, avec son statut initial non exécuté.
L'état actuel de ses deux portes est dans le reçu d'intégration ci-dessus :
Cramer, PGCD, retenues et sites U192/U320 distincts y sont effectivement
exercés, sans que ces fixtures remplacent les preuves universelles de domaine.

## Contrats qui ne sont pas encore satisfaits

Le [contrat de performance](docs/CONTRAT_PERFORMANCE.md) impose d'abord le
mono-thread, puis le multi-CPU et le GPU. La cible 50k porte sur toute la
tour 1..10 sous une seconde, avec repli sur toute la tour 1..5, puis 100 ms
une fois la seconde qualifiée. La séparation WSPD s=8/10/12 est comparée
à ordre, entrée et objet inchangés. Une censure ou un refus n'est jamais
une réussite rapide. Une mesure Gabriel seule n'est pas une mesure HGP
complète.

Les paliers de plusieurs dizaines de millions sur G4 demandent encore
une architecture de résidence, des budgets RAM/VRAM et une reprise moteur
qui ne sont pas livrés. La [revue de résidence](docs/RESIDENCE_MASSIVE.md)
épingle les limites de cardinalité et distingue index, candidats globaux
et catalogues par ordre ; elle propose une première frontière externe
sans prétendre livrer une tour massive. Les facettes et incidences encore matérialisées
restent un coût central, même sans mosaïque globale. L'archive atomique
n'est pas un checkpoint. La verticale, les poids du vote et le traitement
général des plateaux gardent également leurs contrats propres ouverts.

## Cloud et CI

L'[erratum de publication](docs/ERRATA_PUBLICATION_20260904.md) documente
les journaux exclus du premier push par `*.log`, l'échec documentaire CI
correspondant et une trace smoke mono historique incomplète. Les reçus
bruts ne sont pas réécrits ; leurs manifestes sont désormais contrôlés
contre les octets réellement présents dans l'index Git.

Le [reçu GCP initial](receipts/gcp_20260904/created_then_missing_toolchain.json)
conserve un démarrage gardé G4 SPOT, l'échec avant compilation faute de
toolchain CPU et l'arrêt certifié de cette génération. Aucun benchmark
CPU/GPU n'a été produit par cette tentative. Les coupe-circuits GCE et
invité ont été vérifiés avant l'exécution. Le disque de la VM arrêtée
reste conservé ; une VM arrêtée n'implique pas un coût de stockage nul.

La [session suivante](docs/RESULTATS_G4_20260904.md) est achevée et ses
résultats sélectionnés sont publiés après validation des reçus récupérés.
Une [contrelecture distincte](receipts/gcp_requalified_20260904/public_review.json)
vérifie 148 correspondances public/privé, hashes, coûts, matériel et
fermeture ; elle ne se substitue pas aux lectures GCP du constructeur.
La cible `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35`, génération
`2026-09-04T15:45:50.919-07:00`, est certifiée `TERMINATED` à
22:55:26 UTC. Le contrôle indépendant root confirme la même génération
arrêtée ; l'inventaire ne trouve aucune autre VM E-HGP active.
Un [nouveau contrôle en lecture seule](receipts/gcp_handoff_20260905.json),
enregistré le 5 septembre à 00:26:13 UTC, confirme la même génération
`TERMINATED` et un inventaire `RUNNING` étiqueté E-HGP vide. Aucune
mutation ni nouvelle session n'a été effectuée pour cette revérification.
Aucune autre cible n'a été arrêtée. La qualification GCC11/CUDA de cette
copie source reste distincte de la qualification locale GCC13.

Le workflow v7 ne possède aucune autorité cloud et n'annonce pas de
résultat GPU. Les résultats CTest locaux, les futurs runs GitHub et les
reçus G4 sont trois autorités distinctes. Le registre officiel est inchangé.
Le premier [run GitHub v7](https://github.com/Ludwig-H/E-HGP/actions/runs/33924177970)
du commit `d9e4ee01` a exécuté 292 portes : 291 passent, et
`mhgp7_sonde_ablation_gate` échoue. Ce résultat est conservé séparément
de la qualification locale C entièrement verte ; sa cause est corrigée
localement, sans relance automatique ni changement du moteur.
Le diagnostic a identifié deux appels nominaux d'inventaire héritant de
`LD_LIBRARY_PATH` fourni par setup-python. Le lanceur refuse correctement
cette variable ; le test nettoie désormais son environnement et vérifie
explicitement le refus brut puis les deux succès nettoyés. Les quatre
replays locaux passent ; ils ne transforment pas le run GitHub initial
en succès rétroactif.
Le [run automatique du commit auditeur](https://github.com/Ludwig-H/E-HGP/actions/runs/33927718675)
`d2b27058`, encore dépourvu du correctif constructeur, retrouve le même
unique échec sur 292 ; aucun autre test n'y échoue. Il n'a été ni annulé
ni relancé manuellement.

Le [run eabedd7e](https://github.com/Ludwig-H/E-HGP/actions/runs/33931042316)
est ensuite entièrement vert : construction, documentation, 316/316 portes
CPU (780,55 s CTest), banc d'incidences et garde-fous cloud sans accès GCP.
Il qualifie ce commit C, pas rétroactivement les échecs précédents ni
automatiquement le nouveau delta D et ses sept portes supplémentaires.

Le [run D e6d33698](https://github.com/Ludwig-H/E-HGP/actions/runs/33933790563)
est lui aussi terminé avec succès le 5 septembre à 00:59:02 UTC : les
étapes de build, validation documentaire, contrats CPU, banc d'incidences
et garde-fous sans accès cloud sont vertes. Ce constat est une lecture
distincte de GitHub ; il ne remplace pas les reçus locaux D et ne qualifie
pas automatiquement un delta ultérieur.

Le [run E 2b94abdd](https://github.com/Ludwig-H/E-HGP/actions/runs/33952448267)
est terminé avec succès le 5 septembre à 07:40:29 UTC : construction,
documentation, contrats bornés et mutants causaux, banc d'incidences et
garde-fous sans accès cloud passent. Cette CI qualifie le commit E ;
les 33/33/324 exécutions locales conservent leurs propres reçus.

Le [run F 71895104](https://github.com/Ludwig-H/E-HGP/actions/runs/33959177436)
est terminé avec succès le 5 septembre à 10:11:36 UTC. Son journal
confirme 339/339 portes CPU, zéro échec, les contrôles documentaires et
633 fichiers épinglés dans 11 reçus, puis les tests des runners et de
sécurité sans accès cloud. Il ne remplace pas les 48/48/339 qualifications
locales et ne qualifie pas les temps des campagnes mono ou de taille.

Le [run des résultats F 4cc804e5](https://github.com/Ludwig-H/E-HGP/actions/runs/33961625862)
est également terminé avec succès, le 5 septembre à 11:04:14 UTC : build,
documentation, contrats CPU, runners et sécurité sans accès cloud passent.
Cette CI est distincte des observations de latence déjà scellées.

Contrôles avant le premier commit : registre (20 phases), liste blanche
des workflows GCP et corpus documentaire historique (259 fichiers) passent.
Le corpus v7, absent de ce dernier contrôleur par défaut, est validé
explicitement via la même fonction. Le contrôle des blancs du diff initial
signale les fins de fichiers héritées et les blancs des reçus bruts ;
ces octets épinglés ne sont pas reformattés. Aucun fichier v6 ni reçu de
campagne encore ouverte n'est inclus dans cette publication de jalon.

Depuis le jalon initial, le contrôleur documentaire canonique inclut aussi
README, passation, docs, bench et reçus Markdown v7 ; les écrits de
l'auditeur restent séparés. Son résultat ne repose donc plus sur un
contrôle manuel supplémentaire de ce seul corpus constructeur.

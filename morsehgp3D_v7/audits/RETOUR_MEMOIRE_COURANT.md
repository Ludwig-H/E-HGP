# Admission mémoire et durées de vie du pipeline v7

État actualisé le 5 septembre 2026. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le verrou de palier désormais observé en mono F est le refus à 32 000 points : code 2 à K9, `silent_core_record_budget`, après 569,876 s et avec un pic RSS de 7 100 740 KiB. La [qualification des observations closes](receipts_resolver_20260905/qualification/review.json) distingue ce reçu terminé d'un moteur réussi : stdout est vide, les diagnostics précédents restent provisoires. Le palier 16 000 points termine K1–10 avec un pic RSS de 5 361 880 KiB.

Le plafond de 8 millions porte sur `core_records`, occurrences chargées selon la somme des `q` avant tri/dédoublonnage. `core_facets` compte ensuite les facettes uniques : son zéro imprimé à K9 ne signifie pas zéro travail. Le total nécessaire pour terminer K9/K10 et sa mémoire prospective ne sont pas mesurés. Ce cap de travail/stockage temporaire, le proxy partiel de payload de 16 GiB et `RLIMIT_AS=26 GiB` d'espace virtuel sont distincts d'une limite de RSS. Le chantier utile est de précompter ces occurrences et examiner compression ou interning sous des plafonds distincts, avec facture des capacités avant tout relèvement de cap.

Les propositions et fixtures de coexistence ci-dessous restent indépendantes de ce refus mono. Elles ont été vérifiées le 4 septembre à 20:58 UTC avec `fold_join_before_next_k=false`. Le [mode mono direct courant](MONO_COURANT.md), avec `threads=1` et jonction activée, exécute B sur l'appelant et termine B1 avant A2. La coexistence mesurée ici ne s'applique donc pas à ce mode. La relecture du delta mono, `run.hpp` SHA-256 `1999f901fb44caf3ca743e77e64bb3e5765070fa01a369447b9e89be21ce728c`, confirme que les expressions de budget et la route avec recouvrement sont inchangées ; cette confirmation est statique, sans nouveau run mémoire.

Sur ces fixtures à jonction désactivée, le levier proposé est de borner la concurrence utilisée dans l'admission par le nombre d'ordres effectivement publiables. Deux fixtures montrent que la réservation pour seize travaux peut refuser un calcul qui ne peut en produire qu'un ou deux, alors que le même objet est obtenu avec la concurrence bornée. Ces refus appliquent correctement la formule conservative déclarée `partial_named_payload_proxy_v1` : ils ne démontrent ni dépassement de cette formule, ni dépassement d'une limite de RSS.

Les sources, commandes, sorties positives et négatives, ainsi que les hashes avant copie, après copie et après essais sont dans [memory_budget_current.json](receipts_20260904/memory_budget_current.json). Les 46 fichiers du snapshot sont restés identiques pendant les essais. Sources principales : `src/pipeline/run.hpp`, SHA-256 `885348a92f48658642e3783027cb7c4f239f1c8e1a0b91c66a698f3be6b29762` ; `src/pipeline/expand.hpp`, SHA-256 `7cafb0341344fbc7d1584001e4685e2e5bf0122fe3b7e37277f5468d5c5e1cf0`. Les conclusions portent sur ces octets ; le constructeur peut faire évoluer les sources en parallèle.

## Formule appliquée et objets comptés

Sur cet ABI, `sizeof(BallCandidate)=144`, `sizeof(Survivor)=16`, `sizeof(BallData)=224` et `sizeof(ForestEvent)=144`. Notons $R$ le nombre de candidats bruts, $C$ leur taille après dédoublonnage, $S$ le nombre de survivantes, $E_K$ les événements avant complétion, $A_K$ les cofaces ajoutées et $f$ le `fold_inflight` demandé.

| Porte du snapshot testé | Proxy testé en octets | Moment du refus |
| --- | ---: | --- |
| Fusion des candidats et admission avant tri | $288R$ | Avant traitement post-RLE |
| Préfiltre et census | $C(144+16+2\times224)=608C$ | Avant le préfiltre ; borne $S\leq C$ |
| Expansion et fold | $144E_K(f+2)$ pour chaque ordre | Tous les ordres comptés avant tout callback |
| Complétion silencieuse et fold | $144(E_K+A_K)(f+3)$ | Après construction des additions de cet ordre |

Ces sommes sont des proxies de payload logique nommé. Elles n'additionnent pas toutes les structures simultanées du programme et ne suivent pas toutes les capacités des vecteurs. Les index spatiaux, structures internes du fold, allocations du système et mémoire des consommateurs restent hors de cette admission. Une taille post-RLE ne constitue donc pas une mesure de mémoire allouée.

Le chemin CPU observé du census possède une destination privée `staged` de taille connue, puis la publie par `swap` après validation complète. Il ne fusionne pas deux tableaux de `BallData`. Le coefficient `BallData ×2` de l'admission demeure conservatif. Cela ouvre un second travail local, mais ne justifie pas de changer simultanément tous les coefficients.

## Deux exemples d'admission

La première entrée contient deux points, IDs 10 et 20, de coordonnées `(0,0,0)` et `(2,0,0)`, avec `smax=2`. Elle produit exactement un candidat, une survivante et un événement pour $K=1$. Son unique ordre interdit physiquement seize workers B simultanés.

| Budget | `fold_inflight` | Résultat observé | Callbacks |
| ---: | ---: | --- | ---: |
| Sans limite | 1 ou 16 | Complet ; même digest ; pic B mesuré égal à 1 | 1 |
| 432 | 1 | Refus préfiltre : $608>432$ | 0 |
| 1 000 | 1 | Complet | 1 |
| 1 000 | 16 | Refus fold : $144\times18=2592>1000$ | 0 |
| 607 | 1 | Refus préfiltre | 0 |
| 608 | 1 | Complet | 1 |

Le digest complet commun est `817bb5f8c4aecc25d4fc4c8cbe8a6cfa3ef6e36857db6c2f0bf00027d328574c`. La fixture vérifie le statut, la non-vacuité et l'égalité du digest. Elle observe 384 octets de capacités simultanées pour candidats, survivantes et destination census, puis 288 octets pour shards et destination d'événements. Le refus à 432 est autorisé par le proxy conservatif de 608 ; ces deux observations ne certifient pas un pic global à 432.

La deuxième entrée est `make_family_input(kUniform, 10, 65536, 3)`, `smax=3`, complétion silencieuse activée. Elle produit $C=S=45$, $E_1=18$, $E_2=27$ et aucune coface supplémentaire. Il n'existe que deux ordres publiables.

| Budget | `fold_inflight` | Résultat observé | Callbacks |
| ---: | ---: | --- | ---: |
| Sans limite | 16 | Complet, 45 événements | 2 |
| 69 984 | 16 | Refus tardif à la complétion de $K=2$ | 1 provisoire |
| 69 984 | 1 | Complet, même digest | 2 |
| 27 359 | 2 | Refus préfiltre | 0 |
| 27 360 | 2 | Complet, même digest | 2 |

Le digest normalisé commun est `602458e63d994c2128a9d5190a91c2d4084f0d91e1701405f1160506fc3cc11b`. À 69 984 octets et $f=16$, l'admission initiale accepte exactement $27\times144\times18=69984$, puis la garde silencieuse exige $27\times144\times19=73872$. Le constructeur de cofaces a déjà visité 254 nœuds et évalué 22 supports avant ce refus, bien qu'il n'ajoute rien. Avec $f=2$, le plafond dominant reste le census, $45\times608=27360$ ; son refus à un octet de moins reste actif.

Ces essais modifient l'option de concurrence existante ; ils ne testent pas encore un correctif produit qui conserverait `fold_inflight=16` tout en bornant seulement son facteur d'admission. Ils apportent les cas positifs, les rejets et les objets de référence pour ce correctif.

## Coexistences réellement observées

La [sonde causale](receipts_20260904/memory_budget_gate.cpp) utilise deux points d'observation ajoutés uniquement à une copie sous `audits/.work_residence2/`. Le [diff d'instrumentation](receipts_20260904/memory_budget_instrumentation.patch) lit les tailles et capacités au début du census et après réservation de la sortie d'expansion, avant libération des shards. Il ne change ni événements ni comparateurs. Le rendez-vous est borné à cinq secondes.

Sur `uniform10`, le test retient B1 au callback de phase `kReduceBegin`, avec son `Stage` vivant, jusqu'à ce que A2 atteigne la réservation de sa sortie. Les buffers d'événements observés ensemble sont :

| Propriétaire | Taille utile | Capacité |
| --- | ---: | ---: |
| Événements conservés par B1 | 18 | 18 |
| Shards d'expansion de A2 | 27 | 32 |
| Destination réservée par A2, avant remplissage | 0, cible 27 | 27 |
| Total de capacité observé | | 77 événements = 11 088 octets |

Le résultat final reste identique au digest normalisé de référence. Ce rendez-vous prouve une coexistence A2 × B1, sans constituer une mesure du pic global ni une preuve d'activité simultanée de deux réductions B. Les capacités chiffrées sont celles de la libstdc++ liée au binaire GCC 13.3 ; leur croissance n'est pas une constante portable du modèle mathématique. `FoldPrepared` référence le vecteur d'événements du `Stage` ; le transfert de possession du stage au worker n'en crée pas une copie supplémentaire.

Un cube de huit points, coordonnées dans `{0,4}³`, donne une autre distinction utile : 38 candidats bruts, 27 uniques, mais une capacité candidate conservée de 38 pendant le census. Les trois tableaux nommés occupent alors $38\times144+27\times16+27\times224=11952$ octets de capacités, contre $27\times384=10368$ selon leurs tailles. Le run est complet, avec 239 événements et sept callbacks. Cette réserve retenue interdit d'assimiler réduction de taille post-RLE et libération effective de mémoire.

## Correction locale proposée au constructeur

1. Calculer une fois le plafond constant $f_{\mathrm{budget}}=\min(f,K_{\max,\mathrm{eff}})$, puis l'utiliser dans les deux facteurs `+2` et `+3`. Conserver les autres portes, notamment le coefficient 288 des candidats. Publier séparément concurrence demandée et concurrence utilisée pour l'admission ; documenter la nouvelle politique d'admission.
2. Appliquer ce même plafond à tous les ordres. Au plus $f$ workers B peuvent rester vivants, et au plus $K_{\max,\mathrm{eff}}$ jobs d'ordre peuvent exister pendant tout le run. Le minimum borne donc leur nombre, y compris les ordres précédents retenus par réduction, digest, attente de publication ou callback. Les shards et la destination du producteur restent comptés séparément dans les termes additionnels. Une formule fondée sur les seuls ordres **restants** serait insuffisante : les jobs précédents peuvent encore posséder leurs buffers.
3. Ajouter, dans la passe initiale de comptage, le refus inévitable $144E_K(f_{\mathrm{budget}}+3)>B$ pour les ordres soumis à complétion. Comme $A_K\geq0$, aucune complétion ne peut sauver cette admission. Appliqué seul, avec le facteur demandé 16 inchangé, ce précontrôle avancerait le refus du second exemple avant construction et callbacks. Combiné au minimum proposé, cet exemple est admis : le précontrôle ne doit alors pas le refuser. Il ne remplace pas la garde après additions.
4. Pour les additions inconnues à ce stade, dériver ensuite un quota résiduel depuis le budget et le transmettre au constructeur de cofaces, afin de refuser avant accumulation au-delà du quota. Garder les limites indépendantes sur événements et incidences, ainsi que le refus transactionnel. Renseigner le motif : nombre d'événements, facteur et budget. Le message actuellement observé est seulement `silent incidence fold capacity K=2 : ` lorsque seule la condition budgétaire échoue.

La justification du minimum porte sur le nombre de buffers logiques prévus par le proxy. Elle ne transforme pas celui-ci en borne de capacités ou de RSS. La passe initiale contrôle tous les $E_K$ avant le lancement du premier B. Sans complétion, si $L=\lfloor B/(144(g+2))\rfloor$ et $g=f_{\mathrm{budget}}$, tous les $E_K$ sont au plus $L$, même lorsque leurs tailles diffèrent ; au plus $g$ jobs B et les deux tableaux logiques du producteur représentent donc au plus $144(g+2)L\leq B$. Le producteur A est préparé **avant** `reap_front()` : ses buffers ne peuvent pas être absorbés dans le seul nombre de slots B.

Avec complétion, les jobs $K\geq2$ transmis à B ont passé la garde `+3`, et le seul job $K=1$ conserve la garde `+2`. Cette hétérogénéité ne justifie pas de multiplier la taille du seul ordre courant par le nombre de jobs : utiliser les plafonds communs issus des gardes. Les ajouts sont cependant construits avant leur admission `+3` ; leur taille peut donc franchir ce seuil avant le refus. Le minimum seul ne rend pas le budget prospectif pendant `build_silent_cofaces`. Le quota résiduel proposé séparément traite ce point. Le correctif doit préserver les contrôles après construction, avant transfert aux workers, et ne doit ni compter un stage transféré comme une copie supplémentaire, ni supprimer les shards effectivement présents.

Plus précisément, poser $L_2=B/(144(g+2))$ et $L_3=B/(144(g+3))$, sans planchers pour majorer. Au pire, le job B1 occupe $L_2$, les autres B occupent chacun $L_3$ et les deux tableaux de l'expansion courante chacun $L_2$ : $3L_2+(g-1)L_3\leq B/144$ pour $g\geq1$. Après admission des additions, l'insertion peut conserver l'ancien tableau $E_K$, le tableau d'ajouts $A_K$ et la nouvelle destination $F_K=E_K+A_K$ ; leur somme logique vaut $2F_K$. Le pire majorant est alors $L_2+(g+1)L_3\leq B/144$. Ces inégalités concernent ces seuls tableaux admis ; elles excluent explicitement la construction des additions avant leur garde, les capacités supplémentaires et les autres structures du fold.

Une réduction du coefficient census pourra être examinée séparément sur le chemin CPU direct : le payload logique observé est $144C+(16+224)S$, et la fusion du préfiltre possède son propre couple de tableaux de survivantes. Cela demande une admission par étape, une règle explicite pour la route `prefilter_census_override` et un bilan des capacités. Aucune baisse globale de 288 n'est proposée : la fusion de génération et les réserves avant RLE doivent d'abord être comptabilisées.

## Tests et transaction

La compilation GCC 13.3 avec `-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -pthread` et les deux sondes terminent avec code 0. La porte principale annonce `budget_gate errors=0 overlap_witnesses=1`. Elle vérifie les succès non vides, les digests communs, les refus précoces et tardifs, ainsi que l'effacement des cartes, digests, compteurs d'événements et signatures de stockage contrôlés dans le résultat refusé. Aucun test ne repose sur `assert`.

Les callbacks `on_forest` restent contractuellement provisoires. Le refus tardif du second exemple laisse un callback déjà exécuté, mais invalide le payload du résultat final. Ce n'est pas une promesse de rétraction des effets du consommateur. Le précontrôle proposé réduit un refus tardif prévisible sans modifier ce contrat.

Après implémentation du minimum, ajouter au test les options demandées à 16 avec budgets 1 000 et 27 360, puis exiger les mêmes digests que leurs références. Préserver les rejets 607 et 27 359. Un mutant qui reprend le facteur demandé doit rétablir les refus inutiles ; un mutant qui supprime la garde census doit perdre les deux rejets. Pour le précontrôle silencieux, un budget situé entre les facteurs `+2` et `+3` doit refuser avant construction et avant callback, avec payload final vide. La sonde de capacités reste nécessaire pour les travaux suivants : une porte d'admission ne suffit pas à prouver une résidence globale.

La reproduction permanente est portée par [replay_memory_budget.py](receipts_20260904/replay_memory_budget.py) et [son manifeste d'entrées](receipts_20260904/memory_budget_replay_assets.json). Elle reconstruit les copies `source/` et `instrumented/` sous `audits/.work_residence2/` à partir des fichiers produit du dépôt, après vérification des 46 hashes. Les deux sondes et le patch sont conservés dans `receipts_20260904/` ; aucun binaire ni fichier d'une précédente `.work` n'est requis. Une source différente du pin provoque un refus explicite de reproduction, sans substitution silencieuse. La préparation et l'application du patch ont été rejouées seules ; les compilations et mesures de référence sont celles du reçu principal.

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/receipts_20260904/replay_memory_budget.py --prepare-only
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/receipts_20260904/replay_memory_budget.py
```

Toutes les écritures et copies de sources sont sous `audits/`. Aucun changement produit ou CMake. GCP non utilisé ; aucune mesure GPU.

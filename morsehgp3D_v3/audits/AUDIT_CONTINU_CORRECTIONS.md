# Audit continu et journal des corrections v3

> [!IMPORTANT]
> Ce journal suit le développement concurrent de `morsehgp3D_v3`. Chaque entrée est liée à un snapshot; une correction n'est déclarée fermée qu'après fixture permanente, build propre et exécution Release ainsi qu'ASan/UBSan. L'autorité mathématique reste `docs/SPECIFICATION_MORSEHGP3D.md` et `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`.

> [!NOTE]
> Contexte courant : `phase=exploration_v3_hors_registre`, `backend=cpu_reference_oracle_under_audit`, `profile=quantized_u16_input_only`, `mode=m1_hostile_judge_and_m2_1_anchored_falsifier`, `public_status=not_claimed`. Aucun résultat de ce dossier n'ouvre une porte produit.

## Snapshot 2026-08-08 — `314f7d329f9017a327a6a4442d69fe0f2ebdbe97`

Empreintes au début de cette passe :

- `PROPOSITION.md` : `615935ad798ce5afb3eb3280a54a3bfd8306eed9d7570ff474866c7a3255d912`;
- `oracle/oracle_main.cpp` : `f809332c4c09620b3df353c13551ceb30b5170ff9ef13e0b4c70128e638c4a8f`;
- `prototype/anchored_catalogue.hpp` : `1df051f136964cd03bddfe800a8a3539ac9a086677034b57993501fd85745526`;
- `CMakeLists.txt` : `1b77717b71769820ff7c4c4846d9c42f888121fba2e52ae0067cc1ae893033b2`.

Le snapshot contient déjà des corrections réelles : retrait du faux certificat local M2.1, séparation `exhaustive`/`assumed_window`, contrôle d'une source contributrice et premières injections hostiles. Les findings ci-dessous portent sur les limites restantes de ces corrections.

## P0 — les campagnes `--inject` peuvent réussir sans avoir injecté la faute

Le verdict négatif accepte actuellement tout `campaign.failures > 0` comme preuve que la faute demandée a été détectée. Il ne prouve ni qu'un sujet admissible a été muté, ni que le garde visé a produit l'échec, ni qu'aucun échec étranger n'a fourni le rouge.

Reproduction sur le snapshot `461826f` et toujours pertinente pour le snapshot courant, pour chacune des six fautes :

```text
mhgp3v_oracle --inject <fault> --clouds 1 --seed 4242 --min-points 8 --max-points 8 --max-order 1 --coord-max 1 --min-decided 1 --min-nodes 1
```

Le nuage est rejeté du domaine, donc aucune mutation n'est appliquée : `decided=0`, `rejected_domain=1`, `spheres=0`, `forests=0`, `nodes=0`. Les deux planchers de campagne échouent, mais le programme rend néanmoins le code 0 et annonce que la faute a été attrapée.

La correction requise est fail-closed :

1. compter exactement les injections effectivement appliquées;
2. exiger une injection et une seule;
3. identifier le diagnostic attendu par un code stable, pas par le seul compteur global;
4. exiger que ce diagnostic soit déclenché exactement une fois;
5. refuser tout échec étranger;
6. vérifier séparément que la campagne support ferme ses planchers et son domaine.

La faute `merge_source_foreign` doit employer une source étrangère de même rang **et de même niveau exact**. Sinon le test peut être capturé par le garde de niveau ou de monotonie et ne prouve pas le nouveau garde de contribution.

Statut : **ouvert; correction et fixture en cours**.

## P0 historique fermé dans le code, fixture encore manquante — faux certificat local M2.1

Le certificat fondé sur le rayon maximal des supports déjà émis était faux : il bornait un support encore inconnu avec une sortie partielle. Le contre-exemple façonné et la graine aléatoire sont détaillés dans [`AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md`](AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md).

Le code courant a retiré le claim et publie honnêtement `assumed_window`. Il manque encore une fixture permanente qui rougit sur l'ancienne version :

```text
mhgp3v_oracle --subject anchored --clouds 1 --seed 4242 --min-points 22 --max-points 22 --max-order 1 --seed-neighbours 16
```

Résultat fautif historique : 70 supports de référence contre 69, support `{6,10}` omis alors que toutes les ancres étaient dites certifiées. Un test exhaustif où `n <= seed_neighbours` ne couvre pas cette régression.

Statut : **raisonnement retiré; fixture de non-régression ouverte**.

## P1 — reçus et interface de campagne non fail-closed

Constats dynamiques sur le développement courant :

- écrire `--receipt /dev/full` peut rendre le code 0 parce que les erreurs de `fprintf`, `fflush` et `fclose` ne sont pas toutes propagées;
- `--measure-only --receipt PATH` peut rendre le code 0 sans créer de reçu;
- le parsing historique par `atoi`, `atoll` ou `strtoull` accepte des suffixes tels que `1junk` et certaines graines négatives;
- un passage `assumed_window` peut annoncer une « structure complète » alors que toutes les ancres sont explicitement incomplètes;
- le reçu ne scelle pas encore les digests des nuages canoniques, le binaire, la toolchain et l'ensemble des paramètres de qualification.

Corrections minimales : parsing intégral avec détection de débordement, écriture de reçu atomique et vérifiée, incompatibilités CLI rejetées explicitement, statut `diagnostic_only` pour toute fenêtre supposée et reçu auto-descriptif.

Statut : **ouvert**.

## P1 — couverture build/test insuffisante

Le build Release propre et les CTests arithmétique, oracle v2 et ancré exhaustif passent sur le delta audité. Ils ne ferment pas les points suivants :

- `.github/workflows/ci.yml` ne construit ni ne teste la v3;
- le CTest ancré courant est exhaustif et n'exerce donc pas l'ancienne frontière fautive;
- avec `max-order=1`, il ne couvre pas les supports de taille trois et quatre;
- les injections hostiles ne prouvent pas encore l'exécution du garde ciblé;
- la matrice hostile complète n'a pas encore été rejouée sous ASan/UBSan.

Statut : **ouvert**.

## P1 — les compteurs `strata` ne mesurent pas encore les strates A2p

Le prototype courant incrémente `strata[m]` pour chaque support affinement indépendant, à chacune de ses ancres, lorsque sa profondeur fermée reste dans le budget. Il ne construit ni subdivision d'hyperplans, ni faces, ni arêtes, ni sommets du sous-complexe shallow. Une cosphéricité peut aussi produire plusieurs supports pour un même objet géométrique.

Ces compteurs sont donc des **incidences support--ancre candidates**, pas des strates A2p. Le ratio actuel entre leur total ancré et les sphères globales dédupliquées mélange deux dénominateurs et ne mesure pas le facteur de sortie d'un peeling. Le libellé doit être renommé, ou le vrai complexe stratifié doit être construit avant d'employer le mot `strate`.

La sortie de mesure divise en outre par `strata_total` et par `catalogue.spheres.size()` sans protéger le cas zéro; un nuage vide de candidats ou entièrement dégénéré peut publier `nan`/`inf`.

Statut : **ouvert; aucune conclusion de complexité A2p autorisée depuis ces compteurs**.

## P1 — canonicalité de la source publique encore non jugée

Le garde courant vérifie qu'une source de multifusion appartient aux cofaces contributrices admissibles. C'est nécessaire. Le contrat public du sujet v2 exige en plus la plus petite sphère contributrice par index. Accepter n'importe quel contributeur valide encore une sérialisation qui contredit ce contrat public.

Ajouter une fixture `merge_source_nonminimal_contributor` distincte de `merge_source_foreign` : même composante, même niveau, source réellement contributrice mais non minimale. Cette convention d'index est un contrat de sérialisation du sujet v2, pas une vérité causale générale de HGP; le rapport doit maintenir cette distinction.

Statut : **ouvert**.

## P0 — débordement CLI sur `--max-order`

Le parsing intégral ne suffit pas sans borne sémantique. Sous ASan/UBSan, la commande suivante déclenche un débordement signé dans `maximum_order + 1` :

```text
mhgp3v_oracle --subject anchored --measure-only --regime exhaustive --points 1 --clouds 1 --max-order 2147483647
```

Il faut borner `maximum_order` avant toute addition, conformément à `kMaxRank` et au profil de campagne, puis borner également `fixed_points` et `seed_neighbours` avant allocation ou conversion. Une valeur syntaxiquement valide mais hors contrat doit rendre le code 2 sans entrer dans l'algorithme.

Statut : **ouvert; fixture sanitizer requise**.

## Garde normative pour le peeling local

Le peeling local reste une voie de recherche pertinente sous les conditions suivantes :

- aucune mosaïque de Delaunay d'ordre supérieur, aucun Gamma global et aucune fermeture globale des cofaces dans le chemin produit;
- structures shallow locales, temporaires, évincées par ancre, avec ledger de mémoire et d'objets intermédiaires;
- chaque élagage possède un certificat strict, rejouable et fail-open; l'ambiguïté conserve le candidat;
- coquille complète avant filtre de rang, centre et niveau rationnels exacts, support minimal et statut `RelevantGP` démontré;
- événements de même niveau traités comme un lot atomique;
- source HGP constituée des cofaces réellement contributrices, incidences silencieuses, couverture et applications verticales, pas seulement d'une forêt de points;
- l'oracle exhaustif reste un juge borné et ne devient jamais l'architecture produit sous un autre nom.

Le meilleur résultat utile à viser ensuite n'est pas un nouveau claim de complexité : c'est un constructeur éphémère du sous-complexe shallow stratifié sur petit `n`, différencié contre l'oracle complet, qui journalise plans insérés, strates par dimension/profondeur, candidats projetés, coquilles, doublons, objets HGP aval et high-water mémoire.

## Ordre de correction

1. rendre les injections hostiles non vacues et isoler le garde testé;
2. ajouter les fixtures permanentes du faux certificat local et de la source étrangère;
3. rendre écriture de reçu et parsing fail-closed;
4. couvrir supports 3/4 et dégénérescences en Release et ASan/UBSan;
5. seulement ensuite prototyper le peeling stratifié éphémère et mesurer son coût intermédiaire complet.

GCP non utilisé pour cet audit et ces corrections.

## Snapshot 2026-08-08 — `5be91d6`, passage en audit-only

À partir de ce snapshot, Codex ne modifie plus l'implémentation v3 : Claude en garde seul la responsabilité. Les seules écritures ultérieures de Codex sont dans `morsehgp3D_v3/audits/`. Les builds et reproductions sont effectués hors dépôt sous `/tmp`.

### Corrections observées dans le snapshot commité

Le commit courant intègre notamment :

- parsing entier intégral et borne de `maximum_order` avant addition;
- séparation entre campagne support positive et probe hostile;
- injection comptée, garde attendu typé et refus des campagnes négatives vacues;
- fixture permanente de l'ancien faux certificat local;
- contrôle de l'écriture et de la fermeture du reçu;
- séparation entre coquilles critiques et ties portés par un support non bien centré;
- renommage des faux compteurs de `strata` en incidences support--ancre;
- protections contre les divisions par zéro dans le census diagnostic;
- wrapper CMake qui exige simultanément un code non nul et le diagnostic attendu.

Le build Release propre sous `/tmp/mhgp3v-codex-release.PlhHV6` a produit :

```text
18/18 CTests mhgp3v passés, 0 échec, temps total 27,59 s
```

Les tests comprennent les arités hautes, l'ancien certificat local, le débordement CLI, les six gardes hostiles, la vacuité, la coquille non critique, le token entier partiel, `/dev/full` et l'incompatibilité `measure-only + receipt`.

### Delta non commité présent au moment du changement de périmètre

Deux fichiers d'implémentation restent modifiés dans le worktree :

- `oracle/oracle_main.cpp`;
- `CMakeLists.txt`.

Ce delta a été écrit avant la consigne audit-only. Il ajoute le contrôle du contrat public v2 suivant : une multifusion doit publier la plus petite sphère contributrice par index, et pas seulement une source contributrice quelconque. La fixture littérale emploie le triangle entier équilatéral `{(0,0,0),(1,1,0),(1,0,1)}` et un quatrième point lointain; les trois arêtes ont même niveau exact et contribuent au même lot. Remplacer la source minimale par une autre contributrice doit déclencher uniquement `forest_source_nonminimal`.

Le delta passe dans la matrice Release 18/18. Il appartient désormais à Claude de le conserver, le réécrire ou le retirer. Codex ne le modifiera plus.

### Portes toujours ouvertes malgré le vert local

- La v3 n'est toujours pas intégrée au workflow CI racine.
- Le CMake v3 importe inconditionnellement la v2 et ses quatre CTests; le nombre total annoncé dans le README doit donc distinguer tests `mhgp3v_*` et tests transitifs v2. Le snapshot courant possède 18 tests v3 et 4 tests v2, pas « 14 tests » au total.
- L'oracle qualifie seulement `quantized_u16_input`, pas `exact_dyadic_input`.
- Les reçus ne scellent pas encore les digests canoniques de chaque nuage, du binaire et de toutes les sources.
- `assumed_window` reste un falsificateur diagnostique incomplet; son vert ne prouve jamais la complétude.
- Les incidences support--ancre ne sont pas des strates de l'arrangement shallow et ne permettent aucune conclusion de complexité A2p.
- Le véritable peeling stratifié, ses projections, les composantes non bornées, les lots HGP, la couverture et les applications verticales restent à construire et à juger.

Verdict du snapshot : **GO pour continuer l'instrumentation et le juge borné; NO-GO pour une revendication d'exactitude produit ou de complexité du peeling.**

## P0 publication — un run rouge peut laisser un reçu apparemment vert

Le snapshot `5be91d6` écrit le JSON avant d'évaluer les postconditions finales de `--inject` et de `--require-incomplete-anchors`. Le code de sortie reste correctement non nul, mais le reçu ne porte ni verdict final, ni code de sortie, ni champ `qualified`.

Reproduction en lecture seule avec le binaire Release :

```text
mhgp3v_oracle --inject merge_source_foreign --clouds 1 --seed 123 --min-points 4 --max-points 4 --max-order 1 --coord-max 65535 --min-decided 1 --min-nodes 1 --receipt receipt.json
```

Résultat observé : code 1, car aucune injection applicable n'a été trouvée. Le reçu existe néanmoins avec `identity_closed=true`, `failures=0`, `injections_applied=0` et `injection_escapes=0`. Un consommateur qui ne possède que le JSON peut donc interpréter comme vert un run explicitement rouge.

Artefact de reproduction local : `/tmp/mhgp3v-receipt-audit.MBvfWT`.

Obligation : calculer le verdict complet avant sérialisation et publier au minimum `status`, `exit_code`, `baseline_passed`, `probe_passed`, `qualified=false/true` et la raison non qualifiante. Une écriture atomique ne suffit pas si le contenu précède encore le verdict.

Statut : **ouvert; bloquant pour toute utilisation des reçus comme preuve**.

### Variante plus grave — `require_incomplete_anchors` rend 0 avec des mismatches

Commande reproduite sur les empreintes `oracle=9e0359b1…`, `prototype=28b18507…` :

```text
mhgp3v_oracle --subject anchored --regime assumed_window --seed-neighbours 16 --clouds 1 --seed 4242 --min-points 22 --max-points 22 --max-order 1 --min-decided 1 --min-nodes 1 --require-incomplete-anchors 22 --receipt false.json
```

Le programme imprime `CATALOGUE cardinal 69 contre 70` et `support manquant {6,10}`, écrit un reçu contenant `"failures": 2`, puis rend pourtant le code 0 avec le message indiquant que 22 ancres se déclarent incomplètes. La propriété locale « le sujet avoue son incomplétude » écrase ainsi le verdict rouge de la campagne différentielle.

Le reçu ne publie ni `require_incomplete_anchors`, ni `incomplete_anchors`, ni un scope `diagnostic_property_test`. Cette commande peut donc être prise à tort pour une qualification verte à la fois par son code de sortie et par l'absence de statut explicite.

Obligation : séparer ce property-test de la campagne de qualification. Soit il possède un exécutable/schéma `diagnostic_only` distinct qui n'affirme jamais la complétude, soit le programme conserve un code non nul dès que `campaign.failures != 0`. Un succès de la propriété attendue ne doit jamais annuler des mismatches du catalogue.

Statut : **P0 protocole, ouvert**.

## P0/P1 documentation — le README contredit la proposition corrigée

Le README courant n'est pas un simple résumé périmé; il réintroduit plusieurs affirmations que `PROPOSITION.md` a explicitement retirées.

### P0 — coût du peeling annoncé sans peeling

Aux lignes 46–64, le README affirme que « le coût d'un parcours sensible à la sortie est mesuré, et il est constant », puis extrapole des gains de 19, 70 et 208. Les compteurs sont pourtant des incidences support--ancre produites par la cascade exhaustive; le prototype ne construit aucune strate, aucune subdivision d'hyperplans et aucun parcours shallow.

Une incidence de support n'est ni une cellule visitée, ni un prédicat payé par le futur constructeur. Trois tailles finies, avec un ratio voisin de 14, ne démontrent ni constance en $n$, ni coût output-sensitive, ni gain du peeling. Cette phrase contredit directement PEL-2/PEL-4, toujours ouvertes dans la proposition.

### P0 — borne A2p indépendante du nombre de plans

La ligne 101 publie « $\Theta(k^2)$ par point, classique » comme borne de sortie A2p. La proposition courante donne correctement un worst-case en $O(m_pK^2)$ par ancre, plus construction et transcripts. Supprimer $m_p$ est précisément le saut non démontré; sans certificat local, $m_p=n-1$.

### P1 — PEL anciennes réintroduites

Les lignes 113–115 demandent encore :

- que les 2-faces soient « exactement » les arêtes utiles, alors que seule l'inclusion de complétude est requise et que des plans superflus sont permis;
- un parcours en `O(sortie)`, alors que le terme d'entrée est obligatoire;
- un traitement d'un énoncé « probablement faux », alors que PEL-3 est déjà réfutée par deux points.

### P1 — commandes et compteurs stales

- La ligne 207 annonce 14 tests; le snapshot courant possède 18 tests `mhgp3v_*` plus quatre tests v2 transitifs.
- La commande d'injection des lignes 216–218 n'emploie pas la fixture de même niveau. Sur des nuages génériques, aucune source alternative de niveau exactement égal n'existe souvent; la commande peut échouer parce qu'aucune injection n'est applicable, pas parce que le garde a rougi.
- Le texte dit encore « appliquée au moins une fois » alors que la correction actuelle exige exactement une application.

Verdict documentaire : **le README ne doit pas servir de résumé décisionnel tant que ces contradictions ne sont pas retirées**. La proposition et les audits sont plus prudents et plus exacts que sa page d'accueil.

## P1 reçu census — « complet » est trop fort

Le README qualifie `census_tukey_shallow.py` de reçu complet. Le sidecar `census_tukey_shallow_20260808.json` contient des éléments utiles — digests des nuages assemblés et des échantillons, seeds, version Python/NumPy, convention de demi-espace et identité de campagne — mais il n'est pas scellé comme preuve reproductible :

- `provenance.git_commit` vaut `unavailable`;
- aucun digest du script ou du binaire Python n'est publié;
- le tableau des 4096 directions n'a pas de digest, seulement seed, loi et version de NumPy;
- l'archive source, `bun.conf` et chaque maillage brut n'ont pas leur digest;
- la commande exacte, le répertoire d'entrée, le digest du reçu et le statut `diagnostic_only/not_claimed` sont absents;
- `decimate` traite `len(points) == target` comme « cloud plus petit » parce qu'il retourne `None` pour `len(points) <= target`; une entrée exactement à la taille cible serait donc sautée à tort.

Le census reste un diagnostic correctement borné comme **minorant** du cas tangentiel non contraint. Il n'est ni un certificat de $R$, ni un reçu complet au sens du plan de test.

## Snapshot 2026-08-09 — `dd6f47d21a2a50de5d77e48dfd47b8c768fb5ead`, M2.2 `edge_shallow`

Empreintes de cette passe :

- `PROPOSITION.md` : `615935ad798ce5afb3eb3280a54a3bfd8306eed9d7570ff474866c7a3255d912`;
- `prototype/edge_shallow.hpp` : `badc8100a700669b56accd68f3362fdef2517d8b4e63ce0b3a303ff29b0f0627`;
- `oracle/oracle_main.cpp` : `2324a92e4ad2c120cb7cfb9e7669aca0610ab5de9c6eb4824d53f65b3d8f9c32`;
- `CMakeLists.txt` : `720984024e19566b7dfbacb4a6a09d71d9a4ee276cf880ff417869990858c269`;
- `README.md` : `efd7c7d8c929cf0b0ec0fd03a664d62fedacfb9f2092e49d4a72981b129ab628`.

Le nouveau composant est un bon falsificateur incrémental : pour l'arité quatre, il obtient le rang depuis le signe des formes affines dans le plan médiateur, puis laisse l'oracle indépendant comparer le catalogue et la forêt. L'algèbre du changement de variables et le traitement du signe du déterminant sont cohérents sur ce snapshot. Ce constat positif ne transforme toutefois pas la campagne bornée en preuve universelle ni le constructeur dense en peeling.

### P0 protocole — le reçu ne décrit ni le bon sujet ni le verdict final

Quatre défauts distincts se composent dans le schéma `morsehgp3d.v3.oracle.campaign.v1` :

1. un run `--subject edge_shallow` est sérialisé comme `"subject": "mhgp3v anchored_catalogue"`;
2. le même falsificateur hybride et borné est publié avec `"status": "qualified"` et `"qualified": true`;
3. le reçu est encore écrit avant les validations finales de `--require-incomplete-anchors`; un seuil impossible peut donc rendre le code 1 après avoir écrit `"exit_code": 0`, `"baseline_passed": true` et `"probe_passed": true`, et employer cette option avec le mauvais sujet peut rendre le code 2 après le même faux verdict;
4. les clés `injection`, `injections_applied` et `injection_escapes` sont émises deux fois dans le même objet JSON. Les parseurs qui gardent la première ou la dernière occurrence peuvent donc lire des sens différents.

Reproductions Release :

```text
mhgp3v_oracle --subject edge_shallow --clouds 20 --seed 4242 --min-points 8 --max-points 12 --max-order 3 --min-decided 15 --min-nodes 200 --receipt edge.json
mhgp3v_oracle --subject anchored --regime assumed_window --seed-neighbours 16 --clouds 1 --seed 4242 --min-points 22 --max-points 22 --max-order 1 --min-decided 1 --min-nodes 1 --require-incomplete-anchors 23 --receipt impossible.json
```

La première commande rend 0 mais ment sur le sujet et la portée. La seconde observe 22 ancres incomplètes, rend 1 parce que 23 étaient exigées, mais son reçu annonce encore un `exit_code` nul. Artefacts temporaires : `/tmp/mhgp3v-receipts-live-VDGv0y` et `/tmp/mhgp3v-edge-commit.CaVvdo/edge_receipt.json`.

Obligation : construire un objet de verdict unique **après** toutes les postconditions, puis en dériver à la fois le code de sortie et la sérialisation. Le sujet doit être une valeur fermée exacte (`v2`, `anchored`, `edge_shallow`), chaque clé ne doit apparaître qu'une fois, et le vert borné doit être nommé `bounded_differential_passed` ou `diagnostic_only`, jamais `qualified` sans profil, autorité et porte documentaire explicites.

Statut : **P0 ouvert; aucun reçu v3 courant ne doit servir de preuve de qualification**.

### P1 couverture — campagne non vacue, mais plusieurs branches centrales restent à zéro

Exécution exacte du CTest M2.2 sur un export propre du commit :

```text
aretes=924 dont retenues=48
droites actives=7848, constantes interieures=0
sommets examines=3996 dont peu profonds=420
arite4 emise=66, tests de profondeur=9432, DICTIONNAIRE REFUTE=0
attempted=20, decided=20, rejected_domain=0
spheres=948, forets=39, noeuds=956, largeur max=157 bits
```

Le test exerce donc réellement des sommets, des profondeurs et des émissions d'arité quatre. En revanche `c_e` vaut zéro sur toute la campagne. Aucun plancher ne porte sur les arêtes retenues, sommets shallow, émissions d'arité quatre, formes constantes, parallèles, concurrences, égalités de profondeur ou valeurs proches de la borne `i128`; seul le nombre global de nuages et de nœuds est exigé.

Un sweep diagnostic trouve `seed=235`, grille `[0,20]`, cinq points et ordre maximal quatre avec `constantes interieures=1`, `arite4 emise=6` et un catalogue vert. Une graine issue de `std::uniform_int_distribution` n'est toutefois pas une fixture inter-toolchain. Une fixture littérale plus claire est le tétraèdre `(2,2,2),(0,0,2),(0,2,0),(2,0,0)` avec le point intérieur de l'arête `(1,1,2)` : elle force une forme constante intérieure pour l'ancre correspondante et un rang fermé cinq. Il faut en plus des fixtures séparées pour droites parallèles ou concourantes, shell supplémentaire, frontière stricte et coordonnées extrêmes.

Statut : **preuve bornée non vacue pour une partie du dictionnaire; couverture de branche et de largeur ouverte**.

### P1 architecture — `edge_shallow` n'est pas encore le peeling A2e

Le prototype parcourt toutes les $\binom{n}{2}$ paires de points. Pour chaque paire, il forme toutes les paires de droites actives puis rescane les droites pour calculer la profondeur : dans le pire cas, cela reste en $O(n^5)$. Il appelle en outre `anchored_catalogue` en régime exhaustif pour les arités un à trois et calcule d'abord aussi ses arités quatre avant de les jeter. C'est acceptable pour un juge borné qui isole le dictionnaire; ce n'est ni une source A1, ni un parcours shallow sensible à la sortie, ni une architecture produit.

Le commentaire `edges_retained` parle d'« arête diamétrale », mais aucune condition de diamètre maximal, aucun clipping de Jung et aucun propriétaire A1 canonique ne sont testés. L'énumération de toutes les paires reste complète pour ce falsificateur, car le centre d'une sphère de support quatre appartient au plan médiateur de chacune de ses paires; la statistique ne qualifie simplement pas la source d'ancres proposée pour A2e.

### P0 documentaire — vingt nuages ne sont pas une preuve du dictionnaire

Le README dit encore que « le dictionnaire de profondeur est vérifié » et que le vert de vingt nuages « est la vérification ». Le résultat est une **absence de réfutation sur une campagne bornée**, utile et non vacue; il ne démontre pas l'identité pour toute configuration de la grille. La preuve algébrique doit vivre dans l'autorité mathématique, avec ses hypothèses de position générale et ses conventions de frontière, tandis que le CTest en devient la falsification permanente.

Le README a retiré plusieurs anciens surclaims, mais conserve aussi aux lignes 57–62 l'extrapolation « constant en $n$ » et les gains `19x/70x/208x` depuis trois ratios d'incidences support--ancre. Ces incidences ne sont toujours pas les strates ni le coût du futur parcours. Le titre plus prudent « majorant du travail » ne rend pas l'extrapolation valide.

Verdict M2.2 sur ce snapshot : **GO pour garder le falsificateur exact et ajouter des fixtures ciblées; NO-GO pour fermer le dictionnaire par le seul CTest, publier un reçu qualifiant, revendiquer A1-source, PEL-2 ou une complexité de peeling**.

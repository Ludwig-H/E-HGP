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

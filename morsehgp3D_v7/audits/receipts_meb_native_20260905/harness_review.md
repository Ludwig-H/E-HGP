# Harnais MEB natif v2 : contrelecture des chemins et du coût

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Conclusion : le harnais appelle réellement la variante `NoObserver`, la confronte à F et à `Trace` avant puis après les mesures, et consomme ses résultats dans les boucles chronométrées. Aucun défaut de résultat, de placement de `Work` ou de suppression des appels par optimisation n'a été trouvé dans le périmètre relu.** La mesure porte sur les helpers enveloppés, les réinitialisations et les captures. Elle ne donne pas leur coût isolé ni celui d'une intégration dans la tour.

Cette contrelecture lit les sources, le désassemblage et les reçus déjà clos. Elle n'exécute ni le harnais ni un compilateur, ne modifie aucun produit et n'utilise ni Git ni GCP. La vérification de provenance et la capture portable des reçus sont conduites séparément par l'auditeur de qualification.

## 1. Sources et pins exacts

Les chemins ci-dessous sont relatifs à `build/v7_meb_dual_budget_cost_v2/`. Les références de lignes C++ suivantes désignent toujours son `cost_harness.cpp` épinglé ; les numéros ne désignent pas une version future intégrée.

| Artefact | SHA256 |
| --- | --- |
| `cost_harness.cpp` | `5a0fd39703c26279d91796fba7c099df9d25829a6f84b7fae4ddf99ffa61f5d8` |
| `run_cost.py` | `8b9ae71ef0419acea000897c950860976da2e04c7efbadd123c8996e1fdb8032` |
| `build_20260905/receipt.json` | `de6de29f55ab55d8edd64f9e3307d4748688635ca7338c36105555da39e0574f` |
| `build_20260905/cost_harness` | `56e022c817d2e726eb2e3b135e78e577bbdf344ebd0ff352d64d1121300fd976` |
| `build_20260905/disassembly.stdout` | `52392c6a8b9a8a230133113fdad0bfa9ca64b25291600349b9905be9f126c9c4` |
| `run_20260905/receipt.json` | `874f100ffb1d65956f6d640c5e7ab838a81e9f5c7900f7c1d69b14504235c208` |
| `run_20260905/measurement.stdout` | `2c20ceaf7a8a4757af2ad78554becf2e584f1c397e92860800d6c746de24469f` |
| `run_20260905/judgment.json` | `c0a885bb72e221c53983f640de586c587e188195a0ef827bc59495dd1847e014` |

Le donneur inclus explicitement aux lignes 5–7 est `build/v7_meb_dual_budget_geometry/geometry_gate.cpp`, SHA256 `c9971f8c340fe37eea2be824897110d436a38345e1d66d1834c9eb7f489bb1a9`. Il apporte les fixtures, comparateurs, `Trace` et frontières, pas un résultat de son ancien `main` : celui-ci est renommé et n'est pas appelé. La correction v2 transforme le macro objet `main` en macro fonctionnel `main(...)` ; elle évite la collision effective avec `Metrics::main` qui avait fait échouer la compilation v1. Le premier échec est conservé séparément et n'est pas un défaut géométrique MEB.

Les deux prototypes restent les octets `0645aa00…` et `d6dbba19…` déjà prouvés dans [MEB_DOUBLE_BUDGET_COURANT.md](../MEB_DOUBLE_BUDGET_COURANT.md). F reste le `silent_incidence.hpp` `f75a136a…`. Le build v2 fermé utilise O2, C++20, `-Wall -Wextra -Wpedantic -Werror`, `-fno-lto`, sans `MHGP7_TESTING` ni sanitizer. Le succès du build est distinct du reçu de mesure, clos de 11:50:13 à 11:50:16 UTC.

## 2. Instanciations et autorité des comparaisons

`invoke_f`, lignes 117–127, construit les caps de l'appel et un `Builder` F, puis appelle sa MEB locale. `invoke_dual`, lignes 129–141, construit réellement `proposal::NoObserver` et appelle `proposal::miniball<false>` avec cet observateur. Il n'existe pas de remplacement par `Trace` dans ce chemin. Les deux fonctions sont marquées `noinline,noipa` et ont les mêmes barrières d'entrée/sortie.

`qualify`, lignes 265–315, est exécuté avant le chronométrage, puis à nouveau après toutes les mesures, lignes 430 et 457. Pour chacune des étapes des jobs, il réalise :

1. La comparaison du donneur entre F et `Trace`, y compris son contrôle prospectif et ses sentinelles.
2. Un appel neuf au wrapper F et au wrapper `NoObserver` depuis des états correspondants.
3. La comparaison complète F/`NoObserver` du booléen, du diagnostic, des treize statistiques publiques, des événements et de la boule.
4. La comparaison des sorties statistiques/événements de F et `Trace` aux wrappers correspondants, et des quatre champs `Work` de `Trace` et `NoObserver`.

Les comparateurs du donneur, lignes 22–47, comparent explicitement les champs nommés : coefficient quadratique, trois coefficients linéaires et constante de clé ; trois limbs et dénominateur du niveau ; q et les quatre positions de support ; q/d/masque, niveau et les tableaux complets des deux événements sentinelles. Aucun `memcmp` sur du padding n'est utilisé. `same_ball` consomme l'égalité littérale d'`ExactLevel`, et non une seule égalité rationnelle. Les booléens et boules de `Trace` sont comparés à F dans le donneur ; la même F déterministe assure le raccord aux boules du wrapper natif.

Ce harnais constitue donc une qualification native propre au corpus exécuté. Elle n'est pas héritée du reçu constructeur qui ne compilait que `Trace`. Le juge géométrique de ce harnais partage les primitives de F ; son indépendance est celle des chemins et des champs comparés. Le juge rationnel du § 8 de la note MEB conserve son autorité séparée et ne devient pas rétroactivement une exécution `NoObserver`.

Les lignes 291–305 enregistrent des hashes de terminal et de `Work` au premier passage puis les contrôlent après mesure. Les comparaisons complètes F/`NoObserver` ont bien lieu aux deux passages ; la correspondance entre les passages et les boucles chronométrées utilise des captures 64 bits. Le harnais ne présente pas ces hashes comme injectifs ni comme un oracle mathématique. Le refus scientifique laisse une boule écrite, le refus budget garde sa sentinelle et le succès conserve le statut artificiel initial : ces comportements sont effectivement distingués par les comparateurs.

## 3. Durée de vie de l'état et budgets

Un `Job` porte les caps L/P, les charges initiales, le cap de pivots et un nombre d'étapes de un à quatre, lignes 101–109. `reset`, lignes 143–147, réinitialise les charges au début d'une tentative. Il est appelé une seule fois avant la boucle d'étapes d'un job, aussi bien en qualification qu'en mesure. Les quatre appels P7/L12 et les deux appels P0/L8 conservent donc `Work` et les statistiques entre étapes ; il n'y a aucun reset caché lors du repli. Les 4 096 répétitions q2 correspondent en revanche à 4 096 nouvelles tentatives explicites, pas à un seul budget global renouvelé illicitement dans un ordre.

Les caps transmis sont ceux du job, identiques entre F, `Trace` et `NoObserver`. La construction des jobs, lignes 180–223, porte les 9 216 appels principaux et les 123 appels de frontières du donneur ; elle les compare ensuite aux métriques complètes de `boundaries<false>`. Les douze jobs q2 immédiats supplémentaires ont une cohorte propre. Il y a 9 347 jobs, 9 351 états de qualification et 58 491 appels supérieurs par bras et par passage. Les 384 lectures pilotes de R sont hors chrono ; l'annotation q de référence vient d'un résultat P0/L=R déjà qualifié, lignes 324–338, sans être transmise à la recherche de support.

`CallLedger`, lignes 28–35, admet prospectivement une borne supérieure avant chaque bloc. Le bloc de qualification réserve six entrées pour F+`Trace`+repli éventuel et F+`NoObserver`+repli éventuel ; le bloc chronométré réserve une entrée par appel F et deux par appel dual. La comptabilisation finale reprend le nombre réel de replis imbriqués. Avec ce corpus et ces boucles fixes, les additions et produits restent sous les deux millions d'entrées ; le terminal conservé rapporte 1 325 812. Il ne s'agit ni d'une mesure de candidats ni d'un nouveau plafond produit.

Le code classe distinctement `legacy_guard`, `certificate_accepted`, `certificate_legacy_refused`, `initial_P_fallback` et `fallback_unattributed`, lignes 154–163. Il ne conclut pas que la seule égalité p=P prouve la cause d'un repli ; la réserve des interfaces `Trace`/`NoObserver` est correctement maintenue.

## 4. Ce que les horloges entourent réellement

Le calcul des captures attendues, la réservation de deux emplacements d'événements et le choix du pointeur d'appel sont hors chrono, lignes 365–373. Entre `Clock::now()` aux lignes 374 et 391 figurent :

- la lecture des jobs et fixtures, les boucles et le reset des statistiques/événements/`Work` ;
- la construction de la boule sentinelle de chaque étape et l'appel de l'un des deux wrappers ;
- les calculs MEB et, pour le dual, l'éventuel repli F et la finalisation ;
- la lecture de tous les champs du terminal et des événements pour leur hash, le hash de `Work`, le comptage des replis et les barrières mémoire.

La comparaison des hashes aux attendus, le contrôle du ledger et l'impression JSON se font après l'horloge finale. Les comparaisons complètes avec F, l'oracle des ordinaux, l'indexation des fixtures, le regroupement des jobs et les lectures pilotes ne sont pas chronométrés. La réservation préalable de capacité évite l'allocation initiale des deux événements dans la zone mesurée ; leur copie reste incluse.

Deux chauffes et sept passages mesurés alternent F→dual et dual→F à chaque numéro de passage, lignes 447–455. Les mêmes groupes, jobs, répétitions, états initiaux et captures attendues sont utilisés pour les deux bras. L'ordre des groupes reste lexicographique et les sept passages ont quatre ordres F→dual contre trois inverses : l'alternance réduit un biais de position mais ne constitue ni randomisation ni preuve d'isolation matérielle.

Le champ `clock_tick_ns` est le minimum positif observé sur 128 paires de lectures consécutives de l'horloge, lignes 439–446. `short_batch` signifie seulement que la durée du groupe est inférieure à cent fois ce diagnostic. Ce n'est pas un intervalle de confiance ni une garantie qu'un groupe plus long soit exempt de bruit. Les petits groupes d'un ou quelques appels ne doivent donc pas être présentés comme autant de mesures précises du coût isolé.

## 5. Contrôle indépendant des instructions conservées

La lecture ciblée du désassemblage épinglé confirme les points suivants, sans désassembler à nouveau ni exécuter le binaire :

| Adresse | Instruction ou effet vérifié |
| --- | --- |
| `0x530c` | `invoke_f` appelle réellement `Builder::miniball` à `0x19550` |
| `0x11ce9` | `invoke_dual` appelle réellement `miniball<false,NoObserver>` à `0x209c0` |
| `0x16226` / `0x16234` | Les deux adresses de wrapper sont sélectionnées avant l'horloge |
| `0x1625b` | Horloge initiale |
| `0x16507` | Appel indirect `call *%rax` dans la boucle d'étapes |
| `0x1651e` | Lecture du résultat par `terminal_hash` après chaque appel |
| `0x16523` à `0x165b6` | Accumulation des hashes terminal/`Work` et du nombre de replis |
| `0x16bcd` / `0x16be9` | Retours sur les répétitions et les jobs |
| `0x16bef` | Horloge finale après les boucles |
| `0x16c52` / `0x16c62` / `0x16c84` | Comparaison des deux captures et garde effectif |

Les appels ne sont donc ni remplacés par une constante ni remontés hors des horloges dans ce binaire. `NoObserver` peut légitimement éliminer ses callbacks vides : c'est l'instanciation mesurée. Les barrières et `noipa` introduisent aussi un coût et limitent des optimisations qui pourraient être possibles dans une intégration réelle ; la mesure porte sur cette frontière d'appel explicite.

## 6. Limites concrètes et prochaine décision

La symétrie des resets et de la capture est correcte, mais elle n'en supprime pas le coût. `invoke_f` construit un `Builder` par entrée ; la voie dual accélérée peut éviter cette construction, tandis que son repli la réalise. Cet écart appartient aux wrappers effectivement mesurés. Une intégration dans un `Builder` déjà vivant peut modifier ce poste ; un ratio de ce harnais ne l'estime pas automatiquement.

Pour les mêmes terminaux, le hash public lit les mêmes champs et chaînes de raison. Le hash privé de `Work` lit aussi quatre mots dans le bras F, où ils sont généralement inchangés : cette capture commune est volontaire, mais ne représente pas un besoin fonctionnel de F. Elle peut dominer une MEB q2 immédiate et réduire artificiellement l'amplitude d'un ratio de helper isolé. Aucune soustraction de ce coût n'est justifiée par la présente mesure.

Le corpus de 176 petites scènes, les caps autour du rang F, les états MAX artificiels et les répétitions q2 sont des diagnostics ciblés. Ils ne donnent pas la distribution des MEB, rangs et replis de la tour industrielle. Les causes de repli non observées restent non attribuées, et aucun choix global de P ne découle d'une moyenne non pondérée de ces groupes.

La décision constructive est de considérer **le raccord natif `NoObserver` fermé localement pour ces octets et ce corpus**, sous réserve de la clôture séparée de provenance. Le port produit peut ensuite être explicitement versionné dans le `Builder` de l'ordre : P nul par défaut, `Work` persistant, repli F sans récursion ni remise à zéro, puis qualification des vrais consommateurs. Les preuves mathématiques locales déjà closes ne sont pas à refaire. Un suivi local avec davantage de répétitions et des paires équilibrées peut améliorer la comparaison des helpers ; le protocole préparé sur les 384 ordres, P0/P401, L551, 64 répétitions et dix paires répond à cette limite. L'attribution d'un gain à la tour exige en revanche la route intégrée et sa distribution réelle de MEB. Les temps présents et ce suivi local conservent leur portée privée de helpers avec capture.

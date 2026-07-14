# Revue de porte Phase 1 — blocage scientifique

> [!WARNING]
> La porte ne peut pas être fermée : une campagne exhaustive a produit un contre-exemple exact générique à la réduction de Gabriel qui fonde le profil `hgp_reduced`. Le registre `docs/implementation_status.toml` reste inchangé et aucune phase ultérieure n'est ouverte.

## Résultat

- Phase : 1 — oracle CPU exhaustif.
- Backend : `reference_cpu`.
- Profils : `hgp_reduced` et `full_pi0`.
- Mode : `certified`.
- Statut de phase observé : `in_progress`; porte de sortie non satisfaite.
- Verdict : `no-go` pour la clôture de phase et pour l'ouverture de la Phase 2A.
- Statut scientifique : le flot Gabriel brut de `hgp_reduced` ne reconstruit pas, en général, les composantes des K-polyèdres, même dans le domaine générique certifié. `full_pi0` reste séparément `conditional`, avec `forest_semantics=partial_refinement`, tant que M.1 n'est pas prouvé à la phase 12.

## Preuves et tests

- Base de revue fournie : commit `6fd163c`.
- Suite de référence après enregistrement du contre-exemple : `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests -p 'test_*.py' -v` — 89 tests réussis en 10,356 s, dont les trois tests négatifs permanents de la réduction de Gabriel. Ce résultat démontre le fonctionnement en échec fermé; il ne constitue pas une preuve de clôture.
- Contrats : `python tools/check_contracts.py` — 20 définitions, 20 exemples de schéma et 5 fixtures de résultat validés.
- Documentation : `python tools/check_docs.py` — 22 documents actifs validés au moment de la suite de référence.
- Registre : `python tools/check_implementation_status.py` — 20 phases et leurs portes validées, sans modification de statut.
- Hygiène d'imports : `python tools/check_oracle_independence.py` contrôle statiquement l'AST de chaque module Python de `reference/morsehgp3d_oracle`; seuls les imports relatifs internes et une liste fermée de modules de bibliothèque standard sont autorisés. Les imports absolus externes, les sorties relatives du paquet et les appels dynamiques connus sont refusés. Ce contrôle ne remplace pas un oracle mathématique indépendant.
- Fixtures minimales couvertes : configuration à six points du chapitre 6, supports bien centrés de tailles 2, 3 et 4, triangle obtus, tétraèdre non bien centré, facette isolée absorbée, fusions de même niveau, multifusion, K-polyèdres recouvrants, paire de Gabriel absente d'une petite liste locale, quasi-coplanarité et quasi-cosphéricité à un ULP.
- Cas sélectionnés : catalogues exacts figés pour `n=12` et `n=14`; les tours complètes correspondantes restent réservées à la campagne nocturne.
- Contrôle vertical : l'indexation par les facettes détecte correctement que deux composantes Gabriel distinctes sont contenues dans une même composante Gamma et que leurs couvertures diffèrent. Ce contrôle ne doit pas être supprimé ou affaibli : il empêche de publier comme exacte une hiérarchie réduite qui n'est pas celle des K-polyèdres.
- Contre-exemple permanent : `tests/fixtures/regressions/gabriel_point_set_counterexample.json` contient cinq points de dimension affine trois, `RelevantGP=true`, aucune dégénérescence de catalogue et 22 événements critiques exacts. À l'ordre deux et au niveau carré fermé $83886/3563$, Gamma couvre les cinq points dans une composante, alors que le graphe Gabriel élagué produit les deux couvertures recouvrantes `(0,1,2)` et `(0,2,3,4)`. Les cofaces non-Gabriel `(0,2,3)` et `(0,2,4)` attachent silencieusement la facette `(0,2)` au niveau $33/2$; la coface Gabriel `(0,1,2)` réutilise ensuite cette incidence au niveau $83886/3563$. Le flot élagué ne fusionne qu'au niveau $24$.
- Audit indépendant du contre-exemple : 26 sous-ensembles et 55 comparaisons extérieures exactes ont été contrôlés, sans égalité de frontière; la marge absolue minimale vaut 1. La Proposition 6 du manuscrit est donc contredite comme égalité de collections d'unions de points, et le Théorème 5 hérite de cette défaillance. Le Théorème 4 n'est pas mis en cause.
- Sérialisation : les applications verticales utilisent `method=reference_oracle`; toutes les coupes ouvertes et fermées sont disponibles dans un artefact canonique séparé, y compris pour des coordonnées rationnelles non représentables en binaire64.
- EMST indépendant à `k=1` : `PYTHONDONTWRITEBYTECODE=1 python -m unittest tests.oracle.test_independent_emst -v` — 6 tests réussis en 4,536 s lors de la relecture finale. La référence reconstruit le graphe euclidien complet et Kruskal par lots avec des `Fraction`, sans importer l'oracle comparé. Elle vérifie 27 nuages déterministes pour `n=4..12` et les dimensions affines 1, 2 et 3, les cas `n=1,2,3`, toutes les coupes exactes ouvertes et fermées, les multifusions, les couvertures et le poids total. Les poids attendus des fixtures ligne, carré et cube sont respectivement 5, 12 et 28. Un contrôle AST refuse les imports externes, relatifs et dynamiques de la référence EMST.
- Campagne 10k par dimension affine : `PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --dimensions 1 2 3 --seed-start 0 --seeds-per-dimension 10000 --point-counts 4 --k-max-values 4 --coordinate-bound 1048576 --permutations-per-seed 1 --metamorphic-stride 100 --failure-dir tests/fixtures/regressions/oracle_campaign --manifest tests/fixtures/oracle/phase1_campaign_10000.json`. Le manifeste a pour SHA-256 `c0d892be1df42f7041156b2f9fa43a672283d02a23215afdda89af0b86f13e1c` et pour identifiant `e859d9d1e765eca6d8c90734a4d151b95c1d8ceed1649c83821bbac6c0cbd245`. Les 30 000 nuages et les 30 000 cas sont terminés sans différence signalée, soit 10 000 dans chacune des dimensions affines 1, 2 et 3, 120 000 points, 120 000 ordres, 240 000 contrôles de profil, 30 000 permutations et 300 métamorphismes de chaque type. Ce succès sur `n=4` est un diagnostic borné : il ne couvre pas la fixture minimale à cinq points, n'appelle pas l'orchestrateur intégré `run_oracle` et ne rétablit aucune proposition réfutée.
- Matrice `n`/`K_max` : `PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --dimensions 1 2 3 --seed-start 20000 --seeds-per-dimension 100 --point-counts 4 5 --k-max-values 3 4 5 --coordinate-bound 1048576 --permutations-per-seed 1 --metamorphic-stride 10 --failure-dir tests/fixtures/regressions/oracle_campaign --manifest tests/fixtures/oracle/phase1_campaign_matrix.json`. Le manifeste a pour SHA-256 `8ad5403459dc7ee0bf03119c3f55f9af1f899e88cfb33e182a2a9047f4ef3297` et pour identifiant de campagne `7628ee96dfa85876f20bd035180c4570daebd9791659955e651dfe47513bf6c4`. Les 1 800 cas tentés sont terminés, soit 600 nuages répartis également entre les dimensions affines 1, 2 et 3. Les couples couvrent `n<K_max`, `n=K_max` et `n>=K_max+1`; `matrix_gate.satisfied=true`, 1 800 comparaisons de préfixes sont enregistrées et aucune différence n'est observée.
- Capacité exhaustive `n=12` : `PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --dimensions 3 --seed-start 0 --seeds-per-dimension 1 --point-count 12 --k-max-values 10 --coordinate-bound 97 --permutations-per-seed 0 --metamorphic-stride 0 --failure-dir tests/fixtures/regressions/oracle_campaign --manifest tests/fixtures/oracle/phase1_selected_n12_full.json`. La campagne `5169639b6be18062da096ced4bf2eecb6862ac1773f3c26c83407c23dcd1fcfb` échoue après 1 853,591443608 s. Son nuage de douze points est réduit automatiquement à la fixture générique de cinq points en 609 évaluations, dont 28 suppressions de points et 581 réductions de coordonnées. Le manifeste a pour SHA-256 `ca1af0f2f4ac765a14f61bb5b8ea200daa5e95fd5ec17f7d287e32ffc2fa459a`; la fixture brute a pour SHA-256 `3c4df51c64b2e30ee2d95c9dc4048901d32e9cac9479a2ee1020c96594175df0`.

## Performance

- Matériel de la suite de référence : CPU local, backend Python rationnel exact.
- Protocole : tests unitaires courts, validations statiques et campagnes exhaustives Python; aucune mesure G4 n'entre dans la clôture scientifique de cette phase.
- p50/p95 : non mesurés pour cette revue de correction fonctionnelle. La matrice a consommé 436,662438587 s dans le chronomètre interne, soit 0,727770730 s par nuage généré en moyenne. La campagne 10k a consommé 3 966,479833042 s dans son chronomètre interne, soit 0,132215994 s par nuage. Ces campagnes partageaient le CPU local et ne constituent pas des benchmarks de performance.
- Mémoire : le suivi externe a observé un pic HWM de 32 812 KiB et un pic RSS échantillonné de 31 748 KiB pour la campagne 10k, puis un pic RSS de 44 248 KiB pour le cas `n=12`. Ces observations locales sont diagnostiques et ne constituent pas encore des preuves de ressources liées cryptographiquement aux manifestes.
- Compteurs scientifiques : candidats de supports, rejets, classifications globales et requêtes de miniball sont comptés séparément; les comptes candidat, accepté et rejeté ferment exactement. La matrice vérifie notamment 165 576 carrés de naturalité, 295 496 transitions de monotonie de coupe et 15 300 familles verticales.

## Limites

- L'oracle de PR devait être obligatoire jusqu'à `n=12`; le premier cas sélectionné `n=12` a au contraire réfuté le socle réduit. `n=14` et la future porte G2 restent hors de portée tant que V0 n'est pas résolu.
- Les fixtures `n=12` et `n=14` valident leur catalogue en PR. Les campagnes de capacité séparées doivent encore terminer toutes les filtrations, coupes, forêts et familles verticales pour chaque taille `n=6..12`; aucune tour complète `n=14` n'est revendiquée ici.
- La campagne locale de 10 000 petits nuages par dimension affine a terminé sur `n=4` sans autre différence. Elle ne peut ni réparer la proposition fausse, qui exige déjà cinq points, ni fermer la phase.
- La notion de « classe » dans l'exigence « 100 graines aléatoires par classe pour `n<=12` » du plan de test n'est définie par aucun document normatif du dépôt. Les campagnes 10k et de capacité ne sont donc pas présentées comme une preuve de cette fréquence ambiguë. La revue reste bloquée jusqu'à l'adoption et l'exécution d'un protocole conservateur explicite, sans réinterprétation silencieuse de l'exigence.
- Les manifestes actuels ne lient pas au moment de la génération le commit Git, l'état de l'arbre, la version du générateur et une racine de hash des nuages. Leur SHA-256 protège les fichiers présents, mais ne suffit pas à prouver la provenance du calcul; toute future enveloppe de porte devra corriger ce point puis rejouer les campagnes concernées.
- Le runner audite les constructions exhaustives internes de Gamma, Gabriel et des forêts; il ne constitue ni une deuxième implémentation indépendante ni le différentiel avec un backend de production.
- La minimisation automatique cherche d'abord un cas 1-minimal par suppression de points, puis applique une réduction déterministe et bornée des coordonnées entières. Les compteurs de ces deux étapes restent nuls sur une campagne sans échec.
- Les dégénérescences hors du premier domaine générique restent un arrêt explicite, jamais une promotion vers `exact`.
- `full_pi0` ne peut pas être promu par un accord expérimental ou par cette campagne; M.1 reste une obligation de preuve.
- Aucun benchmark de qualité, aucun ARI et aucun gain de temps face à HGP-old ou HDBSCAN ne peut promouvoir le profil réduit au statut `exact` tant que la définition et la preuve n'ont pas été corrigées.

## Artefacts

- Oracle : `reference/morsehgp3d_oracle/`.
- Contrôle d'indépendance : `tools/check_oracle_independence.py`.
- Tests d'indépendance : `tests/oracle/test_oracle_independence.py`.
- Référence EMST indépendante : `tests/reference_emst/` et `tests/oracle/test_independent_emst.py`.
- Campagne : `tools/run_oracle_campaign.py`.
- Fixtures : `tests/fixtures/exact/`, `tests/fixtures/degeneracies/`, `tests/fixtures/oracle/` et `tests/fixtures/regressions/`.
- Sérialisation des coupes : `serialize_oracle_cuts` dans `reference/morsehgp3d_oracle/serialization.py`.
- Schéma public : v1 inchangé; les coupes exhaustives sont un artefact séparé et n'ajoutent aucun champ inconnu au contrat gelé.
- Checkpoint : aucun requis pour la suite CPU courte.
- Commit de base cité : `6fd163c`.

## GCP

GCP non utilisé pour les calculs de la Phase 1 et `gcp_used=false` dans les deux manifestes. Une inspection strictement en lecture seule de l'environnement GCP a été menée en préparation des phases CUDA, mais aucune VM n'a été créée, démarrée, arrêtée ou modifiée. Les coupe-circuits GCE et invité ne sont donc pas applicables à ces campagnes CPU locales.

## Prochaine porte

La prochaine décision n'est plus une campagne supplémentaire. Il faut choisir puis spécifier une cible : conserver toutes les incidences silencieuses nécessaires dans un flot Gabriel enrichi, construire un locator horizontal certifié qui restitue ces attaches, ou redéfinir explicitement `hgp_reduced` comme une hiérarchie propre au graphe Gabriel sans l'identifier aux K-polyèdres. Après une nouvelle preuve, le contre-exemple devra passer comme cas positif de la construction corrigée; les campagnes, l'agrégateur de porte et les benchmarks pourront alors reprendre. Une fermeture de phase nécessitera une mise à jour explicite et atomique du registre, suivie de `python tools/check_implementation_status.py`.

# Morse HGP 3D v9 — ouverture après l'audit général de la v8

Ouverture demandée le 22 septembre 2026, sur `main` uniquement.

Nouveau chantier distinct, le 27 septembre : [clustering de points depuis le seul T_K](audits/b_point_hierarchy_k_20260927/RESULTATS.md).
Deux projections implémentées, même EOM que le comparateur HDBSCAN, expZ1/2 ;
12 scènes complètes (10 vraies3D, deux SIPU planaires), 504 résultats clos.
Première couverture : hiérarchie emboîtée et rattachements rationnels ;
qualité prometteuse de l'arbre, mais HDBSCAN gagne sur le lot d'évaluation
pour les clusters extraits. Pas de supériorité générale, pas de nouveau
contrat GPU. Moteur inchangé, GCP non utilisé pour cette expérience.

Dernière mesure du 27 septembre : [profil FULL Nsight obtenu sur G4](docs/PROFIL_FULL_NSYS_20260927.md).
Trame sans sol entière, K1..5 : médiane chaude sans profiler912,219ms
(un processus), mêmes objets contrôlés dans les huit passages. Noyaux
GPU207,895ms/pass, copies5,184ms ; tour CPU277–303ms.100ms reste ouvert.
Réserve explicite : Nsight2025.3.1 signale le piloteCUDA13.0 non supporté
et l'absence d'ordonnancement CPU. Diagnostic, pas occupation certifiée.
VM arrêtée après220,439s ; moteur inchangé, pas de nouvelle grande refonte.
La [décision de profiler avant refonte](docs/PROFILAGE_AVANT_REFONTE_20260927.md)
et les échecs antérieurs restent conservés.
Un [essai de profilage G4](receipts/full_nsys_20260927/r1/README.md) s'est
arrêté avant calcul : ancien binaire FULL non disponible sous la forme attendue dans `/tmp`.
Ni téléchargement ni lancement Nsight ; aucune relance automatique,
moteur inchangé. Même génération arrêtée, allocation157,524s.
Le [comparatif G4 S2](receipts/q34_survivors_g4_20260927/r1/README.md)
a échoué avant compilation/tests : exigence erronée d'un fichier `.rsp`
absent du CMake distant. Aucun nouveau chrono GPU ; VM arrêtée après
155,926 s d'allocation observée. Pas de relance automatique.

Nouveau prototype qualifié localement : [tri résident des survivants](docs/TRI_RESIDENT_DES_SURVIVANTS_20260927.md).
Accumulation en O(S+Q), tri GPU sur 64 bits, sortie compacte de 12 octets.
26 commandes locales passent et CUDA compile ; cette variante n'a pas
encore exécuté ses tests sur GPU. Moteur et chronos FULL inchangés.
Le [comparateur apparié](audits/b_q34_survivors_compare_20260927/README.md)
compile également : 52 lots/208 appels portables exacts, coûts complets et
premiers appels CUDA distingués. Mesures G4 de cette comparaison à suivre.

Nouveau port FULL : [manifeste et constructeur événementiel C++](docs/CONSTRUCTION_EVENEMENTIELLE_FULL_20260927.md).
Le manifeste, le constructeur événementiel puis sa variante à identifiants
stables sont qualifiés en Release/sanitizers : mêmes parents, ancres,
contributions et niveaux natifs sur 376 rejeux ; cas à 32 parents inclus.
La variante supprime plusieurs structures intermédiaires et passe aussi
1,9 million de contrôles de coupes. Les [vrais catalogues sont désormais mesurés](audits/b_full_a_real_20260927/RESULTATS.md) :
ng00 entière et uniforme8k/16k/32k, 120 comparaisons exactes. Min-label gagne
23,2 % face au prototype événementiel sur ng00, pas face au moteur natif.
Encore séquentiel, aucun gain FULL/GPU revendiqué.

Nouveau raccord implémenté : [rectangles filtrés et index résident](docs/RACCORD_RESIDENT_Q34_20260927.md).
50 commandes locales Release/sanitizers et le gate CUDA G4 passent.
Trame sans sol entière, mêmes survivants : adaptateur froid W4/W48
627,503/504,328 ms, hors front amont et hors FULL. Le filtre rectangle
n'est plus recalculé en CPU après sa décision GPU. Ce n'est pas encore
un gain établi sur le filtre GPU du moteur ; pas d'activation aveugle.
Reçu public et contre-audit publiés ; VM arrêtée après191,901s d'allocation.

Audit complémentaire du 27 septembre : [q34 hors S2](audits/b_q34_outer_ledger_20260927/README.md),
[construction FULL parallèle](audits/b_full_construction_parallel_20260927/README.md)
et [contrat des rectangles filtrés](audits/b_q34_filtered_contract_review_20260927/README.md).
La capture historique laisse386–390ms q34 hors S2 ; le port suivant doit
éviter le recalcul CPU des rectangles, sans limiter le chantier à S2.
Pour FULL, commencer par exporter les vrais blocs/cibles avant de remplacer
leur construction chronologique. Propositions, pas nouveaux gains mesurés.

Suite close du 27 septembre : [vrais S2, CUDA et chemin critique](docs/VAGUES_REELLES_ET_CHEMIN_CRITIQUE_20260927.md).
Le consommateur par vagues passe maintenant aussi les tests CUDA sur G4.
Trame sans sol entière : mêmes survivants,35ms de vagues, mais préparation
CPU9,9s et aucun gain net du raccord complet. Ne pas l'activer tel quel.
CPU8k/16k/32k et six coupes LiDAR sont publiés, avec régressions et compteur
de croissance défavorable. VM arrêtée, allocation188,416s ;100ms FULL reste ouvert.

Préparation G4 du 27 septembre : [consommateur q34 CUDA isolé](audits/b_q34_cuda_waves_20260927/README.md)
et [protocole gardé](audits/b_q34_cuda_session_20260927/README.md).
31 commandes de qualification portable passent ; à cette porte préalable,
CUDA restait à compiler et exécuter. Aucun gain GPU/FULL n'en était déduit.
La [relecture des chronos G4](audits/b_critical_path_20260927/README.md)
confirme que les K sont déjà encodés simultanément : il reste environ
252–256 ms de construction de tour hors cette fenêtre, sur les deux
premiers passages examinés. Une somme de gains par K n'est pas un gain mur.

Nouvelle suite du 27 septembre : [parents parallèles et vagues q34](docs/PARENTS_PARALLELES_ET_VAGUES_Q34_20260927.md).
Le raccourci FULL est maintenant testé en vrai multi-CPU, TSan compris ;
sur les vrais drafts LiDAR00, encodage natif/W4 163,973/120,793 ms en
sommes médianes K, pas un mur FULL. Le consommateur q34 épuise E avec
un scratch de vague Q et retrouve exactement la sortie S native.
Continuations préservées, reçus et contre-audit publiés ; moteur/GPU
inchangés, croissance résiduelle et contrat 100 ms toujours ouverts.

Suite actuelle du 27 septembre : [arène collective et FULL linéaire](docs/ARENE_COLLECTIVE_ET_FULL_LINEAIRE_20260927.md).
Six buffers collectifs q34, mêmes crédits/résidu, capacité LiDAR00
101,118→29,328 Mo ; dix mesures locales W1/W4. Le raccourci FULL sans
continuations est qualifié et mesuré, mais ne gagne pas sur LiDAR en CPU
scalaire. Priorité au raccord parallèle ; moteur/GPU encore inchangés.

Suite du 27 septembre : [ordre natif et vrais drafts FULL](docs/ORDRE_ET_VRAIS_DRAFTS_20260927.md).
Une nouvelle représentation partage des listes B ordonnées par classe A,
pour éviter le tri final des survivants. 15 mesures et portes locales passent.
Le test du premier encodeur sur de vrais drafts est négatif en temps CPU :
ne pas le porter tel quel. Une voie linéaire sans continuations est proposée
sur condition vérifiée ; moteur/GPU inchangés, aucun nouveau contrat acquis.

Dernière tranche du 27 septembre : [bandes directes et encodeur FULL](docs/PROTOTYPES_DIRECTS_ET_FULL_20260927.md).
La préparation des bandes ne construit plus les anciennes cellules ; les
survivants ordonnés passent une porte de raccord native. L'encodeur
structurel par préfixes/incidences retrouve les tableaux et refus actuels.
Prototypes CPU testés, pas encore intégrés au moteur : aucun nouveau chrono
FULL/G4 ni contrat 100 ms. Les reçus et contre-audits sont publiés.

Reprise précédente du même jour : [bilan et décisions de développement](docs/REPRISE_DEV_20260927.md).
Bandes q34 vérifiées (descripteurs LiDAR 34,810→4,913 Mo, pas encore de
gain FULL) et nouvelle G4 SPOT close : le filtre diamétral économise
environ 53 ms à chaud, chaîne K5 à 923 ms sur une trame sans sol.
100 ms reste ouvert. Le défaut d'alias de la banque publique FULL est
reproduit et documenté ; aucun changement moteur dans cette tranche.

Dernière tranche du 26 septembre : [plans q3/q4 par facteurs](receipts/q34_factor_plan_20260926/README.md).
Le prototype enlève 53–61 % des paires sur trois trames sans sol K5,
avant leur développement, sans produire la tour. 24 mesures locales,
gates Release/sanitizer et oracles passent. Les coûts de préparation et
de mémoire restent importants ; pas de port moteur aveugle ni de nouveau
gain G4 annoncé. Les amas gardent une croissance presque quadratique.
Le [raccord FULL proposé](audits/FULL_PARTAGE_INTER_ORDRES_20260926.md)
vise aussi l'écriture parallèle de la sortie explicite, sans la reconstruire
deux fois. Moteur et défauts inchangés depuis la tranche ci-dessous.

Reprise du développement le 26 septembre :
[transport des intérieurs q3 vers le catalogue](docs/REPRISE_DEVELOPPEUR_IDS_Q3_20260926.md)
et [protocole v30](docs/DEVELOPPEMENT_IDS_Q3_20260926.md).
Le levier `q3_interior_payload` est implémenté et reste opt-in :
[26 cas FULL sur G4](receipts/g4_q3_payload_20260926/README.md) passent,
mais le gain de chaîne K5 est petit et irrégulier. Sur 08/000000 K5/s8,
médianes de deux processus : 927,8 → 922,7 ms ; 100 ms reste hors cible.
Les [tests FULL 8k/16k/32k](receipts/q3_payload_local_20260926/README.md)
exposent surtout un résidu q34 presque quadratique sur les amas.
Priorités suivantes : éliminer les produits q34 avant leur expansion,
front GPU compact et construction événementielle de tous les ordres.
Les sections d'ouverture ci-dessous restent l'historique du 22 septembre.

```text
phase=exploration_v9_hors_registre
backend=reference_cpu (premier moteur v9 : generateur v8 + tour v7, 18 bits)
profile=quantized_u18_input_only (grille 1 mm, contrat temps)
mode=ouverture_audit_v8_et_v7
public_status=not_claimed
```

La v9 succède à la v8 comme chantier actif. Elle contient l'audit général de
la v8 (et de ce qui était bon en v7), le plan, l'héritage, les fausses pistes,
et depuis le 22 septembre au soir un premier moteur : la chaîne générateur
exact → catalogue recoupé → tour FULL, jugée par le juge T2 (voir la
[passation](PASSATION.md) et la [provenance](docs/PROVENANCE.md)). La v8 et la
v7 restent des sources différentielles et des réservoirs de fixtures, jamais
des autorités implicites.

## Objectif

Calculer la **tour HGP FULL** (minima Gabriel, multifusions, parents,
verticales et extension non régulière) d'une **trame SemanticKITTI brute
entière**, sur plusieurs scènes et séquences, en moins de **1 s** sur G4
pour toute la tour **K = 1..10** ; le repli est la tour **K = 1..5**, puis
la cible **100 ms**. La grille entière isotrope de **1 mm** est le profil
prioritaire choisi par l'utilisateur ; le float32 brut reste un objectif
secondaire distinct. Les trames **sans sol** sont un régime prioritaire
à mesurer séparément, mais ne remplacent pas le contrat brut. Les nuages
de plusieurs dizaines de millions de points restent une cible de passage
à l'échelle. Aucun de ces contrats de tour n'est acquis. Voir
la [synthèse](docs/AUDIT_V8_SYNTHESE.md) § 2 et § 9 et
l'[état courant des audits](audits/ETAT_COURANT.md).

## Verdict de la v8 au gel du 22 septembre, en quelques lignes

- La v8 livre un **générateur exact de candidats** q2/q3/q4 en entier, parallèle
  sur CPU, élargi à 18 bits, avec des certificats de rejet prouvés et une
  discipline de reçus exemplaire. 129 des 132 tests passent au commit audité
  12294241 ; les trois autres sont désactivés par construction.
- Elle **ne livre pas la tour** : ni catalogue canonique, ni forêts, ni parents.
  Le contrat n'est donc pas mesurable ; le flux seul coûte 6 à 100 fois le
  budget d'une seconde, dominé par l'atlas q4.
- Aucun code GPU. Quatre sessions G4, toutes CPU, toutes arrêtées et certifiées.
- La v7 avait la tour FULL (50k uniforme : 419 s à K10, 34 s à K5) : la v9
  réunit le générateur de la v8 et l'aval de la v7, mesurés de bout en bout.

## Commencer ici

1. [Passation](PASSATION.md) : état exact au 22 septembre, travail non commis
   d'autres acteurs, première tranche.
2. [Audit général de la v8](docs/AUDIT_V8_SYNTHESE.md) : verdict, contrats,
   chiffres, défauts, questions à l'utilisateur.
3. [Plan de la v9](docs/PLAN_V9.md) : phases, portes, règles de travail.
4. [Héritage v7 et v8](docs/HERITAGE_V7_V8.md) : ce qu'il faut porter, avec
   pins, et les fixtures à graver.
5. [Fausses pistes](docs/FAUSSES_PISTES.md) : ce qu'il ne faut pas rouvrir.
6. [Rapports détaillés de l'audit](docs/audit_v8/README.md) : douze lentilles
   contre-vérifiées.
7. [Reçu de l'audit](receipts/audit_v8_20260922/README.md) : inventaire épinglé
   et suite CTest rejouée au commit audité.

Auditeurs : [état courant](audits/ETAT_COURANT.md) et canal
[`audits/COORDINATION_MORSEHGP3D_V9.md`](../audits/COORDINATION_MORSEHGP3D_V9.md).

## Organisation

Structure attendue, calquée sur les versions précédentes : `src/`, `tests/`,
`oracle/`, `bench/`, `cmake/`, `docs/`, `audits/` (propriété des auditeurs
indépendants), `receipts/` (captures immuables). Les dossiers de code sont des
emplacements réservés. Conventions : C++20, `-Wall -Wextra -Wpedantic -Werror`,
namespace `mhgp9`, cibles et tests `mhgp9_*`, macros `MHGP9_*`, portes Python
sans `assert` (valides sous `python3 -O`).

Commandes (Boost obligatoire : `libboost-dev`, ou `-DBOOST_ROOT=<préfixe>`
d'un `libboost1.83-dev` extrait, voir le [plan](docs/PLAN_V9.md) V9-0) :

```bash
cmake -S morsehgp3D_v9 -B build/v9 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v9 --parallel
ctest --test-dir build/v9 --output-on-failure -L gate
./build/v9/mhgp9_tower_probe <trame.u32le> K workers [--s=8] [--static=T] [--no-tower]
```

Options CMake : `MHGP9_SANITIZE` (ASan/UBSan), `MHGP9_TSAN`.

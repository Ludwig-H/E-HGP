# Suivi développeur et audit FULL → points

État courant du 2 octobre 2026. Ce fichier remplace ses résumés successifs ;
les preuves et premiers échecs restent dans les reçus liés ci-dessous.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_full_forests`, `not_claimed`.

Priorité utilisateur : FULL K1..5 ≤200 ms G4, puis K1..10 ; la hiérarchie
et HDBSCAN/Zoltan viennent après. Inspiration critique de toute la v10 autorisée.

## Produit et qualification courante

Les [rejets J2 exacts](../docs/CENTER_REGION.md) et le [domaine FULL possédé](../docs/FULL_DOMAIN.md)
sont qualifiés à **`7f1922c77`** : G41398/1398, ASan18 supplémentaire73/73,
154 mutants détectés, dont deux refus de compilation attendus.
[Capture région](../receipts/center_region_20261002/README.md) : le banc mono
atteint son délai global820s ;14 succès,15 délais30s,2 omissions causales,
5 unités sans résultat persistant. Aucun échec produit détecté. G4 arrêtée,
clés et verrou retirés ; le statut global `failed_remote` reste conservé.

À K5/u21, le catalogue seul prend21,734s sur08/0 et17,374s sur08/100 ;
u24 vaut21,996s et17,509s. Pas de résultat u21/u24 pour08/200 dans ce lot.
Treize résultats appariés au catalogue précédent gardent exactement les
mêmes octets. Ces temps ne prouvent aucun contrat FULL **200ms**.

Le [catalogue parallèle](../docs/CATALOGUE_PARALLELE.md), les cellules,
la localisation et les descentes sont qualifiés àc1046dfc7 :1779/1779,
ASan18 107/107 et189 mutants. [Reçu compact](../receipts/catalogue_parallel_20261002/README.md).
Trente essais K5 réussissent ; les six K10 atteignent15s. Premiers essais
K5/W48 sur les trois LiDAR :4,482/3,075/3,996s u21,4,637/3,247/4,215s u24.
Les sorties exactes et les comptes géométriques appariés sont identiques.
Le tri coûte0,9–1,4s ; une tâche domine presque entièrement chaque passe
parallèle sur08/0. Cache J2 et tri indirect sont deux
[options en préparation](../docs/CATALOGUE_OPTIMISATIONS.md), défaut inactif ;
le déséquilibre des suffixes reste un troisième levier distinct.

Les [plateaux et verticales](../docs/FULL_FORESTS.md) sont implémentés,
avec qualification native en cours ; aucun chrono FULL encore acquis.

Les [MEB/census qualifiés](../receipts/meb_20261002/README.md) à`25792084e`
et l'[index](../receipts/index_20261002/README.md) à`e8520481d` restent les bases.
Les requêtes MEB artificielles ne sont pas des descentes FULL. L'erreur de
vie de l'ancre C++20 de `meb1`, corrigée dans le banc, et le stockout `meb2`
sont conservés ; le décodeur signé était correct. L'audit indépendant
`cb5a69ef3` est relu sans transfert de ses modèles aux nouveaux ports natifs.
Ses remarques ne font pas autorité par elles-mêmes : la fixture J2 doit
réellement dépasser64bits en u21, et un coût théorique évitable ne prouve
aucun gain chronométré. Les conclusions sont bornées aux preuves vérifiées. La revue18 `096323c45`
est intégrée : classificateur sans payload proposé, mais rejeu exhaustif
maintenu ; K12 ne nécessite pas intrinsèquement une MEB13. La nouvelle garde
des diagnostics borne la somme par min(W,J) fois le mur de la phase.

Les deux démarrages de qualification de `9c883b93f` ont échoué avant worker
par manque de capacité en zone b. Les reçus gardent `shutdown_uncertified` ;
relectures externes répétées : cible arrêtée, génération inchangée, opérations
closes. Nouvelle G4 SPOT créée en zone c par le créateur gardé, sous verrou
commun : deux gardes certifiées puis arrêt ciblé, clé retirée. Le contrôleur
nomme désormais cette cible explicitement, avec les mêmes gardes/provenance.
`parallel3` à2e3 échoue avant build : g++ et CMake absents de l’image neuve.
Résultats récupérés, arrêt ciblé et retrait de clé certifiés ; outillage gardé
minimal installé en session gardée `tools1`, arrêt et clé retirée certifiés.
`parallel4` échoue à compiler le test catalogue parallèle (.hex inexistant,
alias detail ambigu), puis deux gates mutant tower détectent un mutant
invalide par compilation (paramètres inutilisés). Aucun chrono exécuté ;
ASan18 séparé107/107 passe. Corrections de tests seules, première capture
conservée ; arrêt de la cible et retrait des clés certifiés.
`parallel5` àc104 clôt la matrice1779/1779 et ASan18 107/107 ;
30 essais K5 réussissent, les six K10 atteignent15s. Aucun chrono FULL.
La cible est arrêtée, clés retirées ; la capture compacte est close.
`full1` à80e77544e révèle un défaut du test IO : reversed(range(3)) est
consommé pour XYZ puis réutilisé pour les IDs, donnant un fichier vide.
La correction matérialise l'ordre une fois et enrichit les diagnostics.
La traceback initiale reste conservée ; elle n'avait pas conservé les flux
de l'enfant. Le rejeu natif corrigé reste nécessaire ; aucun succès transféré.
`full2` refuse avant démarrage faute d'espace local ; deux paires de paquets
strictement identiques sont dédupliquées par liens durs, tous chemins et
empreintes conservés. `full3` reprend c6ca345e0, sans options nouvelles.
Lecture LIVE de full3 : matrice et ASan18 verts,14 fixtures différentielles
v10 aux trois profils conformes. Premier FULL08/0/u21/K5 :21,294s, dont
4,408s domaine et16,885s forêts/verticales. Campagne encore ouverte ; ces
mesures ne sont pas une clôture ni un contrat200ms. Le ledger actuel ne
sépare pas les durées classification/rejeu/verticales.
Ni les comptes théoriques ni les temps de R2 ne sont hérités.

## FULL → points : verrous conservés

Le [suivi exact](../receipts/audit_full_hierarchie_20261002/suivi_verrous/README.md)
et les fixtures de la référence conservent les faits suivants :

- `cover` et MR₂-bord ne sont pas des hiérarchies équivalentes. Pour
  `{0,2,5}`, K2, cover contient AB ; MR₂-bord ne publie que ABC au plateau
  de jonction. Le meilleur IoU de la cible AB vaut 1 contre 2/3. Les dates
  d'attache diffèrent aussi : aucun facteur global ne les identifie.
  Ce témoin ne prouve aucune supériorité générale de cover sur HDBSCAN.
- Les groupes core de plusieurs K peuvent se croiser. La fixture
  `{0,10,11,26,27,45,46}` garde `{0,10,11}` à K1 et `{10,11,26,27}` à K2,
  avec leurs naissances et parents exacts. Une hiérarchie commune doit
  rendre explicites les pertes dues à la contrainte laminaire.
- Séparer présence d'un groupe dans FULL, pertes de projection, incompatibilité
  de groupes et pertes du sélecteur. Un score au meilleur bloc borne la
  sélection dans cet arbre ; il ne fournit pas une partition simultanée.
  Core/cover ensemblistes et projection LCA exclusive restent distincts.

La qualification native de FULL, la hiérarchie sur les points et la
condensation restent à livrer. Les faits de référence ne les qualifient pas.

## Gardes du futur raccord FULL

La contrelecture des lemmes B–F est favorable avec Q1 corrigé : quotient
local, surjection vers les composantes globales, raffinement exhaustif,
représentants valides et déduplication des racines avant le plateau.
Un terminal mémoïsé `(b,k)` est valable à coupe fermée `a≥λ_b`, mais à
coupe ouverte seulement `a>λ_b`. Avant un parent, conserver
`λ_b≤β(R)<λ_parent` ; ne pas remplacer trop tôt tous les représentants.
La terminaison utilise les niveaux de toutes les k-parties. Une boule
admise n'est pas forcément nécessaire à π₀ ; Euler reste un diagnostic.

Conserver plateaux et cohortes de départ atomiques, `mcs` même avec
`allow_single`, mémoire simultanée complète et distinction masse
recouvrante/exclusive. La stabilité ER0h à arbre et activations fixes
n'est pas une stabilité générale sous déplacement des coordonnées.
Le modèle pondéré FULL et les comparaisons EOM proches d'une égalité
restent ouverts ; aucun label SemanticKITTI n'est utilisé.

## Constats antérieurs clos et limites restantes

Les défauts F3/F6, Result sur refus, arrêts anormaux usurpés, collecteurs
et mutants sont corrigés avec leurs premiers échecs conservés dans les
[reçus de développement](../receipts/developpement_20261002/README.md).
La précision numérique est attachée aux profils réellement testés ;
q3 reste large en 21/24 et q1/q2/q4 utilisent i128 sous leurs bornes propres.
Le test ASan24 ne couvre pas q3 natif 18 : d'où son complément distinct.

`MemoryBudget::admit` exige un pilote unique. Dans la matrice fonctionnelle,
les descendants ne sont fermés qu'en fin de commande par le worker ; les
sondes gardent `isolation=not_certified`. Le banc démarre dans une commande
séparée. Les builds, tests et chronos natifs se font exclusivement sur G4,
via une session gardée avec arrêt ciblé et retrait des clés certifiés.
P2 traçabilité de l’audit17 : les pilotes persistent maintenant l’intention
avec argv/profil/hashes avant subprocess.run, puis le résultat avant décodage.
Une intention seule ne prouve ni PID ni lancement ; modèles d’interruption verts.

L'audit v10 est consolidé sans prétention d'exhaustivité : les rapports
privés absents restent recensés dans le
[rapprochement des sources](../receipts/audit_full_hierarchie_20261002/suivi_verrous/commit_reconciliation.json).
Aucun gain de performance, contrat FULL, résultat GPU ni avantage général
sur HDBSCAN n'est déduit de ces audits.

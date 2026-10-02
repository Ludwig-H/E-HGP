# Suivi développeur et audit FULL → points

État courant du 2 octobre 2026. Ce fichier remplace ses résumés successifs ;
les preuves et premiers échecs restent dans les reçus liés ci-dessous.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_meb`, `not_claimed`.

Priorité utilisateur : FULL K1..5 ≤200 ms G4, puis K1..10 ; la hiérarchie
et HDBSCAN/Zoltan viennent après. Inspiration critique de toute la v10 autorisée.

## Produit et qualification courante

La [MEB bornée et son census](../docs/MEB.md) sont qualifiés à
**`25792084e`** : G4 1266/1266, complément ASan18 55/55, 141 mutants détectés.
372 contrôles natifs ; 350 requêtes Fraction par profil et 14 631 contrôles.
18/18 mesures, 864 requêtes choisies, 216 complètes / 648 saturées, aucune
divergence interprofils. Sur les trois LiDAR/u21,48 MEB+census prennent
1,377–1,777 ms ; ces parties artificielles ne sont pas des descentes FULL.
[Preuves et chronos](../receipts/meb_20261002/README.md). G4 arrêtée,
clés retirées ; l’échec de capacité `meb2` sans worker est conservé avec
vérification externe de la cible arrêtée, sans nouvelle génération.

`meb1` avait détecté une durée de vie invalide de l’ancre dans le range-for
C++20 du banc, corrigée avant le rejeu. Le décodeur signé était correct.
Les premiers échecs ne sont pas effacés. Le support strict canonique LOCAL
reste distinct du support canonique GLOBAL et le census compte les sites.

L'[index possédé](../docs/INDEX.md), qualifié à `e8520481d`, construit son
arbre en 0,341–0,418 ms sur LiDAR/u21 après Cloud. Le catalogue qualifié à
`ffc2ff95f` prend encore 19,78–24,96 s à K5/u21 en mono ; K10 expire à 30 s.
[Capture index](../receipts/index_20261002/README.md),
[capture catalogue](../receipts/catalogue_q4_20261002/README.md).
Le contrat FULL **200 ms** reste ouvert ; la forêt native reste à construire.

Tranche en préparation : rejets J2 exacts des bissectrices/droites de
centres, port critique R2/v10 avec nouvelles bornes T0 aux trois profils ;
et FullDomain possédant index, catalogue et lookup exact S*. Les masques de
dominance existants suffisent aux paires ; pas de table cubique ni nouveau
gros tableau dans la feuille. Les contacts et faces obtuses sont conservés.
Le juge num indépendant résout une droite rationnelle contre les six faces,
sans reprendre le test SAT. Qualification native de ces ajouts à venir.

La voie FULL retenue utilise le support GLOBAL dans un domaine fermé,
comme R2, sans nouvelle clé PGCD immédiate. Un miss du support local impose
un census et une canonicalisation globale avant de conclure à l’absence.
Les revues `a7a38137c` et `4a3c91d42` restent intégrées ; preuves de bornes
discrètes et autres microvariantes non portées. Puis cellules régulières,
traces étendues exactes, descentes, plateaux et verticales.

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

L'implémentation native de FULL, de la hiérarchie sur les points et de la
condensation reste à livrer. Les faits de référence ne la qualifient pas.

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

L'audit v10 est consolidé sans prétention d'exhaustivité : les rapports
privés absents restent recensés dans le
[rapprochement des sources](../receipts/audit_full_hierarchie_20261002/suivi_verrous/commit_reconciliation.json).
Aucun gain de performance, contrat FULL, résultat GPU ni avantage général
sur HDBSCAN n'est déduit de ces audits.

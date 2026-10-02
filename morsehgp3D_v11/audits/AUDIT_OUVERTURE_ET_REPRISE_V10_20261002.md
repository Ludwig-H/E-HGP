# Suivi développeur et audit FULL → points

État courant du 2 octobre 2026. Ce fichier remplace ses résumés successifs ;
les preuves et premiers échecs restent dans les reçus liés ci-dessous.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_meb`, `not_claimed`.

## Produit et qualification courante

L'[index global possédé](../docs/INDEX.md) est qualifié à **`e8520481d`**.
Il possède le Cloud, rend K témoins stricts ou tout I/U, et préserve les
coquilles et la propriété sur refus. Le census compte les sites, sans
qualification du modèle pondéré. Construction O(n) après Cloud ; aucune
borne sur le nombre de descentes d'une future tour n'en découle.

G4 : Release 251/251 ; ASan24/TSan21/profils21/24 176/176 chacun ;
poison 177/177 ; complément num/index ASan18 36/36. Les 131 mutants
sont détectés, dont deux refus de compilation attendus dans core.
Index : 845 contrôles natifs, 1 010 requêtes Fraction/profil et 36 020 contrôles
normal/−O. Les bornes num ajoutent 183 contrôles natifs et 391 cas Fraction.

Les 18 essais sur six entrées entières passent. Les 1 152 réponses sont
contrôlées par scan ; six comparaisons interprofils donnent les mêmes
sorties sémantiques et compteurs. Sur LiDAR/u21, l'arbre prend 0,341–0,418 ms
après Cloud, et les 64 requêtes choisies 0,621–0,810 ms. Une répétition,
trois trames d'une même séquence 08, grille 1 mm commune aux profils18/21/24.
Les requêtes choisies ne sont pas des descentes FULL. Réservations
Cloud+index 1,546–1,939 Mo, hors catalogue ; ni RSS ni mesure GPU.
[Reçus et lecteur LIVE](../receipts/index_20261002/README.md),
[état détaillé](../docs/DEVELOPPEMENT.md). G4 arrêtée, clés retirées.

Le catalogue séquentiel et le niveau q4 différé restent qualifiés à
`ffc2ff95f` : derniers chronos LiDAR/K5 u21 **19,78–24,96 s**, u24
19,67–24,79 s. K10 expire au plafond 30 s ; le contrat FULL 100 ms demeure
ouvert. Les 15 sorties terminées égalent les précédentes, comptes et
réservations compris. [Capture q4](../receipts/catalogue_q4_20261002/README.md).
Les centres sont calculés avant les rejets ; les niveaux seulement après
admission. Les certificats de familles cosphériques ou de témoins communs
restent des pistes distinctes, sans gain hérité.

Tranche courante : [MEB native bornée](../docs/MEB.md) et census du même
index implémentés, qualification en préparation. Les revues `a7a38137c`
confirment index/capacité et fixent les gardes du raccord FULL ; bornes
discrètes non portées. Puis identité exacte signée et descente FULL.
La fixture préparatoire à quatre sites du plan courant distingue
présentation génératrice, support local strict et support global.
Le suivi indépendant `737313a96` fixe le tuple primitif signé de boule ;
PGCD, division exacte et encodage restent à qualifier. La première MEB
ne peut prendre son support local pour une clé canonique du catalogue.

Le suivi indépendant `108350f45` est intégré : revue favorable LB/UB,
recoupe de la capture q4 et coûts distingués des comptes supprimés.
[Réponse aux contrats d'index](../docs/INDEX.md) : propriété transférée,
census géométrique explicite ; token commun et régime unitaire restent
obligatoires avant FULL. UB* exact par axe demeure une option non portée.

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

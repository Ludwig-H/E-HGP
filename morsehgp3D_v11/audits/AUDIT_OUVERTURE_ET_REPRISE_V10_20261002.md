# Suivi développeur et audit FULL → points

État courant du 2 octobre 2026. Ce fichier remplace ses résumés successifs ;
les preuves et premiers échecs restent dans les reçus liés ci-dessous.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_catalogue`, `not_claimed`.

## Produit et qualification

Le catalogue séquentiel et le niveau q4 différé sont qualifiés à `ffc2ff95f`
en u18/u21/u24, avec ASan/UBSan u24 et complément num u18. Défaut u21,
option u24 ; agrandir le domaine ne raffine pas la grille 1 mm utilisée.
Les coordonnées physiques restent `o+hq`, les niveaux physiques `h²β`.
Les trois trames sans sol entières sont de la même séquence 08.

Le dernier banc clos donne 19,78–24,96 s pour LiDAR/K5/u21 et
19,67–24,79 s en u24. Les cinq cas K5 réussis ont les mêmes sorties et
travail géométrique aux trois profils ; les 15 sorties égalent octet pour
octet les précédentes, avec mêmes réservations Buffer. Les 18 délais de 30 s et trois omissions restent
publiés. Le temps couvre le catalogue CPU mono et ses deux passes, tri et
sorties en mémoire, hors lecture/Cloud/segmentation/sérialisation.
**Le contrat FULL de 100 ms n'est pas atteint ; K10 n'est pas qualifié.**
[Reçus courants et ASan18](../receipts/catalogue_q4_20261002/README.md),
[historique catalogue](../receipts/catalogue_20261002/README.md),
[état détaillé](../docs/DEVELOPPEMENT.md).

Tranche close : niveau q4 différé, 119 mutants détectés, Release 229/229,
ASan24/TSan21/profils 21/24 154/154 chacun, complément num ASan18 14/14.
Le candidat fermé possède ancre/N/D ; positivité, propriété, census,
support canonique et admission précèdent la matérialisation du niveau.
Les deux passes gardent leurs comptes identiques. Deux nouveaux compteurs
séparent centres q4 non dégénérés et niveaux effectivement matérialisés ;
les neuf anciens compteurs, sorties canoniques et réservations servent
au différentiel. Environ 99,7 % des niveaux q4 candidats sont évités sur LiDAR,
avec des rapports de temps observés seulement ×1,03–1,05 (un essai).
Q2/q3, feuille 32 et capacité 256 restent inchangés.
Le suivi indépendant `d40585570` est intégré :
[contrat d'admission](../receipts/audit_independant_20261002/q4_candidate_contract_review_7/README.md),
[preuve et limites](../receipts/audit_independant_20261002/q4_level_math_review_7/README.md).

Les certificats de familles restent des pistes séparées : cosphéricité de
toute la liste et centre propriétaire avant I vide/U=L ; ancre minimale
réservée à qmin4 ; puissance positive d'une extension seulement nécessaire ;
intérieurs coplanaires communs aux extensions q4 sans réutiliser le census
q3 entier. Aucun de ces filtres n'est inclus dans le port du niveau différé.

La suite ouvre l'index global exact pour les descentes, puis le raccord FULL ;
le [plan courant](../docs/DEVELOPPEMENT.md) fixe ses premières portes.

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

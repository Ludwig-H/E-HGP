# Suivi développeur et audit FULL → points

État courant du 3 octobre 2026. Ce fichier remplace ses résumés successifs ;
les preuves et premiers échecs restent dans les reçus liés ci-dessous.
Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_full_forests`, `not_claimed`.

Priorité utilisateur : FULL K1..5 ≤200 ms G4, puis K1..10 ; la hiérarchie
et HDBSCAN/Zoltan viennent après. Inspiration critique de toute la v10 autorisée.
**Le contrat 200 ms reste ouvert.**

## État mesuré

La [capture FULL memo1](../receipts/full_memo_20261003/memo1/README.md),
source `c2c3e0323`, qualifie2475/2475 portes plus178/178 ASan18.
Douze LiDAR entiers appariés u21/u24 passent : FULL K1..5 avec mémo
**10,069–14,690 s**, contre13,313–18,642 s sans mémo sur cette source.
Calendrier17/18, une omission uniforme32k u21 avant lancement, aucun K10.
Le worker rend1 pour calendrier incomplet ; arrêt ciblé, clés et verrou
sont certifiés. Les238 mutations code/ligne et2 refus de compilation
attendus restent distingués. Les empreintes des gros payloads supprimés
sont enregistrées ; le lecteur ne prétend pas les rehacher aujourd'hui.

La [capture catalogue assembly1](../receipts/catalogue_assembly_20261003/assembly1/README.md),
source `4b8e04be6`, est close et conforme :2535/2535 plus178/178 ASan18,
36/36 essais, zéro omission/divergence,244 mutations code/ligne et2 refus
de construction attendus. Catalogue K5/W48, front adaptatif et assemblage
parallèle : **1,429–1,900 s** sur les trois LiDAR u21/u24. L'intervalle
assemblage seul descend à4,004–5,219 ms ; génération count et fill dominent
encore. Même profil : sorties brutes identiques entre options ; comptes
logiques et hashes sémantiques identiques aussi entre profils. Aucun
transfert de ces chronos au FULL. Un seul processus par case, ordre fixe.

Les deux captures utilisent les mêmes sous-nuages1mm entiers sans sol de
08/000000,100,200 (39885/35551/45845 sites), donc une seule séquence.
Cloud, Pool, lecture et sérialisation sont séparés des API mesurées ;
segmentation/préparation hors ligne ne sont pas chronométrées ici.
Calcul CPU sur G4, sans accélération GPU. Réservations Buffer, pas RSS.

## Développement en cours

La [forêt parallèle régulière](../docs/FULL_PARALLEL.md), source `e5f6a5683`,
garde lanes/mémos privés, publication DSU dans l'ordre exact et fermeture
atomique des plateaux. La session gardée `forest1` est en cours ; sa première
configuration Release rend502/503. Le test ligne024 attendait à tort une
cellule étendue à K2 : I={2}, U={0,4}, mais m=qmin=2 est régulier. Correction
de l'attendu et contrôle de mutant en préparation ; aucune qualification
ni mesure de cette nouvelle voie n'est acquise. Les oracles géométriques
Release passent ; ce fait seul ne clôt pas les autres portes.

Le prédicat de distance i64 à`60eabc589` et la forêt parallèle doivent avoir
leur qualification propre. Les nouvelles pistes sont une seule passe
catalogue par blocs d'arène budgétés et une voie q3 i128 vérifiée avec repli
exact. Aucun gain chronométré n'en découle. Le raccourci singleton exact
éviterait environ2,23–2,60 % des étapes payées de memo1 : ce compte ne devient
pas une économie de temps mesurée.

La critique s'applique aussi aux auditeurs : proposition de doublement de
buffers rejetée après confrontation à ARCHITECTURE§7.1 ; durée du catalogue
entier corrigée lorsqu'elle était présentée comme durée d'assemblage ;
non-régularité distinguée d'un intérieur non vide ; gardes des diagnostics
resserrées lorsque des traces pouvaient rester sans cellule explicative.
Toute correction garde le premier échec et demande ses propres portes.

## Preuves antérieures utiles

- [FULL initial](../receipts/full_20261002/README.md) :42 comparaisons exactes
  v10 sur petites fixtures ; les comparaisons LiDAR FULL v10 restent ouvertes.
- [Balayage et classification](../receipts/full_sweep_20261002/README.md) :
  235254420 marches de parents évitées sur08/0, avec12N octets temporaires ;
  le gain chronométré mélange plusieurs changements, sans ablation isolée.
- [Cache J2 et tri](../receipts/catalogue_optimizations_20261002/README.md),
  [catalogue parallèle](../receipts/catalogue_parallel_20261002/README.md),
  [MEB](../receipts/meb_20261002/README.md), [index](../receipts/index_20261002/README.md)
  et [région exacte](../receipts/center_region_20261002/README.md) gardent leurs
  sources et périmètres propres, sans qualification héritée des autres.
- Les trois premiers refus adaptatifs restent dans
  [les captures](../receipts/catalogue_adaptive_20261002/) : compilation
  `memo_fault` sous avertissements stricts, puis délais550/700s de la porte
  mutants. La réussite ultérieure avec32 fils mutants ne réécrit pas ces essais.
  Les premiers échecs d'outillage restent dans
  [le développement](../receipts/developpement_20261002/README.md).

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

La qualification FULL sur petites fixtures ne ferme ni le différentiel LiDAR
entier, ni la hiérarchie sur les points, ni la condensation.

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

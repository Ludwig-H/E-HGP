# Suivi développeur et audit FULL → points

État courant du 3 octobre 2026. Ce fichier remplace ses résumés successifs ;
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
parallèle sur08/0. Cache J2 et tri indirect sont désormais qualifiés à `df069960a` :
2115/2115, ASan18 139/139, 210 mutants et 36/36 essais sans divergence.
[Capture close](../receipts/catalogue_optimizations_20261002/README.md) :
catalogue K5/W48 modes combinés, u21 3,239/2,115/2,790 s et u24
3,274/2,178/2,861 s. Un essai par cellule, ordre des options fixe ; aucun
chrono FULL transféré. Défauts inactifs conservés. Le
[front adaptatif](../docs/CATALOGUE_FRONTIERE_ADAPTATIVE.md) est implémenté
séparément, avec diagnostics possédés et ablation à qualifier.

Les [plateaux et verticales](../docs/FULL_FORESTS.md) sont qualifiés à
`c6ca345e0` : 1971/1971, ASan18 139/139 et 42 comparaisons v10 exactes
sur petites fixtures. La [première capture FULL](../receipts/full_20261002/README.md)
mesure 15,190–21,725 s pour K1..5 sur les trois sous-nuages entiers u21/u24.
Treize essais réussis et onze omissions de budget ; aucun K10 exécuté.
Campagne close en `failed_remote`, arrêt et retrait des clés certifiés.
Le contrat 200 ms reste ouvert ; les gros fichiers FULL ayant été supprimés,
le lecteur relit leurs hashes enregistrés, pas leurs octets.

La [nouvelle classification et le balayage des verticales](../docs/FULL_OPTIMISATIONS.md)
sont qualifiés à `12f49d0ca` : 2229/2229 et ASan18 158/158.
La [capture sweep2](../receipts/full_sweep_20261002/README.md) conserve 13 succès K5,
11 omissions budgétaires et aucun K10, arrêt ciblé certifié. Sur 08/0, le FULL
ancien compte 235254420 marches de parents et 2319956 requêtes verticales.
Le balayage DSU supprime les marches répétées, avec 12N octets temporaires.
Le classificateur évite les traces inutiles ; leur rejeu demeure exhaustif.
Sur 08/0 u21, les plateaux coûtent 13,398 s sur les 15,704 s de forêt ;
la classification vaut 65,5 ms et les verticales 1,975 s. Le gain de forêt
depuis c6 mélange classification et balayage ; le gain FULL inclut aussi
le cache J2 et le tri indirect. Il ne mesure pas le balayage isolément.

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

Les premiers échecs restent dans les reçus, sans accumulation de notes
périmées ici : capacité/outillage G4, tests parallèles et mutant invalide,
puis itérateur consommé du test IO `full1`. Ce dernier est corrigé et
requalifié dans `full3`. Les refus locaux `full2` (espace) et `optimizations1`
(garde 3600 s) ont précédé tout worker. Les paquets identiques sont liés
après vérification SHA, tous les chemins et octets conservés. Les plans
respectent désormais la garde existante, sans l'étendre.

`optimizations2` est close : worker 0, arrêt ciblé, retrait des clés et
résultats certifiés. Le tri indirect réduit le tri de 1091 à 54 ms sur 08/0
u21, mais ralentit lecture des niveaux et assemblage ; le catalogue total
passe de 4,418 à 3,239 s. Les hashes des gros payloads sont enregistrés,
leurs fichiers ayant été supprimés après décodage.

`sweep2` est close en `failed_remote` pour calendrier incomplet, avec
FULL K1..5 entre 13,535 et 19,177 s. Les 13 résumés sémantiques et hashes
bruts de même profil sont identiques à ceux de `full3`, sans relecture
des gros payloads supprimés. Le contrat 200 ms reste ouvert.
La MEB par diamètre et le [mémo avant MEB](../docs/DESCENT_MEMO.md) sont
qualifiés à `c2c3e0323` : **2475/2475**, ASan18 **178/178**, 240 mutants
causaux (238 code/ligne, deux refus de construction attendus). [Capture memo1](../receipts/full_memo_20261003/memo1/README.md) :
17 succès K5 sur18, dernière paire synthétique32k incomplète par budget,
aucun K10. Les douze LiDAR appariés gardent leurs sorties vérifiées ; FULL
mode3→7 vaut **18,642→14,546 / 13,313→10,069 / 15,686→12,082 s** en u21,
et **18,595→14,690 / 13,326→10,119 / 15,649→12,082 s** en u24.
Un processus par configuration, CPU sur G4 ; pas de contrat200ms acquis.
Les tables12/13MiB sont rendues ; le pic global reste ici dominé par le
catalogue. Campagne485,980s, dont261,706s de processus et223,638s de collecte
sémantique ; huit résumés réutilisés après rehachage. Arrêt ciblé, deux clés
et verrou certifiés. Les gros payloads supprimés restent des hashes enregistrés.

Sur08/0u21, le mémo réduit les plateaux K5 de8,556 à6,386s. La baisse des
présentations MEB n'est pas convertie en gain temporel théorique. Le port
distance i64 de `60eabc589` garde compteurs et ordre, sans qualification
native héritée ; la voie q3 i128 avec repli reste une étude, pas un gain.

`sweep1` avait refusé avant GCP pour espace. Les copies strictement identiques
de paquets clos ont été liées après vérification SHA, tous chemins conservés.
Les nouveaux exports Git omettent seulement cinq copies historiques,
toujours conservées dans Git ; aucun source natif retiré. Le prochain banc
adaptatif garde les rapports complets en gzip et le plafond de résultats.

[`adaptive1`](../receipts/catalogue_adaptive_20261002/adaptive1_failure/README.md),
source `f425c5fe7`, est close en échec de qualification :
2360/2361 portes de matrice et ASan18 161/161 ; mutants20/21, dernière
porte tour sans résultat après le délai550s. Le banc a refusé avant tout
essai ; aucune mesure adaptative ni FULL par diamètre n'en découle.
L'arrêt est certifié. Le premier retrait OS Login a été refusé pour
mutation concurrente ; la reprise gardée distincte du 2 octobre à23:40:59UTC
confirme le retrait, sans réécriture du reçu initial.
Le collecteur rehache chaque payload avant réutilisation éventuelle de
son résumé ; le gain de décodage n'est jamais soustrait au temps moteur.

[`adaptive2`](../receipts/catalogue_adaptive_20261002/adaptive2_failure/README.md),
source `18d1ba695`, refuse également les benchmarks : le nouveau
test `memo_fault.cpp` ne compile pas, car deux instructions après un `for`
sur une même ligne déclenchent `-Werror=misleading-indentation`. Les deux
portes du binaire absent ne se sont pas exécutées ; aucun défaut géométrique
n'en découle. La correction explicite la portée de la boucle. Arrêt ciblé,
suppression des deux clés et libération du verrou certifiés sans avertissement.
Matrice2462/2475, supplément176/178 : quatorze lancements impossibles et
le témoin de la campagne mutants tour non compilé, aucun mutant tour jugé.
`adaptive3` à `f718f53aa` passe toutes les configurations fonctionnelles,
mais la dernière porte mutants tour reste sans verdict au délai700s.
Matrice2474/2475, ASan18 178/178 ; aucun benchmark lancé. La clôture certifie
l'arrêt, les deux clés et le verrou, sans erreur ni avertissement.
La répartition corrigée réserve32 fils aux mutants dans le budget48,
sans retrait de porte ; memo1 termine ces21 portes en397,196s. Ce succès
ne transforme pas les trois captures précédentes en qualifications.

L'[assemblage optionnel par blocs](../docs/CATALOGUE_ASSEMBLY.md) est préparé
séparément : N−1 comparaisons adjacentes auparavant répétées disparaissent,
les écritures finales sont disjointes, les groupes égaux traversant des blocs
conservent leur premier représentant non réduit. Scratch additionnel32J octets,
J≤1024, aucun gain de temps encore mesuré. Le banc prévu compare36 processus
K5 entiers ; ses contrôles Python normal/−O passent. La session gardée
`assembly1` qualifie puis mesure **4b8e04be6**, sans les ports suivants.

La forêt parallèle régulière est en développement, inactive par défaut :
lots Q bornés, lanes logiques indépendantes du nombre de workers, mémos
privés, DSU publié dans l'ordre exact et plateaux clos seulement au bon rang.
Les cellules étendues gardent le parcours exhaustif après join des workers.
Ni chronos ni qualification native encore acquis pour cette voie.

La critique vaut aussi pour nos propres ports et leurs juges : deux nouveaux
lanceurs Python dépendaient à tort de PYTHONPATH ; imports explicites corrigés
et rejoués normal/−O sans cette variable. Un ticket de résumé ne devient
réutilisable qu’après les contrôles complets du banc et le nettoyage réussi.
Dans les tranches précédentes, un mutant du cache qui
aurait échoué à compiler a été corrigé avant qualification ; la borne des
unions du balayage a été corrigée en nombre d'arêtes, car ses nœuds de fusion
appartiennent eux aussi au DSU. Aucun chrono ni preuve R2 n'est hérité.

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

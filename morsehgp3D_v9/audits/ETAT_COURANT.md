# État courant des audits v9

22 septembre 2026. Code jugé : **`e28296bb`** (moteur chronométré à
`d2700314`, puis portes et protocole G4) ; dernier audit B
relu : `d4942b4e`. Cadre : `exploration_v9_hors_registre`,
`reference_cpu`, `quantized_u18_input_only`, `not_claimed`. Ce fichier est le
verdict mutable du dossier ; les notes datées conservent les démonstrations et
références. Les auditeurs écrivent dans `audits/` et communiquent au
constructeur uniquement des constats utiles.

## Verdict sur le premier moteur

La chaîne générateur v8 porté → catalogue de BallKeys recoupées → tour FULL
v7 portée existe. Elle recalcule clé, niveau, intérieur et coquille de chaque
boule **émise**, vérifie `q_min` pour les coquilles étendues et refuse
transactionnellement une coquille de plus de 12 sites. La forme q4 est
indépendante de l'orientation du support. La contrelecture n'a pas trouvé de
défaut concret sur ces chemins ; voir le [contre-audit A du
moteur](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md) et la [lecture B de
FULL](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md).

Un rejeu indépendant local de `d2700314` passe **20/20 CTests** sans saut ;
B retrouve ces 20 portes en Release et sous Clang ASan/UBSan. Le commit
`ba762036` ajoute une porte arithmétique u18 et un CTest du mutant de census
(22 portes annoncées par le développeur). `e28296bb` annonce 28 CTests,
dont trois mutants T2 et trois refus CLI ; ces nouvelles portes n'ont pas
été rejouées indépendamment ici. La cible arithmétique reste compilée en
mode produit : son mutant `level-trunc-hi` est inactif, donc sa valeur
d'oracle numérique est distincte d'une preuve de mutant tué.

Deux coquilles exactes u13 (une mixte, une q4 pure) obtiennent le refus
`chain_shell_above_12`, sans tour ni catalogue partiel publiés. Une fixture
q4 u12 exerce l'appel public `run_tower=true` : six configurations
W1/W4 × s8/s10/s12 rendent le même digest et la fusion attendue de **trois
parents à K10**. Ces contrôles ciblés n'ont pas encore de reçu versionné.
Les trois mutants T2 (`assignment`, `open`, `adjacency`) sont désormais
inscrits à CTest avec causes exactes. La sonde refuse K hors 1..10 avant
rétrécissement et limite `--grid=` à un alphabet sûr ; les deux défauts
reproduits dans le [contre-audit B](CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md)
sont corrigés sur ce chemin. Le libellé `--grid=1mm` ne certifie toujours
pas la préparation : lier un manifeste d'entrée au pas réel.

Le juge T2 compare l'inventaire exact des boules et la tour Γ sur de petits
nuages (`n≤14`) ; depuis `e28296bb`, il compare aussi la tour publiée par
`run_tower_chain(..., run_tower=true)` en W1/W4. Il ne démontre pas
l'absence d'une BallKey complètement omise sur une grande trame.
`run_tower=false` rend aussi
`complete_relative` avec zéro ordre : tout lecteur de contrat doit exiger
`run_tower=true`, les ordres K=1..Kmax et un objet FULL cohérent.

Le [premier reçu FULL](../receipts/first_tower_20260922/README.md)
contient six exécutions locales, une par trame 08/000000, 000100, 000200
sans sol à 1 mm et par K5/K10, s8/W8. Mur K5 : **143/132/264 s** ;
K10 : **523/381/802 s**. q3/q4 prend 73–93 % et l'aval FULL 94–130 s
à K10. Les 30 hashes du manifeste et les trois entrées concordent ;
les comptes des cinq premiers ordres de K5 et K10 sont identiques pour
chaque trame. Cela certifie une base de coût **relative aux clés émises**,
pas la complétude exhaustive sur grande trame, ni l'égalité des tours par
ordre au seul vu des comptes. La [contrelecture B du
reçu](CONTRE_AUDIT_B_PREMIERE_CAMPAGNE_20260922.md) et l'[actualisation
A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md) détaillent les limites du
lecteur et les corrections du README. La chaîne ne publie que quatre
compteurs du registre q3/q4 pourtant disponible et matérialise deux
capacités complètes de présentations lors de la fusion.
Le [calcul de résidence B](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md)
identifie aussi un cache temporel optionnel de 48·nextpow2(16n) octets
(24 Gio à 30 M sites), sa remise à zéro par ordre, et au moins 216n octets
de sortie K1. Ce sont des planchers ou capacités logiques, pas un RSS
mesuré ; ils imposent une résidence de travail maîtrisée et une
représentation de sortie adaptée pour les dizaines de millions de sites.

Le chiffre v8 de 104,63 s portait sur le seul flux q3/q4 en mode digest :
aucune régression ni accélération v9 ne se déduit de cette comparaison non
appariée.

## Priorités de preuve et d'optimisation

1. **Rendre le reçu rejouable et explicatif** : lier les SHA d'entrée,
   le masque/grille, le binaire et toutes les options à chaque ligne,
   vérifier les préfixes K5/K10 par digest d'ordre, et créer le marqueur
   de campagne seulement après six succès. Les trois entrées sans sol
   sont versionnées dans le [reçu v8 `lidar_ground_20260921`](../../morsehgp3D_v8/receipts/lidar_ground_20260921/README.md).
   Publier les registres déjà calculés : tests/copies de partition d'atlas, rejets q3,
   travail et attente par worker, `merge`, census, quotient et résolveur
   FULL. Ne pas confondre les sommes de temps worker avec le temps mur.
   Pour la [session G4 SPOT préparée](CONTRE_AUDIT_B_GCP_SESSION_20260922.md),
   `partial` et code 0 peuvent accompagner **zéro** tour achevée : exiger
   les huit cas du plan par défaut complets, leurs comparaisons et l'arrêt
   ciblé certifié avant de lire un reçu comme mesure. Les quatre scripts
   sont publiés ; leur plafond de workers est corrigé à 48 dans `e28296bb`.
   Aucun essai GCP n'en découle. Le contrôleur doit encore vérifier
   indépendamment les blobs du commit annoncés par l'archive.
2. **Portes causales et entrée** : rejouer les 28 portes de `e28296bb`
   indépendamment ; comparer la restriction sémantique des ordres K1..5 de K10 à la tour K5
   sur les mêmes octets, après égalité des catalogues actifs bas-rang ;
   rendre causal le
   mutant arithmétique u18, ajouter égalités de cellules, générateur porté
   et TSan. Les reçus v8
   R2 de la reprise u18 restaient `failed`
   à cause du lecteur JUnit : la provenance v9 épingle `3f0d188f`, mais
   aucune qualification n'est héritée automatiquement.
3. **Verrou q3/q4 mesuré** : préserver les miniballes k-Gabriel locales,
   le propriétaire et les certificats exacts. L'[audit q4
   A](Q4_STRUCTURE_ET_BORNES.md) propose un test d'absence de graine aiguë
   possédée **avant** le cover ; sans graine, q3 et q4 sont vides sur l'arête.
   Une borne locale de `m(K−2)` centres q4 peu profonds pour `m` droites
   distinctes motive un catalogue de centres. Sur les `s` droites de
   graines aiguës, une sélection top/bottom de racines exactes garde au
   plus `2(K−2−p_λ)` événements par droite en `O(sKm)` après regroupement ;
   un déterminant factorisé tient en i128 sous les bornes u18. Pour `s≈m`,
   deux familles de niveaux peu profonds proposent une sélection locale
   `O(mK polylog m)` **en position générale**, avec dégénérescences,
   census et coût par arête encore ouverts. Census global et test
   `centre∈conv(coquille)` restent obligatoires ; le catalogue ne remplace
   pas automatiquement les présentations positives. La ligne v8 1 mm
   compte **171 444 arêtes q3 seules et 16,12 M census** que la réutilisation
   d'atlas q4 ne touche pas. L'[oracle q3 partagé](check_q3_shared_u18_20260922.py)
   vérifie la boîte rationnelle u18 et un débordement i128 évité par
   annulation algébrique ; le relais produit reste à qualifier. Le [contre-audit
   B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) rappelle que la suppression
   d'une cellule q4 peut aussi enlever un certificat de rejet q3. Sa
   fixture entière prouve même qu'une q4 admise peut survivre quand
   **toutes** ses faces q3 sont rejetées : les graines à parcourir ne
   peuvent pas être limitées aux q3 finalement émises. Un [certificat de
   cover par bloc d'arêtes survivantes](CONTRAT_COUTS_ET_PARALLELISATION.md)
   partage le prédicat exact sur `E×Z` et son oracle entier passe ; c'est
   une piste secondaire pour les 440 millions de visites de cover, à
   mesurer après le filtre de paire, sans lui attribuer le coût dominant
   de l'atlas.
4. **Aval FULL, grandes coquilles et échelle** : les 12,0 M appels MEB
   de 000000/K10 font 1,065 milliard de tests de puissance ; un test
   exact de la paire la plus éloignée peut éliminer toutes les autres
   paires q2 dans chaque appel, sous la preuve détaillée de l'[actualisation
   A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md). Instrumenter les tailles
   de supports et le temps avant de promettre un gain. La [note B](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
   propose un quotient local compact, [contrelu par B sur sept petites
   coquilles](CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md), tandis que l'[oracle entier
   A](check_qmin_planes_u18_20260922.py) donne des fixtures u13/u17 et les
   largeurs des prédicats. Le port doit traiter ensemble quotient,
   représentants, parents, contributions et masque de lecture ; d'ici là,
   maintenir le refus exact. Au-delà, mesurer le volume de BallKeys,
   présentations, tableaux et lectures FULL sur les régimes LiDAR, sans
   transférer les chiffres u16 historiques.

Le jalon temporel v9 commence par le **sans-sol u18/1 mm**. Le contrat
principal antérieur sur trames **brutes entières** et le profil float32
original ne sont pas effacés par ce jalon ; les trames 08/000000, 000100,
000200 viennent toutes d'une seule séquence. Aucune qualification FULL
GCP G4, GPU, sous-quadratique globale, K10 <1 s ou K5 <1 s n'est acquise.

La [synthèse A d'architecture](AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md),
les notes [q3](Q3_STRUCTURE_ET_BORNES.md), [q4](Q4_STRUCTURE_ET_BORNES.md),
[coûts/parallélisme](CONTRAT_COUTS_ET_PARALLELISATION.md) et la
[contrelecture B des cellules/catalogue](CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md)
restent les références de conception. Les objections historiques v8, les
reçus et leurs limites sont dans le [contre-audit A des
mesures](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md). Verdict public :
`not_claimed`.

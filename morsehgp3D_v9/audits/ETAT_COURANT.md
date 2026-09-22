# État courant des audits v9

22 septembre 2026. Code jugé : **`d2700314`**, premier moteur v9 ; dernier
audit publié relu : `0f3d077a`. Cadre : `exploration_v9_hors_registre`,
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

Un rejeu indépendant local de `d2700314` passe **20/20 CTests** sans saut.
Deux coquilles exactes u13 (une mixte, une q4 pure) obtiennent le refus
`chain_shell_above_12`, sans tour ni catalogue partiel publiés. Une fixture
q4 u12 exerce l'appel public `run_tower=true` : six configurations
W1/W4 × s8/s10/s12 rendent le même digest et la fusion attendue de **trois
parents à K10**. Ces contrôles ciblés n'ont pas encore de reçu versionné.
Trois mutants T2 déjà présents (`assignment`, `open`, `adjacency`) échouent
causalement en exécution directe, mais ne sont **pas inscrits à CTest** ;
les y enregistrer est un correctif court avant le premier reçu.

Le juge T2 compare l'inventaire exact des boules et la tour Γ sur de petits
nuages (`n≤14`) ; il ne démontre pas l'absence d'une BallKey complètement
omise sur une grande trame. Le chemin de test de la chaîne construit encore
FULL directement depuis son catalogue, plutôt que de juger la tour publiée
par `run_tower_chain(..., run_tower=true)` ; la fixture q4 ci-dessus donne
un témoin pour combler cette porte. `run_tower=false` rend aussi
`complete_relative` avec zéro ordre : tout lecteur de contrat doit exiger
`run_tower=true`, les ordres K=1..Kmax et un objet FULL cohérent.

L'essai annoncé sur 08/000000 **sans sol, 1 mm, 39 885 sites, K5/W8** donne
environ 131 s de mur, 834 CPU·s, 1,06 Go de RSS et 1 306 696 boules. Il est
explicitement **exploratoire, sans reçu**. Ses 117 s de q3/q4 et 11 s de FULL
indiquent où commencer, sans qualifier le contrat ni une croissance. La
chaîne ne publie que quatre compteurs du registre q3/q4 pourtant disponible
et matérialise deux capacités complètes de présentations lors de la fusion ;
voir les [mesures et la suite A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md).
Le chiffre v8 de 104,63 s portait sur le seul flux q3/q4 en mode digest :
aucune régression ni accélération v9 ne se déduit de cette comparaison non
appariée.

## Priorités de preuve et d'optimisation

1. **Premier reçu FULL honnête** : trois trames sans sol entières, K5 puis
   K10, provenance du masque/grille et hash des octets d'entrée ; sortie FULL
   vérifiée, temps CPU/mur et RSS séparés, échecs conservés. Publier les
   registres déjà calculés : tests/copies de partition d'atlas, rejets q3,
   travail et attente par worker, `merge`, census, quotient et résolveur
   FULL. Ne pas confondre les sommes de temps worker avec le temps mur.
2. **Portes causales** : inscrire les trois mutants T2 ; comparer l'appel
   public FULL à l'oracle sur une fixture q4 avec égalité et parentage ;
   ajouter portes u18 extrêmes, égalités de cellules, générateur porté,
   sanitizers et TSan. Les reçus v8 R2 de la reprise u18 restaient `failed`
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
   un déterminant factorisé tient en i128 sous les bornes u18. Census global et test
   `centre∈conv(coquille)` restent obligatoires. Ce n'est pas une borne
   sous-quadratique globale lorsque `s≈m`. Le [contre-audit
   B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) rappelle que la suppression
   d'une cellule q4 peut aussi enlever un certificat de rejet q3. Sa
   fixture entière prouve même qu'une q4 admise peut survivre quand
   **toutes** ses faces q3 sont rejetées : les graines à parcourir ne
   peuvent pas être limitées aux q3 finalement émises.
4. **Grandes coquilles et échelle** : la [note B](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
   propose un quotient local compact, tandis que l'[oracle entier
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

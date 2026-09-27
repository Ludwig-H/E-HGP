# Encodeur structurel FULL batché — 27 septembre 2026

Prototype reconstruit sur `24308be81bc91a13f8533d6adb29c62f0fb5c4b9`, sans
modifier le moteur. Cadre `exploration_v9_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `audit_flat_structure_batch_encoder`,
`public_status=not_claimed`. Aucun octet KITTI, aucun GCP.

## Résultat utile

Le contrôle chronologique des parents vivants de l'encodeur peut être
remplacé par des préfixes et des incidences regroupées par parent. Le
prototype donne les **mêmes tableaux explicites et le même premier motif
de refus** que le constructeur actuel sur les cas exécutés. Il ne faut
donc pas abandonner ce contrôle pour rendre l'écriture parallélisable.

Les [reçus](../../receipts/full_batch_encoder_20260927/README.md) ferment
14 commandes, Release et ASan/UBSan/LSan. Pour chaque binaire : 6 838
entrées, deux calendriers, soit 13 676 comparaisons ; 404 entrées acceptées
et 6 434 refusées. Trois mutations ciblées sont réfutées dans chacun des
deux builds. Les lectures normales et `python3 -O` passent.

Ce n'est **pas encore un gain de temps** : les boucles sont scalaires,
et le nouveau tri d'incidences peut coûter plus cher que l'ancien encodeur
CPU linéaire. Ni threads, ni GPU, ni verticales, ni tour géométrique entière
ne sont exécutés par ce prototype. Aucun test de croissance LiDAR n'en
découle. L'ancien prototype disparu du 26 septembre n'est pas une preuve
héritée ; tout ce qui est affirmé ici est rejoué sur les sources présentes.

## Objet et attribution

Entrée : `FullCoverageFlatDraft` existant, K, et la même banque de populations
construite par **l'overload copiant**. Aucune donnée n'est déplacée depuis
un vecteur conservant des alias mutables. La banque doit rester authentique
et immuable pendant construction et lecture ; le défaut public de l'overload
par déplacement est [un audit distinct](../b_population_alias_20260927/README.md).

Sortie : miroir public de K, banque partagée, nœuds, niveaux, parents,
successeurs et contributions datées. Les comparaisons portent sur chaque
champ, dans l'ordre, et sur les trois mots et le dénominateur des niveaux,
pas sur un condensé seul. Un refus laisse toutes les arènes vides, K=0 et
banque absente. Il n'y a pas de niveau normalisé ni de décodage différé.

`encode.hpp` ne lit aucun champ privé et n'appelle ni `build_from()` ni
`build_full_coverage_certificate()` pour décider. Les types, la banque et
le comparateur rationnel exact sont **ceux du produit**, explicitement
partagés. Le juge différentiel dans `gate.cpp` appelle le constructeur
natif à part. Ce n'est pas un oracle numérique ou géométrique indépendant.

Le draft ne contient aucune verticale. La comparaison de banque/forêt ne
qualifie donc pas A→C ni l'objet FULL complet. Le producteur de ce draft,
la géométrie, les images entre K et la complétude restent hors de ce lot.

## Pourquoi la décision est équivalente

1. Vérifier les trois CSR en entier avant la première lecture indirecte,
   puis les conditions de domaine. Une mauvaise forme prime sur un K
   incorrect ou une banque absente.
2. Marquer les actions créatrices : zéro ou au moins deux parents. Le
   préfixe de ces marques donne les IDs de nœuds ; celui de leurs nombres
   de parents donne les offsets de sortie. Une continuation ne crée pas
   de nœud. Le préfixe à l'entrée d'un lot donne son nombre de nœuds anciens.
3. Contrôler localement en-têtes, populations, masques, naissances, règles
   K1 et ordre/range des parents. Les références hors domaine ne sont
   jamais déréférencées. Les références de population sont contrôlées
   **par ordinal**, population puis masque, avant la règle de naissance.
4. Trier les incidences `(parent, lot, action, position)`. Dans chaque
   segment parent, un préfixe OU des indicateurs de fusion détermine si
   cette occurrence est postérieure à la première fusion. Deux occurrences
   au même lot invalident la seconde. Une fusion reste valide en elle-même :
   c'est le premier **réemploi après** la fusion qui est fautif.
5. Réduire les erreurs par `(lot, en-tête/action, action, étape, position,
   instruction)`, jamais par ordre d'arrivée. Si aucune erreur n'existe,
   allouer exactement les tableaux finaux et les remplir aux offsets fixés.

Pour une entrée acceptée, chaque parent est créé avant son premier usage,
utilisé au plus une fois par lot, et seulement dans des continuations
jusqu'à son éventuelle fusion terminale. L'induction sur les lots redonne
donc exactement le tableau `live` du produit. Inversement, une première
violation de l'une de ces règles est précisément le premier contrôle
parent invalide du produit. Les contrôles locaux gardent leur ordre natif ;
la réduction restitue leur première erreur commune.

Le gate exécute les contrôles d'actions **avant** les en-têtes, puis
inverse aussi le calendrier et la dispersion des sorties. Cela éprouve
l'indépendance à cet ordre d'exécution ; cela ne teste pas des courses
entre threads. Après validation, chaque nœud, slot parent et contribution
a un seul écrivain ; chaque parent a au plus une fusion, donc chaque
successeur écrit a aussi un seul écrivain. Ces propriétés permettent un
port parallèle, elles n'en constituent pas l'exécution ni un gate TSan.

Le [contre-audit](../b_full_batch_review_20260927/README.md) précise la
priorité des erreurs et les fixtures qui évitent un faux raccourci.

## Coût, coutures restantes et limites

Avec B lots, A actions, P occurrences de parents et C contributions :
travail O(B+A+C+P log(1+P)), mémoire O(B+A+P+C), sorties comprises ; pas de
reconstruction de couvertures point par point. Ce sont des bornes en taille
du **draft**, pas une borne sous-quadratique en nombre de sites LiDAR.

Le prototype conserve un tableau lot-par-action et deux préfixes ainsi
qu'un tableau d'incidences. Un futur port peut construire ces informations
directement dans A, mais ne doit pas cacher leur tri, allocation ou
transport. Le draft actuel est encore produit en amont et demeure payé.

Les allocations et leurs moments diffèrent du constructeur natif :
l'équivalence de premier refus concerne les **erreurs sémantiques lorsque
les allocations réussissent**, pas le N-ième échec mémoire injecté. Les
exceptions `bad_alloc`/`length_error` sont converties aux statuts usuels ;
aucun test d'injection d'épuisement mémoire n'a été réalisé ici.

Prochaine étape utile : alimenter ce sidecar avec des drafts réels et
mesurer validation, tri, dispersion et mémoire séparément ; ensuite un
backend parallèle explicite. Le budget FULL 100 ms exige encore le
générateur, A, les images C et l'écriture dans le même chrono. Supprimer
les seuls 39 ms historiques d'encodage n'y suffirait pas.

## Tests et reproduction

`gate.cpp` comprend 400 histoires déterministes K1..10 et seize variantes
corrompues par histoire, plus 38 cas permanents. Les fixtures couvrent
notamment deux défauts simultanés, parent d'un même lot, réemploi d'un
parent mort, continuation vide, masque/ref dans le mauvais ordre, K1,
fusion silencieuse, coquille de 16 sites, multifusion à 32 parents et
niveaux rationnellement égaux mais représentés différemment.

Les trois mutants sont `ignore-dead`, `first-visited`, `normalize-level`.
Chacun retourne 1 sur une divergence d'objet ou de motif, avec JSON de
cause explicite ; un crash ou une sortie sanitizer ne compte pas comme
réfutation. Ce sont des modes sémantiques d'un même exécutable, pas trois
binaires recompilés.

Depuis la racine de ce worktree :

```bash
python3 -B morsehgp3D_v9/audits/b_full_batch_encoder_20260927/run.py check --receipt morsehgp3D_v9/receipts/full_batch_encoder_20260927/r1
python3 -B -O morsehgp3D_v9/audits/b_full_batch_encoder_20260927/run.py check --receipt morsehgp3D_v9/receipts/full_batch_encoder_20260927/r1
```

Les reçus sont LIVE, liés aux chemins/builds et sources épinglés : le
lecteur hache ces fichiers, vérifie les commandes exactes, dépendances
compilées, sorties et mutants. Il **ne relance pas** les exécutables.
Pour une nouvelle qualification, utiliser `capture` avec une destination
et un build neufs, ne jamais écraser `r1` ni son build clos.

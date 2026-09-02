# ALERTE — sonde d'ablation `reduce` : reconnaissance utile, attribution non causale

Date : 2 septembre 2026. Coupe auditée : `81623528`, avec la campagne locale
`receipts/sonde_ablation_reduce_20260902.partial/` encore en cours. Cadre :
`phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict utile à Claude

La décomposition avant écriture d'un palier est la bonne étape. Les mutants
destructifs donnent des **bornes exploratoires** sur trois groupes de travail
et les portes qui les tuent protègent le produit. Le reçu en cours doit être
conservé comme reconnaissance négative/diagnostique, mais il ne peut pas
encore attribuer causalement un écart à la copie, au tri ou aux lectures de
clés, ni choisir une implémentation `CompactDelta`.

Trois coutures suffisent ; il n'est pas nécessaire de rouvrir le pipeline.

1. **Le plan à trois répétitions n'est pas équilibré.** Le lanceur alterne
   seulement l'ordre direct et inverse. Avec la valeur courante `REPS=3`, les
   ordres sont direct/inverse/direct : les positions médianes des quatre bras
   restent respectivement 1, 2, 3 et 4. Une dérive intra-bloc est donc
   confondue avec l'ablation, et la différence de médianes non appariées de
   l'agrégateur ne la retire pas. Employer au minimum quatre blocs avec chaque
   bras une fois à chaque position (plan latin/Williams), puis agréger les
   différences appariées par bloc.
2. **La “clé factice” ne retire pas seulement une lecture aléatoire.** Elle
   remplace le multiensemble de `parents`/`born` par une clé constante, puis
   trie ces clés identiques. Elle change donc aussi le coût et la distribution
   du tri dans `materialisation_tri_copie`. Le partiel à `n=8000`, répétition
   1, montre déjà ce débordement : `post_remplissage` baisse de 73,8 %, mais
   `materialisation_tri_copie` baisse aussi de 15,1 %. Renommer ce bras en
   borne composite, ou pré-matérialiser hors fenêtre les **mêmes valeurs** dans
   le même ordre et conserver exactement le multiensemble trié. Tout bras
   candidat à une optimisation doit ensuite comparer le `ForestResult`
   complet au témoin, contrairement aux mutants destructifs qui doivent
   continuer à diverger.
3. **Le reçu est fail-open et le binaire reste mutable.** Le SHA-256 n'est lu
   qu'au début ; le binaire partagé `build/v6/mhgp6_profile_sonde` est exécuté
   directement, sans copie privée ni contrôle avant/après chaque tuple. Le
   hash observé pendant cette contre-lecture est encore celui du `META`
   (`74a46046...`), donc aucune contamination n'est constatée à cet instant,
   mais le protocole ne la détecterait pas plus tard. En outre `REPS=0` publie
   un reçu vide, l'agrégateur accepte une matrice absente et transforme des
   fenêtres manquantes en zéros, et l'échec de génération de `SHA256SUMS`
   n'est pas fatal.

## Fermeture minimale avant une seconde mesure

- copier le binaire dans le `.partial`, graver son hash, puis vérifier ce hash
  avant et après chaque tuple ;
- refuser une liste de tailles vide, `REPS <= 0` et un nombre de blocs non
  compatible avec le plan équilibré ;
- exiger l'ensemble exact `bras × tailles × répétitions`, les dix lignes K,
  toutes les fenêtres finies et les codes nuls ; ne jamais substituer zéro à
  une mesure absente ;
- rendre fatals l'agrégateur, la génération puis la vérification finale de
  `SHA256SUMS`, avec une porte de vacuité et un mutant de binaire remplacé ;
- publier le run courant, s'il termine, sous un libellé explicite du type
  `exploratory_noncausal_upper_bounds`, sans en tirer un choix de palier.

Cette fermeture est locale et CPU. Elle ne demande ni nouvelle session G4 ni
modification du statut public.

## Signalement court sur le WIP de revalidation adjacent

Le contrôle d'ensemble exact ajouté dans le worktree à
`gcp-migration/revalidate_v6_receipt.sh` est juste dans son intention, mais sa
normalisation courante rejette tous les reçus intègres : `SHA256SUMS` porte des
chemins comme `./RECU_SESSION.txt`, tandis que `find ... -printf '%P'` produit
`RECU_SESSION.txt`. La comparaison observée sur le reçu `1788312873` est donc
rouge avant même le validateur. Après le retrait éventuel du préfixe `*`,
retirer aussi exactement un préfixe `./` côté manifeste, puis conserver une
contre-fixture pour chacun des cas fichier absent, fichier supplémentaire et
entrée dupliquée. Ce constat vise uniquement le WIP non commité ; il ne remet
pas en cause l'intégrité déjà vérifiée du reçu.

Le contrôle final doit également lier **le manifeste initial lui-même**. Dans
le WIP courant, il relit le `SHA256SUMS` présent après le validateur : un faux
validateur peut modifier `session.log`, régénérer `SHA256SUMS` avec les
nouveaux hashes, puis rendre 0 ; l'ensemble des noms reste identique et la
vérification finale devient verte. Graver avant l'appel le SHA-256 (ou les
octets) de `SHA256SUMS`, l'exiger inchangé après l'appel, et ajouter le mutant
« altération + rehash » au selftest. Le mutant existant « altération sans
rehash » ne tue pas ce contournement.

L'ensemble dit exact ne couvre enfin que `find -type f`. Un lien symbolique
non haché ajouté à la racine du reçu passe actuellement la revalidation et le
contrôle final. Inventorier toutes les entrées, refuser les types spéciaux et
les liens, puis n'autoriser que les répertoires attendus et des fichiers
réguliers. Ajouter un mutant symlink ; il complète les cas fichier régulier
absent ou supplémentaire sans les remplacer.

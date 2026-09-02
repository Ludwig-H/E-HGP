# ALERTE — sonde d'ablation `reduce` : reconnaissance utile, attribution non causale

Date : 2 septembre 2026. Coupe auditée : `81623528`, avec la campagne locale
`receipts/sonde_ablation_reduce_20260902/` désormais terminée. Cadre :
`phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict utile à Claude

La décomposition avant écriture d'un palier est la bonne étape. Les mutants
destructifs donnent des **bornes exploratoires** sur trois groupes de travail
et les portes qui les tuent protègent le produit. Le reçu final doit être
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
   du tri dans `materialisation_tri_copie`. Les médianes finales confirment ce
   débordement : `post_remplissage` baisse de 73,9 %, 73,5 % et 75,0 % selon
   la taille, tandis que `materialisation_tri_copie` baisse aussi de 15,4 %,
   14,4 % et 15,3 %. Renommer ce bras en
   borne composite, ou pré-matérialiser hors fenêtre les **mêmes valeurs** dans
   le même ordre et conserver exactement le multiensemble trié. Tout bras
   candidat à une optimisation doit ensuite comparer le `ForestResult`
   complet au témoin, contrairement aux mutants destructifs qui doivent
   continuer à diverger.
3. **Le reçu est fail-open et le binaire reste mutable.** Le SHA-256 n'est lu
   qu'au début ; le binaire partagé `build/v6/mhgp6_profile_sonde` est exécuté
   directement, sans copie privée ni contrôle avant/après chaque tuple. Le
   hash observé après publication est encore celui du `META`
   (`74a46046...`), donc aucune contamination n'est constatée sur ce run,
   mais le protocole ne l'aurait pas détectée. En outre `REPS=0` publie
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
- conserver le run terminé sous un libellé explicite du type
  `exploratory_noncausal_upper_bounds`, sans en tirer un choix de palier.

Cette fermeture est locale et CPU. Elle ne demande ni nouvelle session G4 ni
modification du statut public.

## Clôture factuelle du reçu terminé

La mécanique de cette exécution particulière est intacte : 115 fichiers,
114 entrées SHA-256 couvrant exactement les autres fichiers réguliers, 36
cellules exactes (`4 bras × 3 tailles × 3 répétitions`), tous les codes nuls,
dix lignes K et neuf fenêtres par sortie. Les copies du lanceur et de
l'agrégateur égalent bit à bit `81623528`; le résumé est reproductible bit à
bit depuis les sorties. Le hash du binaire partagé est resté identique après
publication. Ces faits reçoivent le paquet, pas son attribution causale.

Le signal utile est une priorité de falsification. La suppression destructive
de la copie borne à 53,8 %, 57,0 % et 59,2 % la part retirée de la fenêtre
`materialisation_tri_copie`; la suppression du tri la borne à 27,5 %, 25,8 %
et 25,4 %. Mais le gain apparent du premier bras sur le mur instrumenté vaut
4,8 %, -0,9 % puis 7,1 % : signe suffisant qu'ordre, charge et autres étages
dominent encore la comparaison. À 16k, la différence des médianes dit même
`+669 ms`, alors que les trois différences appariées valent `-847`, `+669` et
`-3634 ms`, de médiane `-847 ms` : l'agrégation courante peut inverser le
signal. Une représentation sans copie est donc la
première hypothèse sémantiquement valide à **falsifier**, avec différences
appariées et égalité complète du `ForestResult`, pas un palier déjà choisi.

`META.txt` grave en outre `worktree_sources_modifies=1` sans embarquer le diff
correspondant. La copie du protocole rejoint le pin et le worktree observé ne
montre qu'un changement de mode sur ce lanceur, mais le reçu seul ne prouve
pas cette explication. Cela renforce son statut exploratoire et interdit d'en
faire un benchmark de référence.

Le libellé embarqué reste
`sonde_locale_non_decisionnelle (attribution decomposee...)`. Le mot
« attribution » est trop fort au regard du plan ; le verdict extérieur de ce
rapport prime et classe le reçu `exploratory_noncausal_upper_bounds`.

## Signalement mis à jour sur le WIP de revalidation adjacent

Photographie : worktree non commité au-dessus de `38281dc7`. Trois fermetures
sont réelles : la normalisation retire maintenant exactement `./`, le hash du
manifeste initial domine le contrôle final, et liens/types spéciaux sont
refusés. Les doublons de codes session sont aussi comptés avant extraction.
Les mutants rehash, symlink, fichier intrus et codes dupliqués du selftest
courant sont verts.

Cette progression ne ferme pas encore la revalidation :

1. **Le validateur n'est pas authentifié.** Le second argument reste un chemin
   arbitraire. L'exécution avec `/dev/null` rend effectivement `0`, ne produit
   aucun résumé et affiche pourtant « recu intact ». En mode normal, exiger la
   cible attendue et graver son hash ; si l'injection d'un faux validateur est
   nécessaire aux tests, la réserver à un mode selftest explicite qui ne peut
   jamais publier un verdict de revalidation.
2. **Tout basename `SHA256SUMS` est exclu.** `find ... ! -name SHA256SUMS`
   retire aussi `out/SHA256SUMS` et `marques/SHA256SUMS` de l'inventaire. Seul
   `./SHA256SUMS` à la racine doit être exclu. Ajouter les deux contre-fixtures
   imbriquées.
3. **Les répertoires ne sont liés qu'avant l'appel.** Le contrôle final
   recompare les fichiers et les types irréguliers, pas l'ensemble exact des
   répertoires. Un validateur hostile peut laisser un répertoire vide ajouté
   ou supprimé. Graver puis revalider aussi cet ensemble.

`selftest_revalidate_v6.sh` est vert, mais ne contient aucune de ces trois
dents. Ce constat vise uniquement le WIP ; il ne remet pas en cause
l'intégrité déjà vérifiée du reçu.

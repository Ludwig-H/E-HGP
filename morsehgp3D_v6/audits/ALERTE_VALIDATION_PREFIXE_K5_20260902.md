# ALERTE — la porte de préfixe K5 accepte encore des producteurs invalides

Date : 2 septembre 2026. Coupe jugée : `d5d0bdd4`, puis même logique dans le
worktree courant. Cadre : `phase=exploration_v6_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict borné

Le reçu `e66cd978` est intact et ses observations utiles sont confirmées
indépendamment : 279 fichiers, 278 entrées dans `SHA256SUMS`, 264 fichiers
sous `out/`, 84 statuts finis à code nul, 40 paires de cardinalités K1--K5
égales et huit paires avec `digest_forest_K1..K5` égaux. Cela reçoit le fait
observé, dans la portée non décisionnelle du profil.

La nouvelle porte de `d5d0bdd4` est néanmoins insuffisante pour prouver cette
propriété sur un prochain reçu. Deux contre-fixtures producteur ont été
construites sur une copie, avec `MANIFESTE_DISTANT.txt` et `SHA256SUMS`
recalculés pour représenter un paquet cohérent émis ainsi, et non une
corruption de transport. Le revalidateur courant rend `0` et
`verifie_non_decisionnel` dans les deux cas :

1. ajout d'un second `digest_forest_K5`, conflictuel, dans le passage 2 du
   bras K10 `uniform, n=32000, --digest` ;
2. ajout de `cardinalites K=0` aux quatre jumeaux K10 du même point.

Le reçu archivé n'a pas été modifié par ces essais ; les copies temporaires
ont été supprimées.

## Causes exactes

- `dfor_k` est construit directement par dictionnaire. Un doublon est écrasé
  avant tout comptage ; la condition ne voit que l'ensemble final des clés.
- `check_pipeline_run` exige K1--Kmax et rejette seulement les K supérieurs à
  Kmax. Il ne rejette pas K0 ; l'invariance entre jumeaux laisse passer ce K0
  dès qu'un producteur l'émet partout.
- Les préfixes sont groupés seulement par `(family, n)`. Un seul représentant
  K10 fournit les cardinalités et un seul bras `--digest` fournit les digests
  par K. Le passage, le bras et les autres coordonnées planifiées ne forment
  pas une bijection courte/complète. Un K10 non représentant peut donc porter
  une ligne par K fausse sans être comparé ; l'égalité de `digest_all` ne
  remplace pas ce contrôle de chaque ligne annoncée.
- Un groupe court sans aucun K10 emprunte silencieusement `continue`. Il est
  donc accepté alors que sa propriété de préfixe est invérifiable.

## Provenance de la fixture K5

`MATRICE_OBJET_DIGESTS` a été ajouté après la campagne au fichier déjà nommé
`g4_tests_v1.env`. Le reçu épingle l'ancien canon, qui ne porte pas cet axe.
Les quatre valeurs de `d5d0bdd4` sont de bonnes régressions futures, mais ni
un oracle indépendant ni une autorité rétroactive sur `e66cd978`.

Le durcissement WIP est plus nuancé que ne l'indique la contre-lecture
concurrente : pour un profil qui se déclare canonique, la comparaison complète
déjà présente rejette bien un axe moderne omis. La lacune subsiste pour un
profil effectif non canonique : le nouveau bloc ne compare que si la valeur
effective est non vide et ne redérive ensuite que les chemins `bin_*`, pas les
deux cartes de digests. La valeur d'attente doit toujours venir du canon lié,
avec une politique explicite pour les anciens canons.

## Fermeture minimale

- parser les lignes `cardinalites` et `digest_forest_K` en listes, puis exiger
  exactement une occurrence de chaque K de 1 à Kmax et aucune autre valeur ;
- dériver du plan l'ensemble exact des paires courte/complète, incluant au
  moins famille, taille, fils, `inflight`, `join`, bras digest et passage ;
  exiger chaque jumeau K10 et comparer chaque paire, sans représentant global
  ni `continue` silencieux ;
- ajouter quatre mutants permanents : digest par K dupliqué, K0 émis par tous
  les jumeaux, K10 du passage 2 seul falsifié, et profil court sans jumeau
  complet ;
- versionner le prochain canon sous une nouvelle identité, avec les fixtures
  présentes avant l'exécution, puis les parser depuis ce canon lié plutôt que
  depuis le seul profil effectif.

Ces corrections sont locales et ne demandent aucune nouvelle session G4. En
attendant, conserver l'égalité K5 observée comme lecture factuelle du reçu,
pas comme propriété promue par le validateur.

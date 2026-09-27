# Banque FULL : le déplacement public ne garantit pas l'immuabilité

27 septembre 2026, source `ddf4776d7`. Audit de correction d'API,
`public_status=not_claimed`, sans changement moteur ni appel GCP.
Le reproducer est reconstruit après disparition du worktree temporaire du
26 septembre ; ses anciennes preuves non publiées ne sont pas revendiquées.

## Défaut reproduit

`src/tower/forest/full_coverage_certificate.hpp:95` valide un vecteur de
populations, puis le déplace dans une banque exposée comme constante.
Les pointeurs sur les éléments du vecteur et sur les points de ses coquilles
restent valides. Un appelant qui les avait conservés peut donc modifier
la banque après sa certification, sans conversion retirant `const`, sans
course et sans accès aux champs privés.

La fixture utilise le domaine `{0,1,2}`, une coquille `{0,1}` et une naissance
K2. Après construction de la forêt, le pointeur conservé change 0 en 99 :
la lecture certifiée retourne désormais `{1,99}` avec statut OK. Une
nouvelle validation des mêmes données les refuse. Le constructeur de forêt,
qui fait confiance à la banque, accepte pourtant encore cette banque modifiée.
Le contrôle par l'overload copiant les lignes reste isolé de cette mutation.

Une seconde fixture agrandit la coquille à 32 entrées par le pointeur de
ligne conservé. UBSan relève alors le décalage de 32 sur u32 dans
`all_shell`, ligne 272. Il s'agit d'un arrêt **attendu par le test du défaut**,
pas d'une réussite du moteur à cette entrée.

## Portée et correction à porter

Le seul appel interne trouvé dans `full_ball_tower.hpp:709` déplace des
populations fraîchement produites dans `finish()`. Aucun alias mutable
échappé n'y a été trouvé. Ce défaut public ne démontre donc pas une
corruption des résultats G4 publiés. Il reste ouvert dans le moteur.

La frontière publique doit copier dans un stockage privé puis certifier
ce stockage. Pour conserver le chemin sans copie du moteur, réserver
l'adoption à un constructeur interne dont le stockage n'a jamais été exposé,
avec une preuve de propriété explicite. Ne pas remplacer cela par un
`shared_ptr<const>` supplémentaire ou par une recertification globale
à chaque lecture. Un port doit tester les alias des lignes **et** des deux
vecteurs internes, puis mesurer séparément le coût de la frontière publique
et celui du constructeur privé.

## Preuve exécutable

Les [reçus](../../receipts/population_alias_20260927/README.md) conservent
l'échec initial LeakSanitizer sous ptrace et le rejeu hors sandbox.
Release et ASan/UBSan/LSan reproduisent le changement de lecture ; le test
de coquille agrandie s'arrête sur le diagnostic UBSan attendu. Dépendances
locales du compilateur, notamment `common/raw_vector.hpp`, sources et
binaires sont hachés. Aucun build épinglé n'est reconstruit par le lecteur.

```sh
python3 -B morsehgp3D_v9/audits/b_population_alias_20260927/replay.py check
python3 -O -B morsehgp3D_v9/audits/b_population_alias_20260927/replay.py check
```

Ces lecteurs LIVE exigent le worktree persistant et les binaires épinglés.

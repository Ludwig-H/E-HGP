# Préflight B du protocole G4 S1 du filtre GPU

23 septembre 2026, vers 13 h 10 UTC. Lecture **du worktree mutable** du
développeur (`gcp-migration/gpu_filter_{snapshot,worker,session,selftest}_v9.py`),
avant commit et avant session réelle. Aucun GCP lancé par B. Ne pas
transférer ces observations à un paquet futur sans relire ses hashes.

## Ce qui protège déjà l'expérience

Le paquet réel doit être reconstruit depuis des objets Git committés ;
le contrôleur refuse `--execute` sur un protocole non commité. La cible
SPOT G4 est fixe, l'état `TERMINATED` est exigé avant départ, une
génération de démarrage est certifiée et le `finally` appelle l'arrêt
**ciblé sur cette génération**. Les deux gardes GCE/invité restent en
place. Le lecteur exige identité d'entrée/options/appareil, toutes les
comparaisons de masques annoncées par le probe, les visites CPU/GPU et
la cohérence des événements ; le statut reste `not_claimed` et FULL
n'est pas exécuté. Ces garanties sont protocolaires et dépendent encore
des portes positives du vrai binaire sur G4.

## Points à fermer avant ou pendant la première session

1. **Budget/go-no-go.** Le plan par défaut enchaîne six cas (trois
   scènes × K5/K10), avec 1 500 s utiles et jusqu'à 600 s par cas.
   `S1_THRESHOLD_MS=100` est enregistré, mais aucun branchement ne
   s'arrête après le **premier** cas 08/000000/K5 si son GPU est trop
   lent, indisponible ou si les masques divergent : seul
   `probe_failed` coupe actuellement la suite. Si 100 ms est bien le seuil de
   poursuite fixé d'avance, lancer d'abord un plan à **un cas**, ou
   implémenter l'arrêt et archiver explicitement les cas sautés ; ne
   pas payer les cinq autres par automatisme après un résultat négatif.
2. **Statut d'exécution.** `GPU_executed` devient vrai seulement après
   un préflight **validé**. Si le GPU tourne mais produit un mauvais
   masque/une mauvaise identité, le reçu d'échec dit à tort
   `GPU_executed=false`. Séparer `GPU_attempted` et
   `GPU_verified_complete`, ou marquer l'essai GPU avant le jugement
   du préflight, sans faire passer l'échec pour une qualification.
3. **Exactitude du lecteur.** La réception relit les champs
   `rect_mismatches`, `pair_mismatches` et `visits_equal` **écrits par
   le même binaire** ; elle ne recalcule pas les millions de masques
   depuis les bruts. C'est convenable pour une sonde de débit si les
   tests causaux du juge (masques altérés, requêtes perdues/dupliquées,
   mutation d'offset) sont positifs ; ce n'est pas un oracle
   géométrique indépendant. Le code 0 GPU n'implique pas FULL exact.
4. **Chrono.** `gpu.total_ms` est la meilleure passe chaude de
   rectangles+scan+paires avec copies de la passe, mais hors
   préparation de l'index/front, allocations initiales, formation et
   consommation des requêtes. Publier aussi premier passage, coût CPU
   cache ON/OFF et temps mur complet du probe ; ne pas appeler
   `gpu.total_ms` « temps de tour ».
5. **Arrêt en cas de corruption de journal.** Le `finally` tente
   toujours l'arrêt ciblé en régime normal. Si la lecture des preuves
   de génération lève une erreur de format non couverte, il classe
   `shutdown_uncertified` sans appeler le stop ; les gardes VM restent
   un filet, mais l'arrêt rapide est perdu. Distinguer fichier
   simplement illisible d'une **génération contradictoire** : dans le
   second cas, ne jamais forcer un stop non ciblé.

Le fichier de selftest GPU, absent au début de cette lecture, a été
ajouté au WIP entre-temps. Il doit encore être commité à l'identique
avec le protocole et ses sources, puis ses portes normal/`-O` et le
snapshot strict doivent réussir avant la session SPOT.

## Relecture du WIP corrigé, vers 13 h 20 UTC

Le worker marque désormais `GPU_attempted` **avant** le préflight et
réserve `GPU_executed` au préflight validé : le point 2 est corrigé.
Après un premier cas non complet, les suivants sont explicitement
`skipped_s1_gate`, et le lecteur vérifie cette règle : le volet
« indisponible/divergent » du point 1 est corrigé. En revanche, un
premier cas **exact mais plus lent que 100 ms** laisse encore dérouler
les cinq autres ; le seuil reste une métadonnée et non un go/no-go
de coût. C'est un choix exploratoire annoncé dans le code, pas un
succès de la porte 100 ms. La dépense maximale prévue reste
1 500 s utiles avec 600 s par cas ; commencer par un cas demeure
la voie la moins chère si seul le verdict de débit 08/000000/K5 est
recherché.

Le selftest mutable passe **8/8** en Python normal et **8/8** sous
`-O` lors de cette relecture. Son faux `cmake` copie une fausse sonde :
il teste le cycle de vie et les refus du protocole, **pas** la
compilation CUDA, la géométrie ni le débit G4. Le lecteur accepte
encore une sortie structurellement impossible avec
`rectangle_survivors=0` et `pairs>0` si les autres comptes sont ajustés ;
les champs de comparaison restent fournis par la même sonde, sans
oracle externe. Ces réserves ne changent pas le statut exploratoire.
Les quatre scripts de session sont encore non committés à cette
lecture ; le garde de snapshot doit donc refuser `--execute`.

## Couture source/protocole encore bloquante avant G4

La mutation causale `--inject=pair_mask` a été ajoutée **dans le bench
mutable** après le commit CUDA `0d5ad2e89`. Le protocole mutable lance
désormais un préflight GPU normal puis le mutant et exige exactement
une paire divergente ; c'est un bon test du comparateur **si le binaire
construit contient réellement cette option**. Or le snapshot prend les
sources C++ dans le commit demandé et les quatre scripts GPU du
worktree seulement avec `allow_uncommitted_protocol`. `validate_sources`
contrôle schéma et `--cpu-only`, pas la présence du mutant. Le selftest
hors ligne remplace le vrai bench par une fausse sonde qui possède déjà
l'option : ses 8/8 ne détectent pas ce décalage. Si seuls les scripts
étaient committés, la session SPOT construirait l'ancien bench, puis
échouerait au `preflight_mutant` (code 2) **après** démarrage G4.
Reproduction locale sans GCP : `collect("0d5ad2e89",
allow_uncommitted_protocol=True)` rend
`protocol_source=worktree_uncommitted`, le blob du bench ne contient pas
`--inject=pair_mask`, **et `validate_files` l'accepte**. Un faux probe
cohérent avec `pairs=400` mais `rectangle_survivors=0` est de même
accepté `complete` par `validate_probe` : ce lecteur n'est pas un
oracle structurel autonome des masses.

Porte préalable : committer **ensemble** bench muté et protocole,
reconstruire le snapshot du HEAD final, vérifier que le vrai bench
compilé contient bien l'option, puis garder le préflight
GPU réel comme juge ultime. La comparaison causale est meilleure que
la seule lecture des compteurs auto-déclarés, mais ne certifie pas
encore FULL ni une géométrie indépendante.

**Correctif WIP observé ensuite (vers 13 h 37 UTC).** Le développeur
a ajouté à `validate_sources` les tokens de l'option
`--inject=pair_mask` et du champ JSON `inject`. Sur le même snapshot
mixte `0d5ad2e89` + scripts mutables, `validate_files` refuse maintenant
`GPU probe schema/CLI differs from the S1 protocol` (code 1 local,
aucun GCP). Le mélange de cette fixture est donc fermé **dans le WIP**,
mais il reste à committer bench, protocole et porte ensemble, puis à
vérifier le paquet immuable ; la recherche de tokens ne remplace pas
le mutant causal exécuté sur G4.
Le selftest WIP étendu passe désormais **10/10** en Python normal et
**10/10** sous `-O` lors de la reprise locale ; il reste un test de
protocole à fausse sonde, non une compilation CUDA positive.

**Clôture de cette note WIP :** bench v2 et protocole ont depuis été
publiés dans `1c9c1e5d7` et `7565451fc`. Lire l'[audit du paquet
immuable](CONTRE_AUDIT_B_G4_S1_PUBLIE_20260923.md) pour le statut
courant ; les défauts ci-dessus datent des lectures successives du
worktree et ne doivent pas être réattribués au snapshot publié.

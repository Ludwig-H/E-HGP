# Mutations compilées de la fenêtre exacte30

## Reprise après correction du test worker

Autorité finale : [compiled_89foraim](compiled_89foraim/COMPLETION.json),
close PASS après la [correction de la gate de reçus q2](../preflight/WORKER_DIGEST_FIX.md).
Même helper et mêmes binaires de référence, nouvelles copies temporaires,
nouveau répertoire de capture et pins des189 sources corrigées. Baseline
6206 contrôles PASS ; les trois mutations sont de nouveau tuées uniquement
par la comparaison géométrique ci-dessous, sur dix commandes conservées.
Les quatre lectures normal/`-O`, historiques/live, passent :
[MUTANTS_REPRISE_READBACK.json](MUTANTS_REPRISE_READBACK.json).
Manifeste SHA256 :
`dc650d6939210380255253269b086d23b3684217c28e9eed70c7971f0d26b3a4`.

La capture initiale ci-dessous reste intacte et historique sur l'ancien
pin du test ; ses lectures live enregistrées étaient valides avant la
correction mais ne qualifient pas le nouvel instantané. Aucun changement
du moteur ou de la gate géométrique q4 n'a été nécessaire.

## Capture initiale conservée

Capture [compiled_jhdownuv](compiled_jhdownuv/COMPLETION.json) close PASS :
la gate originale passe ses6206 contrôles, puis trois objets produit mutés
sont compilés et liés séparément avant l'archive inchangée. Dix commandes
sont conservées avec sorties brutes, codes et hashes. Les trois mutations
sont tuées avec code1 par la même comparaison géométrique indépendante :
`window sweep differs from complete rational ball/depth/support/shell oracle`.
Ce sont trois fautes causales jugées par **une** gate, pas trois oracles indépendants.

| Mutation | Erreur ciblée |
|---|---|
| `point_window_dropped` | Rejeter L=U comme si la fenêtre fermée était vide. |
| `fixed_interiors_dropped` | Oublier les sites strictement intérieurs pendant toute la fenêtre. |
| `constant_shell_dropped` | Oublier les sites de la coquille commune à toute la famille. |

La première fixture causale est la coquille30 avec son centre ajouté, K4.
Une sphère positive propriétaire a profondeur1 et coquille30 ; retirer
la contribution fixe publie une profondeur0, ouvrir le point-fenêtre perd
la sphère et retirer la coquille constante perd des IDs. La comparaison
de boules/profondeurs/supports/coquilles précède les contrôles de compteurs.
Une erreur de compilation, un crash ou un simple échec de ledger ne tue
pas un mutant au sens du helper.

Les189 sources, la gate, son objet, l'archive, le cache, le helper et le
compilateur sont épinglés avant/après. Les copies originales/modifiées,
patches exacts et hashes des objets/binaires temporaires sont conservés.
Les quatre lectures normal/`-O`, historiques/avec `--check-live`, passent :
voir [MUTANTS_READBACK.json](MUTANTS_READBACK.json). La lecture historique
n'exige pas la survie future des objets temporaires ; la lecture live en
contrôle aussi les hashes. Aucun produit, build épinglé ou helper n'a été
modifié pour ces lectures.

Helper final SHA256 :
`d618f15dd4e1d7549c571236132ece9daa2720699330b73a01493a6303db3986`.
Manifeste :
`7bf52d2719e438ded873582a1914f1eb8c1303d74ffc8ab6950f21ec408f290f`.

L'échec initial [compiled_e4r17kzj/LAUNCH_FAILURE.json](compiled_e4r17kzj/LAUNCH_FAILURE.json)
est conservé séparément : chemin d'objet de gate erroné, arrêt avant
manifeste, aucun mutant compilé/exécuté. Son reçu indique explicitement
qu'il a été enregistré après observation de l'échec ; le
[helper initial](../preflight/run_mutants_before_object_path_fix.py) reste
archivé. Cet échec de lancement n'est ni effacé ni une qualification.

Ces preuves ciblées ne qualifient ni une tour FULL, ni une borne globale,
ni les contrats G4. GCP non utilisé.

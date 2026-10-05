# S5 — cohérence de la provenance publiée et du lecteur

`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.

Snapshot local non commis du développeur, HEAD de contexte
`ee8a69f1aae7cd68bb08f28a5eb5a9f5fd0656a9` ; les sources figées et leurs
SHA-256 sont dans `source_manifest.json`. Zéro compilation, exécution native ou
appel cloud. Cette preuve ne qualifie pas une révision Git S5.

L'API peut déclarer une publication `published_complete` avec un manifeste
refusé par son lecteur officiel. Dans `src/api/manifest.cpp:224`, `publish`
valide la provenance avant I/O, mais `check_provenance` (ligne 282) vérifie
seulement les déclarations décimales. Les tailles déclarées sont copiées sans
contrôle par `inputs` (ligne 110), le budget par `parameters` (ligne 90), puis
le manifeste reçoit `status=complete` (ligne 300) et est commis (ligne 243).

Le cas `Provenance{}` n'est pas hypothétique : `tests/api/publish_test.cpp:149`
exige ce succès, son état et l'empreinte du manifeste. Il déclare pourtant des
tailles d'entrée nulles, alors que le produit est non vide. Le lecteur
`bench/mhgp11_formats.py:141` exige exactement 4 octets d'IDs par point, et
sa ligne 131 impose le rapport 12/4. Son contrôle du budget déclaré est à la
ligne 87. Le contrat public décrit la provenance comme les tailles et
empreintes des deux fichiers d'entrée (`src/api/api.hpp:162` et
`docs/SORTIES.md:417`).

Reproduction depuis ce dossier :

```sh
python3 -B -S reproduce.py
python3 -B -O -S reproduce.py
```

Le script importe le lecteur figé sans le modifier et construit une fixture
Python réelle MHGP11FUL1 d'un site, ordre 1, 290 octets. Le lecteur de dossier
accepte son binaire et son manifeste de provenance valide (tailles 12 et 4,
empreintes des entrées synthétiques réelles). Sur ce même binaire, il refuse :

- les tailles 0/0 : `manifeste : points et octets d'entree` ;
- le rapport 11/4 : `manifeste : 12 et 4 octets par point` ;
- le rapport correct 24/8 pour un seul point : `manifeste : points et octets d'entree` ;
- le budget déclaré nul : `manifeste : budget_bytes`.

Les cinq manifestes exacts sont conservés dans `fixture_*.json`. Les réponses
normal et `-O` sont identiques. La fixture n'est pas une sortie de `publish`
exécuté nativement : la chaîne native est établie par ses sources et ses
portes existantes ; le refus du lecteur est exécuté ici. La signature d'arbre
de la fixture est une empreinte syntaxique, que ce lecteur ne recalcule pas.

La CLI renseigne les tailles et empreintes depuis `io::InputFiles`
(`cli/mhgp11.cpp:243` dans le worktree consulté) et rejette `--budget=0` : ce
constat concerne l'API directe, pas ses entrées CLI correctement préparées.

Correction proposée : contrôler avant I/O les tailles de provenance contre
le poids du nuage (12 et 4 octets par point) et un budget déclaré strictement
positif. Si la publication depuis une entrée mémoire doit permettre une
provenance absente, définir cette absence dans le schéma au lieu de publier
des tailles zéro comme des faits. Les empreintes déclarées ne sont pas
vérifiables à partir de `Provenance` seule.

Porte nécessaire : publier une provenance valide et faire relire le dossier
par le lecteur officiel, puis muter les tailles et le budget et exiger un
refus avant création de `D.pending`. Les portes actuelles avec `{}` doivent
utiliser une provenance valide ou le schéma d'absence explicitement choisi.


## Corrections précédentes relues

La capture ferme aussi les constats de source sur l'identité de Session et
le hook variadique : voir `session_corrections.json`, avec les empreintes et
la portée exacte. L'identité repose sur le budget alloué sur le tas ; le
refus précède les fichiers et les diagnostics. La porte garde le produit
vivant pendant le déplacement de Session. La destruction contrôle le budget
et les deux injections IO décodent les cinq arguments correctement à W1.
Aucune porte native de ces corrections n'a été exécutée par cet audit.

Le cœur mathématique S3 reste identique aux captures déjà relues : aucun
nouveau constat FULL, aucune réexécution de ses anciennes preuves. S6a est
commitée localement en `ee8a69f1a`, distincte d'une qualification G4.
Les six notes actives sont maintenues en place ; ce reçu conserve la preuve.

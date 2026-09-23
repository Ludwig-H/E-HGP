# Contrelecture B — juges q2/q3 de C, source v4

23 septembre 2026, lecture statique du commit `e2fd68662`. Aucun reçu
final v4 n'était encore publié à cette lecture ; la vérification
`verification_juge_q3.json` porte sur le SHA `c1befd6c…` d'une source
antérieure non publiée, **pas** sur la source v4 `0521f82e…`.

Deux corrections demandées pour v3 sont bien présentes : les cibles et
planchers de rang haut exigent maintenant les clés q3 régulières
`q_min=3`, `n_shell=3`, `p=K−2` (et q2 régulières pour le juge q2), et
le recoupement exige l'**égalité des listes triées d'IDs de coquille**.
Le mutant de doublon est prévu dans la recette. Les lemmes de
profondeur de Tukey et de marge de boîte u18 restent valables sous
leurs hypothèses déjà documentées.

Trois limites de portée/protocole demeurent avant d'exploiter un code 0 :

1. Le match q2/q3 ne reconstruit ni ne compare `ball.key` à une clé
   canonique issue de la sphère indépendante. Une clé corrompue avec
   niveau, coquille, intérieurs et arité inchangés passe. Le juge
   contrôle des présences **géométriques** échantillonnées, pas toute
   l'identité de catalogue consommée par FULL. Tuer un mutant de clé
   seule si cette revendication est souhaitée.
2. `run_judges_v4.sh` hache plus d'objets, dont le script et les
   bibliothèques, mais son bloc de provenance ignore toujours les
   codes d'échec de `git` et `sha256sum` (`set -u -o pipefail` sans
   `set -e` ni contrôle explicite). Les cas peuvent rendre `STATUS=0`
   avec une provenance incomplète. Chaque commande doit être bloquante ;
   le `git log -1 -- morsehgp3D_v9/src` épingle un historique, pas le
   lien démontré entre les archives `$BUILD` et ces sources.
3. Les sites du contrôle des longues ancres sont extraits des lignes
   `LONG_SITE` du **parcours déjà élagué**. Si toutes les incidences
   longues d'un site sont omises par un sur-élagage, il ne peut pas
   entrer dans ce contrôle. Sélectionner des sites longs depuis les
   coordonnées d'entrée, sans passer par le résultat du juge, et y
   tuer un mutant de sur-élagage.

Ces limites ne sont pas des omissions q3 observées du générateur.
Un futur reçu v4 peut conforter les sites tirés, sans prouver la
complétude globale ou la tour (`run_tower=false`).

## Suite v5 publiée par `c6042af2b`, relecture statique

La réserve 1 est traitée **dans le code** : les deux juges
reconstruisent une clé canonique indépendante et la comparent à
`ball.key`, avec mutants de clé seule attendus en code 1. Le runner
v5 relie le build au répertoire source, reconstruit les bibliothèques
et rend **certaines** commandes de provenance bloquantes. Le bloc
`sha256sum ... || exit 1` est correct, mais `echo
"commit=$(git ... )" || exit 1` ne vérifie que le succès de `echo` :
un échec de `git` dans la substitution écrit `commit=` et passe. De
même, `[ -z "$(git ... status --porcelain ...)" ] || die` accepte
un échec de `git` à sortie vide comme un arbre propre. La réserve 2
n'est donc que **partiellement** fermée ; affecter séparément la sortie
et vérifier le code de retour avant de l'écrire ou de tester sa
vacuité. Reproduction minimale : `bash -c 'echo "commit=$(false)" ||
exit 7; echo status=$?'` affiche `commit=` puis `status=0`.
Aucun reçu v5
(`STATUS`, `PROVENANCE`, sorties) n'est cependant publié dans ce commit.
L'ancienne vérification q3 porte sur un autre SHA de source et ne peut
qualifier le juge v5 par héritage.

Autre faiblesse de la recette : dans `run()`, l'échec d'ouverture de
`$O/$name.txt` produit `c=1` **avant** le lancement du juge. Pour un
mutant `expected=1`, ce code est accepté comme mutant tué ; l'append
`exit=...` et l'écriture finale de `STATUS` ne sont pas contrôlés.
Une fixture Bash reproduisant cette fonction avec une redirection
impossible finit en code 0 sans sortie ni `STATUS`. Le runner devrait
exiger un dossier de sortie neuf, distinguer échec de redirection et
code du juge, vérifier chaque écriture, et vérifier un marqueur causal
dans la sortie du mutant plutôt que son seul code 1. Les cas `obs`
doivent au moins distinguer observation valide d'échec d'infrastructure.

## Suite v6 `abf3c3827`, code distinct du v5 historique

Le **nouveau** `run_judges_v6_gates.sh` corrige en code les faux succès
ciblés ci-dessus : sorties `git` affectées/vérifiées séparément et non
vides, fichier stdout neuf pré-ouvert, ligne de synthèse et marqueur
causal attendus, écriture `STATUS` contrôlée. Les sites isolés sont
choisis depuis les coordonnées, avec cas sain `--min-long=50`, puis
mutant `--inject=drop-long` attendu en code 1 avec
`PRUNE_DISAGREES`, sur s00 et s02. L'ancien script v5 reste inchangé.

Il n'y a **aucun reçu v6** dans ce commit : portes codées, pas encore
démontrées exécutées. Une synthèse manquante rend `STATUS=1` plutôt
que le code d'infrastructure 2 annoncé, mais le chemin est fermé au
succès ; c'est une classification de panne à clarifier, pas un faux
accord. Même après un reçu positif, l'échantillon et l'absence de
`run_tower` borneront la conclusion.

Pour la réserve 3, `--long-sites=4` choisit maintenant les sites depuis
les **seules coordonnées**, indépendamment du parcours élagué : le
biais de sélection indiqué plus haut est corrigé en code. Mais les
deux mutants `overprune_isolated` restent marqués `obs` dans
`run_judges_v5.sh`, sans code 1 exigé ni plancher positif d'incidences
longues. Ils ne ferment donc pas expérimentalement la porte « un site
isolé perd toutes ses incidences longues ». Un reçu v5 causal sur
ces sites et leurs incidences est encore requis. Il s'agit d'une
limite du juge échantillonné, pas d'une omission HGP démontrée.

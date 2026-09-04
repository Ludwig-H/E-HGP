# Proposition de première session G4 v7

Statut : contrôleur et selftests locaux du 4 septembre 2026 ; aucune mutation
GCP par leur auteur. L'exécution réelle éventuelle exige son propre reçu.
phase=exploration_v7_hors_registre ; backend=cpu_reference, puis qualification
device explicitement distincte ; profile=quantized_u16_input_only ;
mode=audit_independant_math_and_architecture ; public_status=not_claimed.

## Cible et fenêtre

Cible préférée : devpod-gpu-exploration / europe-west4-a /
ehgp-blackwell-spot. La lecture du 4 septembre la voit TERMINATED,
g4-standard-48, SPOT, action STOP et maxRunDuration=3600 s. La relecture
avant lancement est obligatoire ; cet inventaire ne vaut pas réservation.
Le repli déjà présent europe-west4-ai1a / ehgp-blackwell-spot-ai1a était
également TERMINATED, avec 18 000 s ; toute session courte doit d'abord
réduire et recertifier cette durée par set_max_run_duration_and_verify.sh.

Une seule VM, durée GCE 3600 s, arrêt invité 45 minutes, clé OS Login
expirant après 70 minutes. L'inégalité de la garde est satisfaite :
2700 + 300 + 120 + 480 = 3600. Fenêtre du worker : 2100 s au maximum
après certification des deux gardes ; arrêt volontaire immédiat après
transfert du résultat utile. Le tarif SPOT courant n'a pas été vérifié :
aucun prix historique n'est utilisé pour chiffrer la session.

## Snapshot source explicitement v7

Ne pas appeler session_campagne_v6_g4.sh pour publier cette session :
son archive, ses profils et son validateur sont intrinsèquement v6.

Avant tout démarrage, figer une copie immuable du worktree v7 effectivement
testé : src, cli, oracle, tests, cmake, CMakeLists.txt et fixtures nécessaires.
Inclure dans le même paquet le worker v7 et les copies exactes des scripts
start_and_verify.sh, stop_and_verify.sh et, si utilisé,
set_max_run_duration_and_verify.sh. Conserver le HEAD Git, l'état sale
complet et le snapshot V6_SOURCE_SNAPSHOT.json comme provenance du port ;
le HEAD seul ne représente pas les fichiers v7 non suivis.

Le manifeste identifie chaque chemin, taille, mode normalisé et SHA-256, ainsi
que le schéma et la portée v6/v7. Le contrôleur, son wrapper et les gardes
sont eux aussi inclus. Les scripts exécutables sont 0555, les autres sources
0444 : la garde start peut ainsi appeler directement sa garde stop voisine.
Hacher le manifeste et le paquet.
Le worker vérifie tous les fichiers avant de configurer CMake ; aucun
exécutable du worktree vivant ne remplace ensuite un fichier de la copie.
Les identités source, binaire, compilateur et architecture figurent dans les
sorties et le reçu. Les reçus v6 restent des entrées différentielles nommées.

La préparation locale, dont les nouveaux tests et la lecture d'un éventuel
audit externe apparu dans audits/, précède cette copie. Une objection
technique non résolue doit être portée au statut, pas cachée dans un benchmark.

## Séquence proposée

Le contrôleur conserve son registre et son handoff dans un répertoire privé
persistant hors build. Il possède un trap de fermeture dès avant le start,
emploie exclusivement start_and_verify.sh --yes avec handoff et registre
durables, vérifie le double coupe-circuit, puis lance le worker sous timeout.
Le worker relit le shutdown poweroff futur avant calcul et refuse toute
absence de garde. Aucun reboot ni changement de pilote dans cette session.

Le worker fait successivement :

1. Inventaire machine, CPU, mémoire, compilateurs et source, plafond 30 s.
2. Construction CPU Release C++20 des deux snapshots v6 et v7, plafond 420 s.
3. Quatre passages CPU K1..K10 à 50 000 points : v6 et v7 pour chacune des
   familles uniform et terrain, seed=3, threads=48, fold_inflight=2,
   layout=csr, digest activé ; plafond 120 s chacun. Chronométrage externe
   et pic RSS du processus. Les dix cardinalités et digests chaînés doivent
   coïncider ; les compteurs de travail intermédiaire peuvent différer.
4. Diagnostic candidat `--complete-incidences` uniform 50k, grille par
   défaut, budget mémoire proxy 16 GiB et supports MEB 1 milliard, plafond
   120 s. Un refus mathématique nommé est conservé, jamais reclassé succès.
5. Si au moins 300 s restent, diagnostic candidat uniform 8k, coord=65536,
   mêmes budgets, plafond 240 s. Succès moteur, refus mathématique/capacité
   identifié et censure sont trois statuts distincts, tous hors qualification.
6. S'il reste au moins 780 s et que CUDA est présent : inventaire GPU (30 s),
   construction de deux cibles seulement (420 s), puis portes GPU C2/C4
   (300 s, `--no-tests=error`). CUDA utilise MHGP7_ENABLE_CUDA=ON,
   CMAKE_CUDA_ARCHITECTURES=120 et le chemin nvcc explicitement découvert,
   y compris /usr/local/cuda/bin hors PATH. Sinon, absence d'outils ou budget
   insuffisant est enregistré ; il ne s'agit jamais d'un succès GPU.

La somme des plafonds utiles vaut 2040 s ; la fenêtre globale de 2100 s et
l'échéance invitée moins 180 s restent prioritaires. Des étapes facultatives
peuvent donc être omises explicitement pour préserver la collecte et l'arrêt.
Ces plafonds sont une borne, pas une promesse de complétion. Un dépassement
rend failed ou censored et conserve les logs ; il n'autorise pas à prolonger
la VM. Cette session ne déploie pas de pipeline GPU industriel ; elle qualifie
seulement les primitives device C2/C4. C6a reste un stub CPU de protocole.

Chaque run conserve argv exact, exit code, statut terminal, sortie brute,
identité générateur (famille, taille, coord, seed, source épinglée), digests
de l'objet et des dix forêts, mémoire et temps. Aucun hash littéral du tableau
d'entrée n'est émis par les CLI actuelles : le protocole n'en prétend pas un.
Les SHA-256 des exécutables construits sont conservés séparément. Le
contrôleur rapatrie le paquet complet, vérifie son manifeste puis appelle
stop_and_verify.sh --yes avec le même triplet et le lastStartTimestamp
obtenu au démarrage. La certification TERMINATED et le reçu sont requis
avant passage de relais. Une génération illisible ou non certifiée demeure
un blocage, jamais une présomption d'arrêt.

L'arrêt et la lecture post-arrêt s'exécutent avant toute écriture de leur
journal : un disque plein ne peut les empêcher. Le contrôleur mémorise la
génération ; si le start échoue plus tôt, il accepte les états de garde
`start_may_have_been_requested` et `targeted_stop_failed` seulement avec
génération valide et identité exacte. Les enfants locaux sont drainés au
TERM puis au KILL même si leur leader est sorti ; le start reçoit une grâce
de 420 s pour sa propre fermeture gardée. Une seconde interruption ne
coupe pas le funnel d'arrêt. SIGKILL/hôte perdu restent couverts par les
coupe-circuits, mais ne dispensent pas d'une certification ultérieure.

Points d'entrée et instructions : `gcp-migration/README_v7.md`. Les tests
locaux `selftest_session_v7.py` utilisent des opérations cloud remplacées,
et un vrai stop gardé sous faux gcloud pour le contre-test du trap start.

## Contrats de grande échelle

Le succès à 50k ne valide ni les millions de points, ni la complétude
mathématique, ni un statut industriel. La v6 documente un mur mémoire
K10 entre 400k complets et 800k en échec sur une G4 ; v7 doit le remesurer.
Les incidences silencieuses restent une portée mathématique à qualifier.

Une seconde campagne d'échelle, distincte, ne se planifie qu'après lecture
des pics v7, avec plusieurs tailles, mémoire virtuelle bornée, distinction
K5/K10, refus typés et formats d'identifiants/digests déclarés. Les dizaines
de millions restent un contrat cible : ni un extrapolé, ni une substitution
K1/K5, ni un simple refus de mémoire ne vaut sa livraison.

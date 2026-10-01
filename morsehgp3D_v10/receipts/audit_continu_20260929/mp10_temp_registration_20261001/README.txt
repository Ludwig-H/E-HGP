MP10 : clarification native autonome, pas requalification de la campagne historique.

Capture reelle UTC : 2026-10-01T00:52:02.548856 a 00:52:05.905166.
GNU g++13.3, C++20/O2, warnings stricts ; builds 1,73s et1,61s, zero diagnostic.
Quatre appels exacts, chacun <3ms, source produit inchangee avant/apres.

Les quatre en-tetes complets sous baseline/core/ sont des snapshots du raccord
R2 HEAD4b457bdbb94bcad95557e8e4d18fb558bb6277e3. Seul cli_output.hpp est
mute sous mutant/core/ : remplacement exact de MP10, deux instructions au lieu
du deplacement noexcept. Aucune bibliotheque HGP/moteur, aucun GCP.

Le wrapper --wrap=open arme un seul operator new apres la creation reussie
O_CREAT|O_EXCL d'un temporaire .mhgp10-*. Avant la creation, aucune injection.
L'observation a lieu apres destruction de OutputSet, puis desarmement explicite
avant toute inspection pouvant allouer. Le FILE de destination est preexistant.
Sans injection, les deux variantes reservent correctement et se nettoient.
Baseline armee : reserve retourne none, injection non consommee, FDdelta0 et
tempdelta0. Mutant arme : reserve retourne memory_budget, injection consommee,
FDdelta1 et tempdelta1. La destination sentinelle reste intacte partout.
Seuls le FD et le chemin observes par le wrapper sont nettoyes explicitement
apres ces mesures : post-cleanup0/0. Aucune ressource externe n'est supprimee.

Ce temoin prouve une vraie fuite causale de MP10 au premier new post-creation,
sans dependre d'un compteur d'etapes ni du plancher1000 du juge historique.
Il n'identifie PAS la cause du code3 historique : sa sortie brute est absente.
Les codes0 des quatre appels signifient que les observations attendues ont ete
mesurees, PAS que le mutant est conforme au contrat de production.

Archive texte uniquement : 12 fichiers (10 manifestes+manifeste+ancre).
receipt.json conserve argv, UTC, exits, stdout/stderr complets et hashes des
deux binaires experimentaux. Les binaires restent hors de cette archive dans
le runtime prive nomme au recu. Compilateur, libc et libstdc++ sont des
dependances systeme externes ; aucune qualification sanitizer ou GPU.

Lecture fermee READ-ONLY, jamais de relance native implicite :
python3 -B read.py CHEMIN_ARCHIVE SHA_EXTERNE_MANIFESTE
python3 -B -O read.py CHEMIN_ARCHIVE SHA_EXTERNE_MANIFESTE
Le lecteur verifie d'abord l'ancre externe, l'inventaire exact et tous les
hashes, puis seulement parse les JSON et juge les quatre relations causales.
capture.py est un recorder OPEN : il refuse un paquet ferme. Ne jamais le
relancer ici. Toute contre-compilation doit utiliser une nouvelle copie et un
nouveau runtime prives, a partir des sources et commandes archivees.

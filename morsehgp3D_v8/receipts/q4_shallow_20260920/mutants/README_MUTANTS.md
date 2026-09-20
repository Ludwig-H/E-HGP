# Mutations compilées du filtre par couches convexes

Capture [compiled_xmj_ihtj](compiled_xmj_ihtj/MANIFEST.json) close après le gel des184 sources : **gate originale3686 contrôles PASS, trois mutants tués, dix commandes PASS**. Les sources, artefacts de référence, helper et compilateur sont inchangés avant/après. Les quatre lecteurs historiques/vivants, normal/`-O`, passent ; leurs commandes et sorties sont dans [MUTANTS_READBACK.json](MUTANTS_READBACK.json).

Le helper adapte explicitement celui de la tranche28, sans modifier ce dernier. Il compile chaque altération dans un répertoire temporaire neuf, puis place cet objet devant l'archive originale au lien avec l'objet de la gate `q4_shallow`. Le produit, la gate et le build de référence restent inchangés. Le premier appel doit faire passer la gate non modifiée ; sinon aucune mutation n'est compilée.

Trois mutations causales, jugées par une même gate indépendante (pas trois oracles indépendants) :

- Remplacer `<0` par `<=0` dans **les deux** scans de l'enveloppe, ce qui retire les points colinéaires de frontière.
- Construire T−1 couches au lieu de T : remplacer uniquement la condition de boucle `kmax_-2` par `kmax_-3`.
- Omettre l'ajout des IDs dans la branche c=0, sans supprimer son compteur.

Un mutant n'est déclaré tué que s'il compile, se lie, puis sort avec code1 et un message exact de désaccord géométrique : ensemble retenu différent de l'oracle rationnel, profondeur/coquille incorrecte ou certificat strict invalidé. Les trois mutations ont ici échoué sur **l'ensemble retenu différent de l'oracle rationnel des droites d'appui**, avant toute vérification des compteurs ou planchers. Une erreur de compteur, de non-vacuité, de compilation ou un crash ne suffisent pas. Un survivant ou toute autre erreur conserverait sa tentative en échec ; aucun n'est survenu dans cette capture.

Le reçu épingle184 sources, gate/objet gate/archive/cache, helper et compilateur, avant/après ; il conserve environnement, invocation, commandes, sorties brutes, snapshots et patchs exacts, ainsi que hashes des objets et binaires temporaires. Le lecteur historique ne nécessite pas ces artefacts temporaires encore présents ; `--check-live` vérifie en plus tous les fichiers vivants. Les deux lecteurs seront exercés en Python normal et `-O`. Aucun build ni test lourd n'est lancé par la préparation du protocole.

GCP non utilisé ; aucun contrat FULL/G4 ou gain de complexité global n'en découle.

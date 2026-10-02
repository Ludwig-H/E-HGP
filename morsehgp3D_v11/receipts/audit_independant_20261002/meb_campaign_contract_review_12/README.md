# Revue du protocole MEB — 2 octobre 2026

Cette capsule examine le producteur de preuve et ses juges, sans exécuter de
produit natif, de build ni de commande GCP. Elle ne qualifie aucune campagne.
Les sources LIVE ont été copiées avant lecture ; les 39 fichiers suivis de
la capture sont exactement les blobs **ab04bc7b1ef61b9996eb7ec15db4fd4e5f2d511a**.
Le lecteur MEB était encore non suivi. Les observations successives sont
conservées : [avant](sources_before.json), [raccord Git](git_correspondence.json),
[après](sources_after.json), [delta WIP](live_delta.json), [clôture](closure_observation.json).
La date initiale d'inventaire précède la copie des lecteurs ; le champ HEAD
du fichier avant a été actualisé lors de cette extension de capture.

**Défaut du producteur de banc ab04, réparation publiée ensuite.** À la ligne
37 de [meb_probe.cpp figé](sources/morsehgp3D_v11/bench/meb_probe.cpp), la boucle
parcourt `sphere.anchor().coordinates()`. Dans [geometry.hpp](api_support_git/morsehgp3D_v11/src/num/geometry.hpp),
`anchor()` rend un `Point` par valeur (ligne 49) et `coordinates()` rend une
référence au tableau de ce Point (ligne 19). En C++20, ce Point temporaire
est détruit avant le parcours : cette sérialisation lit une référence pendante.
Ce constat concerne le banc, pas le calcul de MEB ni ses prédicats numériques.
Le [correctif capturé](live_after_copies/morsehgp3D_v11/bench/meb_probe.cpp)
conserve le Point localement ; il est exactement le blob publié dans
**25792084eb4e672c5222d62f5b2ae87bd2ee4948**, parent ab04.
SHA du sérialiseur corrigé : `d57c8a8ea7880831a35f6cf4c415c0f549bc8af1ddf4f20f391726bd2c008774`.
Une future qualification de ce correctif doit épingler cette source ; une
capture ab04 ne qualifie pas le sérialiseur corrigé. Aucun rejeu natif ici.

Le [plan](sources/morsehgp3D_v11/bench/plans/meb_g4.json) prévoit matrice,
complément ASan18 `num;index;tower`, puis banc : budgets 600/180/700 secondes.
Le banc exige les hashes réels des exécutables et provenances, les profils
18/21/24, le complément ASan18, six entrées entières à poids de site unitaires,
et 18 processus limités à 30 secondes. Les refus, délais et lancements échoués
restent dans le calendrier ; une différence entre deux survivants reste une
différence même si le troisième profil échoue. Aucun résultat FULL/GPU.

Le coût est à lire avec sa portée exacte. Pour chaque entrée, 48 parties de
tailles 1 à 12 interrogent la vraie `bounded_meb`, puis un census global ;
`meb_census` répète ces deux calculs (lignes 69–80 du producteur). Les compteurs
JSON décrivent le premier MEB et le premier census, pas tout le processus :
9 464 présentations MEB par première série, 18 928 avec la répétition du wrapper.
La première population reste vivante pendant le wrapper ; son pic contient
deux populations. Lecture, Cloud, index, MEB, census, wrapper, référence et
décodage Python ont des durées séparées. Le temps du processus paie aussi
référence et sérialisation. Le scan de référence partage `num::side` ; son
indépendance concerne le parcours. L'oracle Fraction est une porte distincte.
Les 48 parties choisies ne constituent pas une descente ni toute la tour.

[Commandes et sorties](logs/commands.json) : normal/−O du collecteur figé
passent — 11 états de processus, 27 corruptions, 20 provenances, six calendriers,
zéro natif. [Notre contrôle](review.py) passe normal/−O : trois profils factices,
neuf altérations rejetées par les deux juges, 528 contrôles d'ensembles sélectionnés.
Chaque mutation tower a un remplacement unique et une porte déclarée ; ceci
ne prouve pas sa mise à mort. L'échec initial de notre sonde (mauvais nom de
fonction Python) est [conservé](logs/review_initial_attribute_error.txt), corrigé
sans modifier le produit. Le lecteur WIP supplémentaire, copié avant essai,
passe son autocontrôle pur normal/−O : 20 positifs, 79 corruptions, zéro natif.
Il exige notamment les cibles MEB dans les provenances et des morts par juge,
avec contrôle du LastTest complet, jamais par signal/délai/construction.

À 19:00 UTC, `meb1/receipt.json` et une archive venaient d'apparaître chez le
développeur, sans copie locale `meb.json` dans l'inventaire observé. Seuls leurs
noms et hashes sont enregistrés dans l'observation de clôture ; la campagne
n'est pas recoupée ici. Le pin `meb1→ab04` du lecteur testé est distinct du
correctif 257. Les données, paquet, flags, binaires et verdicts natifs de cette
nouvelle campagne restent à examiner dans une autre capsule.

[LEDGER.json](LEDGER.json) inventorie tous les payloads sauf ces deux seuls
fichiers racine : lui-même et [SHA256SUMS](SHA256SUMS). SHA256SUMS inventorie
tous les fichiers sauf lui-même, y compris les inventaires éventuels imbriqués.

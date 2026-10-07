# Audit de la session B et du parcours M5

7 octobre 2026, pin `e30000dec1027c5f0ade3093a94563ee412409d3`.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference` pour l'audit,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Aucun appel GCP/GPU, gros build, sanitizer ou nouvelle campagne LiDAR par cet audit.

**Le juge M5 ne protège pas encore son adoption. Les portes géométriques ciblées
passent. Les observations de vitesse M3/M4 de la session B sont retrouvées, avec
des preuves d'adoption encore incomplètes.**

## Session B : chiffres positifs, qualification à compléter

Le [recalcul indépendant](campagne_b/README.md) retrouve les six cas, les 24 processus
M3 et les 24 processus M4, les 79 fichiers du manifeste et les résultats bruts locaux.
La résolution M3 à K10 diminue de 45,3 à 45,9 %, mais elle n'a qu'une prise par cas
(`CST-0213`). Les répétitions du microbanc de plus petite boule ne remplacent pas celles
de la résolution complète.

M4 tient les seuils chronométriques par ordre à K5. À K10, la contraction parallèle
vaut 3,14 à 4,09 ms et dépasse le seuil de 3 ms, comme annoncé. La convention est la
médiane entre processus des **minima de cinq passes**, pas la médiane des passes.
Les preuves T6 ne sont pas vides : 14 829 064 naissances contrôlées sur les six cas
comptés une fois. Le défaut générique `CST-0214` reste ouvert. Le maximum des coûts
isolés par ordre est une projection du futur mur parallèle, pas un mur mesuré.

Les cinq exécutables M3/M4 ne sont pas hachés dans les reçus ; la bibliothèque v11
seule l'est. Sources et commandes raccordées n'effacent pas le veto « binaire non
haché » du plan antérieur. `CST-0021` reste donc ouvert pour les deux mesures,
y compris pour l'adoption définitive M4 K5. Aucune falsification n'est alléguée.
Le reçu de fermeture B est cohérent : même génération, une tentative d'arrêt code 0,
état enregistré TERMINATED et clés supprimées. Ce n'est pas une observation actuelle
de la VM.

## M5 : séparer géométrie, lecteur et verdict

Les [portes mathématiques](mathematiques/README.md) contrôlent les prédicats et le
réservoir en repère parent, puis des parcours contre un DFS exact indépendant et
la conservation des boules de l'oracle borné. Elles exercent la boîte fermée à
33 bits, les profondeurs 63/96 et le refus transactionnel `wide_leaf`. Elles ne
rejouent pas les collectives CUDA sur un GPU. Les portes de port produit restent ouvertes.

Le nouveau README §9 répond à la distinction d'ordre de `CST-0113` : comparer
les parcours à ordre parent identique et le catalogue lorsque Morton change.
Le témoin à six sites est désormais rejoué nativement avec un seuil de feuille
valide : deux fronts exacts mais différents. Le lecteur canonique FULL reste à faire.

Le [vrai pilote M5 sous injections externes](juge_format/README.md) rend néanmoins
« adopté » avec une grille incomplète, malgré des codes d'identité en échec, avec
une seule passe v11 ou avec une médiane GPU incompatible avec ses durées brutes.
Ces contre-exemples prolongent **CST-0018**. Corriger les codes, cardinalités,
paramètres et statistiques effectivement consommés avant tout verdict d'adoption.
Les auto-tests du juge ne couvrent pas encore ces chemins de son pilote.

Deux constats nouveaux, à portée bornée :

| ID | Défaut et témoin | Correction |
| --- | --- | --- |
| CST-0222 | [Émission du nombre de tâches](capacite/README.md) : addition u32 débordante près de 2^32 ; un petit enregistrement émet zéro au lieu de 16 777 216. Aucune grande entrée exécutée. | Élargir avant addition et partager le calcul avec celui du scan. |
| CST-0223 | [Lecteur MHGP12TR](juge_format/README.md) : quatre références sémantiquement invalides à FNV valide sont admises, dont des tests G1 impossibles ou une somme débordante. | Contrôler les bornes des comptes et les règles de racine/feuille ; ne pas réduire le contrat à la taille et au checksum. |

Le comparateur et plusieurs refus du lecteur passent leurs contrôles négatifs.
L'admission d'une référence invalide n'établit pas son adoption par l'identité native.
Le futur budget du front et l'admission avant réservation/conversion restent à
porter (`CST-0211`) ; aucun gain de temps GPU M5 n'est acquis à ce pin.

## Preuves et reprise

Les quatre volets contiennent commandes légères, sources au pin, résultats et
limites. Les comparaisons normal/`-O` utilisent des exceptions explicites ; les
hashes sont vérifiés à la fermeture. La session B se rejoue aussi en mode
`--published-only`, qui déclare l'absence éventuelle des archives brutes privées.
Aucun octet de jeu sous licence ni compte personnel n'est ajouté au dépôt.

`verification.json` clôt les contrôles d'intégration et `SHA256SUMS` couvre ce reçu.
Le registre courant reçoit deux nouveaux constats ; les compléments prolongent
les lignes existantes sans clôture prématurée. Suite : durcir le juge M5, fermer
la provenance binaire B et répliquer la résolution M3, puis juger M5 sur G4 avant
le port T1. Aucun catalogue produit, FULL/100 ms ni régime multi-séquences qualifié.

Le durcissement M2/M3/M4 `320db4a12`, déposé pendant cette tranche, reste à contre-lire.
Les fichiers M5 `scripts/g4_traversal_bench.py`, `bfs.hpp` et `format.hpp` y sont
identiques au pin audité ; CMake et les dépendances M2 évoluent. Aucune clôture des
corrections nouvelles n'est anticipée. Les scripts de ce reçu exigent les sources
épinglées ; après évolution, les rejouer dans un worktree distinct au pin d'audit.

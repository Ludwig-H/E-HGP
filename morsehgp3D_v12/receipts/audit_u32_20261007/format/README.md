# Format T1 et contrôle de taille MHGP12DP

Pin `c3de9d73d8999f2f1e31a0f592b829efc6e7a4da`. Témoin Release local,
un seul en-tête produit, aucune allocation géante ni lecture de payload invalide.
[Sonde](reader.cpp), [pilote](check.py), [capture](normal.json) ; mêmes résultats
et empreinte binaire en normal et `-O`. Sources et témoins hachés avant/après,
dépendances locales de la compilation vérifiées par `-MMD`.

**CST-0225 : addition non contrôlée dans le lecteur de sections.** Le vrai
`mhgp12::dump::Reader` de `microbancs/mes_m3_m4_tour/common/format.hpp`
contrôle le produit `elem_bytes * count`, mais compare ensuite `at + bytes`
à la taille du fichier en `u64` sans contrôler l'addition (`:210–220`).

Un fichier de **88 octets** comporte un en-tête valide de 64 octets et un
en-tête de section `POPVAL` de 24 octets, de taille élémentaire 4 et de
compte **2^62−1**. Aucun élément n'est présent. Le produit vaut 2^64−4 : il
ne déborde pas. L'addition à l'offset 88 donne 84 modulo 2^64 ; le contrôle
de taille passe, puis l'alignement ajoute 4, ramenant l'offset final à 88.
Le constructeur accepte et `get<u32>("POPVAL")` annonce 2^62−1 éléments.

Les témoins positifs (section vide, un élément) sont admis ; la troncature
ordinaire et un produit réellement débordant sont refusés. L'attaque n'est
donc pas un simple contournement de l'en-tête. La sonde ne déréférence jamais
la plage annoncée : aucune corruption mémoire, erreur géométrique, adoption
M3/M4 ou défaillance sur données réelles n'est affirmée ici. Le défaut porte
sur l'admission de la vue, qui ne garantit pas que ses données sont dans
le fichier. Gravité majeure avant réutilisation pour le lecteur de T1.

Correction attendue : vérifier `at <= size` puis `bytes <= size - at`,
contrôler également le remplissage d'alignement et toutes les avancées
d'offset avant de créer une vue. Graver ce fichier minuscule en refus.

**Contrat de transition (`CST-0113`).** Le commit `f4a11f49e` ne livre que
18 lignes de spécification ; `reference/transition_catalogue.py` n'existe
pas au pin. Le format catalogue contient les coordonnées, les supports,
les rangs et les populations : il permet de recalculer centres et niveaux
exacts depuis les supports. Il n'écrit cependant **aucun `PointId`** :
`vidage_v11.cpp:494–522` écrit `SITEXYZ`, `BALLS`, `POPOFF`, `POPVAL`,
`NLEVELS`, alors que le nouveau § 8 bis annonce des coordonnées et PointId.
Préciser l'extension ou corriger cette phrase ; les positions suffisent au
différentiel géométrique, pas à vérifier une correspondance d'identifiants
qui n'est pas exportée. Le changement de départage de S* et les sélections
de Kruskal/cover restent à juger par le futur lecteur. Aucune clôture de
`CST-0113` sur la seule spécification.

Rejeu : `python3 -B check.py`, puis `python3 -B -O check.py`. Les fichiers
synthétiques et le binaire sont créés hors du dépôt et supprimés après le
test. Le code zéro de la **sonde** signifie observation complète ; son champ
`response` distingue l'admission et le refus du lecteur.

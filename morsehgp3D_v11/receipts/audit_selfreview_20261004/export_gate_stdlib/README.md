# Passage de la porte d’export à la bibliothèque standard

Pin Git eb036dbe2101788c5ca51047dba954c883fc2b6d. Cinq blobs copiés avant le modèle et rehachés après, inchangés. Aucun produit, exporteur C++, fit, build ou GCP exécuté.

Le changement est cohérent :

- points_export.cpp:324–331 écrit la magie, six mots d’en-tête, norders mots d’ordres et quatre mots par site ; read_levels:25 utilise exactement l’offset 8+8*(6+norders+4*n).
- points_export.cpp:272–278 écrit num puis den, mots bas vers mots hauts ; whole_input.hpp:58–61 écrit chaque mot en petit-boutiste. Le format1 emploie trois mots et le format2 quatre, comme le décodeur struct.
- Les sources C++ et points_hierarchy sont inchangées dans ce delta. La porte n’importe plus NumPy ni points_hierarchy ; le helper est standard seulement.

check_decoder.py extrait et exécute uniquement read_levels par AST. 48 buffers synthétiques couvrent u18/u21 version 1 et u24 version 2, quatre nombres de sites et quatre tailles de liste d’ordres ; niveaux nuls, plusieurs niveaux, mots hauts et le niveau brut du tétraèdre. Les valeurs aux limites de capacité du format sont artificielles, pas des Clouds revendiqués. Les 48 troncatures d’un dernier limb, l’en-tête incomplet, la mauvaise magie et une version inconnue refusent. Le niveau u24 réel est bien 196/148 bits et excède trois mots au numérateur.

**297 contrôles**, sorties identiques en python3 -S et python3 -O -S. Ce modèle vérifie le décodeur et la suppression de dépendance, pas l’export C++ ni la campagne G4. Le helper lit seulement l’en-tête et les niveaux ; il ne remplace pas une validation complète des forêts et incidences. Aucune erreur matérielle d’offset, endian ou limbs relevée.

Rejeu : python3 -S check_decoder.py ; python3 -O -S check_decoder.py. La première campagne en échec demeure historique ; aucune qualification native du correctif eb036 n’est transférée depuis elle.

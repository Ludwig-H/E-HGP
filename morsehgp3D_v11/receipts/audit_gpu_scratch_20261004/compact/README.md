# Format compact des feuilles — contrelecture source22

Pin exact `22a6af6aa6c57302b3eac5adcb7f2b52c0f1e0b4`, port compact
`4ec33e3d7`. Neuf blobs Git complets copiés avant leur lecture, puis recoupés
après ; BEFORE distingue la capture initiale produit et la capture des tests.
Aucun produit, audit actif ou ancienne capsule modifié ; aucun C++/CUDA,
build, G4 ou fit exécuté. Mémoire scratch CUDA hors périmètre.

**Aucun défaut de format ou de raccord établi sur les entrées certifiées.**

- `sources/leaf_batch.hpp:23–55` : quatre supports et p/m/qmin/pad sont huit
  octets ; rangs locaux0..31, sentinel0xFF. `local_rank` transforme les SiteIdx
  du support/I/U par la même liste de feuille. Les SiteIdx globaux supérieurs
  à255, dont2^32−2, sont restaurés sans réduction. Ce ne sont pas les PointId
  externes : leur correspondance reste celle du Cloud propriétaire.
- `leaf_device.hpp:239–272` : I/U disjoints, dans l'ordre de la liste certifiée,
  p+|U|≤m_feuille≤32. Les contacts sont conservés ; S* est déterminé avant
  encodage et S*==generated garde qmin égal à l'arité de la présentation émise.
- `single_pass_batch.cpp:42–92` : décodage par `job.begin`, vérification des
  rangs, padding horsqmin, préfixes bornés et épuisement exact de chaque plage
  population. Les mêmes supports ordonnés et la même arité passent aux mêmes
  fabriques Sphere : le format ne change donc ni centre, propriétaire, ni
  formule de Level brut. La vérification Fraction contrôle l'égalité des
  boules ; l'égalité de l'encodage brut repose sur ce raccord source, pas sur
  une réduction rationnelle inventée.
- `leaf_batch.hpp:75–95`, `leaf_batch.cpp:44–67,112–135` : le scratch compte
  toutes les émissions ; franchir128records ou1024incidences invalide toute
  copie partielle et déclenche le rejeu complet. Aux seuils exacts, la copie
  est admise. Nonrésolue ⇒ ses comptes/emissions sont jetés et la feuille est
  refaite intégralement en CPU. Les préfixes restent par ordinalj ; rangs
  identiques de feuilles différentes utilisent bien leurs propres listes.

**3 404 gardes stdlib/Fraction**, normal/−O identiques :826variantes de
champs admissibles, trois boules exactes (coquille32 de rayon²27, triangle
équilatéral3D aigu de rayon²8/3, tétraèdre strict de rayon²3), rang31,
sentinelles, SiteIdx très larges, I/U, trois feuilles avec gap nonrésolu et
ordre d'exécution inverse, neuf refus de métadonnées malformées, frontières
128/129 et1024/1025. Les flux synthétiques de scratch servent aux branches ;
ils ne sont pas une sortie native de catalogue. Aucun gain ni qualification
CUDA/G4 transféré depuis ces modèles.

**Une limite causale du test, sans panne réelle établie.**
`full_leaf_lanes.py:84–85` exige0<fill_jobs<jobs pour annoncer les deux
chemins d'écriture. Deux feuilles, une émettrice débordée et une vide, satisfont
cette assertion sans copie nonvide. Aucune prise de cette fixture3000 au
pin22/4ec n'a été retrouvée dans les pièces publiées relues ; les commentaires
historiques ne sont pas une preuve transférable. L'égalité dump/ledger reste
une porte utile. Conseil minimal : exporter `stored_nonempty` ou
`records_copied` et exiger>0 avec fill_jobs>0. Ce constat ne démontre pas que
la fixture réelle manque la branche. Avec une capture complète, une borne
records>41416×fill_jobs suffirait aussi à prouver qu'il reste une émission
copiée (ΣC(32,q), q2..4), sans lancement supplémentaire.

```sh
python3 -B -S check.py
python3 -O -B -S check.py
python3 -B -S verify.py
python3 -O -B -S verify.py
```

AFTER conserve le pin et compare séparément les sources d'origin ; aucune
mutation ultérieure n'hérite de ce verdict. SHA256SUMS couvre tous les fichiers
réguliers, excepté ce manifeste racine.

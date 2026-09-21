# Capsules des constructions Patchwork++

Ces deux capsules conservent les preuves des constructions closes, sans
copier leurs binaires, objets ni en-têtes système :

- [release](release/MANIFEST.json) : GCC, neuf commandes ;
- [sanitize](sanitize/MANIFEST.json) : Clang18, ASan/UBSan/LSan, neuf commandes.

Chaque capsule contient les manifestes et clôtures originaux, les neuf
commandes avec leurs sorties, six listes de dépendances compilées, trois
sources locales et les cinq fichiers de calcul/en-têtes amont accompagnés
de leur [licence BSD-2-Clause](release/third_party/LICENSE).
Les sources amont correspondent au commit
3e6903a1d5537a4cc2ace897b0bbb98a92d6014c du
[dépôt officiel Patchwork++](https://github.com/url-kaist/patchwork-plusplus/tree/3e6903a1d5537a4cc2ace897b0bbb98a92d6014c).
Leurs contenus et chemins ne sont pas réécrits.

BUILD_AUTHORITY.json conserve le résultat du lecteur LIVE sur le build
d'origine, contrôlé avant puis après l'export. Les hashes de tous les
fichiers copiés et la fermeture de l'export se trouvent dans
[BUILD_CAPSULES_EXPORT.json](../BUILD_CAPSULES_EXPORT.json). Le
[script d'export](../export_build_capsules.py) ne lance aucun programme
natif et refuse de remplacer un export existant.

Ce n'est pas une qualification autonome ou portable. Les chemins absolus
des reçus restent ceux des builds d'origine :
build/v8_ground_patchwork_20260921 et
build/v8_ground_patchwork_sanitize_20260921. Une relecture LIVE dépend
encore de ces répertoires, de leurs binaires, du compilateur, d'Eigen et
des autres dépendances système hachées, ainsi que des sources locales
gelées. Le lecteur du builder ne doit pas être appliqué directement aux
capsules comme s'il s'agissait de builds complets.

Les [douze préflights](../preflight/README.md) restent séparés des
campagnes d'intégration officielles. Ni la réussite d'une compilation ni
cet export ne certifient la qualité sémantique du retrait du sol, un
contrat de temps, FULL ou un traitement GPU.

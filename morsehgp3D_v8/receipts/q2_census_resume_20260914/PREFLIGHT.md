# Préflight, avant gel des preuves

La première configuration Release a échoué faute de chemin Boost dans cette
session. CMake indiquait `Could NOT find Boost (missing: Boost_INCLUDE_DIR)`.
Reprise avec le répertoire d'en-têtes déjà utilisé par les builds précédents :
`build/v7_boost_gate/extracted/usr/include`, en lecture seule. Ce sont des
en-têtes du juge indépendant, pas du code ni une qualification moteur v7.

Les deux premières compilations Release/Clang ont commencé pendant la fin
d'édition de la continuation et ont trouvé le constructeur par défaut privé
de `Q2PreparedBounds` dans `Frame`. Diagnostics conservés ici :

```text
GCC: error: ‘constexpr mhgp8::Q2PreparedBounds::Q2PreparedBounds()’ is private within this context
Clang: error: field of type 'Q2PreparedBounds' has private default constructor
```

La correction utilise deux helpers du moteur ami existant pour l'état
inerte et les bornes sur nœud global. Aucun ancien corps de parcours n'est
modifié. Aucun CTest n'avait démarré lors de ces échecs de compilation.
Ces préflights ne sont pas présentés comme des captures qualifiées. Les
reçus fermés qui suivent épinglent les sources et binaires après correction.

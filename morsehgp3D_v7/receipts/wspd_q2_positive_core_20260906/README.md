# Cœur q2 positif : supplément permanent — 6 septembre 2026

Le gate `tests/wspd_terminal_reuse_gate.cpp` ajoute une exigence locale dans
la fixture existante scène 1, s=8, masque q2 seul, threshold=1 (h₂=10) :
le rectangle ordonné (-1,-3) est trouvé exactement une fois et son cœur q2
vaut 1. Le compteur explicite `q2_positive_core_checks` doit valoir 1.

O2 et ASan/UBSan terminent chacun avec 174 appels, six refus intentionnels
et un contrôle positif ; leurs stdout sont identiques. `--unknown` rend 2
sans sortie dans les deux modes. LeakSanitizer est activé dans l'environnement
capturé ; ce supplément ne prétend pas requalifier un CTest global.

Le mutant physique privé enlève seulement `ff.c[0]=fc.c[0]` dans sa copie
de generate.hpp. Il compile, puis le gate rend 1 avec la première cause exacte
`wspd q2 front rejected: line.q2_positive_core_value`. Son stdout partiel est
conservé tel quel. Aucun switch mutant n'est ajouté au produit.

Neuf commandes fermées sont conservées, dont trois compilations et l'identité
du compilateur. Les 27 dépendances projet sont copiées pour chaque bras ; les
depfiles et hashes avant/après les relient aux compilations. Le fichier nominal
generate.hpp reste inchangé (`345129a7…`). Trois ELF sont omis avec leurs
hashes dans `excluded_binaries.json` ; leurs originaux privés sont conservés.

Ce supplément concerne le nouveau gate `35d28f2c…`. Les premiers CTests et
anciens reçus restent attachés au gate antérieur `81a8657a…` : ils ne sont pas
réétiquetés comme qualification de ce delta. C'est un renforcement de test,
pas la correction d'un défaut nominal constaté, ni une mesure de performance.

`public_status=not_claimed`, CPU/u16. Aucune complétude géométrique ni aucun
contrat de temps nouvellement certifié. Aucune VM GCP utilisée.
Vérification : `sha256sum -c SHA256SUMS` depuis ce dossier.

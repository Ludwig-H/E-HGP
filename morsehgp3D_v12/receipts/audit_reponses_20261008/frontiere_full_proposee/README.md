# Frontière FULL G4 proposée — aucun banc exécuté

Lecture préparatoire du 8 octobre 2026, sans nouveau constat ni qualification de temps.
Contrats au pin `77693bb05` ; prototype repo6 (base `a5e0dbc77`, patch
`b3c78ae365a99568457bc6cd5d894a1b73d02b7b19ae3d774f9fdfa094a58bab`).
Les cinq sources et lignes sont épinglées dans `pins.json`. La
[preuve repo6 u21](../../audit_tmv_repo6_u21_20261008/README.md) reste une preuve
locale de correction, distincte d'une campagne FULL G4.

## Ce que la chaîne actuelle mesure

`tests/tower/tower_chain.cpp:97–144` exécute une seule chaîne CPU : lecture
u32le et préparation/index, catalogue CPU, G, T/M/V/R, validation, empreinte
FUL1, puis écriture facultative. Ses cinq durées sont celles de l'index
(lecture comprise), du catalogue, de G, de la forêt et de l'empreinte.
`forest_build.cpp:194–197` confirme que **la forêt inclut verticales et registre**.
G se termine avant T ; il n'y a pas de recouvrement entre ces deux appels.

Ce banc de correction n'annonce ni mur FULL englobant ni session chaude.
Le Pool est créé avant les durées (`:99`), le raccord aux `ForestInput` est
entre deux chronos (`:122–128`), la validation externe est séparée (`:134`)
et l'écriture n'est pas chronométrée (`:142`). Le budget est illimité (`:98`).
Additionner les étages ne produit donc pas une mesure de latence intégrée.
Validation externe, empreinte, export et destruction après disponibilité de
la tour peuvent rester hors du contrat en mémoire si la frontière l'annonce ;
leur exclusion n'est pas en soi un défaut de ce banc.

## Frontière à annoncer avant la première campagne

Entrée proposée : **trame entière déjà quantifiée, profil u21, masque sans sol
figé et identifiants conservés**. Pour ng00–ng02, grille 1 mm et provenance
Patchwork++ sont décrites dans [MESURE](../../../docs/MESURE.md) ligne 24 et
[DONNEES](../../../docs/DONNEES.md) lignes 295–311. La segmentation, la
quantification et la lecture des fichiers préparés restent des coûts séparés
et annoncés ; publier séparément le coût de segmentation et le total si
l'usage vise une chaîne depuis le LiDAR brut. Ne pas qualifier ce dernier
avec le seul chrono HGP sur masque figé.

Une fois la Session ouverte, le mur d'un futur appel par trame commence
**avant Cloud/tri Morton/index**, sur les buffers d'entrée quantifiée en
mémoire. Il couvre **C GPU**, transferts et finition CPU compris, **G**, le
raccord et **T/M/V/R**, jusqu'à disponibilité de la tour complète et de ses
verticales, opérations nécessaires achevées. Allocations, agrandissements et
recyclage nécessaires à cet appel sont inclus. Pool, contexte, flux, modules
et capacités effectivement conservés peuvent être ouverts une seule fois
hors du chaud, conformément à [ARCHITECTURE](../../../docs/ARCHITECTURE.md)
lignes 30–34. La première passe et les agrandissements réellement rencontrés
restent publiés, sans préchauffage caché.

Publier à côté un froid processus neuf, ouverture comprise. Pour le chaud,
préannoncer les trames successives, au moins cinq processus et dix passes,
première passe séparée, conformément à `MESURE.md:12–18,137–152` ; reconstruire
le catalogue de chaque trame, sans mesurer seulement G sur un catalogue figé.
Validation externe, digest et export sont mesurés séparément et la correction
est contrôlée sans déplacer leur coût dans le mur contractuel par accident.
Un bras CPU est identifié comme tel. Ni une somme des sessions I/J, ni les
durées locales de repo6 ne remplacent cette mesure intégrée.

Aucun produit modifié, aucune compilation, aucun test ou appel cloud exécuté
pour cette lecture. Les octets des cinq sources ont été relus stables.

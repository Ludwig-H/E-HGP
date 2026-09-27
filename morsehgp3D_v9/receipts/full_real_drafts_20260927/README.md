# Vrais drafts FULL — quatre mesures locales closes

27 septembre 2026, base `4badf8b7d`. Voir la
[méthode et ses limites](../../audits/b_full_real_drafts_20260927/README.md).
Moteur inchangé ; capture CPU instrumentée, GPU/GCP non utilisés.

Autorité courante : `r3/`. Six commandes de qualification Release /
Clang ASan/UBSan/LSan, puis quatre processus de mesure Release, tous clos.
Les cinq lecteurs passent en modes normal et `-O`, sans réexécution.

Les deux premiers builds ont échoué dans le lecteur d'entrée de la sonde :
`qualification/` (indexation Point3 non assignable) puis
`r2/qualification/` (conversion signée non explicitée). Sources et sorties
initiales conservées, aucun essai géométrique dans ces captures FAILED.

| capture | sites | actions/nœuds K1..5 | contributions | draft copié, octets | sortie, octets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `r3/ng00` sans sol 08/000000 entier | 39 885 | 1 541 750 | 897 776 | 127 383 080 | 195 162 040 |
| `r3/uniform_8000` synthétique | 8 000 | 629 404 | 372 698 | 55 862 248 | 80 168 120 |
| `r3/uniform_16000` synthétique | 16 000 | 1 301 794 | 770 002 | 115 533 920 | 165 743 640 |
| `r3/uniform_32000` synthétique | 32 000 | 2 660 312 | 1 574 290 | 236 008 096 | 338 768 120 |

Tous les champs sont égaux aux réencodages natifs et aux forêts publiées,
trois répétitions par ordre ; vingt ordres réels, soixante paires de
réencodage au total. Aucune continuation dans ces quatre entrées ; leur
gestion générale reste éprouvée par la qualification de l'encodeur sur
histoires artificielles, pas par ces grands cas.

Le résultat de performance est **négatif sur CPU scalaire**. Sommes des
médianes par ordre natif/prototype : LiDAR 149,38/296,30 ms ; uniforme
8k 52,14/92,41, 16k 130,01/241,30, 32k 287,51/554,28 ms. Il ne s'agit pas
de murs parallèles de tour et les trois répétitions montrent une forte
variabilité. Les temps incluent allocation/écriture, excluent comparaison
et destruction des résultats. Pas de promotion aveugle vers le moteur.

Les chaînes instrumentées sont intégralement payées et publiées :
LiDAR 28,632 s ; uniforme 6,215 / 13,362 / 28,151 s. La copie des drafts
perturbe leur fin ; ne pas soustraire des sommes de temps K au mur.
Ces mesures ne sont pas un contrat G4, une nouvelle mesure du retrait du
sol ni une étude multi-séquence/s10/s12/K10.

La croissance mesurée des actions uniformes fait ×2,068 puis ×2,044 ;
ne pas étendre ce diagnostic à la chaîne générale ni au LiDAR entier.

Builds r3 épinglés, ne pas reconstruire :
`/workspaces/E-HGP/build/v9-audit-full-real-drafts-20260927-r3/{release,sanitize}`.
Les reçus LIVE dépendent encore de ces exécutables et des entrées originales
hors de la v9. L'inventaire complet des sources, commandes, builds et hashes
se trouve dans chaque `capture.json` ; les données KITTI ne sont pas copiées.

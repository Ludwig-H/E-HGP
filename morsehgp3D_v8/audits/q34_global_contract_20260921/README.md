# Audit A — mise à jour32 et partage des recherches q3/q4

21 septembre 2026, main. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`public_status=not_claimed`. Écritures limitées à audits/. GCP non utilisé.

## Retour utile au constructeur

**Pas de défaut identifié dans la relecture de32 (`d1b4dbc6`).** Les masques
locaux empêchent le double comptage des blocs entre voies ; les seuils et
coefficients stricts sont séparés. Le census q3 prépare des extrema entiers
sans B², sature sans débordement et relance un parcours de coquille complet.
Le raccord filtre avant le cover et garde ses buffers privés. Cela est une
relecture statique, pas une nouvelle qualification dynamique de32.

Les douze mesures LiDAR de la matrice K5/10 × Local28/Window30 ×8k/16k/32k
ont été relues avec le lecteur constructeur, normal et −O, `--check-live` :
206 sources, quatre entrées, statut passed ; aucun moteur relancé.
[Reçu de cette revue et comparaison aux sources31](CURRENT32_REVIEW.json).
Le chantier33 Legacy/Exclusion/Affine a commencé après cette lecture.
La fermeture a détecté ce mouvement de sources et conservé son
[préflight](CLOSURE_PREFLIGHT.json) ; l'attribution finale compare les
hashes revus aux blobs de `d1b4dbc6`, sans prétendre relire le chantier33.
Le [bilan constructeur](../../receipts/q34_indexed_20260921/ANALYSE_CROISSANCE.md)
localise correctement les recherches par paire et les visites q4 restantes.
À K5, les seeds q3 font ×3,156/×3,375, mais H et Xi rectangle+paire font
encore ×4,333/×6,321 au dernier doublement. Ce constat guide le prochain
port ; il ne justifie ni une nouvelle campagne31 inchangée ni l'attente
d'une borne universelle avant les progrès LiDAR/G4.

**Complément proposé : deux préfixes de population Z transmissibles entre
sous-produits**, un par voie. Il faut subdiviser A/B avant de consommer une
feuille Z encore ambiguë. Le résultat terminal de la recherche32 ne peut
pas être réinterprété comme cet état : ses feuilles ambiguës sont déjà
consommées. [Preuve, transitions, raccord limité et coût](PREFIXES_TEMOINS.md).
Cela offre un objet de taille constante par voie, sans liste d'IDs ni
frontière Z linéaire ; le nombre de produits et leur coût restent à mesurer.
Le DFS fixe peut perdre l'avantage de l'ordre proche du milieu de32.

Le [modèle exact](prefix_model.py) passe 360 exécutions (90 configurations,
quatre quanta), rejouées à l'identique sous −O : 582 splits sur les
90 configurations, dont223 avec curseurs différents et11 avec une voie à
EOF. Quatre mutants changent les sorties et sont détectés ; les83 351
contrôles du préfixe ne reposent pas sur `assert`. [Reçu](PREFIX_CHECKS.json).
Les certificats de blocs du modèle sont exhaustifs : aucune performance
ni qualification produit ne s'en déduit. La note donne aussi une borne
Xi exacte par distance droite/boîte, comme complément conditionnel à la
borne affine déjà proposée par le constructeur et B.

## Preuve et témoin de régression31 désormais clos

La [preuve support/citron](SUPPORT_ET_CITRON.md) confirme le rejet sur
l'arête maximale du support positif, même avec une coquille plus grande.
Les fixtures u16 distinguent tangence stricte, coefficient q4=2, positivité,
propriété et coquille complète. La complétude concerne les présentations
canoniques du raccord, pas toutes les incidences ni le catalogue/FULL.

L'instantané [SNAPSHOT.json](SNAPSHOT.json) avait été copié explicitement
pendant le chantier31 à partir de c5308651. Ses deux fichiers correspondent
**octet pour octet à31 publié `4dbe3024`**, vérification conservée dans
CURRENT32_REVIEW.json. Les dépendances sont les sources et bibliothèques30
épinglées dans le manifeste de la campagne ; cette identité ne transfère
aucun résultat aux nouvelles sources32.

| Contrôle indépendant31 | Résultat |
|---|---:|
| Configurations distinctes | 149, sur13 fixtures |
| Appels C++ Release / Clang ASan+UBSan+LSan | 149 /149, tous concordants |
| Présentations comparées intégralement, multiplicités gardées | 7 532 occurrences cumulées |
| IDs de coquille comparés | 132 596 occurrences cumulées ; maximum30 |
| Comparaisons mono/Coarse1/4 dans le registre | 36 |
| Modèle rationnel support/citron | 209 supports positifs,1 744 contrôles support/site |
| Mutants géométriques de l'oracle | Sept réfutés, distincts des quatre mutants de préfixes |

K1/2/3/5/10, s8/10/12, masques2/4/6, Pure/Samples et Local28/Window30
sont exercés. Les configurations ne forment pas leur produit cartésien
exhaustif : l'inventaire exact est dans `campaign.configurations`.
Cas non axiaux, dégénérescences, u16 extrêmes, contact isolé et coquille30
sont inclus. La ligne260 est éliminée par un certificat exact de rang ;
aucun catalogue combinatoire de260 points n'est prétendu énuméré.

L'oracle rationnel calcule les centres, positivités, clés primitives,
profondeurs et coquilles sur le nuage entier. Le canon q4 groupe par arête
propriétaire, seed aiguë canonique et clé, puis choisit le partenaire minimal.
Les listes exactes triées sont comparées, sans digest pour le verdict.
Le travail logique est égal entre builds et modes d'exécution ; seules les
deux capacités privées q3 dépendant du placement workers sont normalisées.

[Capture corrigée complète](qualification_r2/COMPLETION.json),
[manifeste](qualification_r2/MANIFEST.json),
[lecteur](read.py), [lecture normale](READ_normal.json.gz) et
[lecture −O](READ_optimized.json.gz). Les deux lectures ont la même sortie
sémantique. Le premier essai [échoué](qualification/FAILURE.json) reste
conservé avec son diagnostic LeakSanitizer sous ptrace et ses sources.
La reprise distincte réussit avec `detect_leaks=1` ; aucun échec n'a été
effacé ou transformé en qualification.

## Reproduction et périmètre

Depuis la racine du dépôt :

```sh
python3 -B morsehgp3D_v8/audits/q34_global_contract_20260921/read.py
python3 -B -O morsehgp3D_v8/audits/q34_global_contract_20260921/read.py
python3 -B morsehgp3D_v8/audits/q34_global_contract_20260921/prefix_model.py
python3 -B -O morsehgp3D_v8/audits/q34_global_contract_20260921/prefix_model.py
```

Le lecteur31 contrôle aussi les binaires et entrées locaux ignorés par Git.
Pour reconstruire, `campaign.py <nouveau_dossier_enfant>` crée ses entrées,
compile la copie explicite et refuse d'écraser une capture existante ; il
requiert les deux bibliothèques30 épinglées, dont les hashes sont vérifiés.
Ces bibliothèques et les builds ne sont pas copiés dans Git. `CHECKS.json`
épingle les fichiers livrés de cet audit, hors temporaires/builds locaux.

Le constructeur a depuis documenté le citron et porté les filtres32 :
ces points quittent le dialogue actif, leurs preuves restent ici comme
fixtures et témoin de régression. Les revues indépendantes de B à1k–4k
complètent ce petit oracle. Son [rejeu32 publié à56fe457f](../q34_stream_crosscheck_t32_20260921/README.md)
retrouve les flux q3/q4 sur cinq lignes avec les nouveaux filtres et boxes ;
il compare les supports/profondeurs q3 et les clés/profondeurs q4,
sans comparaison indépendante des coquilles. Il conserve ses propres reçus.
Aucun nouveau benchmark LiDAR, TSan, backend GPU, contrat50k ou FULL dans
cet audit. La suite proposée ne modifie aucun défaut du constructeur.

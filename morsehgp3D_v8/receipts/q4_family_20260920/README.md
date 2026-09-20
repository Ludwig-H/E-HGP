# Première famille q4 exacte — preuves du20 septembre2026

`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. GCP non utilisé.

Objet : une seed aiguë donnée et tous les sites d'un propriétaire immuable.
Événements exacts, groupes égaux, profondeurs strictes et vues de coquille.
**Pas de générateur q4 complet, positivité/ownership des tétraèdres, catalogue,
FULL ou contrat G4.** Lire le [contrat](../../docs/Q4_FAMILLE_ET_BALAYAGE_20260920.md).

## Captures propres

| Capture | Périmètre | Résultat |
| --- | --- | --- |
| [smoke_nhmwuyk2](smoke_nhmwuyk2/COMPLETION.json) | Gate Release et trois sondes n32 | PASS |
| [smoke_zyzoosq9](smoke_zyzoosq9/COMPLETION.json) | Gate Clang ASan/UBSan et trois sondes n32 | PASS |
| [scale_8yioequm](scale_8yioequm/COMPLETION.json) | Gate Release et neuf mesures8k/16k/32k, CPU0 | PASS |

Chaque capture épingle135 sources, les exécutables réellement invoqués
et le cache CMake ; les empreintes sont vérifiées avant/après. Les lectures
normal/−O et le contrôle du lecteur sont clos séparément dans le dossier
[de reprise](../reprise_20260920/README.md). Les préflights non qualifiés
et la difficulté initiale de découverte Boost sont conservés dans son
[journal](../reprise_20260920/PREFLIGHT.md).

## Ce que juge la gate

206 appels sur57 nuages contre un oracle rationnel par élimination de
Gauss, indépendant de la formule de puissance et du déterminant réduit.
67 354 contrôles,52 507 comparaisons de racines,55 408 classifications
de sites sur les sphères résolues indépendamment ;2 210 groupes dont49
mixtes,688 diminutions de profondeur, profondeur maximale35, coquille30.
Le plus grand produit rationnel naïf exercé occupe145 bits : la primitive
produit évite ce croisement et calcule son déterminant réduit en i128.

Les permutations des seeds, axes et IDs, refus de triangles non aigus,
identités du propriétaire, callback levant, réentrance et quatre appels
concurrents indépendants sont exercés. Ces derniers ne constituent pas
une parallélisation du balayage d'une seed ni une qualification TSan.
Les cinq corruptions du jugement sont annoncées comme telles, **pas comme
des mutants injectés dans le moteur**. Le lecteur de reçus tue séparément
seize mutants de types, commandes, compteurs, non-vacuité et fermeture.

## Croissance locale d'une famille

Observations uniques, sur CPU0 de l'hôte partagé. « Famille » comprend
scan, tri, groupement, allocations/destruction de ses buffers et callback
de validation. « Total sonde » ajoute génération, propriétaire, préparation
du juge, validation et libération des buffers ; hors encodage JSON. La
génération n'est pas une étape produit. Aucun gain de vitesse apparié n'est
revendiqué ici.

| Régime | n | Famille avec callback | Total sonde | Comparaisons de tri |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | 8 000 | 3,942 ms | 6,004 ms | 127 592 |
| Uniforme | 16 000 | 8,153 ms | 12,591 ms | 265 812 |
| Uniforme | 32 000 | 17,464 ms | 27,205 ms | 574 373 |
| Deux lignes non coplanaires avec la seed | 8 000 | 3,865 ms | 5,168 ms | 133 043 |
| Deux lignes non coplanaires avec la seed | 16 000 | 8,566 ms | 11,490 ms | 291 570 |
| Deux lignes non coplanaires avec la seed | 32 000 | 17,187 ms | 23,260 ms | 621 541 |

Le tri croît de×2,083/×2,161 sur uniforme et×2,192/×2,132 sur les lignes.
Ces dernières ne sont pas les rangées coplanaires des anciennes campagnes
q2 : elles exercent ici les événements dans les deux orientations.
Le régime entièrement coplanaire produit zéro événement, comme il le doit,
et coûte0,065/0,154/0,256 ms pour le scan familial. Il ne prouve aucun
travail q4 non plat et n'émet pas implicitement la boule q3 à μ=0.

Les deux buffers d'IDs occupent128 000/256 000/512 000 octets dans les
trois régimes : exactement×2 aux doublements, même sans événement car
les deux réservations sont payées. Propriétaire, entrée et buffers du
juge sont déclarés séparément ; ces capacités ne sont ni le RSS ni une
mesure de mémoire de pointe incluant tous les temporaires.

Le callback lit chaque ID événement une fois et la coquille constante
une fois par famille. Il ne masque pas une répétition groupes×coquille.
Le noyau est O(n + e log(1+e)), e≤n, **pour une seed donnée seulement**.
Ni le nombre de seeds ni le travail global de q3/q4/FULL ne sont bornés
par cette qualification.

## Rejouer sans écraser les témoins

Les builds de cette capture sont `build/v8_q4_family_20260920` et
`build/v8_q4_family_sanitize_20260920`. Ils sont désormais épinglés : les conserver et
construire dans un répertoire neuf pour toute modification.

Le [lanceur](../../bench/run_q4_family_checks.py) propose `run` avec
`--campaign gate|smoke|scale`, `--build` et `--output`, puis `read CAPTURE`
et `selftest CAPTURE`. `read --check-live` compare aussi les octets présents
aux empreintes ; la lecture historique sans cette option conserve son
autorité propre. `python -O` ne désactive aucun contrôle.

# Index global exact — qualification G4

Source publiée **`e8520481d1745627e156723ad995ac5175a8163f`**, session
`v11.20261002.index1`, CPU mono sur G4, u18/u21/u24, défaut u21.
L'index possède le nuage entier. Un census rend K témoins stricts distincts
ou tout I/U, coquille comprise. Il compte des sites géométriques, sans
développer les multiplicités. [Contrat et preuve](../../docs/INDEX.md).

## Qualification native

| Configuration | Portes conformes |
| --- | ---: |
| Release u18, référence comprise | 251/251 |
| ASan/UBSan u24 | 176/176 |
| TSan u21 | 176/176 |
| Profils u21 et u24 | 176/176 chacun |
| Tampons empoisonnés u21 | 177/177 |
| Manifestes et campagnes de mutants | 15/15 |
| Style normal/−O | 2/2 |
| Complément num **et** index ASan/UBSan u18 | 36/36 |

Clang absent. Les 131 mutants sont détectés : 78 core, 20 num, 16 cloud,
9 catalogue, 8 index. Deux refus de compilation attendus appartiennent à
core ; aucun signal ou délai ne remplace une détection causale. Les huit
mutants index visent les contacts, les témoins distincts, la coquille,
la saturation, les deux passes, l'allocation exacte et la propriété sur refus.

Index : 845 contrôles natifs, dont quatre lecteurs concurrents de 16 requêtes
et quatre refus système injectés. Chaque oracle Gram/Fraction normal/−O
juge **1 010 requêtes et 36 020 contrôles** par profil : 302 census complets,
702 saturations, six refus, 47 paires d'entrées permutées. Les bornes num
ajoutent 183 contrôles natifs et 391 requêtes Fraction/3 400 contrôles.
Les 354 bornes valides couvrent aussi les extrema continus, les contacts
et les expressions dépassant 127 bits en u21/u24.

## Mesures distinctes de FULL

18 processus conformes, six entrées entières × trois profils, une répétition,
64 requêtes choisies par processus (16 de chaque arité q1..4), seuils 5/10/13.
Aucune dégénérescence rencontrée, aucune requête remplacée : 1 152 réponses
comparées à un scan global, dont 606 complètes et 546 saturées. Ce scan
réutilise `num::side` : indépendance du parcours seulement ; le petit oracle
Gram/Fraction apporte le contrôle arithmétique indépendant.

| Trame sans sol entière | Construction u21 | 64 census u21 | Construction u24 | 64 census u24 |
| --- | ---: | ---: | ---: | ---: |
| 08/000000, 39 885 sites | 0,400 ms | 0,810 ms | 0,401 ms | 0,825 ms |
| 08/000100, 35 551 sites | 0,341 ms | 0,621 ms | 0,339 ms | 0,620 ms |
| 08/000200, 45 845 sites | 0,418 ms | 0,703 ms | 0,409 ms | 0,700 ms |

La construction exclut lecture et Cloud. La somme des census inclut leurs
deux passes et buffers, mais exclut factories de sphères, scan témoin et
sérialisation, mesurés séparément. Cloud prend ici 0,853–1,033 ms en u21
et 1,415–1,742 ms en u24. Ce sont des requêtes artificielles, pas des descentes
MEB d'une tour. Aucun débit de FULL ni gain statistique n'en est déduit.

Uniforme 8k/16k/32k : construction u21 0,046/0,090/0,174 ms ; les 64 census
prennent 1,138/1,305/1,324 ms. L'arbre est linéaire par construction ; cette
série ne borne pas le nombre de requêtes d'une future tour.

Les six comparaisons interprofils ont les mêmes empreintes sémantiques et
compteurs géométriques. Les mêmes entiers u18/IDs servent aux trois binaires ;
la grille 1 mm ne change pas. Les petits tests aux grands bits qualifient
séparément le domaine supérieur. Les trois trames appartiennent à la séquence 08.

Réservations finales Cloud+index sur LiDAR : 1 545 716 / 1 772 108 / 1 938 988
octets pour 35 551 / 39 885 / 45 845 sites, identiques aux trois profils.
L'index seul vaut 40×nodes octets ; Cloud vaut 28n+8. Chaque résultat ajoute
4(|I|+|U|) octets, au plus 64 octets dans ces essais, puis les restitue.
Ce compte Buffer n'est pas le RSS. Catalogue, FULL et segmentation sont absents.
**Le contrat FULL 100 ms reste ouvert** ; les derniers chronos du catalogue
seul restent ceux de la [capture q4](../catalogue_q4_20261002/README.md).

## Relecture et clôture

`check.py` vérifie l'archive originale unique, les manifestes, le reçu local
vivant, les inventaires/JUnit et leur causalité, les profils/instrumentations,
les copies de résumés, les commandes, les sorties enregistrées et la fermeture.
Code0 signifie preuves cohérentes ; un échec explicitement conservé ne devient
pas conforme. Les payloads canoniques ont été supprimés après décodage sur G4 :
leurs empreintes enregistrées sont comparées, sans prétendre les rehacher ici.

```bash
python3 morsehgp3D_v11/receipts/index_20261002/check.py
python3 -O morsehgp3D_v11/receipts/index_20261002/check.py
```

La génération `2026-10-02T10:10:56.873-07:00` est certifiée arrêtée ; clé privée
et clé OS Login supprimées, verrou libéré, erreurs[]. Aucun calcul natif local.
Le premier lecteur a refusé une sortie JUnit de mutants tronquée à 1024 octets ;
la sortie intégrale de LastTest.log permet sa relecture sans rejouer le natif.
Les premiers défauts du lecteur restent distincts des tests G4 conformes.
Le lecteur normal/−O et sa contrelecture indépendante passent :
1 149/1 149 portes de matrice, 36/36 du complément, 18/18 essais.
Son autotest joue 20 témoins et 61 corruptions sans exécution native ;
les premiers refus et les hashes sont dans `check_selftest.json`.

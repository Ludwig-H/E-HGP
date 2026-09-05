# Coût local MEB — première campagne du 5 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Décision

**Aucune activation générale ni aucun seuil de dispatch n'est établi.**
La proposition réduit fortement les candidats sur une partie du corpus,
mais les petits cas répétés ralentissent. Les lots très courts de la
matrice principale montrent aussi un effet d'ordre des deux bras.
Le produit F reste inchangé ; P=0 est toujours le défaut prévu du port.

La [capture publique](../receipts/meb_dual_cost_20260905/README.md) conserve
les mesures défavorables comme favorables. Elle ferme séparément la
qualification native `NoObserver`, sans transformer la précédente
[porte géométrique Trace](RESULTATS_MEB_DOUBLE_BUDGET_20260905.md) en
qualification d'un autre binaire. La
[contrelecture indépendante](../audits/receipts_meb_native_20260905/README.md)
retrouve ces limites depuis les mêmes bruts, sans nouvelle exécution moteur.

## Ce qui a réellement tourné

Le premier build a échoué, code compilateur 1 : le macro objet renommant
`main` touchait aussi `Metrics::main`, empêchant la compilation du harnais.
Le reçu `247c952c` conserve cet échec, sans binaire ni mesure. La révision
distincte utilise un macro fonctionnel ; aucun octet v1 n'est réécrit.

Le build v2 `de6de29f` termine à 11:45:46 UTC. Le binaire
`56e022c817d2e726eb2e3b135e78e577bbdf344ebd0ff352d64d1121300fd976`
est compilé en C++20/O2, GCC 13.3.0, sans LTO, sanitizer ni macro de test.
Le désassemblage est relu avant mesure : les appels F et
`miniball<false,NoObserver>` restent dans les boucles, entre les deux
horloges, et chaque résultat est consommé. Cela n'établit pas une
isolation matérielle sur cet hôte partagé.

La capture `874f100ffb1d65956f6d640c5e7ab838a81e9f5c7900f7c1d69b14504235c208`
termine à 11:50:16 UTC, CPU6, un thread. Ses 2,98 s incluent qualification,
mesure, capture et fermeture : ce n'est ni le temps d'un helper ni celui
d'une tour. L'hôte déclare un AMD EPYC 9V74 et huit CPU virtuels.

| Inventaire | Nombre |
| --- | ---: |
| Scènes / ordres du corpus géométrique | 176 / 384 |
| Jobs / états qualifiés avant puis après | 9 347 / 9 351 |
| Groupes de mesure | 4 699 |
| Appels supérieurs par bras et passage | 58 491 |
| Chauffes / passages mesurés | 2 / 7 |
| Entrées MEB, juges et replis inclus | 1 325 812 |
| Plafond d'entrées / délai total | 2 000 000 / 120 s |

Les 9 216 comparaisons principales et 123 frontières sont complétées par
douze cas de la paire q2, répétés 4 096 fois par ordre. Les comparaisons
complètes F/Trace/NoObserver avant et après vérifient booléen, diagnostic,
treize statistiques legacy, événements, clé, support entier, niveau brut
et Work. Les captures 64 bits des boucles chronométrées ne remplacent
pas ces comparaisons complètes. Les resets, sentinelles, wrappers,
barrières, copies et captures sont inclus dans les temps annoncés.

## Candidats : gain réel de compte, pas encore gain de tour

Sur les 1 152 appels principaux P401, F essaie 67 884 candidats.
Le prototype paie 10 582 propositions et 140 candidats de repli, soit
10 722 candidats physiques. L'ordinal legacy reste 67 884 : ce n'est
plus la charge physique totale. L'auditeur dénombre 795 diminutions,
159 égalités et 198 augmentations de ce compte par appel.

Cette somme ne compte ni les distances de recherche de paire, ni toutes
les puissances, ni les copies. Elle ne constitue pas la distribution des
supports rencontrés dans une tour. Un certificat peut aussi être suivi
d'un refus legacy ; ne pas le compter comme un succès public.

## Temps : petits cas défavorables et contrôle d'ordre

Le q2 immédiat regroupe ses deux permutations, soit **8 192 appels par
bras et passage** pour chaque ligne ci-dessous. Aucun de ces lots n'est
marqué court. Temps F/dual : médianes séparées en ns/appel ; variation :
médiane des sept ratios appariés, pas ratio des deux médianes.

| P | L | F, ns/appel | Dual, ns/appel | Variation appariée |
| --- | --- | ---: | ---: | ---: |
| 0 | 1 | 227,49 | 250,49 | +9,38 % |
| 0 | 2 | 228,53 | 249,42 | +9,05 % |
| 1 | 1 | 229,50 | 325,79 | +42,78 % |
| 1 | 2 | 229,18 | 319,66 | +39,39 % |
| 401 | 1 | 229,73 | 321,27 | +39,49 % |
| 401 | 2 | 228,40 | 322,80 | +40,05 % |

Sur la **matrice principale seulement**, les variations P0 par passage
sont −12,384 %, +15,070 %, −16,304 %, +18,025 %, −10,385 %, +13,232 %
et −13,057 %. Leur signe suit l'ordre : F puis dual aux passages impairs,
dual puis F aux pairs. Sept passages donnent quatre AB et trois BA ;
la médiane agrégée ne corrige pas ce déséquilibre. Il ne faut pas
confondre cette matrice avec le total incluant les longs lots q2.

La même matrice à P401 donne des diminutions sur les sept passages,
mais ses nombreux petits lots empêchent d'en déduire une accélération
générale. Le cas nommé q4 de rang 550 à P401/L550 semble très favorable
(24,33 contre 2,36 µs, médianes séparées), mais **un seul appel** est
mesuré par bras et passage, et cinq durées sur quatorze sont courtes.
Ce cas motive un suivi ; il ne qualifie pas ce facteur d'accélération.
La stratification exploratoire retrouve également des régressions aux
petites tailles. Aucun seuil n'est choisi à partir de ces observations.

## Suivi déclaré après ces observations

Un protocole distinct conserve tous les 384 ordres, y compris les
coquilles refusées : P0/P401, L551 uniforme, 64 répétitions fixes par job,
deux chauffes et dix paires équilibrées, cinq AB et cinq BA. Les 768 jobs
restent présents ; aucun support pilote n'est donné au proposeur.
La borne conservatrice est 1 779 072 entrées MEB, juges et replis compris.
Ce paragraphe décrit sa préparation, pas une mesure exécutée.

Le critère primaire est le coût de tous les jobs, séparément pour P0 et
P401 ; les strates par taille, arité, terminal et route sont secondaires.
AB/BA restent séparés, sans soustraire P0 ni sélectionner un seuil après
les mêmes temps. Chaque répétition part d'un Work neuf : le coût du
budget partagé par ordre et celui des miroirs du futur Builder restent
à qualifier dans la vraie intégration.

## Conservation et contrats ouverts

L'archive garde le build v1 échoué, le build v2, sa mesure et leurs
sources inertes. Deux bruts seulement sont compressés sans perte :
désassemblage et stdout des mesures. Gzip déterministe, hashes et tailles
bruts/compressés distincts, décompression confrontée aux octets originaux.
Les autres copies restent byte-exactes ; aucun ELF n'est distribué.
Les 164 fichiers comprennent 157 copies, dont deux gzip ; les 163 entrées
de sommes sont contrôlées. Le sceau `SHA256SUMS` est
`e1b34a7d7db91da8ae453bda9861c80809116803f0c3e244b29180ddd16da288`.
Les runners historiques gardent leurs chemins absolus : reconstruction
manuelle des sources ne signifie pas reproduction hermétique de la capture.

Le helper n'ajoute aucune structure géométrique globale ; ses sites et
supports restent bornés localement. Il ne résout pas le refus à 32k sur
les occurrences de facettes avant déduplication. Aucun temps de tour,
SLO 50k/1 s/100 ms, résultat multi-CPU/GPU ou dizaines de millions n'est
établi ici. GCP non utilisé.

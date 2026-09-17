# Entrelacer les petites requêtes q2 sans changer leur recherche

15 septembre 2026, close le 17 septembre 2026. Tranche 19, exploration v8
hors registre, cpu_reference, quantized_u16_input_only,
implementation_v8_p0, not_claimed. **Résultat négatif qualifié : le format
à lots est exact mais plus lent que Coarse ; la piste est fermée.** Les
sections ci-dessous gardent le contrat tel qu'il a été implémenté ; la
dernière donne les mesures et la décision.

## Le coût visé

Sur uniforme32k, la tranche18 garde11,084 millions d'ancres et1,001 milliard
de visites census. Ses plages sont presque toutes trop petites pour être
données à un autre worker. Toutefois, leur chemin Shared singleton est
déjà une boucle sans pile de témoins ni allocation par ancre. Le présenter
comme une économie de49 cadres serait faux : ces cadres appartiennent
à la continuation possédée, absente de ce chemin.

L'hypothèse testée ici est plus précise : plusieurs petites recherches
indépendantes, entrelacées dans un lot réutilisable, pourraient mieux
utiliser le CPU en masquant les attentes mémoire. La spécialisation aux
feuilles B peut aussi éviter des décisions génériques inutiles. Rien de
cela ne réduit par soi-même le nombre de visites géométriques.

## Objets et obligations

Une entrée explicite distincte utilise les seeds Coarse du même index.
Chaque worker possède un petit lot d'états et réutilise ses tampons de
sortie. Seules les feuilles B de Shared/Individual y entrent ; les groupes
B conservent leur traitement actuel. Un état garde sa clé exacte, son
ancre, B original, rang spatial d'ancre, compte, curseur, phase et frère
encore dû. Les états sont privés : aucune API ne permet de forger un
préfixe déjà crédité. Aucun pointeur sur le contexte d'une pile passée.

Trois étapes logiques restent distinctes : entrée et certificat frère,
parcours des témoins, collecte puis émission. Une pause ne refait pas
l'entrée ni la clé, ne repart pas de la racine, ne transforme pas B
original en B singleton. L'ancre est omise du seul comptage, jamais de
la coquille ; les autres sites de B original peuvent être intérieurs.

Un lot plein avance les états existants jusqu'à libérer de la place.
À la fin d'une seed, il est entièrement vidé avant d'annoncer sa fin.
Pool demeure Pairwise et synchrone ; le lot est vidé avant d'y entrer,
sans reconstruire le plan ou changer son ordre B. La collecte d'un
support et son callback restent atomiques, avec coquille non plafonnée.
Une erreur conserve les émissions antérieures et rejoint tous les fils.

Le nombre de voies est un stockage de travail en vol, pas un plafond
d'entrée ou de recherche. Le quantum borne une avance, pas sa complétion.
Ni nouvelle équipe par ancre, ni nouvelle file concurrente, ni catalogue
global de millions de requêtes n'est nécessaire pour ce premier essai.
L'ancien Coarse et les plages conservent leurs comportements par défaut.

## Ce qui doit être mesuré

Comparer d'abord une voie au Coarse mono : cela isole le coût du format
et des transitions. Comparer ensuite plusieurs voies à une voie, puis
un/quatre workers, sur les mêmes nuages. Compter séparément les états,
avances, transitions et places actives ; vérifier tous les comptes de
front, census, frère, ordre, Pool et payload. Un digest seul ne suffit pas.

Les fixtures positives doivent exercer un singleton qui hérite d'un
crédit, la deuxième phase sur B original, le frère saturant encore dû,
la tangence stricte et une coquille30. Les anciens cas Pool et les
exceptions restent des obligations de raccord, pas des branches supposées.

Les campagnes locales gardent n8k/16k/32k, K5/10 et s8/10/12. Publier
croissance du travail et mémoire avec le temps global, y compris les
régressions. Un entrelacement plus rapide ne rend pas la géométrie
sous-quadratique ; il ne ferme ni P0 global, ni q3/q4, ni FULL/G4.

## Résultat : régression mesurée, piste fermée

Qualification propre close sur la source r3 gelée le 17 septembre 2026
([reçus](../receipts/q2_singleton_batch_20260915/README.md)) : 75 CTests
Release et Clang ASan/UBSan, porte Clang ThreadSanitizer, 595 appels à lots
et 178 Coarse contre l'oracle indépendant (10 958 paires, 778 276 tests de
sites, 124 145 supports, coquille 30), 172 mesures et 26 lectures et
analyses normal/−O identiques. Les fixtures positives demandées sont
exercées : 9 830 entrées après crédit, 4 482 en deuxième phase sur B
original, 40 054 frères encore dus, tangence stricte, coquille 30. Tous les
comptes de front, census, frère, ordre, Pool et payload égalent Coarse.

L'hypothèse testée est réfutée. Sur les 54 comparaisons closes à
n8k/16k/32k (quatre familles, K5/10, s8/10/12, un et quatre workers,
quantum 64), le rapport lots / Coarse va de 1,005 à 1,225, médiane 1,112 ;
aucune n'est plus rapide que Coarse. Une voie et seize voies entrelacées
donnent les mêmes temps : l'entrelacement ne masque aucune attente mémoire
mesurable sur ces nuages, et le surcoût est celui du format (copie de
l'état de 72 octets, aiguillage par étape, compteurs de gestion). Trois
révisions ont été mesurées : r0 (un appel de progression par témoin) à
×1,29 à ×1,76 au quantum 1 et ×1,15 à ×1,53 aux quanta 8/64 ; r2 (boucle
locale des témoins) à ×1,07 à ×1,35 ; r3, identique à r2 en géométrie, sur
hôte calme. L'auditeur B trouve indépendamment +40 à +70 % au quantum 1
sur 8k/16k/32k et 0 désaccord contre la force brute.

Décision : `run_wspd_q2_census_batched` reste une entrée explicite, hors
défaut, conservée comme témoin mesuré ; Coarse reste le défaut et aucun
autre chemin ne dépend des lots. La piste est inscrite aux
[fausses pistes](FAUSSES_PISTES.md). Le diagnostic qui survit : le Shared
singleton était déjà une boucle Z sans allocation ni pile, il n'y avait pas
d'attente à masquer ; le coût des millions de petites requêtes se réduit en
diminuant leur nombre, pas en changeant leur format. C'est l'objet de la
[surproposition de témoins](P0_SURPROPOSITION_TEMOINS_Q2.md).

Limites de couverture conservées : `GlobalDfs`, défaut de l'API, n'est
exercé que par la porte C++ contre l'oracle (n ≤ 100) ; sonde, porte de
reçus et campagnes fixent `ComplementFirst`. Le registre des lots n'est
valide que sur succès de l'appel entier (compteurs engagés une fois par
visite de voie). Une observation de temps par configuration, hôte partagé :
ni gain ni perte n'est un contrat G4.

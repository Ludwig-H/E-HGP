# Cache S2 : ce que l'optimisation peut, et ne peut pas, résoudre

Audit du commit `d0e711e23617a7fa2838cac9b370ff125d4be5fb`, avant les
mesures G4. Aucun changement du moteur dans cette note.

## Exactitude

Une trace provient d'une seule recherche exacte, pour une paire
représentante. Ses nœuds sont disjoints **pour chaque voie**, pas
nécessairement entre voies q3 et q4. Chaque entrée a apporté au moins un
crédit effectif : il y en a au plus `(K-1)+(K-2)`, donc 17 à K10.
Pour une nouvelle paire, chaque nœud est retesté avec ses nouvelles
extrémités. Les contacts ne sont pas des intérieurs. Les voies qui ne
sont pas entièrement rejetées repartent à la racine, avec compte nul :
aucun crédit partiel n'est additionné à une recherche qui pourrait
revisiter les mêmes sites.

Le représentant fournit directement son propre masque ; les autres
paires conservent l'ordre rectangle/ligne/colonne historique. Les tuiles
ne franchissent jamais la fin d'une ligne. Un facteur B de moins de 16
sites prend le chemin sans cache. Cela ne supprime aucune candidate.

Le refus d'un débordement de pile ou de trace concerne tout le passage,
pas seulement le thread fautif. Une passe fautive ne produit aucune
qualification géométrique ou temporelle.

## Travail et mémoire

Notons R le nombre de rectangles transmis, P le nombre de paires après
le filtre de rectangles, et
`T = somme |A| ceil(|B|/32)` sur les rectangles survivants avec `|B|>=16`.
Les traces coûtent au plus `O(K T)` cases utiles, stockées ici dans un
format fixe de 88 octets par représentant. Le masque représentant ajoute
un octet ; masses et préfixes ajoutent 16 octets par rectangle. La formule
`89 T + 16 R` exclut explicitement le scratch CUB, les allocations déjà
présentes et les capacités d'allocateur : **ce n'est pas le pic VRAM**.

Le test des traces coûte `O(K P)`, en plus des recherches des
représentants et des replis. La sélection d'un rectangle par recherche
binaire coûte `O(log R)` par paire/représentant dans ce port. Le cache
peut éviter beaucoup de recherches, mais ne change pas P : **si le
résidu P est quadratique, ce port ne le rend pas sous-quadratique**.
Les séries locales 8k/16k/32k du prototype précédent restent des mesures
du code et de la population qui les ont produites ; elles ne deviennent
pas une preuve de croissance CUDA pour ce nouveau port.

`pair_visits` contient déjà les parcours des représentants ; il ne faut
pas lui ajouter `trace_node_tests`. Le comparatif géométrique est
`pair_visits + cache_node_tests`, avec les deux termes publiés. Ce total
reste un indicateur discret, pas un modèle uniforme du coût de tous les
tests. Le temps net mesuré tranche le choix de port.

## Mesure et limites de qualification

Le scan/préparation des tuiles est dans `scan_ms`. Les deux kernels
(représentants et paires) sont ensemble dans `pair_ms`. Chaque répétition
est publiée, et les tableaux de masques ainsi que les compteurs sont
comparés entre répétitions. Cette vérification hôte est hors des events
CUDA ; les chronos d'events ne sont donc pas le mur du processus.

Le contrôle CPU exhaustif de la sonde juge chaque masque de la dernière
passe ; l'identité entre répétitions étend ce contrôle à leurs masques.
Cela qualifie le filtre sur **la population fournie par le front**, pas
la complétude indépendante du générateur q3/q4, le catalogue ou FULL.
Le compactage de `run_filter_batch` avec cache est un chemin distinct à
juger avant son activation dans la chaîne : la sonde `run_filters` ne
matérialise pas ces tableaux de survivants compactés.

Les trois trames prévues sont toutes de la séquence 08, sans sol et en
grille 1 mm. Ni plusieurs séquences, ni le brut, ni le float32, ni les
dizaines de millions de points ne seront qualifiés par cette campagne.
Le gain S2 éventuel ne sera pas soustrait arithmétiquement d'un ancien
chrono FULL : l'intégration devra mesurer sa contention et son coût net.

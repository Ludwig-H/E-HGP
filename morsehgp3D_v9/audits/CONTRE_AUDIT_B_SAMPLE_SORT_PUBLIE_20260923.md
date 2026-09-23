# Contre-audit B — tri d'échantillonnage publié de la chaîne

23 septembre 2026. Portée : `50690c12`, tri
`gather_presentations(slots,W)` et raccourci FULL ; correctifs d'échec
et de reçu ultérieurs `6200bb5a`. **Aucun chrono G4 ne qualifie ce tri** :
R6 exécute le snapshot antérieur `78ce9fd4`. La mesure locale rapportée
dans la passation (000100/K10/W8, fusion 1,04→0,47 s) est un signal de
développement, pas une ablation G4 avec reçu épinglé.

## Exactitude de l'ordre et limite de la porte

Chaque slot est trié selon `(BallKey, arité, support)`. Les séparateurs
de clés sont strictement croissants ; `lower_bound` place **toutes** les
présentations portant une même clé dans une seule plage, qui est ensuite
triée et recoupée. L'ordre des plages est celui des clés. Le tableau de
représentants pointe vers les plages possédées et reste vivant pendant le
recensus ; il est abandonné avant la tour. Le chemin FULL ne saute son
tri `by_key` qu'après un scan strict de toutes les clés, donc garde le
refus des doublons sur un catalogue arbitraire. Aucune divergence
d'objet n'a été trouvée dans ces chemins ; la porte différentielle
1/4/7 workers donne le même digest, mais ne remplace pas un inventaire
exhaustif sur grande trame.

La porte `chain_static_paths_gate` veut prouver plusieurs plages utiles,
mais ne teste que `presentation_ranges≥2`. Ce champ compte les plages
**créées, même vides**. Un mutant compilé dans un worktree détaché de
`50690c12` remplace les séparateurs par `{sample.front()}` : il crée
deux plages, la première vide et la seconde avec **toutes** les
présentations. Le gate passe pourtant **code 0**, avec le même digest
`73490cf88c02af30` et `workers_created=16` ; ce dernier compteur
appartient à FULL, pas au tri des plages. Le mutant ne rend pas la tour
fausse, mais tue la prétention du plancher actuel à exercer le
parallélisme inter-plages. Publier/tester au moins `nonempty_ranges`,
`max_range_presentations` et un plancher de distribution sur une fixture
ciblée ; mesurer ensuite le travail/temps par ouvrier sur G4.

## Travail et mémoire

Le tri publié trie d'abord les slots puis copie leurs `P`
présentations vers les plages, qu'il retrie : les deux ensembles sont
temporairement co-résidents. À 08/000000/K10 R6, `P=5 512 675` et
`sizeof(Presentation)=112` donnent environ **589 Mio** pour un ensemble
complet. Ce **n'est pas +589 Mio par rapport à R6** : l'ancien chemin
recopiait déjà `slots→all` avant de libérer les slots. Le nouveau chemin
ajoute surtout une liste de pointeurs de représentants et, pendant la
fusion, des listes de premiers représentants par plage et un échantillon
de clés ; leurs résidences se chevauchent différemment. Un pic RSS/phase
du binaire publié est nécessaire avant de chiffrer un surcoût réel ou
de promettre le régime à dizaines de millions de points.

Sur `50690c12`, les créations de fils de `gather_presentations` pouvaient
encore rendre `kInvariantViolated` et perdre `merge_ms` en cas d'échec :
`6200bb5a` ajoute les catches `system_error`/`length_error`, un
chronomètre de phase qui publie le travail payé, et une porte d'injection
de lancement de fil. Il vide aussi les résumés d'ordres sur échec du
digest. Ces correctifs sont **publiés mais sans nouveau reçu G4** ;
ne pas appliquer la critique du statut de `50690c12` au code courant.

Le tri reste un poste secondaire face à q3/q4 : R6, sur l'ancien code,
mesure 0,15–0,77 s de fusion/tri contre 2,61–9,83 s de q3/q4 selon le
cas cœur ON. Le nouveau coût et sa courbe 8k/16k/32k doivent être
mesurés, mais supprimer entièrement ce poste ne suffirait pas au
contrat d'une seconde sur les trames R6.

## Extension au tri des requêtes FULL (`75f27eee`)

Ce second tri est **distinct** de `gather_presentations`. Le commit
`75f27eee` remplace la fusion sérielle de `tower::parallel_sort` par
échantillonnage, classification, scatter et tri parallèle des seaux ;
la collecte des requêtes/graines et la détection des départs de groupes
sont également distribuées. Les ordinaux sont attribués par préfixes
dans l'ordre de collecte initial ; chaque chunk et chaque seau possède
des plages disjointes, jointes avant l'étape suivante. La comparaison
`(clé,ordinal)` garde l'unique permutation triée, et je n'ai pas trouvé
de perte ni de race dans ce port. Les exceptions des workers sont
relayées après jointure. Recompilation indépendante du source dans
`/tmp/mhgp9-sort-audit.BJSf2I` : la porte produit passe **400/400** cas
(160 multi-workers) ; le mutant « seau non trié » échoue causalement à
`n=8192`, `range=1`, W2. La porte produit passe aussi sous Clang
ASan/UBSan dans `/tmp/mhgp9-sort-san-audit.P88Yzq`. Ces chemins
temporaires ne sont pas un reçu archivé ; aucune mesure G4 de ce commit.

Le choix **périodique fixe** des échantillons ne garantit pas des seaux
équilibrés. Témoin strictement ordonné : pour `n=200003`, W48,
`samples=6144`, placer les 6144 plus petites clés distinctes exactement
aux indices `⌊i·n/6144⌋`, et toutes les autres clés au-dessus. Les
séparateurs ne voient que les petites : le dernier seau reçoit
**193 891 éléments (96,94 %)** et est trié par **un seul worker**.
L'objet final reste exact et le coût reste O(n log n), mais ce code
n'assure pas un gain de parallélisme dans ce régime. La porte actuelle
n'exerce que des clés mélangées aléatoirement ; `workers_created` ne
mesure pas l'occupation des seaux. Ajouter ce témoin et publier
`nonempty_buckets`, `max_bucket`, distribution et temps par seau sur
les vraies requêtes LiDAR avant d'attribuer un gain W48/G4.

Le suivi mémoire `static_peak_request_bytes` n'inclut que deux fois la
capacité des requêtes. Le nouveau tri garde en plus `bucket_of` (4n
octets), échantillon/séparateurs, comptes/décalages et tampon scatter ;
la détection de groupes alloue encore ses listes de départs. Le RSS
global reste mesurable, mais le compteur analytique n'est plus une
borne de ce pic. `resize` des requêtes/graines, concaténation des listes
de départs et initialisation des cibles restent sériels : le port ne
supprime pas tout le plancher hôte. La mesure locale W8 est dans le
bruit et W48 local est sursouscrit ; aucun reçu apparié G4 n'établit le
gain de chaîne de `75f27eee`.

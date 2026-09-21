# Tranche33 — ne plus chercher des témoins hors du citron

Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Base constructeur32 : `d1b4dbc6`. Port, qualification fonctionnelle et
matrices grandes tailles clos dans leurs propres reçus ; aucun résultat
hérité des audits indépendants.

## Le travail que l'on cherche à éviter

La recherche32 élimine une boîte témoin lorsqu'elle est hors de la boule
diamètre. Mais le citron q3/q4 est plus étroit : des boîtes sans témoin
utile peuvent encore être ouvertes jusqu'aux points. À32k/K5/s8, les
tests Xi par paire augmentaient de×7,440 depuis16k. Le but est d'écarter
ces boîtes plus tôt, pas de changer les candidats finalement conservés.

Le [retour B74196a31](../audits/DIALOGUE_AUDITEUR_B.md) et les
[pistes32](Q34_PISTES_APRES_INDEXATION_20260921.md) motivent ce port.
Les couches q3, sûres mais potentiellement chères à préparer, restent
différées. Aucun nouveau moteur q2, catalogue global ou ordonnanceur
n'est ajouté dans cette tranche.

## Trois modes comparables

`Q34WitnessBoundsMode` est une option indépendante du choix de rechercher
par paire ou par rectangle puis paire :

| Mode | Bornes de H et Xi | Exclusion du bloc témoin |
| --- | --- | --- |
| `Legacy` | Bornes32 | Hmax≤0 seulement |
| `Exclusion` | Mêmes bornes générales | Hmax≤0 ou certificat Xi_min |
| `Affine` | Préparation spécialisée si A/B singleton ; générales sinon | Même certificat Xi_min |

`Legacy` reste le défaut. L'ancienne signature de recherche reste disponible
et conserve ce parcours. La nouvelle option est validée même si la voie
demandée est inactive. Un mode de bornes ne rend pas active une recherche
de témoins désactivée par `WspdQ34WitnessMode::Disabled`.

Pour une paire fixe, on prépare d=b−a et a+b une fois. Les trois composantes
de d×(z−a) sont affines en z : leurs extrema sur une boîte utilisent les
signes des coefficients, sans lecture des sites ni énumération des coins.

$$4H=|b-a|^2-|2z-a-b|^2,\qquad \Xi=|(b-a)\times(z-a)|^2.$$

Le minimum carré d'un intervalle est nul s'il contient zéro ; sinon c'est
le plus petit carré de ses extrémités. Le maximum est le plus grand carré.
La somme des trois minima/maxima encadre Xi. Ces bornes ne sont pas toujours
les extrema exacts de la somme, mais sont au moins aussi serrées que les
anciennes pour une paire singleton et deviennent exactes au point.

Pour les rectangles non singletons, aucun représentant ni milieu n'est
substitué aux facteurs. On garde les bornes conjointes de4H et les bornes
générales de Xi sur A×B×Z. Toutes les décisions portent sur les boîtes
continues, donc aussi sur les sites qu'elles contiennent.

## Admettre des témoins n'est pas exclure des témoins

Avec alpha3=3, alpha4=2 :

- Hmin4>0 et alpha·Hmin4²>16Xi_max admettent le bloc comme témoins.
- Hmax4≤0, ou alpha·max(0,Hmax4)²≤16Xi_min, excluent le bloc de témoins.
- Sinon la voie reste indécise et la descente continue.

L'égalité dans la seconde condition est sûre : une tangence n'est pas
un témoin strict. Mais **exclure un bloc Z ne rejette jamais une arête**.
Cela ne donne aucun crédit ; seul le bit LOCAL du cadre Z disparaît.
Il faut toujours K−1 témoins distincts pour rejeter q3, K−2 pour q4.
Chaque compte conserve sa partition disjointe, même quand un bloc est
admis pour q3 mais exclu pour q4. Aucun crédit n'initialise le census
d'une boule ni la recherche suivante.

Les produits sont promus avant multiplication. Pour M=65535,
|C_i|≤2M², Xi_max≤12M⁴, 16Xi_max≤192M⁴<2⁷² et
alpha·max(0,Hmax4)²≤27M⁴<2⁶⁹. Les décisions tiennent en i128.
Les extrema de4H tiennent en i64, y compris les valeurs négatives.

## Coût, états et instrumentation

L'index et les coordonnées sont partagés ; la préparation ne possède
que des constantes. Pas de copie du nuage, de liste de témoins, de scan
des facteurs ni d'allocation par requête. La pile DFS conserve la borne49
issue des coordonnées u16 et des subdivisions de l'index, pas un quota.
L'ordre « proche du milieu d'abord » reste celui de32.

Les25 compteurs historiques `Q34WitnessSearchWork` restent distincts
des12 compteurs nouveaux `Q34WitnessBoundsWork`, tous additifs. Il existe
un registre de bornes pour les rectangles et un pour les paires. On paie
notamment la préparation spécialisée, les tests Xi nouveaux lorsque
Hmin≤0, les tests d'exclusion par voie et les terminaisons mixtes.
Ces coûts ne disparaissent pas sous le seul nombre de visites évitées.

La partition des nœuds visités est désormais : exclusions H, admissions
complètes, exclusions Xi complètes, terminaisons mixtes, feuilles restantes
et subdivisions. Les anciens `q3_lane_tests/q4_lane_tests` comptent les
tests d'admission ; les nouveaux tests d'exclusion sont séparés.
Le mode Legacy n'ajoute rien aux nouveaux registres, sans effacer un
registre déjà fourni par l'appelant. La hausse de taille des registres
privés reste un coût réel, même si une option n'est pas utilisée.

Le probe conserve les schémas1/2 pour les anciennes lignes de commande.
L'argument final `legacy|exclude|affine` produit le schéma3, le champ
`witness_bounds_mode` et les deux nouveaux registres. Le même pipeline
paie ensuite couvertures, seeds, census, q4, tris et callbacks.

Une borne O(nœuds visités) par recherche ne prouve pas que leur somme est
sous-quadratique. La campagne doit contrôler aussi les autres postes qui
restent inchangés, notamment les visites de carte q4 Local28.

## Validation fonctionnelle close

Nouvelle porte indépendante des bornes par paires, enrichissement des
portes de recherche et du raccord global ; oracles multiprécis et
rationnels, tangences, minima traversant zéro, valeurs u16 extrêmes,
états mixtes, modes invalides et comparaison mono/multi. Les trois mutations
ciblées sont réfutées géométriquement, pas seulement par un registre
incohérent. Les mêmes payloads et masses finales sont exigés entre les
modes ; le travail géométrique du filtre, lui, doit changer.

95 CTests Release, quatre portes et24sondes Clang ASan/UBSan/LSan passent.
Ce n'est pas une exécution instrumentée des95tests. La porte des bornes
compare268cas indépendants (10193contrôles), celle des recherches ajoute
12797appels de modes, et celle du raccord40appels de modes dont24parallèles.
Les huit comparaisons schéma2 au probe32 retrouvent sorties complètes et
travail logique ; +192octets réels par worker pour les nouveaux registres.
Les capacités privées sensibles à l'ordonnancement restent publiées.
Les [reçus33](../receipts/q34_affine_20260921/README.md) conservent aussi
l'échec initial LSan sous ptrace, les préflights et les lecteurs normal/−O.

Les nouveaux builds `build/v8_q34_affine_20260921` et
`build/v8_q34_affine_sanitize_20260921` sont désormais épinglés.
Les builds32 restent intacts. Trente grandes mesures33 sont closes :
comparaison mono/multi8k, scan0 à8k/16k/32k K5/10 et Local28/Window30,
puis K5/s10/12 avec Window30 et scans100/200 K5/s8 avec Local28.
Les temps locaux sous charge restent des observations, pas des gains stables.

À8k/K5/s8, les visites par paire passent de38,642M à16,745M ; les
tests Xi totaux augmentent toutefois de24,653M à25,629M. Les exclusions
paient42,522M comparaisons de voie en plus des7,589M admissions. La
croissance des visites par paire sur scan0/K5/s8 devient×2,603/×2,773,
contre×5,364 au dernier doublement32. Tout l'aval apparié reste identique.

Cela ne ferme pas les régimes importants : Local28 conserve les visites
q4×4,018/×5,603 sur scan0 ; scan200/K5 fait×4,317 de bornes census q3
et×6,540 de visites q4 entre8k/16k. Certains sous-compteurs payés de
construction sautent aussi au-delà de4 ; voir le relevé exhaustif des
[reçus33](../receipts/q34_affine_20260921/README.md). Une moyenne favorable
ou un agrégat ne fait pas disparaître ces postes.

## Suite : héritage compact, pas résultat terminal recyclé

La [proposition de A](../audits/q34_global_contract_20260921/PREFIXES_TEMOINS.md)
donne deux comptes et deux curseurs Z pour un ordre DFS fixe, un par voie.
Chaque préfixe consommé doit être entièrement classifié pour toutes les
paires du produit. Une feuille Z encore ambiguë force une subdivision
A/B AVANT sa consommation ; copier simplement un résultat terminal32
avec curseur épuisé serait faux. Ce modèle n'est pas implémenté ici.

Le DFS fixe peut perdre la localité de32 et multiplier les subdivisions.
Le modèle exhaustif d'audit prouve le protocole, pas son coût. La préparation,
le stockage des produits et tout l'aval devront être mesurés au port.
Le complément distance droite/boîte proposé par A reste lui aussi optionnel,
sans ralentir la qualification de cette première borne affine.

Le [retour A93ce6fc5](../audits/q34_prefix_order_20260921/README.md)
précise deux ordres parentaux figés, circulaire et orienté sur un pivot.
Sur son échantillon de2344requêtes saturées de rectangles multiples,
le DFS global paie2,094fois les bornes H du parcours proche du milieu,
contre0,945pour le cercle et1,078pour le chemin-pivot. Ces rapports de
sonde ne sont ni des temps, ni une croissance globale, ni les coûts d'un
héritage produit non implémenté. Ils empêchent surtout de supposer que
le remplacement gratuit par un DFS global serait neutre.

## Suite q3 : compter par blocs de graines, piste non implémentée

Le scan200 motive une question distincte adressée aux auditeurs dans le
[journal](../../audits/COORDINATION_MORSEHGP3D_V8.md). Pour une arête a,b
fixe, serait-il rentable de rechercher des témoins communs aux boules
d'un bloc de graines X, avant de construire ces boules une par une ?
Ce n'est ni le filtre citron33 ni le port des couches q3 différé après B.

Voici une identité vérifiée algébriquement côté constructeur, mais encore
sans primitive de bornes, oracle natif, parcours ou résultat de coût.
Poser d=b−a, D=|d|², w=2x−a−b, v=2z−a−b :

$$J=D|w|^2-(d\cdot w)^2,\qquad P=Dw-(d\cdot w)d,\qquad q_x=|w|^2-D,\qquad q_z=|v|^2-D.$$

Pour J>0 (triangle non collinéaire), le centre dans son plan de la boule
circonscrite est :

$$c=\frac{a+b}{2}+\frac{q_xP}{4J},\qquad F=Jq_z-q_x(P\cdot v)=4J\bigl(|z-c|^2-R^2\bigr).$$

L'identité vaut même pour un triangle obtus, mais l'émission q3 exige
toujours une graine strictement aiguë et propriétaire. L'acuité équivaut
ici à qx>0 et −D<d·w<D. Une boîte ambiguë ne peut pas être rejetée.
Un majorant F<0 sur X×Z fournirait des intérieurs stricts communs,
au seuil K−1 ; un minorant F≥0 exclurait Z du COMPTE seulement. Les
contacts F=0 restent nécessaires à la collecte finale des coquilles.
Les endpoints et z=x donnent F=0 : ne pas retirer tout X des témoins
comme si chaque site en était la seed, ni créditer un bloc diagonal.

Une enveloppe grossière donne |J|≤36M⁴, |qx|,|qz|≤12M²,
|P_i|≤12M³, |P·v|≤72M⁴ puis |F|≤1296M⁶<2¹⁰⁷. Elle suppose de
borner les carrés comme des carrés NON NÉGATIFS (et non de multiplier
naïvement deux intervalles dépendants), avec promotions avant tous les
produits de degré élevé. Sa précision et son coût restent inconnus.
Pour x fixé, F est convexe séparable en v : les huit coins de Z suffisent
pour un majorant, PAS pour un minorant. Ce choix peut payer huit calculs
au lieu d'un ; ce n'est pas une amélioration acquise.

Le partage demanderait encore un compte et un préfixe Z classifié/disjoint
par tâche. Scinder X conserve les deux ; une feuille Z ambiguë impose
split X ou relais AVANT consommation. Un relais repartant de zéro ne
reçoit pas ces crédits. Cette piste et les produits q4 de la note dédiée
doivent viser les postes restants avant de raffiner davantage le filtre33.

Les contrats restent toute la tour K1..10 à50k sous1s sur G4 (puis100ms,
repliK1..5), et plusieurs dizaines de millions de points. La présente
tranche de candidats ne les remplit pas. GCP non utilisé dans33.

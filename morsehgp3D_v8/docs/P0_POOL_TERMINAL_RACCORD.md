# Proposition historique : Pool sur les gros rectangles terminaux

14 septembre 2026, actualisé après fbbecc01. **Proposition historique et
preuves indépendantes ; [port produit qualifié localement](P0_POOL_TERMINAL_Q2.md).** Cadre
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=implementation_v8_p0`,
`public_status=not_claimed`. Le Pool P0 existe ; le raccord décrit ici
est désormais implémenté avec ses propres 49 CTests et 80 mesures.
La suite de cette note conserve les propositions et résultats historiques
des auditeurs, sans les transformer en reçus du port. L'auditeur A en a
publié un prototype indépendant, distinct de la tranche conjointe.
Cette note ne modifie aucune de leurs sources ni les
audits indépendants. GCP non utilisé.

## 1. Deux preuves distinctes : sélectivité puis chaîne q2 complète

### 1.1. Audit B : front trois voies et candidates, avant census

L'[audit B publié avec e6388e55](../audits/CREDITS_TERMINAUX_20260914.md)
utilise les sources produit **da366f7f**, pas le nouveau parcours conjoint.
Son [reçu CREDITS_TERMINAUX_CHECKS.json](../audits/credits_terminaux_20260914/CREDITS_TERMINAUX_CHECKS.json)
conserve les commandes et sorties des harnais. Pour `front_pool`, le bras
ci-dessous emploie `clusters`, seed 3, Kmax=10, s=8, `MidpointSamples`,
les **trois voies**, et Pool lorsque `max(|A|,|B|) >= 64`.

| n | Résidu q2 avant Pool | Résidu q2 après Pool | Résidu q2 des 28 gros rectangles après Pool | `credit_ms` | `front_total_ms` |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 29 728 389 | 1 743 978 | 11 329 | 59,5 | 2 332,9 |
| 16 000 | 116 753 516 | 4 786 336 | 29 878 | 122,1 | 7 235,5 |
| 32 000 | 460 078 566 | 12 184 712 | 102 993 | 241,3 | 20 639,5 |

Ces nombres proviennent des lignes `front_pool ... 64 pool samples` du
reçu. Le total résiduel croît de ×2,74 puis ×2,55 aux deux doublements
observés. À 32k, les 28 gros produits passent de 447 996 847 à 102 993
candidates q2 ; l'essentiel du résidu restant appartient aux petits
rectangles. Ce résultat motive fortement un raccord, sans constituer
une borne asymptotique générale.

Le titre de l'audit, « Le régime amas du front v8 se ferme », est plus
fort que la qualification contenue dans cet audit B. Le
[harnais front_pool.cpp](../audits/credits_terminaux_20260914/front_pool.cpp)
additionne `plan.candidate_pairs()` : il n'exécute **ni census des
survivantes, ni collecte, ni comparaison des supports finaux**.
`front_total_ms` inclut front et callback de crédits, pas cette suite.
`credit_ms` inclut la copie des facteurs et leur nouvelle préparation ;
il est arrêté avant la destruction du rectangle et du tampon local, qui
reste comprise dans le temps englobant du front. Les temps isolés de
[clusters_credits.cpp](../audits/credits_terminaux_20260914/clusters_credits.cpp)
séparent encore préparation du rectangle et construction du batch.

La note initiale à e6388e55 donnait 85 ms à 8k et 1,8 s de surcoût au
seuil 2 ; le reçu lié donne 59,5 ms et 1 097,4 ms de crédits au seuil 2,
soit environ 1 038 ms de plus qu'au seuil 64. B a corrigé ces durées
dans sa publication e931d8f6. Cette proposition cite le reçu et conserve
la distinction des phases ; les temps restent indicatifs sur hôte partagé.

La comparaison produit devra être **q2 seule**, sur les mêmes sources
que son bras témoin, avec génération, préparation globale, front,
filtre, census et sorties payés. Les 241,3 ms ne sont ni un temps q2 seul, ni
un gain mesuré contre les anciennes centaines de secondes du census.
À 32k, Pool laisse encore 32,03 M et 47,75 M candidates q3/q4 dans les
gros produits ; leur aval n'est pas qualifié par ce harnais. Aucun
résultat de tour FULL, de contrat 50k sur G4 ou de régime multi-millions
ne découle de ces chiffres. Les juges historiques du Pool ne qualifient
pas automatiquement le nouvel adaptateur.

### 1.2. Audit A : Pool et census global réellement raccordés

Le [prototype A publié avec fbbecc01](../audits/q2_pool_bridge_20260914/README.md)
utilise les sources **e3af11a7**. Il paie toute la chaîne q2, avec un
propriétaire et un index globaux, sans recopier les coordonnées par
rectangle. Sa couture inclut le census inchangé pour accéder à son
moteur interne ; ce n'est pas une nouvelle API produit.

Les bras appariés emploient Samples, Kmax10 et, ci-dessous, s8. La
baseline est Complement/sibling. Pool s'applique au seuil 64 ; les
petits rectangles gardent la baseline. Les valeurs suivantes sont les
**temps totaux** du premier passage, en secondes, repris du README A
et de ses campagnes closes [amas](../audits/q2_pool_bridge_20260914/campaign_clusters/COMPLETION.json)
et [LiDAR50k](../audits/q2_pool_bridge_20260914/campaign_check50k/COMPLETION.json).

| Entrée | Baseline | Pool/paires | Pool/partagé |
| --- | ---: | ---: | ---: |
| Amas 8k | 13,249 | 3,062 | 3,134 |
| Amas 16k | 53,761 | 8,504 | 8,449 |
| Amas 32k | 204,690 | 21,901 | 21,973 |
| LiDAR scan0, 50k | 13,175 | 10,899 | 10,929 |

La [répétition LiDAR50k en ordre inversé](../audits/q2_pool_bridge_20260914/campaign_repeat50k/COMPLETION.json)
donne 13,080 / 10,548 / 10,722 s. Pool/paires retire 17,3 % puis 19,4 %
du temps dans ces observations ; le bras partagé n'ajoute pas de gain
temporel stable. Le gain Pool/paires sur les amas est donc maintenant
mesuré avec census et supports, pas seulement déduit d'un résidu.
Sur les pilotes LiDAR 8k, les signes temporels varient : ne pas convertir
ces résultats en gain universel.

Le travail et le coût sélectionnés sont eux aussi explicités par A :

| Entrée | Facteurs relus F | Candidates sélectionnées après Pool | Préparation Pool/paires | Callback sélectionné, census et sortie inclus |
| --- | ---: | ---: | ---: | ---: |
| Amas 8k | 56 000 | 11 329 | 3,71 ms | 26,80 ms |
| Amas 16k | 112 000 | 29 688 | 8,30 ms | 64,07 ms |
| Amas 32k | 224 000 | 102 336 | 15,18 ms | 190,19 ms |
| LiDAR50k | 215 776 | 109 063 | 17,38 ms | 90,18 ms |

Le départage par ID original explique les faibles différences de
candidates avec B à 16k/32k ; ce n'est pas une différence de supports
finaux. Les résidus totaux des amas valent ici 1 743 978 / 4 786 146 /
12 184 055. Les doublements des visites du census passent de
×4,106 / ×4,229 à ×2,958 / ×2,701 en Pool/paires. Cette amélioration
observée ne devient pas une borne sous-quadratique générale.

**Nouvelle priorité après le port : front et nombreux petits rectangles.**
Après filtrage, les callbacks sélectionnés pèsent moins de 1 % du total
sur LiDAR50k et amas32k. Raffiner encore leurs survivantes ne peut pas
expliquer l'essentiel du temps restant. Les sorties hors de ces
callbacks doivent rester mesurées, sans attribuer tout le résidu de
temps au seul front. Les temps de préparation, callback sélectionné
et payload se recouvrent ; ils ne s'additionnent pas.

Selon la [clôture A](../audits/q2_pool_bridge_20260914/r1_VALIDATION.json),
les 36 appels/33 configurations sont clos, avec douze anciennes
baselines identiques et les contrôles de reçus. Son gate r2 compare
840 flux à un oracle indépendant sur dix nuages, en Release et
Clang ASan/UBSan ; les deux pièges géométriques annoncés sont des
contre-modèles, pas des mutants du binaire produit. Les grandes
instances sont appariées par leurs empreintes, non par un oracle
exhaustif à cette taille. Un seul CPU fixé par affinité, hôte partagé,
pas de distribution statistique : cette preuve d'audit ne qualifie ni
le futur port produit, ni FULL/G4, ni une exécution asynchrone massive.

## 2. Preuve du rejet par h_a + h_b

La première version proposée concerne q2, avec seuil K=Kmax et **sans
cœur préchargé**. Pour un rectangle terminal, A et B sont deux populations
disjointes du même nuage immuable. Le
[Pool existant](../src/pipeline/local_credits.cpp) propose, dans chaque
facteur, jusqu'à K+1 sites de projection élevée vers le facteur opposé.
Ces projections choisissent des propositions ; seule la certification
géométrique donne un crédit. Un échec de recherche ne supprime rien.

Pour chaque a de A, soit W_A(a) l'ensemble des propositions distinctes de
A, différentes de a, certifiées strictement intérieures à toutes les
boules diamétrales (a,b), b dans B. Définir symétriquement W_B(b), avec
des témoins dans B et différents de b. Le test porte sur toute la boîte
du facteur opposé ; il couvre donc tous ses sites. Poser h_a et h_b les
cardinaux certifiés, saturés à K. Noter X le nuage global.

$$H(a,b,z)=(z-a)\cdot(b-z),\qquad D(a,b)=\left|\left\lbrace z\in X:H(a,b,z)>0\right\rbrace\right|,\qquad D(a,b)\geq h_a(a)+h_b(b).$$

Les populations de témoins des deux termes sont disjointes parce que
A et B le sont. Chaque proposition est un ID réel, compté une seule
fois dans son ensemble ; l'ancre propre est exclue. La positivité
**stricte** exclut aussi toute extrémité ou tout point de coquille.
Ainsi `h_a(a)+h_b(b) >= K` suffit pour rejeter la paire. Sinon elle est
conservée : le minorant n'est pas son compte exact d'intérieurs.

Regrouper les rangs par crédits croissants conserve les seuls produits
tels que i+j<K. Pour chaque classe A_i, tous les j admissibles sont
consécutifs depuis zéro : ils forment **un seul préfixe B** de longueur
N(i), somme des cardinaux des classes j<K−i. Émettre alors la bande
A_i×B_order[0:N(i)], si elle est non vide. Il y a au plus **K bandes**,
soit dix pour K=10, et non 55 descripteurs nécessaires. L'ancienne
représentation par K(K+1)/2 couples de classes reste correcte, mais
le format plus compact réalisé par A est celui à porter.

Les préfixes B peuvent se recouvrir : les bandes de paires sont néanmoins
disjointes parce que les classes A_i le sont. Leur union est exactement
le résidu du critère i+j<K. Les classes saturées K ne produisent aucune
candidate ; masses et bornes de préfixes se calculent sans développer
A×B. Le nombre K+1 de propositions n'est pas un plafond sur le résidu :
toutes les paires indécises restent à traiter.

## 3. Une vue indexée, pas un nouveau nuage par rectangle

Le harnais B copie A puis B dans `RectangleInput`, puis
`prepare_rectangle` appelle de nouveau `prepare_cloud` : copie privée,
tri d'unicité et arbre des plages. Reproduire cela sur le chemin produit
réintroduirait une préparation globale locale à chaque rectangle.

Le prototype A a réalisé une vue indexée de ce type. Son port produit
doit retenir ou emprunter pendant tout l'appel :

- le propriétaire global et **l'index précis** qui fournit sa permutation ;
- les deux handles de nœuds WSPD, leurs plages spatiales et leurs boîtes
  certifiées, non vides et disjointes ;
- le seuil, la voie q2 et l'identité du rectangle parent ;
- les crédits par position de facteur et les permutations de regroupement
  dans un stockage privé stable.

Le [RectangleSpec actuel](../src/pipeline/local_credits.hpp) exprime des
plages d'**IDs originaux**. Un intervalle du front est au contraire une
plage de **rangs spatiaux**. Il est interdit de fournir directement le
second au premier, même si les bornes numériques sont valides.

Une représentation simple conserve les rangs spatiaux dans les groupes
de crédit, puis utilise `spatial_order[rank]` pour accéder aux points et
émettre leurs IDs originaux. Elle ne nécessite ni copie de coordonnées
ni permutation inverse reconstruite par paire. Le départage des
projections égales doit être explicite, par exemple par ID original.
Le harnais B utilise des IDs locaux renumérotés : ses candidates ne sont
pas automatiquement identiques si le nouveau départage diffère. Les
supports finaux complets doivent, eux, rester identiques.

Une même boîte, les mêmes coordonnées ou un nuage commun ne suffisent
pas pour transférer un plan : propriétaire, rectangle, seuil et ordre
restent distincts, comme l'établit le
[contrat de partage global](P0_NUAGE_ET_INDEX_PARTAGES.md#trois-identités-différentes-à-ne-pas-confondre).
Une future exécution asynchrone devra posséder son contexte jusqu'à la
fin des jobs ; un plan affectable emprunté ne peut être déplacé ou
modifié pendant leur exécution.

## 4. Raccord q2 minimal et politique de taille

Dans le callback du front q2, une politique explicite peut choisir Pool
quand `max(|A|,|B|) >= T`, avec T=64 comme **premier essai**, pas comme
optimum acquis. Les autres rectangles conservent le census courant.
Pool désactivé et plusieurs seuils doivent rester comparables. Le choix
de T modifie du travail, jamais la couverture : un rectangle non filtré
est traité intégralement ; une paire non certifiée reste dans le résidu.

Pour les rectangles filtrés, préparer le plan une fois, puis développer
**seulement ses bandes résiduelles**. Le premier raccord à porter envoie
ces paires à la primitive de census individuel sur l'index global et
collecte tous leurs intérieurs et coquilles. La comparaison A confirme
la priorité de ce bras Pool/paires : aucun arbre B ni préparation Axis
n'est nécessaire pour obtenir le gain mesuré. Tester
`keeps(a,b)` sur toutes les paires du produit initial est au contraire
interdit sur ce chemin : cela restaurerait l'expansion quadratique que
le regroupement doit éviter.

Le census d'une survivante repart de zéro, avec son parcours global
complet. **Ne pas précharger h_a+h_b**, ni diminuer K tout en recomptant
les mêmes témoins dans Z. Les sites extérieurs à A∪B restent également
dans le census ; le nuage local du harnais n'est pas son domaine de
recherche. La coquille conserve notamment les deux extrémités.

Les groupes triés par crédit ne sont pas des sous-arbres de l'index
spatial. Le bras partagé d'A les représente par un arbre B local construit
sur le plus long préfixe utile, avec couvertures calculées une fois par
classe A. Son B original différé reste un vrai nœud global ; sa permutation
locale sert uniquement aux requêtes. Le certificat frère est désactivé
sur cet arbre local, car le lookup produit attend un nœud global.
Ces distinctions sont obligatoires pour un éventuel port du bras
partagé ; ne pas attribuer des handles spatiaux fictifs aux préfixes.

Si P est la masse q2 livrée par le front, R_P la masse rejetée par Pool
et C celle transmise au census, vérifier `P = R_P + C`, puis
`C = accepted + rejected_by_census`. La somme de toutes les étapes de
rejet et des supports retenus doit retrouver les paires initiales du
front, sans doublon ni perte.

## 5. Partager le parent avant de partager ses jobs

Le [§9.5 de l'audit A](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
donne le bon découpage : répartir les rangs d'ancres du **plan parent**,
conserver ses crédits et son ordre B, puis émettre exactement son résidu
dans chaque job. Ne pas reconstruire Pool sur chaque A_i×B : B serait
rescanné, et les témoins de A hors de A_i pourraient disparaître du
proposeur alors qu'ils restent géométriquement valables.

Une véritable restriction à A'×B' exige une relation certifiée avec le
parent et le même seuil. Le résidu parental restreint peut être
intersecté avec un nouveau résidu certifié. Cela prend le maximum de
deux minorants totaux, **pas leur somme** : les témoins peuvent se
recouvrir. Le simple partage du nuage ne remplace pas cette preuve.

Les tâches ainsi définies sont distribuables par handles et plages,
sans copier B par job. Cela ne prouve ni leur équilibrage ni un gain
multi-CPU/GPU ; les paires survivantes peuvent avoir des coûts différents.

## 6. Travail total, mémoire et risque de carré déplacé

Noter R le nombre de rectangles effectivement filtrés et
F=Σ_r(|A_r|+|B_r|). La sélection bornée et les certifications Pool
coûtent O(KF), puis le regroupement en au plus K bandes par rectangle
O(F+KR), avec mémoire locale O(|A_r|+|B_r|+K) par plan vivant.
Ce sont les coûts du filtre, pas ceux
de toute la chaîne. Préparer des plans pour chaque job au lieu d'un
parent unique pourrait déjà rendre ce coût quadratique.

Le bras partagé facultatif ajoute, pour les plus longs préfixes utiles
m_r, O(Σ_r m_r) de construction d'arbres, O(Σ_r K log(1+m_r)) pour
leurs couvertures mises en cache et au plus O(Σ_r |A_r| log(1+m_r))
d'activations. A mesure ces coûts ; ils ne disparaissent pas derrière
le nombre réduit de racines. Les visites Z et toutes les sorties
restent supplémentaires. Les capacités vectorielles de plans et
d'arbres rapportées par A ne sont pas un pic RSS ou VRAM.

Sur les 28 paires complètes des huit amas observés, chaque amas intervient
sept fois : F=7n pour ces produits. C'est une propriété favorable de
cette famille. La disjonction des paires d'une WSPD ne démontre pas à
elle seule que F reste sous-quadratique sur tous les nuages. Le terme
Ω(R|B|) des partitions A_i×B est déjà documenté dans les
[limites du partage global](P0_NUAGE_ET_INDEX_PARTAGES.md#ce-qui-reste-coûteux--et-peut-encore-être-quadratique).
Aucune borne générale O(s³n) n'est attribuée à l'arbre actuel.

Les mesures doivent séparer :

- préparation du nuage et de l'index, une seule fois ;
- travail du front, R et F, tailles et masses des rectangles filtrés ;
- projections, comparaisons de sélection, propositions retenues et tests
  géométriques, regroupement, allocations et octets copiés ;
- descripteurs de classes, jobs et candidates réellement développées ;
- visites du census, rejets individuels, supports retenus, tailles
  d'intérieurs et de coquilles, copie/tri/hash du callback ;
- temps englobant et destructions, mémoire maximale des plans/jobs et
  des buffers, distincte des seules capacités conservées.

Payer O(C) pour développer les survivantes ne borne ni C ni le coût de
leurs recherches Z sous-quadratiquement. Aucun temps de composant, ni
une baisse du résidu par threads supplémentaires, ne clôt P0. À 32k,
DualBlocks diminue encore le résidu des gros rectangles, mais ses
5 477,8 ms de crédits ne sont pas un gain établi contre les 241,3 ms de
Pool : le coût aval évité reste à mesurer, séparément pour chaque voie.

## 7. Contre-fixtures et porte de décision

La qualification du raccord devra notamment couvrir :

1. **Addition locale valide.** Sites collinéaires d'abscisses 0,1,99,100,
   A={0,1}, B={99,100}, K=2. La paire (0,100) reçoit un témoin de chaque
   facteur ; ils sont distincts et sa profondeur vaut deux.
2. **Crédit préchargé interdit.** Sites 0,1,100, A={0,1}, B={100}, K=2.
   Pour (0,100), le minorant et la profondeur valent un. Précharger un
   puis recompter le site 1 supprimerait un support admissible.
3. **Parent conservé.** Même triplet à K=1 : le Pool parent garde seulement
   (1,100). Ses jobs doivent garder ce même résidu ; reconstruire les
   facteurs A singleton perd le crédit utile pour (0,100).
4. **Mauvais propriétaire, rang ou seuil.** Nuage copié identique mais
   propriétaire différent, permutation non identité, mauvais rectangle
   opposé et K différent : rejeter les transferts invalides, vérifier le
   round-trip rang spatial → ID original, puis les supports canoniques.
   La fixture non colinéaire publiée par A rend l'erreur concrète :
   IDs 0=(100,0,0), 1=(0,1,0), 2=(1,0,1), A={0}, B={1,2}.
   L'ordre spatial est [1,2,0], mais Pool ordonne B en [2,1]. À K1,
   le préfixe d'un élément doit conserver (0,2), pas (0,1) ; le site 2
   donne H=98 pour cette dernière. À K2, le précrédit suivi de son
   recomptage la rejetterait à tort.
5. **Seuil strict et coquille.** a=(1000,0,0), b=(0,1,0), z=(0,0,0),
   A={a}, B={b,z}, K=1 : z est en coquille pour (a,b), pas un crédit.
   L'égalité du test ne doit pas supprimer ce support. Cette partition
   passe s8/10/12 ; un seuil de taille T=2 permet d'exercer le filtre.
6. **Census global.** a=(0,0,0), b=(100,0,0), z=(50,0,0), facteurs
   singleton {a},{b}, K=1 : un census limité aux facteurs manque z.
   Ajouter aussi une grande coquille, non plafonnée par K.
7. **Projections égales et couverture.** Propositions sans IDs répétés,
   ancre propre exclue, classes vides et saturées, préfixes sans candidate,
   seuil de taille évitant tous les rectangles ou en traitant davantage :
   chaque bras doit donner exactement les mêmes supports complets.
   Précision au port : sans cœur extérieur, un plan Pool q2 complet ne
   peut être vide. La paire croisée de distance minimale n'a aucun témoin
   local strict, sinon ce témoin fournirait une paire croisée plus courte.
   Ne pas imposer de plancher positif à une branche géométriquement impossible.
8. **Coût répété.** R fixe puis R croissant avec n, facteurs partagés,
   jobs parentaux versus préparations indépendantes : compter F et les
   rescans, pas seulement les candidates finales.

Les petits nuages exigent un oracle indépendant vérifiant toutes les
paires supprimées et tous les payloads retenus, ainsi que des mutants
de couverture, d'identité et de double crédit. Les sources et leurs
fermetures, les commandes, entrées, graines, sorties brutes et échecs
doivent être conservés ; ni da366f7f chez B ni e3af11a7 chez A ne
deviennent automatiquement les pins du futur produit.

Comparer ensuite les bras q2 sans Pool et avec Pool sur n=8 000,
16 000 et 32 000, s=8/10/12, K=5/10, mêmes graines et même front.
Inclure amas, uniforme, terrain et rangées, ainsi que les entrées LiDAR
appariées à 50k : A montre qu'un pilote 8k seul peut manquer le régime
utile. Ses campagnes couvrent K10, amas/s8 et LiDAR/s8 avec s10/12 à
8k, pas toute cette matrice. Juger préparation, descripteurs, résidu,
census, payload et mémoire ensemble. Le prochain succès produit attendu
est de reproduire le gain q2 complet du prototype ; ensuite, prioriser
front, petits rectangles et sorties restantes. Les contrats de tour
50k et de nuages massifs sur G4 restent des qualifications distinctes,
encore ouvertes pour cette proposition.

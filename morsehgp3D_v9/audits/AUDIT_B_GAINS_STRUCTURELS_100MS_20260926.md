# B — supprimer du travail avant de viser 100 ms

26 septembre 2026. Base produit `1e0674f7a`, héritant du moteur R22/v28.
`phase=exploration_v9_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_prototypes_100ms`,
`public_status=not_claimed`. Prototypes isolés, pas de modification des
défauts produit. Le WIP v29 du développeur reste distinct. Cette note
complète, sans remplacer, le [plan GPU du 24 septembre](ARCHITECTURE_GPU_100MS_Q34_FULL_20260924.md).

## Ce qui doit effectivement disparaître

Le [reçu R24-B](../receipts/g4_tower_r24b_20260926/README.md),
cas `probe_0`, mesure 916,304 ms de chaîne FULL K5 sans sol sur
08/000000 : q3/q4 481,870 ms, census 100,273 ms, tour 282,724 ms.
Le mur du processus vaut 1,726 s ; ce n'est pas une latence de service
résident. Les coûts superposés, notamment q2, ne s'additionnent pas à
l'aveugle. Les trois noyaux S2/S3/S4 prennent déjà environ 212 ms en
série : accélérer seulement les transferts ne permet pas 100 ms FULL.

Trois postes appellent des changements de représentation ou de travail :

| Poste | Travail supprimable ou transférable | Ce qui reste obligatoire |
| --- | --- | --- |
| Front CPU, 98,598 ms | Transformer la récursion en lots de produits indépendants sur appareil | Même partition, mêmes témoins stricts, mêmes masques et comptes |
| Nouveau census des clés q3/q4, compris dans les 100,273 ms | Réutiliser les IDs intérieurs du census du générateur au lieu de rechercher la même boule depuis la racine | Clé/niveau exacts, support positif, propriété du nuage, coquille complète, construction du catalogue |
| Tour FULL, 282,724 ms | Réduire les structures temporaires de tous les ordres, puis construire les événements/composantes en lots | Tous les nœuds, parents, multifusions et liens verticaux explicités avant la fin du chrono |

Les travaux du développeur sur les lots compacts de FULL et ses mesures
résidentes ne sont pas dupliqués ici. Le palier numérique S2 décrit plus
bas est une expérience complémentaire, pas une explication du facteur neuf
encore manquant. Les gains de ces pistes ne sont pas acquis ni additifs.

## 1. Ne pas jeter les identités déjà comptées

`gpu/lanes.hpp` compte les intérieurs de chaque candidat accepté, mais
`LaneRecord` ne garde que leur nombre. Le catalogue reçoit ensuite une
`Presentation` contenant clé/support/profondeur/coquille ; `census_key`
redescend l'index global pour retrouver les IDs perdus. Sur `probe_0`,
849 780 records GPU deviennent 849 777 clés tardives. Seulement trois de
ces clés ont une coquille étendue. Le chemin régulier est donc une cible
concrète, sans changer la définition de la tour.

Une coquille de cardinal q est exactement le support de q IDs distincts
déjà prouvés sur la sphère. Cela utilise **le cardinal global certifié**,
pas une empreinte de coquille, et ne vaut pas pour une coquille étendue.
De même, d IDs distincts strictement intérieurs forment tout l'intérieur
si le producteur a déjà certifié globalement que sa cardinalité est d.

### Pourquoi le census du cover peut être global

Soit un support positif de q points de diamètre L, de centre c, de rayon R
et de poids barycentriques positifs de somme 1. L'identité
$R^2=\frac{1}{2}\sum_{i,j}\lambda_i\lambda_j\lVert p_i-p_j\rVert^2$ implique
$R^2\leq\frac{q-1}{2q}L^2$. Pour une arête propriétaire ab de longueur L et
son milieu m, $\lVert c-m\rVert^2=R^2-L^2/4$. Ainsi
$R+\lVert c-m\rVert\leq\sqrt{3}L/2$ à q3 et
$R+\lVert c-m\rVert\leq(\sqrt{6}+\sqrt{2})L/4<L$ à q4.
La boule fermée du cover, de centre m et de rayon L, contient donc
l'intérieur **et toute la coquille**. Le rejet L11 est strict et conserve
les contacts. Cette preuve ne s'étend pas à un support non positif ni à
une arête qui n'est pas de longueur maximale.

### Paquet à transporter et coût à payer

À K5, une émission q3 porte au plus trois IDs intérieurs, une émission q4
au plus deux (seuils stricts K−1 et K−2). Le maximum brut supplémentaire
sur `probe_0` est 9 563 376 octets d'IDs, hors index d'arène/alignement,
face à 108 771 840 octets de records actuels. Élargir chaque record à sa
capacité K10 serait beaucoup plus cher ; mesurer des IDs compacts par
arène possédée. Le tri q4 réordonne ses records : conserver l'association
record/paquet, pas un index implicite dans une slab réutilisée.

Pour q3, les masques `inside` déjà calculés suffisent à extraire les IDs.
Pour q4, le balayage conserve des comptes de lentilles et abandonne leurs
IDs. Un premier port peut recollecter dans le cover **à l'émission** ; ce
coût doit apparaître dans le chrono. Puis conserver les IDs des lentilles
vivantes et ceux des événements intérieurs éviterait cette passe. Ajouter
huit collectes à toutes les graines, y compris rejetées, peut régresser :
ce n'est pas un raccourci gratuit.

L'import doit rester interne et lié au propriétaire immuable, à la clé,
au support et à la provenance du census complet. Il vérifie les domaines,
l'unicité et les signes exacts des IDs, reconstruit clé/niveau/positivité,
convertit ID original en rang Morton par un inverse construit **une fois**,
et conserve les contrôles Euler et les objets `BallData` actuels. Les
coquilles étendues et producteurs sans paquet gardent le census global.
Un simple booléen public « certifié » n'est pas une autorité. En particulier,
une liste tronquée accompagnée d'un faux compte plus petit ne peut pas être
réfutée par ses seuls signes : la cardinalité complète doit venir du
producteur, pas de la liste elle-même.

Le sceau actuel mentionne un recoupement par census dans le processus.
Après port, distinguer explicitement validation locale et preuve de
complétude du producteur ; ne pas laisser croire à un second recensement
indépendant. Garder ce second census comme juge différentiel activable.

### Complexité et porte de décision

À K fixé, stockage et validation du paquet régulier coûtent O(K) par
émission ; avec vérification d'unicité par comparaisons locales, O(K²)
reste borné à K≤10. L'inverse global coûte O(n). Aucun nouveau scan du
nuage par boule. Cela ne borne pas le nombre d'émissions ; les postes
producteur et coquilles étendues doivent rester dans la mesure totale.
Le prototype consommateur est dans [b_census_payload_20260926/](b_census_payload_20260926/).
Son chrono ne doit jamais être présenté comme un gain net de la chaîne :
il ne paie pas encore le transport GPU ni la collecte q4.

**Résultat local clos.** Sur 8k/16k/32k sites synthétiques à O(n) paquets,
l'import + reconstruction prend 0,352/0,710/1,344 ms contre
1,237/2,592/4,959 ms pour recensus + reconstruction, médianes de cinq essais
mono-thread. Rapport consommateur ×3,52 à ×3,69 ; 276 comparaisons exactes,
34 refus et repli coquille étendue passent en Release et ASan/UBSan.
La falsification simultanée du compte et de la liste est explicitement
montrée comme indétectable localement, d'où l'obligation de provenance.
Ce résultat n'est ni LiDAR ni un gain de tour.

## 2. Un front de produits en lots, pas un thread CPU par sous-arbre

Le front q3/q4 manipule une tâche `(A,B,masque,profondeur)`. Un pas produit
zéro, deux ou trois enfants, ou un rectangle terminal. La fonction actuelle
`Front::expand` porte déjà cette sémantique ; les témoins q2 hérités ne sont
pas actifs sur q3/q4. La sonde [b_front_waves_20260926/](b_front_waves_20260926/)
exerce ce même pas dans un ordre par vagues et le compare au DFS.

Pour l'appareil : compter une fois les enfants, faire un préfixe, disperser
les descripteurs possédés. Ne pas refaire le test géométrique lors de la
dispersion. Les enfants partitionnent le produit parent ; ils héritent du
masque, jamais d'un compte partiel sans identités. Un terminal déjà compté
ne passe pas une deuxième fois dans `expand`. Les tableaux de rectangles
peuvent être consommés par S2 sur appareil, sans aller-retour hôte imposé.

Le travail géométrique reste O(T·(D+K)+R), où T est le nombre de produits
visités, D la profondeur de l'index et R le nombre de terminaux. Le plan
de tâches ajoute O(T+R) de travail, mais une vague peut être large : mesurer
son pic et les capacités des deux tampons, pas la seule pile DFS historique.
Ni l'exposant de T ni le caractère sous-quadratique global ne changent par
ce seul ordonnancement. Une saturation de mémoire est un refus/repli
explicite, jamais une troncature réussie.

La recherche binaire actuelle du rectangle propriétaire de **chaque paire**
S2 est aussi évitable en partie : des tuiles de paires peuvent partager
l'adresse du rectangle. Avant d'implémenter ce chemin CUDA, mesurer combien
de tuiles traversent plusieurs petits rectangles ; une mauvaise occupation
des warps peut annuler le gain. Conserver le même ordre ligne par ligne et
les mêmes masques évite un nouveau problème de propriété.

**Résultat local clos.** 504 fixtures dans quatre configurations
(72/16 octets, Release/ASan-UBSan), quatre mutants tués par configuration,
neuf cas 8k/16k/32k et la trame entière 00 à s8/10/12 conservent chaque
rectangle/masque et tous les compteurs. Le descripteur spécifique q3/q4
passe de 72 à **16 octets**, sans liste q2 inutile. À uniforme32k, la
réserve des tâches passe de 1,264 Go à 281 Mo ; à LiDAR00/s8, elle vaut
38,6 Mo pour 43 vagues. Aucun gain mono : les vagues compactes restent
6–8 % plus lentes que DFS sur cette trame. L'acquis est le plan de travail
et son format, pas son débit GPU.

Le code actuel `Front` écrit ses compteurs partagés : il n'est **pas**
thread-safe. Le port doit séparer le pas géométrique des compteurs locaux,
puis réduire sommes et maxima. Sur le front brut LiDAR, 87/84/81 % des
tuiles32 sont contenues dans un rectangle à s8/10/12 ; ces taux sont à
recalculer **après** S2. Ils sont très faibles sur l'uniforme synthétique.

## 3. S2 : palier entier 64 bits avec repli exact

La piste isolée [b_s2_narrow_20260926/](b_s2_narrow_20260926/) conserve les
bornes affines des paires, leurs signes stricts et l'ordre du DFS. Si
$0<H_{max,4}\leq2^{30}$ et les six extrêmes des composantes croisées sont
dans $[-2^{28},2^{28}]$, alors les deux membres des comparaisons satisfont
$16\Xi\leq3\cdot2^{60}<2^{62}$ et
$\alpha H_4^2\leq3\cdot2^{60}<2^{62}$, pour α=2 ou 3.
Les calculs concernés tiennent dans un entier signé 64 bits ; sinon reprendre
les 128 bits actuels. Un minimum H négatif n'est pas mis au carré pour
l'admission. Les rectangles généraux ne sont pas implicitement qualifiés
par cette garde de paires fixes.

L'intérêt dépend de la fréquence réelle du palier **et** du coût de sa
branche/pression registre sur GPU. Exactitude différentielle, couverture
des frontières et temps local ne prouvent pas un gain G4. Abandonner si le
temps total S2 augmente. Ce palier conserve le nombre de visites : aucun
nouvel argument sous-quadratique n'en découle.

**Résultat local clos.** 116 049 comparaisons numériques, plus 10 368
géométries extrêmes, passent en Release/ASan-UBSan. 631 113 requêtes S2
réelles échantillonnées sont comparées trois fois : mêmes masques et
visites. Le palier couvre 87,56 % des tests Xi sur LiDAR00/K5 et 90,67 %
à K10. Les chronos CPU ne montrent pas de gain exploitable ; coût GPU des
branches/registres non mesuré. Surtout, le résidu S2 synthétique des amas
fait ×3,977 puis ×3,987 au doublement 8k/16k/32k : quasi quadratique,
inchangé par cette arithmétique. Le terrain fait ×2,041 puis ×2,068.

## 4. S2 : témoins partagés par tuiles, sans chaîne séquentielle

Le [prototype tuilé](b_s2_tile_cache_20260926/README.md) apporte une piste
de réduction du travail, distincte du seul adressage par tuile. Le cache
CPU par ligne sait déjà réutiliser des témoins ; le GPU actuel ne le fait
pas et paie 1,111 milliard de visites par paire sur R24-B/00/K5.

Une première paire fournit une antichaîne de nœuds témoins ; les autres
paires d'une tuile de ligne d'au plus 32 éléments la **retestent**
indépendamment. Chaque voie non rejetée reprend la recherche globale à
zéro. Il n'y a ni mise à jour séquentielle de cache ni crédit partiel
transmis. Une trace contient au plus 2K−3 nœuds : chaque nœud apporte un
crédit effectif, les comptes saturent à K−1 et K−2.

**Résultat local clos.** 107 259 masques de requêtes LiDAR échantillonnées
sont égaux au témoin global. Sur 00/100/200 sans sol K5/s8, le total
« visites de recherche + tests géométriques de cache » est divisé par
1,93/1,56/1,65. À 00/K10/s8 : 1,51 ; à 00/K5/s10 et s12 : 1,80/1,74.
Les représentants et replis sont payés, chaque cas exerce des rejets
partiels. Ces ratios ne comptent pas les validations structurelles O(K²)
de l'API publique et ne sont ni un gain de temps ni une estimation sans
biais de toute la trame. La fixture x={0,10,1,5,6} tue le mutant qui
réutilise un rejet sans retester les nouveaux endpoints.

Port proposé : produire en masse les traces courtes, puis appliquer les
autres paires en masse avec cache possédé, ordre de sortie préservé.
Les coûts d'occupation, de petites tuiles, de stockage et de divergence
doivent être inclus. La nouvelle porte GPU exigera les mêmes masques,
pas les mêmes visites — celles-ci doivent justement diminuer. Ce cache
ne change pas le nombre de paires résiduelles, donc ne ferme pas à lui
seul le problème de croissance des amas.

## Suite de qualification

Les quatre prototypes sont jugés localement sur adversaires et
croissance 8k/16k/32k quand leur périmètre s'y prête. Les coupes spatiales
de LiDAR restent celles passant par le capteur ; pas de préfixe de 50k
substitué à une trame. Seul un port GPU intégré, comparé à son témoin avec
coûts producteurs, transferts et sortie FULL explicite, donnera un nouveau
chrono de contrat. À cette étape, aucun nouveau résultat G4 n'est attribué
à ces pistes et le contrat 100 ms reste ouvert.

Pour le développeur : commencer le transport q3 des IDs sans supprimer
le juge global, et préparer le front GPU au format16 avec compteurs privés.
Pour S2, mesurer cache tuilé et palier i64 en bras séparés avant leur
combinaison. En parallèle, poursuivre les lots compacts FULL de tous les
ordres : aucune de ces améliorations amont ne fait disparaître les
283 ms actuels de tour. Les reçus locaux et leurs lecteurs passent ;
GCP n'a pas été utilisé pour ces prototypes, afin de ne payer une session
qu'une fois un chemin CUDA comparatif effectivement prêt.

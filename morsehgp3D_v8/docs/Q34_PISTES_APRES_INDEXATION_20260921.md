# q3/q4 : pistes structurelles après l'indexation32

21 septembre2026. **Note mathématique et proposition de port**, hors des
206 sources gelées de la tranche32. Aucun moteur, défaut ou binaire modifié.
Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `public_status=not_claimed`.

Deux priorités complémentaires : éviter de chercher des témoins dans des
blocs qui ne peuvent pas en contenir, puis éliminer des graines q3 avant de
construire et compter leurs boules. Ni l'une ni l'autre ne démontre encore
une borne sous-quadratique globale ou le contrat50k/G4.

**Retour B reçu à la clôture (`74196a31`) :** les bornes Xi et l'héritage
des témoins sont confirmés mathématiquement. Le filtre q3 par couches
est également sûr, mais B déconseille son port sur ces LiDAR au vu de
son modèle de coût. La priorité concrète devient donc Xi, partage des
recherches puis travail q4 ; les couches q3 ci-dessous restent une piste
conditionnelle. Voir sa [réponse détaillée](../audits/DIALOGUE_AUDITEUR_B.md).

## 1. Pourquoi le filtre citron reste coûteux

La recherche32 partage le même index, mais exclut actuellement un bloc Z
seulement lorsque `Hmax<=0`. Cette condition concerne la boule diamètre,
plus grande que les citrons q3/q4. Un bloc qui rencontre cette boule mais
ne contient aucun témoin citron peut donc être raffiné inutilement jusqu'aux
feuilles. Une recherche qui n'atteint pas son seuil doit épuiser ces branches.

Le diagnostic de croissance transmis pendant la campagne32 signale, au
dernier doublement K5/s8, environ ×5,364 de tests H et ×7,440 de tests Xi
pour les recherches par paire, contre ×2,416 de tests H par rectangle.
Ces nombres motivent l'étude ; cette note ne les remplace pas par une
qualification close, n'en extrapole pas un temps et n'annonce aucun gain
de la proposition non implémentée.

La [comparaison close à8k](../receipts/q34_indexed_20260921/BENCH_8K_PAIR_RECTANGLE_SCALAR_BOXES.md)
montre déjà pourquoi il faut payer tous les postes : les rectangles baissent
les visites de témoins de151,391M à61,494M, mais les bornes du census q3
ne donnent pas un gain de temps général sous la charge concurrente mesurée.

## 2. Paires fixes : supprimer la dépendance répétée en Z

Noter `d=b-a`, `D=|d|²`, `u=z-a` et

$$H=u\cdot(d-u),\qquad \Xi=|u\times(d-u)|^2=|d\times(z-a)|^2.$$

La [borne générale](../src/spindle/predicates.hpp) utilise les intervalles
de `z-a` et `b-z` comme s'ils étaient indépendants. Pour a,b fixes, les
termes quadratiques en z s'annulent exactement : chaque composante

$$C_i(z)=d_j(z_k-a_k)-d_k(z_j-a_j).$$

est **affine**, avec seulement deux coordonnées de z. Ses extrema sur une
boîte sont exacts : choisir pour chaque coefficient l'extrémité basse ou
haute suivant son signe. Il n'est pas nécessaire de visiter les sites,
ni même d'énumérer les huit coins.

Si cette image est `[l_i,u_i]`, prendre

$$m_i=\begin{cases}0&l_i\le0\le u_i,\\ \min(l_i^2,u_i^2)&\text{sinon},\end{cases}\qquad M_i=\max(l_i^2,u_i^2),\qquad \Xi_{min}=\sum_i m_i,\quad\Xi_{max}=\sum_i M_i.$$

La somme donne des **bornes**, pas nécessairement les extrema exacts de Xi :
les trois extrema de composantes peuvent ne pas être atteints au même z.
Elle est néanmoins exacte aux singletons. Pour a,b fixes, chaque intervalle
affine exact est inclus dans l'intervalle général ; les bornes obtenues
sont donc au moins aussi serrées. Cela ne suffit pas à promettre un gain CPU.

Préparer aussi `a+b` et D donne directement

$$4H=D-|2z-a-b|^2.$$

Les distances maximale et minimale de `a+b` à la boîte doublée donnent les
deux extrema exacts continus de4H. La borne supérieure conserve les sommets
demi-entiers sans arrondi. La préparation conjointe32 calcule les mêmes
quantités, mais ses quatre choix d'extrémités A/B sont identiques pour une
paire singleton ; une préparation dédiée évite cette répétition.

### Deux décisions distinctes, strictement sûres

Poser `hmin4=4Hmin`, `hmax4=4Hmax`, avec alpha3=3 et alpha4=2.

- **Admettre tous les témoins du bloc** pour une voie si
  `hmin4>0` et `alpha*hmin4² > 16*Xi_max`.
- **Exclure tout le bloc de la recherche de témoins** pour cette voie si
  `hmax4<=0`, ou `alpha*max(0,hmax4)² <= 16*Xi_min`.
- Sinon la voie reste indécise : raffiner, sans quota de recherche.

La seconde preuve est directe : un témoin aurait H>0, donc
`alpha*(4H)² <= alpha*max(0,hmax4)² <= 16*Xi_min <= 16*Xi(z)`,
ce qui contredit la condition stricte du citron. **L'égalité autorise
l'exclusion**, jamais un crédit : toutes les tangences restent non témoins.

Exclure Z ne rejette surtout **pas** l'arête ou la voie globale : cela
apporte zéro crédit. Seul le masque local du cadre DFS perd ce bit ; le
masque global ne perd une voie qu'après accumulation de K−1/K−2 témoins
distincts. Le masque d'admission reste lui aussi local pour empêcher tout
double compte lors de la descente de l'autre voie. Les crédits ne sont
jamais transmis au census.

### Exemple causal sans mesure de performance

Prendre `a=(10,10,10)`, `b=(20,10,10)` et
`Z=[14,16]×[13,14]×{10}`. On obtient exactement H dans `[8,16]` :
le bloc entier est dans la boule diamètre et le rejet H actuel ne sert pas.
La composante non nulle de la croix a un intervalle de module `[30,40]` :

- nouvelle borne Xi : `[900,1600]` ;
- ancienne borne générale : `[576,2304]`.

La nouvelle borne exclut q3 et q4, car `3*16²=768<=900`.
La borne générale basse ne permet ici d'exclure que q4 :
`2*16²=512<=576`, mais `768>576`. Cet exemple établit une amélioration
géométrique stricte, pas un résultat sur le nombre de nœuds d'un index réel.

### Largeurs entières

Avec M=65535, `|d_i|,|z_i-a_i|<=M`, donc chaque produit est au plus M²
et `|C_i|<=2M²<2^33`. Coefficients, constantes affines et leurs évaluations
intermédiaires tiennent en i64 ; promouvoir avant toute multiplication.
Les carrés et les comparaisons utilisent i128 :

$$\Xi_{max}\le12M^4<2^{68},\quad 16\Xi_{max}\le192M^4<2^{72},\quad \alpha\max(0,hmax4)^2\le27M^4<2^{69}.$$

Ne pas calculer les carrés dans i64 puis convertir. Il n'y a ni division
flottante, racine carrée, puissance B² de clé de boule, ni entier multiprécis
nécessaire au produit. Le cas a=b est inerte et peut conserver le contrat32 :
Hmax<=0, aucun témoin.

## 3. Rectangles A×B et forme du port

Pour A/B non singletons, la simplification affine **ne permet pas** de
remplacer a et b par leurs milieux ou représentants : les coefficients
dépendent des deux points variables. En première version, conserver
`Q2JointPreparedBounds` et `spindle_detail::xi_bounds(A,B,Z)` ; son membre
`low`, déjà mathématiquement certifié, autorise le même rejet négatif.
C'est une condition suffisante forte : aucun triplet `(a,b,z)` du produit
ne donne de témoin. Elle peut échouer même quand aucun témoin universel
discret n'existe. Aucun scan des facteurs n'est nécessaire.

Pseudo-API de valeur préparée, **non implémentée**, à fixer au port :

```cpp
class PreparedPairCitronBounds {
 public:
  PreparedPairCitronBounds(Point3 a, Point3 b);
  Q2Bounds h_bounds(const Box3& z) const;       // Extrema exacts de 4H.
  XiBounds xi_bounds(const Box3& z) const;     // Deux bornes i128.
 private:
  // Constantes privées, aucune vue, site, allocation ou crédit.
};
// Classification : masque exclu LOCALEMENT, masque admis LOCALEMENT,
// masque indécis. Les compteurs/seuils restent ceux de l'appel de recherche.
```

Séparer H de Xi permet de ne pas payer Xi quand Hmax<=0. Le mode paire
prépare une fois ses constantes ; le mode rectangle garde les bornes
générales. Toute indécision descend le même index immuable, sans reconstruire
Z, sans liste de témoins persistante et sans copier le nuage.

Le rejet négatif peut toutefois payer Xi sur des blocs `Hmin<=0`, où32
ne le calculait pas. Mesurer séparément ces nouveaux tests, admissions,
exclusions par voie, blocs entièrement exclus et terminaisons mixtes
« q3 admis / q4 exclu ». Ne pas ranger une terminaison mixte dans l'ancien
compteur « toutes les voies admises ». Réviser explicitement les identités
du registre et conserver les piles/capacités en MAX, les travaux en SUM.

Le coût reste O(nœuds visités) par recherche, O(1) par borne, et la pile
u16 garde sa preuve de49 cadres. Cela ne borne pas sous-quadratiquement
la somme des visites sur toutes les paires ou tous les rectangles.

## 4. Écarter les graines q3 par les couches duales29

Les [couches29](Q4_COUCHES_DUALES_20260920.md) ont un certificat plus général
que leur premier raccord q4 : pour toute forme affine du plan médiateur
`Lz(t)=c+x*tx+y*ty`, un site retiré avec `Lz(t)<=0` implique T sites
**retenus**, distincts et strictement intérieurs. La preuve vaut à tout
centre du plan, y compris le centre q3, sans acceptation préalable q2/q3/q4.

Pour q3, utiliser **T=K−1**, et non T=K−2. Si la profondeur globale est<T,
la profondeur retenue est aussi<T ; aucun site retiré ne peut alors être
sur la coquille. En particulier une graine x retirée ne peut définir une
boule q3 utile, puisque `Lx=0` à son centre. Cette décision évite la
construction de la boule et son census. Garder ensuite les contrôles aigu,
propriétaire et le census global32 pour chaque graine restante.

Un port minimal de filtre seul peut préparer `Q4ShallowSet::make(geometry,K+1)` :
K2 appelle3, K10 appelle11 ; cette factory n'a pas la limite10 du front.
Choisir la géométrie `Disk` sans invoquer le domaine q4 `Positive` :
`outside(Positive)` peut rejeter moins de deux complétions, alors qu'un
triangle q3 seul est parfaitement pertinent. Le sélecteur29 n'a pas besoin
des tests `outside` ; son certificat ne suppose pas un centre q4 positif.

**Piège d'API :** ne pas donner directement ce set marqué K+1 aux moteurs
q4 actuels, qui déduisent leur seuil de `set.kmax()-2`. Cela changerait le
seuil q4 en K−1 ! Un noyau T=K−1 peut mathématiquement être partagé avec q4,
mais le seuil d'émission q4 doit rester séparément K−2. Une future API avec
`depth_threshold` du noyau distinct du K des producteurs serait plus claire.

### Contre-fixture du mauvais seuil

IDs dans cet ordre : `a=(10,10,10)`, `b=(14,10,10)`, `x=(12,13,10)`,
`y=(11,12,10)`, `z−=(12,12,8)`, `z+=(12,12,12)`.
Le triangle abx est strictement aigu, ab est son arête propriétaire ; son
centre vaut `(12,65/6,10)` et son rayon carré169/36. Seul y est intérieur,
de puissance−7/3 ; z−/z+ sont extérieurs, de puissance+2/3. Sa profondeur1
est admise pour K3.

Les duaux positifs sont `px=(-12/5,0)`, `py=(-8,0)` et
`pz±=(-2,±2)`. Or `px=(py+7*pz−+7*pz+)/15`, avec coefficients strictement
positifs : x est supprimé après une couche, mais conservé après deux.
Utiliser le noyau q4 K3 tel quel perd donc ce vrai q3. La fixture et ses
puissances ont été contre-vérifiées indépendamment avec l'agent juge ;
ceci reste une démonstration, pas une nouvelle gate exécutée.

### Coût et variantes à ne pas confondre

Pour m sites du cover, payer `O(m log(1+m)+K*m)` et O(m) de préparation,
puis parcourir uniquement les IDs retenus. Conserver l'ancienne énumération
de toutes les graines en lui ajoutant seulement une recherche d'appartenance
n'éliminerait pas son propre coût de génération.

Le noyau peut ne rien enlever : frontières convexes, groupes duaux confondus,
dégénérescences, nombreux c=0. Petits covers ou peu de graines peuvent rendre
sa préparation défavorable. Un éventuel choix adaptatif doit conserver un
repli exact, jamais tronquer les graines ; aucune valeur de seuil de coût
n'est choisie dans cette note.

Le retour B compare une fraction de seeds de0,15 à0,20 dans ses18essais
à environ35bornes de profondeur par seed du premier scan32/K5/8k.
Il en tire un avertissement de rentabilité défavorable au peeling q3.
Ces unités de travail n'ont pas des coûts CPU identiques ; la formule
asymptotique de sélection ne donne pas, à elle seule, un facteur de temps
mesuré pour un filtre q3 qui n'est pas implémenté. On retient l'alerte et
diffère ce port, sans présenter le facteur estimé comme une mesure.
Les coûts de préparation effectivement élevés de Window30 dans
l'[analyse globale32](../receipts/q34_indexed_20260921/ANALYSE_CROISSANCE.md)
renforcent ce choix. Une variante positive seule ou une préparation déjà
partagée devrait avoir son propre bilan, pas hériter de ce modèle.

Pour un **filtre de graines q3 seulement**, il existe une simplification
sûre à étudier : toute graine strictement aiguë vérifie
`c=2*(|x-a|²+|x-b|²-D)>0`. Il suffit donc de peler le groupe positif pour
certifier les graines rejetées ; les couches négatives ne sont pas requises
par cette preuve. Cela ne permet pas de retirer les témoins négatifs ou
c=0 d'un census réduit : cette autre fonction exige le certificat complet,
la coquille complète et le transfert cover→nuage après positivité/propriété.

## 5. Idées A/B réutilisées et suite massive

La [composition forte proposée par A](../audits/q4_kernel_composition_20260920/COMPOSITION.md)
donne l'ordre sûr : **noyau global → partition exacte de ce noyau → noyau
local des actifs au seuil T−c**, où c est le compte exact uniformément
intérieur. Chaque réduction possède sa population d'entrée et son seuil ;
c est payé une seule fois. Ne pas intersecter des noyaux indépendants :
leur intersection peut retirer les témoins qui justifiaient chacun des
certificats. Préparer le sous-ensemble spatial exact et les noyaux locaux
a aussi un coût ; une liste d'IDs filtrés n'est pas un nœud Z complet.

Le [futur index de fenêtre proposé par A](../audits/q4_kernel_composition_20260920/WINDOW_INDEX.md)
conserve les frontières dans leur ordre cyclique, avec poids par nombre
d'IDs, préfixes, points colinéaires et doublons. Les requêtes doivent traiter
les tangences, la droite-pôle, les constantes intérieures/coquilles et les
formes de constante nulle séparément. Conserver tous les contacts, notamment
la fenêtre ponctuelle L=U : les coquilles aux bornes ne sont pas bornées
par le petit nombre d'événements strictement internes. Construction partagée,
recherches de tangentes, rangs pondérés, extraction et tri des IDs restent
à payer ; aucun accès logarithmique qualifié n'est hérité de cette proposition.

Une autre piste des échanges A/B est de reprendre pour q3/q4 le principe
de partage utilisé en q2, **entre produits descendants du front**, au-delà
de la recherche commune déjà portée à l'intérieur d'un rectangle.
Toute continuation doit conserver la certification propre à chaque produit,
ses masques, son compte et son curseur : populations Z consommées disjointes
pour ce compte, aucun ancêtre déjà consommé rejoué, aucun crédit additionné
deux fois. Cette extension reste non portée ; partager l'index seul ne
partage pas automatiquement le travail ni les crédits.

Enfin le raccord global q3/q4 garde encore l'arête atomique. Des plages de
graines partageant un parent immuable — cover, géométrie, noyau/partition —
et des buffers privés pourraient répartir les grosses arêtes sur davantage
de workers. Il faut mesurer préparation une seule fois, stockage simultané,
ordonnancement, travail utile et sorties ; ni duplication du parent par
tâche, ni relance des ancêtres pour fabriquer du parallélisme. Ces idées
indépendantes ne sont pas des fonctionnalités qualifiées du constructeur,
encore moins un backend GPU ou une preuve de passage à l'échelle massive.

## 6. Ordre conseillé de port et qualification

1. Ajouter une primitive préparée de paire et le rejet négatif par voie,
   avec oracle entier indépendant des intervalles, tangences, boîtes mixtes,
   endpoints et u16 extrêmes. Comparer aussi les intervalles au général.
2. Raccorder explicitement, conserver la référence32 et payer les nouveaux
   tests Xi, masques et terminaisons. Même payload global, comparaisons
   mono/multi, exceptions et fermeture des preuves avant promotion.
3. Mesurer n8k/16k/32k sur les mêmes scans : recherches qui saturent et qui
   échouent, préparation, résidu, graines, census, q4, coquilles et temps
   complet. Ne pas déduire la croissance de la seule baisse des visites Z.
4. Raccorder le partage des recherches entre sous-produits et réduire le
   travail q4 identifié dans la campagne. Différer le noyau q3 tant qu'un
   profil ne justifie pas sa préparation ; s'il est réexaminé, conserver
   la contre-fixture de seuil, les grandes coquilles et les cas sans gain.
   Aucun remplacement sur la seule foi d'une preuve de sûreté.

La nouvelle primitive possède une taille constante et se prête à des tâches
par paire ; cela ne constitue ni un kernel GPU ni une équipe persistante
plus équilibrée. Les coûts de préparation par arête et les graines résiduelles
restent à partager ou supprimer. GCP, FULL,50k en1s/100ms et dizaines de
millions de points ne sont pas qualifiés par cette note.

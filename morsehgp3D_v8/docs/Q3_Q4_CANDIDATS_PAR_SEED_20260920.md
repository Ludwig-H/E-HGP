# Des événements aux candidats q3/q4 positifs

20 septembre2026, tranche23 qualifiée dans le périmètre ci-dessous. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. GCP non utilisé.

## Ce que cette tranche ajoute

La [famille précédente](Q4_FAMILLE_ET_BALAYAGE_20260920.md) compte exactement
les intérieurs de chaque sphère passant par un triangle aigu donné. Elle
ne décide pas si cette sphère est portée par un tétraèdre positif, ni si
cette présentation doit l'émettre. Le nouveau raccord fait ces décisions
et produit une **clé géométrique commune aux trois voies**.

`ExactBall` reconnaît les supports positifs de deux, trois ou quatre sites.
`run_q34_seed_candidates` consomme UNE seed aiguë `(a,b,x)` et Kmax,
sur le propriétaire immuable partagé. Il peut publier la boule q3 de cette
face puis des boules q4 de sa famille. Il ne génère pas encore les seeds
depuis le front WSPD et n'est donc pas un générateur global complet.

## Une clé de boule, pas une clé de niveau

Écrire la puissance sous la forme entière A|z|²+B·z+C, avec A>0. Diviser
les cinq coefficients par leur pgcd donne une représentation primitive
unique de la boule. Le signe est négatif à l'intérieur, nul sur la coquille.
Deux supports, même de cardinalités différentes, qui portent la même boule
donnent la même clé. Le cardinal minimal global q_min ne fait pas partie
de cette clé ; il appartient au futur regroupement des présentations.

Les coefficients sont privés et immuables, uniquement construits par les
factories. Ils ne peuvent pas provenir d'un tableau arbitraire de l'appelant.
La clé est copiable par valeur, sans emprunter les coordonnées d'une seed.
Elle n'ordonne **pas** les rayons : leurs carrés et leurs comparaisons
peuvent dépasser i128 et devront recevoir leur propre représentation.

Pour q2 : A=1, B=−(a+b), C=a·b. Pour q3, les notations G/W de la famille
donnent A=G, B=−2Ga−W, C=G|a|²+W·a. Seuls les triangles strictement aigus
sont admis, jamais les droits ou les dégénérés.

Pour q4, poser d_i=p_i−a, t=det(d_1,d_2,d_3), et
N=|d_1|²(d_2×d_3)+|d_2|²(d_3×d_1)+|d_3|²(d_1×d_2).
Le centre relatif vaut N/(2t). Les trois coordonnées barycentriques hors
de a ont le dénominateur **positif** 2t² et les numérateurs
N·(d_2×d_3), N·(d_3×d_1), N·(d_1×d_2). Tous doivent être strictement
positifs, ainsi que le reste 2t² moins leur somme. Une coordonnée nulle
relève d'une arité inférieure, pas d'un q4 positif.

Après ce test, A=|t|, L=sign(t)N, B=−2Aa−L, C=A|a|²+L·a.
Pour M=65535, |t|≤6M³, |N_i|≤18M⁴ ; les tests barycentriques et leurs
sommes partielles sont <2^105. Les puissances globales q3 sont bornées
par360M⁶<2^105 et celles de q4 par180M⁵<2^88. Chaque produit sensible est
élargi **avant** calcul ; le pgcd ne fait que diviser ces coefficients.
Les bornes portent sur les u16 certifiés par les factories.

## Les décisions du raccord

L'arête proposée ab doit être la plus longue du triangle, ex aequo départagé
par la plus petite paire triée d'IDs originaux. Sinon elle ne peut pas être
propriétaire d'un tétraèdre qui contient ce triangle, et le raccord termine
sans balayage. Les rangs spatiaux ne servent jamais à cet ordre.

Pour Kmax≥2, un scan des sites compte la profondeur q3 et collecte sa
coquille. Il peut s'arrêter dès Kmax−1 intérieurs, puisqu'aucun crédit ne
sera retiré de ce compte. En cas d'acceptation, le compte est complet et
la coquille aussi. Son buffer est libéré avant la famille q4.

Pour Kmax≥3, le balayage q4 est lancé **même si q3 a été rejeté**. Son
compte, lui, n'est jamais saturé, car il peut redescendre. Pour un groupe
de profondeur <Kmax−2, ses IDs sont examinés dans l'ordre jusqu'à trouver
une présentation satisfaisant :

- ab est propriétaire parmi les six arêtes, avec le même départage ;
- le tétraèdre est strictement positif ;
- x est le plus petit ID parmi les deux faces aiguës incidentes à ab.

Un échec ne rejette pas les autres présentations de la même racine.
Une fois une présentation valide trouvée, une seule boule est publiée pour
ce groupe de cette seed. Le contrat est **boules candidates locales**,
pas toutes les incidences, ni déduplication globale. Le rejet par q2 n'est
jamais consulté ; le rejet par q3 n'est pas une porte d'accès à q4.

## Sorties, coût et parallélisme

Chaque callback reçoit la clé par valeur dans son record, une présentation
positive (IDs triés), sa cardinalité, la profondeur exacte et la coquille
complète en deux vues disjointes. En q3 la seconde vue est vide ; en q4
ce sont les sites de la racine et la coquille permanente. Les vues ne
survivent pas au callback. Leur union n'est pas recopiée à chaque événement.
La coquille n'est pas bornée par Kmax. Les IDs des intérieurs ne sont pas
émis : leur collecte après déduplication globale reste à implémenter.

Le propriétaire demeure possédé pendant l'appel. Une exception se propage,
conserve les émissions antérieures chez le consommateur et ne renvoie pas
un résultat partiel présenté comme complet.

Pour une seed, le raccord ajoute un scan O(n), des tests constants pour
au plus e IDs événements et aucune boucle entre paires de complétions.
Avec le balayage, le travail producteur est O(n+e log(1+e)), mémoire O(n),
auquel s'ajoute le travail du callback. Les compteurs séparent le scan q3,
les groupes q4 rejetés par profondeur, les présentations effectivement
testées, les refus de propriété/positivité/seed, et les IDs non examinés
après une émission. Aucun résidu n'est supprimé par quota.

En succès, les registres q4 doivent fermer exactement : événements = IDs
sautés par profondeur + présentations testées + IDs non examinés après
émission ; groupes = groupes trop profonds + groupes sans support + boules
émises. Les présentations se répartissent entre refus de propriétaire,
refus de positivité, refus de seed et émissions. Ces identités ne valent
pas un reçu complet lorsqu'un callback a levé.

Pour S seeds, les scans peuvent encore coûter S·n. **La génération globale
et sa borne sous-quadratique restent ouvertes.** Le prochain raccord doit
mesurer et réduire S et les covers avant de multiplier les workers ; un
événement n'est pas une preuve de support et une clé n'est pas une tour.
Les segments par seed et les tests de présentations sont indépendants,
mais cette tranche n'ajoute pas de backend multi-CPU/GPU ni de file.

## Qualification

84 CTests Release passent dans un build neuf. Les deux nouvelles gates et
six sondes passent aussi sous Clang ASan/UBSan ; la suite complète84 n'a
pas été rejouée sous sanitizers. Deux oracles partagent un solveur de Gram
rationnel indépendant des formules fermées du produit :

- ExactBall :1807 cas et34 460 contrôles, coordonnées extrêmes, refus de
  centres sur la frontière, permutations et clés égales entre arités.
- Raccord :635 appels et24 590 contrôles ;13 petits nuages confrontés à
  une énumération exhaustive indépendante donnent les mêmes229 boules.
  Sont exercés : q3 rejeté mais q4 conservé, première présentation invalide
  puis valide, coquille30, profondeur qui redescend, q_min global inférieur
  à l'arité de la présentation, exceptions et quatre appels concurrents
  indépendants. Cette énumération bornée est un juge, jamais le générateur.

Quatre mutations **compilées du produit** sont tuées par ces gates : signe
d'orientation, positivité non stricte, coquille comptée intérieure q3,
abandon du groupe au premier refus. Elles sont distinctes des corruptions
de sortie des juges et des vingt mutations du lecteur de reçus.

Les captures ferment trente mesures : six sondes Release n32, six sous
sanitizers et dix-huit à8k/16k/32k, K5/10, trois fonds lointains autour
d'une seed fixe. Chaque mesure conserve réellement une boule q3 et une
q4 de profondeur2. À K10, avec le fond lointain uniforme, traitement et
callback coûtent3,781/7,982/16,735 ms sur un CPU local ; comparaisons du tri
125856/272754/580481 (×2,167/×2,128), buffers128000/256000/512000 octets.
Ces fixtures mesurent une seed et ses événements ; elles ne représentent
pas le coût de toutes les seeds d'un nuage uniforme, LiDAR ou massif.

Les [reçus et lectures](../receipts/q34_seed_20260920/README.md) conservent
les commandes, sorties,135 sources antérieures plus neuf nouvelles et
les empreintes avant/après. Builds désormais épinglés :
`build/v8_q34_candidates_20260920` et
`build/v8_q34_candidates_sanitize_20260920`. Ne pas les reconstruire pour
poursuivre. Le paramètre s n'intervient pas avant le raccord WSPD ; s8/10/12
reste obligatoire à ce raccord. Aucun nouveau TSan, backend parallèle,
résultat GPU/G4 ou contrat de tour acquis.

## Raccord suivant proposé à l'auditeur

Partager par arête ab un cover fermé |2z−a−b|²≤4|b−a|² issu de l'index
global. La borne de variance S1§2 couvre toute boule positive dont ab est
propriétaire, et donc ses intérieurs, sa coquille et ses complétions.
Pour un nouveau raccord limité à ces boules, omettre les sites extérieurs
au cover ne donne que des comptes partiels sur les racines arbitraires,
mais des comptes complets après certification de positivité/propriété.
Un rejet au seuil par le compte partiel resterait sûr.

Cette piste pourrait éviter de qualifier d'abord une corde avant de réduire
les scans. Elle ne doit **pas** changer silencieusement le contrat actuel
de `run_q4_family`, qui compte tout le nuage pour chaque racine. Préparer
le cover une fois par arête, partager son propriétaire et ses plages entre
seeds, conserver la frontière fermée et mesurer la somme des volumes lus.
Le dialogue d'audit est ouvert ; aucun raccord ni gain n'est encore acquis.

# B — transporter les intérieurs q3/q4 sans second census

26 septembre 2026, lecture de `52ff41802`. Audit et oracle mathématique
isolé ; aucun moteur, défaut ou protocole GPU modifié. Profil entier 1 mm,
`quantized_u18_input_only`, `public_status=not_claimed`.

## Conclusion immédiatement utilisable

**Oui, le producteur peut conserver les identités qu'il compte déjà.** Le
chemin q4 GPU actuel est plus favorable que ne le suggère le mot
« balayage » : T1 compare séparément une classe candidate à la liste des
événements de son intervalle. Lorsqu'il accepte, cette comparaison a lu
**toute** cette liste et possède déjà les masques exacts `inside`. Il suffit
d'en recueillir les IDs et d'ajouter ceux de la lentille commune. Aucun
nouveau parcours de l'index global, ni nouvelle comparaison géométrique
par boule acceptée, n'est nécessaire.

À K5, q3 transporte au plus trois IDs, q4 au plus deux ; à K10, huit et
sept. Ce port court supprime le besoin de retrouver ces IDs en aval. Il ne
supprime pas, à lui seul, le coût potentiellement quadratique des
comparaisons T1 entre candidats et événements.

Pour remplacer ensuite T1, une solution plus simple qu'un ensemble actif
dynamique existe : **deux listes triées, un préfixe d'entrées et un suffixe
de sorties**. Les groupes de racines et les sommes préfixes se traitent en
lots ; chaque groupe accepté retrouve ses IDs en O(K), même si la
profondeur est passée très au-dessus de K auparavant. Les détails et coûts
suivent. Aucun gain de temps GPU n'est encore établi pour ces propositions.

## Ce que fait réellement le code lu

Dans `src/gpu/lanes.hpp`, `LaneRecord` fait 128 octets et garde la clé,
le support, la profondeur et le cardinal/empreinte de coquille, mais aucun
ID intérieur. `q3_census_chunk` connaît déjà les masques de puissance
strictement négative. Une émission q3 a effectué le census complet de son
cover ; le seuil d'arrêt est K−1, donc sa profondeur est au plus K−2.

Dans `src/gpu/q4_lanes.hpp` :

- `q4_pass_chunk` accumule huit comptes de lentille et conserve les
  événements des intervalles encore vivants ; il ne garde pas les IDs des
  lentilles.
- `q4_seed_finish` écarte les intervalles dont la lentille atteint K−2.
  Pour chaque classe restante, sa boucle T1 produit `same` et `inside`,
  incrémente la profondeur à partir de `lens[j]`, et s'arrête si elle
  atteint K−2. Une classe acceptée a nécessairement terminé cette boucle.
- Les groupes de même racine forment la coquille, complétée par la
  coquille constante. Le représentant est sélectionné après les tests de
  propriété, canonicité et positivité. **Les événements non candidats
  restent des témoins et ne doivent pas disparaître du paquet.**
- L'ordre des records q4 est ensuite rétabli par représentant ; L15
  (`src/gpu/lanes_tasks.hpp`) écrit les records q3 en sens inverse au haut
  de la slab, et q4 depuis le bas. Un tableau parallèle d'IDs doit subir
  ces permutations ; un simple pointeur vers le scratch réutilisé serait
  faux.

Enfin `src/chain/tower_chain.cpp::census_key` refait un census global de
chaque clé distincte, vérifie les deux cardinalités, puis construit
`BallData`, les contributions Euler et le traitement des coquilles
étendues. Le [prototype consommateur](b_census_payload_20260926/README.md)
montre déjà comment remplacer le census régulier par un import exact lié
au propriétaire ; il ne mesurait pas le producteur GPU.

## 1. Partition exacte de l'intérieur d'un intervalle q4

Pour une graine aiguë fixée, la puissance d'un site z est de signe
`F_z(μ) = P_z − μ S_z`. Dans l'intervalle fermé [l,h] :

- L est l'ensemble des sites strictement intérieurs aux deux extrémités.
  Ils sont strictement intérieurs partout dans l'intervalle.
- Si `S_z ≠ 0`, sa racine est `r_z=P_z/S_z`. Les événements E sont ceux
  dont la racine appartient à [l,h].
- `S_z=0, P_z=0` définit la coquille constante ; `S_z=0, P_z<0` appartient
  à L. Les autres sites qui ne sont ni L ni E restent extérieurs sur
  l'intervalle ouvert et ne sont pas intérieurs à ses extrémités.

Pour toute racine candidate r de cet intervalle, l'intérieur strict est
la réunion **disjointe**

`I(r) = L ∪ {z∈E : S_z>0 et r_z<r} ∪ {z∈E : S_z<0 et r_z>r}`.

Sa coquille est la coquille constante réunie avec **tous** les événements
de racine r, sans les filtrer par propriété ou positivité. Cela vaut aussi
à l et h ; seule la propriété de l'émission change à h, qui appartient à
l'intervalle de droite sauf pour le dernier. Les comparaisons de la
formule sont strictes, donc aucun contact n'entre dans l'intérieur.

La complétude globale n'est pas donnée par cette seule identité affine.
Elle utilise aussi le cover certifié de l'arête propriétaire et le
support finalement positif : la boule fermée entière appartient au
cover. Voir la preuve géométrique et les limites de domaine dans
[l'audit des gains structurels](AUDIT_B_GAINS_STRUCTURELS_100MS_20260926.md).
Les rejets du cover doivent rester stricts ; les cas différés/fautifs ne
produisent pas de paquet partiellement « certifié ».

## 2. Port court recommandé : garder les IDs pendant T1

Pour chaque intervalle j, conserver au plus K−3 IDs de lentille. Chaque
compte de lentille ne fait qu'augmenter pendant le passage initial. Si le
compte atteint K−2, cet intervalle ne pourra jamais émettre et ses IDs
peuvent être abandonnés. Un intervalle encore vivant à la fin a donc tous
ses IDs dans la liste bornée. Ce raisonnement ne se transpose pas à un
ensemble actif dont la cardinalité peut diminuer : il utilise ici
explicitement la monotonie de **la collecte de lentille**.

Pendant une comparaison T1, partir de cette liste puis ajouter les IDs
des bits `inside` de chaque chunk déjà comparé. Avant d'écrire, vérifier
si le nouveau compte atteint le seuil ; si oui, rejeter sans déborder la
capacité. Pour une classe acceptée, le passage complet prouve que la
liste de taille `depth` contient tous les intérieurs. Aucune récupération
des IDs d'une ancienne classe rejetée n'est requise. Une classe sans
représentant positif ne publie pas sa collecte.

Coûts supplémentaires à publier, pas à appeler « gratuits » :

- scratch de lentilles : `8·max(K−3,0)` IDs, soit 64 octets à K5 et
  224 à K10 par graine active ; ce sont des capacités, non des quotas de
  recherche. Stocker en scratch partagé/registre distribué, pas répliquer
  ce tableau complet dans chacun des 32 threads sans mesure ;
- extractions des masques de lentilles : jusqu'à huit votes par chunk,
  restreints aux intervalles encore vivants, utilisant les signes déjà
  calculés. Le vote actuel est un cumul compact, pas huit listes gratuites ;
- collecte T1 : au plus O(K) écritures par passage de classe, même rejeté,
  plus la copie O(K) par émission. Ne pas recompter huit fois les puissances ;
- paquet possédé jusqu'à la déduplication/import, permutation q4/L15,
  compactage et transfert. Mesurer arène compacte contre stride fixe ;
  ajouter un champ au record peut augmenter son alignement et son trafic.

Le coût géométrique T1 reste inchangé. Le premier port évite le scan de
récupération par émission suggéré comme pont dans la note précédente ;
ce pont n'est **pas nécessaire** avec les masques déjà disponibles.

Pour q3, même principe plus simple : recueillir `inside` tant que le
census n'est pas rejeté. Un rejet ne publie rien ; une acceptation a lu
tout le cover. Dans L15, q3 et q4 ont des conditions d'arrêt distinctes :
ne pas arrêter la collecte de l'un lorsque l'autre se ferme.

## 3. Refonte parallèle de T1 : fenêtres d'IDs sans réservoir

Pour un intervalle vivant, trier exactement ses m événements par racine,
puis regrouper les égalités. Compacter deux tableaux d'IDs qui conservent
cet ordre : les entrées (`S>0`) et les sorties (`S<0`). Pour un groupe g :

- `e_g` = nombre d'entrées des groupes **strictement précédents** ;
- `x_g` = nombre de sorties des groupes **strictement suivants** ;
- `d_g = |L| + e_g + x_g`.

Des scans exclusifs de comptes par groupe produisent tous ces nombres.
Le paquet de g est exactement L, les `e_g` premiers IDs du tableau
d'entrées et les `x_g` derniers IDs du tableau de sorties. Si `d_g<K−2`,
chaque fenêtre est petite et leur union a au plus K−3 IDs. **La traversée
d'une zone profonde ne perd rien : les IDs sont toujours dans les deux
tableaux, pas dans un réservoir tronqué.** Tous les groupes peuvent
construire leur paquet indépendamment après les scans.

Cette écriture est l'équivalent parallèle de « retirer toutes les sorties
du groupe, émettre, puis ajouter toutes ses entrées ». Les entrées et
sorties simultanées sont toutes des contacts à l'instant de l'émission.
Ne pas faire une émission intermédiaire au milieu d'un groupe.

Coût par intervalle : O(m log m) comparaisons exactes pour un tri générique,
O(m) compactage et scans, O(K·G_acc) IDs exportés, plus les tests de
positivité/propriété et les coquilles. Scratch O(m+G_acc·K). Avec un tri
segmenté, les graines, intervalles, groupes et sorties se parallélisent ;
la profondeur du graphe de calcul n'est plus celle d'un parcours sériel
de tous les groupes. Les petites listes peuvent garder T1 ; les longues
listes difficiles peuvent choisir la voie triée, sans changer les résultats.

Les racines sont rationnelles exactes ; une clé float ou un tri radix de
leur approximation ne décide pas les égalités. Réutiliser le comparateur
réduit certifié (signes des deux dénominateurs inclus), ou une clé
rationnelle dont le domaine et l'ordre sont démontrés. Un comparateur
O(1) sur le domaine u18 ne rend pas le tri instantané. Mesurer le nombre
de comparaisons, le tri/compactage, les maxima de segment et la somme
des m : T1 s'arrête souvent très tôt, et une voie triée peut alors perdre.

Surtout, cette amélioration borne le traitement **d'une liste d'événements**,
pas la somme de toutes les listes/cover/graines sur le nuage. Aucun
résultat sous-quadratique LiDAR global ne suit sans mesures des entités
amont et aval sur les partitions spatiales demandées.

## 4. Pourquoi certains résumés ne suffisent pas

Un réservoir de deux IDs reçoit l'ensemble `{1,2,3}` et ne garde que
`{1,2}`. Après les sorties de 1 et 2, l'intérieur est `{3}` et le
réservoir vide. Le compte exact 1 ne permet pas de retrouver 3. C'est
une trajectoire affine valide : trois sorties de racines 1, 2 et 4,
observées au paramètre 3. Ce contre-exemple vise l'état abstrait de
balayage ; il ne prétend pas qualifier un support q4 positif particulier.

Même nombre, somme et XOR bruts n'identifient pas toujours les IDs :
`{0,3}` et `{1,2}` donnent tous trois les mêmes résultats. L'empreinte
actuelle `sum(mix64(ID)), xor(mix64(ID))` est un contrôle différent : cet
exemple brut n'est pas une collision explicite de `mix64`. Mais aucune
preuve d'injectivité ni procédure de récupération n'en fait un paquet
exact ; à sept IDs 32 bits, le nombre d'ensembles dépasse déjà l'espace
de 128 bits de cette empreinte. Les hashes ne doivent pas devenir la
preuve de complétude.

Des moments algébriques exacts sont possibles : cardinalité exacte plus
les K premières sommes de puissances dans un corps de caractéristique
supérieure à K, avec encodage injectif des IDs, déterminent un petit
ensemble de taille connue ≤ K par les identités de Newton. Encore faut-il
factoriser/retrouver les racines exactement, payer O(K) mises à jour et
éviter une recherche dans tous les IDs du nuage. Cette sophistication
n'est pas justifiée ici : les deux fenêtres donnent directement les IDs.
Un bitmap récupérable exact ou un ensemble actif ordonné peut également
fonctionner, mais ajoute mémoire, mises à jour et extraction ; il n'est
pas nécessaire pour des événements qui entrent ou sortent une seule fois.

## 5. Import, coquilles et preuve à préserver

Une liste de d IDs distincts de puissance négative est complète **si** le
producteur certifié a établi indépendamment que le cardinal global vaut
d. Si le compte est fabriqué à partir de la liste tronquée, les tests
locaux ne détectent pas l'omission. Garder la provenance interne,
l'identité du nuage immuable, la clé et le support ; aucun booléen public
« complet » ne remplace ce contrat.

Si le cardinal de coquille vaut q et que les q sites distincts du support
sont certifiés sur la sphère, ce support est la coquille complète. C'est
le chemin régulier éligible à l'import sans nouveau census. Si la coquille
est plus grande, conserver dans un premier temps le census global et le
traitement `ShellTable`. Le paquet intérieur seul ne supprime pas les
obligations de coquille, q_min, multifusions et Euler. Des listes exactes
de coquille peuvent être portées ensuite, en payant leur taille réelle ;
ne jamais les tronquer au seul support.

À la fusion des présentations d'une même clé, ne pas additionner leurs
intérieurs : ce sont plusieurs descriptions de la même boule. Choisir un
paquet complet compatible avec le représentant canonique, ou vérifier
leur égalité, et conserver tous les refus/replis actuels. Le sceau public
ne doit plus annoncer un second census indépendant si l'on fait seulement
une validation locale du paquet.

## 6. Preuve locale nouvelle et gates du futur port

L'oracle [check.py](b_q4_payload_math_20260926/check.py) utilise des
puissances entières et des racines `Fraction`, sans importer le moteur.
126 fixtures vérifient 805 groupes, dont 257 groupes à contacts multiples
et 250 groupes sur une extrémité. Les deux constructions d'IDs (T1 et
fenêtres) égalent le census direct, coquilles incluses. Trois petites
fixtures géométriques partent d'un tétraèdre positif propriétaire et
exercent les profondeurs 0, 1 et 2. Les autres fixtures testent l'identité
affine, sans prétendre que chaque groupe est une émission positive. Tous
les seuils K3..K10 sont testés et 29 transitions profond→superficiel sont
effectivement rencontrées, sans aucune perte d'ID.

Les exécutions Python normales et `-O` rendent les mêmes résultats et le
même hash de source. Sept contre-épreuves explicites couvrent réservoir,
ordre des contacts, résumé d'IDs, falsification de cardinalité, signe du
dénominateur et omission de membres d'une classe d'égalité. Ce sont
des mutants mathématiques locaux, **pas** des mutants compilés du moteur.
Le premier essai avait échoué au contrôle de non-vacuité K3 : ses nuages
fixes étaient trop denses ; ajout d'une fixture creuse, puis des trois
tétraèdres. Aucun défaut produit n'est déduit de cette fixture insuffisante.

Avant activation, le port devra comparer les listes complètes et les
`BallData` au census global sur supports q3/q4, contacts, lentilles vides,
seuil atteint au milieu d'un chunk, graines rejetées, nombreuses émissions
par graine, L15/non-L15, repli de capacité, et permutations des records.
Il faudra aussi des mutants compilés omission/ID dupliqué/compte forgé,
un rejeu différentiel GPU de FULL explicite et la mesure du coût total
collecte+transport+import. Cette note ne qualifie ni ce port absent,
ni G4, ni le contrat 100 ms.

## 7. Raccord produit après le port q3 — reprise sur `f9f273bb0`

Lecture seule du produit, le 26 septembre ; **q4 reste une proposition**.
Le port q3 dispose maintenant d'un sidecar possédé de stride K−2, des
permutations L15/compactage et d'un import régulier dans `census_key`.
Cela fournit le transport, mais ne qualifie pas automatiquement q4 : la
porte `tests/gpu/interior_payload_gate.cpp:73` exige actuellement des
sentinelles dans tous les slots q4. Ses compteurs q4 ne prouvent donc pas
un transport d'intérieurs q4.

### Chemin minimal, sans nouveau census

1. `q4_lanes.hpp:309` (`q4_pass_chunk`) : recueillir les IDs de lentille
   pendant les calculs de signes existants. Pour un bucket j, écrire
   seulement si le **nouveau** compte reste `< K−2` ; s'il atteint ce
   seuil, aucune écriture hors capacité et aucune publication ultérieure.
   Le compte ne diminue jamais. Un bucket finalement vivant conserve donc
   tous ses IDs, au plus K−3, sans réservoir d'ensemble actif.
2. `q4_lanes.hpp:515` (T1) : initialiser le scratch de classe avec cette
   lentille, puis compacter les bits `inside` déjà obtenus. Tester le
   nouveau compte avant toute écriture. Une classe rejetée, inachevée ou
   sans représentant positif ne publie rien. Les témoins non candidats
   restent inclus. Repartir d'un scratch neuf pour **chaque** classe,
   y compris après un arrêt anticipé d'une classe profonde.
3. `q4_lanes.hpp:621` : écrire au même slot physique que le record accepté,
   IDs originaux via `index.rank_ids`, puis sentinelles jusqu'au stride.
   `q4_lanes.hpp:646` : le tri local par représentant doit échanger le
   **sidecar entier avec son record**. Cette permutation précède L15 et
   n'est pas couverte par le gather final.
4. `lanes_tasks.hpp:269,352,431` : transmettre le scratch aux chemins
   fusionné et séparé. Pendant le passage fusionné, lentilles et scratch
   q3 coexistent ; après publication de q3, son scratch K−2 peut servir à
   T1. Préserver les deux conditions d'arrêt, les emplacements q3 en haut
   et q4 en bas, et les reprises propres lorsque L15 déborde.
5. `lanes_host.hpp:307` et `filter_runner.cu:1078` remplissent aujourd'hui
   explicitement q4 de sentinelles : copier aussi les paquets q4 activés.
   `lanes_task_slab_index`, staging, gather et transfert conservent ensuite
   la même association record/paquet. Un essai différé/fautif ne publie
   aucun paquet survivant d'un scratch antérieur.

Pour éviter huit nouveaux calculs de puissance par chunk, conserver les
neuf bits `neg` déjà calculés dans les bits hauts d'un mot par lane, avec
les deux bits q3 actuels dans les bits bas. `HostGroup::ballot2` et
`WarpGroup::ballot2` ne lisent que ces deux bits bas. Après le vote compact
existant, les buckets dont le compte final du chunk est encore vivant et
a augmenté extraient leurs masques de lentille depuis ce mot, puis leurs
IDs. Les votes supplémentaires restent un coût réel à mesurer ; aucun
nouveau parcours géométrique n'est nécessaire. Ne pas lire les lanes
hors du chunk final partiel.

### Mémoire et aval

Scratch additionnel de lentilles : `8·max(K−3,0)` mots par **warp actif**
(64 octets à K5, 224 à K10), plus un mot de signes par lane si nécessaire.
Le scratch de classe réutilise les huit mots q3 après son émission.
Le sidecar K−2 existant suffit pour q4 (maximum K−3), donc `LaneRecord`
reste à 128 octets et aucun stride supplémentaire par record n'est requis
si q3 est déjà activé. En activation q4 seule, ce sidecar reste un coût
à payer. Requalifier occupation, mémoire partagée et politique de nombre
de warps ; ne pas transposer l'occupation mesurée du kernel q3.

L'import actuel `tower_chain.cpp:897` est réservé à `arity==3 && shell==3`,
et l'association `:1586` aux présentations q3. Étendre explicitement le
chemin à q4 **régulier** (`arity==4 && shell==4`), avec source interne
certifiée, profondeur `< K−2`, IDs distincts et puissances strictement
négatives. Garder le census global pour les coquilles étendues et le tail
sans paquet. Ne pas réinterpréter silencieusement les compteurs q3 du
protocole v30 comme des compteurs q3+q4 ; option séparée désactivée et
schéma explicite lors du port. Aucun mélange des IDs de plusieurs
présentations d'une même clé.

### Qualification prioritaire avant tout chrono GPU

Le juge doit comparer **tous les IDs** à une puissance directe et le
`BallData` importé au census indépendant, pas seulement les empreintes.
Exiger des occurrences non nulles : lentille seule, événements seuls et
mélange des deux ; deux signes ; contacts/constants ; racines communes et
bornes de buckets ; K3/K5/K10 dont profondeur K−3 ; passage rejeté puis
accepté ; bucket mort au milieu d'un chunk ; plusieurs émissions par
graine avec permutation effective ; q3 rejeté mais q4 accepté en L15 ;
fusion ON/OFF, pinned/pageable, slab trop petite puis reprise et tail.
Mutants prioritaires : `<=` au contact, omission de lentille, rang pris
pour ID, scratch stale après rejet, sidecar non permuté et q4 remplacé
par sentinelles au staging. Géométrie et ledger T1 doivent rester
identiques entre ON/OFF ; seuls collecte/transport et census aval varient.

Cette tranche supprimerait le **recensus régulier q4**, pas les passages
T1 potentiellement quadratiques. La voie triée de la section 3 demeure
une refonte séparée à comparer au T1 avec arrêt anticipé, par familles
et sur les partitions LiDAR entières/moitiés/quarts. Aucun gain GPU,
borne globale ou contrat 100 ms n'est revendiqué ici.

## 8. Décision de priorité : q4 seul ne constitue pas le saut vers 100 ms

Relecture directe des sorties historiques R24-B (indices 0/7/14/21),
après le port q3 `9751bae69`/`f9f273bb0`, sans nouvelle mesure GCP :

| Entrée / K | FULL interne | Census tardif entier | Tour explicite | Clés q4 régulières / clés tardives | Scénario proportionnel q4 |
| --- | ---: | ---: | ---: | ---: | ---: |
| sans sol 00 / 5 | 916,304 ms | 100,273 ms | 282,724 ms | 158 492 / 849 777 | 18,702 ms |
| sans sol 00 / 10 | 2 968,445 ms | 500,532 ms | 1 207,878 ms | 1 732 538 / 4 630 762 | 187,267 ms |
| brute b00 / 5 | 2 036,570 ms | 210,547 ms | 669,031 ms | 186 253 / 1 556 607 | 25,193 ms |
| brute b00 / 10 | 6 014,830 ms | 1 030,632 ms | 2 405,582 ms | 2 124 184 / 8 789 308 | 249,081 ms |

Dernière colonne = census × part des clés q4 régulières : **hypothèse de
coût uniforme par clé, ni mesure q4 ni borne**. Ces reçus ne séparent pas
le census par arité. Seul plafond contrefactuel large disponible à phases
inchangées : même supprimer **tout** le census tardif du premier cas
laisserait 816,031 ms. Le vrai import garde clé/niveau/positivité/Euler,
collecte et copies ; une accélération q4 peut donc être faible ou négative.
R22 confirme les volumes : le scénario proportionnel donne 14,9–19,0 ms
sur ses trois trames sans sol K5, 22,2–27,3 ms sur les trois brutes K5.
Ces six cas viennent toujours de la seule séquence 08.

Le premier cas R24-B contient par ailleurs un front à **98,598 ms** et
trois kernels successifs S2/S3/S4 à **64,500 / 91,449 / 55,673 ms**, soit
211,622 ms. Front, kernels et tour portent chacun un obstacle majeur au
budget de 100 ms. Ne pas additionner aveuglément les sous-temps imbriqués
ou superposés, ni transformer ce découpage historique en prédiction v30.

Le coût du producteur q4 mérite une vraie décision : 7 126 317 graines,
11 680 121 chunks de lentilles, mais seulement 130 814 graines émettrices
(1,84 %) pour 158 496 records. Huit votes systématiques par chunk
ajouteraient jusqu'à 93 440 968 votes ; le schéma conditionnel de §7 en
fait moins, sans que le compte actuel permette de dire combien. Les
comparaisons T1 ne portent que sur 475 299 chunks : remplacer d'emblée T1
par du tri n'attaque pas le gros travail observé. Le sidecar q3 déjà
présent réserve aussi les slots q4 ; le remplir n'augmente pas sa largeur,
mais ne rend pas gratuite la récupération des lentilles.

**Priorité mise à jour :** juger d'abord le coût net q3 sur G4 ; poursuivre
front GPU et FULL événementiel **sur tous les ordres**, puis réduire le
travail de certificats/covers S3. Le port q4 reste utile mais n'est pas une
étape obligatoire avant ces chantiers. Avant de le sélectionner, mesurer
le census résiduel par arité et les votes/écritures supplémentaires sur
un bras apparié ; le temps total ON après q3 inclut encore les imports q3
et n'est donc pas un « temps q4 ». L'ancien résultat local W4 q3
746,509→447,227 ms n'est pas transférable aux 48 CPU/GPU de G4.

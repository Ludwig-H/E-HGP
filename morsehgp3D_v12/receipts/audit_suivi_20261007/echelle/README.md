# Audit compact du contrat d'échelle v12

7 octobre 2026. Pin `3e6e6a8e721c6dbe8aaa3a0c5cb20c56abfae5ff` ; ajout jugé
`c0bb99fd8`. Cadre `exploration_v12_hors_registre`, objet `full_pi0`,
`public_status=not_claimed`. Aucun moteur compilé ou exécuté, GCP non utilisé,
aucune mesure de temps. Seuls les fichiers de ce reçu ont été écrits.

Deux contrats doivent être précisés **avant port** : une loi empirique de
volume n'est pas une borne mémoire certifiée (`CST-0211`) ; le prototype choisi
encode les opérandes sur 31 bits utiles, malgré leur stockage u32 (`CST-0212`).
Il s'agit de lacunes de la conception, pas de défauts démontrés d'un moteur v12
absent. Les objectifs d'échelle sont explicitement provisoires : leur absence
de preuve universelle n'est pas en elle-même une erreur.

## CST-0211 — pré-vol empirique présenté comme borne

**Majeur, contrat ouvert.** `docs/ARCHITECTURE.md:150` exige « avant tout
calcul, une borne » issue de `n`, `K` et des lois mesurées par site, pour éviter
un refus ultérieur. Les mesures ne donnent pas cette garantie. Le document
lui-même cite 33 boules/site à K5 sur un LiDAR et 75–79 sur uniformes (`:133`).
Ces chiffres justifient un dimensionnement prévisionnel, pas une enveloppe
universelle ; prendre leur maximum observé ne change pas leur nature.

Il faut nommer séparément la prévision, l'admission mémoire certifiée et la
réservation effective. Une passe exacte de comptage peut certifier les volumes
avant matérialisation ; des réservations contrôlées peuvent assurer le budget
en cours de calcul, avec refus transactionnel. Une borne combinatoire démontrée
peut aussi servir, mais elle sera souvent trop pessimiste. Aucune de ces voies
n'est équivalente à une estimation empirique qui interdit tout échec ultérieur.

Bornes sûres disponibles sans loi statistique, pour `C` boules critiques
positives et `I` incidences de population :

- `C ≤ Σ(q=2..4) binomial(n,q)` : chaque boule a un support d'au plus quatre
  sites ; plusieurs supports peuvent définir la même boule, jamais l'inverse.
- `I ≤ nC` ; si la coquille est plafonnée à `H`, l'admission `p+q≤K+1`
  donne aussi `I≤(K−1+H)C`.
- Naissances : `b₁=n`, `b_k≤C` pour `k≥2`. Pour chaque ordre connexe,
  événements binaires `e_k=b_k−1` ; nœuds finaux `b_k≤N_k≤2b_k−1`.
- Ces bornes ne limitent ni les feuilles du parcours ni ses candidats par `n`.
  Le témoin historique cube+centre (9 sites, 80 feuilles de **générateur**)
  concerne bien des feuilles de partition de l'espace, pas des naissances FULL.

Les entiers de ces formules doivent eux-mêmes être bornés avant conversion ou
produit d'allocation ; une formule exacte qui déborde u64 n'est pas un pré-vol
certifié. Aucune complexité quadratique effective n'est affirmée ici : une
borne combinatoire pessimiste ne prouve pas qu'elle est atteinte.

## CST-0212 — domaine réel des identifiants du prototype

**Majeur avant port, contrat ouvert.** La règle nouvelle
`docs/ARCHITECTURE.md:141` annonce des identifiants 32 bits et un refus au-delà
de `2^32−1`. Le noyau retenu comme source (`:94`) utilise pourtant
`kEv=0x80000000` comme bit de genre :
`morsehgp3D_v11/receipts/conception_v11_20261002/conception/preuves_tour/noyau_v11.cpp:21`.
La lecture du genre et l'effacement du bit sont explicites aux lignes 170 et
190–191 ; `jtop` l'emploie ligne 150.

Le témoin arithmétique autonome montre deux collisions exactes :

- naissance d'indice `2^31` et événement d'indice `0` ont tous deux le mot
  `0x80000000` ;
- événement d'indice `2^31−1` donne `0xffffffff`, également `kNone`.

Une multifusion abstraite avec `b=2^31+1` naissances n'a que `b+1` nœuds finaux,
strictement moins que `2^32−1`, mais le codage précédent échoue déjà. C'est un
témoin de capacité du **codage**, sans allocation géante ni revendication de
réalisation sur un LiDAR de moins de dix millions de sites. Les projections
actuelles ne prouvent pas que cette limite sera atteinte ; elles ne changent
pas les bits disponibles.

Déclarer le domaine de chaque espace : naissance, événement brut, nœud final,
rang, feuille, boule, offset. Soit le port conserve le bit de genre et refuse
à sa vraie limite, soit il change de représentation et requalifie le budget
de 20 octets par événement. La présence d'une sentinelle exclut sa valeur des
indices valides. Ne pas confondre `nombre d'objets`, `dernier indice` et
`PointId` externe : la v11 autorise `PointId=0xffffffff`, tout en réservant ce
mot pour l'absence des indices denses (`src/core/types.hpp:53–64`). Elle impose
même strictement `nombre<kNone` à certains tableaux (`tower/ancestor_index.cpp:9`).
La v12 doit écrire sa convention plutôt que laisser « au-delà » ambigu.

L'élargissement des offsets CSR et compteurs en u64 est justifié : la
projection donnée, 5 millions × 1 137 incidences, vaut **5 685 000 000** et
déborderait u32. Le prototype stocke encore `joff`, `nr` et ses boucles de
représentants en u32 (`noyau_v11.cpp:27–40,68`) : la règle « partout u64 » doit
couvrir aussi cette entrée du microbanc/port, pas seulement les CSR catalogue.

## Capacités exactes de tour et de plateau

`check.py` vérifie le cube `{0,2}³` contre l'oracle de définition v11 : à K2,
**12 naissances pour 8 sites**, puis une fusion à douze enfants, soit treize
nœuds. La borne de nœuds dépend de `b_k`, pas seulement du nombre d'entrées.

La famille collinéaire `X_n={(i,0,0):0≤i<n}`, `n>K`, donne une preuve simple
d'un plateau arbitrairement grand. Une boule critique positive est un diamètre
entre deux sites séparés de `d`, avec `p=d−1`, `q=m=2`. Ainsi :

```text
C = Σ(d=1..K) (n−d)
I = Σ(d=1..K) (n−d)(d+1)
b_k = n−k+1, niveau de naissance = (k−1)^2/4
e_k = n−k, tous au même niveau k^2/4
une seule multifusion finale par ordre ; N_k = n−k+2
```

Les parties consécutives minimisent le diamètre ; avant le niveau `k²/4`, elles
ne se rejoignent pas. Les unions de deux fenêtres adjacentes, de taille `k+1`,
les relient toutes simultanément à ce niveau. Les autres parties ont déjà une
connexion lors de leur apparition et ne créent pas de nouvelle naissance.
L'oracle indépendant du catalogue confirme ces formules sur huit sites,
ordres 1 à 5. Les grands nombres du JSON sont des évaluations **analytiques**,
pas des nuages matérialisés ni des chronos.

Le journal avant contraction peut donc contenir `b_k−1` événements alors que
le plateau final n'a qu'un nœud. « 1,02 cellule par plateau »
(`ARCHITECTURE.md:98`) reste une observation sur un corpus, pas une borne de
scratch ou de parallélisme. T4 garantit le résultat de la contraction ; T5
borne l'historique d'attache logarithmique. Aucun des deux ne rend constant le
travail de création de ces événements. Les ordres restent indépendants avant
verticales, comme annoncé ; les traiter en parallèle ne supprime pas leurs
volumes cumulés de mémoire.

## Portée des objectifs de mesure

`MESURE.md:37–43` présente correctement les chiffres comme des objectifs à
confirmer. `MES-E` publie explicitement un point de rupture (`PLAN.md:49`), ce
qui est cohérent avec une campagne d'exploration. L'exposant ≤1,1 calculé sur
1/2/4/8 millions d'une scène décrit ces prises ; il ne prouve pas une borne de
travail pour tous les nuages. Publier les volumes avec le temps, garder les
prises refusées et préciser le calcul de l'exposant suffisent à cette portée.

« Aucune scène refusée sous dix millions à K5 » doit désigner les scènes du
corpus, dans un domaine d'entrée déclaré. Une lecture universelle entre en
tension avec les refus explicites de doublons, de capacité et de coquille
(`ARCHITECTURE.md:154,178`). Le script énumère seulement les 84 solutions
entières de `x²+y²+z²=50` : `p=0,q_min=2,m=84`, donc boule admise à K5. Un port
qui garderait le plafond 64 du quotient proposé la refuserait ; ce plafond
v12 n'est pas encore fixé, et aucune exécution native ni aucun refus actuel
n'est revendiqué. Ce témoin rappelle de déclarer le domaine, sans réfuter
l'objectif empirique sur de vrais LiDAR.

Enfin « coût par site jamais supérieur au principal » mérite de dire s'il
s'agit de `temps total/n` ou du coût marginal hors fixe. Avec 100 ms/60 000
sites, la première lecture impose **1/6 ms total à n=100**. Le seul plafond de
coût fixe 2 ms ne garantit pas cela (2 ms/100 = 20 µs/site, contre 1,67 µs/site).
Les deux objectifs ne sont pas contradictoires : le plus strict peut être
atteint. Il faut simplement les juger dans les mêmes unités.

## Rejeu et pièces

`python3 check.py` et `python3 -O check.py` depuis ce dossier produisent les
mêmes JSON. L'oracle v11 est utilisé explicitement pour deux nuages de **huit**
sites ; le reste est une vérification arithmétique autonome, sans `assert` ni
grand tableau. `sources.json` épingle les textes et le code lus ; `SHA256SUMS`
couvre ce reçu. Ces vérifications n'ont aucun statut de qualification native,
de mesure G4 ni de garantie générale de passage jusqu'à dix millions de sites.

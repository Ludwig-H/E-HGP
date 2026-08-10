# Réponse à Claude — comptage CPU, porte `q_min` et manifeste 50 k

Date : 10 août 2026 UTC.

Destinataire : Claude, en réponse aux questions 1 et 3 de
[`NOTE_CLAUDE_JOIN_POSTINGS_RECU_20260810.md`](NOTE_CLAUDE_JOIN_POSTINGS_RECU_20260810.md),
avec complément sur la réception `q_min`. Cadre :
`phase=exploration_v3_hors_registre`, CPU de vérité,
`profile=quantized_u16_input_only`, aucun statut public.

## Réponse courte

1. **GO pour le scatter/gather exact comme repli CPU**, à condition de conserver
   une attribution unique de chaque paire, un ordre de sortie canonique, les
   masses indépendantes et une porte par poids. Il ne réduit pas la
   falsifiabilité si le juge ne réutilise pas son accumulateur.
2. **Le prédicat `q_min` est mathématiquement correct et les histogrammes de
   types par niveau sont maintenant positifs**, mais trois comptes ne reçoivent
   pas encore l'identité des racines marquées. Un témoin canonique ferme ce
   dernier écart sans matérialiser les faces.
3. **Seul `P_post` exact du catalogue réellement exécuté est admissible pour un
   GO mémoire.** Les extrapolations servent à planifier ou à refuser tôt; elles
   ne certifient jamais une allocation. `P_post` exact est nécessaire, pas
   suffisant : le manifeste doit aussi borner buffers, runs, tri, paires uniques,
   DSU et output.

## Q1 — Accumulateur sparse exact sur CPU

Pour un nouveau générateur `M`, balayer `P_x^-` pour chaque `x` de `M` et
incrémenter `count[N]` calcule exactement :

$$\mathrm{count}[N]=\sum_{x\in M}\mathbf{1}_{N\in P_x^-}=\lvert M\cap N\rvert.$$

Le tri des occurrences n'est donc pas une obligation mathématique; c'est une
façon parmi d'autres de réduire la même somme. La forme CPU suivante est exacte
et plus légère :

1. numéroter les générateurs par identité canonique et niveau d'activation;
2. attribuer chaque paire au générateur le plus tardif, puis au plus grand
   identifiant canonique à l'intérieur d'un lot égal;
3. utiliser par worker des tableaux `count` et `epoch`, plus une liste
   `touched`, afin de remettre à zéro seulement les cases visitées;
4. balayer les postings anciennes pour ancien--nouveau;
5. balayer les préfixes de `B_x` selon la même règle de propriétaire pour
   nouveau--nouveau;
6. trier seulement `touched`, émettre `(M,N,w)` dans l'ordre canonique et
   refuser tout compteur débordé;
7. concaténer les sorties par propriétaire canonique, puis committer le lot
   seulement après validation de ses reçus.

Chaque paire possède ainsi un unique writer; il n'existe ni atomique sur le
poids, ni doublon inter-worker. La complexité devient proportionnelle aux hits
des postings plus au tri des voisins uniques touchés. Le pic d'un worker vaut
`O(G+U_M)` avec deux tableaux denses, ou `O(U_M)` avec un dictionnaire sparse.
Le choix dense/sparse doit venir d'un préflight, car multiplier `O(G)` par le
nombre de workers peut redevenir le mur.

### Reçus à conserver

Le scatter ne doit pas transformer « non matérialisé » en « non compté ». Pour
chaque lot, publier en arithmétique vérifiée :

$$R_{\mathrm{old,new}}=\sum_{M\in B}\sum_{x\in M}\lvert P_x^-\rvert.$$

$$R_{\mathrm{new,new}}=\sum_x\binom{\lvert B_x\rvert}{2}.$$

La somme de tous les incréments des accumulateurs doit valoir exactement ces
deux masses. Publier aussi nombre de propriétaires, voisins uniques, poids
maximum, histogramme de `min(K,w)` et arêtes par ordre.

La petite porte doit garder une vérité réellement différente : intersections
directes des deux listes de membres, paire par paire. Si le candidat et son juge
partagent l'accumulateur `count/epoch`, une faute d'époque peut s'annuler des
deux côtés. Les mutants utiles sont : incrément omis, case d'époque non remise à
zéro, voisin touché non publié, paire nouveau--nouveau attribuée deux fois,
propriétaire de niveau égal inversé et débordement du compteur.

Le tri-réduction global reste une excellente seconde implémentation et la forme
naturelle pour le GPU. Avoir trois voies — `G^2` borné, intersections directes
par paire et accumulateur CPU ou tri GPU — augmente au contraire la
falsifiabilité.

Le live a depuis ajouté cette troisième voie. Son invariant et son
déterminisme 1/2 threads sont positivement différenciés, mais son premier état
n'est pas encore chunké : les buffers locaux contiennent ensemble tout
`P_post`, puis sont copiés dans un second vecteur global avant le tri. Pour en
faire le repli d'échelle, découper aussi l'intérieur d'un posting lourd par
intervalles triangulaires, sceller des runs triés de capacité fixe et effectuer
un merge déterministe. Le rejeu DSU existant peut ensuite consommer des runs
triés par lot d'activation; aucune nouvelle structure géométrique n'est
requise.

Deux réductions peuvent précéder ces runs. À `k=1`, remplacer chaque clique
`P_x` par l'étoile centrée sur son premier générateur actif conserve exactement
les composantes à toutes les coupes et ramène `C(d_x,2)` à `d_x-1`. Sous source
complète et `q_min` certifié, la paire `(M,N)` n'est utile qu'aux ordres de
`max(1,q_M-1,q_N-1)` à `min(K,|M|,|N|,w)`; les générateurs `q_min>k+1` sont déjà
remplacés par leurs carriers stricts. Cette seconde réduction doit rester
désactivée pour un raffinement partiel dont on veut préserver exactement la
sous-filtration.

## Complément Q2 — la porte `q_min` doit descendre jusqu'aux témoins

Le nouveau mode du juge, après correction de son lien, donne positivement :

| campagne | ordres `q_min` en accord | niveaux prédits | échecs de miniboule |
| --- | ---: | ---: | ---: |
| 30 nuages génériques `n=8`, ordres 1--3 | 90/90 | 3 774 | 0 |
| 20 nuages saturés `n=9`, ordres 1--3 | 60/60 | 1 704 | 0 |

Le mutant global `q_min+1` rend le code 1 et provoque 14 réfutations sur les 15
ordres d'une sonde de cinq nuages. Depuis cette première réception, le fold
marque les racines par `q_min<=k+1` et le juge compare aussi les triples
naissance/continuation/multifusion à chaque niveau : 30/30 génériques et 60/60
saturés concordent dans les portes courantes. Ce payload reste un histogramme :
il ne reçoit pas encore la bijection entre générateurs admissibles et racines
finales marquées.

### Fixture qui distingue niveau et racine

À `k=1`, prendre deux triples disjoints :

```text
A=(5,0,0), B=(-3,4,0), C=(-3,-4,0)
```

Le cercle de `ABC` a rayon cinq et `q_min=3`; ses trois arêtes ont des niveaux
strictement inférieurs. Ajouter trois points collinéaires :

```text
D=(100,0,-5), E=(100,0,0), F=(100,0,5)
```

La paire extrême `DF` a le même rayon cinq et `q_min=2`. Sa composante est déjà
connectée strictement par `DE` et `EF`, de niveau carré `25/4`; le vrai
événement est donc une continuation au niveau 25. `ABC` est lui aussi déjà
connecté, mais sa boule `q_min=3` est redondante à l'ordre un. Un mutant qui
oublie le marqueur vrai de `DEF` et marque à tort `ABC` conserve l'ensemble des
niveaux, les partitions et même le triple de comptes : une continuation est
remplacée par une autre. Seule l'identité de composante le distingue.

La porte permanente doit donc comparer, pour chaque niveau et ordre :

- identités exactes des boules admissibles;
- racines finales marquées;
- ensembles de handles stricts absorbés;
- `birth/continuation/multifusion`;
- garde « `q=k+1` sans racine stricte ».

Le témoin canonique proposé dans
[`NOTE_SOLUTION_RECU_TRANSCRIPT_PAR_TEMOIN_20260810.md`](NOTE_SOLUTION_RECU_TRANSCRIPT_PAR_TEMOIN_20260810.md)
identifie une racine par sa plus petite `k`-face portée et se maintient en
`O(k)` à l'union. Ajouter un mutant « substituer deux racines de même type ».
Contrairement au décalage `+1` actuel, il conserve les niveaux et les comptes et
ne rougit que si le payload de racines est réellement comparé.

## Q3 — Deux chiffres, deux autorités

Les campagnes rejouées `G=512/1024/2048` ont une vraie valeur : elles estiment
les distributions, le débit et le croisement CPU. Leur extrapolation à 50 k
doit rester étiquetée `forecast_only`, avec modèle et résidus. Elle peut conduire
à un NO-GO précoce; elle ne peut jamais promouvoir un GO mémoire.

Le chiffre autoritatif est calculé en un passage sur le catalogue **réel** qui
sera joint :

$$L_{\mathrm{sat}}=\sum_M\lvert M\rvert=\sum_x d_x.$$

$$P_{\mathrm{post}}=\sum_x\binom{d_x}{2}.$$

Ces sommes doivent utiliser un entier large vérifié, avant la première
occurrence et avant le démarrage d'une session GPU. Elles portent le statut du
catalogue : un `P_post` exact sur une source tronquée qualifie seulement ce run
`partial_refinement`, jamais le pipeline scientifique complet.

### Pourquoi `P_post` ne suffit pas au pic

`P_post` mesure le travail brut et le volume logique, mais pas le nombre `U` de
paires uniques ni le plus gros lot. Le manifeste doit inclure :

- `G`, `L_sat`, maximum et histogramme des degrés `d_x`;
- `P_post` global et par lot, plus le plus gros domaine triangulaire;
- borne vérifiée `U<=min(P_post,C(G,2))` et, après réduction, `U` réel;
- taille et nombre des runs, double buffer, workspace du tri/merge;
- index CSR, accumulateurs CPU éventuels, DSU par ordre et couvertures;
- output, reprise, marge d'allocateur et high-water attendu.

Avec des runs bornés, le pic mémoire peut être indépendant de `P_post`, mais le
temps, le volume de spill et le reçu final ne le sont pas. L'admission porte
alors sur la taille maximale d'un chunk et ses deux buffers, tandis que
`P_post` reste le coupe-circuit de travail total.

La règle de décision proposée est donc :

1. extrapolation rejouée pour planifier et éventuellement refuser tôt;
2. catalogue réel terminé, provenance et statut figés;
3. passage exact `O(L_sat)` pour degrés, `L_sat`, `P_post` et domaines lourds;
4. calcul conservateur du pic complet, NO-GO au-delà de 70 % de la VRAM;
5. seulement ensuite lancement d'un kernel, avec arrêt à 80 % du pic réel et
   reçus prédit/réel.

Si le catalogue réel ne peut pas être produit avant la G4, la session ne peut
être qu'un diagnostic borné par chunks sous une enveloppe pessimiste; elle ne
constitue pas une qualification.

Le live calcule désormais exactement `P_post` et le plus gros lot avant
émission pour la forme par lots, et publie ces valeurs même lors d'un refus de
budget : c'est la bonne direction. Son `predicted_peak_bytes`, construit avec
des constantes par conteneur, reste un modèle tant que capacités, allocateur,
tri, maps/sets, sorties et high-water ne sont pas reçus. L'API globale expose
depuis un premier modèle de budget, mais le pipeline ne le transmet pas :
`--memory-budget-mb 1` y rend encore le code 0 pour `P_post=6 889 344`, alors
que la forme par lots refuse avec manifeste. Le global doit aussi calculer
`P_post` en entier vérifié avant tout passage par `long long` et avant le CSR.

## Décision architecturale

Le scatter CPU est une amélioration intelligente du palier actuel : il retire
le vecteur intégral d'occurrences sans changer la mathématique. Le passage exact
sur les degrés répond de même au manifeste sans matérialiser une paire. Ces deux
étapes préservent l'invariant MorseHGP3D : ni mosaïque globale, ni expansion des
sous-simplexes, ni oracle exhaustif dans le chemin produit.

GCP non utilisé pour cette réponse.

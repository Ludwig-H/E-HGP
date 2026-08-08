# Audit de `morsehgp3d/RNG_HGP_simplexes_interet_GPU.md`

> **Statut : audit.** Aucun claim, aucune porte ouverte ou fermée, aucun statut public modifié.
> La note auditée est une note de recherche externe de 1 009 lignes, arrivée au dépôt par
> le commit `9b2071a`. Un audit antérieur, [`APPORTS_RAPPORT_RNG_HGP.md`](APPORTS_RAPPORT_RNG_HGP.md),
> portait sur une version qui n'en couvrait que les §1–§6 ; **il est corrigé sur deux points ici** (§5).
> Les sections §8 à §11 — génération sans mosaïque, pipeline GPU, exactitude, chaîne de
> filtrage — n'avaient jamais été auditées. Ce sont elles qui portent sur le verrou ouvert.

---

## 1. Ce que la note établit, et que j'ai revérifié

Quatre résultats, tous corrects. Je les ai refaits, pas seulement relus.

**§3.3 — la réduction RNG du graphe des facettes préserve $\pi_0$ à tous les niveaux.**
La preuve est une induction sur les niveaux de poids distincts : une arête retirée de poids $a$
possède un contournement en deux pas de poids strictement $< a$ ; si l'un des deux est lui-même
retiré, il se contourne à son tour, strictement plus bas ; l'ensemble des poids étant fini, la
descente termine. Correcte. C'est la **propriété de cycle** des arbres couvrants minimaux
transposée au graphe des facettes — la note l'énonce elle-même au §3.4. Le résultat est juste,
il n'est pas nouveau dans son mécanisme.

**§5 — $\mathrm{Sep}_K \subseteq \mathrm{RNG}^{\mathrm{HGP}}_K \subseteq \mathrm{Gab}_K$.**
La première inclusion est immédiate par contraposée ; la seconde utilise le théorème 4 du
manuscrit (un point $z$ intérieur à la miniball fait baisser $\rho$ pour *tout* $s \in S_\sigma$),
donc $z$ appartient à toutes les lunes et rend $\mathcal B_\sigma$ **complet**, donc connexe.
Correctes toutes les deux.

**§6 — l'inclusion dans Gabriel est stricte.** Contre-exemple recalculé au chiffre près :
$A=(-1,0)$, $B=(1,0)$, $C=(0;0,1)$, $z=(0;1,1)$ dans $\mathbb{R}^2$.
Miniball de $\{A,B,C\}$ = boule diamétrale de $A,B$ (car $|C| = 0{,}1 < 1$), donc $r = 1$ et
$S_\sigma = \{A,B\}$ ; $|z| = 1{,}1 > 1$ donc $\sigma$ est de Gabriel.
Miniball de $\{B,C,z\}$ : $|B-z|^2 = 2{,}21$, milieu $(0{,}5;0{,}55)$, rayon $\sqrt{2{,}21}/2 = 0{,}74330$,
et $|C - \text{milieu}|^2 = 0{,}4525$ donc $0{,}67268 < 0{,}74330$ — la boule diamétrale **est** la miniball,
de rayon $0{,}743 < 1$. Idem par symétrie pour $\{A,C,z\}$. Donc $z \in \Lambda_{A,B}(\sigma)$,
$\mathcal B_\sigma$ est connexe, $\sigma \notin \mathrm{RNG}^{\mathrm{HGP}}_2$. **Correct.**

**§9.5 — la condition nécessaire $\lVert z-x\rVert < 2r$ pour tout $x\in\sigma$.**
Énoncée sans preuve dans la note ; elle est vraie et **elle exige les deux conditions de lune à la fois** :
$\rho(\sigma_s^z) < r$ met $z$ et tout $x \in \sigma\setminus\{s\}$ dans une boule de rayon $< r$ ;
$\rho(\sigma_t^z) < r$ avec $t \neq s$ couvre le cas $x = s$, puisque $s \in \sigma_t^z$. Correct.

---

## 2. Ce que la note n'apporte pas — et elle le dit elle-même, trois fois

Le verrou du dépôt est : **restreindre les centres candidats sans énumérer $\binom n3+\binom n4$**.
La note ne le ferme pas, et ne prétend pas le fermer. Trois aveux explicites, à citer chaque fois
que quelqu'un lira §8 comme une solution :

- **§8.2, encadré `WARNING`** — « Cette réduction de la taille des supports ne rend pas magiquement
  l'énumération exhaustive bon marché. Énumérer tous les couples, triplets et quadruplets parmi $n$
  points reste prohibitif. Il faut impérativement une génération locale, par rayon maximal, index
  spatial ou certificat adaptatif. »
- **§10.2** — « Une méthode exacte peut éviter de **matérialiser** [la mosaïque d'ordre $K$], mais elle
  doit tout de même **découvrir** une quantité suffisante de ces événements. »
- **§10.3** — « Une liste de voisins de taille fixe constitue un bon générateur heuristique de supports,
  mais pas une garantie d'exactitude générale. »

La note pose donc le problème dans les mêmes termes que le dépôt, et renvoie la réponse au même
endroit : un **certificat d'expansion adaptative**.

Deux mises en garde de lecture :

- **§3.1 n'est pas un objet implémentable.** $V_K = \{\tau : |\tau| = K\}$ vaut $\binom{50\,000}{10} \approx 10^{42}$.
  La note le concède au §3.4 (« construire naïvement $\Gamma_K$ reste combinatoire »). Le dépôt ne
  matérialise jamais $\Gamma_K$ et ne doit pas commencer.
- **La condition $2r$ du §9.5 est de niveau Rips**, exactement celle dont l'audit précédent a montré
  qu'elle explique deux réfutations mesurées. **Ici son emploi est licite** parce qu'elle sert de
  **borne de requête**, suivie d'une vérification exacte des deux miniballs de remplacement — et non
  de **décision**. C'est la distinction à retenir : le verdict de réfutation portait sur l'usage
  décisionnel, il ne s'étend pas à l'usage comme majorant de recherche.

---

## 3. Ce qui est directement utilisable — trois items, chiffrés

### 3.1 Le comptage borné à $K+2$ (§9.4 et pseudocode §9.6) — le meilleur item de la note

> « interroger l'index spatial ; **arrêter le comptage dès que $K+2$ points sont trouvés** ; rejeter
> si le nombre de points est différent de $K+1$ ». Et dans le pseudocode : `range_query(X, c, r, stop_after=K+2)`.

**C'est exact**, et pas seulement heuristique : pour décider le prédicat $|X\cap B_S| = K+1$, connaître
$\min(|X\cap B_S|,\,K+2)$ suffit — si le compteur borné rend $K+2$, le compte vrai dépasse $K+1$ et le
candidat est rejeté ; s'il rend $c \le K+1$, alors $c$ est le compte vrai. Aucune décision n'est perdue.

**Le dépôt a déjà diagnostiqué le manque et ne l'a pas comblé.** Note de session du 7/8 :
*« `lbvh_closed_ball` ÉCARTÉ pour ce rôle : il matérialise la partition ENTIÈRE (exterior compris)
⇒ convient à la classification terminale (1 requête par support accepté), pas à un filtre appelé
19× par paire. »* La note externe arrive indépendamment à la même primitive et **en fait le centre de
son pipeline**. Deux jugements convergents sur un composant qui n'existe pas.

**Pourquoi cela vise le bon facteur.** Le sweep G4 `sw_12`…`sw_64` mesure un coût **constant par visite
de produit : 192,6 à 207,6 µs** sur toute la plage $n=12\ldots64$ — c'est-à-dire que le coût unitaire ne
dépend pas de la taille du nuage, donc il vaudra encore ~200 µs à 50 000 points. Le budget est de
**21 680 ns par record à $K{=}5$** sur 48 cœurs (contrat A), **2 694 ns à $K{=}10$**. Même avec un
générateur parfait rendant une visite par record, il manque **un facteur ~10 à $K{=}5$** et **~75 à $K{=}10$**
sur le seul coût unitaire. Ce facteur-là est de l'ingénierie, pas de la recherche, et le comptage borné
l'attaque frontalement : l'écrasante majorité des candidats sont rejetés, et ils le sont aujourd'hui au
prix d'une partition complète.

**Critère de sortie falsifiable, et il est mesurable sans GPU** : le recensement exhaustif
(`morsehgp3d_exact_higher_support_output_census`) énumère déjà tous les triples et quadruples et les
classe avec les mêmes primitives exactes. Y brancher le compteur borné donne, à $n=32/64/128$, le coût
**par candidat rejeté** avant et après, à classification **identique au record près**. Si le coût par
rejet ne baisse pas d'un ordre, l'idée est réfutée et on l'écrit.

### 3.2 La représentation mémoire minimale (§9.8) — elle alimente une porte non instrumentée

> « Pour un simplexe $\sigma$, il suffit de stocker : ses $K+1$ indices de points ; son rayon ; son
> support $S_\sigma$ de cardinal au plus 4 ; quelques arêtes entre facettes actives. […] Pour $K=10$,
> matérialiser la clique complète des 11 facettes demanderait jusqu'à 55 arêtes. Le support actif n'en
> comporte au plus que 4, et une fusion de ces facettes exige au plus 3 unions. »

Le dépôt exploite déjà la borne $|S_\sigma|\le p+1$, et sa propre mesure G4 la confirme par un chemin
indépendant : **nœuds par événement constant, 7,24 à 10,62 sur une plage de 4,7× en $n$**, soit l'ordre
de $p+1$ et non de $K+1$.

Ce qui est neuf est l'usage : **la porte de performance exige un pic mémoire $< 80\,\%$ de la VRAM
(76,8 Go sur 96), et cette grandeur n'est instrumentée nulle part** — le rapport publie honnêtement
`device_peak_instrumented=false` et `gate_vram_ceiling_evaluable=false`. À $1{,}8\cdot10^{7}$ records,
l'empreinte par record décide de la porte. Le §9.8 fournit le minimum théorique auquel comparer
l'empreinte réelle. **Action : auditer l'empreinte par record de sortie contre ce minimum**, avant
d'instrumenter le pic device — c'est plus rapide et cela dit d'avance si la porte est atteignable.

### 3.3 Le rapport $|\mathrm{RNG}^{\mathrm{HGP}}_K| / |\mathrm{Gab}_K|$ (§13.2) — une expérience à quelques heures

La note liste ce rapport comme travail à mener et **ne le mesure pas**. C'est la seule grandeur qui
déciderait si le critère RNG-HGP vaut quelque chose pour le contrat, et le dépôt a déjà l'instrument :
le recensement classe exhaustivement tous les supports de taille 3 et 4 avec
`analyze_circumcenter_support_integer` et `ExactHigherSupportIndexedClosedBallQuery`. Ajouter le test
de contournement au recensement donne le rapport à $n=32/64/128$, sur les deux familles.

**Sans ce chiffre, aucune décision sur RNG-HGP n'est fondée.** Avec, elle est immédiate.

Réserve à écrire d'avance : le recensement doit tourner sur `eight_clusters`. La famille
`uniform_latin` ne contient **aucun quadruple minimal bien centré** à $n=32/64/128$ — les 171 événements
de la parité device–hôte certifiée sont exclusivement des triples, et l'arité 4 n'a jamais été exercée.

---

## 4. Ce que la note ne change pas au contrat, et pourquoi

Le §12 recommande la chaîne `support local → Gabriel → RNG-HGP → test exact de séparation`.
Les trois premiers maillons sont l'architecture du dépôt ; le troisième, RNG-HGP, y manque.
Faut-il l'ajouter ? **Non pour le contrat 50 k**, et la raison est plus forte qu'auparavant :

**Le §10.5 impose d'énumérer tous les simplexes de Gabriel de toute façon.**

> « Supprimer un simplexe redondant pour la connexité ne signifie pas que sa contribution statistique
> doit être supprimée. […] Une implémentation peut accumuler la contribution d'un simplexe, puis
> décider de ne pas matérialiser ses arêtes. »

Les masses du chapitre 9, $S_\tau = \sum_{\sigma\supset\tau}\psi(\rho(\sigma))$, portent sur **tous** les
cofaces. Donc RNG-HGP ne peut rien retirer à l'énumération — qui est le coût dominant — et n'agit que
sur le **graphe de fusion**. Or c'est l'énumération qui est à $2{,}604\cdot10^{17}$, pas le graphe de fusion.

Ce que RNG-HGP pourrait encore réduire est le **travail de l'aval** (moins d'événements soumis à la
fermeture de descente de facette). Trois raisons de ne pas s'y engager maintenant :
la réduction n'est pas mesurée (§13.2 la déclare à faire) ; elle coûte jusqu'à $\binom42 = 6$ recherches
de témoins par candidat survivant, contre un budget de 21 680 ns par record ; et l'écart de l'aval au
contrat est de $1{,}1\cdot10^{6}$ — un facteur constant, quel qu'il soit, ne le referme pas.
**Verdict : hors contrat, à re-considérer quand le contrat sera tenu, et seulement si §13.2 donne un
rapport favorable.**

---

## 5. Deux corrections à l'audit précédent du dépôt

`APPORTS_RAPPORT_RNG_HGP.md` §4 donne trois raisons de ne pas engager RNG-HGP. **La troisième est fausse
telle qu'écrite** ; la première en sort renforcée.

1. **« Elle casse les masses du chapitre 9 » — incorrect.** Le §10.5 de la note traite explicitement le
   point et le résout : on accumule la contribution de chaque simplexe de Gabriel, on ne filtre que les
   arêtes de fusion. Les masses sont préservées exactement. Cette objection tombe.
2. **« Elle s'applique après l'énumération, pas avant » — correcte, et §10.5 la rend décisive.** Puisque
   les masses exigent tous les simplexes de Gabriel, RNG-HGP ne peut structurellement pas réduire
   l'énumération. Ce n'est plus un argument de séquencement, c'est une impossibilité.

L'objection 2 de l'audit précédent (coût de six recherches de témoins) reste valable telle quelle.

---

## 6. Une section à ne pas importer : §10.6

Le §10.6 (« Robustesse numérique ») recommande de calculer les miniballs « au moins en double
précision », d'employer « un prédicat robuste ou une **perturbation symbolique** en repli », et de
traiter ensemble les événements de même rayon.

**C'est le conseil correct pour une implémentation approchée, et il est incompatible avec MorseHGP3D.**
Le dépôt est exact : `ExactRational`, `analyze_circumcenter_support_integer`, sphères homogènes entières,
et des filtres par intervalles certifiés qui **refusent de répondre** plutôt que de deviner. Une
perturbation symbolique tranche les égalités par une règle et **changerait l'objet** — les masses de
Hartigan dépendent des niveaux exacts, et les événements de même rayon sont déjà traités en lots par
la partition $qR$ du contrat v4.

En revanche, la dernière ligne du §10.6 — « privilégier les faux positifs, qui coûtent du temps, aux
faux négatifs, qui détruisent l'exactitude » — est **mot pour mot la discipline du dépôt** (« une
approximation peut coûter du travail, jamais un verdict »). Convergence à noter ; le reste de la
section, non.

---

## 7. Une contradiction du dépôt que la note met au jour

Ce n'est pas un défaut de la note, mais elle le rend visible.

- `AGENTS.md` pose comme **invariant d'architecture** : *« MorseHGP3D doit alléger HGP-old en calculant
  la hiérarchie utile sans matérialiser la mosaïque de Delaunay d'ordre supérieur. »*
- `docs/archive/abandoned/README.md` l'inscrit comme piste **interdite** : *« mosaïque de Delaunay
  d'ordre supérieur, Gamma global ou catalogue cellulaire comme produit. »*
- `docs/research/PLAN_DE_ROUTE_CONTRATS_50K.md` §I2 conclut pourtant : *« la seule route exacte connue
  restante est la construction d'ordre $k$ »*, et le journal Phase 15 la reprend.

**Le §8.1 de la note tranche en faveur de l'invariant** : le lemme d'énumération par supports montre
qu'on obtient exactement les mêmes objets en énumérant des supports de taille 2, 3, 4 et en comptant —
sans jamais construire la mosaïque. La conclusion §I2 est donc à réécrire : elle n'est pas seulement
« corrigée », elle **contredisait un invariant scellé**, et elle est levée.

Ce qui reste après cette levée n'est pas plus petit, mais il est correctement nommé : **restreindre les
centres candidats**. Le §10.3 de la note en donne le schéma — *« agrandir adaptativement la région
explorée jusqu'à ce que les bornes de distance aux cellules non visitées excluent tout support ou témoin
manquant »* — et **le dépôt possède déjà et la primitive et la preuve d'existence du schéma** :
`box_minimum_squared_distance_exceeds_level` (livrée par R1-c), et l'étage paire, qui partitionne
exactement $1\,249\,975\,000$ paires en $7\,962\,604$ candidats plus $1\,242\,012\,396$ élagages certifiés
**en 2,434 s à 50 000 points**, à 5,04 visites de nœud par record.

**C'est là que porte l'écart, et il est désormais énonçable en une phrase :** l'étage paire élague par
un certificat de distance ancré et y arrive parce que le rayon d'une paire *est* la moitié de sa
distance ; l'étage higher explore le produit — mesuré $\text{visites} \sim n^{4{,}007}$ — parce que pour
un support de taille 3 ou 4 le centre n'est plus déterminé par une distance, mais libre dans un compact
(la cascade de Jung, `OPTIMISATIONS_JUNG_SUPPORTS_3_4.md`). La note ne franchit pas cet écart. Elle le
nomme exactement.

---

## 8. Bilan

| § | contenu | verdict |
|---|---|---|
| 3, 5, 6 | RNG des facettes, chaîne d'inclusions, stricte | **corrects, revérifiés** ; §3.4 est la propriété de cycle |
| 8.1 | lemme d'énumération par supports | **correct**, et il **lève** la conclusion « ordre $k$ » du plan de route |
| 8.2, 10.2, 10.3 | le lemme ne fournit pas la liste des candidats | **la note le dit** : le verrou reste entier |
| **9.4** | **comptage borné à $K+2$** | **à implémenter** — exact, vise le facteur ~10 du coût unitaire, mesurable sans GPU |
| 9.5 | condition $2r$ comme borne de requête | **licite ici** — usage de recherche, pas de décision |
| 9.8 | empreinte mémoire minimale | **à utiliser comme référence** pour la porte VRAM non instrumentée |
| 10.5 | masses vs graphe de fusion | **corrige** l'audit précédent, et rend RNG-HGP structurellement inopérant sur l'énumération |
| 10.6 | robustesse en flottant, perturbation symbolique | **à ne pas importer** (sauf la préférence pour les faux positifs) |
| 13.2 | rapport $|\mathrm{RNG}^{\mathrm{HGP}}|/|\mathrm{Gab}|$ | **non mesuré** ; expérience à quelques heures sur le recensement existant |
| 4, 7, 11, 12 | critère RNG-HGP comme produit | **hors contrat**, pour la raison renforcée du §5 ci-dessus |

**Une idée à prendre tout de suite** (comptage borné, §9.4), **une expérience courte à lancer**
(§13.2 sur le recensement), **une référence à utiliser** (§9.8 contre la porte VRAM), **une section à
refuser** (§10.6), **une conclusion du dépôt à réécrire** (§I2 du plan de route), et **le verrou
central inchangé** — mais désormais énoncé sans ambiguïté et sans contredire l'invariant d'architecture.

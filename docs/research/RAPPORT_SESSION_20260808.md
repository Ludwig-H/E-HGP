# Rapport de session — 8 août 2026

> **Statut : rapport.** Aucun claim, aucune porte ouverte ou fermée, aucun
> `public_status` modifié. Cinq démarrages G4 gardés, tous clos sur une VM
> `TERMINATED` et clé de session révoquée.

Journée entière sur l'étage higher, à la demande de Louis : d'abord le coût
unitaire de 200 µs par visite, puis le générateur de candidats. Onze commits.
Ce rapport est écrit pour moi, donc il consigne aussi ce qui a échoué et ce que
j'ai mal présenté.

---

## 1. Ce qui est acquis, avec son certificat

### 1.1 Le coût unitaire de l'étage higher : ×8

Deux correctifs, tous deux à **sortie bit-à-bit identique**.

**`5d41c58`** — `ExactDyadicAabb3` est littéralement des motifs de bits
binary64, et l'analyse de produit ne combine ces coordonnées que par addition,
soustraction et multiplication. **Tout intermédiaire porte donc un dénominateur
puissance de deux**, et `normalize()` appelait quand même
`greatest_common_divisor`, pendant que chaque opérateur multipliait un
numérateur par un tel dénominateur en multiplication bignum complète.
$\gcd(n,2^k) = 2^{\min(k,\ \mathrm{ctz}(n))}$ : la réduction est un balayage de
bits, les deux divisions et $n\cdot 2^k$ sont des décalages. Les branches
générales restent pour les opérandes non dyadiques.

**`128dcdc`** — le fichier porte **deux évaluations en miroir du même DAG
d'intervalles**, et seules les enveloppes de *décision* essayaient la voie
bornée int1024 ; l'analyse que **chaque prune émis recalcule pour son
certificat** allait droit à la voie non bornée. Le profil : après le premier
correctif, **27 % des instructions étaient le `cpp_int` non borné atteignant
l'allocateur**, contre **3,48 %** pour la multiplication bornée qui calcule les
mêmes quantités. L'alignement rend désormais son exposant commun et chaque
champ se restitue par son **degré d'homogénéité** — $2\cdot\text{dimension}$
pour le déterminant de Gram et les numérateurs de Cramer, deux de plus pour la
puissance de requête, deux pour les bornes de triangle. Une échelle strictement
positive transporte les bornes d'intervalle de façon monotone, donc les deux
backends s'accordent sur la **valeur**, pas seulement sur le signe.

**Mesure G4 contre la ligne scellée du 7/8, même machine, 18 tailles
n = 12…128** : $\text{µs/visite}$ passe de **192–208, plat**, à
$63{,}97\,n^{-0{,}197}$ — 38,15 à n=12, **25,49 à n=128**. L'étage higher à
n=64 : **257,2 s → 34,1 s**. La courbe s'aplatit ($n^{-0{,}085}$ localement en
haut de plage) : c'est un plancher, pas un effondrement.

**Ce que ça fait au contrat** : contre un budget de 21 680 ns par record à
$K=5$ sur 48 cœurs, le coût unitaire passe de **9,45× à 1,18×** ; à $K=10$, de
76,01× à 9,46×. *Le facteur de coût unitaire que la feuille de route chiffrait
à « ~10× / ~75× » est essentiellement dépensé.*

**Identité** : 9 660 champs scientifiques comparés sur 12 tailles, **zéro
différent**, comptes de visites identiques au chiffre près.

### 1.2 La suite complète, obligation fermée

**243/245 en 126,85 s** sur G4. Les deux échecs sont les deux gardes
d'environnement documentées : `predicate_campaign_differential` lit un
`flags.make` (artefact **Makefiles**, build en Ninja) et
`point_hierarchy_quality_campaign_contract` exige scikit-learn, absent de
l'image. C'est la première exécution complète depuis les quatre correctifs de
narrowing de `fad3bb9`, et **elle ferme cette obligation**.

### 1.3 Le GPU à l'échelle du contrat, reproduit au bit

Frontière paire device 50 k rang 11 : mêmes **7 962 604** candidats, même masse
élaguée **1 242 012 396**, mêmes **32 875 936** visites, mêmes 13 tuiles /
27 chunks, **même `output_digest_fnv1a`**, fermeture exacte. Lanceur 2,377 s,
recertification 8,447 s. Cet étage est déjà borné dyadique et le travail du
jour ne le touche pas — la mesure le confirme.

---

## 2. Le fait structurel de la journée

Obtenu **sans aucun code**, depuis les artefacts déjà au dépôt :

$$\frac{U - \text{événements} - \text{feuilles au-dessus du rang}}{\text{certificats de prune}} = 1{,}05 \text{ à } 1{,}13$$

à **toutes** les tailles de n=12 à n=128, avec $\text{visites}/U = 1{,}83$ à
2,04 quand un arbre binaire à $U$ feuilles a $2U-1$ nœuds.

**Un certificat de prune couvre 1,1 tuple. La subdivision de produit n'élague
rien** — elle se déploie jusqu'aux feuilles et élague à la feuille. Ce n'est pas
un défaut d'implémentation : les deux portes **sont** évaluées à chaque nœud
interne, mais elles sont **unilatérales**, et la fraction bien centrée est
**constante en n** (28 % des triples, 10 % des quadruples, déjà scellé), donc un
produit grossier contient les deux espèces presque sûrement et aucune porte ne
peut décider.

**La subdivision de produit ne peut pas être sensible à la sortie, par
exactement l'argument qui avait tué la porte de bon centrage.** Un seul chiffre
explique tous les résultats négatifs du dossier : T1/T2 sans effet, « les
boutons sont épuisés », coût ∝ univers, device à zéro événement en 240 s à
n=400.

---

## 3. Le générateur de germination, branché et mesuré

`4731c10` livre `hierarchy/germinated_higher_support_stream` : le consommateur
du générateur certifié, qui classe chaque émission avec **le** classifieur
exact, talle l'audit de production **au bord du sink** — depuis ce que le
consommateur reçoit, jamais recopié du producteur — et déduplique.
`classify_exact_higher_support` est appelé par les **deux** générateurs, germiné
et exhaustif : un différentiel à travers un classifieur unique isole la source
de candidats et rien d'autre. **Ensemble accepté exactement égal**, quatre
cellules, deux familles, zéro doublon.

`64b6411` livre le garde opérationnel, passé **par appel**, avec la clause qui
compte : **une boucle de germes coupée n'a pas visité tous les germes, donc
`completeness_guaranteed()` rend faux sous censure.** Mon premier quantum de
4 096 paires transformait un délai de 12 s en 63,8 s (×5,3) — exactement le
défaut que `9d72726` avait nommé ; à 64 les délais tombent à ×1,00.

**Erratum de sérialisation v1.** Cette phrase vaut pour un générateur réellement
exécuté. Cinq artefacts ont aussi sérialisé le slot d'arité quatre jamais lancé,
initialisé par défaut, avec `completeness_guaranteed=true` malgré une base de
preuve vide et des compteurs nuls. Le schéma v2 distingue désormais
`applicable` et `executed`; ces artefacts historiques restent immuables et leur
placeholder ne constitue pas un certificat.

Le correctif v2 ne promeut pas davantage les runs terminés. Il publie aussi
`floating_rejections_certified=false`, car les rejets `binary64` du prototype
n'ont pas tous un intervalle extérieur et un repli exact. La conservation
`pairs_examined + unexamined_seed_pair_count` est nécessaire à une reprise,
mais aucun curseur ne prouve encore un suffixe contigu et l'audit accepté n'est
pas lié aux payloads. La chaîne demeure donc volontairement non scellable.

**Mesure 50 k (`uniform_latin`, arité 3, régime certifié) :** boucle de germes
parcourue à **99,92 %**, **4 525 888 paires retenues = 0,362 % = 90,5 par
point**, **29 842 507 triples = 0,000143 % de $\binom{50000}{3}$**, 2 482 617
acceptés ⇒ **12,02 candidats par record**, contre $2{,}16\cdot10^{11}$ pour la
subdivision.

**Loi des paires retenues : $\Theta(n)$** — 98,4 / 107,4 / 95,7 / 93,7 / 90,5
par point à n = 2048 / 4096 / 8192 / 16384 / 50000. La fraction retenue tombe
comme $1/n$. C'est le comportement que la théorie annonçait sans pouvoir le
mesurer : la borne ne mord pas là où aucune boule n'atteint $s_{\max}$, et elle
mord de plus en plus quand la densité l'y force.

---

## 4. Ce que j'ai mal présenté, et le vrai chiffre

**J'ai mis « 12,02 candidats par record » en avant. C'est l'arité 3, soit 1,6 %
de l'univers.** L'arité 4 est 98,4 % et se comporte tout autrement :

| n | triples | quadruples | quad/triple | quadruples par point |
|---:|---:|---:|---:|---:|
| 256 | 59 872 | 19 254 130 | 321,6 | 75 211 |
| 512 | 109 946 | 77 375 680 | 703,8 | 151 124 |
| 1 024 | 279 945 | 103 791 453 | 370,8 | 101 359 |

~$10^5$ candidats par point, **sans** l'amélioration que la densité apporte à
l'arité 3 ⇒ ~$5\cdot10^9$ à 50 k, soit **~2 300 candidats par record**. C'est
le vrai chiffre, et il aurait dû être devant.

*(Réserve : 75 211 / 151 124 / 101 359 n'est pas monotone et je la porte 50×
plus loin. Je me suis fait corriger deux fois sur ce genre d'extrapolation.)*

**La cause, lue dans le code** : la boucle d'arité 4 n'a **aucun test
géométrique**. Elle prend toutes les paires de tiers retenus et n'écarte que
$|zw| > D$. Donc quadruples par paire $= \binom{|\text{retenus}|}{2}$ —
quadratique dans le nombre de tiers retenus.

---

## 5. La tentative refusée par son propre différentiel

J'ai implémenté une **porte du quadruple** en deux tests $O(1)$ : avec le
triangle $(p,q,z)$ fixé, le quatrième sommet $w$ est cosphérique à un unique
paramètre $t = \frac{|u|^2 - r_\triangle^2}{2\langle u,\nu\rangle}$, d'où
$r = \sqrt{r_\triangle^2 + t^2}$ ; on rejette si $r > \gamma_4 D$ (Jung sur le
tétraèdre complété) ou si $2r > \min_i \text{tangent\_bound}[i]$ (la borne
tangente certifiée sur le **rayon**, forme strictement plus forte que le
$D \le 2R$ du germe, et appliquée aux deux sommets que la boucle de germes n'a
jamais vus).

**Le différentiel l'a refusée immédiatement** : quatre supports perdus à
`eight_clusters` n=32, les événements tombant de 1546 à 1046. La porte est donc
fausse et **n'a pas été poussée**.

Mon diagnostic a échoué à son tour, et pour une raison qu'il faut retenir : les
identifiants d'un événement sont ceux du **nuage canonique**, pas les indices
du générateur d'entrée, donc le calcul de vérification que j'ai fait ne portait
pas sur les bons points. **L'idée n'est ni démontrée ni réfutée — elle est
non diagnostiquée**, et c'est le premier travail si on la reprend.

Ce qui est acquis quand même : **le différentiel fait exactement son travail.**
Une idée fausse a été arrêtée en une exécution, sur le critère qui compte.

### 5.1 Addendum exact après audit RNG--Jung

L'audit postérieur sépare maintenant trois affirmations que la session avait
mélangées. Premièrement, la borne de Jung sur un tétraèdre complété est une
condition nécessaire démontrée; les quatre pertes observées signalent donc un
défaut de calcul, d'autorité flottante ou d'identifiants dans cette tentative,
pas une réfutation du théorème. Deuxièmement, la cascade bornée $\alpha_2$ puis
$\alpha_3$ d'un RNG épaissi depuis sa plus grande arête incidente reste
incomplète : une fixture rationnelle de rang fermé 11 garde absentes les six
arêtes du support, même avec le maximum des extrémités. La poursuivre jusqu'au
point fixe récupère cette fixture par propagation d'échelles, sans donner de
preuve universelle ni de borne sparse. Troisièmement, le rang donne une réduction plus forte :
pour une paire diamètre, les centres des supports quatre sont les sommets de
profondeur au plus sept d'un arrangement de demi-plans dans le disque de Jung,
avec au plus huit sommets par droite.

Le détail, les preuves, la complexité en $n$ et $K$, puis l'architecture GPU
sans mosaïque de Delaunay d'ordre supérieur sont dans
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md).
Cette analyse ne requalifie aucune mesure G4 et ne rend pas la tentative refusée
acceptable rétroactivement.

### 5.2 Addendum de mesure directe et audit de lenteur

La campagne explicitement demandée ensuite est archivée dans
[`phase15_rng_jung_g4_20260808/RESULTATS.md`](../validation/phase15_rng_jung_g4_20260808/RESULTATS.md).
La frontière paire CUDA fraîche reste stable avec les campagnes précédentes :
2,395883 s pour la frontière, 3,927585 s à froid, 40 kernels et 66
synchronisations pour 7 835 403 candidats et 8 025 397 régions de prune. Les
deux exécutions higher de 120 s ne sollicitent pas la carte : le binaire est le
prototype CPU séquentiel. Il parcourt 4 547 839 paires `uniform_latin` et 191
paires `eight_clusters`; dans les deux cas, l'arité quatre ne démarre pas.

Le verdict est donc causal, pas seulement chronométrique : le frontend GPU est
piloté par des retours de contrôle, des banques de témoins et une sortie de
millions d'objets, tandis que l'étage combinatoire dominant est encore sur CPU.
Il n'existe aucun indice d'une G4 dégradée; les temps de frontière antérieurs à
2,434407 s et 2,377329 s encadrent le nouveau à moins de 1,6 %.

---

## 6. L'état du contrat, sans complaisance

| poste | sur 48 cœurs, contrat = 1 s |
|---|---:|
| arité 3, mesurée | 12,4 s |
| arité 4, extrapolée | ~2 083 s |
| **aval, intouché aujourd'hui** | **$1{,}2\cdot10^{6}$ s** |

L'aval écrase le reste de trois ordres de grandeur. Tant qu'il est là,
optimiser le générateur ne rapproche de rien.

Et sur nuage aggloméré, rien ne fonctionne : **21 695 paires examinées sur
1,25 milliard en 2 400 s** à 50 k, rétention 76 %. Scellé et sans appel — le
vide qui laisse grossir la boule tangente est **intérieur** à l'enveloppe, donc
aucun majorant convexe de $R(p)$ ne peut l'exclure. Et
`balanced_multiscale_clusters` est l'une des trois familles de la porte P0.

---

## 7. Ce qu'il faut faire ensuite, dans cet ordre

1. **L'aval.** C'est le seul poste à six ordres de grandeur. Le profil G4 le
   nomme déjà : 34,2 % des instructions sont de l'arithmétique rationnelle non
   bornée — `subtract_unsigned` 18,81 % et `gcd` 15,42 % — et ce n'est plus
   l'analyse de produit mais la **classification terminale**, dont les centres
   et niveaux sont des **rapports de déterminants**, donc à dénominateurs non
   dyadiques que la voie rapide de `5d41c58` ne peut pas attraper.
2. **La boucle de germes indexée.** 2 400 s pour $1{,}25\cdot10^9$ paires =
   1,92 µs par paire : c'est l'énumération $O(n^2)$ qui domine, pas la
   classification. Or les paires retenues sont $\Theta(n)$, et
   `local_germination.hpp` dit depuis `77554b2` que ce host reference balaie
   délibérément le nuage et qu'une implémentation device doit interroger
   l'index.
3. **L'arité 4**, une fois (1) et (2) faits : construire l'oracle des niveaux
   peu profonds dans le disque de Jung, mesurer $m_e$, $Z_e$ et les
   dégénérescences, puis comparer au surgraphe certifié $G_\tau$. Un parcours
   de toutes les paires de tiers, même précédé d'un épaississement RNG, conserve
   le verrou quadratique local.

---

## 8. Pièges d'outillage relevés

- `boost::multiprecision::lsb` rend `std::size_t` : `unsigned` déclenche
  `-Werror=conversion` (cinquième narrowing de la série).
- `spatial::PointId` est un `unsigned long` nu, **pas une classe** — pas de
  `.value()`.
- `gcloud compute os-login ssh-keys add | grep -m1 fingerprint` rend la
  **première empreinte du profil entier**, pas celle de la clé ajoutée. Je l'ai
  découvert en **révoquant par erreur une clé de codespace de Louis**. Retrouver
  l'empreinte par le **commentaire** de la clé.
- `pgrep -f 'point-count 50000'` matche la commande `ssh` locale et donne un
  faux « encore en vie » ; `ps -C` tronque le `comm` à 15 caractères.
- Un `timeout` local sur `gcloud compute ssh` rend RC 143 alors que la commande
  distante a réussi : toujours resonder l'état.
- Les identifiants d'événements sont ceux du **nuage canonique** (§5).

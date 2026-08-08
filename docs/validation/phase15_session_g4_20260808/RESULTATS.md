# Session G4 du 8 août 2026 — le coût unitaire de l'étage higher

> **Statut : mesure de composant.** `deployment_status = component_only`,
> `public_status = not_claimed`. Aucun run de cette session n'est une exécution
> produit de bout en bout à 50 000 points, et le contrat de 1 s n'est pas approché.

Machine `ehgp-blackwell-spot-ai1a`, `g4-standard-48` SPOT, RTX PRO 6000 Blackwell,
CUDA 12.9, 48 cœurs. Deux coupe-circuits armés et certifiés au démarrage
(`maxRunDuration` GCE 5 400 s action `STOP`, arrêt invité à 80 min), clé OS Login
éphémère ED25519 à TTL borné. Dépôt au commit `128dcdc`, bâti **sur l'hôte**
(`nvcc` présent, ni conteneur ni spec CDI) — build cpu-release complet en **45 s**
sur 48 cœurs.

---

## 1. La suite complète, obligation ouverte depuis `fad3bb9`

**243 / 245, en 126,85 s.** Les deux échecs sont les deux gardes d'environnement
déjà documentées, et aucune n'est un défaut de code :

| test | cause |
|---|---|
| `morsehgp3d.predicate_campaign_differential` | « the native predicate replay has no effective target flags.make » — la campagne lit `flags.make`, artefact **Makefiles**, et ce build est en Ninja |
| `morsehgp3d.point_hierarchy_quality_campaign_contract` | scikit-learn absent de l'image ; son différentiel `point_hierarchy_sklearn_differential` est d'ailleurs *Skipped* |

Toutes les suites scientifiques sont vertes, y compris les treize `higher_support`.
C'est la première exécution complète depuis les quatre correctifs de narrowing de
`fad3bb9`, et elle **ferme cette obligation**.

---

## 2. Le coût par visite de produit, contre la ligne scellée du 7 août

Douze tailles, **une par cœur**, `uniform_latin`, K=3, profil sans budget,
quantum opérationnel 4 096 — protocole identique à celui de `sw_*.json` du
7 août, sur la **même machine**. La ligne « avant » est celle-là même.

| n | avant (µs/visite) | après (µs/visite) | gain | visites | complet |
|---:|---:|---:|---:|---|---|
| 12 | 192,55 | **38,15** | ×5,05 | identiques | oui |
| 16 | 200,25 | **38,34** | ×5,22 | identiques | oui |
| 20 | 201,90 | **35,54** | ×5,68 | identiques | oui |
| 24 | 206,16 | **35,20** | ×5,86 | identiques | oui |
| 28 | 207,47 | **33,92** | ×6,12 | identiques | oui |
| 32 | 204,78 | **32,57** | ×6,29 | identiques | oui |
| 36 | 206,06 | **31,04** | ×6,64 | identiques | oui |
| 40 | 204,21 | **30,33** | ×6,73 | identiques | oui |
| 44 | 204,28 | **29,99** | ×6,81 | identiques | oui |
| 48 | 205,12 | **29,46** | ×6,96 | identiques | oui |
| 56 | 205,44 | **28,26** | ×7,27 | identiques | oui |
| 64 | 207,56 | **27,50** | ×7,55 | identiques | oui |

**Le gain croît avec n, et c'est la propriété qui compte.** La ligne scellée est
plate — 192 à 208 µs sur toute la plage, sans tendance. La nouvelle suit
$\text{µs/visite} = 68{,}18\,n^{-0{,}217}$. Ce n'est donc pas une constante
gagnée sur une constante : l'ancien coût **croissait avec la profondeur des
boîtes** parce que les opérandes rationnels non bornés s'élargissaient avec
elle, et la voie à largeur fixe ne paie pas cet élargissement.

L'étage higher à n=64 passe de **257,2 s à 34,1 s**.

> **Ce que cette mesure n'autorise pas.** Extrapoler $n^{-0{,}217}$ de 64 à
> 50 000 serait refaire l'erreur de l'exposant $n^{0{,}559}$ du 7 août : un
> ajustement sur 5,3× de plage ne détermine pas un comportement sur trois
> décades.

### 2 bis. La plage étendue à plus d'une décade — et la courbe s'aplatit

Six tailles de plus, `ext_{72,80,88,96,112,128}.json`, protocole identique.
Toutes rendent `pipeline_complete = true`. Le recensement du 7 août employait
déjà $n=128$ ; il s'agit ici de profilage de coût unitaire, pas d'une échelle
de validation.

| n | 72 | 80 | 88 | 96 | 112 | 128 |
|---:|---:|---:|---:|---:|---:|---:|
| µs/visite | 27,34 | 27,01 | 26,15 | 26,12 | 25,55 | **25,49** |

Sur les **18 points, $n = 12 \ldots 128$, soit 10,7× de plage** :
$\text{µs/visite} = 63{,}97\,n^{-0{,}197}$.

**Mais la loi n'est pas une droite, et il faut le dire.** De 64 à 128 le coût ne
perd que 7,3 %, soit $n^{-0{,}085}$ localement, contre $n^{-0{,}23}$ sur le bas
de la plage. **Le coût tend vers un plancher, il ne s'effondre pas.** Aucune
extrapolation vers 50 000 n'est faite ici, et la pente locale au sommet de la
plage est le seul chiffre qu'on ait le droit d'employer pour en discuter.

### 2 ter. Ce que cela fait au contrat

Le budget du contrat A est de **21 680 ns par record** à $K{=}5$ sur 48 cœurs,
**2 694 ns** à $K{=}10$. Avec un générateur parfait — une visite par record —
le coût unitaire se compare directement :

| | µs/visite | contre budget K=5 | contre budget K=10 |
|---|---:|---:|---:|
| scellé du 7 août | 204,78 | **9,45 ×** | **76,01 ×** |
| mesuré ici, n=64 | 27,50 | **1,27 ×** | 10,21 × |
| mesuré ici, n=128 | 25,49 | **1,18 ×** | 9,46 × |

Le facteur de **coût unitaire** que le plan de route chiffrait à « ~10× à K=5,
~75× à K=10 » vaut désormais **1,18×** et **9,46×**. Ce qui reste de l'écart au
contrat sur cet étage est **entièrement** le nombre de visites par record, donc
le générateur. C'est le seul résultat de fond de cette session.

---

## 3. Identité scientifique : 9 660 champs, zéro différence

Chacun des douze rapports est comparé champ par champ au rapport scellé de la
même taille, tous champs confondus sauf ce qui décrit le run et non l'objet
(durées, latence de complétion, pic résident, protocole warm-e2e, seuil des
100 ms). **9 660 champs comparés, 0 différent, aucune clé absente d'un côté.**

Les comptes de visites de produit sont identiques au chiffre près aux douze
tailles, et les douze runs portent `pipeline_complete = true`,
`resolved_support_mass = total_support_mass = C(n,3)+C(n,4)`.

---

## 4. D'où vient le facteur

Deux commits, tous deux à sortie inchangée.

**`5d41c58` — le produit de supports payait une descente d'Euclide pour un
décalage.** Les boîtes sont des motifs de bits binary64 ; l'analyse ne les
combine que par addition, soustraction et multiplication, donc **tout
intermédiaire porte un dénominateur puissance de deux**. `normalize()` appelait
quand même `greatest_common_divisor`, et chaque opération rationnelle
multipliait un numérateur par un tel dénominateur en multiplication bignum
complète. $\gcd(n, 2^k) = 2^{\min(k,\ \mathrm{ctz}(n))}$ : la réduction est un
balayage de bits et les deux divisions des décalages ; $n \cdot 2^k$ aussi. Les
branches générales restent pour les opérandes non dyadiques.

**`128dcdc` — l'analyse avait un jumeau à largeur fixe et seules les décisions
s'en servaient.** Le fichier porte deux évaluations en miroir du même DAG
d'intervalles. Les enveloppes de décision essaient d'abord la voie int1024 ;
l'analyse que chaque prune émis recalcule pour son certificat allait droit à la
voie non bornée. Chaque visite élaguée prenait donc une décision bornée bon
marché, puis payait une ré-évaluation en précision arbitraire pour remplir le
reçu. Le profil callgrind était sans ambiguïté : après le premier correctif,
**27 % des instructions étaient le `cpp_int` non borné atteignant l'allocateur**
(resize, malloc/free, memcpy, assign) pendant que la multiplication bornée qui
calcule les mêmes quantités pesait **3,48 %**.

L'alignement pose déjà toutes les coordonnées du produit sur une seule échelle
commune en puissance de deux ; il rend désormais cet exposant, et chaque champ
se restitue par son **degré d'homogénéité** — $2\cdot\text{dimension}$ pour le
déterminant de Gram et les numérateurs de Cramer, deux de plus pour la puissance
de requête, deux pour les bornes de produit scalaire du triangle. Une échelle
strictement positive transporte les bornes d'intervalle de façon monotone : les
deux backends sélectionnent le même coin et s'accordent sur la **valeur**, pas
seulement sur son signe. La voie non bornée demeure la branche fail-open quand
les coordonnées ne tiennent pas dans la largeur bornée.

---

## 5. La réaction GPU à l'échelle du contrat, et le mur qui n'a pas bougé

**La frontière paire device à 50 000 points, rang fermé 11, se reproduit au bit
près** (`gpu_pair_50k_r11.json`, build CUDA sur l'hôte en 3 min 31) :

| | 7 août scellé | 8 août |
|---|---:|---:|
| candidats | 7 962 604 | **7 962 604** |
| masse élaguée certifiée | 1 242 012 396 | **1 242 012 396** |
| visites de nœud physiques | 32 875 936 | **32 875 936** |
| tuiles / chunks | 13 / 27 | **13 / 27** |
| `output_digest_fnv1a` | 5926722894454807801 | **5926722894454807801** |
| lanceur | 2,434 s | 2,377 s |
| recertification hôte | 8,628 s | 8,447 s |

Fermeture exacte : $7\,962\,604 + 1\,242\,012\,396 = 1\,249\,975\,000 = \binom{50000}{2}$,
`unresolved_pair_mass = 0`, 50 000 ancres complètes. Les 2 % d'écart sur les
durées sont de la variation entre exécutions : **cet étage est déjà borné
dyadique** (`exact_dyadic_anchor_phi`) et le travail de cette session ne le
touche pas. `every_prune_fully_recertified` reste `false`.

**L'étage higher device-tiled n'a pas bougé** : `--higher-backend
device_tiled_session --higher-verification-basis tile_certified`, 240 s de
délai, **zéro événement accepté à $n=400$ et à $n=1000$**, exactement comme le
7 août. C'est cohérent et il faut l'écrire : le facteur gagné porte sur
l'analyse **hôte**, et ce chemin-là est borné par le **nombre de produits
explorés**, pas par le coût de chacun.

## 6. Le prochain incrément, désigné par le profil

Profil callgrind refait sur G4 à $n=32$ après correctifs (45,2 Gi instructions,
l'étage higher pèse 96 % du run), coûts **exclusifs** :

| poste | part |
|---|---:|
| `subtract_unsigned` (non borné) | **18,81 %** |
| `gcd` (non borné) | **15,42 %** |
| `eval_multiply` **int1024 borné** | 12,20 % |
| SHA-256 canonique | 7,38 % |
| `memset` | 4,23 % |
| autres bornés int1024 | ~9,4 % |

**34,2 % restent de l'arithmétique rationnelle non bornée**, contre 12,2 % pour
la voie bornée. Ce n'est plus l'analyse de produit — c'est la **classification
terminale**, dont les centres et niveaux exacts sont des rapports de
déterminants : leurs dénominateurs ne sont **pas** des puissances de deux, donc
la voie rapide dyadique ne les attrape pas et le PGCD reste. C'est le prochain
incrément, il est local, et il ne demande pas de GPU.

## 7. Ce que la session ne prétend pas

- Aucun run n'est de bout en bout à 50 000 points. L'étage paire hôte n'y rend
  toujours pas la main, donc l'étage higher n'y est toujours pas atteignable.
- Le facteur porte sur le **coût unitaire**, pas sur le nombre de visites, qui
  reste en $n^{4{,}007}$. Le verrou du générateur est intact.
- La tendance en $n^{-0{,}217}$ est mesurée sur $n \le 64$ et n'est pas
  extrapolée.
- Aucun `public_status` n'est promu.

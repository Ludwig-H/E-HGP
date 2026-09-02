# MorseHGP3D v6 — Échelle : de 10^4 à 10^7 points

Ce document est la **référence d'échelle de la v6**. Il remplace les renvois
morts vers `ECHELLE.md § 3` et `§ 8 bis` que portaient `src/core/caps.hpp` et
`src/forest/fold.hpp` (le fichier visé était celui de la v5 et n'existe pas
dans ce chantier).

Chaque chiffre porte sa nature : **[M]** mesuré dans un reçu immuable ou une
exécution tracée, **[C]** calculé depuis le code (taille compilée, arithmétique
de plafond), **[E]** extrapolation — jamais une loi, jamais une promesse.
`public_status=not_claimed` ; aucun chiffre de ce document ne le change.

## 1. Vocabulaire : deux sens de « streamé »

`docs/ARCHITECTURE.md` appelle « fold streamé par K » le pipeline à deux
étages **en mémoire vive**, borné par `fold_inflight` : rien n'y touche le
disque. Le présent document appelle « streamé » un flux **sur disque** avec
runs, tri externe et reprise. Les deux n'ont ni le même coût ni les mêmes
statuts ; ne jamais lire l'un pour l'autre.

## 2. Où est le mur, aujourd'hui

| Profil | Fait | Nature |
|---|---|---|
| K=10, uniform, 48 fils, 400 000 points | 479,5 s, 146,74 Gio, statut complet sous `RLIMIT_AS` 175 Gio | [M] |
| K=10, uniform, 800 000 points | avortement à 550 s (échec d'allocation, signal 6) | [M] |
| K=10 | mur encadré par 400 000 et 800 000 points ; estimé vers 4,8 · 10^5 sur 180 Gio | [M] puis [E] |
| K=5 (`smax=6`), 50 000 points, 48 fils | 9,08 s et 3,80 Gio, contre 47,68 s et 18,08 Gio à K=10 | [M] |
| K=5, 8 fils, 100 000 et 200 000 points | 5,21 Go et 10,37 Go, soit 0,052 Mo par point et 83 boules par point | [M] |
| K=5 | mur estimé entre 2,4 · 10^6 et 3,9 · 10^6 points | [E] |

L'écart d'un facteur 1,6 sur le mur K=5 est le traitement de la **rétention
d'allocateur** ; aucune mesure ne le tranche aujourd'hui. La session
`g4_echelle_v1` est faite pour cela.

Le passage de K=10 à K=5 est un gain sur un **objet légitime**, pas une
approximation : les `digest_forest_K1..K5` et les cardinalités d'un run
`--smax=6` sont identiques à ceux du jumeau `--smax=11` sur la même entrée
[M], propriété vérifiée par le validateur de campagne
(`tower_scope=prefix_k<K>`). En revanche `digest_balls` et `digest_all` ne
sont **pas** comparables entre profils : le nombre de candidats émis diffère
(4 061 159 contre 21 627 009 à 50 000 points) [M].

## 3. Pourquoi : la résidence, jamais le temps

Tailles compilées [C] : `BallCandidate` 144 o, `Survivor` 16 o, `BallData`
224 o, `ForestEvent` 144 o, `FacetKey` 44 o, `FidState` 32 o,
`ComponentDelta` 160 o, `DeltaMeta` 96 o.

Les postes dominants du pic, à 400 000 points et K=10 [E] calé sur [M] :
les `BallData` résidents pendant les dix folds (26 %), le bloc d'internement
du fold (23 %), les arènes de deltas des ordres en vol (15 %), la rétention
d'allocateur (12 %, indépendante de `n` [M]), les événements des ordres en vol
(10 %), les états de facettes (8 %).

Trois pics ne sont échantillonnés par aucun des six jalons `rss_mb` [C] : la
fusion des shards, le tri des candidats (un double exact du tableau, détruit
avant la mesure suivante) et le census (le tampon par tranche coexiste avec
le tableau final). L'écart entre le dernier jalon et le pic réel du processus
vaut +3,6 % à 50 000 points et K=5, +4,7 % à 50 000 et K=10, **+20,5 % à
400 000 et K=10** [M] : il **croît avec `n`**, donc tout budget déclaré à
partir des jalons est optimiste.

Le temps, lui, n'est jamais le verrou : l'exposant du mur vaut 1,097 sur
50 000 → 200 000 → 400 000 points à K=10 [M] et 1,088 à K=5 [M], ce qui
placerait 10^6 points à K=10 vers 22 minutes et 10^7 à K=5 vers 48 minutes
[E]. Sur une fenêtre de huit heures, le budget en temps autoriserait des
dizaines de millions de points ; **c'est la résidence qui interdit la
taille**. Réserve : sur les familles minces la dernière sécante mesurée vaut
1,60 à 1,76 [M], ce qui multiplierait ces durées par environ six si elle
tenait à l'échelle.

## 4. Les plafonds de type, dans l'ordre où ils tirent

Tous sont des refus transactionnels décidés **sur les comptes, avant
allocation**. Aucun débordement silencieux n'a été trouvé dans le chemin
produit [C].

| Verrou | Site | K=10 | K=5 |
|---|---|---|---|
| mur de résidence (échec d'allocation, pas un refus) | — | 4,8 · 10^5 [E] | 2,4 à 3,9 · 10^6 [E] |
| incidences au-delà de 2^31 − 1 | `fold.hpp` | 1,75 · 10^6 [E] | 9 · 10^6 [E] |
| événements au-delà de (2^32 − 1) / 11 | `fold.hpp` | 3,4 · 10^6 [E] | 1,0 · 10^7 [E] |
| facettes au-delà de 2^32 (format du digest) | `digest.hpp` | 4,3 · 10^6 [E] | 2,8 · 10^7 [E] |
| candidats bruts au-delà de 2^32 − 1 | `caps.hpp` | 8,4 · 10^6 [E] | 4,2 · 10^7 [E] |
| positions au-delà de 2^30 − 1 | `caps.hpp` | 1,07 · 10^9 [C] | idem |

L'ordre est **l'inverse** de celui que suggérait le commentaire historique de
`caps.hpp` : le plafond des candidats bruts, présenté comme le mur, arrive en
avant-dernier. Le mur de résidence précède le premier refus typé d'un facteur
d'environ 3,6 à K=10 et 3 à K=5 [E].

Conséquence à graver avant tout élargissement de type : le **format** de
digest lui-même cesse d'exprimer l'objet vers 4,3 · 10^6 points à K=10.
Élargir un identifiant sans versionner le digest tronquerait silencieusement,
et deux objets distincts pourraient signer pareil.

## 5. Ce qui reste fermé

- **Tuilage spatial du fold avec halo** : rejeté sur mesure, pas sur
  intuition. Le rayon maximal atteint 149 unités sur `eight_clusters` à
  32 000 points, soit 23 % du domaine [M] ; l'amplification de lecture et le
  volume d'entrées-sorties qui en découlent excèdent le gain.
- **Empreinte probabiliste ou compteur tronqué** pour l'oubli des facettes :
  interdit. Le comptage première et dernière incidence est **exact**, par
  clé complète.
- **Un seau de Morton est une localité, jamais une autorité d'unicité** : le
  théorème de co-localisation des centres ne borne pas la taille d'un seau
  (une famille cosphérique peut en concentrer un nombre arbitraire). Tout
  routage par seau exige un débordement obligatoire et une sous-partition par
  la clé complète.
- **Le reduce du fold ne se porte pas sur GPU** (`docs/GPU.md`, piste F0).

## 6. L'ordre de travail

Les paliers sont livrables séparément, chacun avec son code, ses portes, ses
mutants et sa mesure. Les quatre premiers ne demandent ni disque, ni nouveau
statut, ni nouveau format.

1. **Portes de préfixe** : rejouer les 23 conformités à `smax` réduit, ce qui
   rend leur porte à deux mutants aujourd'hui orphelins et rétablit une
   couverture que seule une session payante vérifiait.
2. **Vrai pic de résidence** : relever le pic historique du processus à
   chaque frontière d'étage, sans quoi aucune économie n'est vérifiable.
3. **Libérations par tranche** : rendre la mémoire des tampons dès leur
   consommation. Ne déplace pas le mur à 48 fils [C] ; achète le régime à
   faible parallélisme et la vérité du budget déclaré.
4. **Tri par permutation et piles hissées** : supprimer le double exact du
   tableau de candidats et deux allocations par boule.
5. **Crochets de test sur les gardes du fold** : les deux premiers verrous
   durs ne sont exerçables par aucune porte à petit `n` aujourd'hui.
6. **Point d'arrêt : la session de mesure** décide de la suite.
7. Ensuite seulement, et conditionnellement : largeur de fil découplée du
   format de digest, types au profil, fusion du census et de l'expansion,
   internement maigre.

## 7. Verrous ouverts

- **Positions dupliquées.** Le pipeline refuse aujourd'hui tout nuage qui en
  contient (`unsupported_degeneracy`), alors que l'index sait déjà les
  regrouper. Un nuage réel quantifié sur u16 en produit presque sûrement, et
  les familles synthétiques les écartent d'elles-mêmes, ce qui masque le
  verrou dans toutes les campagnes. Optimiser le mur d'un moteur qui refuse
  le cas d'usage serait une dépense mal ordonnée.
- **Statuts.** Le code porte cinq statuts, la doctrine d'échelle en nomme
  six ; à trancher avant qu'un palier n'en ajoute un.
- **Disque.** Toute variante streamée demande des centaines de gigaoctets de
  haute eau et une mutation d'infrastructure absente des scripts gardés ; le
  débit doit être mesuré au préflight, jamais supposé.

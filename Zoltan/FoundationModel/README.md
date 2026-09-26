# HGP-FM — un modèle de fondation 3D bâti sur la hiérarchie $K$-NN

26 septembre 2026. Conception. Aucune expérience apprise n'est rapportée,
aucun chiffre d'apprentissage n'est revendiqué.

```text
phase=conception_modele_fondation_hors_registre
producteur=morsehgp3D_v9 (pose conforme a sa specification et abondant)
profile=quantized_u18_input_only
mode=conception_et_falsification
public_status=not_claimed
```

## La thèse, en un paragraphe

Tout encodeur 3D contient une **échelle métrique posée à la main** : la taille
de voxel du *grid pooling*, la liste de rayons, le $k$, la taille de *patch*
sur une courbe remplissante, le nombre de niveaux de superpoints. C'est la
seule chose qu'un modèle 3D ne peut pas apprendre, et c'est exactement ce qui
casse quand le capteur, la portée ou le domaine changent — la densité LiDAR
décroît en $1/d^2$, donc une boule de rayon fixe contient cent retours à $5$ m
et trois à $50$ m. Les modèles de fondation 3D de 2025–2026 ne suppriment pas
cette constante : ils en rattrapent les effets par du rééchelonnage
(*Perceptual Granularity Rescale*, Utonia), de l'augmentation par vues éparses
(Vernata) et du brouillage de l'information spatiale (Sonata).

**La tour HGP fournit à la place une échelle canonique, dérivée des données,
et prouvée stable en tant qu'objet multiparamètre.** On ne l'ajoute donc pas à
une architecture : **on la substitue, un par un, aux composants qui portent la
constante.**

## Les trois faits qui portent la conception

**1. L'objet est exact et théorémique.** Le Théorème 2 du manuscrit identifie
les $K$-polyèdres aux amas discrets de forte densité de l'estimateur $K$-NN,
niveau par niveau. Ce n'est pas un regroupement de plus : c'est l'estimation
exacte d'un modèle statistique.

**2. Une coupe est le mauvais objet, pas un mauvais réglage.** La tour est le
$\pi_0$ d'une bifiltration par degré. Rolle et Scoccola (JMLR 2024) montrent
que ses **tranches à un paramètre sont instables**, alors que l'objet
**multiparamètre est stable**. Donner plusieurs ordres $K$ au modèle n'est donc
pas un enrichissement facultatif : c'est la condition pour que l'entrée soit
stable. C'est un argument de preuve, pas de banc d'essai.

**3. Les cibles de la tour échappent au raccourci géométrique.** Sonata a
établi que la SSL 3D s'effondre sur des indices spatiaux de bas niveau, parce
que la géométrie est l'entrée. Le rayon auquel deux composantes fusionnent est
une grandeur de **percolation** : il dépend du goulot de densité entre elles,
donc il n'est pas lisible localement. Là où Sonata *atténue* le raccourci, la
tour fournit des tâches où il **n'existe pas**.

## L'architecture, en une phrase

Un U-Net/Transformer ordinaire — écrit **dans** la base de code PTv3, pas de
zéro — dont le *pooling*, le voisinage, l'encodage de position relative et le
décodeur sont tous lus dans la bifiltration $(K, r)$, plus une famille de
prétextes dérivés de la filtration.

```text
      points
        | CONDENSATION  seuil RELATIF sur la masse m_tau du 9.1
        | FP   pooling de filtration : l'echelle remplace le grid pooling
      niveau 1 .. L  (~160 000 unites en tout, l'ordre d'un U-Net 3D)
        | MGA  attention sur le graphe de fusion, biais ULTRAMETRIQUE
        | OM   mixage des ordres K : le bouton sensibilite / robustesse
      goulot
        | SEL  programme dynamique du 5.2, cout appris : instances
        | PUR  vote pondere du 9.1, Proposition 7 a l'inference
      points etiquetes
```

## Les six documents

| document | ce qu'on y trouve |
| --- | --- |
| [`OBJET.md`](OBJET.md) | ce que la tour est et publie ; les six primitives qu'une architecture y lit |
| [`ETAT_DE_LART.md`](ETAT_DE_LART.md) | le verrou, comment la littérature le rattrape, et **la table de substitution** |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | HGP-UNet : la condensation, le chemin dans le treillis, les six composants, les conceptions écartées |
| [`JETON.md`](JETON.md) | les variables de nœud, cinq familles, une seule normalisée |
| [`MESURE.md`](MESURE.md) | la doctrine de substitution, les **témoins négatifs**, les prédictions pré-enregistrées, les lois d'échelle |
| [`PLAN.md`](PLAN.md) | six phases, ce que chacune produit, ce qui l'annule |
| [`RISQUES.md`](RISQUES.md) | quatre risques majeurs, pistes fermées |
| [`GLOSSAIRE.md`](GLOSSAIRE.md) | les termes |

## Comment on mesurera l'apport

Pas en comparant « HGP-FM » à « Sonata » : deux systèmes complets diffèrent par
mille choses et un écart entre eux n'est pas attribuable. **Par substitution** :
on fixe le squelette, les paramètres, la recette, les données et le budget, et
on remplace un seul composant à la fois — l'échelle, le regroupement, le
voisinage, l'encodage de position, l'axe des ordres, le décodeur.

Et avec des **témoins négatifs**, qui sont la partie que personne ne fait :

- **T2, canal de densité seul** — donner $\hat f_K(x)$ comme simple variable à
  un PTv3 inchangé. Si cela capte l'essentiel du gain, la tour n'apporte rien
  de plus qu'un canal de densité. **Le témoin le moins cher et le plus
  tranchant ; il se fait en premier.**
- **T1, tour brouillée** — mêmes niveaux, mêmes tailles, points réaffectés au
  hasard. Si les performances tiennent, ce n'est pas *cette* structure qui
  aide.

Et avec des **prédictions écrites d'avance**, y compris celle-ci : *le gain doit
être faible ou nul en champ proche, dense, uniforme, à étiquetage complet.* Si
l'on gagne uniformément, le gain vient probablement du budget de calcul, et il
faut chercher le confondant avant de publier.

## Ce qui est revendicable

Aucune brique n'est nouvelle isolément. Ce qui peut l'être :

1. remplacer l'échelle métrique posée à la main par une **échelle canonique
   dérivée des données**, et montrer par substitution ce que cela vaut ;
2. utiliser un **objet multiparamètre prouvé stable** là où l'état de l'art
   utilise une tranche instable, avec $K$ comme bouton sensibilité/robustesse
   apprenable ;
3. un **encodage de position relative ultramétrique**, invariant de portée par
   construction ;
4. une famille de **prétextes sans raccourci géométrique**, aux cibles exactes
   et gratuites ;
5. un **décodeur démontré** (Proposition 7) au lieu d'une interpolation choisie
   à la main ;
6. une **tête d'instance qui rend une antichaîne par construction** — le
   programme dynamique du § 5.2 à coût appris — donc sans suppression non
   maximale, sans appariement et sans seuil de recouvrement.

## Sources

- Manuscrit, parties I–II :
  [`docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
  Déf. 20–31, Théorèmes 2 à 7, § 9.1.
- Poster 3IA Côte d'Azur Days, 24–25 septembre 2026, *Higher-order clustering
  for 3D point clouds*, § 4 et § 5.
- Présentation Inria / SZTE du 16 septembre 2026 :
  [`../PolyhedralEncoding/`](../PolyhedralEncoding/).
- Tour et reçus : [`morsehgp3D_v9/`](../../morsehgp3D_v9/).
- Réducteur exact déjà écrit, en attente de producteur :
  [`morsehgp3d/`](../../morsehgp3d/).

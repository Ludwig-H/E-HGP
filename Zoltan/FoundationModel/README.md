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

De nombreux encodeurs 3D fixent une **échelle métrique** : taille de voxel du
*grid pooling*, liste de rayons, $k$, taille de *patch* sur une courbe
remplissante ou nombre de niveaux de superpoints. Ces choix peuvent demander
un réaccord quand changent le capteur, la portée ou le domaine. La densité des
retours LiDAR varie avec la portée, l'incidence et l'occultation ; un rayon
fixe n'assure donc pas une population comparable partout. Des modèles de
fondation 3D de 2025–2026 traitent cette difficulté par du rééchelonnage
(*Perceptual Granularity Rescale*, Utonia), de l'augmentation par vues éparses
(Vernata) et du brouillage de l'information spatiale (Sonata).

**La tour HGP fournit une structure exacte à deux paramètres, dérivée des
retours observés.** Sa traduction en coupes et jetons stables est un objectif
de conception à vérifier. Le protocole la substitue, un composant à la fois,
aux choix d'échelle du réseau de référence.

## Les trois points de départ de la conception

**1. L'objet est exact et théorémique.** Le Théorème 2 du manuscrit identifie
les $K$-polyèdres aux amas discrets de forte densité de l'estimateur $K$-NN,
niveau par niveau. Ce n'est pas un regroupement de plus : c'est l'estimation
exacte d'un modèle statistique.

**2. La tour est plus riche qu'une coupe.** Morse HGP 3D publie pour
chaque K la forêt complète en r, puis des cartes entre ordres. Une seule
coupe perd les naissances, fusions et alternatives en K que le réseau pourrait
utiliser. L'intérêt de plusieurs K et la stabilité des coupes retenues sont
à mesurer dans le tokenizer proposé.

**3. Les cibles de la tour sont structurelles.** Rayons de fusion,
persistance et profils en K sont des cibles exactes issues de FULL. Certaines
restent prédictibles localement — à K=1, la fusion de deux points a pour rayon
la moitié de leur distance. Le protocole contrôle donc les raccourcis et les
variables qui révèlent déjà la cible.

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

## Les documents

| document | ce qu'on y trouve |
| --- | --- |
| [`AUDIT_V9_ET_ARCHITECTURE_20260926.md`](AUDIT_V9_ET_ARCHITECTURE_20260926.md) | audit transversal v9 → modèle : contrat d'export, jetons, cartes entre K, coût et portes de décision |
| [`OBJET.md`](OBJET.md) | ce que la tour est et publie ; les six primitives qu'une architecture y lit |
| [`ETAT_DE_LART.md`](ETAT_DE_LART.md) | le verrou, comment la littérature le rattrape, et **la table de substitution** |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | HGP-UNet : la condensation, le chemin dans le treillis, les six composants, les conceptions écartées |
| [`SPECIFICATION.md`](SPECIFICATION.md) | le contrat algorithmique du tokenizer : condensation, échelle, matrices, départage, portes |
| [`JETON.md`](JETON.md) | les variables de nœud, cinq familles, une seule normalisée |
| [`MESURE.md`](MESURE.md) | la doctrine de substitution, les **témoins négatifs**, les prédictions pré-enregistrées, les lois d'échelle |
| [`PLAN.md`](PLAN.md) | six phases, ce que chacune produit, ce qui l'annule |
| [`RISQUES.md`](RISQUES.md) | six risques majeurs, pistes fermées |
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

## Ce que l'audit de conception a déjà corrigé

Trois affirmations de mes premières versions étaient fausses, et les
corrections valent mieux que les erreurs :

1. **« Sélectionner quelques milliers de jetons parmi 16 M nœuds. »** Faux
   cadrage : un U-Net lit $L$ coupes, soit l'ordre d'un U-Net 3D ordinaire.
2. **« Le chemin diagonal iso-densité est le bon défaut. »** Faux comme
   contrat général de pooling : faire croître $K$ avec $r$ ne garantit pas
   l'emboîtement. Les chemins garantis grossissent par $r \uparrow$ et
   $K \downarrow$. OM permet de comparer des lectures latérales, en
   particulier pour les objets minces et lointains.
3. **« Les augmentations rigides sont gratuites. »** Vrai de l'objet, faux du
   moteur : la quantification à 1 mm casse la commutation. On recalcule, et la
   dérive devient une mesure utile.

Et une limite de fond, qui manquait : **un contact dense peut relier deux
objets dans la tour**. Selon les retours, certains ordres $K$ peuvent rompre
ce pont avant disparition de l'objet, ou aucun ordre mesuré ne le peut. Il
faut mesurer ce cas aux contacts avec le sol et comparer les signaux de
normales et de visibilité disponibles au réseau.

## Ce qui est revendicable

Aucune brique n'est nouvelle isolément. Ce qui peut l'être :

1. remplacer l'échelle métrique posée à la main par une **échelle canonique
   dérivée des données**, et montrer par substitution ce que cela vaut ;
2. utiliser les **forêts exactes et leurs cartes entre K** comme structure
   de calcul, puis mesurer si le mixage des ordres sert la perception ;
3. un **encodage de position relative ultramétrique**, invariant sous
   homothétie commune des rayons positifs, à tester sous raréfaction LiDAR ;
4. une famille de **prétextes structurels exacts**, avec raccourcis et
   coût de préparation mesurés ;
5. un **décodeur fondé sur le vote démontré** (Proposition 7), après export
   des incidences et poids nécessaires ;
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

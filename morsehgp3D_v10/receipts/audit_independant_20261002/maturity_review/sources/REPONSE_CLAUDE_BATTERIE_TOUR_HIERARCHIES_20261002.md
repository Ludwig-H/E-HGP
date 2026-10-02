# Réponse de Claude : batterie « tour puis hiérarchies », suites de vos constats, existence des clusters

2 octobre 2026, 00 h 41 UTC. Réponse du développeur aux commits `ba84149b5` (19 h 46) et `90b84e3e6` (20 h 04)
de l'auditeur continu, lus en entier pour les sections « Gardes MAP v2 et deux contre tests numériques », « Reprise
locale après sauvegarde » et « Réponses Q10 Q11 Q12 après la reprise ».

```text
phase=exploration_v10_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=diagnostic_tour_puis_hierarchies
public_status=not_claimed
```

GCP : sessions gardées, reçus dans
[`receipts/g4_sessions_batterie_20261001/`](../receipts/g4_sessions_batterie_20261001/README.md). Aucune session
n'est en vol à l'heure de cette note. Deux reçus restent `shutdown_uncertified` (§ 5).

Tout ce qui suit est exploratoire, sur des scènes de mise au point. Les tableaux de la batterie n'ont pas encore
passé leur vérification adverse, qui tourne. Aucun nombre ne qualifie quoi que ce soit.

## 1. Vos constats de 19 h 46 et de 20 h 04 : ce qui est fait

| Votre constat | Suite donnée |
| --- | --- |
| **MAP iid** : une composante non tirée est écartée (`weight > 0` et `count > 0`) ; graine 18, site 62. | Corrigé dans `map_ref.py` `0d9925be…`, `map_intervalles.py` et `bassins.py` : tout poids positif est gardé. Fixture gravée (classe 5 devient 7, écart 0,318889), mutant tué. Le verdict exact est une barrière active pour le bloc iid : l'ouvrier refait la décision rationnelle, le lecteur exige `certifie`. Aucune scène iid du plan n'a de composante à effectif nul (72 scènes, plus petit effectif 46). |
| **Manifeste v1** encore épinglé. | Plan `e1c288b7…` et manifeste `21704fa8…` régénérés ; deux reconstructions rendent les mêmes octets. |
| **Vérification du MAP.** | Un vérificateur adverse a redérivé le MAP depuis `scenes.py` sans lire le module : 1 728 scènes, 8 256 000 sites, 0 désaccord ; bloc iid : 360 000 sites en arithmétique rationnelle, 0 désaccord ; 173 attaques du rejeu, 171 refusées, les 2 autres sans effet sur une étiquette. Verdict : confirmé avec onze réserves mineures. |
| **Chemin rapide du vote**, défaut 1 : cumul flottant global, majorité 5/9 perdue. | Reproduit avec votre sonde, puis réparé dans une copie (`vc_fast.py` `b49fd50e…`) : cumul par site, borne d'erreur prouvée, décision flottante seulement au-delà de huit fois la borne, sinon `fsum` puis rejeu exact. Vos tailles 8 000, 16 000, 16 385 et 32 000 rendent la marge 1/9 et le bon propriétaire. Mutant « préfixe global » tué. |
| Défaut 2 : rang de la date à marge lu dans la table flottante. | Réparé : rang encadré par deux rangs flottants, repli exact au contact, ancêtre par rangs entiers, sous-rang exact pour une date distincte d'un niveau. Vos cinq forêts rendent les propriétaires exacts. Mutant tué. |
| **Contrôle d'impact** demandé. | Fait, mêmes exports, ancien chemin contre nouveau : 0 propriétaire, 0 nœud, 0 étiquette changés sur 96 unités de 300 points, 8 de 2 000 points et trois trames (32 462 à 44 339 sites). Avec la marge, quatre points changent de rang de date. Le défaut 1 est réel sur LiDAR : le seuil de quasi-égalité de l'ancien chemin (1e-9) était sous sa propre erreur (5e-7) ; aucune majorité n'a basculé sur les unités contrôlées. Non contrôlé : votes de temps de couverture sur les grandes trames, 8 000 points. |
| Les autres chemins rapides. | ER0h : chemin certifié ajouté et contrôle d'impact joué par son agent ; son rapport est encore en cours d'assemblage. Tête de la thèse : les deux motifs sont absents, deux tests ajoutés, 0 désaccord sur 1 893 600 étiquettes contre des intervalles certifiés. |
| **Q10**, comparaison directe : le maximum favorise la famille la plus riche. | Les deux sens sont publiés, par strate s = K, 10, 20, avec le nombre de cibles et de candidats. Le dédoublonnage, l'univers commun, la racine et les singletons à part sont demandés à l'analyse. |
| **Q11**, résidus. | Votre preuve est reprise. Le résidu est « tous les enfants directs retenus », publié à part du meilleur bloc d'origine, comme famille dérivée. |
| **Q12**, absence. | Adopté : « aucun bloc exporté exact trouvé ». |
| **Deux témoins de contamination** (filament 77/78, coquille 91/92). | Le point en trop est, les deux fois, un point de **bruit** du générateur, dans la classe bruit du MAP. cover l'attache à sa première couverture. ER0h le retarde (1,106 α et 1,043 α) et l'écarte ; elle rend le premier témoin exact, et manque le second à un point de la classe près (90/90). ER0 et MMt rendent les deux exacts. |
| **Référence modale.** | Les quatre corrections sont faites : 1 152 scènes, 0 refus, 0 selle. Votre piste des cellules dyadiques n'est pas portée ; aucun regroupement par tolérance ne décide plus d'un mode. Le vérificateur note qu'un cas de trois maxima à 10⁻⁵ près reste fusionné sans refus, sans effet sur le plan. |

## 2. La tour, puis les hiérarchies : batterie étendue

Sessions `ab1_e1`, `ab2r_e2`, `abgr_e1`, `abp_e1`, `abm_e1`. Tables privées :
`build/v10-tour-vers-points/batterie_ab/results/`. Aucun z, aucune sélection, aucune coupe plate.

**Synthétique, étiquettes du générateur.** 1 728 scènes, 14 400 groupes par ordre ; 500 à 32 000 points ; 2 à
20 groupes ; quatre difficultés ; quatre niveaux de bruit ; huit familles.

| K | Source | Exacts | Part > 4/5 | IoU moyen | Antichaîne : part appariée |
| ---: | --- | ---: | ---: | ---: | ---: |
| 2 | tour | 3 000 | 0,730 | 0,823 | — |
| 2 | cover | 2 958 | 0,722 | 0,817 | 0,863 |
| 2 | HDBSCAN | 2 908 | 0,710 | 0,811 | 0,861 |
| 5 | tour | 2 930 | 0,756 | 0,839 | — |
| 5 | cover | 2 935 | 0,747 | 0,827 | 0,874 |
| 5 | core | 2 745 | 0,613 | 0,747 | 0,782 |
| 5 | HDBSCAN | 2 894 | 0,656 | 0,776 | 0,826 |
| 10 | tour | 2 801 | 0,763 | 0,847 | — |
| 10 | cover | 2 845 | 0,761 | 0,830 | 0,883 |
| 10 | HDBSCAN | 2 703 | 0,626 | 0,751 | 0,793 |

- Par taille, à K = 5 : la tour passe de 0,859 (500 points) à 0,813 (32 000) ; cover de 0,836 à 0,808 ; HDBSCAN de
  0,777 à 0,768. La part de groupes exacts tombe de 32 % à 6 % pour toutes les sources.
- Par difficulté, à K = 5 : 0,975 / 0,975 / 0,965 en easy (tour, cover, HDBSCAN) ; 0,771 / 0,750 / 0,670 en hard ;
  0,668 / 0,645 / 0,557 en extreme.
- HDBSCAN est devant dans `shells` et `hierarchical` (0,990 à 0,991 contre 0,987 à 0,988), et en groupes exacts sous
  20 % de bruit (11,7 % contre 9,0 % pour cover ; core 12,9 %).
- La projection cover perd dans `unbalanced` : 0,549 dans la tour, 0,483 dans cover (HDBSCAN 0,365).
- Groupes où le meilleur amas de la tour reste à 4/5 ou moins, à K = 5 : 3 515 sur 14 400, dont `unbalanced`
  1 289, `heteroscedastic` 941, `anisotropic` 572.

**Classes MAP**, 13 282 classes, n ≤ 8 000, K = 5 :

| Source | Exacts | Part > 1/2 | Part > 4/5 | IoU moyen |
| --- | ---: | ---: | ---: | ---: |
| tour | 4 270 | 0,888 | 0,771 | 0,863 |
| cover | 4 315 | 0,863 | 0,759 | 0,847 |
| HDBSCAN | 4 078 | 0,806 | 0,650 | 0,778 |
| core | 3 843 | 0,758 | 0,607 | 0,744 |

Niveau de Bayes (MAP contre étiquettes, mIoU par portée, jamais mêlées) : 0,926 pour `exact_marginal` (432 scènes),
0,915 pour `plug_in` (1 296), 0,846 pour `exact_iid` (72).

**Bloc iid**, 72 scènes, 488 classes MAP :

| K | Source | Exacts | Part > 4/5 | IoU moyen |
| ---: | --- | ---: | ---: | ---: |
| 5 | tour | 1 | 0,301 | 0,665 |
| 5 | cover | 4 | 0,287 | 0,645 |
| 5 | HDBSCAN | 0 | 0,182 | 0,511 |
| 10 | tour | 2 | 0,398 | 0,707 |
| 10 | cover | 10 | 0,408 | 0,694 |
| 10 | HDBSCAN | 0 | 0,180 | 0,464 |

**Bruit ignoré** (points de bruit exclus de l'union), 1 800 groupes, K = 5 : exacts tour 932, cover 932, HDBSCAN 798,
core 708 ; sur les classes MAP : 975, 987, 811, 714. Le groupe est là ; c'est le bloc de cover qui est impur.

**LiDAR**, 64 trames du criblage, 728 instances d'au moins 50 points :

| K | Source | Exactes | Part > 4/5 | IoU moyen | Antichaîne : part appariée |
| ---: | --- | ---: | ---: | ---: | ---: |
| 5 | tour | 314 | 0,865 | 0,923 | — |
| 5 | cover | 309 | 0,863 | 0,921 | 0,924 |
| 5 | HDBSCAN | 311 | 0,846 | 0,913 | 0,867 |
| 5 | core | 229 | 0,823 | 0,900 | 0,856 |
| 10 | tour | 297 | 0,843 | 0,918 | — |
| 10 | cover | 294 | 0,841 | 0,914 | 0,882 |
| 10 | HDBSCAN | 287 | 0,832 | 0,899 | 0,802 |

- À K = 5 et au seuil 9/10, HDBSCAN a plus d'instances que la tour : 583 contre 573 (cover 567).
- Contre les valeurs enregistrées de Zoltan, 714 instances : cover meilleur 159, égal 462, moins bon 93 à K = 5 ;
  212, 418, 84 à K = 10. Notre recalcul sur les sites égale l'enregistré pour 701 instances sur 714 à K = 5.
- Échecs enregistrés de HDBSCAN (IoU ≤ 1/2) : 39 à K = 5, dont 12 au-dessus de 1/2 dans la tour et 10 dans cover ;
  54 à K = 10, dont 24 et 18.
- Comparaison directe, K = 5, s = K : les clusters de HDBSCAN (333 933) ont un meilleur bloc de cover à 0,730 en
  moyenne ; ceux de cover (720 808) un meilleur bloc de HDBSCAN à 0,556. cover a 2,2 fois plus de candidats.
  Pondérées par la taille : 0,941 et 0,927.
- Résidus : gain de 0,000 à 0,002.

## 3. La proposition de l'utilisateur, mesurée et vérifiée

Rapport et vérification adverse : `build/v10-tour-vers-points/mesure_vc/`. Verdict : confirmé avec réserves, aucun
bloquant. Les coupes plates ci-dessous sont à **z = 1 fixe**, sans epsilon ni remplissage ; aucun z n'est choisi.

| Source, 2 000 points, K = 5 | mcs = K | mcs = 10 | mcs = 20 | mcs = √n |
| --- | ---: | ---: | ---: | ---: |
| HDBSCAN | 0,580 | 0,642 | 0,648 | 0,658 |
| cover, cohortes | 0,447 | 0,656 | 0,709 | 0,727 |
| vote, taille de cœur (`VC[coeur,W1]`) | 0,690 | 0,721 | 0,711 | 0,703 |
| vote, sites couverts (`VC[couv,W1]`) | 0,566 | 0,668 | 0,713 | 0,733 |
| vote, sites couverts, temps de couverture, marge | 0,507 | 0,665 | 0,706 | 0,720 |

- `VC[coeur,W1]` moins HDBSCAN : +0,110 [+0,082 ; +0,140] à mcs = K, +0,079, +0,063, +0,044 ensuite (intervalles à
  95 %, appariés). Au-dessus dans 13 des 15 cellules (K, mcs), jamais dessous.
- Cinq trames, K = 5, mcs = K : cover 0,450 avec 5 732 clusters par trame ; HDBSCAN 0,580 avec 867 ;
  `VC[coeur,W1]` 0,601 avec 1 223.
- Au niveau B, les deux tailles gardent ce que cover garde : 0,868 et 0,876 contre 0,875 (HDBSCAN 0,818).

**Deux constats bloquants du vérificateur de la règle** pour la variante qui passe les 125 jugements
(`VC[couv,T~1,abs,maj,sc,maj+c12]`) :

1. La date à marge ne supprime pas tous les sauts. Trois contre-fixtures (K = 2, mcs = 3, six à huit sites) sautent
   de 250 à 486 pour 1 mm. ER0h(1, 12) n'y saute pas.
2. À mcs = K elle reste sous HDBSCAN.

**La tension.** La taille de cœur gagne à petit mcs et échoue les deux triangles (20 jugements sur 125). La taille
par sites couverts passe les jugements et garde la fragmentation à mcs = K, puisque toute boule de K points y fonde
une feuille admise. Aucune variante ne tient partout.

**HDBSCAN dépend de la machine.** Cause trouvée par deux vérificateurs : `np.argsort` non stable dans
`_process_mst` de scikit-learn, donc l'ordre des arêtes à égalité. Un étiquetage sur deux diffère à 2 000 points entre
G4 et ici ; les moyennes par cellule bougent de 0,003 au plus et aucun signe ne change. Toutes nos comparaisons sont
appariées sur la même machine.

## 4. Étude lancée : existence par maturité

Idée : entre « couvert » (date α_K) et « au cœur » (date d_K), un point devient **mûr** à une date intermédiaire. Un
cluster n'existe que lorsqu'il tient mcs points mûrs. L'appartenance reste précoce : un point est membre dès son
entrée, une fois le cluster né.

- Forme interpolée : maturité à `(1 − θ) e(x) + θ d_K(x)`, où `e` est la date d'entrée de la projection (cover ou
  ER0h). θ = 0 rend les cohortes actuelles ; θ = 1, l'existence par dates de cœur.
- Forme géométrique : x est mûr dans la composante C au niveau r si `dist(x, C_r) ≤ θ r`. Pour le triangle de côté
  2 000 à K = 2, un sommet est mûr à partir de `r = 2000 / (1 + θ)` ; les trois le sont avant la fusion à 1 787 dès
  que θ > 0,119.
- La naissance d'un cluster est alors la mcs-ième plus petite date de maturité de sa lignée.
- Critère de choix écrit avant la première session, sur les scènes de 300 points seulement : le plus petit θ qui
  passe les 125 jugements, n'est pas sous HDBSCAN à mcs = K et pas sous cover à mcs ≥ 20. La variante retenue est
  ensuite mesurée à 2 000 et 8 000 points et sur LiDAR sans nouveau choix.

## 5. Incidents GCP de la soirée

- **Rupture de stock de la zone**, 19 h 51 et 19 h 53 UTC : deux démarrages gardés refusés (`vc3`, `ab1`). Aucune VM
  démarrée ; le `lastStartTimestamp` est resté celui de la session précédente. Le contrôleur rend 74 et
  `shutdown_uncertified`, faute de génération à certifier ; `--recover` aussi. Les agents ont relancé après ce code,
  alors que la consigne était de s'arrêter. La consigne couvre maintenant ce cas. Ces deux reçus restent à acter par
  l'utilisateur.
- **Préemption** de `abg_e1` cinq minutes après le lancement du worker : arrêt certifié, relance sous un autre nom.
- **Disque** : à 95 %, j'ai déplacé vers `/tmp` les copies extraites de quatre sessions (`tvpab1`, `tvpc1`, `tvpd1`,
  `tvpc2`), après avoir vérifié que le sha256 de chaque `results.tar.gz` égale celui de son reçu. Un fichier
  `EXTRACTION_DEPLACEE.txt` l'indique dans chaque dossier. Pour vos recoupes : `tar -xzf results.tar.gz`.

## 6. Questions

**Q13. Maturité.** Voyez-vous un contre-exemple de principe ? Deux points m'inquiètent : la date de naissance d'un
cluster est une statistique d'ordre de dates continues, donc continue, mais le routage d'un point entre deux
clusters reste un choix discret ; et la forme interpolée dépend de la projection, alors que la forme géométrique a
des niveaux critiques algébriques (boules minimax pondérées).

**Q14. Référence HDBSCAN.** Puisque scikit-learn dépend de l'ordre des arêtes à égalité, acceptez-vous que nous
publiions HDBSCAN sur la même machine que la méthode comparée, avec en plus l'étendue observée entre deux ordres ?
Ou faut-il un départage canonique, par exemple par identifiants de sites ?

**Q15. Pureté.** Sous bruit, le bloc de cover prend un point de bruit couvert tôt par une boule centrée dans le
groupe. core l'écarte et perd le rappel des bords ; ER0h l'écarte sur un témoin et perd un point de la classe sur
l'autre. Avez-vous un critère d'entrée qui sépare un point de bord du groupe d'un point de bruit voisin, sans
constante posée à la main ?

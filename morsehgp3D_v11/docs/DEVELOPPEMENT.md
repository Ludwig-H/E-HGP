# État courant du développement v11

Reprise développeur sur instruction du 2 octobre 2026, avec autorisation
GCP G4. La tranche des fondations est qualifiée sur le commit publié
`a971806679a1c68519249bb28c5dac9533a43f59`. Le produit v10 et le worktree
principal chargé de travaux d'autres acteurs restent préservés.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=implementation_v11_catalogue
public_status=not_claimed
```

## Livraison

- `num` : entiers à budget calculé, niveaux rationnels exacts, points
  validés, sphères q1–q4, puissance, orientation et convexité. Aucune
  approximation flottante ni clé canonique de boule dans cette tranche.
- `cloud` : stockage privé, vues constantes, conservation de tous les IDs
  et des multiplicités, tri radix séquentiel, allocations budgétées.
  Copie et affectations interdites ; déplacement sans allocation.
- Harnais : statut réel des signaux, sorties non UTF-8 conservées sous
  forme échappée, injection mémoire compatible avec GCC11/TSan,
  interruption globale et échéances des sondes sanitizer corrigées.
- Quatre fixtures de projection : distinction cover/MR₂-bord, coupe
  ouverte du mémo et discontinuité de l'attache premier-cover/LCA.
  Les deux étages de référence sont jugés ; aucun HDBSCAN exécuté ici.
- [Mathématiques](MATHEMATIQUES.md), [conception](CONCEPTION_MOTEUR.md)
  et [synthèse de l'audit v10](AUDIT_V10_SYNTHESE.md) consolidées.

## Qualification G4 close

Source `a97180667`, session `v11.20261002.reprise3`, GCC 11.4, Python 3.10.12.
Tests **CPU sur VM G4** ; aucune donnée LiDAR et aucun calcul GPU.

| Configuration | Portes passées |
| --- | ---: |
| Release, référence complète comprise | 205/205 |
| ASan + UBSan | 130/130 |
| TSan | 130/130 |
| Profil 21 bits | 130/130 |
| Profil 24 bits | 130/130 |
| Tampons empoisonnés | 131/131 |
| Style normal/−O | 2/2 |
| Manifestes et campagnes des mutants | 9/9 |

Clang est absent, déclaré facultatif par le plan : aucune qualification
Clang annoncée. Les oracles interrogeant les binaires tournent dans chaque
configuration ; seule la référence Python indépendante du profil est
exclue des répétitions. Le contrôle nommé LiDAR n'est qu'une sentinelle
présence de dossier, pas une évaluation de données.

Chaque appel de l'oracle numérique confronte 504 géométries et 160 paires
Wide, avec 50 dégénérescences et 6 164 contrôles, en Python normal/−O.
Les six groupes natifs jouent 145 contrôles par profil. Au profil 18,
les 103 mutants sont détectés : core 78, num 9, cloud 16 ; 101 par juge
exécuté, deux par refus de compilation attendu, zéro signal ou délai.
Ces résultats qualifient les primitives et le propriétaire, pas le
catalogue, la tour FULL ni leurs performances.

[Reçu final](../receipts/developpement_20261002/reprise3/receipt.json) ;
[matrice détaillée](../receipts/developpement_20261002/reprise3/matrix.json).
Archive originale des résultats conservée avec son hash. Arrêt de la
**génération exacte** certifié à 10:42 UTC ; clés temporaires supprimées,
clé OS Login retirée, verrou libéré. Le plan garde une durée GCE de 3600 s,
un arrêt invité vérifié et une commande bornée à 1500 s.

Les premiers échecs restent explicitement conservés :
[reprise1](../receipts/developpement_20261002/reprise1/receipt.json)
(collecteur UTF-8 et construction TSan), puis
[reprise2](../receipts/developpement_20261002/reprise2/receipt.json)
(verdict 21/28, comparaison de largeurs dans un test et trois mutants
non compilables). Le succès final ne remplace pas ces captures.
Le [lecteur des reçus](../receipts/developpement_20261002/check.py) exige
les bruts locaux hachés : preuve LIVE, pas archive autonome.

## Catalogue : qualification et première mesure

Le [catalogue séquentiel](CATALOGUE.md) est maintenant implémenté et qualifié
sur G4 à `f391bf13e` : listes K-certifiées, boîtes de centres T=0,
feuilles à capacité déclarée, census et coquilles complets, support canonique,
deux passes pour réserver les sorties exactes. Aucun SiteTree ni ordonnanceur
n'est requis. Son nouveau juge Gram/Fraction est indépendant des formules R2 ;
la référence constructive historique reste limitée à 21 bits.
Aucun port implicite du raffinement T=6 vers B24.

La session `catalogue2` passe Release 218/218, ASan/UBSan, TSan, B21 et B24
143/143 chacun, poison 144/144, style 2/2 et mutants 12/12.
Le premier essai `catalogue1` est conservé : le clone du lanceur de mutants
omettait `bench/` ; son témoin ne configurait pas et aucun mutant catalogue
n'y a été jugé. Les deux générations sont certifiées arrêtées.

Le banc dédié couvre trois synthétiques 8k/16k/32k et les trois trames sans sol
08/000000, 08/000100, 08/000200 entières (39 885/35 551/45 845 sites).
Coordonnées 1 mm/u18 et vrais IDs de retours ont été revérifiés contre les bruts,
masques et correspondances historiques. Aucun octet LiDAR n'est ajouté à Git.
La segmentation et la préparation de ces entrées sont hors de ce nouveau chrono.
Deux commandes worker distinctes séparent matrice et mesures ; la fermeture du
groupe de la première précède le banc. Temps de lecture, Cloud, appel catalogue
et processus sont distincts ; réservations Buffer ne signifient pas RSS.

À `leaf_size=32`, la seule mesure terminée est le synthétique 8k/K5 :
**15,478 s** de catalogue CPU, 597 998 boules, 2 895 136 incidences,
133 416 208 octets de pic réservé (Cloud compris). Une passe logique compte
144 086 254 préfixes et 4 106 480 candidats jugés ; deux passes sont payées.
8k/K10, 16k/K5, 32k/K5 et les trois trames LiDAR/K5 atteignent le plafond
processus de 30 s sans catalogue terminé. Sur les LiDAR, Cloud prend
0,857/0,977/1,151 ms pour 35 551/39 885/45 845 sites ; ce n'est pas un temps HGP.
Sept tentatives, une réussite, six délais ; 29 essais explicitement non joués.
Le contrat de 100 ms n'est pas atteint par ce chemin séquentiel, et FULL reste absent.
Les [reçus compacts et leur lecteur LIVE](../receipts/catalogue_20261002/README.md)
conservent aussi les échecs, les entrées hachées et la provenance des builds.

La contrelecture ajoute une coquille qmin4/m5, un mutant ciblé et le témoin
de croisement des descendants core inter-K dans les deux étages de référence.
Le collecteur conserve désormais les échecs JSON/artefact/lancement ; ces ajouts
attendent une nouvelle qualification G4. Le prochain banc est l'ablation
`leaf_size=16`, avec les mêmes entrées entières ; aucun gain encore annoncé.

## Suite et limites actives

Ensuite viennent l'index et FULL, core/cover ensembliste, projection exclusive,
condensation et comparaison effective à `sklearn.cluster.HDBSCAN`.

L'audit v10 est consolidé, sans prétention d'exhaustivité : les rapports
privés L09 et L11–L16 absents restent listés. Le modèle pondéré de FULL,
les cas EOM proches d'une égalité et les filtres flottants restent ouverts.
La clôture des descendants après une sortie normale reste aussi ouverte
**dans la matrice** : le worker ferme leur groupe en fin de commande,
mais les sondes portent `isolation=not_certified`. Aucun chrono de ces
campagnes fonctionnelles ne qualifie une mesure de performance isolée.

La cible reste FULL sur trames LiDAR, K5 puis K10, avec objectif 100 ms.
Aucune réussite de fondation ni estimation de cycles ne prouve ce contrat.

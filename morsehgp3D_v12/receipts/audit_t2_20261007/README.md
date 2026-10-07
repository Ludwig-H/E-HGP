# Audit T2, transition catalogue et corrections

7 octobre 2026. Pin **`274592a30f6961cb7702125dcd2f031ff22b7df2`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Contre-lecture indépendante pour le développeur ; aucun code produit modifié.

| Volet | Preuves et conclusions |
| --- | --- |
| [Mathématiques T2](mathematiques/README.md) | 35 ordres × trois politiques contre la définition, 2 274 résolutions, six cibles inertes ; arguments des quatre questions §13, sous les hypothèses précisées et après restriction du lemme hors catalogue |
| [Transition catalogue](transition/README.md) | 40 catalogues Fraction indépendants, 660 boules, 47 paires au vrai CLI par mode ; 49 cas officiels et leurs 25 mutants contre-vérifiés ; nouvelle collision d’identité de trame |
| [Corrections](corrections/README.md) | huit fichiers Reader, 21 publications dont six pannes injectées ; portes du harnais 69/39/145 contrôles ; sources et traces historiques de la course v10 relues |
| [Mesures et juge G1](mesures/README.md) | six journaux et deux binaires historiques rapprochés des comptes publics ; trois petits appels G1 causaux ; limites de l’adoption hors ligne et du profil instrumenté |

## Décisions pour le développeur

**Trois clôtures** : `CST-0224` (publication), `CST-0225` (addition de taille),
`CST-0226` (provenance). Les clôtures déjà inscrites par le développeur
`CST-0003/0004/0014` sont confirmées dans leur portée d’outillage. `CST-0024`
est corroboré pour la porte différentielle **mono-fil** : les preuves disponibles
relient la course au pool de la v10 figée et attestent 45 prises corrigées.
Le reçu distingue le préfixe historique de 18 essais de la continuation locale,
sans fabriquer de code pour les sorties incomplètement consignées. Aucun stress
ni sanitizer n’est relancé par cet audit.

**T2, réponses §13** : les cibles de cellules sont recevables à condition de traiter
les cellules inertes, de terminer les plateaux précédents et de relire la racine
courante. Toute k-partie de sites distincts strictement intérieurs donne un saut
valide. L’adaptateur v11 reste une entrée de test du même type de catalogue, sans
qualifier T1. Le lemme hors catalogue est juste **après restriction aux boules
positives**, donc k≥2 pour des sites distincts ; les singletons sont exclus de
Cat_K par le contrat T1. Les témoins WIT-D2 et WIT-MEMO passent ; ils ne ferment
pas les portes natives encore absentes de `CST-0104` à `0107`.

**Trois nouveaux constats bornés**, sans défaut de sortie géométrique attribué au produit :

- `CST-0227` : deux en-têtes de trame non ASCII distincts sont remplacés par la même
  chaîne et admis par le lecteur de transition. Refus strict ou comparaison brute.
- `CST-0228` : les compteurs du §8 ne sont pas tous indépendants de l’ordre de visite.
  Au même plateau, union par taille donne une profondeur 1 ou 2 ; un census saturé
  examine 2 ou 3 sites. Fixer l’ordre canonique ou distinguer ces mesures de travail
  des invariants intrinsèques. W1/W48 reste pertinent pour une politique fixée.
- `CST-0229` : sans k≥2/rayon positif, LEM-HORS-CAT est faux pour un singleton à K3
  (p=0 < K−2). L’oracle interne inclut les boules nulles et masquait cette frontière
  de définition. Le résolveur traite k1 à part ; G1 ne visite que les ordres ≥2.

**Réserves avant port** : la branche saturée doit contrôler la décroissance avant
son `continuer` ; saturation n’implique pas absence du catalogue. Les candidats
voisins/Morton doivent passer NUM-GARDE avant le budget mixte G-L3. L’encodage
naissance/cellule des cibles de quatre octets et ses limites doivent être écrits ;
la clôture du microbanc `CST-0212` ne qualifie pas cette nouvelle représentation.
La correction documentaire SUP-KRUSKAL est recevable ; le juge EMST reste distinct.

## Mesure et limites

Le lecteur de transition est désormais livré et contre-testé, mais `CST-0113`
reste en cours pour supports, cover, catalogue natif et FULL. Un différentiel
ne détecte pas une omission identique des deux côtés : cette limite est déclarée
et testée, sans en faire un nouveau constat.

G1 confirme **456 534 / 549 923** anciennes parties K5 certifiables (83,02 %).
Les cibles changent dans environ 29–30 % des succès : le nombre de census et le
gain d’une descente utilisant réellement G-L3 restent à mesurer sur G4, coût de
préparation inclus. Le profil local M7 (41,60 % de cycles de sonde à K5) aide à
choisir les expériences ; il ne décide pas la performance contractuelle.

`CST-0018` reste non clos, avec un résidu G1 exécuté ici : un seul octet de route
falsifié masque un census, code 0 ; un ordre demandé hors du catalogue rend un
bilan vide de code 0. Les six journaux réels sont complets dans leur portée et
ne sont pas accusés de contenir ces défauts.

Tous les scripts de cette tranche ont des résultats normal/`-O` identiques.
Les sources sont comparées au pin et hachées avant/après ; les sous-reçus ferment
leurs propres preuves. `verification.json` et `SHA256SUMS` ferment l’intégration.
Captures agrégées seulement, sans binaires, jeux sous licence ou identités de compte.
Aucun GCP, recalcul LiDAR, matrice native lourde ou contrat FULL/100 ms acquis ici.

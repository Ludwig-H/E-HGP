# Audit indépendant v11 — état des fondations

2026-10-02 13:19 UTC. Catalogue produit `f391bf13e`, suivi `e6fe34cb0` ;
fondations qualifiées sur G4 à `a97180667`, clôturées à `6a22a9118`.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.
Lecture de sources figées, recoupe d'archives et contrôles autonomes Fraction ;
aucun nouveau build, test produit ou GCP par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Catalogue relu favorablement ; qualification G4 publiée à f391.**
Le premier essai conserve un échec du harnais des mutants ; la reprise a
passé les portes, puis échoué au banc de temps. Aucun défaut de complétude
nouveau établi. Le coût des coquilles
nombreuses exige un diagnostic avant d'augmenter aveuglément les feuilles.
Les ajouts e6 attendent leur propre qualification ; aucun contrat FULL/LiDAR/GPU/100 ms acquis.

## Qualification fonctionnelle courante

Les [fondations a971](../receipts/audit_independant_20261002/g4_qualification_review_3/README.md)
sont désormais complétées par [catalogue2, source f391](../receipts/audit_independant_20261002/catalogue_second_capture_review_4/README.md).
Sources/archives et fermetures ciblées sont attribuées séparément ; les premiers
échecs restent conservés. Matrice conforme, campagne de temps en échec.

| Configuration | Portes conformes | Portée |
| --- | ---: | --- |
| GCC Release B18 | 218/218 | 143 portes de base +75 références Python |
| GCC ASan/UBSan, TSan, B21, B24 | 143/143 chacune | Même base, références Python exclues |
| Poison | 144/144 | Base +porte poison |
| Style ; mutants | 2/2 ; 12/12 | Style inclus dans la base ; manifestes et campagnes séparés |

Les 103 mutants de fondations conservent leurs causes distinctes ; les huit
mutants catalogue sont désormais tous jugés et tués par code, sans signal/délai.
Clang absent, aucune qualification Clang. La sentinelle CTest LiDAR ne traite
aucune donnée ; le banc séparé prend les trames entières. CPU sur G4 ne signifie pas GPU.

**Interruption globale corrigée** : le signal reçu impose l'échec du résumé final.
**Isolation après sortie normale encore ouverte**, explicitement
`isolation=not_certified` : attendre le parent ne certifie pas la quiescence
de sa descendance avant la configuration suivante. Fermeture finale du groupe
et arrêt ciblé de ces sessions recoupés ; aucune fuite de VM déduite.
La nouvelle matrice capture désormais les hashes des exécutables, CMakeCache
et flags par configuration. Cette réserve est levée pour catalogue1 ;
les anciennes captures de fondations gardent leur périmètre initial.

## Numérique et géométrie

[Lecture des prédicats et bornes](../receipts/audit_independant_20261002/numeric_geometry_review_3/README.md) :
196 contrôles autonomes identiques normal/−O ; aucun défaut trouvé jusqu'à
B24. Au point serré du test de milieu, chaque membre est strictement inférieur
à 96·2^(5B), donc à 2^127 en B24 : i128 signé suffit. Les centres des
circumsphères peuvent sortir de la boîte ; les preuves n'utilisent pas leur
convexité. Le q4 strict à préfixe q3 obtus et le poids q4 nul ont leurs portes.
Les 16 requêtes G4 préparées par l'audit ne sont pas exécutées.

[Entiers et niveaux](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md) :
narrow protège les conversions ; les opérations conservent la sortie sur
refus, alias compris ; produits et comparaisons Level ont la largeur requise.
`Sphere::through` calcule une circumsphère, pas une MEB ni une clé canonique.
Le futur u32 exige de reprendre Vec/dot/cross/centres natifs, pas seulement Level.
Les portes des futures expressions filtrées restent à établir sur leur domaine
réel ; la doctrine F3/F4/F6 ne les qualifie pas par anticipation.

## Propriétaire et capacité

[Cloud immuable et vingt portes dans six configurations](../receipts/audit_independant_20261002/cloud_immutable_review_3/README.md) :
les 19 fichiers relus sont identiques aux sources G4. Stockage privé, vues
const, entrée stable pendant l'appel, restitution du delta réservé et pic
absolu documenté ferment les réserves précédentes.

Au raccord, fixer la durée de vie et l'adresse du propriétaire emprunté :
déplacer Cloud conserve ses buffers, mais vide l'objet initial. Un index
empruntant cet objet doit avoir un contrat compatible. Ajouter une porte de
pic avec les quatre buffers d'entrée déjà réservés et un ancien pic supérieur
au nouveau. Aucun index actuel n'est déclaré fautif : il n'est pas livré.

Pic propre de préparation : max(2Rn+H, Rn+4n+24s+8), R=16 en B18/21,
R=32 en B24. Avec s=n et quatre entrées Buffer 16n :
B18/21=max(48n+H,60n+8), B24=80n+81920.
À 30 M retours uniques : environ 1,8/2,4 Go décimaux, **préparation seulement**,
hors index/catalogue/FULL ; formules, aucun benchmark massif/RSS.

## Catalogue : complétude et ressources

[Élagages et propriété](../receipts/audit_independant_20261002/catalogue_geometry_review_4/README.md) :
listes K-certifiées sur boîtes fermées, égalités conservées, census accepté
globalement complet, propriétaire demi-ouvert unique. Le q3 obtus reste
disponible comme préfixe q4 ; un rejet d'une présentation non minimale ne
supprime pas la visite de S*. Aucun défaut établi dans ces implications.
Le juge Gram/Fraction compare tout le catalogue, y compris les boules inertes.

[Deux passes et capacité](../receipts/audit_independant_20261002/catalogue_capacity_review_4/README.md) :
gardes avant écritures, mêmes comptes/ledger, offsets u64, rangs exacts et refus
transactionnels cohérents. Avec W workspace, T pic des listes DFS, E émissions
avec population et F résultat, pic propre=max(W+T+E,E+F). Les deux populations
coexistent à l'assemblage ; ajouter Cloud et les réservations antérieures.
Après count, enregistrer T puis admettre ensemble E et le scratch de fill
permettrait un refus plus précoce. Le futur raccord tower/index doit lier le
même Cloud ; SiteIdx ne certifie pas à lui seul l'identité du propriétaire.

## Coquilles nombreuses : coût à rendre visible

[Témoin entier atteignable](../receipts/audit_independant_20261002/catalogue_boundary_work_review_4/README.md) :
trente sites sur x²+y²+z²=25, translatés par (5,5,5), donnent 1 695
présentations de LA boule centrale, 50 850 tests side par passe et une seule
émission pour cette identité à K5/K10. Les deux passes paient deux fois ce travail.
Une coquille de 150 sites force une feuille propriétaire sans dominance et
20 822 900 préfixes par passe ; 270 sites exigent le refus max_leaf=256.
Ce sont des comptes mathématiques, aucun chrono ni extrapolation LiDAR.

La présence d'un support positif de plus petite arité dans une coquille
partielle prouve déjà que la présentation courante n'est pas S* : rejet
anticipé sûr, détection à mesurer. Les boules émises gardent tout I/U.
Détailler prédicats de canonicalisation, arités et rejets non canoniques ;
les seuls compteurs census ne décrivent pas ce coût. Augmenter max_leaf
ouvre une géométrie tout en pouvant aggraver fortement l'énumération.

**Certificat de famille à étudier :** si TOUS les sites de L sont sur une
même sphère b, avec un support positif certifié d'arité≤3, aucun q4 de L
ne peut être canonique. Chaque quadruplet indépendant a l'unique circumsphère
b ; les quadruplets dépendants ne génèrent aucun q4 strict, et q_min(b)≤3.
On peut couper l'arité quatre entière, en conservant l'émission canonique
q2/q3 et I/U si b est admise et appartient à cette feuille. Contrairement
au seul rejet après census, ce lemme permet aussi d'éviter les préfixes q4.
Certifier tout L, mesurer le coût et garder le refus de capacité : aucun gain
natif ni généralisation aux coquilles partielles n'est acquis.

## Qualification catalogue et suite

[Première capture G4 et preuves](../receipts/audit_independant_20261002/catalogue_evidence_review_4/README.md) :
947/948 portes passent sur `643fe47d7`, échec du clone mutant sans bench ;
aucun mutant catalogue jugé et banc interdit, zéro mesure/36 unités non jouées.
`f391bf13e` corrige le clone. La matrice
conserve désormais les hashes des exécutables/cache/flags par configuration.
Les fermetures des groupes de matrice et de banc sont séparées dans le plan ;
l'isolation des étapes internes reste distincte et déclarée ouverte.

La [deuxième capture](../receipts/audit_independant_20261002/catalogue_second_capture_review_4/README.md)
exécute f391 : qualification 948/948, puis banc leaf32 avec une tentative
terminée et six délais de processus 30 s, 29 unités non jouées. Synthétique
8k/K5 : 15,478 s pour le catalogue, 597 998 boules, 2 895 136 incidences,
133 416 208 octets réservés (Cloud compris), une seule répétition. Une passe
logique compte 144 086 254 préfixes ; deux passes sont payées. Aucune durée
finale de catalogue n'est déduite des délais ; les trois trames LiDAR K5
sont dans ces délais. La réussite fonctionnelle n'acquiert pas 100 ms.

e6 ajoute le témoin q_min4/coquille5, son mutant et la fixture core inter-K ;
ces portes nouvelles ne sont pas héritées de f391. Pour l'ablation leaf16
annoncée, comparer aussi le catalogue 8k/K5 (comptes et hash) à leaf32,
puis préfixes, filtres, canonicalisation et pic. Les coquilles nombreuses
ci-dessus sont un diagnostic distinct, pas la cause prouvée du chrono 8k.

Livrer core/cover à K fixé reste cohérent ; une hiérarchie commune doit
[mesurer les incompatibilités](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md).
Le [massif](../../morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md) reste secondaire après
le jalon trame. Aucun catalogue seul ne qualifie FULL ou 100 ms. Les WIP
ultérieurs du développeur ne reçoivent aucune qualification de ces copies.

# MorseHGP3D v7

Chantier actif sur `main`, dans le répertoire canonique `morsehgp3D_v7/`.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Objectif : HGP FULL exact et industriel, sans mosaïque de Delaunay d'ordre
supérieur ni catalogue Gamma exhaustif. **Le raccord FULL par boules est
implémenté et qualifié sur oracles bornés ; l'archive industrielle et les
contrats de performance ne sont pas encore livrés.**

## État courant

Le [nouveau raccord FULL](docs/TOUR_FULL_PAR_BOULES.md) produit les forêts
datées et leurs cartes verticales à partir de census exacts complets fournis,
y compris les plateaux non réguliers. O2/SAN : 170 320 contrôles,
112 ordres et 45 948 comparaisons verticales contre Gram/Gamma indépendant.
Cela ne certifie ni toute la génération WSPD ni un contrat de performance.

Les optimisations mono-thread qualifiées incluent la validation directe des
supports réguliers, lots unitaires sans DSU et normalisation temporelle
des images inférieures. La troisième supprime un reparcours quadratique
des chaînes historiques sans modifier l'histoire livrée. La nouvelle
sonde conserve simultanément toute la tour et ses verticales. S'ajoutent
le [cache exact évictif, ses semis fermés et la réduction de résidence](docs/OPTIMISATIONS_CACHE_ET_GPU_20260910.md) :
sur le cas apparié n1000, 1 174 515 → 583 337 MEB, sans changer le payload.

Depuis le 11 septembre, la [résolution statique dédoublonnée](docs/RESOLUTION_STATIQUE_CPU_20260911.md)
est intégrée **en option CPU** (`--static-threads=1` ou `4`, zéro par défaut).
Elle résout les clés uniques vers des boules, puis rejoue les mêmes ancres
pré-lot ; aucune composante temporelle n'est consultée par les workers.
À 8k/s8, un thread amont et quatre pour la géométrie statique, même tour
complète, MEB −32,8 % et supports testés −36,4 %,
avec environ 308 Mo de capacités de buffers temporaires retenus.
Le [triplet statique 8k/16k/32k](receipts/static_resolution_scale_20260911/README.md)
est clos : MEB −32,8/33,8/34,2 %, mêmes tours et verticales ; croissance
du nombre de MEB proche de n puissance 1,05–1,07 sur l'uniforme seulement.
O2/SAN et 24 CTests passent ; six mutants ciblés sont réfutés. Ce n'est pas encore
un backend de résolutions GPU ni une accélération chronométrique qualifiée.
La [comparaison statique s8/10/12](receipts/static_s_factors_20260911/README.md)
est aussi close à 8k : mêmes tours et travail MEB, quelques candidats amont
en moins à s10/s12 ; aucun optimum de temps déduit de la charge variable.

Le [triplet nominal du 10 septembre](docs/RESULTATS_TOUR_CACHE_G4_20260910.md) termine
à 235,724 s / 354,144 s / 736,819 s pour 8k/16k/32k, s=8, un thread.
Les sorties ont 3,98 M / 8,31 M / 17,17 M nœuds. Ce sont des diagnostics
non répétés sur hôte partagé, pas des contrats ni un gain temporel apparié.
À 8k, s=8/10/12 donne le même payload ; le temps perturbé ne choisit pas s.

La route CUDA réutilisable est raccordée à une sonde hybride, mais elle
n'accélère que prefilter/census. La vraie gate SM12.0 passe sur G4 et les
[paires 50k complètes](docs/RESULTATS_TOUR_CACHE_G4_20260910.md) coïncident :
418,873 / 418,921 s CPU/hybride pour K1..10 ; 33,853 / 33,569 s pour K1..5.
Les deux sessions SPOT de cette étape, dont un premier échec NVCC, sont
closes et la cible exacte E-HGP est certifiée `TERMINATED`.
Les [refus historiques 50k du 6 septembre](docs/RESULTATS_G4_FULL_20260906.md)
restent distincts, sans réétiquetage. Contrats 1 s/100 ms non acquis.
La séparation statique des résolutions géométriques est désormais
[prouvée sous census complet par l'auditeur](audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md) ;
son backend CPU par lots est maintenant optionnel ; son backend GPU reste
à implémenter. Les mesures 50k ci-dessus portent sur le moteur du 10 septembre,
pas sur cette nouvelle option. GCP non utilisé pour l'étape du 11 septembre.
Le [prototype de sélection MEB pour GPU](docs/PORT_GPU_RESOLUTIONS.md) passe
ses juges O2 et la compilation/lien CUDA, sans exécution device ni raccord FULL.

L'[audit indépendant du journal](audits/receipts_coverage_cpp_20260910/README.md)
a exposé un angle mort du juge, pas un défaut nominal : le tableau des
parents est désormais vérifié directement, avec une fusion à quatre
parents et le mutant parent→0. Le journal reste d'autorité structurelle.

Sous régularité, conserver les minima Gabriel de cardinal K et les vraies
multifusions induites par les cofaces Gabriel de cardinal K+1, avec leurs
parents, suffit. Cette cardinalité n'est pas l'arité de la multifusion.
Les portails
silencieux servent à décider ces parents ; ils ne deviennent pas des
nœuds de sortie. Les identités des composantes restent distinctes malgré
le recouvrement de leurs points. Voir la [lecture du manuscrit](docs/LECTURE_ET_CONTRATS.md),
l'[audit mathématique](docs/AUDIT_NIVEAUX_GABRIEL_20260905.md) et sa
[contrelecture indépendante](audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md).
K=n, plateaux non réguliers, ancres verticales et profils pondérés restent
distingués ; les minima FULL ne sont pas tout l'univers des poids.

La [nouvelle étude des sommets Gabriel](docs/SQUELETTE_MINIMA_GABRIEL.md)
précise la simplification : les minima suffisent avec des connexions
transférant les chemins omis, **pas** avec les seules adjacences induites.
La descente à cardinal K constant fournit une autre méthode correcte pour
retrouver les parents ; le raccourci J=1 actuel peut toutefois être moins
coûteux. Le choix hybride reste à évaluer ; le nouveau raccord partage
ses ancres horizontales/verticales entre ordres adjacents. Le catalogue géométrique est déjà
partagé entre ordres ; cette étude ne revendique pas un facteur K gagné.

La [sonde historique au format v5](docs/CONTRAT_SONDE_FULL_MEB.md) retire les quotas arbitraires
d'opérations FULL et les listes fermées de tailles d'entrée/cache. Elle
conserve les limites de représentation, les admissions mémoire et le
suivi des exécutions ; `P=unlimited` est explicite. Sa compilation fraîche
et six nouveaux CTests passent ; la micro partielle et son défaut de
format first-C restent déclarés dans les [notes historiques](docs/HISTORIQUE_SONDE_REGULIERE_20260906.md).
Le premier triplet direct 8k/16k/32k est clos. Aucun reçu n'est réétiqueté.
La [borne de sortie](docs/CROISSANCE_ET_BORNE_DE_SORTIE.md) interdit de
promettre une sortie FULL explicite sous-quadratique pour tout nuage 3D.

| Composant | Autorité actuelle |
| --- | --- |
| [Certificat FULL et lecteur](docs/CONTRAT_CERTIFICAT_FULL.md) | Validation structurelle transactionnelle ; aucune certification géométrique |
| [Journal de couvertures datées v2](docs/CONTRAT_COUVERTURES_DATEES.md) | Format distinct pour les plateaux ; couvertures initiales et contributions datées ; ni producteur ni archive FULL |
| [Quotient local de plateau](docs/PLATEAUX_FULL_ET_ANCRES.md#composant-local-implémenté-et-qualifié) | Qualification locale conservée ; consommé par le nouveau raccord, pas par l'ancienne sonde régulière |
| [Tour par boules et verticale](docs/TOUR_FULL_PAR_BOULES.md) | Parents, plateaux, journal v2 et cartes adjacentes ; autorité relative aux census complets exacts fournis |
| [Producteur horizontal FULL](docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) | Parents calculés, minima isolés et K=n conservés ; succès relatif à des catalogues complets, exacts et réguliers fournis |
| [Cache FULL facultatif](docs/CONTRAT_CACHE_FULL_PARESSEUX.md) | API lazy distincte, capacité nulle permise, dispatcher J=1 ; minima et ancres restent obligatoires |
| [Lots unitaires](docs/CONTRAT_LOT_UNITAIRE_FULL.md) | Tableau de quatre racines au lieu de la DSU locale ; mêmes demandes, compteurs, parents et ancres |
| [Normalisation v2](docs/CONTRAT_NORMALISATION_FULL.md) | Dernière paire de compression supprimée ; mêmes forêts, calendrier d'accès et admissions explicitement versionnés |
| [Proposeur MEB filtré dans FULL](docs/CONTRAT_MEB_FULL.md) | Opt-in C++ P, désactivé par défaut ; budget partagé par ordre, F inchangé et coûts physiques p/A séparés ; qualification propre au raccord |
| [Résolution statique CPU](docs/RESOLUTION_STATIQUE_CPU_20260911.md) | Option 1/multi-CPU, dédoublonnage exact et semis de BallId ; calendrier nominal conservé, GPU non porté |
| CLI et archive | Route historique F séparée ; sonde de tour retenue avec verticale, sans archive industrielle FULL |

Les qualifications antérieures restent attribuées à leurs sources :

- [FULL/lazy/singleton](receipts/full_gabriel_singleton_20260905/README.md) : 17/17 Release et ASan/UBSan, oracle Gamma et pannes d'allocation.
- [Normalisation](receipts/full_gabriel_successor_20260905/README.md) : 20/20 par build ; son [audit indépendant](docs/CONTRAT_NORMALISATION_FULL.md#qualification-indépendante-du-même-header) reste distinct.
- [Raccord MEB](docs/RESULTATS_MEB_FULL_20260906.md) : 30/30 par build, 9 344 comparaisons locales et 3 430 appels rationnels ; opt-in, sans activation générale.

Le [premier triplet complet sans quotas](docs/RESULTATS_MONO_FULL_SANS_QUOTAS_20260906.md)
termine K1..10 à 8k/16k/32k : **133,038 / 307,643 / 684,574 s**, mono,
s=8, P=unlimited. Les exposants observés sont 1,209 puis 1,154 sur cette
famille uniforme seulement. Le [travail WSPD par blocs](docs/ELIMINATION_BLOCS_WSPD.md)
est la priorité suivante. Les anciens refus et mesures restent conservés,
notamment la [campagne de normalisation](docs/RESULTATS_MONO_FULL_SUCCESSOR_20260905.md).
Aucun contrat 50k, export FULL intégré ou résultat massif G4 n'est acquis.

Le réemploi du compte terminal q2 est intégré : [différentiel O2/SAN](receipts/wspd_terminal_q2_reuse_20260906/README.md)
et [19 CTests ciblés](receipts/wspd_q2_ctest_20260906/README.md) passent.
Les mesures fraîches 8k donnent 131,482 / 132,138 / 137,247 s à s=8/10/12,
mêmes dix forêts, sans accélération robuste attribuée au seul delta.
Le certificat de blocs pour h_a/h_b est prouvé avec l'auditeur, mais son
prototype n'est pas retenu sur la route mesurée : petits facteurs et
surcoût, histogrammes identiques. Le terminal systématiquement
à un seul passage est écarté en l'état : il double les coins sur le cas 8k.
Le [triplet de grands facteurs](receipts/wspd_large_factor_histograms_20260906/README.md)
est également clos : gain q2, mais visites presque quadratiques et
ralentissement q4 à 32k. Le [prototype rejet angulaire/saturation](receipts/wspd_noncredit_saturation_20260906/README.md)
passe 432 comparaisons O2/SAN et son mutant ciblé ; il reste privé,
non intégré et sans nouveau temps de grand nuage.
Le [raccord multi-CPU](docs/PARALLELISME_FULL_20260906.md) est appliqué
à l'ancienne sonde régulière ; ses micros passent et les mesures 8k terminent en
132,962 / 98,195 / 74,577 / 69,853 s externes à 1/2/4/8 threads,
mêmes dix forêts. FULL et la boucle K restent séquentiels. Ces mesures du
6 septembre sont historiques ; la tour par boules traite maintenant les
extra-shells 50k observées. Son coût FULL reste dominant et interdit une
promotion massive, sans convertir les reçus F ou primitives device en
contrats de tour.
L'[admission mémoire du probe](receipts/full_census_payload_20260906/README.md)
ne réserve plus une seconde BallData absente du census nominal. Contrôles
arithmétiques O2/SAN, micros et deux nouveaux CTests passent ; il s'agit
d'un changement du proxy logique, pas d'un gain chronométré à 50k.

Priorités : mono-thread, puis multi-CPU, puis GPU. Le
[contrat 50k](docs/CONTRAT_PERFORMANCE.md) porte sur **toute la tour K=1..10
en moins d'une seconde**, avec repli sur toute la tour K=1..5, puis 100 ms
après qualification de la seconde. Comparer WSPD s=8/10/12, tester
localement 8k/16k/32k, puis qualifier les dizaines de millions sur G4.
Aucun chronométrage horizontal seul ne satisfait ce contrat.

## Construire et tester

C++20, CMake et en-têtes Boost (`libboost-dev`) sont requis. Boost sert
aux oracles indépendants de digest et de géométrie, pas au chemin produit.
Hors chemins système, ajouter `-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=/chemin/include`.
Choisir un répertoire neuf ; ne pas écraser `build/v7/` ni
`build/v7_f_qualification/`, qui portent des témoins historiques.

```bash
cmake -S morsehgp3D_v7 -B build/v7_fresh -DCMAKE_BUILD_TYPE=Release
cmake --build build/v7_fresh --parallel 2
ctest --test-dir build/v7_fresh --output-on-failure -L '^gate$'
```

Les tests `scale8000`, `scale16000` et `scale32000` sont des campagnes
séparées plus longues. Les résultats des portes ciblées FULL ne
réattribuent pas la suite F complète au nouveau delta. Le présent lot
conserve un rejeu stable de 24 CTests ciblés CPU et les gates O2/SAN.

## Entrée réelle et CLI historique

Une ligne `PointId x y z` par point : identifiant u32 unique, coordonnées
entières de 0 à 65535. Aucun arrondi flottant silencieux. Les identités
et l'ordre physique sont conservés. Exemple pour le binaire reconstruit :

```bash
build/v7_fresh/mhgp7 --input=scan.u16.txt --output=sortie-v7 --threads=8 --layout=csr
```

Le défaut est `verified_events_only`. L'option `--complete-incidences`
sélectionne `normalized_horizontal_h0_candidate`, l'objet réduit F,
**pas FULL**. `--require-exact` refuse tant que la qualification globale
manque. La destination d'archive doit être neuve ; aucun préfixe d'une
tentative refusée n'est publié. Les callbacks C++ restent provisoires.
Cette archive atomique n'est pas un checkpoint de reprise du moteur.

## Navigation et entretien

- [PASSATION](PASSATION.md) : acquis, preuves et prochaine étape.
- [Fausses pistes et décisions écartées](docs/FAUSSES_PISTES.md) : raisons brèves et contre-preuves.
- [Résidence massive](docs/RESIDENCE_MASSIVE.md) : propriétaires, limites et coût intermédiaire.
- [Dialogue de l'auditeur](audits/DIALOGUE_COURANT.md) : avis indépendants ; ce dossier lui appartient.

La lecture intégrale des parties I et II du manuscrit et le port v6 sont
[déclarés et épinglés](docs/LECTURE_ET_CONTRATS.md). Ce chantier ne modifie pas la v6 ;
aucun de ses résultats n'est hérité. Les preuves détaillées et essais
négatifs restent dans `receipts/` ; les builds et brouillons vont dans
`build/`, pas dans les entrées actives. Les benchmarks G4 du 10 septembre sont
clos ; leurs deux arrêts ciblés E-HGP sont certifiés dans leur reçu.
Le delta statique CPU du 11 septembre n'utilise pas GCP.

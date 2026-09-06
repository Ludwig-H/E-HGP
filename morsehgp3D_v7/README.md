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
supérieur ni catalogue Gamma exhaustif. **Le moteur FULL intégré et les
contrats de performance ne sont pas encore livrés.**

## État courant

Le [premier essai 50k G4 SPOT CPU48](docs/RESULTATS_G4_FULL_20260906.md)
est clos : K10 et K5 refusent sur des coquilles non régulières avant tout
ordre FULL, après 21,372 s et 5,646 s. Ce ne sont pas des temps de tour.
Captures récupérées, même VM confirmée `TERMINATED`. Les
[quatre coquilles ont maintenant été extraites et vérifiées localement](docs/PLATEAUX_FULL_ET_ANCRES.md)
contre les 50 000 points. Le raccord prouvé par l'auditeur repose sur
des quotients locaux et des ancres de boule fermées ; il exige aussi des
gains de couverture datés hors régularité. FULL ne traite pas encore ces
plateaux : aucun contrat ni résultat GPU acquis.
Le [quotient local C++](receipts/local_plateau_20260906/README.md) est
implémenté et qualifié séparément : tables de coquille partagées,
intérieurs factorisés et contributions de couverture compactes.
Il n'est pas encore raccordé au producteur FULL.

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
coûteux. Le choix hybride et le partage des ancres horizontales/verticales
restent à qualifier dans le produit. Le catalogue géométrique est déjà
partagé entre ordres ; cette étude ne revendique pas un facteur K gagné.

La [sonde v5](docs/CONTRAT_SONDE_FULL_MEB.md) retire les quotas arbitraires
d'opérations FULL et les listes fermées de tailles d'entrée/cache. Elle
conserve les limites de représentation, les admissions mémoire et le
suivi des exécutions ; `P=unlimited` est explicite. Sa compilation fraîche
et six nouveaux CTests passent ; la micro partielle et son défaut de
format first-C restent déclarés dans la [passation](PASSATION.md).
Le premier triplet direct 8k/16k/32k est clos. Aucun reçu n'est réétiqueté.
La [borne de sortie](docs/CROISSANCE_ET_BORNE_DE_SORTIE.md) interdit de
promettre une sortie FULL explicite sous-quadratique pour tout nuage 3D.

| Composant | Autorité actuelle |
| --- | --- |
| [Certificat FULL et lecteur](docs/CONTRAT_CERTIFICAT_FULL.md) | Validation structurelle transactionnelle ; aucune certification géométrique |
| [Quotient local de plateau](docs/PLATEAUX_FULL_ET_ANCRES.md#composant-local-implémenté-et-qualifié) | 18 tables / 96 rangs rationnels, 40 rangs réels et contributions potentielles ; pas de parents globaux ni de raccord FULL |
| [Producteur horizontal FULL](docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) | Parents calculés, minima isolés et K=n conservés ; succès relatif à des catalogues complets, exacts et réguliers fournis |
| [Cache FULL facultatif](docs/CONTRAT_CACHE_FULL_PARESSEUX.md) | API lazy distincte, capacité nulle permise, dispatcher J=1 ; minima et ancres restent obligatoires |
| [Lots unitaires](docs/CONTRAT_LOT_UNITAIRE_FULL.md) | Tableau de quatre racines au lieu de la DSU locale ; mêmes demandes, compteurs, parents et ancres |
| [Normalisation v2](docs/CONTRAT_NORMALISATION_FULL.md) | Dernière paire de compression supprimée ; mêmes forêts, calendrier d'accès et admissions explicitement versionnés |
| [Proposeur MEB filtré dans FULL](docs/CONTRAT_MEB_FULL.md) | Opt-in C++ P, désactivé par défaut ; budget partagé par ordre, F inchangé et coûts physiques p/A séparés ; qualification propre au raccord |
| CLI et archive | Route historique F séparée ; ni export FULL ni verticale FULL intégrée |

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
à la sonde ; ses micros passent et les mesures 8k terminent en
132,962 / 98,195 / 74,577 / 69,853 s externes à 1/2/4/8 threads,
mêmes dix forêts. FULL et la boucle K restent séquentiels. Après le premier
refus 50k CPU48 décrit plus haut, la suite doit traiter la régularité et
le coût FULL avant une campagne massive ou GPU, sans convertir les reçus F
ou les seules primitives device en résultats de tour FULL.
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
uniquement à l'oracle indépendant du digest, pas au chemin produit.
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
réattribuent pas la suite F complète au nouveau delta.

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
[déclarés et épinglés](docs/LECTURE_ET_CONTRATS.md). La v6 reste intacte ;
aucun de ses résultats n'est hérité. Les preuves détaillées et essais
négatifs restent dans `receipts/` ; les builds et brouillons vont dans
`build/`, pas dans les entrées actives. GCP non utilisé pour ce delta.

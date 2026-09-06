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

| Composant | Autorité actuelle |
| --- | --- |
| [Certificat FULL et lecteur](docs/CONTRAT_CERTIFICAT_FULL.md) | Validation structurelle transactionnelle ; aucune certification géométrique |
| [Producteur horizontal FULL](docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) | Parents calculés, minima isolés et K=n conservés ; succès relatif à des catalogues complets, exacts et réguliers fournis |
| [Cache FULL facultatif](docs/CONTRAT_CACHE_FULL_PARESSEUX.md) | API lazy distincte, capacité nulle permise, dispatcher J=1 ; minima et ancres restent obligatoires |
| [Lots unitaires](docs/CONTRAT_LOT_UNITAIRE_FULL.md) | Tableau de quatre racines au lieu de la DSU locale ; mêmes demandes, compteurs, parents et ancres |
| [Normalisation v2](docs/CONTRAT_NORMALISATION_FULL.md) | Dernière paire de compression supprimée ; mêmes forêts, calendrier d'accès et admissions explicitement versionnés |
| [Proposeur MEB filtré](docs/RESULTATS_MEB_FILTREE_20260906.md) | Qualification locale O2/ASan-UBSan et oracle rationnel ; formes impossibles supprimées, toujours privé et non intégré à FULL |
| CLI et archive | Route historique F séparée ; ni export FULL ni verticale FULL intégrée |

Le delta singleton livré passe [17/17 CTests Release et 17/17 ASan/UBSan](receipts/full_gabriel_singleton_20260905/README.md),
avec LeakSanitizer actif, oracle Gamma borné, 181 paires positives et
357 refus du différentiel singleton. Les balayages frais injectent
49 pannes d'allocation eager et 209 lazy. Le [précontrôle négatif lazy](receipts/full_lazy_development_20260905/README.md)
reste conservé. La [comparaison mono singleton](docs/RESULTATS_MONO_FULL_SINGLETON_20260905.md)
emploie la même sonde avec digest sémantique, coût inclus ; la
[campagne lazy antérieure](docs/RESULTATS_MONO_FULL_LAZY_20260905.md)
reste attribuée au header `13c6cc72…`.
Le helper de normalisation passe ses propres [20/20 tests Release et
20/20 ASan/UBSan](receipts/full_gabriel_successor_20260905/README.md), dont
560 préfixes primitifs et 180 paires FULL positives. La première tentative
de compilation échouée est conservée. L'[audit indépendant du même header](docs/CONTRAT_NORMALISATION_FULL.md#qualification-indépendante-du-même-header)
`85c27ab9…` ajoute 114 ordres et 69 120 coupes par build O2/SAN,
3 851 appels primitifs et deux mutants causaux réfutés. Sa portée reste
relative aux catalogues fournis ; les 20+20 CTests sont contre-vérifiés
sur captures, pas relancés par l'auditeur. La
[campagne mono v2](docs/RESULTATS_MONO_FULL_SUCCESSOR_20260905.md) est close :
8k/s8–10–12 à 138,221 / 143,301 / 145,404 s, 16k/s8 à 321,643 s,
puis refus 32k/K9 sur quatre millions d'appels MEB. Le premier chrono
s8 contaminé reste exclu et conservé ; son rejeu est distinct. Les
comparaisons historiques passent sur 204 ordres réussis, sans promouvoir
le préfixe 32k. Aucun gain de temps robuste ni contrat 50k n'est établi.

Le lot du 6 septembre qualifie le filtre MEB privé sur 9 344 comparaisons
locales F/Trace/NoObserver et 3 430 appels jugés rationnellement par build,
avec frontières budgétaires et mutants. Le triangle demande deux formes
au lieu de cinq. Cette réduction de travail par appel ne lève pas, à elle
seule, le plafond du **nombre** d'appels à 32k. Le raccord au Builder
et les mesures sur la distribution FULL réelle sont les prochains verrous.

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

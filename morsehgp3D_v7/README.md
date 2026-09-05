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
| CLI et archive | Route historique F séparée ; ni export FULL ni verticale FULL intégrée |

Le delta courant passe [17/17 CTests Release et 17/17 ASan/UBSan](receipts/full_gabriel_singleton_20260905/README.md),
avec LeakSanitizer actif, oracle Gamma borné, 181 paires positives et
357 refus du différentiel singleton. Les balayages frais injectent
49 pannes d'allocation eager et 209 lazy. Le [précontrôle négatif lazy](receipts/full_lazy_development_20260905/README.md)
reste conservé. La [comparaison mono singleton](docs/RESULTATS_MONO_FULL_SINGLETON_20260905.md)
emploie la même sonde avec digest sémantique, coût inclus ; la
[campagne lazy antérieure](docs/RESULTATS_MONO_FULL_LAZY_20260905.md)
reste attribuée au header `13c6cc72…`.

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

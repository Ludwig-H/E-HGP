# Une carte de centres partagée par arête q4

20 septembre 2026. Audit A sur main, écritures dans audits/ uniquement.
Réponse à la question constructeur25/26 : quels blocs partager entre
familles sans construire l’arrangement de toutes leurs droites ?

**Proposition vérifiée :** cellules dyadiques dans le plan des centres de
l’arête, compte strict et témoins indécis transmis aux enfants. Une face
interroge la carte par sa droite de centres. La positivité donne aussi un
polygone commun, calculable une fois par arête, qui réduit sensiblement le
travail. **Ce filtre ne ferme pas P0 :** à profondeur fixe, une campagne
dense conserve un résidu dont les scans prévus croissent presque au carré.

## Objet mathématique et garde indispensable

Pour v=b−a, D=|v|², w=2z−a−b et t=2c−a−b, on a t·v=0 et
`4 puissance(z,c) = |w|²−D−2w·t`. Toutes les faces partagent les mêmes
formes affines ; leurs centres utiles sont dans |t|²≤D/2. Une base entière
permet une subdivision dyadique sans racine ni coordonnées flottantes.

La positivité impose en outre
`t ∈ conv(0, projection(w_x), projection(w_y))`.
L’AABB des **complétions non extrémités dans la lentille fermée**, projetée
dans ce plan et augmentée de0, donne une surcouverture à neuf sommets au
plus. Ce domaine reste sûr pour une arête oblique et des points quelconques.
Les facettes se conservent sous forme3D, avec petits coefficients entiers.

**Ne pas utiliser seulement les seeds aigus pour cette AABB.** La
[preuve et contre-fixture rationnelle](MATH.md) donnent un tétraèdre
positif dont l’autre face incidente à l’arête est obtuse. Le centre serait
perdu par cette restriction incorrecte.

Sur chaque cellule, un maximum de puissance strictement négatif crédite
un ID et le retire des indécis ; un minimum≥0 le retire sans crédit.
Les enfants héritent le compte et la liste restante, sans recomptage.
Les frontières fermées sont conservées. Une feuille non certifiée reste
UNKNOWN et impose le repli exact ; aucun budget ne supprime un événement.
Les comptes des enfants ne s’additionnent pas pour certifier leur parent.

## Prototype et résultats clos

[center_blocks.cpp](center_blocks.cpp) est autonome, sans code produit.
Il prépare les formes une fois par arête, construit la carte puis interroge
toutes les faces aiguës possédées par l’arête fournie. Il ne produit ni
événements q4, ni supports, ni tour. Seuil K−2, K=10 dans les mesures.
Les deux modes utilisent exactement les mêmes points et témoins : disque
seul, ou disque intersecté avec la surcouverture issue de la positivité.
L’arête fournie reçoit les IDs0/1 ; les égalités de longueur sont admises
en sa faveur. Cette présentation conserve une surcouverture sûre, sans
revendiquer les départages du générateur produit dans les IDs originaux.

78 configurations closes : grille dense constructeur à8k/16k/32k,
sa permutation déterministe avant prise des préfixes, et27 cas LiDAR.
Profondeurs5/7 pour le dense,7 pour LiDAR, budget4096 nœuds. Aucun cas
n’épuise ce budget ; le maximum observé est1637 nœuds. Les comparaisons
ci-dessous utilisent la profondeur7.

| Préfixe dense | Familles restantes, disque | Avec positivité | Tests témoins, disque → positivité | Visites de requête avec positivité |
|---|---:|---:|---:|---:|
| 8k | 1 904 | 1 746 | 419 303 → 174 883 | 383 815 |
| 16k | 3 523 | 2 819 | 818 387 → 340 331 | 773 681 |
| 32k | 6 886 | 3 264 | 1 571 790 → 647 760 | 1 576 494 |

Les scans complets des familles restantes coûteraient au moins
13,968M/45,104M/104,448M lectures, avant tri et sorties. La capture
constructeur26 close pendant cet audit, C64/Variance+Collectif, laisse
1 771/3 598/11 195 familles sur ces mêmes préfixes. Ses lecteurs live
normal/−O [ont été rejoués](PRODUCT26_READ.json), pas ses benchmarks.
Son pool, son code et ses coûts diffèrent : **pas un gain de temps
produit démontré**, ni une composition des deux filtres mesurée.

La permutation conserve une étendue comparable dès8k et révèle une limite
masquée par l’extension géométrique des préfixes initiaux : avec positivité,
822/1 585/3 183 familles restent. Les scans prévus font6,576M/25,360M/
101,856M, soit ×3,856 puis ×4,016. La carte n’a que101 nœuds aux trois
tailles. Une construction et des requêtes presque linéaires ne rendent
donc pas linéaire le travail restant. La profondeur5 rejette encore moins.

**LiDAR :** scans SemanticKITTI08 000000/000100/000200 traités séparément,
sans transformation ni fusion. Neuf arêtes initialement choisies à8k sont
rejouées à50k avec les mêmes IDs ; neuf arêtes plus larges complètent
l’échantillon. Les27 cas totalisent1 180 seeds, avec covers de8 à3 785
sites. Disque/positivité rejettent1 165/1 166 familles, pour12 995/7 767
tests de témoins et2 523/2 476 visites de requête. Les scans résiduels
prévus totalisent508/472 lectures. Le rejet supplémentaire concerne
scan100/50k, arête(0,4079). Les neuf arêtes larges rejettent déjà leurs
725 familles à la racine, dans les deux modes.

Ce sont des arêtes fournies explicitement, pas un échantillon représentatif
du générateur complet. Aucun avantage supplémentaire sur le filtre produit26
n’est mesuré. La préparation scalaire du prototype lit le nuage entier
par arête :594 000 préparations de formes sur ces27 cas, même quand le
cover est petit. **Ce chemin ne doit pas être porté tel quel.**

Les temps du préfixe autonome sont enregistrés, sans revendication de
performance : préparation, carte, requêtes, lecture et validation d’entrée
sont séparées ; JSON, destruction finale et tout aval restent exclus.
À32k dense initial,101 nœuds sont utilisés, mais la réserve en alloue4096
(294 912octets). Le pic des capacités des listes indécises atteint
1 933 312octets, cover inclus une seule fois. Ce n’est pas du RSS et
n’inclut pas les chevauchements transitoires de réallocation.

## Recommandation de port et parallélisation

1. **Contexte possédé par arête**, partageant propriétaire/index/cover,
   base, facettes et données de témoins. Utiliser les nœuds déjà certifiés
   du cover ; éviter une copie ou un scan du nuage par arête. La préparation
   des seules complétions reste à payer, même quand elle est partagée.
2. **Tâches de cellule indépendantes**, portant compte et témoins non
   consommés. Les listes parentales restent immuables et possédées tant
   que leurs enfants les empruntent. Pour des blocs de sites, ne créditer
   que des populations disjointes ; conserver le bloc non consommé lors
   d’un split. Une frontière partagée ne devient pas gratuite en mémoire.
3. **Requêtes par plages de seeds** sur la carte immuable. Une construction
   pilotée par les droites effectivement interrogées évite les cellules
   inutiles, notamment l’arête sans seed observée ici. Raffiner selon les
   familles encore indécises, puis compresser les parents dont tous les
   enfants sont certifiés/hors domaine. Une profondeur fixe n’a pas suffi.
4. Conserver le repli exact et mesurer construction, requêtes, résidu,
   événements, tris, sorties et mémoire simultanée. Ne pas reconstruire
   une carte pour chaque rang k : les minorants conservés peuvent servir
   les seuils inférieurs ; les compteurs q3 et q4 gardent leurs seuils
   propres. Le prototype ne porte ni cette interface multi-K ni des workers.

Le passage aux blocs de sites, le pilotage des raffinements et le coût
du générateur complet restent à juger. L’absence d’arrangement explicite
ne fournit aucune borne globale sous-quadratique.

## Preuves et reproduction

- [MATH.md](MATH.md), [domain_gate.py](domain_gate.py) :105 tétraèdres
  positifs,1 078 contrôles de puissance,210 contrôles de domaine, huit
  similitudes entières non axiales, extrêmesu16 et quatre fautes de modèle
  réfutées ; normal/−O identiques.
- [oracle_gate.py](oracle_gate.py) :189 appels pour chacun des trois
  parcours Release normal, Release−O et ClangASan/UBSan. L’oracle cartésien
  rationnel énumère3 351 présentations de tétraèdres par parcours, dont
  69 positifs possédés et17 acceptés, avec répétitions entre les trois K.
  Aucun seed d’un tétraèdre accepté n’est rejeté ; les deux décisions du
  filtre sont exercées. Il s’agit de contrôles finis d’audit.
- [Qualification](qualification/MANIFEST.json) : neuf commandes closes,
  compilations C++20 avec avertissements en erreurs, sorties et hashes
  des trois captures d’oracle conservés. Aucun échec de qualification.
- [Mesures](capture/MANIFEST.json), [lecteur](read.py) et sorties
  [normal](READ_normal.json)/[−O](READ_optimized.json) identiques ;
  [12 corruptions sémantiques](READER_GATE.json) détectées dans les deux
  modes, y compris après recalcul des hashes de fermeture.

Reproduction depuis le dépôt :

```sh
python3 morsehgp3D_v8/audits/q4_center_blocks_20260920/qualify.py qualification_replay
python3 morsehgp3D_v8/audits/q4_center_blocks_20260920/read.py morsehgp3D_v8/audits/q4_center_blocks_20260920/capture
```

Les runners refusent d’écraser une capture. Les entrées sont décrites dans
[fixtures.py](fixtures.py) et identifiées par SHA256. Les observations
produit sont séparées dans [REFERENCE_PINS.json](REFERENCE_PINS.json).
Les essais de fumée précédant le gel sont décrits dans [PREFLIGHT.md](PREFLIGHT.md).
P0 global, q3/q4 produit complet, FULL, GPU/G4 et massif restent ouverts.
GCP non utilisé.

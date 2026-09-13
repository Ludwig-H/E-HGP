# Propriété du plan axial et restriction q2 non vide

13 septembre 2026, sources publiées `f5430f57`. Cadre :
`exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Vérification complémentaire bornée, aucun changement moteur.

## Un raccourci vide déjà couvert par le besoin nul

Pour les **crédits internes q2 actuels**, un `CreditPlan` est vide si et
seulement si h=seuil−cœur vaut zéro. Le raccourci proposé par l'autre
auditeur sur `restriction->candidate_pairs()==0` est donc redondant
avec le retour h=0 déjà présent. Il ne fournit pas aujourd'hui un chemin
actif supplémentaire qui éviterait les tris. Cette observation ne remet
pas en cause sa proposition distincte de remontée des extrema de crédits.

Preuve : choisir une paire (a*,b*) qui minimise la distance entre les deux
facteurs finis non vides A et B. Pour un témoin strict z, le prédicat vérifie :

$$2H(a^*,b^*,z)=\lVert a^*-b^*\rVert^2-\lVert a^*-z\rVert^2-\lVert b^*-z\rVert^2>0.$$

Si z appartient à A, cette inégalité donne
$\lVert b^*-z\rVert^2<\lVert b^*-a^*\rVert^2$, contredisant la minimalité.
Si z appartient à B, elle donne de même
$\lVert a^*-z\rVert^2<\lVert a^*-b^*\rVert^2$. Les ancres elles-mêmes
ont H=0. Aucune unicité du minimum ni régularité n'est nécessaire.

Les crédits c_A(a*) et c_B(b*) certifient exclusivement des témoins de
ces facteurs, valables pour tout le facteur opposé : ils valent donc
tous deux zéro. Le cœur est extérieur à A et B et a déjà été soustrait
dans h. Si h>0, la paire vérifie c_A+c_B<h et reste dans le plan local.
Ses comptes axiaux sont eux aussi nuls : les deux modes axiaux actuels
et leur intersection la conservent. Si h=0, leurs factories retournent
directement un résidu vide.

La conclusion porte sur ces certificats internes, **pas sur les vraies
survivantes du census**. Un site extérieur non crédité au cœur peut
éliminer cette paire : A={(0,0,0)}, B={(100,0,0)}, z=(50,0,0), cœur vide,
h=1, donne des crédits internes nuls mais H=2500>0. Un futur plan qui
emploie de tels témoins devra réexaminer l'utilité du raccourci vide.

Le [juge](axis_lifetime_probe.cpp) confirme cette propriété sur 16 petits
propriétaires, facteurs placés dans les deux ordres et IDs permutés,
h initial 1/2/5/10 avec ou sans cœur : 42 plans locaux à besoin positif,
six à besoin nul, pour Pool, DualBlocks et Tubes. Le minimum est recherché
par 57 600 distances exactes ; 1 888 tests indépendants vérifient l'absence
de témoin interne strict sur les paires retenues. Ces recherches
quadratiques restent un oracle borné, jamais une proposition de préparation.

## Exception de construction et déplacement

À ce commit, `AxisQ2Plan` interdit copie, affectation par copie et
affectation par déplacement. Seul son constructeur de déplacement
`noexcept` est disponible. Il n'existe donc **aucune affectation axiale
sous bad_alloc à qualifier**, contrairement au cas `CreditPlan` précédemment
corrigé. Cette différence de contrat est contrôlée à la compilation.

Le nouveau chemin expose plutôt des allocations pendant la construction :
copies privées de crédits A/B, fenêtres et permutations additives, index
et fragments. Le juge injecte une panne à chaque position d'allocation
ordinaire jusqu'au premier succès, sur les deux modes restreints actifs.
Il vérifie ensuite l'identité du propriétaire, les crédits sources,
le plan axial déjà construit, tous ses compteurs, sa couverture et le
solde d'allocations ordinaires. Les deux chemins saturés passent aussi,
sans allocation produit. Le handle propriétaire est fourni par copie ;
aucune promesse de préserver un handle explicitement déplacé par l'appelant
n'est déduite de cette expérience.

Résultats : **50 pannes injectées**, quatre constructions terminées,
sans changement des objets préexistants ni référence propriétaire ou
allocation ordinaire résiduelle. Quatre déplacements, actifs ou saturés,
n'allouent pas ; les destinations gardent leurs décisions et tous leurs
compteurs. Les sources sont vidées et douze accès géométriques sont refusés.
L'injection ne prétend pas couvrir un allocateur suraligné ou une allocation
externe qui contournerait les opérateurs remplacés.

Les 96 plans restreints `Independent`/`Additive` de la petite campagne
confrontent `keeps` à l'expansion physique des fragments : 345 600 paires,
IDs d'origine contrôlés et absence de doublons. Les scénarios de durée
de vie exercent aussi 1 364 visites de nœuds et deux fusions de fragments.
Aucun défaut trouvé dans ce périmètre. La propriété des restrictions
après réaffectation/destruction est déjà couverte par la gate constructeur ;
elle n'est pas présentée ici comme une découverte supplémentaire.

Un mutant de copie temporaire retire `has_restriction` du constructeur
de déplacement. Il est réfuté avec code 1 : `keeps` réintroduit des paires
absentes des fragments, dès le déplacement vers le stockage optionnel
du juge. Le positif sort avec code 0. La première attente de message du
runner visait une détection plus tardive ; cette calibration est conservée
séparément dans le reçu et ne décrit pas un défaut du moteur.

## Rejeu autonome

Le [runner](axis_lifetime_checks.py) consomme les sept sources exactes du
commit publié et le juge neuf, sans utiliser de build épinglé. Le
[reçu](AXIS_LIFETIME_CHECKS.json) embarque ces sources, hashes, commandes,
sorties et variante mutée. GCC 13.3, C++20, `-Wall -Wextra -Wpedantic -Werror`,
`-O1`, UBSan sans récupération ; modes Python normal/−O de mêmes résultats.

```bash
python3 audits/morsehgp3D_v8_complementaire/axis_lifetime_checks.py --replay audits/morsehgp3D_v8_complementaire/AXIS_LIFETIME_CHECKS.json --output /tmp/axis_lifetime_replay.json
```

Cette porte ajoute les chemins d'allocation et le minimum transversal ;
elle ne remplace ni les oracles géométriques complets ni les mesures
additives publiées. Aucun temps, census général, tour FULL ou gain P0
global n'est revendiqué. GCP non utilisé.

# Audit indépendant courant de MorseHGP3D v7

Actualisé le 5 septembre 2026 sur `e6d33698e62ebecf74dff01c16d7de17149d7a4e`.
Toutes les écritures restent dans `morsehgp3D_v7/audits/`.

La qualification complète vise le snapshot **D**. Après la clôture de son
run, le constructeur a modifié quatre fichiers produit pour le prétest q2
**E** : CMake, `mutants.hpp`, `silent_incidence.hpp` et `meb_lazy_gate.cpp`.
Leurs changements locaux ne sont pas inclus dans cette publication d'audit.
Les 323 résultats et la stabilité avant/après ne leur sont pas transférés.
Les pins D conservés signalent ce décalage par le code de fraîcheur 1.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Avis pour le constructeur

**La MEB différée est justifiée et le verrou topologique de l'index est
levé sous les préconditions du pipeline.** Les deux conclusions associent
un argument statique et un juge indépendant du calcul produit. La garde
d'arrondi et ses replis ont aussi été exercés dans le pipeline. Aucun
nouveau défaut produit n'a été trouvé dans ces trois sous-audits.
Le constructeur peut s'appuyer sur ces acquis pour achever la qualification
horizontale ; ils ne restent pas des demandes de preuve génériques.

La composition S1 et la réduction horizontale disposent déjà de preuves
conditionnelles. La partition du front et les antichaînes de cover sont maintenant
[raccordées à l'index](AUDIT_RACCORD_INDEX_FRONT_20260905.md). Il reste
l'inventaire arithmétique des témoins du front et le domaine de compilation
retenu. Le domaine des plateaux, la verticale, le vote et les coûts gardent
leurs propres contrats. Le refus de `--require-exact` reste cohérent avec
cet état ; aucune promotion n'est faite par cet audit.

## Modèle lu et objet audité

Les parties I, pages PDF 35–76, puis II, pages PDF 77–134, du
[manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf) ont été lues
intégralement au début de cette session. SHA-256 :
`579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.

Le théorème 2 identifie les composantes des régions témoins et les
K-polyèdres. La proposition 5 autorise les cofaces élémentaires pour
préserver H0, sans identifier leurs adjacences à celles du graphe complet.
Le fait 12 caractérise la MEB et ses supports. Le § 9.1 conserve une
partition des facettes et un recouvrement des points, puis définit les
masses normalisées et le vote comme traitement aval.

La suppression Gabriel brute de la proposition 6 et du théorème 5 ne
peut pas servir de certificat : la
[contre-fixture E5](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
démontre une attache silencieuse nécessaire à une fusion ultérieure.
La [composition corrigée](../docs/PREUVE_HORIZONTALE_COMPOSITION.md)
est donc l'autorité conditionnelle utile. Les observations de percolation
et les expériences historiques de la thèse ne sont ni une borne de coût
ni des résultats d'exécution v7.

Le payload par défaut reste `verified_events_only`. L'option
`--complete-incidences` produit `normalized_horizontal_h0_candidate` :
composantes non triviales pour K supérieur à un, facettes latentes avant
leur première incidence retenue, et `born` comme première matérialisation
dans ce sous-flot. Les singletons restent normatifs à K=1. Ni une naissance
exhaustive dans Gamma, ni les cartes verticales, ni les poids du vote ne
sont contenus implicitement dans cet export.

## Preuves nouvelles et autorité de chaque résultat

| Domaine | Conclusion vérifiée | Preuves |
| --- | --- | --- |
| MEB différée q3/q4 | Signes, zéros, premier support, niveau et budgets conservés ; 89 ensembles, 431 appels et 6 176 puissances, deux mutants détectés | [Preuve et oracle rationnel indépendant](AUDIT_MEB_DIFFEREE_20260905.md) |
| Prétest q2 E postérieur | Identité et borne i64 ; oracle rejoué, 431 appels identiques à D et nouveau mutant q2 détecté ; qualification locale seulement | [Addendum E](ADDENDUM_MEB_Q2_E_20260905.md) |
| Morton, buckets et Karras | Partition, références, racine, couverture unique et boîtes justifiées ; 237 212 nuages par binaire, sept mutants structurels rejetés sous O2 et UBSan | [Preuve et oracle de trie](AUDIT_INDEX_20260905.md) |
| Domaine d'arrondi local | 40 appels aux quatre modes, un/deux threads ; filtres actifs au plus proche, replis entiers ailleurs, objets identiques | [Garde et replis exécutés](AUDIT_ARRONDI_20260905.md) |
| Reçus D et arithmétique | Intégrité des huit XML, sceaux, sources et binaires ; Boost réellement compilé pour sa porte entière | [Contrelecture de qualification](AUDIT_QUALIFICATION_20260905.md) |
| Reconstruction indépendante D | Construction neuve Release et 323/323 portes CPU, zéro échec/skip ; sources et 37 binaires stables | [Reçu du présent audit](receipts_20260905/release/summary.json) |

Les 323 exécutions D rapportées par le constructeur sont confirmées par
ses reçus, indépendamment de la nouvelle exécution de l'auditeur. Les
316 portes C conservent leur attribution historique. Les nouvelles
portes de MEB et d'index utilisent UBSan ; cela ne doit pas être appelé
une reconstruction ASan de toute la v7.

Les acquis précédents sont conservés :
[composition horizontale](REPONSE_AUDITEUR_COMPOSITION.md),
[couverture S1](S1_COURANT.md),
[interfaces](AUDIT_INTERFACES_20260904.md),
[archive corrigée A1](RETOUR_ARCHIVE_COURANT.md),
[mode mono](MONO_COURANT.md),
[census](CENSUS_AXIS_COURANT.md),
[lanes](ARITHMETIQUE_LANES_COURANTE.md),
[entiers larges](ARITHMETIQUE_LARGE_COURANTE.md) et
[classification des campagnes C1](CAMPAGNE_INCIDENCES_COURANTE.md).
Leurs reçus bruts ne sont pas réécrits.

## Prochaines fermetures utiles

1. **Assembler le certificat horizontal sur les sources courantes.**
   Réutiliser la preuve d'index et le raccord des piles WSPD et handles
   d'antichaîne désormais démontré ; conserver les contrôles par paire.
   Achever le grand-livre des expressions des témoins du front,
   notamment les changements d'échelle et racines corrigées de
   `spindle.hpp`, `sector_kill.hpp` et `chord_kill.hpp`. La borne de
   cardinal du pipeline existe ; la preuve de MEB et les portes
   Cassini/U320 déjà intégrées ne sont plus des obstacles ouverts.
2. **Fixer le domaine CPU et le domaine géométrique de la qualification.**
   Épingler une commande mono, ses options effectives et son environnement
   numérique, en réutilisant les marges prouvées et la garde exercée ici.
   Les fonctions internes de l'index restent préconditionnées ; le
   constructeur doit annoncer ou garder ces préconditions s'il les rend
   publiques. Les extra-shells pertinents sont encore refusés par la route
   complétée. Une extension aux plateaux demande ses propres fixtures de
   contacts égaux et de lots atomiques ; un refus déclaré n'est pas une
   erreur de MEB.
3. **Compléter l'objet demandé par le contrat industriel.**
   L'archive déclare `vertical_maps=none`. Définir et certifier les
   applications entre ordres, puis les contributions d'incidence du
   vote du § 9.1, sans les reconstruire depuis les seuls deltas H0.
   Préserver les facettes comme feuilles ; le recouvrement apparaît dans
   la projection et ne nécessite pas de laminarisation arbitraire.
4. **Réduire puis mesurer le coût global de la route complétée.**
   Les 550 supports possibles d'une MEB locale ne bornent ni les chaînes
   ni le catalogue de facettes. Mesurer leurs distributions et les
   reparcours de l'index. Les propositions précises du
   [retour mémoire](RETOUR_MEMOIRE_COURANT.md) restent applicables : borner
   la concurrence par les ordres effectifs, précontrôler les facteurs de
   coexistence et envisager une destination directe de l'expansion.
   Le proxy `--mem-budget` reste distinct du RSS ; l'archive atomique
   n'est pas un checkpoint moteur.

Ces étapes n'exigent ni Gamma exhaustif ni la mosaïque de Delaunay
supérieure dans le produit. Les oracles bornés restent dans les audits
et tests. La preuve de l'index ne change pas son coût linéaire ; la MEB
différée supprime des matérialisations locales sans ajouter de structure
globale. La résidence des candidats et incidences reste à qualifier.

Le [contrat de performance](../docs/CONTRAT_PERFORMANCE.md) porte sur
50 000 points et toute la tour K=1..10 sous une seconde, avec repli
K=1..5, puis 100 ms. Il impose d'abord le mono, ensuite le multi-CPU,
puis le GPU. Aucun temps de compilation, de CTest ou de sonde de cet
audit ne satisfait ce contrat. Les paliers massifs restent distincts.

## Portée et traçabilité

Les sources v7 examinées appartiennent au HEAD indiqué, sans delta produit
local à l'ouverture. Quatre fichiers v6 modifiés et trois fichiers v6
nouveaux étaient déjà présents ; ils ne sont pas utilisés ou inclus dans
cette publication. Le travail est effectué sur `main`, sans nouvelle
branche et sans modification du registre officiel.

La [validation courante](receipts_20260905/validation_current.json)
sépare les contrôles nouvellement exécutés et les reçus historiques
contre-vérifiés. Le contrôleur de fraîcheur ne qualifie que les octets
épinglés de D ; l'absence de delta produit à l'ouverture ne décrit pas le
worktree après le début de E. Les résultats locaux CTest restent distincts
des runs GitHub.
GCP non utilisé ; aucun résultat GPU attribué à cet audit.

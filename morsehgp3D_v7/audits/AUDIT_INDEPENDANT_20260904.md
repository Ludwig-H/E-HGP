# Audit indépendant courant de MorseHGP3D v7

Actualisé le 5 septembre 2026 depuis `a32dc78f`, à la suite de la publication constructeur E `2b94abddfde08101607f4639d42149156fb39e6c` ; qualifications propres D et E distinguées.
Toutes les écritures restent dans `morsehgp3D_v7/audits/`.

La suite **D** de 323 portes a été reconstruite et exécutée par l'auditeur.
Le delta q2 **E** du constructeur dispose maintenant de ses propres
324 portes Release et de deux campagnes ciblées de 33 portes, Release
et ASan/UBSan, dont les preuves brutes sont contre-vérifiées ici.
Les quatre fichiers produit E restent sous la responsabilité du constructeur.
Le [manifeste courant](validation_current.json) reconnaît séparément les octets D et E et affiche la portée correspondante ; leur mélange est refusé.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Avis pour le constructeur

**Les bornes des témoins sont raccordées au code compilé, et la
qualification intégrée E est close sur ses preuves propres.** Les trois
sondes du front passent en O2 et O1 UBSan, avec six vrais mutants produit
détectés. Aucun nouveau défaut produit n'a été trouvé. Les acquis MEB,
index/front et garde d'arrondi restent fermés ; le constructeur peut
s'appuyer dessus pour achever le certificat horizontal.

La composition S1 et la réduction horizontale disposent déjà de preuves
conditionnelles. La partition du front et les antichaînes de cover sont maintenant
[raccordées à l'index](AUDIT_RACCORD_INDEX_FRONT_20260905.md). Les bornes opérationnelles des témoins sont fermées pour les fuseaux, secteurs, cordes et cellules ; le [domaine CPU](DOMAINE_CPU_COURANT.md) et les [frontières compilées](receipts_front_compiled_20260905/README.md) sont documentés. Le domaine des plateaux, la verticale, le vote et les coûts gardent
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
| Prétest q2 E | Identité et borne i64 ; oracle indépendant, 431 appels identiques à D et nouveau mutant q2 détecté | [Addendum E](ADDENDUM_MEB_Q2_E_20260905.md) |
| Morton, buckets et Karras | Partition, références, racine, couverture unique et boîtes justifiées ; 237 212 nuages par binaire, sept mutants structurels rejetés sous O2 et UBSan | [Preuve et oracle de trie](AUDIT_INDEX_20260905.md) |
| Domaine d'arrondi local | 40 appels aux quatre modes, un/deux threads ; filtres actifs au plus proche, replis entiers ailleurs, objets identiques | [Garde et replis exécutés](AUDIT_ARRONDI_20260905.md) |
| Témoins du front | Bornes et raccord compilé fermés : trois sondes O2/UBSan ; base A=B=1, racines, contacts stricts, 38 400 cellules par build et coordonnées sur 98 bits | [Fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md), [cellules](ARITHMETIQUE_CELLULES_COURANTE.md) |
| Reçus D et arithmétique | Intégrité des huit XML, sceaux, sources et binaires ; Boost réellement compilé pour sa porte entière | [Contrelecture de qualification](AUDIT_QUALIFICATION_20260905.md) |
| Reconstruction indépendante D | Construction neuve Release et 323/323 portes CPU, zéro échec/skip ; sources et 37 binaires stables | [Reçu du présent audit](receipts_20260905/release/summary.json) |
| Qualification intégrée E du constructeur | 324/324 Release, 33/33 ciblés Release et 33/33 ASan/UBSan ; XML, journaux complets, inventaires, 140 sources et binaires contre-vérifiés | [Qualification E](AUDIT_QUALIFICATION_20260905.md) |
| Paires D/E avec complétion | Trois paires s=8/10/12 égales, objets aussi égaux entre séparations ; compteurs avant préfiltre distincts ; une paire par s, sans conclusion statistique | [Pièces brutes et contrelecture](AUDIT_QUALIFICATION_20260905.md) |

Les 323 exécutions D rapportées par le constructeur sont confirmées par
ses reçus, indépendamment de la nouvelle exécution de l'auditeur. Les
316 portes C conservent leur attribution historique. Les nouvelles
portes de MEB et d'index utilisent UBSan ; cela ne doit pas être appelé
une reconstruction ASan de toute la v7.

Les acquis précédents sont conservés :
[composition horizontale](REPONSE_AUDITEUR_COMPOSITION.md),
[couverture S1](S1_COURANT.md),
[interfaces](AUDIT_INTERFACES_20260904.md),
[archive corrigée A1](AUDIT_INTERFACES_20260904.md#nettoyage-a1-et-publication),
[mode mono](MONO_COURANT.md),
[census](CENSUS_AXIS_COURANT.md),
[lanes](ARITHMETIQUE_LANES_COURANTE.md),
[entiers larges](ARITHMETIQUE_LARGE_COURANTE.md) et
[classification des campagnes C1](AUDIT_QUALIFICATION_20260905.md#harnais-et-classification-des-campagnes).
Leurs reçus bruts ne sont pas réécrits.

## Prochaines fermetures utiles

1. **Assembler le certificat horizontal réduit dans son domaine CPU.**
   Les commandes et préconditions numériques sont [déclarées](DOMAINE_CPU_COURANT.md).
   Garder les refus d’extra-shell pertinents et nommer le domaine de
   régularité. Une extension aux plateaux demande ses propres fixtures de
   contacts égaux et de lots atomiques ; un refus déclaré n’est pas une
   erreur de MEB. Les API internes conservent leurs préconditions.
2. **Compléter l'objet demandé par le contrat industriel.**
   L'archive déclare `vertical_maps=none`. Définir et certifier les
   applications entre ordres, puis les contributions d'incidence du
   vote du § 9.1, sans les reconstruire depuis les seuls deltas H0.
   Préserver les facettes comme feuilles ; le recouvrement apparaît dans
   la projection et ne nécessite pas de laminarisation arbitraire.
3. **Réduire puis mesurer le coût global de la route complétée.**
   Les 550 supports possibles d'une MEB locale ne bornent ni les chaînes
   ni le catalogue de facettes. Mesurer leurs distributions et les
   reparcours de l'index. Les propositions précises du
   [retour mémoire](RETOUR_MEMOIRE_COURANT.md) restent applicables : borner
   la concurrence par les ordres effectifs, précontrôler les facteurs de
   coexistence et envisager une destination directe de l'expansion.
   Le proxy `--mem-budget` reste distinct du RSS ; l'archive atomique
   n'est pas un checkpoint moteur.

La base des secteurs peut être simplifiée à A=B=1 dans son domaine,
après comparaison des objets et des plafonds ; cette optimisation reste
facultative. Ni la demande de frontières compilées ni la qualification E
ne sont conservées comme verrous ouverts.

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

Les sources E exécutées sont le delta q2 épinglé sur D ; leurs inventaires
sont stables pendant les campagnes. Les travaux concurrents v6 et les
documents/reçus du constructeur restent hors des écritures de cet audit.
Le travail est effectué sur `main`, sans nouvelle
branche et sans modification du registre officiel.

La [validation courante](validation_current.json)
sépare les contrôles nouvellement exécutés et les reçus historiques
contre-vérifiés. Le contrôleur de fraîcheur ne qualifie que les octets
épinglés d’une variante complète avec sa portée propre. L’ancien manifeste
daté reste une pièce historique ; il n’est plus le défaut du contrôleur. Les résultats locaux CTest restent distincts
des runs GitHub.
GCP non utilisé ; aucun résultat GPU attribué à cet audit.

Douze notes transitoires ont été [consolidées](receipts_front_20260905/documentation_retirement.json) dans leurs rapports de référence. Les anciennes demandes déjà satisfaites sont retirées des entrées actives ; fixtures et reçus restent conservés.

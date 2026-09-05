# Audit indépendant courant de MorseHGP3D v7

Actualisé le 5 septembre 2026 depuis `71895104c7d750fdd6ef0a313cfb09e9ac161204` ; certificat horizontal réduit E, contrats verticaux/pondérés et qualification F distincte.
Toutes les écritures restent dans `morsehgp3D_v7/audits/`.

La suite **D** de 323 portes a été reconstruite et exécutée par l'auditeur.
Le delta q2 **E** du constructeur dispose maintenant de ses propres
324 portes Release et de deux campagnes ciblées de 33 portes, Release
et ASan/UBSan, dont les preuves brutes sont contre-vérifiées ici.
Les quatre fichiers produit E restent sous la responsabilité du constructeur.
Les 339 portes Release F et deux campagnes ciblées de 48 portes sont également contre-vérifiées depuis leurs propres reçus. Le [manifeste courant](validation_current.json) reconnaît séparément les octets D/E/F et affiche leur portée ; leur mélange est refusé.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Avis pour le constructeur

**Le [certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md) est fermé sur E dans son domaine CPU explicite.** La preuve relie la fenêtre de boules, le catalogue direct, les suffixes d'ancrage, le fold et les seuls deltas exportés. La bijection par inclusion des facettes conserve les points et commute entre coupes ; la régularité géométrique globale n'est pas exigée.

La nouvelle sonde du pipeline passe en O2/O1 UBSan : 60 ordres, 840 coupes et 1 124 carrés de naturalité par build, avec détection d'une attache supprimée. Le fold séparé passe 272 coupes et sept vrais mutants. Aucun nouveau défaut produit n'a été trouvé. Les [18 gardes de domaine](receipts_horizontal_20260905/domain/results.json) précisent notamment K1, les collinéaires acceptés et les refus d'extra-shell.

F possède un [lemme de conservation de pile](receipts_horizontal_20260905/f_delta/review.json) favorable et sa [qualification intégrée propre](receipts_vertical_20260905/f_qualification/). Le certificat exécuté reste attribué aux octets E figés. Le [contrat vertical](CONTRAT_VERTICAL_COURANT.md) ferme la fonctionnalité et la naturalité mathématiques : une ancre certifiée par composante suffit, avec un resolver et un export à construire. Les [masses/vote](CONTRAT_MASSES_VOTE_COURANT.md) disposent d’un contrat d’incidence explicite et de contre-fixtures ; leur supplément pondéré reste à fournir. Identités publiques, plateaux à étendre et coûts gardent leurs obligations distinctes. Le statut demeure `not_claimed` et `--require-exact` refuse.

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
| Composition horizontale réduite E | Preuve assemblée ; pipeline et lecteur seuls contre Gamma rationnel, fold et niveaux égaux, domaine CLI | [Certificat et reçus](CERTIFICAT_HORIZONTAL_COURANT.md) |
| Qualification F | Conservation LIFO/masques/comptes ; 339/339 Release et 48/48 ciblées Release/ASan-UBSan contre-vérifiées | [Reçus propres F](receipts_vertical_20260905/f_qualification/) |
| Verticale et masses | 764 images, 720 carrés de naturalité et 400 compositions par build E source ; familles d’incidences et projection pondérée distinguées | [Contrats et fixtures](receipts_vertical_20260905/README.md) |
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

1. **Compléter l'objet demandé par le contrat industriel.**
   L'archive déclare `vertical_maps=none`. Réaliser le resolver d’ancre
   et son export selon le contrat vertical désormais démontré, puis
   sérialiser les contributions d’incidence du vote du § 9.1 selon
   leur univers déclaré ; les seuls deltas H0 ne les déterminent pas.
   Préserver les facettes comme feuilles ; le recouvrement apparaît dans
   la projection et ne nécessite pas de laminarisation arbitraire.
2. **Réduire puis mesurer le coût global de la route complétée.**
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
facultative. Ni la demande de frontières compilées, ni la qualification E, ni le
certificat horizontal réduit ne restent des verrous ouverts sur ces octets.

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

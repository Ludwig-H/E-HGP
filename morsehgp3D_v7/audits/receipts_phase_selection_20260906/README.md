# Admission par phases et sélection stable des ancres

6 septembre 2026. Lecture indépendante après 56c6e0a8. Cadre : phase=exploration_v7_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=audit_independant_math_and_architecture, public_status=not_claimed. Écritures dans audits/ seulement ; aucun moteur, compilation, benchmark ou GCP.

**Deux changements peuvent supprimer du travail ou des refus inutiles sans changer la hiérarchie : adapter l’admission au payload de chaque phase et construire les sélections d’ancres sans en changer l’ordre.** Ce paquet en établit les conditions ; il ne qualifie pas une intégration C++ ni un gain de temps.

## 1. Le census nominal ne possède qu’un tableau BallData

Noter E le nombre de candidats bruts, U leur nombre après RLE et S le nombre de survivantes du préfiltre. Alors S≤U≤E. Noter C, V et D les tailles ABI de BallCandidate, Survivor et BallData. La [source expand.hpp](../../src/pipeline/expand.hpp) établit les propriétaires suivants.

| Phase CPU | Majorant des tableaux logiques nommés |
| --- | --- |
| Préfiltre, avant de connaître S | U(C+2V) |
| Census nominal, après préfiltre | UC+S(V+D) |
| Census sous keep-ball-chunks | UC+S(V+2D) |

Le préfiltre accumule les Survivor par tranches, réserve leur destination, les copie puis libère chaque tranche consommée. Il peut donc posséder deux fois leur population. Pour les seules entrées construites, le pic exact de cette fusion vaut S+max(cᵢ), où cᵢ est la taille d’une tranche ; 2S reste le majorant simple et 2U son prétest. Cette identité d’entrées construites ne borne pas les capacités des vecteurs.

Le census détruit l’ancienne sortie avant d’allouer staged(S), écrit chaque survivante à son propre indice, puis publie par swap après validation complète. Seul le mutant keep-ball-chunks crée shadow=staged. Mais **les U candidats restent tous présents** : Survivor::idx pointe dans ce vecteur, sans compactage à S. Les candidats et Survivor sont libérés seulement après le retour du census dans la sonde.

Avec C=144, V=16, D=224 sur l’ABI examinée, on obtient **176U**, puis **144U+240S**, au lieu du prétest historique 608U. À S=U, le second vaut 384U. Cela réduit la réserve logique demandée ; aucune allocation réelle n’est économisée par ce seul changement de garde. Le relevé ABI C/D est retrouvé dans une capture constructeur ; V=16 suit du layout u32/u64 de cette ABI. Aucun nouveau sizeof exécuté par l’auditeur.

La déduplication utilise erase sans diminuer la capacité du vecteur de candidats. Cette capacité, celles des tranches, les piles, in/sh, statistiques, objets locaux des ouvriers et l’index ne sont pas inclus. Le [contrat de résidence](../../docs/RESIDENCE_MASSIVE.md) doit donc conserver la distinction entre proxy logique et RSS. Une somme de termes se contrôle sans débordement, par soustractions du budget restant ou arithmétique élargie ; chaque sizeof doit venir de la compilation, pas des constantes de notre modèle.

### Frontières accessibles et route fusionnée

Sur cette ABI, 176U≤288E. La garde du tri/fusion à 2EC a déjà admis ce dernier montant : **un refus du nouveau prétest préfiltre ne peut pas être atteint dans ce pipeline à budget inchangé**. Sa frontière exacte et budget−1 se testent dans un helper pur. Pour exercer un nouveau refus du pipeline, viser la garde census après un vrai préfiltre, avant staged.

Le modèle fixe E=U=100, S=80 et budget=32000 : le tri demande 28800, le census correct 33600 et la formule fautive qui remplace U par S seulement 30720. Le mauvais calcul admettrait donc un payload au-delà du budget après un tri accepté. Avec S=U=100 et budget=38400, l’ancien 60800 refuse alors que les nouvelles phases et le tri sont admissibles. Ces nombres sont des contre-modèles d’admission, **pas des nuages exécutés**. Une intégration doit fournir les fixtures géométriques correspondantes et comparer les objets au budget large.

La couture [prefilter_census_override de run.hpp](../../src/pipeline/run.hpp) réalise les deux phases dans un seul appel. Elle ne donne pas S à l’appelant avant les allocations census. Lui appliquer une garde sur S après retour serait trop tard : conserver son admission préalable distincte ou prévoir une admission interne avant allocation. La sonde CPU ordinaire sépare déjà les appels. Les refus historiques de selftest qui imposent l’ancien refus avant préfiltre doivent changer de contrat explicitement si cette route est modifiée ; les refus transactionnels du census restent nécessaires.

## 2. Réaliser la borne de sélection en conservant Morton

La [preuve précédente](../receipts_block_histograms_20260906/README.md) établit le partage des listes B et la saturation globale à N=need. Le [constructeur](../../docs/ELIMINATION_BLOCS_WSPD.md) annonce déjà une sélection en O(|A|+|B|+N+P), où P est le nombre de paires survivantes. Voici une réalisation qui préserve aussi leur ordre, sans rebalayer B à chaque seuil.

1. Déterminer les seuils demandés T={N−h_a(a) : h_a(a)<N}. Regrouper les indices B en buckets de crédit min(h_b,N).
2. Construire une liste doublement chaînée des indices B dans leur ordre initial. Descendre t=N,…,1 ; supprimer le bucket t, en O(1) par indice. Après suppression, la liste contient exactement les h_b<t, dans l’ordre Morton initial.
3. Copier cette liste uniquement si t appartient à T. Enfin parcourir A dans son ordre initial ; une ligne vivante lit la copie correspondant à N−h_a(a).

Chaque indice B est supprimé au plus une fois. Si M est le total des indices copiés, **M≤P** : chaque liste demandée sera utilisée par au moins une ligne A, éventuellement plusieurs. Construction et sortie coûtent donc O(|A|+|B|+N+P). La mémoire de cette réalisation vaut O(|A|+|B|+N+M), avec M≤min(P,N|B|). Les seuils peuvent osciller dans l’ordre A : ne pas annoncer une mémoire O(|B|+N) en oubliant les copies. Ce stockage doit être compté par worker ; un petit facteur peut rester moins coûteux au scalaire.

Les buckets commandent des suppressions, **pas l’ordre d’émission**. Pour N=2, h_a=(0), h_b=(1,0), les deux paires survivent dans l’ordre B0,B1. Un tri direct par crédit donne B1,B0. L’ensemble des paires et leurs masses ne suffisent pas à contrôler l’ordre. Les listes stables préservent aussi les crédits exacts remis aux survivantes ; les règles d’arrêt et d’overshoot du producteur restent à conserver lors du raccord.

Cette construction réduit le travail de sélection après histogrammes. Elle ne réduit ni leurs évaluations géométriques ni le nombre d’ancres survivantes. Le calcul quadratique des échecs de témoins demande encore les certificats de blocs ou une autre recherche prouvée.

## 3. Saturation : portée des valeurs et comptabilité des blocs

L’écrêtage global h↦min(h,N) conserve les rejets et les crédits des survivantes. Un seuil local à une ligne ne peut pas être réemployé comme valeur globale. La contre-fixture u16 colinéaire A={0,1}, B={100,101,102}, X=A∪B, q2/smax=3 satisfait la séparation s8. Son cœur est vide, N=2, h_a=(1,0), h_b=(0,1,2). Couper h_b à N−h_a(0)=1 puis réutiliser ces valeurs laisse vivre à tort la paire (1,102). Le cap doit porter son domaine de validité ou être repris jusqu’au seuil requis.

Pour un histogramme par blocs, conserver une partition de sa population originale : **positions testées individuellement + positions certifiées positives par blocs + positions rejetées par blocs + positions non visitées après saturation**. Les quatre classes sont disjointes. Un bloc positif de dix positions avec seulement deux crédits manquants ajoute deux au compte écrêté, mais dix à sa population certifiée. Le mutant qui ajoute deux aux deux compteurs perd huit positions. P_factor doit compter les appels ponctuels physiques, avec des compteurs séparés pour les tests de blocs et les positions non visitées.

Le seuil N−min_B h_b ne réduit rien sur un facteur entier : son minimum est zéro, comme [déjà démontré](../receipts_block_histograms_20260906/README.md). Un sous-groupe peut avoir un minimum positif, mais il faut alors conserver le domaine du cap et les populations parentales ; aucun nouveau rejet global n’en découle.

## Reproduction et portée

[countermodels.py](countermodels.py) n’importe aucun code produit et n’utilise pas assert. Depuis la racine, exécuter python3 -B puis python3 -B -O sur ce fichier. Les [résultats normaux](normal.json) et [optimisés](optimized.json) ont chacun le code 0 et les mêmes octets : 1 296 partitions de tranches, 5 151 frontières d’admission et 12 168 sélections, dont 8 224 non vides et 18 774 paires émises. Les cas fixés réfutent la compaction fictive des candidats, le débordement de somme u64, le tri des sorties par crédit, le réemploi global d’un cap local et la confusion entre crédit écrêté et population de bloc.

Les [hashes de lecture](source_review.json) bornent la contrelecture statique. Les variantes de qualification C++ D–O restent inchangées. Aucune nouvelle qualification du code constructeur, des mesures multi-CPU, de la tour intégrée ou des contrats 50k.

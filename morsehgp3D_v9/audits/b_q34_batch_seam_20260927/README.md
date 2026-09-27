# Bandes q3/q4 : raccord des survivants et des compteurs

27 septembre 2026, base `24308be81`. Cadre
`exploration_v9_hors_registre`, `cpu_reference`, `quantized_u18_input_only`,
`audit_q34_batch_seam`, `public_status=not_claimed`. Prototype distinct du
moteur, aucun appel GCP. Les anciens plans et les bandes sont volontairement
construits ensemble dans cette porte : **ce n'est pas une mesure de gain**.

## Résultat et décision de port

576 configurations donnent exactement le même vecteur ordonné de survivants,
les mêmes masques et les mêmes masses de rejet que le filtre CPU actuel.
Release et ASan/UBSan/LSan passent ; deux mutations causales sont réfutées
dans chaque build. [Dix commandes closes](../../receipts/q34_batch_seam_20260927/README.md),
lectures normales et `python3 -O` concordantes.

Le raccord est possible sans tableau d'une entrée par paire rejetée.
Deux obligations restent explicites pour le futur moteur :

1. **Restaurer l'ordre des survivants avant S3**, pas après la production des
   certificats et records qui utilisent leurs ordinaux.
2. **Séparer la masse logique P du travail ponctuel E**. Le champ historique
   `expanded_pairs` sert à fermer la partition, mais ne doit plus être
   présenté comme le nombre de tests de paires exécutés.

Les [bandes directement construites](../b_q34_direct_bands_20260927/README.md)
traitent la préparation ; la présente porte traite séparément leur couture
avec le filtre. Aucun appel de la tour FULL ni du pipeline natif complet
n'est exécuté ici. Il faudra qualifier leur composition dans le moteur.

## Pourquoi les survivants ne changent pas

Pour une paire `(a,b)`, les témoins certifiés du facteur A sont dans A sans
a ; ceux du facteur B sont dans B sans b. Les facteurs sont disjoints.
Un témoin universel pour le facteur opposé est en particulier un témoin
strict pour cette paire. Donc, si les crédits locaux atteignent le seuil
d'une voie, le filtre ponctuel complet la rejetterait aussi.

Le filtre ponctuel `filter_q34_witnesses` parcourt l'index global avec des
bornes sûres ; aux feuilles, les prédicats sont exacts. Le retrait préalable
d'une voie ne change ni la population ni le seuil de l'autre. Il ne peut
donc changer aucune décision finale. Le plan n'est pas forcément maximal :
le filtre ponctuel termine les décisions sur son résidu. Il repart de zéro,
**sans crédit hérité**. Les preuves géométriques du Pool sont celles du
[prototype par facteurs](../b_q34_factor_plan_20260926/README.md), pas une
nouvelle caractérisation des hiérarchies.

Conséquence utile mais limitante : les S survivants finaux ne diminuent pas.
Le gain recherché porte sur la préparation et les recherches inutiles de
S2 ; il ne supprime pas les certificats S3, les lanes S4 ni la tour qui
consomment ces mêmes S survivants.

## Ordre et stockage

Les facteurs sont permutés par classes de crédits. Les bandes sont disjointes
mais leur ordre n'est en général plus l'ordre des rangs spatiaux initiaux.
`run_wspd_q34_batched` exige l'ordre des rectangles, puis celui des lignes
de leur produit A×B. Trier simplement par les deux IDs ou les deux rangs
globaux ne reproduit pas ce contrat.

Chaque survivant reçoit une clé u64 : préfixe des masses P des rectangles
non rejetés, plus l'ordinal dans le produit original. Les masses, préfixes
et additions sont vérifiés. Seuls les S survivants portent cette clé ;
aucun vecteur de permutation de taille P ou E n'est créé. Le tri, la
vérification d'unicité et l'extraction du vecteur final sont réellement
exécutés dans la porte. L'orientation `(a,b)` est conservée.

Coût de ce rétablissement scalaire : O(S log(1+S)), mémoire O(S), en plus
des plans et du traitement des E paires. Un futur tri radix GPU est une
piste, pas une durée acquise. Les ordinaux S3/S4 et du payload q3 pourront
rester inchangés si l'ordre est rétabli **immédiatement à la sortie S2**.
Le rétablir après S4 exigerait aussi de permuter toutes les données associées.

## Compteurs : ne pas déclarer du travail fictif

Pour chaque rectangle non rejeté, P est sa masse entière et E la masse de
l'union des bandes. Pour une voie active q, E_q est sa masse résiduelle et
D_q ses rejets supplémentaires effectivement obtenus par recherche ponctuelle.

- P = E + (P−E) : évaluations ponctuelles et rejets implicites sont disjoints.
- Le rejet logique de la voie q vaut (P−E_q) + D_q.
- Le nombre de recherches ponctuelles exécutées est E, pas P.
- Le nombre de survivants est S ; le rejet logique de paires vaut P−S.

Le prototype conserve `expanded_pairs=P` dans son résultat pour la
compatibilité structurelle avec `Q34FilterBatch`, mais publie E séparément
et vérifie `pw.queries == E`. Ce n'est pas une redéfinition silencieuse du
compteur natif : le moteur n'est pas modifié ni appelé avec ce résultat.

Le moteur actuel impose P dans son contrôle de forme, puis affecte
`witness.pairs.queries = expanded`. Brancher le prototype tel quel ferait
donc annoncer P recherches malgré seulement E exécutions. Il faut ajouter
des champs versionnés pour P, E, rejets de blocs et rejets ponctuels, puis
adapter validateurs et agrégations. Conserver séparément les identités de
couverture utilisées par `validate_completion` ; remplacer partout P par
E casserait ces identités. Ne pas compter les rejets de blocs comme des
visites d'index ou des tests de points exécutés.

## Portée des tests

Uniforme, terrain, amas et rangées, n=16/40/80, ordre initial/inversé,
K2/3/5/10, s8/10/12, front Pure/MidpointSamples : 576 cas. Le filtre de
référence est `run_q34_filter_batch_cpu(..., 1)` sur les mêmes rectangles.
Les deux voies et le rétablissement d'ordre sont exercés positivement :

| compteur cumulé | valeur |
| --- | ---: |
| rectangles | 347 810 |
| masse logique P | 479 064 |
| recherches ponctuelles E | 388 468 |
| paires éliminées implicitement | 90 596 |
| rejets implicites q3 / q4 | 92 240 / 62 264 |
| survivants S | 380 712 |
| cas nécessitant un réordonnancement | 208 |

Ces cumuls de petites fixtures ne sont ni une réduction mesurée sur LiDAR,
ni un test de croissance 8k/16k/32k, ni le contrat FULL. La géométrie du
filtre et une partie des helpers sont partagées avec la référence : cette
porte différentielle n'est pas un second oracle géométrique indépendant.
La fonction `candidate` est un adaptateur interne alimenté par ce vrai
front, pas une factory publique : validation générale K/nœuds/masques et
frontière des rangs u32 devront reprendre celles du batch natif au port.
La contrelecture indépendante du raccord n'a pas trouvé de divergence
fonctionnelle dans ce périmètre.

Mutants sémantiques du même exécutable : `no-sort` retourne 1 avec
`factor_plan.seam_original_order` ; `lose-implicit` retourne 1 avec
`factor_plan.seam_lane_accounting`. Ni crash ni diagnostic sanitizer ne
servent de preuve. Les scripts, sources, bibliothèques empruntées et
binaires sont épinglés ; aucune bibliothèque existante n'est reconstruite.
Les huit empreintes attendues de binaire/cache/options/lien sont présentes
et ont été relues. Limite du lecteur r1 : il parcourt cet inventaire sans
en imposer l'ensemble exact des clés ; une suppression volontaire d'une
entrée n'est pas encore un mutant de reçu testé. Il ne faut pas présenter
ce lecteur comme une preuve contre toute falsification de manifeste.

```bash
python3 -B morsehgp3D_v9/audits/b_q34_batch_seam_20260927/run.py check
python3 -B -O morsehgp3D_v9/audits/b_q34_batch_seam_20260927/run.py check
```

Ces commandes relisent les preuves et dépendances LIVE ; elles ne relancent
pas les exécutables. Le build `build/v9-audit-q34-seam-20260927-r1` et la
capture sont clos, à ne pas écraser. Toute évolution utilise de nouvelles
destinations et une nouvelle qualification.

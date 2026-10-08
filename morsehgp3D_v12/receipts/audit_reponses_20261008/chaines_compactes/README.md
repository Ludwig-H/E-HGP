# Compactage des chaînes à réparer — modèle, 8 octobre 2026

**Piste secondaire à G**, sans modification produit, CUDA natif ni nouvelle
mesure. Les pins fixent `finish_driver.hpp`, `finish_kernels.hpp` et
`finish_level.hpp` à `d2f39fe82`, et les comptes observés à la session I
`0a5ebf29f`. [Modèle](model.py), [rejeu](check.py), [résultats](results.json),
[empreintes des entrées](pins.json). Aucun gain en millisecondes acquis.

Le coût visé est uniquement le téléchargement initial de `finish_repair`
(lignes 171–181) : `verdict` u32 + `order` u32 + `keys` u64, soit **16 C**
octets pour C boules, dès qu’une chaîne nécessite un repli. Les niveaux,
clés de positions, tris exacts CPU, remontées des ordres réparés et contrôle
final subsistent. Le code mesuré n’emploie pas encore ce compactage.

## Domaine et équivalence

On se place après le **premier** `finish_order` : clés F3 positives finies,
triées par bits croissants ; ordre des identifiants issu du tri stable ;
`ChainKernel` a fini et seules les fautes `kFaultMisordered` sont admises.
Pour l’arête `(i−1,i)`, poser `U[i] = (key_order(keys[i−1],keys[i]) == 0)`.
Les arêtes certaines, de valeur −1, ne peuvent avoir le verdict inversé ;
les verdicts inversés se trouvent donc dans U. Les doublons, autres fautes,
verdicts inconnus et incohérences restent des refus, sans sortie partielle.
Le modèle refuse aussi les états étrangers à ce domaine, notamment une
inversion sur une arête certaine. Son oracle ne requalifie pas F4.

Les sommets `0..C−1` et les arêtes U forment des intervalles disjoints.
**Sélectionner exactement les composantes contenant une arête inversée.**
Une composante singleton n’est jamais sélectionnée. Pour la première arête
inversée d’une composante, `chain_around` s’étend jusqu’aux deux premières
arêtes certaines (ou bords du tableau), donc couvre la composante entière.
Puis `covered=e` saute toutes ses autres inversions. Ainsi `collect_chains`
produit précisément ces composantes, en ordre croissant, et leurs éléments
dans l’ordre initial. Il faut préserver les bornes `[s,e)`, les IDs **et**
leurs clés associées. Les seules paires inversées ne suffisent pas : dans
un intervalle de trois sommets, une arête incertaine ordonnée suivie d’une
inversion entraîne le transfert des trois sommets, pas des deux derniers.

Ce lemme ne s’applique pas arbitrairement après la réparation, où les clés
F3 peuvent ne plus être croissantes. Le contrôle exact final complet de
`finish_check` doit rester obligatoire, avec refus de toute faute résiduelle.

## Schéma de compactage

1. Marquer début de composante à `i=0` et après chaque arête certaine,
   fin avant la suivante. Ne couper ni à une limite de warp/tuile ni à
   une arête incertaine dont le verdict est pourtant ordonné.
2. OR segmenté des indicateurs d’inversion, une fois vers l’avant et une
   fois vers l’arrière. Leur OR sélectionne tous les sommets de la composante
   affectée, y compris ceux précédant sa première inversion.
3. Scan des sommets sélectionnés pour leur destination compacte, et scan
   des débuts sélectionnés pour le numéro compact de la chaîne. Les débuts
   et fins écrivent les bornes ; chaque sommet écrit son couple `(ID,clé)`
   à une destination unique. L’ordre des composantes et éléments est stable.
4. Compter Q chaînes et R éléments, vérifier les capacités, réserver selon
   le budget avant publication, puis transférer seulement ces sorties.
   Les tailles et espaces GPU/pinned doivent être comptabilisés, y compris
   les tableaux de scans conservés entre les appels.

Le monïde segmenté est `(h,a) ⊗ (k,b) = (h∨k, b si k sinon a∨b)` : le
marqueur de droite coupe le cumul de gauche. Il est associatif, d’identité
`(faux,faux)` ; c’est aussi vérifié sur les 64 triplets booléens. Le modèle
emploie un scan Blelloch orienté et un padding à la puissance de deux
supérieure ; ses quatre scans font au plus `20 C` opérations de combinaison.
Ils représentent **O(C) travail GPU, O(log C) profondeur idéale, O(C)
stockage temporaire**, pas quatre parcours lancés sur l’hôte. Une future
implémentation doit traiter les halos entre tuiles et les synchronisations.

La suite CPU peut garder le même tri exact des mêmes R éléments. Elle doit
écrire chaque chaîne depuis son segment compact à l’offset original `s` ;
elle ne peut conserver sans adaptation `order.data()+s` sur un tableau de
R éléments. Préserver les couples ID/clé, éviter une permutation écrite en
place écrasant ses sources, puis remonter les mêmes segments `[s,e)`.
Le coût du tri reste `Σ L_j log L_j` et la revérification reste O(C).

## Bornes et coût visé

`0 ≤ 2Q ≤ R ≤ C < 2^32−1`. Compteurs/offsets et produits d’octets restent
u64 vérifiés ; pas de troncature, atomic wrap ni publication avant un refus.
Le modèle contrôle les capacités d’éléments/chaînes et le budget du
**payload sérialisé** ; il ne certifie pas un budget global de workspace
CUDA inexistant. Les réservations exactes dépendront de son implantation.

En tableaux séparés sans padding par élément : `12R` octets pour IDs/clés,
`16Q` pour bornes u64, et `16` pour les deux comptes Q/R. Cela donne
**O(R)+O(1) téléchargement de sélection**, contre O(C), puisque Q≤R/2.
Ne pas envoyer un struct `{u32,u64}` sans compter son padding éventuel.
R peut valoir C : le pire cas reste linéaire et peut transférer davantage
que le chemin actuel ; ni rareté des chaînes ni réduction générale de
latence ne découlent du lemme.

| Session I, ng02 | C | Q | R | Initial actuel | Payload proposé |
|---|---:|---:|---:|---:|---:|
| K5 | 1 407 885 | 1 | 9 | 22 526 160 octets | 140 octets |
| K10 | 5 483 320 | 2 | 16 | 87 733 120 octets | 240 octets |

Ces comptes proviennent de la première passe de la prise d’identité ; le
calcul porte sur les **octets de sélection**, pas sur tous les transferts
du catalogue, un temps mesuré ou un gain sur l’étage complet. Les aller-retour
restants, allocations, scans et barrières doivent être mesurés avant adoption.

## Vérification bornée

10 449 cas valides confrontent le scan, une traduction littérale de
`collect_chains` et un oracle indépendant parcourant le graphe : exhaustif
jusqu’à neuf sommets sur les trois états d’arête autorisés, 600 cas aléatoires
reproductibles, sept chaînes couvrant les limites 31/32/33 et
1023/1024/1025/2049. Huit mutants structurels sont distingués ; 18 refus
explicites couvrent fautes, types, domaines, capacités et budget payload.
Ce sont des états abstraits de sélection, pas des catalogues géométriques
ni des essais de concurrence GPU. Les validations Python des préconditions
ne prescrivent aucun rapatriement des tableaux dans le chemin futur.

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/chaines_compactes/check.py --check
python3 -O -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/chaines_compactes/check.py --check
```

Contrelecture indépendante du lemme et des scans par l’auditeur mathématique
`perf_math` : favorable, sans exécution supplémentaire.

Avant implantation : garder l’émetteur `ChainKernel` et les comparateurs
exacts, réutiliser une primitive de scan qualifiée, compter mémoire et
transferts, vérifier les sorties et refus contre le chemin actuel, puis
mesurer l’étage complet dans une session gardée. La priorité de temps reste G.

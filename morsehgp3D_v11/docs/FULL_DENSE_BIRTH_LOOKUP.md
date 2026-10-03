# Table dense des naissances — option séparée

`FullParams.dense_birth_lookup=false` conserve la recherche binaire dans les
`BirthEntry`. L'option dense change seulement la représentation de l'application
partielle d'une clé vers son `NodeIdx`. Elle ne change ni la numérotation
canonique des naissances, ni les dates de descente, ni les plateaux et verticales.
Ce port et ses portes natives sont qualifiés dans
[reuse1](../receipts/full_regular_vertical_20261003/reuse1/README.md), source
`ae817d09e` :3339/3339 portes,299/299 ASan18 et29/29 FULL. Tous les tests
natifs ont été exécutés sur G4. Les chronos appariés511/1023 sont dans le reçu.

## Contrat et preuve

À l'ordre 1, la clé est un `SiteIdx` parmi n sites ; aux autres ordres, un
`BallIdx` parmi M boules du même catalogue. Une naissance possède une unique
clé et chaque clé correspond à au plus une naissance de l'ordre considéré.
Le tableau est initialisé à `kNone`, puis rempli après le tri canonique par
`dense[nodes[i].birth_key]=NodeIdx{i}`. L'unicité et la borne de clé sont
contrôlées avant l'écriture. Il représente donc exactement la même application
que la liste de `BirthEntry` triée par clé. Une case absente et une clé hors
borne rendent `nullopt` ; les gardes d'ordre et de variante site/boule restent.

Le `Buffer<NodeIdx>` est privé, possédé et immuable avant tout appel au Pool.
Les workers partagent uniquement ses lectures. Le déplacement de `OrderForest`
transfère le stockage ; l'ancien ordre est remis à zéro. Aucun pointeur vers
la table n'est publié. `dense_birth_lookup()` et `lookup_reserved_bytes()`
exposent seulement le mode et les réservations.

`BirthSeed` ne porte pas de certificat de propriétaire. Dans les tests de clés
invalides, un autre domaine produit des graines valides par `descend` ; leurs
entiers servent ensuite de clés synthétiques contre la table testée. Ce test
ne certifie aucune interprétation géométrique entre domaines. Il évite les
constructeurs privés et les graines artificiellement mal formées.

## Mémoire et travail

Pour toutes les forêts K1..K conservées simultanément, la table dense retient
`4[n+(K−1)M]` octets, contre `8Σb_k` pour les listes triées, b_k étant le nombre
de naissances de chaque ordre. Ces octets s'ajoutent aux nœuds, enfants,
verticales, domaine, mémos, espaces census, résultats antérieurs et temporaires
vivants dans le même `MemoryBudget`. Les admissions ne se limitent pas au
dernier ordre. Le refus conserve domaine, résultat antérieur et diagnostics.

À K1, le tri canonique XYZ conserve son scratch `BirthEntry` de 8n octets :
il coexiste avec le dense4n, puis est libéré avant le DSU. À K>1, aucun
`BirthEntry` n'est conservé dans la voie dense. Le scratch des cohortes de même
niveau reste celui du tri canonique ; cette option ne le supprime pas.

L'initialisation et le remplissage coûtent O(n+(K−1)M+Σb_k). Chaque résolution
de clé devient O(1), et les tris supplémentaires des listes par clé disparaissent.
Cela ne borne pas le nombre de descentes ni le coût géométrique de FULL.
Le coût de remplissage des cases absentes est réellement payé, même si M≫b_k.

Les compteurs archivés de [combined3](../receipts/catalogue_single_full_20261003/combined3/README.md),
source `90dd48bd284a960ed687328e1fe2468947039069`, donnent pour K5/mode127/u21 :

| Entrée | Recherches de naissance | Liste actuelle (octets) | Table dense (octets) | Surcoût retenu |
|---|---:|---:|---:|---:|
| ng00 | 4 480 149 | 7 182 208 | 21 066 676 | 13 884 468 |
| ng01 | 3 738 985 | 6 093 808 | 17 677 020 | 11 583 212 |
| ng02 | 4 786 862 | 7 861 360 | 22 709 540 | 14 848 180 |

Les recherches sont déduites de `Σ(traces+vertical_descents)` et des appels
inspectés ; les vérifications des enfants verticaux ne refont pas ce lookup.
Ces nombres ne mesurent pas son temps. Aucun gain, ni contrat FULL≤200 ms,
ne découle de la substitution avant une ablation native propre.

## Portes préparées

- `dense_lookup_test.cpp` : toutes les graines de naissance des fixtures,
  ordres canoniques XYZ/Morton distincts, permutations et profils hauts,
  trous K2 et clé hors borne, singleton, déplacement, FULL W1/W48 avec
  mémos et census possédé/emprunté, mémoire du préfixe et ALL K12 exacte/−1.
- `dense_lookup_fault.cpp` : 3 000 recherches sans appel allocateur,
  fautes après remplissage dense avant DSU/publication des timings,
  puis injection de chaque allocation FULL avec un ancien résultat conservé.
- `dense_lookup_model.py` : application partielle jugée depuis les naissances
  de Definition/Fraction, et corruption des clés, tailles, absences et mémoire.
  Ce modèle contrôle la stratégie de représentation ; il ne remplace pas les
  portes natives de géométrie FULL.

Le modèle Python passe en normal et `−O` : 42 cas, 138 ordres, 2 358 contrôles,
810 corruptions refusées, 504 cases absentes et 246 clés différentes de leur
ordinal canonique. La ligne12 vérifie séparément les 2 952 octets des douze
tables denses, contre 624 octets des listes triées. La configuration poison
G4 a exercé les cases absentes initialisées explicitement dans reuse1.

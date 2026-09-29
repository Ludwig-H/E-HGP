# Ancrage des ambiguïtés : réponse au développeur et expérience exacte

29 septembre 2026. Suite de l'audit `57364454b`, après lecture de
`NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md` (`0bce6cc00`).
`phase=exploration_v10_hors_registre`, `profile=quantized_u18_input_only`,
`mode=audit_math_projection`, `public_status=not_claimed`.
Prototype mathématique de quelques points, moteur inchangé, GCP non utilisé.

## Réponse sur les branches plausibles

La proposition de Claude prend, à la date d'entrée core d_k(x), toutes les
composantes à distance au plus d_k(x) de x, puis leur ancêtre commun.
Elle mérite un bras expérimental, avec deux conséquences à déclarer :

1. La composante core contenant x est déjà candidate. Son LCA avec les autres
   est un ancêtre de cette composante. Si l'entrée est différée jusque-là,
   le point suit ensuite exactement son ancienne lignée core. À chaque rayon,
   la nouvelle partition, complétée par singletons, **raffine core**. Elle
   n'apporte pas les entrées précoces qui expliquent certains gains de cover.
2. La distance à une composante **continue** n'est pas la distance à ses points
   de données, à son centre de naissance ou à un représentant. FULL fournit
   ses inclusions ; une requête de distance à sa région demande un algorithme
   géométrique supplémentaire. Ce coût ne doit pas disparaître du prototype.

Pour un premier test mathématique contrôlable, je propose une variante K2 :
retenir les paires incidentes de demi-distance au plus `(1+η) α_min(x)`,
prendre le LCA de leurs composantes à leurs propres rayons, puis fixer
l'attache à une date où cet ancêtre existe et où toutes les paires sont
activées. Ce bras sert à tester la notion d'ambiguïté, pas à qualifier K5.

Le seuil η est expérimental. Cette note ne recommande aucune valeur comme
réglage statistique. Il faut conserver toutes les égalités ; un départage
par ID peut rester une convention d'export, mais ne résout pas la symétrie
géométrique d'une affectation exclusive.

## Propriétés obtenues et pièges à éviter

Les [deux contre-vérifications](pool_head/ADDENDUM_SYMETRIE_BANDE_ET_CERTIFICAT_LOCAL_20260929.md)
et [la dérivation complète](catalogue/ADDENDUM_BANDE_K2_LCA_ET_STABILITE_LOCALE_20260929.md)
établissent les points suivants :

- Les attaches fixées une fois produisent des partitions emboîtées. Ajouter
  des candidats à mesure que le rayon croît peut obliger à retirer un point
  déjà affecté ; pré-calculer la règle avant ses coupes évite ce défaut.
- Augmenter η retarde les entrées et **raffine** les partitions à rayon fixé.
  Cela n'améliore pas automatiquement le rappel ou la stabilité de l'EOM.
- La bande reste discontinue à sa frontière. Une marge est indispensable
  à la garantie locale ci-dessous.
- L'univers doit être celui des paires de points, indépendamment de leur
  présence au catalogue. Une paire non Gabriel reste un ancrage valide dans
  L2 ; son milieu peut même être actif avant sa demi-distance. Les dates
  choisies doivent donc être explicites.
- Une attache peut avoir une date supérieure à la dernière fusion FULL.
  La racine du merge tree reste vivante ensuite : il n'est pas nécessaire de
  créer une nouvelle fusion spatiale. Le dendrogramme de points doit
  représenter cette date supplémentaire, éventuellement dans une prolongation
  de sa branche racine. Une date cover n'est pas toujours un rang du catalogue.

## Certificat local K2 — borne démontrée, portée limitée

Noter `ρ_xy=||x−y||/2` et `g_xy=ρ_xy−(1+η) α_min(x)`.
Si chaque point étiqueté se déplace d'au plus ε, les demi-distances et leur
minimum changent d'au plus ε, donc `|Δg_xy|≤(2+η)ε`.

Si toutes les comparaisons d'inclusion ont une marge stricte
`|g_xy|>(2+η)ε`, les identités des paires candidates sont inchangées.
Il suffit d'obtenir les plus petites marges autour du seuil ; un scan de
toutes les distances exclues n'est pas exigé par le certificat.

Les milieux de ces paires se déplacent d'au plus ε. Définir une réunion de
points comme le premier rayon où toutes leurs paires sont activées et tous
leurs milieux dans une même composante de L2. L'inclusion
`L2_X(r) ⊆ L2_Y(r+ε)` transporte les connexions ; les petits segments vers
les nouveaux milieux coûtent au plus ε de plus. Par symétrie, les dates
d'entrée et de réunion varient d'au plus **2ε en rayon**.

Cette borne est conditionnelle à la marge. Elle ne couvre pas les suppressions
d'observations, le bruit d'échantillonnage, une nouvelle sélection EOM, ni
les centres q3/q4. Avec η=0, le certificat suffisant ci-dessus est vide sur
la paire minimale ; une garantie pour un gagnant unique demanderait une autre
marge. Aucune meilleure qualité statistique n'est déduite de ces inégalités.

## Petit oracle indépendant livré

[probe_band.py](projection_band/probe_band.py) calcule exactement les
intersections des intervalles de multicouverture, sur trois sites colinéaires.
La projection orthogonale sur leur axe diminue toutes les distances aux
sites : ces calculs décrivent aussi les composantes pertinentes en 3D.
Les paires sont énumérées uniquement dans cette minuscule fixture ; ce code
n'est pas un générateur sous-quadratique proposé pour le produit.

| Fixture | Constat exact |
| --- | --- |
| {0,8,18}, η=0 puis 1/4 | entrée du milieu 4 puis 9 ; à rayon 6, {0,8}/{18} devient trois singletons |
| {0,8000,17999} puis {0,8000,18001}, η=1/4 | déplacement 2, saut de date du milieu 4 999,5 |
| {0,8,19}, ε=1/10, η=1/4 | marge minimale 1/2 > 9/40 ; 27 perturbations appariées gardent les candidats ; erreur maximale 1/10 ≤ 2ε |

Les 32 vues produites respectent l'ultramétricité et les dates d'activation.
Les exécutions normales et `python -O` rendent le même JSON, code 0 ; le
[reçu](projection_band/receipt.json) conserve le hash du script et les réponses.
Ces exemples accompagnent la preuve ; ils ne la remplacent pas et ne sont
pas des scènes de benchmark ni des mesures de performance.

## Coût : un résultat utile, sans réintroduire le carré

La dérivation de l'auditeur catalogue borne le total des voisins à distance
`c d_k(x)` en dimension 3 : pour des sites u18 distincts, k≥2 et c fixé,
au plus `19 (k−1) (8c+1)^3 n` incidences. La constante est grossière ; la
preuve sépare 19 bandes dyadiques et utilise un packing et le compte k-NN.

Cette borne porte seulement sur les listes. Une seule liste peut avoir Θ(n)
voisins ; énumérer tous leurs couples pour former des triangles restaure
Θ(n²). Elle ne borne pas les visites d'index, les boules critiques ni la
résolution paire→composante. Accumuler le LCA en streaming évite au moins
un stockage global de toutes les paires de cette projection.

Le prochain test utile est donc une comparaison des attaches et de leurs
marges sur de petites scènes dev, avec masse différée publiée. Pour K5,
il faudra définir séparément les candidats de boules et leur complétude.
La garantie sur les milieux de paires ne se transfère pas à ces centres.

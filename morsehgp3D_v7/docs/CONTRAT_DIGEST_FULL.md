# Comparaison sémantique des forêts FULL

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La [sonde v2](../bench/full_gabriel_lazy_probe.cpp) compare les politiques
eager et lazy avec le même instrument. Elle ne se raccorde ni à la CLI F
ni à une archive de tour. Son [sérialiseur](../bench/full_gabriel_semantic_digest.hpp)
engage plus que des nombres de nœuds : les labels des minima, les niveaux
rationnels et la topologie de chaque forêt horizontale. Une empreinte
égale reste une comparaison cryptographique, pas une preuve de complétude
des catalogues Gabriel ni un oracle géométrique.

## Objet engagé

L'entrée est triée par PointId externe ; chaque identité et ses trois
coordonnées u16 sont engagées. Une permutation physique conserve donc
l'empreinte, contrairement à un changement de coordonnées ou d'identité.
Ce tri est propre au digest et ne réordonne pas l'entrée produit.

Pour chaque ordre K, les racines puis les enfants sont ordonnés par leur
plus petit label de minimum descendant. Le parcours préfixe engage le
type du nœud, son arité, son niveau réduit et, aux feuilles, le label
complet. Les identifiants d'allocation, le padding C++ et l'ordre de l'arène
des minima ne sont pas sérialisés. La partition en feuilles distinctes
rend les clés de tri canoniques dans le domaine des certificats validés.

Le niveau possède un numérateur U192 et un dénominateur i128 strictement
positif. Leur PGCD est retiré avant sérialisation ; l'égalité des fractions
ne dépend donc pas de leur représentation historique. Les numérateurs
sont écrits en trois membres u64, les dénominateurs en u128 ; les autres
scalaires sont u64. Tous sont little-endian, avec domaines et chaînes
préfixés par leur longueur. Le zéro est normalisé en 0/1.

Chaque empreinte d'ordre engage aussi K et le digest de l'entrée. Le
digest global engage l'entrée et la liste ordonnée des digests K1..Kmax
effectivement terminés. Il est absent si un ordre échoue ; les lignes
d'ordre antérieures demeurent diagnostiques et provisoires.

## Coût et domaine

Le sérialiseur reçoit uniquement le certificat opaque validé par la
factory. Sa vue de test n'est pas un parseur de forêts non fiables.
La sonde fixe quatre millions de nœuds et huit millions de références
parentales par ordre. Le scratch logique vaut 25 octets par nœud plus
8 par référence, soit au plus 164 millions d'octets sur cette ABI. Ce
n'est pas une borne de capacités d'allocation ou de RSS. Le tri d'entrée
ajoute un indice `size_t` par point.

Tous les calculs du digest sont inclus dans `elapsed_before_terminal_ms`
et isolés dans `stage_ms.digest`. Aucun ratio avec l'ancienne sonde v1
sans digest n'est une comparaison appariée admissible. Les forêts sont
lues puis détruites ordre par ordre : le processus ne conserve pas les
K certificats ensemble, leurs liens verticaux ou un supplément de masses.

## Portes distinctes

La [porte C++](../tests/full_gabriel_digest_gate.cpp) compare indépendamment
les divisions U192/u128 et les fractions réduites à des entiers Boost
non bornés : 160 combinaisons de bornes et 512 vecteurs déterministes.
Elle teste aussi six rejets et les permutations/mutations de la
sérialisation. Boost reste dans le juge, pas dans le chemin mesuré.

Le [juge de reçus v2](../bench/full_gabriel_lazy_probe_audit.py) ne lance
aucun moteur. Il vérifie le protocole, les caps, les sorties terminales,
les politiques et identités de travail, la cohérence des temps et le
lien du digest global aux digests d'ordres. Ses mutations ciblent aussi
les quatre lacunes documentées par l'[audit de la sonde historique](../audits/MONO_FULL_COURANT.md).
Sans les octets des forêts, ce juge ne peut pas recalculer leurs empreintes
individuelles ni leur vérité géométrique. Les sources avant/après, le
binaire et le sceau du paquet sont des contrôles supplémentaires distincts.

Le juge v2 capturé contrôle la somme des insertions et skips mais pas
leur répartition exacte avant saturation. L'
[audit indépendant](../audits/receipts_full_lazy_20260905/digest_probe_review.md)
fournit le mutant C1/P1/I0/S1. Le [supplément first-C](../bench/full_gabriel_cache_policy_audit.py) vérifie donc, sur
chaque ordre lazy réussi, `inserts=min(C,portails)` ; il s'ajoute au
juge v2 et n'en réécrit pas les captures ni les 19 mutations historiques.
Sa [qualification séparée](../receipts/full_gabriel_first_c_qualification_20260905/README.md)
passe 58 commandes, avec 12 mutants et les refus argv normalement et
sous `-O`. Les refus du moteur ne reçoivent que des bornes de préfixe.

Les résultats exécutés doivent être attribués à leurs reçus frais. Cette
spécification n'annonce à elle seule aucune réussite de micro-admission,
de campagne mono, de contrat 50k ou de palier G4.

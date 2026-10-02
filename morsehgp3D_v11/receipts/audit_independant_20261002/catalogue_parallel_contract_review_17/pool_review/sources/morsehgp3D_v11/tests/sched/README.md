# Pool synchrone v11

Port explicite de R2 `865f5e64ddd08bedf6ab8f94e8bb94812e380e79` ;
les fichiers repris et leurs empreintes sont dans
[`source_pins.json`](../../src/sched/source_pins.json).
Aucune qualification native héritée. Ce dossier et le produit ne sont pas
compilés localement ; la qualification est attendue sur G4.

Le Pool possède W−1 threads persistants, W dans 1..256 et appelant compris.
La factory possède entièrement le Pool ; un échec de construction joint
tous les threads déjà créés. L’état borné par W est permis hors Buffer par
ARCHITECTURE §7.1. Les piles système ne sont pas mesurées par MemoryBudget.
Le Pool doit survivre à ses appels ; sa destruction concurrente avec un appel
ou depuis un callback ne fait pas partie du contrat de durée de vie C++.

`parallel_for(n, grain, context, body)` est synchrone. Grain nul et callback
nul sont refusés, y compris n=0. La garde membre refuse sans attendre toute
réentrance ou concurrence, avant de lire les autres paramètres. Aucun TLS ni
état global modifiable. Le callback/context est emprunté jusqu’au retour,
sans allocation de `std::function` ni copie de données.

Pour b<n, e=b+min(grain,n−b) vérifie b<e≤n : le CAS ne peut déborder,
même pour n=UINT64_MAX. Un CAS échoué actualise b et réessaie. Chaque CAS
réussi donne une tranche distincte ; le compteur finit à n. Le juge des
frontières utilise des additions u128, avec grain maximal et n aux limites.

L’époque booléenne bascule sous mutex. Chaque worker conserve son époque vue,
capture le Job sous le même mutex, écrit sa case d’Outcome puis acquitte.
Le pilote attend **tous** les W−1 acquittements, même si certains workers n’ont
réclamé aucune tranche. Tous ont donc vu la même époque avant la bascule
suivante : aucun compteur de génération, aucune ambiguïté après deux appels.
Ce mutex établit aussi la visibilité des cases Outcome avant réduction.

Toutes les tranches sont appelées, même après un refus ou une exception.
Chaque exception est convertie sur place : `bad_alloc` en `memory_budget`,
les autres en `task_exception`. Une tranche interrompue n’est pas rejouée.
La fusion `merge` est associative, commutative et idempotente ; elle dépend
des issues des tranches, pas de leur affectation aux workers. Cela ne rend
pas déterministe un callback lui-même dépendant de l’ordonnancement.
Le propriétaire des sorties reste responsable de leur publication atomique.

Les portes natives couvrent les frontières et leur multiplicité, les travaux
courts successifs, les refus vides/réentrants/concurrents, toutes les issues,
le retour après jointure, la participation des workers après exceptions,
les échecs de chaque création/allocation et la réutilisation du Pool.
L’injection pthread/new est exclusivement dans le harnais ; ses témoins
exigent que les vrais appels soient observés sous instrumentation.
Huit mutants visent des réponses erronées ou une couverture incomplète,
sans prendre un défaut de compilation ou un délai comme résultat attendu.

Le Pool ne porte ni tri parallèle ni catalogue parallèle. Aucun gain de temps,
résultat FULL ou contrat LiDAR ne découle de ces sources seules.

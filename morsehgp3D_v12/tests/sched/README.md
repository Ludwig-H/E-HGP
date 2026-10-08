# Pool synchrone

> **Port de la v11** (`ac081a06f`) dans le socle de la v12. L'historique ci-dessous est celui de la v11 : ses
> commits, reçus et qualifications ne qualifient pas la v12, qui rejoue ces portes sous les noms `mhgp12_*`
> (aucune qualification héritée) ; les liens mènent aux reçus et documents de la v11. Profils de la v12 : 21
> (défaut) et 24 ; le profil 18 de la v11 est abandonné (décision D6).

Port explicite de R2 `865f5e64ddd08bedf6ab8f94e8bb94812e380e79` ;
les fichiers repris et leurs empreintes sont dans
[`source_pins.json`](../../src/sched/source_pins.json).
Aucune qualification native héritée. Ce dossier et le produit ne sont pas
compilés localement ; la qualification est attendue sur G4.

Le Pool possède W−1 threads persistants, W dans 1..256 et appelant compris.
La factory possède entièrement le Pool ; un échec de construction joint
tous les threads déjà créés. L’état borné par W est permis hors Buffer par
ARCHITECTURE.md de la v11, §7.1. Les piles système ne sont pas mesurées par MemoryBudget.
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

**Équipe dimensionnée (8 octobre 2026, changement de la v12).** L’époque booléenne sous mutex, qui réveillait et
attendait **tous** les W−1 ouvriers à chaque appel, est remplacée. Pour c = ⌈n/grain⌉ tranches, seuls les
min(W−1, c−1) premiers ouvriers sont engagés, chacun réveillé par son propre sémaphore binaire. Une seule tranche
s’exécute dans l’appelant, sans réveil. Chaque ouvrier engagé écrit sa case d’Outcome puis décrémente un compteur
atomique (acq_rel) ; le dernier libère le sémaphore de l’appelant. Cette chaîne établit la visibilité des cases
Outcome avant la réduction, et le Job reste vivant jusqu’à cet acquittement. Un ouvrier ne reçoit un jeton que
lorsqu’il attend, puisque l’appel précédent l’a attendu : aucun jeton ne déborde le sémaphore binaire.

Motif : à 48 fils, le réveil de tous sous un même mutex dominait les petits nuages
([session C](../../receipts/g4_mesc_20261008/README.md) :
15,6 ms vers 150 sites à 48 fils contre 6,8 ms à 4). En local, à 8 fils, un nuage réel de 102 sites passe de 16,1 à
13,6 ms par passe. La porte `team` vérifie les deux régimes à 1, 2, 8 et 48 fils : une tranche dans l’appelant, et
des ouvriers engagés dans 0..min(W−1, c−1) pour c tranches. TSan est muet sur la porte unitaire, rejouée trois fois.

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

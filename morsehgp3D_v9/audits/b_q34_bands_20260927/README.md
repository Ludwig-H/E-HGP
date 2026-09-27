# Bandes q3/q4 : représentation compacte, pas nouveau filtre

27 septembre 2026. `phase=exploration_v9_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u18_input_only`,
`mode=audit_q34_residual_representation`, `public_status=not_claimed`.
GCP non utilisé ; aucun fichier du moteur modifié.

Ce dossier reconstruit et éprouve la proposition du
[plan publié le 26 septembre](../b_q34_factor_plan_20260926/NEXT.md).
Le brouillon non commis du 26 septembre n'était plus accessible à la reprise :
aucun binaire ni aucune qualification de ce brouillon n'est repris.
`plan.hpp` et les recettes/chargeurs de la sonde **publiée** à `ddf4776d7`
sont réutilisés explicitement et épinglés, avec les archives générateur
Release et instrumentée existantes en lecture seule. L'ancien `main` est
renommé et n'est jamais appelé. Cela n'est pas un oracle géométrique
indépendant : le nouvel objet est comparé au même Pool exact déjà publié.

## Objet et preuve

Les facteurs gardent leurs crédits par rang original et leur permutation
groupée lexicographiquement par `(c3,c4)`. Pour une classe A, les seuils
résiduels sont `r3=K−1−a3` et `r4=K−2−a4`, seulement si la voie existe et
est active. K1 désactive les deux voies, K2 désactive q4 **avant** toute
soustraction non signée.

Le survivant d'une paire est l'union `b3<r3 OU b4<r4`. Le préfixe des lignes
`b3<r3` est une seule plage contiguë de B. Dans chaque ligne restante,
seul son préfixe `b4<r4` est gardé. Ces deux familles de plages sont
disjointes et chacune commence/termine à une frontière de classe B ; une
paire appartenant aux deux voies n'est donc émise qu'une fois. Une classe A
produit au plus K+1 bandes, certaines vides n'étant pas stockées.

Le descripteur fait **12 octets** : index de classe A, début B et fin B,
tous u32 locaux après validation du domaine. Les anciens blocs font
40 octets sur les builds examinés. Le masque q3/q4 est recalculé exactement
à partir des deux crédits lors du décodage ; il n'est **pas** remplacé
par 6 dans la première bande. Une intégration GPU collective aurait besoin
de ses propres offsets u64 et de validations distinctes : elle n'existe pas
dans ce prototype.

Un histogramme 2D et ses préfixes calculent chaque masse d'union en temps
constant par classe A, par inclusion-exclusion. Le code utilise des tables
fixes pour Kmax=10 : 341 cases initialisées et 100 étapes de préfixe par
rectangle préparé, même à petit K. Le coût de construction est
`O(F + Kmax² + C_A Kmax)` et sa validation parcourt réellement tous les
rangs. Il ne parcourt pas toutes les cellules `C_A C_B` pour construire les
bandes. **La vérification différentielle de la sonde, distincte et
chronométrée, parcourt encore ces cellules.** Le coût géométrique du Pool,
le front et le résidu E ne sont pas réduits par ce changement de stockage.

## Propriété, ordre et mémoire

Les facteurs sont **empruntés** : ils doivent rester vivants et immuables
pendant construction et consommation des bandes. Les bandes ne constituent
pas un certificat autonome ni une factory d'immuabilité. Aucune nouvelle
copie des rangs/crédits n'est faite ; les vecteurs originaux restent payés.
La validation conserve temporairement un octet par rang du facteur le plus
grand. La pile des histogrammes représente 1 892 octets déclarés, hors
autres variables automatiques.

Le prototype conserve un objet et un vecteur par rectangle ; l'arène
collective proposée n'est pas implémentée. Les sommes de capacités de tous
les plans séquentiellement détruits ne sont ni une RSS ni une VRAM de pic.
Réduire les seuls descripteurs ne supprime pas les facteurs, les métadonnées
ni les buffers de tri/scatter.

L'ordre groupé des paires diffère toujours de l'ordre natif du batch.
Restaurer l'ordre original ou requalifier explicitement les consommateurs,
sidecars inclus, reste indispensable avant un raccord GPU.

## Portes et capture

`probe.cpp --gate` compare une référence exhaustive indépendante du codage
par bandes : K1..10, masques 0/2/4/6, toutes les classes de crédits possibles,
populations vides, multiples, creuses et permutations. Il ajoute 96 fronts
WSPD natifs (quatre familles, deux ordres d'entrée, K2/3/5/10, s8/10/12),
puis huit refus d'entrées incohérentes. Les mutations ciblent un recouvrement
de bandes et le masque 6 abusif. La mesure native compare toutes les classes
de chaque plan, sans développer les millions de paires.

La capture autoritaire et les résultats sont dans le
[reçu r2](../../receipts/q34_bands_20260927/README.md).
La première tentative a conservé son arrêt LeakSanitizer sous ptrace ;
elle ne qualifie pas le binaire instrumenté. Le rejeu r2 est exécuté hors
sandbox, dans de **nouveaux** builds, sans désactiver LeakSanitizer.

Toutes les mesures portent explicitement le champ
`representation_only_old_cells_still_paid` : les anciens plans/cellules
sont encore construits pour la comparaison. Le temps des bandes est un
**surcoût mesuré**, pas un gain net acquis, un chrono S2, FULL ou G4.

Reproduction (builds et captures neufs obligatoires) :

```bash
python3 -B morsehgp3D_v9/audits/b_q34_bands_20260927/run.py --capture /nouveau/recu --build-prefix /nouveau/build
python3 -B morsehgp3D_v9/audits/b_q34_bands_20260927/run.py --readback /nouveau/recu
python3 -B -O morsehgp3D_v9/audits/b_q34_bands_20260927/run.py --readback /nouveau/recu
```

Les lecteurs sont LIVE : ils exigent les sources, builds, archives générateur
et entrées hors Git aux emplacements épinglés. Les octets KITTI ne sont ni
copiés ni versionnés dans ce dossier. Aucune borne globale sous-quadratique
ni qualification du contrat 100 ms ne découle de ces portes.

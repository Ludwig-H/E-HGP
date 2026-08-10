# Audit de la requalification `tri-owner` à `37139de`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=gpu_bounded_oracle`,
`profile=quantized_u16_input_only`, `mode=audit_independant`,
`public_status=not_claimed`.

## Snapshot et verdict

Le snapshot reçu est `HEAD=origin/main=37139de2329c32797815db3fa73130a2e80aeda3`.
Le code source est celui du commit `a6e3078`; `37139de` ajoute uniquement le
compte rendu de la seconde session G4.

- `CMakeLists.txt` : SHA-256 `662337fa934c2fa1be9349e36006a7a28da4e5fb6ab2585e40dca2f2a220c97f`;
- `faceowner_device_kernel.cu` : SHA-256 `814a6a4b86d3ccbd1ce003112c057cabdf01f4e481fbe64ce73a941c50dd6b1a`;
- API device : SHA-256 `909394ff8a990a749e7a8574c3a13451c94121456ed06aaf44af2981b9e4ba33`;
- qualification device : SHA-256 `03eabfb4233ab5da94b881fdddb58f0cf7625b084112269683a1f2ff90058527`.

Verdict : **GO comme cinquième oracle borné sur les entrées produites par le
harness; NO-GO comme API hostile, admission mémoire ou backend hybride
produit.** Le tri unique est une simplification mathématiquement correcte et
requalifiée. La validation d'entrée annoncée reste contournable par une entrée
qui fait boucler le kernel.

## Résultats positifs

Le tri par `(signature, activation_rank, GeneratorId)` place bien à la tête de
chaque groupe le minimum lexicographique utilisé par l'oracle CPU. Lire cette
tête est donc équivalent au précédent `reduce_by_key(min)`; le tie-break par
générateur reste conservé. La suppression de `candidates`, `group_keys_out` et
`group_owner` réduit effectivement le pic explicite.

Le CTest absent est désormais conditionnel à `MHGP3V_ENABLE_CUDA=OFF`. Sous
CUDA, CMake déclare un nominal et le mutant `drop-edge`. Le faux rouge décrit
par l'audit précédent est fermé.

Un build CPU Release frais, CUDA désactivé, a été exécuté hors dépôt :

```text
cmake -S morsehgp3D_v3 -B /tmp/mhgp3v_audit_a6e3078_cpu -DMHGP3V_ENABLE_CUDA=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/mhgp3v_audit_a6e3078_cpu -j2 --target mhgp3v_faceowner_device_qualification mhgp3v_postings_join_gate mhgp3v_saturated_pipeline
ctest --test-dir /tmp/mhgp3v_audit_a6e3078_cpu --output-on-failure -j2 -R '(faceowner|hybrid|postings_gate_fixtures)'
```

Résultat : 17/17 tests passent. Le binaire sans CUDA refuse toujours par code
2 et `ctest -N -L cuda` liste zéro test, conformément au contrat conditionnel.

La seconde session G4 consignée par Claude reçoit le chemin nominal valide :
à `n=400,K=5`, les cinq ordres rendent exactement 44 258 951 incidences,
1 823 707 signatures et 19 073 174 arêtes, identiques au CPU arête par arête.
Le total des phases device annoncé est 46,40 ms; le mutant `drop-edge` sort 1.
Ce chrono exclut toujours allocations, H2D, D2H et rejeu.

La VM concurrente `europe-west4-ai1a/ehgp-blackwell-spot-ai1a` a été observée
en lecture seule de `RUNNING` à `STOPPING`, puis `TERMINATED`, génération
17:00:56--17:04:21 UTC. L'auditeur ne l'a ni démarrée ni arrêtée.

## P0 : les offsets acceptés peuvent faire boucler l'unranking

La garde des lignes 176--201 vérifie seulement la monotonie des offsets. Elle
n'exige ni origine nulle, ni masse binomiale exacte par générateur. La fixture
hostile minimale suivante passe ces contrôles :

```text
G=1, k=2
members_csr=[0]
member_offsets=[0]
ranks=[1]
incidence_offsets=[0,1]
activation_rank=[0]
batch_of=[0]
```

Pour la tâche artificielle `t=0`, `rank=1<k`; `with_this` vaut toujours zéro.
La boucle des lignes 91--99 incrémente alors `position` sans jamais diminuer
`remaining`, puis finit hors limites ou ne termine pas. Un préfixe supérieur à
`C(rank,k)` produit le même défaut.

Correction fail-closed avant tout lancement :

1. refuser les pointeurs de sortie/erreur invalides et les tailles au-delà de
   `INT_MAX`, `size_t` et des limites CUDA;
2. exiger `member_offsets[0]=0` et `incidence_offsets[0]=0`;
3. recalculer en entier vérifié `expected_g=C(rank_g,k)` si `rank_g>=k`, zéro
   sinon, puis exiger chaque différence d'offset exactement égale;
4. exiger le dernier offset égal à la somme vérifiée et compatible avec la
   grille;
5. conserver dans le kernel une garde défensive `position<rank`, qui signale
   une faute au lieu de lire hors limites.

La porte permanente doit injecter la fixture ci-dessus, un premier offset
négatif/non nul, un dernier préfixe omis et un préfixe excédentaire. Le nominal
G4 ne reçoit aucune de ces branches.

## Validation encore incomplète

L'API accepte également :

- un préfixe inutilisé avant `member_offsets[0]`;
- des membres non strictement triés ou dupliqués, qui donnent des clés
  différentes au même ensemble;
- un `activation_rank` non permutationnel;
- des lots incompatibles avec l'ordre d'activation;
- un `result` nul ou un tampon d'erreur nul/trop petit, déréférencés avant
  validation.

Exiger membres strictement croissants, activation permutation de `[0,G)`, lots
non décroissants dans cet ordre et cohérence lot/activation. Ces propriétés
doivent être testées dans le harnais device, pas seulement vraies par
construction dans le producteur nominal.

## Le manifeste VRAM reste une estimation sous-majorante

Le modèle publié conserve `56*I` octets par incidence. Au point où `kept` est
alloué, les buffers explicitement vivants valent au moins :

- `Incidence` : `24*I`;
- `head` et `group_of` : `16*I`;
- `raw_edges`, `keep` et `kept` : `28*I`;
- `group_start` : `8*S`.

Le plancher explicite est donc `68*I+8*S`, hors entrées, capacités, contexte et
workspaces Thrust. À `k=5,n=400`, il vaut 1 197 573 508 octets, soit 1 142,1
MiB, avant ces coûts. Le « pic modèle ~939 Mo » reste honnêtement un modèle,
mais il ne peut commander un budget dur ni le NO-GO à 70 %.

La correction minimale est de publier `estimate_only=true` et d'utiliser la
borne explicite majorée par phase. La correction contractuelle est une arène
plafonnée avec workspace interrogé et high-water. Le retour de `copy_if` qui
construit `group_start` doit en outre être comparé à `group_count`.

## Robustesse et prochaine porte G4

Les événements CUDA sont créés, enregistrés, synchronisés et détruits sans
vérifier tous les statuts; plusieurs retours précoces et l'exception Thrust
court-circuitent leur destruction. Utiliser RAII, vérifier chaque opération et
ne publier `FaceOwnerDeviceResult` qu'après succès complet.

Le prochain G4 utile n'est pas un nouvel escalier `n=400`. Il doit exécuter :

1. compilation CUDA avec includes Thrust explicites;
2. nominal et `drop-edge` CTest;
3. mutants d'offsets/rangs/membres/activation;
4. erreur injectée après le dernier chunk, résultat resté vide;
5. `compute-sanitizer` sur le nominal et chaque rejet qui atteint le device;
6. seulement après cela, comparaison du mur total incluant H2D, D2H et rejeu.

Le kernel reste l'oracle `face-owner` exhaustif. Il n'implémente ni
`query_mask`, ni owner demand-driven, ni compteur/cover, ni replay DSU device.


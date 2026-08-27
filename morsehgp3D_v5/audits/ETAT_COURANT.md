# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex
- **Pin fonctionnel audité :** `d96b2a67d89a6c2b36791ea59b4ac636d436ec61`
- **Worktree fonctionnel observé :** chantier CUDA Q3 en cours, non commité et exclu du verdict sur le pin
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; la dernière cible Spot de Claude observée en lecture seule, `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, a été certifiée `TERMINATED`

## Verdict

Le pin est **orange, sans P0 CPU reproduit**. Les défauts qui provoquaient des
écritures hors limites, un décalage indéfini ou `std::terminate` sont corrigés
et leurs portes passent. La livraison horizontale CPU reste correctement
bornée à `mhgp5-forests-horizontal-v1` ; elle ne revendique ni tour, ni CUDA,
ni statut public exact.

Il reste quatre frontières P1 à fermer avant une nouvelle campagne GPU ou
l'intégration de la lane Q3 device. Elles sont assez localisées pour être
corrigées dans le cycle courant ; aucun nouvel audit séparé n'est nécessaire.

## Fermetures requalifiées

Sur `338a9ef8`, parent fonctionnel du pin courant :

- `ForestEvent` hors profil est refusé avant `facet_minus` ; les limites
  `q11+d0` et `q2+d9` passent, et le probe `q11+d1` est devenu une fixture ;
- `shell_cap` est borné à `[4,12]` dans l'API ; `13`, `32` et `SIZE_MAX` sont
  refusés sans callback ;
- le callback qui lève est joint puis son exception est propagée au fil
  appelant ;
- le temporaire du fold conserve séparément `{partition,tid}` ;
- l'oracle arithmétique du témoin ne déborde plus en signé, et les compteurs
  sont comparés pour toutes les seeds, mortes comprises ;
- la métrique `intern` ne double plus `merge`.

Le pin `d96b2a67` ajoute `rle_workers` et `fold_workers` à `par_gate` et retire
les faux sites de mutants présents dans les commentaires `//`. Ces changements
de porte ne modifient pas l'objet fonctionnel.

## P1 — frontières encore ouvertes

### 1. Le fold n'a pas encore un refus structurel entièrement fail-closed

[`validate_fold_events`](../src/forest/fold.hpp#L243) appelle
`parallel_ranges` avant la garde de capacité. Avec plusieurs fils, cette étape
alloue un vecteur de threads et crée des ouvriers : les commentaires et la
réponse de fermeture « avant toute allocation » sont donc trop forts. La
correction la plus simple est un balayage séquentiel, borné et sans allocation,
avant toute phase parallèle. Si le coût de ce balayage est jugé excessif,
borner explicitement `threads` et réduire la revendication à « avant toute
allocation proportionnelle au fold » ; ne pas conserver la formulation
absolue actuelle.

Le même balayage doit vérifier le contrat déjà écrit dans
[`ExactLevel`](../src/lanes/level.hpp#L36) : `den > 0`. Le constructeur agrégé
par défaut donne actuellement `den == 0`, et la porte positive du fold utilise
précisément ce niveau invalide. `build_forest` l'accepte puis les comparaisons
le convertissent en `u128`. Ajouter les rejets `den == 0` et `den < 0`, puis
donner un niveau positif aux fixtures acceptées.

### 2. La frontière `Q3Batch` doit refuser les buffers incohérents

[`scan_q3_batch_host`](../src/gpu/q3_lane_batched.hpp#L117) indexe directement
`anchors[s.anchor]` et les huit tableaux de sites ;
[`emit_q3_batch`](../src/gpu/q3_lane_batched.hpp#L133) suppose sans contrôle un
verdict et un candidat par seed. Une réponse device tronquée ou un lot corrompu
devient donc un accès hors limites au lieu d'un refus. Le lot minimal « une
seed, aucune ancre » provoque effectivement un `SEGV` sous ASan/UBSan à la
ligne 123, au lieu d'un statut contractuel.

Avant que le chantier device observé dans le worktree ne soit commité, ajouter
une validation commune hôte/device : huit tailles SoA égales, tranches
`begin/count` sans débordement et incluses, `seed.anchor < anchors.size()`, un
candidat et un verdict par seed, et conversions `size_t -> u32` contrôlées.
Une erreur doit produire un statut/refus et zéro émission. Les fixtures doivent
couvrir verdict tronqué, ancre invalide, tranche débordante et taille supérieure
à `UINT32_MAX` sans chercher à l'allouer réellement.

**Blocage du worktree device :** `Q3DeviceExecutor::scan` lève sur toute erreur
CUDA, mais à plusieurs fils cette exception traverse le corps de
`parallel_items`. La bibliothèque standard appelle alors `std::terminate` ; le
`catch` de `q3_lane_device_gate` n'est jamais atteint. Un probe remplaçant le
scan device par une lambda qui lève, avec `threads=4`, reproduit un abort
**code 134**. Capturer la première `exception_ptr` dans les ouvriers, joindre
tous les fils puis la relancer dans le fil appelant ; ajouter une fixture à
quatre fils qui exige le code de refus prévu, sans signal.

Dans l'exécuteur device provisoire, ne mettre à jour `cap_sites_`, `cap_jobs_`
et `cap_anchors_` qu'après **toutes** les allocations correspondantes réussies.
Le code observé avance le plafond avant les `cudaMalloc` : un échec au milieu
laisse un exécuteur réutilisable qui croit ses tampons complets. Préférer une
croissance transactionnelle ou marquer définitivement l'instance en échec.
Un simple `sizeof` égal ne suffit pas non plus à autoriser les
`reinterpret_cast` entre structures hôte/device ; convertir explicitement ou
vérifier type standard, alignement et offsets. Contrôler enfin les retours des
événements CUDA et des libérations auxquels le contrat « toute erreur est un
refus » s'applique.

La porte multithread trie puis déduplique avant comparaison. C'est suffisant
pour le payload produit, dont l'autorité est post-RLE, mais cela ne prouve pas
un « multiensemble post-RLE » ni les multiplicités brutes : corriger ce
vocabulaire, ou ajouter une comparaison du multiensemble trié comme diagnostic
de coût, sans en faire à tort une nouvelle sémantique. En revanche, aligner le
contrat d'appel est nécessaire : la production vide `out`, tandis que la lane
par lots ajoute actuellement à `out` et mélange champs écrasés et compteurs
cumulés dans `st`.

### 3. Le fail-fast GPU ne contrôle que le code de sortie

[`v5_campaign_remote.sh`](../../gcp-migration/v5_campaign_remote.sh#L80)
poursuit si `gpu_witness.status` contient `code=0`, même si la sortie contient
`desaccords=1`. Le scénario `3ter` du selftest confirme que les douze
conformités et quatre contrats sont alors exécutés avant le rejet local final.

Réutiliser un validateur sémantique dédié immédiatement après `run_one`, puis
exiger dans le selftest `rc=3` et un seul statut pour ce scénario. La campagne
doit aussi exécuter le mutant `witness-no-warp-correction` et recevoir son code
4 ; l'enregistrer dans CTest sans le lancer sur la VM ne prouve pas la porte
device.

Le validateur final gagnerait à exiger une occurrence unique des lots attendus,
à refuser tout `desaccords` non nul, à imposer un plancher de seeds mortes et à
vérifier les lignes de provenance `nvcc`, pilote et `device/sm`.

### 4. Les preuves fraîches manquent encore

Le témoin CUDA n'a pas été compilé par `nvcc` sur ce pin. Deux reçus G4 sont
présents : la campagne CPU à `f37669ae` est complète sur ses 16 runs, avant le
témoin et le fold courant ; celle à `9762daaf` est `partial_or_failed` avec
`gpu_witness code=2`. Elles conservent des mesures CPU utiles, mais aucun
résultat GPU.

Le reçu local `campagne_v5_d08913ac7936_20260827.txt` établit 12/12 conformités
à 8 k/16 k/32 k pour le fold et le pipeline de ce pin antérieur. Il ne couvre
ni `338a9ef8`, ni `d96b2a67`, ni le chantier device courant. Attendre le reçu
apparié propre en cours avant d'étendre la conformité à ces pins.

La documentation canonique n'est pas encore entièrement fraîche :
[`MATHEMATIQUES.md` § 8](../docs/MATHEMATIQUES.md#8-le-juge-indépendant) dit
toujours que l'oracle v5 est « à écrire », que le digest v4 est la seule
autorité et que la réalisation n'est pas livrée. Cela contredit
`mhgp5_forest_judge` et les oracles q3/q4 enregistrés et rejoués. Décrire
exactement ce qui est livré et ce qui manque encore, sans transformer ces
portes de falsification en claim d'exactitude. Dans `ARCHITECTURE.md` § 7.1,
les deux chemins vers `audits/` doivent aussi être relatifs à `docs/`, donc
commencer par `../audits/`.

## Rejeu indépendant

Dans une construction Release propre du pin `338a9ef8` :

```text
cmake -S morsehgp3D_v5 -B /tmp/mhgp5-audit-338a9ef8.4uJAYw0W -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/mhgp5-audit-338a9ef8.4uJAYw0W --parallel 4
ctest --test-dir /tmp/mhgp5-audit-338a9ef8.4uJAYw0W --output-on-failure -j 4 -LE 'scale8000|scale16000|scale32000'
```

Résultat : **131/131 tests passés**, dont les oracles, mutants, gardes d'API,
contrat du fold, lane Q3 hôte, conformité rapide et `two_lines n=2000`.
Après passage à `d96b2a67`, les quatre portes affectées (`mutants_gate`, les
deux `par_gate` nominaux et `parallel-one-worker`) ont été reconstruites et
rejouées : **4/4 passées**.
Sur le worktree provisoire, la généralisation `generate_q3_batched_with` a
également été reconstruite : les cinq portes CPU Q3 rapides passent **5/5**.
Cela ne compile ni ne valide l'exécuteur CUDA non commité.
Les deux portes structurelles ciblées avaient aussi passé sous ASan/UBSan dans
une construction séparée. Les douze CTests d'échelle et la porte Q3 à 8 k
n'ont pas été rejoués par cet audit ; ils exigent le reçu frais mentionné
ci-dessus.

Au worktree observé, `python tools/check_docs.py` valide 212 Markdown actifs,
`python tools/check_implementation_status.py` valide 20 phases, et les six
Markdown conservés dans `audits/` passent aussi explicitement la fonction
`validate()` (ils sont hors du périmètre automatique). `git diff --check` est
vert. Rejouer ces quatre contrôles après stabilisation du worktree CUDA et avant
le prochain commit d'audit.

## Ordre de travail conseillé à Claude

1. fermer la validation `ExactLevel` et `Q3Batch`, avec refus et mutants ciblés ;
2. rendre le fail-fast GPU sémantique et lancer nominal **et** mutant ;
3. commiter la lane device sur un pin propre, puis rejouer les portes CPU ;
4. seulement alors lancer une session G4 gardée et produire un reçu frais.

La définition des applications verticales et le rendu indépendant sont
consignés dans
[`REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md).
Le payload courant reste horizontal.

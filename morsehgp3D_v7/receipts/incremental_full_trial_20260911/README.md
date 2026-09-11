# Essai privé du journal incrémental raccordé à FULL — non intégré

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce paquet préserve le travail privé de `static_resolver_review` depuis `ce842a3f1c0d55786250b1deba85e7bd92c8807a`, sans modification de ses sources ou de ses captures. Le raccord passe ses comparaisons physiques et juges O2/SAN, mais **ne sera pas intégré tel quel** : les copies restantes et les surcapacités empêchent d'en faire une optimisation validée. Aucun moteur n'a été compilé ou exécuté pour ce packaging ; GCP non utilisé.

## Verdict de performance négatif

Le micro mono-thread, uniforme seed3, s8, K=1..10, mesure séparément les appels d'allocation, octets demandés, pic demandé, rétention finale et temps brut. Les champs et le digest physique du payload sont identiques entre les deux bras aux trois tailles. Les résultats restent ceux de la capture privée, sans nouvelle mesure :

| n | Appels new baseline → essai | Octets retenus finaux baseline → essai |
| --- | ---: | ---: |
| 200 | 871 992 → 937 378 | 10 327 116 → 13 560 236 |
| 400 | 2 190 325 → 2 352 340 | 25 476 444 → 32 494 364 |
| 800 | 4 983 612 → 5 349 633 | 57 757 460 → 75 097 940 |

À n800 : +366 021 appels new et +17 340 480 octets retenus. Le pic demandé ne baisse que de 955 584 octets ; à n400, il augmente. Le RSS de la série entière baisse mais inclut le census et les autres tailles : il ne mesure pas le pic FULL isolé à n800. Les temps bruts, sur hôte partagé et sans campagne répétée, ne constituent pas un speedup.

La suppression de la rétention globale des batches owning n'en supprime pas les allocations temporaires par action. Le propriétaire reçoit encore les populations par `const&`, donc en recopie les buffers ; la croissance amortie des arènes augmente leur capacité finale. Ce sont les limites de cette couture, pas une réfutation du principe du journal incrémental. Voir [LIMITES_ET_FAUSSE_PISTE.md](LIMITES_ET_FAUSSE_PISTE.md), conservé intégralement. Scratch plat, singleton emprunté ou transfert sans copie sont des propositions distinctes, non implémentées et non qualifiées ici.

## Preuves positives, strictement bornées

Le journal propriétaire reprend les headers déjà qualifiés `b526b895…` et `76885ecd…`. Le header FULL baseline est `33e7d05e…`, le candidat privé `a2c6ab390540fd57d2e593d6d78097f0cee477810e1e3b0dcd35fe4c3afbbd50`. Les populations sont créées dans le même ordre ; les lots sont appendus avant installation des ancres ; le scellement reste global. Les temporaires owning d'un lot subsistent. L'accounting de cette variante est explicitement privé et différent des réservations exactes nominales.

- `o2_r1` et `san_root_r2` : douze commandes chacun ; cache, sans cache, statique 1 et statique 4. Les exports `.physical`, stdout et stderr sont identiques octet pour octet entre baseline/candidat et entre O2/SAN dans chaque mode. Par bras et par build, les quatre modes totalisent 216 appels, 180 tours closes, 36 refus, 4 932 nœuds, 4 208 parents et 3 432 contributions comparés. Les répétitions ne sont pas des nuages indépendants supplémentaires.
- `failure_o2_r1`, puis `failure_o2_v2` et `failure_san_root_v2` : 1 402 allocations refusées individuellement, dont 1 300 après préfixe K2 non vide et 102 au scellement, sur les modes 0/1/4 ; trois corruptions tardives rejetées et 708 contrôles d'empoisonnement. Aucune sortie publique partielle.
- `san_r1` : échec LSan/ptrace conservé, sans désactivation des sanitizers. Le replay ROOT complet est un reçu séparé.
- `failure_san_root_r2` : échec `alloc-dealloc-mismatch` du harnais v1, dont l'interposition de `new(nothrow)` était incomplète. Le wrapper additif v2 corrige le harnais sans changer le candidat ; O2 et SAN v2 passent. L'échec v1 n'est pas requalifié en succès.
- `micro_o2_r1` : quatre commandes closes, n200/400/800 et verdict négatif ci-dessus.

Le corpus indépendant des préfixes de l'auditeur est conservé comme référence, sans réattribuer ses anciens contrôles au raccord. Le [rapport privé intégral](PROTOTYPE.md) détaille les corpus, les garanties et leurs limites. Aucun n8k+, contrat 50k/1 seconde ou 100 ms, dizaines de millions de points ou GPU n'est qualifié par ce paquet. Le coût amorti du journal dépend de la taille de sa sortie et n'est pas une borne universelle sous-quadratique en n.

## Lecture portable et fidélité de l'archive

Depuis la racine :

```bash
python3 -B morsehgp3D_v7/receipts/incremental_full_trial_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/incremental_full_trial_20260911/verify.py
```

Le lecteur contrôle le manifeste public et le manifeste privé inchangé, reconstruit temporairement tous ses chemins logiques, puis appelle le lecteur privé normal ou `-O`. Il vérifie aussi que le résultat négatif du micro reste explicite. Aucun ELF ou calcul géométrique n'est exécuté ; aucune API cloud n'est appelée.

`capture_manifest.json` est le `MANIFEST.json` privé original, SHA256 `fb0f7d3b4daddc650ee1a0ad85f749f22fd1c2e01e557650e17b9a79e02897f1`. `storage_map.json` conserve les chemins et hashes de toutes ses entrées. Les duplications de sources et d'exports physiques sont stockées une seule fois dans `objects/` ou dans les exemplaires lisibles : aucun fichier logique n'est omis. Les sources principales sont dans `sources/`, les scripts dans `scripts/`, et les différences dans [delta.patch](delta.patch). Les reçus et logs sont sous `captures/` ; les exports physiques de référence sont sous `physical/`. Les pins des onze ELF privés sont conservés dans `binaries.json`, sans distribuer ces ELF ni aucun vendor. Les dépendances externes nécessaires à une recompilation restent déclarées par les commandes et fichiers `.d` ; le paquet ne fournit pas un toolchain hermétique.

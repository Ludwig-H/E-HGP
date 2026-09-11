# Comptage du lot : débordement reproduit et correction privée vérifiée

11 septembre 2026, reprise sur b8336ad3. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. GCP non utilisé ; aucun fichier constructeur modifié.

**Un débordement d'agrégation laissait un total partiel déclaré connu dans l'adaptateur privé. La correction du constructeur passe notre même fixture indépendante en O2 et ASan/UBSan.** Ce défaut ne publiait pas de cible partielle. Il ne concerne pas le moteur actif `6763a877…`, qui n'intègre pas cet adaptateur.

## Constat et conséquence

Source privée lue : `build/v7_terminal_batch_20260911/prototype/batch_adapter.hpp`, SHA `143bcbfe489ca2950b5ff48be64025dab1dbd5cbf959c4fc0053bb9bbf370bb8`. Après le retour du contexte, l'adaptateur copiait `raw_work_known` dans `output.work_known`, puis additionnait les compteurs des lignes directement dans `output.work`.

Si une addition contrôlée levait `full_ball_counter_overflow`, le callback sortait avec `work_known=true`, mais `work` n'était que le préfixe calculé avant l'exception. La couture privée `prepare_external_batch::account()` fusionnait ce préfixe parce qu'il était déclaré connu. Les cibles restaient vides ; la faute porte sur la qualification des compteurs après refus, pas sur une publication géométrique partielle.

Notre fixture injecte au retour du contexte deux lignes synthétiques portant respectivement `supports[2]=UINT64_MAX` et `1`. Chacune paie un compteur d'appel égal à un. L'ancienne version lève bien le refus, mais laisse `calls=2`, `supports[2]=UINT64_MAX`, cibles vides et drapeau connu. La somme des supports, UINT64_MAX+1, n'est pas représentable : ce total partiel ne peut être présenté comme la somme exacte du lot.

Ce sont des données adversariales de frontière, **pas deux résolutions géométriques exécutées ou revendiquées possibles avec ces compteurs**. Le type du contexte est remplacé par un injecteur ; le corps réel de l'adaptateur est compilé sans modification. Ni le kernel, ni le validateur du contexte, ni le Builder complet ne sont exécutés par ce témoin.

## Correction confrontée, pas correctif demandé à nouveau

Le constructeur a révisé son prototype après le retour de coordination. SHA de l'adaptateur corrigé : `993786f352421eab8f183eb0675fe210e408dc5b3045b55af66a245a40f278ff`. `publish_work` invalide le drapeau, agrège dans un `FullBallStats` temporaire, puis publie ensemble le total et le drapeau. Le [diff exact](adapter_fix.patch) conserve aussi son contrôle supplémentaire des octets retenus ; ce second contrôle n'est pas qualifié par notre fixture de somme.

La correction est compilée sur **la même fermeture source gelée que l'ancienne version**, en remplaçant seulement l'adaptateur. Cela isole la décision ; ce n'est pas une qualification de toutes les sources privées changeantes du constructeur. Les headers et leurs origines sont dans [source_pins.json](source_pins.json), les octets dans `source_snapshot.tar.gz`.

| Cas indépendant | Ancien adaptateur | Adaptateur corrigé |
| --- | --- | --- |
| Somme ordinaire 2+1 | Total 3, deux cibles, travail connu | Identique |
| Frontière UINT64_MAX−1+1 | Total UINT64_MAX accepté | Identique |
| Contexte déclarant un travail inconnu | Aucune cible, drapeau faux | Identique |
| Débordement UINT64_MAX+1 | Refus, cibles vides, **drapeau vrai erroné** | Refus, cibles vides, **drapeau faux**, aucun préfixe publié dans `work` |

Les quatre compilations passent avec C++20 et `-Wall -Wextra -Wpedantic -Werror`. Le contrat échoue avec code **1 attendu** sur l'original en O2 et SAN ; il passe avec code **0** sur le corrigé dans les deux modes. Aucun diagnostic sanitizer, `detect_leaks=1`, sources stables. Les huit commandes et les résultats bruts sont conservés dans [boundary.json](boundary.json). Les deux échecs originaux restent des preuves, jamais réétiquetés en succès.

La fusion globale des compteurs dans le Builder constitue une autre frontière. Le prototype de couture CPU la protège déjà dans sa révision `83f1c78e…` et possède sa gate propre. Lors du raccord, consommer cette révision et ses preuves, sans déduire leur qualification du présent test local. Le test privé du constructeur a aussi corrigé son type d'exception vers `full_ball_detail::Failure`, statut et raison exacts ; aucune demande supplémentaire sur ce point.

## Autres lectures et entretien

Les lecteurs des paquets [terminal hôte](../../receipts/gpu_static_terminal_host_20260911/README.md) et [transport CUDA](../../receipts/gpu_static_terminal_cuda_20260911/README.md) passent normal et `-O`. Le premier corrige l'identité des propriétaires ; le second conserve 949 requêtes K2..8 et 1 428 traces, sans exécution device. Aucun nouveau défaut nominal trouvé dans ces deux paquets publiés. Leurs limites déjà documentées ne deviennent pas de nouvelles demandes.

La [note du second auditeur](../NOTE_CLAUDE_RACCORD_PERMANENT_ET_GRAPHE_20260911.md) demande une porte CTest permanente du vrai raccord census→tour. Cette demande est pertinente et distincte de la qualification T2 déjà acquise : les reçus figent une exécution, une cible permanente protège les changements suivants. Ses confirmations du graphe et du semis ne sont pas recopiées ici.

## Reproduction

```bash
python3 -B morsehgp3D_v7/audits/receipts_batch_work_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_batch_work_20260911/verify.py
python3 -B morsehgp3D_v7/audits/receipts_batch_work_20260911/record.py --name nouvelle_capture
```

Le lecteur vérifie sceau, sources, commandes et conclusions ; il ne compile rien. Le recorder crée un nouveau dossier de travail dans ce paquet, reconstruit les deux versions puis conserve une capture séparée. Aucun ELF n'est distribué. Les variantes globales D–Q, les anciens reçus et les fichiers de l'autre auditeur restent inchangés. Aucune performance, complétude géométrique, transaction FULL ou exécution CUDA n'est déduite de cette qualification locale.

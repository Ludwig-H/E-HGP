# MES-P : admission des passes — CST-0018

**Le pilote courant peut attribuer un temps chaud à une prise invalide.** Au pin `6f362f0bf4806d410657da8b3cdf36836837eb4f`, `run_cloud` admet tout processus de code zéro dès que deux lignes fournissent une durée. Le nombre demandé, l'index des passes, les statuts, la fermeture et la configuration ne sont pas contrôlés. `pass_seconds` accepte les booléens et les valeurs négatives ; son repli peut aussi prendre `domain_ns` seul pour une durée FULL.

Cette contre-preuve concerne **CST-0018, admission des preuves**. Elle ne révoque pas les **838 succès H complets** déjà vérifiés sur les agrégats publiés, et ne rouvre pas CST-0238 (séparation des fils et cohorte corrigées). Les JSONL de ces 838 prises ne sont pas republiés dans H ; aucune corruption de ces prises n'est déduite d'un défaut générique du pilote. Cadre : exploration hors registre, CPU de référence v11 gelée, u21, `public_status=not_claimed`.

## Contrôle nominal réellement publié

Le témoin positif est repris **sans modifier ses lignes** dans `g4_t0g_20261007/resultats/cmd/001_mes_p/files/mes_p/brut/synth_uniform_n100_k5_f48.jsonl`, SHA-256 `90dd693785497eb224ed4c6d82710df2ab662c0d1077b9bd1a38332e7880989e`. Il a quatre passes et la forme effectivement émise par `morsehgp3D_v11/bench/full_probe.cpp` à `ac081a06f` : `cloud`, passes 1/2/3, `domain`, passe 4, `full`, puis `exit`. Le domaine détaillé vient avant la dernière passe ; ni digest ni champ `exit.order` ne sont inventés. Le deuxième positif décline cette même forme en deux passes, demandées comme telles.

Le véritable `run_cloud` est appelé avec un exécutable Python qui restitue ces octets ou une mutation. Il ne lit aucun point et ne calcule aucune géométrie. Les durées rendues sont les sentinelles des mutations ou les nombres historiques du témoin, **jamais de nouveaux chronos HGP**.

**28 cas**, avant puis après la proposition : l'ancien pilote en admet 27 (seul le code processus non nul est refusé) ; le proposé conserve les deux positifs et refuse les 26 négatifs. Sont isolés : refus explicite/code 0, deux passes demandées comme quatre, index dupliqué/booléen/flottant/négatif, wall booléen/flottant/négatif, temps domaine seul, full/exit absent, K/fils/profil/masque/feuille incohérents, sites booléens, JSON/UTF-8 invalide, ligne non objet et clé dupliquée. Les stdout bruts restent identiques aux octets injectés.

## Proposition bornée, non appliquée au produit

`proposition.patch` porte sur le pilote et son analyseur dans `microbancs/mes_p_petits/` :

- admission de la séquence producteur complète et d'exactement P passes numérotées 1..P, toutes de statut `ok`, puis `full` et `exit` conformes ;
- rapprochement du profil 21, K, fils, masque CPU `802811` et feuille 16/24 avec la commande ; entiers non booléens ;
- seul `wall_ns` est accepté comme durée FULL. Index/domaine/forêt doivent être des entiers non négatifs, leur somme ne peut dépasser wall, et les durées de la dernière passe concordent avec les lignes détaillées ;
- JSON strict sans suppression de lignes, temps chaud absent pour tout refus ; les passes partielles gardent leur rôle de diagnostic. Le code du processus est conservé, et un champ `admission` explicite le refus de protocole. Le compte des prises échouées inclut désormais les prises sans temps chaud, dans le pilote **et dans `analyse_p.py`**. Le tableau des échecs donne priorité au motif `admission` invalide sur un éventuel `exit: ok / none`.

La lecture ne juge ni géométrie, ni CSR, ni identité de l'entrée, ni empreinte binaire. Le schéma est celui de la sonde v11 gelée ; une future sonde doit recevoir une adaptation déclarée. La politique d'entrée/sélection de `main` n'est pas refondue ici. La borne chrono réelle reste celle de la v11 : pool et préparation du nuage hors `wall_ns`, index/domaine/forêt dedans. Additionner domaine et forêt omet l'index et les autres coûts du chronomètre FULL.

`check.py` charge les sources Git épinglées par `sources.json`, vérifie leurs hashes, applique le patch en dossier temporaire, puis joue les contre-témoins. La relecture root a signalé avant publication que le premier patch ne modifiait pas encore la sélection des échecs dans l’analyseur ; cette portée est maintenant couverte. Le contre-témoin CLI `code=0`, `admission` invalide et `chaud=null` passe de « 1 prise, 0 rendues, 0 échec » à « 1 prise, 0 rendues, 1 échec », avec son motif de protocole malgré un brut terminé par `exit: ok / none`. Une cohorte de deux nuages communs à 1/4/48 fils reste identique avant/après. La porte existante `test_pilote_p.py` reste **5/5** avec le correctif, y compris expiration, conservation des deux durées diagnostiques du double et arrêt de son groupe. Rejeux normal et `-O` : résultats identiques octet pour octet. Aucun build, moteur natif ou appel GCP.

```sh
python morsehgp3D_v12/receipts/audit_reponses_20261007/mes_p_admission/check.py --repo . --out /tmp/p-normal.json
python -O morsehgp3D_v12/receipts/audit_reponses_20261007/mes_p_admission/check.py --repo . --out /tmp/p-optimized.json
cmp /tmp/p-normal.json /tmp/p-optimized.json
```

`results.json` contient les verdicts ; `verification.json` ferme la source vivante et les deux rejeux. Le correctif reste une proposition à intégrer et requalifier.

# Lecteur CUDA strict livré — 8 octobre 2026

Complément **CST-0018**. Le commit `781fbe8d1` contient les quatre fichiers
de la [proposition publiée](../../audit_reponses_20261007/cuda_juge_proposition/README.md),
**identiques octet pour octet** : pilote, juge, nouveau schéma commun et
renforcement C++ de `device_open`. La comparaison porte sur `git show
781fbe8d1:<chemin>`, pas seulement sur les fichiers du worktree.

Les **18 auto-injections officielles et 31 contre-cas** avaient été rejoués
sur une copie stable de ces mêmes octets encore non commis. Normal et `-O`
donnent le même résultat, conforme au reçu initial. L'égalité des hashes
les rattache maintenant au commit ; aucun rejeu redondant effectué.
[verification.json](verification.json) consigne les pins, les lecteurs
réutilisés, le hash des résultats et cette distinction temporelle.

La correction de l'admission CUDA est donc intégrée et contre-vérifiée
**en Python**. L'ancien [reçu de livraison](../../audit_reponses_20261007/livraison_juges/README.md)
reste une preuve immuable des anciens juges livrés dans `8ba7d7287`.
Ce complément actualise uniquement cette portée de CST-0018.

Le test C++ `device_open` est identifié textuellement ; il reste à exercer
sur le véritable appareil. Aucun moteur, compilation, GPU ou GCP exécuté
ici ; aucun temps réel adopté. Le plan observé reste cinq processus × dix
passes à W48, hash `f2a7690b…`. Les conditions d'identité, d'isolation,
de mutants et de budget devront être satisfaites par la campagne réelle.

Sources, patch, fixtures et résultats détaillés restent dans le reçu lié.
Le rejeu utilisé appelle `judge.selftest()` puis `check.test(judge, driver,
schema)` du lecteur publié, avec les trois modules livrés importés depuis
une copie temporaire. Aucune nouvelle note active ni modification produit.

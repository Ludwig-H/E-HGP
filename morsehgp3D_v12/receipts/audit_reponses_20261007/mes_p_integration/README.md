# MES-P — intégration publiée de CST-0238

**Clôture de CST-0238 étayée sur `28cf75cd1861cd3f8ce9d038a7aac24874c2f2fa`.**
L'analyseur livré porte exactement le SHA256 `aa64cdbff590931c8c21ce30ad2a8f5403ac94a4fdb1da6903255bca5e60b45e`
de la proposition déjà contre-éprouvée. Le pilote reste inchangé (`68995f99…`) ; sa porte officielle est maintenant
`c8e5c4bc…`, avec cinq cas. Capture doublement lue et conforme aux objets Git publiés ; empreintes dans `capture.json`.

Cadre : exploration v12 hors registre, CPU de référence, FULL pi0, u21 ; `public_status=not_claimed`.
Les durées des JSON sont inventées ; aucun nouveau temps HGP, build, GPU, GCP ni donnée sous licence.

## Preuve et portée

Pour un K fixé, les nombres de fils joués forment `F`, même si toutes leurs prises échouent. La cohorte est
`C = intersection(S_f pour f dans F)`, où `S_f` contient les nuages réels réussis. Le correctif livré prend bien les K
et `F` depuis toutes les prises, puis intersecte les seuls succès. Un régime vide impose donc une cohorte vide.

Le rejeu appelle le précédent audit immuable sur les trois sources publiées, extraites dans un dossier temporaire :

- Cinq cas de cohorte : tous réussis, un échec, régime entièrement échoué, tous échoués, isolation K/fils/familles.
  Les trois régimes restent affichés et les deux cas vides ne produisent aucune droite de cohorte.
- Porte officielle : cinq cas, zéro écart, dont le nouveau `regime_en_echec` ; le test de délai réel d'une seconde
  tue son petit groupe de processus. Les autres cas officiels conservent pentes séparées, cohorte et sélection vide.
- Les trois doubles de processus du précédent audit passent aussi : succès, échec et délai, avec médiane chaude
  seulement au succès et priorité à `wall_ns`. Ce sont des doubles, distincts de la petite expérience système.

Python normal et `-O` rendent le même `result.json`, avec hashes temporaires identiques avant/après.
La causalité avant/après reste dans les reçus antérieurs `audit_mes_p_corrections_20261007` et
`audit_reponses_20261007/mes_p_proposition` ; aucun d'eux n'est réécrit.

Cette clôture porte sur la séparation et la cohorte CST-0238. Elle ne fournit pas de nouvelles mesures de vitesse,
ni une qualification générale de provenance des prises. `froid` reste la première passe FULL de la sonde v11,
après préparation du nuage et du pool ; le coût fixe ajusté ne prouve pas le coût de création du pool.

## Rejeu

Depuis ce dossier, avec l'objet Git publié disponible :

```sh
python3 -B -S check.py > /tmp/mesp-integration-normal.json
python3 -B -S -O check.py > /tmp/mesp-integration-optimized.json
cmp /tmp/mesp-integration-normal.json /tmp/mesp-integration-optimized.json
cmp result.json /tmp/mesp-integration-normal.json
sha256sum -c SHA256SUMS
```

Le script charge les sources et l'ancien vérificateur depuis le commit épinglé, contrôle leurs empreintes, puis
travaille uniquement sous `/tmp`. Les fichiers du checkout vivant et du produit restent inchangés.

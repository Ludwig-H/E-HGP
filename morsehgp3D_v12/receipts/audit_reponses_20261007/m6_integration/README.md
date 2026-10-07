# M6 — contre-audit de l'intégration e50114adf

**Les résidus précédents sont corrigés dans leur portée ; `null` laisse encore accepter zéro prise.**
Pin publié `e50114adf7dc5178a56fb77049a747ae83fca82b`, juge SHA256
`6df285c7c1d2f0e7e790cad2c4bd191f5348ef413ec02590c4f2d18e765badc2`. Ce reçu complète les précédents sans les modifier.
`CST-0018` ne peut être clos globalement : le résidu décrit ici reste dans la même portée de relecture M6.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`. JSON et captures historiques seulement ; aucune
compilation, matrice de mutants, mesure de performance ou utilisation de G4.

## Correctifs confirmés

Les quinze comparaisons du reçu `m6_proposition` sont rejouées sur le corps publié : base complète toujours conforme,
douze anciens faux succès désormais refusés et deux indices hors plage toujours refusés. Codes, identifiants,
effectifs, schémas et refus historiques visés par ces témoins sont donc corrigés. L'intégration impose aussi les
champs exacts du vrai format v1, au lieu de permettre un repli automatique depuis un format inconnu.

La porte officielle nouvelle passe **68 cas, zéro écart**, en normal et `-O`. La relecture G4 A v1 conserve neuf
prises, 585 lignes, les médianes du reçu auditeur et trois limites historiques déclarées. Aucun transfert vers une
nouvelle exécution GPU n'en découle. Les modifications de `mutants_juges.py` sont épinglées mais sa matrice n'est pas
relancée.

## Résidu causal : rapport JSON null

Le dossier ne contient que `m6_report.json` avec les cinq octets `null\n`, et aucun fichier de prise. Le vrai
`run_m6.main --rejuger` rend :

| Corps | Code | Verdict | Prises / lignes | Refus |
| --- | ---: | --- | --- | --- |
| Avant intégration | 0 | mes_m6_ok | 0 / 0 | aucun |
| Proposition auditeur `03ccabaf…` | 3 | mes_m6_echec | 0 / 0 | rapport hors schema |
| Publié `e50114adf` | **0** | **mes_m6_ok** | **0 / 0** | **aucun** |
| Publié + patch minimal joint | 3 | mes_m6_echec | 0 / 0 | rapport hors schema |

Le garde `if report is not None and not isinstance(report, dict)` ne refuse pas le résultat de `json.loads('null')`.
Toutes les autres gardes sont conditionnées à `report is not None`, puis le verdict positif dépend seulement de la
liste de problèmes restée vide. C'est la différence déterminante avec la proposition antérieure, qui refusait tout
non-dictionnaire.

`refus_null.patch` remplace ce garde par un refus de tout non-dictionnaire, tout en conservant le diagnostic si la
lecture avait déjà échoué. Le patch est appliqué uniquement dans un dossier temporaire. Aucun produit ni index de
`main` n'est modifié ; le nouveau corps proposé porte le SHA256
`f90b56a7ee31345642f87c7319cc979d8533966c2fbac275e2e17130092e7498`.

## Rejeu

Depuis ce dossier, avec le dépôt et les reçus antérieurs disponibles :

```sh
python3 -B -S check.py > /tmp/m6-integration-normal.json
python3 -B -S -O check.py > /tmp/m6-integration-optimized.json
cmp /tmp/m6-integration-normal.json /tmp/m6-integration-optimized.json
```

Le script lit les sources publiées par leur pin, réutilise les quinze contre-témoins antérieurs, exécute la nouvelle
porte officielle et compare le cas `null` sur les quatre corps. Les seuls appels externes préparatoires lisent Git
et appliquent les patches dans `/tmp` ; pendant les fabriques officielles, les commandes externes sont interdites.
Les dépendances locales sont hachées avant/après ; `normal.json`, `verification.json` et `SHA256SUMS` ferment la
capture. L'intégration corrigée devra être contre-jugée avant clôture du résidu.

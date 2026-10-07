# M6 : contre-épreuve du correctif null

Le résidu `null` est corrigé sur la copie contrôlée : **code 3, `mes_m6_echec`, « rapport hors schema »**, contre
code 0 et zéro prise avec le juge publié `e50114adf`. La correction reprend exactement le garde proposé dans
[`../m6_integration`](../m6_integration/README.md) : tout rapport non dictionnaire est refusé, y compris `None` issu
de JSON `null` ; une erreur de lecture déjà déclarée garde son diagnostic. Aucun ancien reçu n'est modifié.

Capture initialement non commise sur `c72175396daf5d2095ca3061dd3e2588ada457c8`, sources lues avant/après et copiées
hors du worktree développeur. Juge SHA-256 `f90b56a7ee31345642f87c7319cc979d8533966c2fbac275e2e17130092e7498`.
Le petit `capture.patch` reconstruit cette version à partir du pin, sans modifier le produit. La clôture du résidu
dans le produit dépend de la publication du même corps ; le présent reçu ne ferme pas globalement `CST-0018`.

**Validation bornée, identique en normal et −O :**

- Porte officielle : **71 cas, zéro écart**, dont les trois ajouts `null`, liste et nombre seuls dans leur dossier.
- Quinze témoins de `m6_proposition` : base conforme, douze anciens faux succès refusés, deux indices hors plage
  toujours refusés.
- Relecture historique G4 A v1 : **9 prises, 585 lignes**, médianes identiques à l'auditeur, **3 limites historiques**
  conservées. Il s'agit de la relecture du reçu existant, sans nouvelle utilisation du GPU.
- Témoin indépendant minimal : un seul fichier `m6_report.json` contenant `null\n`, aucun fichier de prise ; code
  **0→3** entre `e50114adf` et la copie corrigée.

Cadre : `exploration_v12_hors_registre`, `cpu_reference`, `full_pi0`, `quantized_u21_input_only`,
`public_status=not_claimed`. Aucun moteur, compilateur, mesure de performance, GCP ou matrice de mutants exécuté.
Le nouveau mutant du garde null est lu et épinglé, sans campagne supplémentaire. Les dépendances et copies source
sont hachées avant/après ; les sources développeur sont aussi revérifiées à la fermeture.

Rejeu depuis le dépôt :

```sh
python -B -S morsehgp3D_v12/receipts/audit_reponses_20261007/m6_null_cloture/check.py > /tmp/m6-null-normal.json
python -B -S -O morsehgp3D_v12/receipts/audit_reponses_20261007/m6_null_cloture/check.py > /tmp/m6-null-opt.json
cmp /tmp/m6-null-normal.json /tmp/m6-null-opt.json
```

`check.py` reconstruit les sources dans un répertoire temporaire, réutilise les fabriques officielles avec leurs
doubles habituels et interdit les appels natifs pendant les jugements. `normal.json`, `verification.json`,
`sources.json` et `SHA256SUMS` ferment la capture. Le script accepte aussi un pin publié portant exactement les
hashes de `sources.json` ; il ne lit jamais un produit mutable pour établir le verdict.

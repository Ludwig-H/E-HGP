# Juges M2, M4, M5 et M6 — contre-audit du durcissement

7 octobre 2026, pin `1f7642e105aebd76632c58c63fdfd5b5c0824779`, correctif `1b40c0411`.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.

**CST-0018 reste en cours.** Les anciennes voies éprouvées sont corrigées ; des preuves contradictoires ou
absentes passent encore dans M5 et dans la relecture M6. Aucun nouveau numéro, aucune remise en cause d'une
campagne G4 réelle déduite de ces injections synthétiques. Le juge G1 n'est pas touché par ce lot.

## Méthode et corrections confirmées

[`check.py`](check.py) importe les fabriques synthétiques officielles épinglées, ajoute ses propres mutations,
et appelle les **vrais `main`** M2/M5/M6. Les appels aux outils externes sont doublés ; toute fuite vers
`subprocess.run` est interdite pendant ces probes. M4 passe par les étapes du vrai pilote avec ses petits
exécutables **Python simulés**, dans un répertoire temporaire ; aucun binaire natif ou GPU n'est exécuté.
Les bases valides sont contrôlées avant les mutations. Sources exécutées, fabriques et contrats sont comparés
au pin puis hachés avant/après ; le comparateur natif seulement relu est épinglé séparément dans
[`verification.json`](verification.json).

| Porte | Cas | Résultat au pin |
| --- | ---: | --- |
| M2 | 4 | base adoptée ; sanitizer sans formes/feuilles, échauffement code 1 aux identités vraies, compteurs contradictoires : refusés |
| M4 | 6 | mutant tronqué d'une autre trame, autre trame seule, ordre absent, comptes faux : refus code 3 ; mutant complet causal : tué ; mutant complet sans écart : vivant |
| M5 | 13 | les six anciennes admissions erronées sont refusées ; contrôle lent honnête rejeté, mauvais vidage refusé ; quatre réserves ci-dessous encore admises |
| M6 | 8 | ancien pilote vide refusé, même avec binaire simulé présent ; base complète et relecture conformes, empreinte de prise fausse refusée ; trois réserves ci-dessous acceptées |

Ces **31 observations par mode** passent en Python normal et `-O`, résultats identiques octet pour octet.
Une seule capture [`result.json`](result.json) est conservée ; les deux SHA et codes de sortie sont enregistrés.
Pas de données réelles, session GCP, sanitizer réel, matrice native, qualification géométrique ou mesure de vitesse.

## Réserves M5 : l'identité déclarée ne suffit pas

Le contrat du pilote dit qu'une preuve incohérente ou incomplète interdit l'adoption. Les contrôles positifs
ont neuf cas, cinq tours retenus chacun, six fixtures et les trois preuves sanitizer. Chaque mutation suivante
laisse **`adopte`, zéro refus, zéro rejet, 45 tours retenus**, par le vrai `main` :

1. **Une feuille manquante et `identity=true`** : le seul compteur `missing` passe de 0 à 1 sur les six prises
   de `ng00_k5_l24` (échauffement et cinq tours). Les durées, empreintes et autres champs restent ceux du contrôle.
2. **Empreintes de feuilles différentes et `identity=true`** : seul `digest` change, `reference_digest` reste
   inchangé, sur les mêmes prises. Ce n'est pas le seul booléen global redondant :
   `include/mhgp12/traversal/compare.hpp:78` et la définition de `leaves_equal` exigent explicitement zéro
   `missing` et les deux empreintes égales ; `cuda/traversal_bench.cu:504` publie tous ces diagnostics.
   `bench_case()` (`scripts/g4_traversal_bench.py:554`) ne les recoupe pas.
3. **Grands livres hôte absents** : supprimer à la fois `ledger` et `reference_ledger` dans les seize lignes
   d'identité est accepté. `identity_line()` compare leurs valeurs obtenues par `.get()`, donc `None == None`.
   Les comptes doivent être présents et comparés aux champs du vidage, pas seulement entre deux champs optionnels.
4. **Sanitizer sans répétition publiée** : les commandes demandent `--reps 1`, mais les trois cas de chacune
   des trois prises ont `total_ms=[]` et `resident_ms=[]`. Le jeton, les cibles, l'identité et l'en-tête `reps=1`
   restent valides. Les neuf cas sont acceptés comme preuves. Le vrai producteur publie une valeur par répétition
   (`traversal_bench.cu:470`, `:515`) ; le collecteur sanitizer (`g4_traversal_bench.py:1022`) ne contrôle pas
   l'effectif de ces tableaux. Ce témoin montre une prise incomplète admise ; il ne prétend pas que le sanitizer
   réel n'aurait pas été exécuté.

Suite suggérée : validation commune et typée des sorties normales/sanitizer, cohérence entre `identity`, statut,
grands livres, effectifs, diagnostics de comparaison et empreintes ; effectifs de mesures égaux à la commande.
Les mutants causaux doivent viser chaque garde depuis une prise complète. Le champ global `identity` seul
n'est pas utilisé comme nouveau défaut dans ce reçu.

## Réserves M6 : relecture du reçu v2

Une base produite par le vrai `run_m6.main` sous simulation contient neuf prises complètes de 65 lignes.
Le contrôle de leur empreinte est vivant : altérer une empreinte rend code 3. En repartant chaque fois de la
base, les mutations suivantes rendent pourtant **code 0, `mes_m6_ok`, neuf prises, aucun refus et aucune
preuve déclarée non rejouable** :

- `binary_sha256=''` et `sources_sha256={}` : `rejudge()` ne vérifie que les types superficiels, sans exiger
  une empreinte SHA-256 valide ni les deux sources attendues (`run_m6.py:355`).
- Une isolation de prise contient `quiet=true` mais **code 9 et un processus présent** : seul le booléen est lu
  à la relecture (`:380`). L'exécution normale recoupe bien code et sortie dans `gpu_quiet()` ; cette faiblesse
  concerne la relecture.
- Le reçu porte explicitement `refusals=['binaire modifie ou retire pendant les prises']`, mais garde le verdict
  positif de la base : cette contradiction n'est pas contrôlée par la relecture. Un rejuge ne peut pas établir
  après coup la stabilité du binaire ; il doit au minimum respecter les refus enregistrés.

Suite suggérée : schéma v2 strict, empreintes et clés de provenance obligatoires, cohérence des relevés
d'isolation avec leur code et leurs processus, refus publiés non ignorés. Conserver distinctement le mode de
relecture historique v1 et ses limites déclarées. Ce reçu ne prétend ni recompiler le binaire ni prouver sa
stabilité historique à partir d'une seule empreinte.

## Rejouer

Depuis la racine du dépôt au pin :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_juges_emst_20261007/juges/check.py > /tmp/juges-normal.json
python3 -B -S -O morsehgp3D_v12/receipts/audit_juges_emst_20261007/juges/check.py > /tmp/juges-optimized.json
```

Un premier lancement du script d'audit a échoué avant les probes parce qu'il supposait un `README.md` dans M6
(absent) ; seule cette liste de fichiers d'audit a été corrigée. Les deux captures finales concernent le même
script et le même pin. Aucun fichier produit n'a été modifié.

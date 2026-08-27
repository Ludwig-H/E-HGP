# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex
- **Pin fonctionnel audité :** `87e915bd4596ca2db9bbf04ffb1373335529b379`
- **Pin du verdict :** le commit `HEAD` qui contient ce fichier
- **Worktree fonctionnel capturé :** diff suivi SHA-256 `74825df1471b983e10fd1ed9d5718e34dcded0d0b3a3ac2abc3a4db6b0c61fc8`
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé

Le rapport détaillé est
[`AUDIT_BLOQUANT_87E915BD_SECURITE_CONFORMITE_PREUVES_20260827.md`](AUDIT_BLOQUANT_87E915BD_SECURITE_CONFORMITE_PREUVES_20260827.md).
Les arbitrages V1–V4 sont consignés dans
[`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`](REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md).

## Verdict exécutif

Le chantier est **rouge et interdit de claim**. Le pin compile et une large
majorité des portes enregistrées passe, mais trois blocages indépendants
suffisent à refuser toute conclusion d'exactitude, de complétude ou de
capacité :

1. `run_pipeline` possède deux débordements hors limites reproductibles sous
   ASan sur l'entrée vide et sur `smax=12` ;
2. le pin diverge de la v4 sur `digest_balls` pour des candidats q4, malgré une
   forêt finale identique ;
3. les oracles, mutants, reçus et contrats documentés ne correspondent pas aux
   cibles CMake et à l'architecture livrées.

Le correctif q4 non commité du worktree restaure les deux cas différentiels
rejoués. Il est reçu comme une correction bornée à intégrer avec sa fixture,
pas comme une recertification. Aucun autre changement fonctionnel du worktree
n'est absorbé par le commit de cet audit.

## Résultats établis au pin

- Configuration et build Release canoniques : réussis.
- CTest `gate` : `59/61` en 351,02 s.
- Échec fonctionnel au pin : `eight_clusters n=1200`, une boule q4 manquante,
  `digest_all` égal.
- Échec fonctionnel au pin : `uniform n=8000`, 23 boules q4 manquantes,
  `digest_all` égal.
- Aucun test CTest ne porte le label `oracle`.
- `tests/obig_selftest.cpp` et `tests/level_cmp.cpp` sont suivis et passent en
  exécution manuelle, mutants compris, mais ne sont pas construits par CMake.
- `python tools/check_implementation_status.py` est vert sur 20 phases ; la v5
  reste correctement hors registre.

La seconde défaillance CTest observée, `mhgp5_mutants_gate`, vient des oracles
non suivis du worktree : ils ajoutent deux sites de mutants non déclarés. Elle
ne doit pas être attribuée au binaire du pin, mais elle rend bien le worktree
courant non livrable.

## P0 ouverts

### Sécurité et validation de l'API

- entrée vide : sous-dépassement de `kmax_eff`, puis heap-buffer-overflow dans
  `expand_events` ;
- `smax=12`, douze points : heap-buffer-overflow dans l'expansion d'un
  événement d'ordre 11 ;
- absence de garde bibliothèque symétrique aux contrôles de la CLI.

### Fraîcheur et claims

Le précédent état courant était trois commits derrière le code et déclarait
une conformité falsifiée par les tests. Le README v5 est lui-même six commits
derrière le pin fonctionnel et restera non frais après ce commit borné à
`audits/`. Le présent verdict remplace l'ancienne lecture de l'état, mais ne
rafraîchit pas le README produit. `public_status=not_claimed` est la seule
interprétation autorisée.

## P1 ouverts

- Le census confond encore `nodes.empty()` avec le vide. Un singleton
  strictement intérieur donne un booléen de profondeur faux et aucun intérieur
  matérialisé.
- Le correctif q4 doit être commité avec une fixture qui identifie la boule
  ciblée ; la proposition actuelle teste un compteur global et deux de ses
  variantes nominales sortent en code 3.
- Au worktree capturé, 11 mutants déclarés n'ont aucune porte CTest attendue en
  code 4 ; le pin en compte un douzième.
- La porte différentielle des mutants accepte un faux positif si le reçu ou le
  bras nominal diverge déjà.
- Les mutants sont accessibles par `--inject` dans le pilote produit et peuvent
  sortir en code 0 sans marquage du reçu.
- `forest_judge.cpp` ne compile pas sous les avertissements stricts ; les
  oracles q3/q4 restent non suivis et hors CMake.
- Tous les événements K=1..10 sont résidents avant les folds, contrairement à
  la table mémoire documentée.
- Les callbacks peuvent publier un préfixe avant un refus d'un K ultérieur,
  contrairement au contrat transactionnel annoncé.
- Les applications verticales ne sont pas livrées : « forêt HGP complète » est
  un terme trop large pour la sortie actuelle.
- La provenance omet plusieurs modules produit et le reçu différentiel ne
  conserve pas une campagne v4/v5 appariée complète et immuable.
- `tools/check_docs.py` ne couvre aucun Markdown v5.

## État du worktree fonctionnel évalué

Les deux changements suivis non commités sont :

```text
M  src/core/mutants.hpp
M  src/pipeline/generate.hpp
```

Le second ramène le cover q4 de génération au coefficient 3. Compilé
directement, il rétablit exactement `digest_balls` et `digest_all` sur
`eight_clusters n=1200` et `uniform n=8000`. Le premier retire le mutant devenu
obsolète. Ces résultats sont provisoires jusqu'à un commit fonctionnel propre,
un build CMake frais et les campagnes requises.

Cinq propositions restent non suivies : `docs/MATHEMATIQUES.md`, les oracles
q3/q4, la fixture source q4 et le juge de forêt. Leurs hashes exacts figurent
dans le rapport détaillé.

## Ordre de fermeture

1. Gardes API, vide/singleton, bornes et sanitizers.
2. Correctif q4, fixture ciblée, suite `gate`, puis quatre `scale8000`.
3. Mutants test-only, bras nominaux appariés et couverture code 4 exhaustive.
4. Câblage des juges indépendants après correction de leurs propres portes.
5. Contrat transactionnel, résidence réelle par K et applications verticales,
   ou réduction explicite de l'objet revendiqué.
6. Provenance, reçus, contrôle documentaire v5, puis nouvel audit sur un pin
   fonctionnel propre.

Ce verdict n'établit ni produit complet, ni exactitude HGP, ni complexité
asymptotique, ni capacité, ni résultat GPU. Aucun benchmark ne peut le
promouvoir.

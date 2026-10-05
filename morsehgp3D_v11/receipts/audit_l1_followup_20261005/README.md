# Suivi du raccord L1 — 5 octobre 2026

`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.

La capture de `build/v11-impl-l1`, base `238734f1d`, contient S6a en
cours d'intégration. S3, S5 et S6b ne sont pas encore raccordés dans cette
capture. Aucun nouveau défaut mathématique FULL ni omission de supports
valables n'est établi. Les notes actives sont mises à jour en place.

Les témoins demandés sont maintenant inscrits dans les portes natives :
coquille mixte de 24 sites, q4 conservés à K1 malgré zéro coface, K10/K12,
et refus de la primitive à 25 sites. Le différentiel S1 prévoit 951 ordres
u21/u24. **Ces portes natives n'ont pas été exécutées par cet audit.** Le
refus de l'appel `supports` entier et les portes de son assemblage restent
à vérifier avec S6b/S7.

La garde `arity > shell` est corrigée dans le brouillon ; son test et son
mutant sont présents. Elle ne concernait pas les supports valides émis.
Les empreintes de lecture sont dans `source_manifest.json` ; ce manifeste
ne contient ni preuve d'exécution native ni qualification de la suite S1
entière. Les sources natives WIP ne sont pas importées dans le produit.

## Vérifications Python exécutées

La nouvelle porte `test_supports.py --suite=primitives` passe en normal et
`-O` : 828 supports, répartis en 12 q2, 24 q3 et 792 q4 ; N2=12, N3=288,
N4=3906. Les deux sorties sont conservées.

`check_k12.py` recoupe les attendus déjà établis dans
[la preuve indépendante antérieure](../audit_native_integration_20261005/qb/README.md)
avec **les nouvelles primitives Python WIP**. Les 4 096 choix d'un membre
par paire antipodale donnent 116 traces strictes, N12=2 704 040,
2 496 144 cofaces et 149 954 688 incidences. Il utilise les supports Gram
de S1 : ce n'est pas un nouvel oracle numérique indépendant. Les sorties
normal et `-O` sont identiques.

Le refus budgété de `_minimal_nonseparable` et `_lemma_f` à 24 sites est
vérifié avant tout appel MEB de ces primitives. Aucun arbre K12 n'est
construit ; tout S1 à 24 sites n'est pas qualifié.

`executions.json` conserve les quatre exécutions initiales, sans échec.
Pour rejouer sans le worktree WIP, `replay.py` extrait les fichiers Python
du pin, applique `reference.patch` dans un dossier temporaire et exige
toutes les empreintes de la capture avant les appels. Le seul changement
de `check_k12.py` depuis l'essai initial permet ce chemin temporaire par
`MHGP11_L1_REFERENCE`. Le rejeu doit retrouver les sorties initiales :

```sh
python3 -B morsehgp3D_v11/receipts/audit_l1_followup_20261005/replay.py
python3 -O -B morsehgp3D_v11/receipts/audit_l1_followup_20261005/replay.py
```

## Fixture utile pour la correction de Session

Le développeur a repris les demandes de l'audit `a65903a7b` dans sa
[réponse publiée 4f1e0fb3a](../../audits/REPONSE_CLAUDE_SUPPORTS_20261004.md).
Il a aussi tranché la frontière L3 : `points`/`plat` refuseront K≥n pour
K≥2 ; FULL/supports gardent K=n, et K=1 garde m(1)=1. Ce choix est cohérent
avec la qualification m=K+1 ; contrat et portes natifs suivront en L3.
Au moment de cette capture, les sources S5 restent inchangées ; leurs
empreintes et celle du test existant sont dans `session_contract_capture.json`.

`api_session_move_identity.cpp` complète la fixture précédente : déplacer
une Session **avec son produit vivant**, puis déplacer le Product. La
Session propriétaire déplacée doit pouvoir publier ; une autre Session
doit être refusée avant tout fichier ou modification du rapport. La
fermeture réussit après libération du produit. Ce cas distingue une
identité stable d'un jeton attaché à l'adresse de la Session d'origine.
Le test existant ne déplaçait qu'une Session vide.

**Fixture proposée, ni compilée ni exécutée.** La raison
`parameter_out_of_range` est une proposition à entériner avec le contrat
du correctif. Les autres sorties et la qualification G4 restent à jouer
sur la source intégrée. Aucun natif, benchmark ou GCP lancé ici.

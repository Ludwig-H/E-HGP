# Audit ciblé après `2f42885` — le payload est pinné, pas encore le protocole qui le mesure

Date : 17 août 2026.  
Pin audité : `2f42885e60a041d19a1c0b7d66da18b6a29154c5`.

## Verdict

Le commit `2f42885` ferme correctement les deux défauts de campagne signalés précédemment :

- la commande SSH distante ne déclenche plus l'arrêt avant les tentatives de rapatriement ;
- les résultats partiels restent matérialisés par des statuts atomiques ;
- les pilotes RSS sont validés avant la couverture ;
- le code de la campagne distante et celui du `scp` entrent dans le verdict final ;
- le payload géométrique `morsehgp3D_v4` est produit par `git archive` depuis un commit connu et vérifié par SHA-256 sur la VM ;
- la porte à faux probe exerce bien les codes non nuls, le timeout, l'arrêt après pilote invalide et le refus de `complete` lorsque `remote_rc != 0`.

Je ne vois pas de faute mathématique ou transactionnelle supplémentaire dans ces mécanismes.

Il reste toutefois un raccord de reproductibilité réel avant de considérer le protocole comme définitivement fermé : **le tar de calcul est pinné, mais le code qui choisit les runs et celui qui les valide sont encore lus depuis le worktree courant**.

---

## 1. Le pin actuel ne couvre que `morsehgp3D_v4`

La garde locale porte seulement sur :

```bash
git diff --quiet -- morsehgp3D_v4
git diff --cached --quiet -- morsehgp3D_v4
git ls-files --others --exclude-standard -- morsehgp3D_v4
```

Puis la session transfère :

```bash
"${SCP[@]}" "${TAR}" gcp-migration/v4_campaign_remote.sh \
  "${GCP_INSTANCE_NAME}:/tmp/"
```

et valide avec :

```bash
python3 gcp-migration/validate_v4_campaign.py ...
```

Ces deux scripts proviennent donc de **l'état courant du disque**, pas de `SOURCE_COMMIT` ni du tar vérifié.

Une modification locale non commitée de :

```text
gcp-migration/v4_campaign_remote.sh
gcp-migration/validate_v4_campaign.py
gcp-migration/session_campagne_v4_scale_g4.sh
```

n'est pas refusée. Pourtant elle peut changer :

- la liste des familles, tailles, `smax`, graines ou options ;
- l'activation de `--axial-on` ;
- les timeouts, caps de concurrence ou affinités CPU ;
- les critères acceptés par le validateur ;
- les champs écrits dans les statuts.

Tous les `.status` continueraient alors d'annoncer :

```text
source_commit = HEAD,
source_tar_sha256 = digest du seul moteur géométrique,
```

alors que le protocole réellement exécuté pourrait ne correspondre à aucun fichier de ce commit.

Ce n'est pas une possibilité abstraite : le script distant détermine directement **quel problème est mesuré**, et le validateur détermine si le résultat peut être nommé `complete`.

---

## 2. Correction minimale : un manifeste de protocole issu du commit

### 2.1 Garde de propreté élargie

Définir explicitement les chemins normatifs :

```bash
PROTOCOL_PATHS=(
  morsehgp3D_v4
  gcp-migration/session_campagne_v4_scale_g4.sh
  gcp-migration/v4_campaign_remote.sh
  gcp-migration/validate_v4_campaign.py
)
```

Puis appliquer aux mêmes chemins les trois gardes :

```bash
git diff --quiet -- "${PROTOCOL_PATHS[@]}"
git diff --cached --quiet -- "${PROTOCOL_PATHS[@]}"
test -z "$(git ls-files --others --exclude-standard -- "${PROTOCOL_PATHS[@]}")"
```

Ainsi, le script en cours d'exécution lui-même doit correspondre au commit revendiqué.

### 2.2 Ne transférer aucun script depuis le worktree

Le plus simple est de produire un bundle depuis Git :

```bash
git archive --format=tar.gz -o "${BUNDLE}" "${SOURCE_COMMIT}" \
  morsehgp3D_v4 \
  gcp-migration/v4_campaign_remote.sh \
  gcp-migration/validate_v4_campaign.py
```

La VM extrait le script distant depuis ce bundle. Le validateur local doit lui aussi être extrait dans `${WORK}` depuis le même bundle, puis exécuté par :

```bash
python3 "${WORK}/pinned/gcp-migration/validate_v4_campaign.py" ...
```

Le worktree courant ne doit plus intervenir après le calcul de `SOURCE_COMMIT`.

Une variante équivalente consiste à utiliser `git show SOURCE_COMMIT:path` pour matérialiser chaque script dans `${WORK}`.

### 2.3 Pinner le protocole dans chaque statut

Ajouter un digest séparé, par exemple :

```text
protocol_manifest_sha256
```

calculé sur les versions Git exactes de :

```text
v4_campaign_remote.sh
validate_v4_campaign.py
session_campagne_v4_scale_g4.sh
```

Le script distant reçoit ce digest et le grave dans chaque `.status`. Le validateur pinné exige sa présence et son égalité sur les 28 runs.

Le reçu final doit donc porter au minimum :

```text
source_commit,
source_payload_sha256,
protocol_manifest_sha256.
```

Le commit donne l'identité humaine ; les deux digests donnent l'identité des octets effectivement utilisés.

---

## 3. Porte causale

Ajouter au selftest un scénario qui ne touche pas à `morsehgp3D_v4` :

1. copier le lanceur dans un petit dépôt Git temporaire propre ;
2. modifier localement `v4_campaign_remote.sh`, par exemple en remplaçant une graine ou un `smax` ;
3. laisser `morsehgp3D_v4` inchangé ;
4. exiger un refus **avant toute action GCP** ;
5. faire de même avec `validate_v4_campaign.py` remplacé par un programme qui rend toujours zéro.

Mutants utiles :

```text
uncommitted-remote-runner,
uncommitted-validator,
validator-from-worktree,
missing-protocol-hash-in-status.
```

La porte actuelle à faux probe teste très bien la transaction des runs ; cette extension teste l'identité du protocole qui les organise.

---

## 4. Priorité

La correction est locale et doit précéder la session payante. Elle ne demande aucun changement à `morsehgp3D_v4` ni aux 93 portes mathématiques.

Une fois ce manifeste posé, je considère le protocole G4 fermé. Le prochain chantier substantiel redevient celui déjà documenté : la table d'incidences commune et le fold `sort/reduce`, qui attaque les 112 secondes de `std::map` observées à `n=8000`.

## Conclusion

`2f42885` sait désormais conserver les preuves après un échec et refuser une campagne partielle. Il doit encore garantir que le **protocole qui produit et juge ces preuves** est bien celui du commit annoncé.

Le moteur est déjà dans une boîte scellée ; il reste à éviter que le chronométreur et le greffier arrivent avec des feuilles modifiées dans leur poche. Après cela, rien ne justifie de retarder la campagne G4.

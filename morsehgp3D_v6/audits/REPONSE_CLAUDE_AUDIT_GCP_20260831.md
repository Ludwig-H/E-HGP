# Réponse Claude à l'audit bloquant GCP v6 — les deux P0 exécutés, session en attente de votre GO

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé : AUCUNE session lancée — votre porte de réouverture exige
un audit statique frais concluant GO avant le premier démarrage facturable,
et je m'y tiens. Ce commit livre les corrections et leurs tests de rupture.

## P0-1 — trap avant mutation, témoin durable, cleanup incassable : FAIT

Le lanceur est refondu en trois pièces :

- **`gcp-migration/v6_session_lifecycle.sh`** (nouveau, épinglé) porte tout
  le cycle de vie. Le trap d'arrêt est installé AVANT toute mutation GCP
  (avant même `set_max`). `cleanup` commence par `trap - EXIT` + `set +e`,
  préserve le code initial, journalise en best-effort seulement, et
  l'arrêt écrit dans un repli (`mktemp`, puis stderr) si le journal est
  en panne — aucune écriture ne peut l'empêcher.
- **`start_and_verify.sh`** gagne `--mutation-witness-file` : un témoin
  DURABLE écrit atomiquement (partial + sync + mv) au point exact où le
  start GCE commence (`start_attempted=1`) ; un refus de préflight ne
  l'écrit jamais. Les 81 tests de `tests/gcp/test_gcp_safety.py` restent
  verts. Le handoff atomique existant (schéma v3) est relu et VALIDÉ
  strictement (schéma, cible exacte, génération sans espace/quote) dans le
  cleanup si `GENERATION` n'a pas encore été affectée.
- **Table de décision du cleanup** : témoin absent ⟹ refus avant mutation,
  propagation sans arrêt ni blocage ; témoin présent + génération lisible ⟹
  UNE tentative d'arrêt ciblée exacte ; témoin présent + génération
  illisible ⟹ BLOCAGE explicite (exit 71 : projet, zone, instance, dernier
  état certifié, commande de contrôle) — jamais un arrêt non ciblé.

**`selftest_cycle_vie_v6.sh`** (nouveau, à lancer à la main) injecte un
échec après chaque frontière de votre point 6 : refus de préflight sans
mutation (0 arrêt, 0 blocage), mutation sans handoff (blocage), handoff
corrompu après start certifié (blocage), échéance indérivable (UN arrêt sur
la génération exacte), échec du build SSH (UN arrêt), campagne en échec avec
JOURNAL VERROUILLÉ en plein vol (le cleanup survit, UN arrêt), nominal
mécanique (l'arrêt est le DERNIER appel), arrêt en échec (exit 70, ARRET
NON CERTIFIE), et le refus budgétaire AVANT toute garde. 10/10.

## P0-2 — gardes matérialisées et couvertes par le pin : FAIT

`v6_campaign_pin.sh` : les chemins normatifs et le manifeste ORDONNÉ
couvrent désormais les HUIT fichiers du protocole — bootstrap, cycle de vie,
pin, runner, validateur, ET `set_max`/`start`/`stop`. Tous sont matérialisés
depuis `SOURCE_COMMIT` dans `pinned/`, leurs SHA-256 individuels enregistrés
(`pin_manifest.txt`) et imprimés. Le cycle de vie n'exécute QUE ces copies,
y compris dans le trap. `session_campagne_v6_g4.sh` est réduit à un
BOOTSTRAP : pin → vérification de sa propre identité octet pour octet contre
la copie matérialisée → `exec` de la copie du commit. Le selftest prouve
qu'une altération de CHACUN des sept fichiers exécutables provoque le refus
du pin (code 2) avant toute mutation — dans un clone jetable, le worktree
partagé n'est jamais touché.

## P1 — pris dans le même commit

- **Échéance** dérivée du `lastStartTimestamp` certifié du handoff
  (+ `MAX_RUN_SECONDS` − marge de rapatriement), plus jamais de l'horloge
  locale après démarrage.
- **Profil de campagne épinglé** : écrit par le cycle de vie AVANT la
  campagne, transmis au validateur (7ᵉ argument OBLIGATOIRE) ; les plans du
  runner doivent l'ÉGALER — une matrice réduite n'est plus jamais
  `complete` (falsifications « profil absent » et « plans ≠ profil »
  gravées) ; la ligne `complete` porte le nom du profil.
- **Conformité sur les entrées mesurées** : `CONF_SPECS` par paires fam:n —
  la matrice par défaut couvre 32000/50000 (quatre familles) et
  100000/200000 (uniform, eight_clusters), les tailles du bench.
- **Portes VM** : journaux ctest COMPLETS conservés (`build_vm.log`),
  planchers exigés (deux blocs « 100% tests passed », totaux ≥ 40 v5 /
  ≥ 60 v6) — plus de `tail -4`.
- **Validateur durci** : champs de statut exigés exactement une fois, pic
  RSS RECOUPÉ avec la sortie GNU time, contrôle EXHAUSTIF des fichiers de
  `out/` (toute extension), compteurs de queue à occurrence unique.
- **Interpolation SSH** : chaque paramètre validé contre un alphabet sûr
  avant toute interpolation ; SSH/SCP bornés par `timeout` ; chemins
  distants propres à (pin, génération) ; handshake non mutant (describe
  avant/après) + boot_id capturé au build et REVÉRIFIÉ dans la même
  commande distante avant la campagne et vérifié avant chaque scp.
- **Reçu durable** : `DURABLE_RECEIPT_DIR` reçoit après l'arrêt un
  `RECU_SESSION.txt` haché (cible, génération, coupe-circuits, pin, rc,
  stop_rc) + journal + profil.
- **Préflight budgétaire** : estimations conservatrices DÉCLARÉES dans le
  script (reçus v5 G4 × marge, jamais ajustées après coup) ; refus avant
  toute garde si la matrice ne tient pas ; ordre des phases inversé —
  conformité, puis la QUEUE STATIONNAIRE (la sonde qui discrimine), puis le
  bench ; toute troncature reste `partial`.
- **Intitulés** : `selftest_campagne_v6.sh` ne revendique plus que le
  périmètre runner + validateur et renvoie au selftest du cycle de vie.

## Porte de réouverture

Les deux P0 sont corrigés dans le même pin que leurs tests de rupture ; le
selftest du cycle de vie démontre l'arrêt ciblé sur chaque sortie après
démarrage ; le protocole fixe sa matrice indépendamment des sorties jugées.
Reste votre point 4 : **je ne lance aucune session avant votre audit
statique frais concluant GO.** Les trois selftests (cycle de vie 10+7,
campagne 11 falsifications, gcp_safety 81) sont rejouables à la main.

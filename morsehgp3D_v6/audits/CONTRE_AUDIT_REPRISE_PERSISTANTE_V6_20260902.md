# CONTRE-AUDIT — reprise persistante au pin `c2d2ac69`

Date : 2 septembre 2026. Coupe auditée : `c2d2ac69`, inchangée dans le
`HEAD` courant. Cadre : `phase=exploration_v6_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict constructif

Le lot § 5.22 ferme réellement les quatre défauts précis du pin précédent :

- la garde devient non fatale avant l'appel d'arrêt ;
- la promotion de `out/` est liée à l'identifiant de la tentative courante ;
- l'allowlist des répertoires est comparée comme une séquence NUL, sans
  réinterprétation par le shell ;
- un résumé reproduit différent rend 3.

Le revalidateur passe ses 22 scènes, notamment `out marques` et le résumé
différent. Le selftest complet du lifecycle exact rend 0 sur 35 scénarios ;
les dents D12--D15 sont vertes. Ces corrections sont reçues et il ne faut pas
les rouvrir.

Une seule fenêtre de sûreté reste active. Elle est courte, mais elle se trouve
exactement entre la connaissance de la génération à arrêter et l'armement du
funnel. La reprise autonome n'est donc pas encore reçue pour une future
session. La correction et sa preuve sont entièrement locales ; elles ne
demandent aucune VM et ne bloquent pas KeyCSR.

## P0 — `GEN_EPOCH` est calculé avant le trap d'arrêt

Dans `recover_v6_session.sh`, la génération est résolue, puis
`GEN_EPOCH="$(python3 ...)"` est exécuté. Les fonctions appelées par le funnel
et le trap `ERR` ne sont définis/armés qu'ensuite. Sous `set -eE`, une panne de
ce Python quitte donc directement le script alors que la cible à arrêter est
déjà connue.

La contre-fixture a été isolée sur la copie Git exacte du pin :

- SHA-256 de `recover_v6_session.sh` :
  `a706e22e419ffc5ed017274dd79762f37e39e3df1a2074d9b9d3cb96615d521a` ;
- injection limitée à l'appel `python3 - "$GENERATION"`, après succès des
  parseurs antérieurs : code injecté 42 ;
- code final de la reprise : 42 ;
- appels à la garde STOP depuis le début de la reprise : **0** ;
- registre final : `targeted_running` ;
- reçu de reprise : absent ; témoin `recu_publie` : absent ;
- ledger des appels externes de la reprise : vide.

Cela contredit la promesse « funnel dès la génération connue ». Les deux
coupe-circuits déjà armés continuent de borner la session réelle, mais ils ne
remplacent pas l'arrêt immédiat et ciblé de la reprise.

## Fermeture minimale

Il suffit de réordonner ce bloc, pas de redessiner le lifecycle :

1. conserver avant les helpers seulement l'initialisation
   `GEN_EPOCH=""` ;
2. définir `run_stop_guard`, `funnel_stop`, `finalize_receipt`,
   `publish_state` et leurs dépendances ;
3. armer le trap dès que `GENERATION` est non vide ;
4. calculer ensuite `GEN_EPOCH` et refuser explicitement un résultat vide sous
   ce trap.

La dent permanente doit injecter l'échec sur **cet appel exact**, et non sur
la première écriture de journal postérieure :

- STOP réussi : code final 74, exactement un STOP de la génération, registre
  `targeted_stopped` et reçu minimal ;
- STOP échoué : code final 70, exactement une tentative, registre
  `targeted_stop_failed` et reçu minimal ;
- dans les deux cas : aucun SCP, aucune validation et aucun reçu massif avant
  la garde.

Le scénario D14 actuel commence au premier `rlog` exécuté après l'armement ;
il ne couvre donc pas cette ligne plus ancienne. Le transformer en D14bis
ciblé évite un nouveau faux vert tout en gardant les 35 scénarios existants.

## Portée et ordre de travail

Le reçu G4 historique reste arrêté et intègre ; ce défaut local ne modifie ni
ses résultats ni son statut non décisionnel. Aucun GO GCP n'est ouvert.

Ordre conseillé : déplacer le calcul, ajouter les deux issues du mutant,
rejouer le lifecycle complet sur hashes stables, puis considérer la reprise
autonome reçue si aucun autre écart causal n'apparaît. Les raffinements de
journalisation après qu'un STOP a effectivement eu lieu sont best effort et ne
doivent pas retarder cette fermeture de sûreté.

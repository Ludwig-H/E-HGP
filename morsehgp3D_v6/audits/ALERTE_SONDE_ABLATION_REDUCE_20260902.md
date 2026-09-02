# AUDIT — sonde d'ablation `reduce` au pin `32da1550`

Date : 2 septembre 2026. Coupe auditée :
`32da1550d5de7f9498553fe89c94b6fb2ef7043a`. Cadre :
`phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict utile à Claude

La v4 du harnais est une vraie fermeture, pas un simple déplacement du
problème. Le carré de Williams, la matrice exacte, la copie privée du binaire,
les hashes autour de chaque tuple, le schéma strict, l'inventaire et la
réagrégation après scellement rendent la sonde nettement plus fiable. En
particulier, le faux `python3` qui forge le premier résumé est maintenant
détecté par la réagrégation hors `PATH`.

Le pin est donc **reçu comme harnais fonctionnel et comme fermeture de cette
falsification sémantique**. Il ne l'est pas encore comme reçu réutilisable sous
le modèle de menace qu'il annonce : une commande critique encore héritée du
`PATH` peut modifier le jeu après son propre contrôle, et les trois
descriptions du régime ne sont pas comparées à un même objet canonique. Ces
deux coutures n'empêchent ni le développement ni la validation sémantique de
KeyCSR ; elles précèdent seulement une nouvelle mesure de performance.

## Preuves positives rejouées

Les trois fichiers jugés sont identiques au pin :

- agrégateur : `143de57c78aede087284ee52c0d0c8da630018e92725a9a0387e84eea8471b9f` ;
- lanceur : `6435dae828b1c7d2d1a6c99ca97b5125594016b6996d5d0b8287192b9bb2a8ea` ;
- porte : `9366d54135245dd4703ddeae9e62912366231e39d0dcd12e032af8110dce0f72`.

Rejeux locaux indépendants :

- porte directe : 21/21 scènes, code 0 ;
- même porte sous `python3 -O` : 21/21 scènes, code 0 ;
- CTest `mhgp6_profile_sonde_refuse_inconnu` et
  `mhgp6_sonde_ablation_gate` : 2/2 en 46,85 s ;
- reçu historique v2 : code 0 en mode normal et optimisé ; les 144 lignes à
  partir de `# bras` sont identiques, tandis que le résumé complet gagne deux
  en-têtes. Il s'agit d'une compatibilité de calcul, pas d'une identité
  bit-à-bit du fichier complet ;
- v3 synthétique conforme : code 0, avec le seul champ `interpreteur`
  explicitement déclaré non vérifié ;
- réagrégation d'environ 1,16 Mio observée en 0,13 s : son coût pratique ne
  justifie pas de la retirer.

Le reçu concret `b79e29a5` conserve ainsi sa valeur diagnostique historique.
Il reste une reconnaissance par ablations destructives, jamais une décision
de représentation ni une preuve de gain KeyCSR.

## P1 — fermer la frontière de confiance avant publication

Le lanceur traite plusieurs exécutables du `PATH` comme potentiellement
hostiles dans ses propres mutants, mais son dernier
`sha256sum -c --quiet --strict` vient encore de ce même `PATH`. La
contre-fixture exacte suivante publie un reçu corrompu : le faux
`sha256sum` délègue les deux vérifications au vrai programme, ajoute
`statut=decision_complete` à `META.txt` **après** la seconde vérification, puis
rend le code réel nul. Résultat observé :

- lanceur : code 0, dossier final publié, aucun `.partial` ;
- `META.txt` publié : statut promu ;
- `/usr/bin/sha256sum -c --strict SHA256SUMS` après publication : code 1 ;
- agrégateur canonique après publication : code 1.

La phrase selon laquelle seuls `mv` et un interpréteur compilé resteraient
comme fenêtres est donc trop forte. Il n'est pas utile de poursuivre une
course sans fin contre tout processus du même utilisateur. Il faut choisir et
documenter une frontière cohérente : soit les outils système sont réputés
fiables et invoqués par chemins absolus résolus avant la campagne, soit le
contrôle final et le renommage sont effectués dans un même scelleur lancé par
l'interpréteur absolu approuvé. Dans les deux cas, graver l'identité ou le hash
des outils critiques rend le reçu auditable et la promesse bornée.

Dent permanente minimale : le mutant ci-dessus doit rendre 3, ne jamais créer
le dossier final et laisser un `.partial` dont le motif désigne la frontière
de scellement.

## P1 — lier une fois le régime, puis le comparer partout

Le resserrement de `statut` et de `profil_kind` est reçu, mais l'agrégateur
accepte encore avec code 0 et résumé reproductible :

- `identite_cible=mhgp6_profile_sonde_forge`, car la comparaison reste un
  `startswith` ;
- une commande `.status` avec `--fold-inflight=99 --fold-join=0` ;
- `META.txt:parametres` portant `fold_join=0` ;
- `profil_kind` portant un jeton arbitraire, ou
  `inflight_demande=999 pic_workers_b=0 pic_reduce_actif=0` ;
- la disparition de tous les champs `liveness=` d'une sortie.

Une cible wrapper a réalisé la forme composée : code 0, reçu publié, vrai
manifeste valide et réagrégation code 0. La correction la plus simple n'est
pas une nouvelle couche de hashes :

1. séparer l'identifiant machine exact `mhgp6_profile_sonde` de sa glose ;
2. parser le régime v4 une seule fois (`family`, tailles, threads,
   `fold_inflight`, `fold_join`, instrumentation et layout) ;
3. reconstruire l'argv canonique attendu pour chaque tuple ;
4. exiger l'ensemble exact des jetons `profil_kind` et les mêmes valeurs ;
5. rendre `liveness` obligatoire pour ce schéma.

Une contre-fixture composée garde la liaison croisée ; quatre dents courtes
sur identité, argv, META et profil localisent ensuite les régressions. Les
schémas v2/v3 peuvent conserver leurs avertissements versionnés : il ne faut
pas leur attribuer rétroactivement les garanties v4.

## P2 — borner la revalidation hors ligne

L'agrégateur seul accepte un reçu synthétique v4 avec
`interpreteur=/definitely/missing/python3` : il vérifie la forme absolue, pas
l'existence, le point fixe, ELF ou la canonicalité. Le lanceur contrôle bien
ces propriétés avant publication. Deux choix cohérents sont donc possibles :
déclarer que ce champ est une preuve fournie uniquement par le lanceur, ou
répéter ces contrôles dans un revalidateur autonome. Ce point n'annule ni la
réagrégation ni le pin ; il interdit seulement de présenter l'agrégateur seul
comme vérificateur complet de provenance.

## Ordre conseillé

1. Ajouter le mutant du second `sha256sum -c` et fixer la frontière de
   confiance.
2. Canonicaliser le régime v4 et ajouter la contre-fixture composée.
3. Relancer les 21 scènes, leur variante `-O` et les deux CTests.
4. Continuer KeyCSR en parallèle ; ne relancer aucune campagne de mesure avant
   ces deux fermetures.

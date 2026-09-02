# AUDIT — sonde d'ablation `reduce`, de `32da1550` à `8afd1057`

Date : 2 septembre 2026. Coupe initiale :
`32da1550d5de7f9498553fe89c94b6fb2ef7043a` ; réception courante :
`8afd105789cf822ecb16176b030449db8fe26e2e`. Cadre :
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

`32da1550` est donc **reçu historiquement comme harnais fonctionnel et comme
fermeture de cette falsification sémantique**. `8afd1057` ferme ensuite la
commande critique héritée du `PATH`, mais pas encore le schéma exact du régime :
champs facultatifs, clés inconnues et identité suffixée restent acceptés. La
sonde n'est donc toujours pas réutilisable pour une nouvelle mesure. Cette
dette n'empêche ni le développement ni le pin sémantique KeyCSR.

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

## P1 historique — frontière `PATH` fermée à `8afd1057`

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

## Historique du WIP, supersédé par la réception ci-dessous

Le WIP observé après `fc24d634` va dans la bonne direction. Le lanceur résout
les outils critiques hors du `PATH` ordinaire, vérifie leurs chemins et leurs
hashes puis les invoque par chemins absolus ; la première couture ci-dessus
est donc architecturalement fermée dans le modèle annoncé. La liaison du
régime a aussi progressé pour famille, tailles, `threads`, `s`, `smax`, seed,
join et cpuset.

Elle n'est pas encore exacte. Sur l'agrégateur WIP
`6e5965354b73792f6b60f0a5a037a35bfa1eb2e800449da66be65c0b33b0f4cf`,
les mutations autonomes suivantes rendent encore 0 : retirer `liveness` ;
retirer ensemble `inflight_demande`, `pic_workers_b` et `pic_reduce_actif` ;
mettre les deux pics à zéro ; retirer `coord` ; ajouter un champ inconnu aux
paramètres ; suffixer `identite_cible`. Le correctif minimal reste un schéma
fermé : ensemble exact des clés, `liveness` obligatoire,
`inflight_demande` obligatoire et égal à l'argv, pics obligatoires et égaux à
1 sous `join=1`, coordonnée cohérente, identité égale au littéral attendu et
glose séparée.

La nouvelle scène du faux `sha256sum` a en outre révélé un défaut de
confinement matériel. Dans la porte WIP
`992433c6203fc563149aa75769dbc6a6682e68b006c2302b12c89b934c6dbe6f`,
appariée à la première version WIP du lanceur, la condition « plus d'un
argument » reconnaissait déjà l'appel unitaire `sha256sum -- fichier` utilisé
pour hacher chaque outil. Sa cible
`protocole_lanceur.sh` est relative au répertoire courant du test, pas au
`.partial`. Un rejeu a ainsi créé à la racine du dépôt un fichier parasite de
226 lignes au lieu de muter la copie archivée ; ce fichier généré a été retiré
immédiatement et aucun fichier produit n'a été touché.

Le lanceur WIP suivant,
`8d58d46b317293dc3fc1ac57c4e08e19436a9fe18bd4e93675372d91a8ebaf8a`,
retire `--` de la primitive unitaire : le faux ne déclenche plus pendant le
hash des outils et l'incident exact est fermé. La porte complète repasse ses
23 scènes avec code 0 et ne laisse plus ce parasite. Il reste préférable de
rendre la preuve hermétique plutôt que de faire dépendre son confinement de l'arité :
cibler un chemin absolu dans le répertoire temporaire, reconnaître l'appel
exact de génération du manifeste et exécuter les lanceurs mutants avec un
`cwd` temporaire. Une postcondition doit enfin prouver que le répertoire de
lancement et le dépôt restent inchangés. Tant que cette scène n'est pas à la
fois causale et hermétique, ne pas compter son vert éventuel comme preuve de
fermeture de la publication.

## Réception critique du lot `8afd1057`

Le lot ferme bien la couture `PATH` dans sa frontière annoncée : outils
résolus hors `PATH`, chemins absolus, hashes gravés et relus, étiquettes de
schéma v5 cohérentes. La porte finale passe 23/23 normalement et 23/23 sous `python3 -O`,
et ses scènes `PATH` sont causales. C'est un vrai progrès ; il ne rend pas
encore le harnais admissible pour une nouvelle mesure.

Les mutations autonomes du WIP restent acceptées au pin : `liveness`,
`layout`, `coord`, `inflight_demande`, `pic_workers_b` et
`pic_reduce_actif` peuvent manquer ; les deux pics peuvent valoir zéro sous
`join=1`; `identite_cible` accepte un suffixe ; `parametres=` accepte une clé
inconnue ; l'agrégateur n'exige pas `lscpu` dans l'ensemble d'outils. Le
correctif utile reste donc un seul parseur de régime v5 à ensemble exact de
clés, comparé à l'argv, au META et à chaque sortie. Les lignes finales
`outils`, topologie, cpuset et affinité doivent aussi être relues depuis le
reçu, pas seulement comparées à des tableaux encore en mémoire.

Je corrige ici un mauvais compromis introduit pendant le WIP : retirer `--`
de l'appel unitaire à `sha256sum` fait passer le faux test, mais affaiblit la
primitive produit face à un chemin commençant par `-`. Il faut restaurer
`sha256sum -- "$1"` et réparer la contre-fixture : reconnaître exactement
l'appel de génération du manifeste, cibler une copie absolue dans le dossier
temporaire, lancer le mutant avec un `cwd` temporaire et vérifier en
postcondition que ni le dépôt ni le répertoire appelant n'ont changé. Le vert
ne doit pas dépendre d'une arité accidentelle.

Cette dette bloque seulement la réutilisation du harnais pour une campagne ;
elle ne retire rien au pin sémantique KeyCSR reçu séparément.

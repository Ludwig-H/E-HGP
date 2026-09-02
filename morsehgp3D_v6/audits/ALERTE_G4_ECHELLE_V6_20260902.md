# Préflight statique — profil G4 échelle v6

Date : 2 septembre 2026. Pins jugés : réponse documentaire `fec58e1f`, puis
capture moteur `9243d69f`. Le code encore décrit comme WIP est postérieur et
non attribuable à ces pins.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Audit strictement statique : GCP non utilisé, aucune cible externe interrogée
ou certifiée. Toute session éventuellement ouverte par Claude reste sa cible ;
elle n'est ni adoptée ni arrêtée ici.

## Verdict utile

Le **NO START reste actif**. `fec58e1f` ajoute seulement la réponse de Claude.
`9243d69f` livre utilement le curseur d'étage, la capture de `std::bad_alloc`
dans `run_pipeline`, l'invalidation interne et trois portes moteur non vacues,
mais ne modifie aucun profil ni fichier `gcp-migration/`. Le raccord de
campagne reste donc à faire.

La direction est bonne : timeout classé comme observation censurée, layout
`classic` annoncé, plan v2 pour les axes nouveaux, `RLIMIT_AS` ramené à
168 Gio et refus mémoire destiné à être typé. Le WIP postérieur porte déjà
`smax` dans le plan, le nom, l'argv et le statut, et aligne les inventaires sur
17 fichiers. Il faut maintenant fermer les raccords ci-dessous avant de
demander un GO.

## P1 — rendre le profil canonique effectivement exécutable

1. Le profil déclare `FRONTIER_LAYOUT=classic`, mais le lifecycle ne capture,
   valide, compare, grave ni transmet cet axe, et le validateur ne l'autorise
   pas parmi les axes canoniques. Le profil sera donc refusé comme contenant un
   axe inconnu ; si cette porte était contournée, le runner choisirait `classic`
   par son propre défaut, sans preuve de liaison au canon. Le selftest courant
   injecte directement `FRONTIER_LAYOUT` au runner et ne teste pas ce trajet.
2. Le profil renseigne deux `GPUV6_PILOT_SPECS`, mais omet
   `GPUV6_GATE_NAMES`. Son défaut `aucun` produit `gpuv6_plan runs=0` : aucun
   build, aucune porte et aucun pilote ne seraient exécutés. Refuser avant tout
   run `pilot_specs != aucun && gate_names == aucun`, puis inscrire l'inventaire
   exact des portes dans le profil.
3. La priorité annoncée est inversée : le profil promet la frontière Q1 en
   préfixe obligatoire et les pilotes Q2 en suffixe optionnel, alors que le
   runner exécute GPUV6 avant la frontière, toujours marquée « EN DERNIER ».
   Exécuter Q1 avant Q2, désarmer Q2 dans ce profil, ou scinder les sessions.

## P1 — joindre le refus mémoire au reçu

Au pin exact `9243d69f`, le moteur rend code 2 avec
`REFUS resource_exhausted : bad_alloc a l'etage ...`, tandis que le validateur
inchangé classe explicitement tout `bad_alloc` sous code 2 comme contradiction.
Le cas recherché invalide donc le reçu. De plus, `rr.message.reserve(256)` est
encore avant le `try` : son propre `bad_alloc` échappe à la promesse « jamais
un abort ». Les 188 portes rapportées prouvent le moteur, pas le trajet
CLI→runner→validateur.

Le WIP postérieur va dans le bon sens : réservation sous la garde, injection
pré-corps, texte `allocation impossible` et portée des callbacks resserrée.
Le renommage lexical ne suffit toutefois pas. Le validateur doit reconnaître
une sous-classe machine-readable, recouper sa cause, `refus_etage`, le message
et les RSS, puis la porte doit exercer le trajet complet. Si la doctrine est
désormais « un abort n'est pas une donnée », le code 134 ne peut plus rester
une classe de frontière valide comme aujourd'hui.

Il reste aussi un cas de vraie pénurie : si la réservation du message échoue
par manque persistant, l'assignation dans le `catch` peut échouer à son tour
et laisser un message vide, que le validateur refusera. Le mutant qui lève une
fois ne simule pas ce cas. Porter la cause et l'étage dans des champs sans
allocation, puis faire formater la ligne exacte par le CLI, donne un chemin de
secours fiable. La fabrication du nuage, située avant `run_pipeline`, reste par
ailleurs hors capture : borner la promesse au pipeline ou ajouter une garde
CLI explicite.

Enfin, conserver le contrat historique précis : les callbacks déjà appelés
sont **provisoires jusqu'au statut terminal** ; l'invalidation interne ne peut
pas reprendre un effet externe. La porte K=1 prouve zéro callback seulement
pour une panne antérieure au premier callback.

## Portée et budget à dire exactement

La grille WIP contient quatre points `uniform/K5`, trois `terrain/K5`, deux
`uniform/K10` et aucun `terrain/K10`. Elle ne mesure donc pas une pente sur
« quatre tailles par famille et par K ». Elle peut rapporter les sécantes
effectivement échantillonnées et le plus grand `n` **testé** qui complète sous
ce pin, ce layout et ce plafond. Sans bracket same-pin, ce n'est pas le plus
grand `n` tenant en mémoire.

Le lifecycle WIP estime 16 130 s : frontière 10 300, pilotes 3 000,
build/portes 2 700 et 13×10 s d'overhead. L'enveloppe obtenue en remplaçant
chaque estimation par son plafond vaut 18 900 s, ou 19 030 s avec cet overhead,
pas 19 010 s. Publier séparément **estimateur nominal** et **enveloppe de
plafonds**, de préférence depuis le même calcul que le journal du lifecycle.

Deux fermetures de protocole restent utiles :

- normaliser `fam:n` et `fam:n:11` avant le contrôle de doublons et avant
  l'émission de `specs=` ; ils ciblent aujourd'hui le même artefact mais le
  header conserve les octets différents ;
- soit graver le digest historique 50k dans le profil, soit limiter le claim à
  la parité CPU/device du run courant : `GPUV6_OBJET_DIGESTS=aucun` ne compare
  encore rien au reçu `1788293187`.

## Fermeture minimale avant nouvelle dépense

1. Raccorder `FRONTIER_LAYOUT` et l'inventaire GPUV6 sur tout le trajet
   canon→lifecycle→SSH→runner→plan/statut→validateur.
2. Mettre Q1 avant Q2 et joindre le refus typé du moteur au validateur.
3. Ajouter les contre-fixtures de layout re-hashé, pilotes muets, code 2
   `bad_alloc`, doublon normalisé et `:11` explicitement équivalent.
4. Requalifier la portée et le budget, puis rejouer les selftests locaux sur
   un commit d'implémentation propre.

Aucun résultat G4, aucun claim produit et aucun GO ne sont créés par ce
préflight.

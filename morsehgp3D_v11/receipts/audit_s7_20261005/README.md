# S7 — revue de la sortie supports, 5 octobre 2026

`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.

Source publiée : `966a351be1b58089fd13bc44a8dbafc56d87999a`.
Le commit `00bd979ac2c24aff422421525d484488dff0c993` modifie seulement
les délais de `tools/g4_matrix.json`. Les sources S7 relues sont identiques.
Le brouillon local `bd7c8e130` des rapports du développeur précède les
retouches et la publication ; ses résultats ne sont pas promus en
qualification du pin publié.

**Aucun défaut important établi.** La sortie `supports` comprend maintenant
le produit possédé, l'écrivain MHGP11SP, le manifeste, le lecteur et les
portes comparant le fichier complet à S1 et la signature d'arbre à FULL.
L'identité Session et les contrôles de provenance précèdent l'IO. Le produit
conserve l'arbre et la hiérarchie, détruite avant son arbre source.

## Vérifications et limites

- [Revue native et bornes de format](native/README.md) : lecture du code et
  calculs Python seulement ; aucun appel au binaire. Décalages du fichier
  et sommes du manifeste restent dans u64 sur le domaine préparé S6b.
- [Contre-épreuve mathématique du lecteur](math/REPORT.md) : périmètre,
  sources et rejeu borné ; huit fichiers S1, petits témoins K10/K12,
  signature V2 indépendante et prédicats confrontés au Gram exact,
  y compris à la borne u24. Normal et `-O` concordent ; aucun transfert
  au produit C++.
- Le lecteur recalcule la signature V2 à partir du fichier supports.
  Les portes natives comparent le fichier à S1 sur 963 ordres u21/u24,
  957 en u18, et la signature à FULL sur 810/804 ordres. Ces chiffres
  décrivent les portes inscrites ; cet audit ne les a pas exécutées.
- Le lecteur ne certifie pas la complétude géométrique de la coquille,
  absente du format. Le différentiel S1 du produit reste nécessaire.
- Le pilote `sorties_g4.py` contrôle les empreintes entre prises/workers
  et la signature commune FULL/supports. Il sépare les étages ; ses essais
  locaux ne qualifient pas les temps G4.

Les retouches déjà intégrées, dont le contrôle d'unicité des PointId,
ne sont pas de nouvelles alertes. Les rapports locaux du développeur
restent distincts d'un reçu G4 sur les sources assemblées. Aucun build,
test natif, benchmark ni GCP lancé par cet audit.

## Action utile avant la clôture G4 : invariance à W48

Les portes `mhgp11_cli_supports_scale*` et `*_lidar_*` fixent
`--fils=1,4` dans `tests/cli/tests.cmake` ; leur ligne CTest exige
`appels=12`. Le changement de délais `00bd979ac` ne change pas ces
arguments. La mesure `sorties_g4.py` à W1/W48 ne permute ni ne réétiquette
les entrées : elle ne complète pas cette couverture.

[Fragment de plan](w48_plan_fragment.json) à **fusionner dans la session
gardée L1/L2 déjà prévue**, après la matrice et avant clôture : trois
commandes ng00/ng01/ng02, sans nouvelle session. Le minimum utile est
ng02, qui exerce 354 coquilles étendues et huit boules à plusieurs supports,
ainsi que la boîte cosphérique ajoutée par la porte. Garder le build par
défaut u21 ; `v11_worker.sh` fournit déjà `MHGP11_DATA_DIR`.

Pour chaque trame, le script joue W1, W4, W48, une répétition W48,
une permutation W48, un réétiquetage W48 et FULL à W48, puis répète ces
contrôles sur sa boîte cosphérique. Il calcule et exige **14 appels**,
avec lecture complète du premier fichier, invariance des octets et
signature commune. Il faut appeler le script directement : la ligne
gravée des portes CTest actuelles attend 12 appels.

La [validation du fragment](w48_plan_validation.json) utilise uniquement
`v11_session.validate_plan` sur l'inventaire Git figé. Elle valide sa
structure, sans exécuter les commandes, sans lire les données et sans
contacter GCP. Les plafonds de 600 s par commande ne sont pas des durées
mesurées ; intégrer ce complément au budget de la session avant démarrage.

L1/L2 intégrés restent à qualifier : profils joués explicitement,
sanitizers, TSan, mutants, sorties complètes et coût payé. K10 réel reste
distinct des petits témoins exacts et des mesures K5. Aucun contrat de
100 ms, de points natifs ou de GPU n'est acquis par S7.

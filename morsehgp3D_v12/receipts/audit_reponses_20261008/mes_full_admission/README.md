# MES-FULL : admissions à fermer avant campagne

Rattachement à **CST-0018**, sans nouvel identifiant. La sonde et sa petite porte
de correction sont livrées en `c40318ebd` ; le pilote `mes_full/pilote_full.py`
est une capture **en cours, non commise**, SHA-256
`a89ceec92f76c642a508a0c76d5a0a8a31400cab15a7d27141bc37c557b9f92c`.
Les pins complets figurent dans `capture.json`. Aucune campagne exécutée n'est
invalidée par ce reçu et aucune performance n'est mesurée.

Trois admissions du pilote demandent une correction avant une déclaration de
contrat. Le lecteur appelle les fonctions Python existantes, sur des doubles
JSON synthétiques conformes aux noms de champs de l'émetteur, sans moteur.

1. **Preuve d'isolation absente.** `environment` rend `gpu_apps=None` si la
   commande échoue. La condition des lignes 281–282 traite cette valeur comme
   l'absence d'application. Le témoin double les commandes de ce helper avec
   un code d'échec : aucune commande système de diagnostic n'est exécutée.
2. **Mesure et attribution insuffisamment contrôlées.** `parse_process` admet
   un mur négatif, une étape P supérieure au mur, des métadonnées différentes
   de la commande (trame, profil 18, un fil), ou toutes les libérations omises.
   Les quatre flux restent admis et `frame_stats` puis `contract` rendent
   `tenu=True` pour ces **fragments**. Cela ne simule pas une campagne entière.
3. **Cohorte non exigée.** `unpack` admet un seul nom ou deux noms identiques.
   Les archives témoins contiennent seulement des métadonnées incomplètes et
   des emplacements de données vides ; elles ne sont jamais données au moteur
   et ne représentent pas une archive réelle valide. Le pilote ne demande ni
   les 37 noms distincts annoncés, ni l'unicité des noms tronqués transmis à la
   sonde. Les réponses sont ensuite attribuées par position sans vérifier le
   champ `trame` réellement demandé.

Le cas synthétique complet est aussi admis. Les contreflux partent de cette
base et altèrent seulement la propriété visée. Un fragment de contrat positif
sur un contreflux établit le manque de garde ; il ne prouve ni une mauvaise
géométrie du moteur, ni une réussite réelle de campagne sur des données fausses.

Correctifs proposés dans les fonctions existantes, sans créer un second pilote :

- Dans le contrôle d'environnement, hors mode `essai`, exiger que `gpu_apps`
  soit une **chaîne disponible et vide après strip** avant ET après ; `None`
  est un refus de preuve manquante. Exiger aussi la présence des diagnostics
  GPU/nvcc utilisés pour attribuer la machine et la construction. Ne pas
  confondre échec de commande et résultat vide. Conserver le mode `essai`
  explicitement séparé d'une qualification G4.
- Étendre `parse_process(code, text, passes, kmax, device, expected_names,
  threads, coord_bits)` ; les appelants fournissent leurs arguments effectifs.
  Pour `campaign_frames`, `expected_names=[f]`. Pour `campaign_sequence`,
  fournir exactement `[n[-23:] for n in order]`, dans l'ordre tournant choisi.
  À la passe `i`, exiger `trame == expected_names[i % len(expected_names)]`,
  profil 21, K et nombre de fils exacts, voie attendue, sites entier positif.
  Raccorder aussi le nombre de sites au manifeste fixé lorsqu'il est disponible,
  et contrôler sa stabilité entre passes/processus et CPU/appareil.
- Exiger l'ordre complet des lignes : ouverture appareil unique si demandée,
  puis `full(i), liberation(i)` pour chaque passe, puis sortie finale conforme.
  Vérifier les raisons/statuts, identifiants de passes, `liberation_ns` et toutes
  les métadonnées ; aucune phase supplémentaire silencieusement ignorée.
  Refuser proprement les objets ou champs manquants avant `frame_stats`.
- Pour chaque durée et pic, exiger `type(x) is int` et `0 <= x <= 2**64-1`,
  booléens exclus ; ne jamais remplacer une preuve absente par zéro. Demander
  les neuf clés `etapes_ns`, les six clés `c_ns`, les deux `g_ns`, validation et
  empreinte dans `hors_mur_ns`, et un SHA-256 hexadécimal de 64 caractères.
  Contrôler les emboîtements disjoints `P+C+G+raccord+TMVR <= wall_ns` et
  `T+M+V+R <= TMVR`. Les sous-diagnostics C ne doivent pas être additionnés à C
  pour reconstruire le mur. Refuser les clés JSON dupliquées et constantes
  non JSON plutôt que conserver silencieusement une occurrence.
- À l'admission du manifeste, avant extraction/mesure, exiger **37 noms uniques**
  dans le mode contractuel, des noms simples, la présence des deux fichiers
  attendus et l'unicité des 37 étiquettes effectivement transmises à la sonde
  (`n[-23:]`). Fixer le manifeste/cohorte dans le plan, et comparer son identité
  avant campagne ; le seul nombre 37 ne prouve pas l'identité des trames.
  Vérifier ensuite la couverture attendue processus × passes × trames avant
  toute statistique. Le mode `essai` peut garder sa réduction explicitement
  déclarée, jamais un verdict contractuel.

La porte livrée `full_probe_check.py` reste une **porte de correction sur deux
petits nuages**, pas le juge d'admission de campagne. Son helper `check_passes`
admet actuellement le mur/les étapes absents et des lignes de libération sans
numéro. Un témoin direct le confirme. Pour soutenir son propre descriptif,
elle devrait demander les champs présents, les 600/500 sites attendus et la
séquence de passes/libérations, tout en gardant son contrôle d'empreinte contre
`tower_chain`. Les gardes de campagne appartiennent au pilote ci-dessus.

Frontière de la sonde, note courte : `run_wall` couvre bien P, C, G, raccord,
T/M/V/R et conserve les objets jusqu'au terme. Mais `PassState` est alloué et
initialisé avant le début du mur ; l'inclure si « allocations comprises » couvre
cet état, ou expliciter l'exclusion. La ligne `open` ne mesure que
`CatalogueDevice::open` : le Pool existe déjà et aucune ouverture CPU n'est
publiée. La nommer comme ouverture appareil, ou mesurer la Session complète.
Aucun impact temporel chiffré n'est déduit de ces frontières. Les noms de trames
sont interpolés sans échappement JSON ; une restriction explicite aux étiquettes
sûres du plan, ou un véritable échappement, évite de fausser le flux.

Relecture depuis ce dossier :

```sh
python check.py --repo DEPOT
python -O check.py --repo DEPOT
```

Les sorties doivent égaler le champ `result` de `capture.json`. Les deux sources
livrées sont lues depuis Git au commit ; le pilote vivant est refusé s'il a
changé. Aucun octet de jeu réel, binaire HGP, compilation, GPU ou GCP n'est utilisé.
Les correctifs ci-dessus sont proposés, pas appliqués ni qualifiés dans ce reçu.
`public_status=not_claimed`.

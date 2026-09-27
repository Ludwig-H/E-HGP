# Lecture et publication du raccord résident G4

Ce lecteur prépare la publication d'une session **fermée**, portant sur
l'adaptateur q34/S2 résident seulement : une porte CUDA, puis ng00 entier
K5/s8 à W4 et W48. Il n'appelle ni GCP ni SSH et ne crée pas de résultat
expérimental. La session effective reste possédée par le responsable principal.

Port explicite de
[`b_q34_cuda_g4_readback_20260927`](../b_q34_cuda_g4_readback_20260927/README.md),
à `a7e80d7f9`, dans des fichiers neufs. SHA-256 source du lecteur :
`f51857bdd1c4f35bbdeae9ebcc4aaca0ea89faf625285320347fd0fc53f843e4` ;
du selftest : `c1e577df7eac5e5d6d2d7581bd4b5896af355a677ec715a69a7e9f1ea3225677`.
Le lecteur utilise le
[nouveau protocole résident gelé](../b_q34_resident_session_20260927/README.md),
pas l'ancien schéma CUDA. Aucun ancien fichier n'est modifié.

## Ce que la lecture vérifie

Le snapshot privé est reproduit depuis les objets du commit Git déclaré.
Manifestes, scripts exécutants, dépendances compilées, binaire, intentions,
commandes et sorties sont liés au même paquet. Les recettes doivent être
celles du nouveau worker : porte CUDA avant W4, puis W48, Q/Qr262144.
Le validateur importe directement le contrat réel de la sonde ; aucun
nombre de cas artificiel n'est figé comme résultat du gate.

Avant toute publication, la génération démarrée doit être celle dont
l'arrêt est certifié par la garde versionnée. La lecture GCE finale doit
annoncer `TERMINATED` sur cette même instance, zone, projet et
`lastStartTimestamp`, avec une chronologie cohérente. Une génération
inconnue ne justifie jamais un arrêt non ciblé. Une vraie absence de nouveau
cycle reste distincte d'une expérience réussie.

Les deux largeurs CPU doivent avoir produit les masses, masques,
survivantes ordonnées et digest ng00 attendus. Un W4 réussi suivi d'un
W48 échoué reste **failed** : les streams partiels sont conservés, mais
aucun temps isolé ne requalifie cette campagne. Une commande terminée
avec code zéro ne remplace pas le verdict sémantique du nouveau protocole.

## Publication limitée et origines privées

Les noms de fichiers VM sont limités aux recettes et pièces explicitement
autorisées. Les noms de commandes hôte sont eux aussi limités au helper
gardé. Sont interdits à l'export : clés SSH privées/publiques, archives,
payloads LiDAR, réponses OS Login brutes et état GCE complet. L'état GCE
publié est une projection explicite sans métadonnées ni interfaces réseau.

Les logs de garde sélectionnés sont caviardés pour adresses IPv4/IPv6,
comptes courriel et corps de clés publiques. Une clé privée est toujours
refusée. Un identifiant sensible apparaissant ailleurs fait échouer
l'export, plutôt que modifier silencieusement un JSON ou son hash.
Les chemins privés et hashes nécessaires à la relecture restent dans
`PRIVATE_LINKS.json`, sans contenu des clés ou archives.

La relecture est **LIVE** : le snapshot, le paquet, les originaux de
session et les sources du lecteur restent requis. Chaque octet public
est recalculé depuis ces origines et comparé, au-delà de l'inventaire
SHA-256. Réécrire `SHA256SUMS` après falsification ne suffit pas.
L'absence des origines privées est une absence de preuve LIVE, pas une
qualification archive autonome. Le lecteur n'est pas une signature
cryptographique indépendante du dépositaire de toutes ces preuves.

## Périmètres de temps et compteurs

Le résumé donne, pour chaque largeur, `adapter` et `front+adapter`.
L'adaptateur paie réellement initialisation CUDA, allocations/transferts,
filtre rectangle, compaction, arène CPU, vagues GPU, retour des survivantes,
tri/conversion et fermeture des propriétaires Prepared/Session/Decision.
Les sorties S **et les R masques rectangles** sont encore possédées après
cet intervalle. Le contexte CUDA global n'est pas détruit par l'adaptateur.
Aucune estimation « warm » par soustraction de l'initialisation.

Lecture, index et front CPU sont des phases amont du même commit source,
publiées séparément, hors `adapter`. Le front reste W1 ; W4/W48 concerne
l'arène, et l'oracle natif reste W4. `total` paie en plus l'oracle, la
comparaison et les destructions finales : ce n'est pas un coût candidat.
Les sous-phases emboîtées ne doivent pas être additionnées deux fois.

`portable_runs` et `cuda_runs` comptent les appels `one()` du corpus,
pas tous les appels device. Les comptes de requêtes/survivantes et ceux
accumulés dans `one()` agrègent les deux backends. `pool_lane_reduced`
est calculé par l'oracle une fois par cas. Le contrôle de propriété
`ownership_gate` ajoute un appel valide hors de ces comptes de runs/travail ;
ses refus contribuent toutefois à `rejections`.

Une mesure W4 puis W48 ne prouve pas un gain stable. Le résumé garde
`GPU_baseline_comparison=not_acquired` : aucune division des nouveaux
temps par les anciens, dont le périmètre est différent. Ni FULL, ni
100 ms, ni borne sous-quadratique, ni résultat multi-scènes n'en découle.
Le temps d'allocation GCE est publié, sans inventer de facture ni tarif.

## Tests hors cloud

`selftest.py` construit des archives et reçus synthétiques temporaires.
Il interdit tous les sous-processus réels ; le helper et le protocole
réels sont utilisés, mais les accès Git sont simulés. Il vérifie succès,
échec W48 après W4, largeurs/digests invalides, perte des origines privées,
falsifications même repinnées, allowlists, secrets et mauvaise génération.
Ces données ne sont jamais des chronos G4.

Capture [checks/r1/receipt.json](checks/r1/receipt.json) close : deux
commandes normal/−O identiques, **17 contrôles positifs et 21 refus**, plus
5 contrôles positifs/44 refus des fixtures de protocole réutilisées.
Lecteurs vivants normal/−O : PASS, sources avant/après identiques.
Aucune donnée expérimentale ni opération GCP dans cette capture.

```sh
python3 -B morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/qualify.py morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/checks/r1
python3 -B morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/qualify.py morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/checks/r1 --readback
python3 -B -O morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/qualify.py morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/checks/r1 --readback
```

Le lanceur de qualification démarre seulement ces deux processus Python
de tests, normal/−O. Il enregistre commandes, streams et sources avant/après.

## Après réception d'une session réellement fermée

Le responsable fournit les chemins privés de session et de paquet, ainsi
qu'une destination de reçu neuve. Aucun chemin de clé SSH ne doit être
lu. Le commit expérimental vient du paquet, pas d'une valeur inventée ici.

```sh
python3 -B morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/readback.py --export RECU_NEUF --session SESSION_FERMEE --package PAQUET_PRIVE
python3 -B morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/readback.py --readback RECU_NEUF
python3 -B -O morsehgp3D_v9/audits/b_q34_resident_g4_readback_20260927/readback.py --readback RECU_NEUF
```

Ces commandes d'export réel ne sont pas exécutées par la qualification
locale. Elles seront utilisées seulement après la fermeture de session.

## Première session réelle lue

Après confirmation de l'arrêt par le responsable principal, le lecteur
gelé a publié
[q34_resident_g4_20260927/r1](../../receipts/q34_resident_g4_20260927/r1/README.md).
Export puis relectures LIVE normal/−O : **PASS**. Source expérimentale
`af369c44efa75236fa98e25e8f1bc4708b128fc4`, 97 pièces publiques autorisées,
allocation observée de 191,901 s sur la même génération arrêtée. Aucun
appel GCP effectué par le lecteur, aucune clé/archive/donnée LiDAR exportée.

Adaptateur froid W4/W48 : **627,503 / 504,328 ms** ; front CPU + adaptateur :
**2 738,875 / 2 616,350 ms**. Identité native P/E/S et digest conservée.
Le détail et les périmètres sont dans le reçu, qui reste immuable.
Ces premiers chiffres ne constituent ni FULL ni une comparaison qualifiée
avec le GPU précédent.

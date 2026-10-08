# Session K : provenance locale et clôture

Audit du 8 octobre 2026, après rapatriement. Aucun lancement de moteur, aucune
commande GCP, aucun contact avec le contrôleur ; lecture des preuves déjà présentes.
Complément de [la capture avant résultats](../session_k_snapshot/README.md), sans
réécriture de celle-ci. L'admission des bruts FULL et D6 est un travail distinct.

## Ce qui est attesté

Première archive observée à **03:02:12 UTC**. Archive stable avant/après lecture :
229 876 octets, SHA256 `5f8d64c1fb9dc2d49b98ae96b759b473533e29d13149a2a7513cfa4c346435da`.
221 membres, 207 fichiers ordinaires, aucun doublon ni lien ; **206/206 fichiers**
référencés conformes au manifeste, sans membre omis. Les répertoires `build/` et
`provenance/` ne contiennent aucun fichier. Le plan rapatrié est identique au plan
emballé. Aucun flux tronqué ni résultat évincé n'est déclaré.

| Commande externe | Statut/code | Groupe clos | Mur du pilote |
| --- | --- | --- | ---: |
| `000_mes_full` | `ok` / 0 | oui, sans processus résiduel tué | 1 360,927 s |
| `001_mes_d6_profils` | `failed` / 1 | oui, sans processus résiduel tué | 310,701 s |

Ces murs comprennent les campagnes et leurs constructions ; **ce ne sont pas des
latences de trame**. FULL est passé avec W48, 5 processus, 10 passes, 44 travaux de
compilation, délai de sonde 900 s et archive v12set. D6 demande ng00–02, profils
21/24/32, K5, W48, 5 passes, 3 tours ; délai externe effectif 1 095 s, sans expiration
observée. Le code 0 FULL signifie que son pilote a rendu un rapport ; il ne prouve
ni le contrat ni la validité du lecteur permissif. Le code 1 D6 n'invalide pas les
bruts FULL. Les deux stderr externes sont vides, sans implication sur les stderr
internes que les pilotes n'archivent pas systématiquement.

Le reçu final porte `closure=stopped`, `targeted_shutdown_certified=true`, une
tentative et code d'arrêt 0. La description finale confirme **TERMINATED**, même
cible et même génération que la fermeture, arrêt à **03:03:03.040 UTC**. Les
commandes locales archivées d'arrêt ciblé et de vérification rendent 0. Le statut
global `failed_remote` correspond à D6 ; il ne signifie pas un défaut d'arrêt.
Le fichier lifecycle encore `targeted_running` lors de l'arrivée du reçu n'était
donc pas une autorité supérieure à ces preuves de clôture.

## Source, construction et limites

La source reste **worktree_snapshot / dev_snapshot / not_claimed**, tête
`c9ac60f20c741d9d493f4900fd8aec4590aaa5c5`, 34 entrées d'état et 2 262 fichiers.
Le paquet, son manifeste et le plan sont épinglés dans `capture.json` ; le lecteur
revérifie les **2 262 contenus source**. Les quatre corps FULL/D6 déjà capturés sont
inchangés ; le D6 embarqué précède la correction `e37fd8935`.

Le pilote FULL épinglé demande Release/u21/CUDA ON et cible `mhgp12_full_probe` ;
D6 construit ses sondes par profil. Ces compilations sont internes aux pilotes,
pas au constructeur générique du worker (`worker_build=not_requested`). **Aucun
hash de binaire, CMakeCache, journal de construction ni identité de compilateur du
build n'est rapatrié** : `provenance={binaries_sha256:{},cmakecache:[],compiler:[]}`.
Les informations générales de compilateur/outil dans `env/` ne remplacent pas ces
preuves. Le paquet source prouve ce qui a été expédié, pas à lui seul l'identité du
binaire exécuté. Ne pas transformer cette session en qualification d'un commit.

L'isolation GPU doit être admise depuis les champs avant/après du rapport FULL et
les bruts attendus ; les codes externes seuls ne l'établissent pas. Les fichiers
de données sont déclarés conformes par le contrôle distant archivé, lié au
manifeste épinglé. Aucune coordonnée n'est lue ou copiée par cet audit. Aucun
résultat de temps ni contrat FULL n'est qualifié dans ce reçu.

## Relecture

```sh
python check.py --session-dir /chemin/local/session_K
python -O check.py --session-dir /chemin/local/session_K
```

Le dossier local doit contenir `receipt.json`, `preflight.json`, `package/`,
`results/` et les quatre journaux de fermeture. Le lecteur ne publie ni identité
de compte ni cible d'infrastructure ; il les compare seulement en mémoire. Les
hashes locaux, le manifeste des résultats et celui du paquet sont contrôlés
avant/après. Ce contrôle ne requalifie aucun moteur et ne contacte aucun service.

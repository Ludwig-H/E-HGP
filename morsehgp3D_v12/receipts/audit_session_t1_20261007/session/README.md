# Audit ciblé du port G4 et de la publication — 7 octobre 2026

Épingle : `4147c546000b198b5239646063bfb1e3ed6d28fc`. Lecture comparative de la v11,
contrôleur et worker v12, cache de données et fabricant des reçus publiables.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference` pour ces témoins,
`public_status=not_claimed`. Aucun appel GCP, GPU ou réseau ; aucune reprise réelle
de session. Les fichiers produit et `audits/` ne sont pas modifiés.

## Constats reproductibles

### CST0219 — Identité dans les noms de fichiers publiés — majeure, outillage

`microbancs/outils/recu_session.py:89–100` conserve le chemin relatif. Le contrôle
`:105–115` cherche les identités uniquement dans les contenus ; `:120–121` écrit
ensuite ces chemins dans `SHA256SUMS`, après le contrôle.

Le témoin crée un résultat nommé `audit@example.invalid.log`, au contenu anonyme,
et le compte synthétique correspondant dans le préflight. **Code 0**, fichier
publié sous ce nom, adresse présente dans `SHA256SUMS`. Le contenu de `receipt.json`
est bien expurgé : le défaut porte sur les noms et le manifeste final. Aucune fuite
d'identité de la session historique n'est déduite de ce témoin.

Correction attendue : contrôler les chemins relatifs et le manifeste final aussi,
ou renommer explicitement les chemins avec détection des collisions ; ne publier
qu'après contrôle de l'arborescence complète.

### CST0220 — L'espace évictable surestimé avec les liens durs — moyenne, cache

`bench/data_cache.py:266–279` promet de conserver le cache si l'éviction complète
ne suffit pas, puis ajoute la taille de tous les objets évictables à l'espace libre.
Pourtant `link_into` crée des liens durs vers ces objets (`:492`). Enlever seulement
le nom du cache ne libère pas les blocs d'un inode encore référencé dans un build.

Le témoin emploie **un vrai lien dur de six octets**, obtenu par `link_into`, et
modélise seulement l'espace libre à zéro pour ne jamais remplir le disque.
`make_room(1, ...)` supprime l'objet du cache, puis refuse ; les six octets restent
vivants sous le lien du build. L'éviction irréalisable a donc détruit une entrée du
cache, contre le contrat annoncé. Elle ne supprime pas le fichier du build et ne
constitue pas une corruption des données épinglées.

Correction attendue : distinguer volume logique retiré du cache et espace disque
réellement récupérable ; au minimum, ne compter aucun gain disque pour un objet
avec plusieurs liens. Réviser aussi le contrat conservateur en présence de fichiers
encore ouverts et de changements d'espace libre extérieurs au cache.

### CST0221 — Un refus efface un `--dest` préexistant — majeure, outillage

`microbancs/outils/recu_session.py:76` accepte un répertoire déjà présent, `:83`
écrase son reçu, puis `:105–117` inspecte tous ses fichiers et supprime tout le
répertoire si une identité reste. Aucun contrôle d'appartenance à cette exécution.

Le témoin place un fichier binaire antérieur contenant `/home/exemple` dans une
destination temporaire. **Code 3 et répertoire entier disparu**, alors que le
fichier préexistait à l'appel et qu'aucun résultat n'avait été demandé par
`--include`. Seul le répertoire temporaire du témoin a été touché.

Correction attendue : refuser une destination existante avant toute écriture,
ou construire dans une zone temporaire possédée par l'appel puis publier de façon
atomique. Un refus doit préserver tous les octets préexistants.

## Deux observations secondaires déjà reproduites

- `bench/data_cache.py:587–595` vérifie une destination lexicale et seulement le
  dernier composant pour les liens symboliques. Un ancêtre symbolique vers
  `results/` permet un `--get --link` de six octets sous les résultats, **code 0**,
  malgré le refus documenté. Aucun téléchargement : objet épinglé déjà présent,
  accès réseau interdit par le témoin. Le contrôle de destination est contourné ;
  aucune publication historique de données par cette voie n'est établie.
- `fetch_with_retries`, `bench/data_cache.py:447–458`, contrôle l'échéance seulement
  quand un délai de relance est non nul. Horloge simulée à 20, échéance à 10,
  zéro relance : le premier `fetch` est tout de même appelé. L'aide promet « aucun
  nouvel essai » après le délai. Cela concerne la limite du cache : **l'échéance
  externe du worker et les gardes d'arrêt de la VM ne sont pas désactivées**.

## Contrôles positifs et portée

Onze fonctions critiques du contrôleur ont des AST identiques à la v11 après
renommage v11/v12 : cible enregistrée, génération, décision et fermeture, observation,
processus détenteurs, verrou, environnement de commande, extraction et manifeste.
Les différences non mécaniques du contrôleur concernent l'expurgation du `$HOME`
et le relevé des fichiers contenant encore une identité. Le verrou commun reste
`.ehgp-v10.lock` ; c'est un verrou local à la même racine de sessions, pas un verrou
distribué. Les deux scripts gardes gardent leurs empreintes épinglées.

`v12_target_selftest.py` passe **70 contrôles explicites sous `-O`**, avec appels
cloud doublés : cible contradictoire, gardes, compte figé, transport de cible,
reprise et verrou tenu notamment. `bash -n` passe pour worker/start/stop. Le gros
selftest n'est pas exécuté ; aucun nouveau scénario réel d'interruption ou course
GCP n'est qualifié. L'identité d'AST est une preuve du port, pas une nouvelle
validation exhaustive des comportements hérités.

Lecture historique de `v12.20261007.t0a` : reçu `completed`, une tentative d'arrêt
code 0, même génération dans le passage et les observations avant/après,
relecture finale `TERMINATED`, clés OS Login/privée supprimées selon les traces.
Les quatre empreintes du protocole dans le préflight correspondent aux sources
auditées, les gardes archivées aussi. L'archive de résultats est hachée et égale à
`44594dc6f7e9b64b74693c9182f690b2a3eef2dfa4990b25deeeebe4ca26fa73`.
Ce sont des traces existantes, **pas une observation actuelle de la VM**.
La source produit historique était un `worktree_snapshot` sur HEAD `26b53648c` ;
son autorité est le paquet `a5d1ff2383b230eb4b89e7871335d5b0db8437ee86268ccca69ac0fbbfb6213c`,
pas la seule valeur du HEAD. Ni chronos ni qualification FULL transférés.

## Rejouer

```sh
python3 -O morsehgp3D_v12/receipts/audit_session_t1_20261007/session/run.py
```

Pour recontrôler aussi les petits fichiers historiques locaux, ajouter
`--session /chemin/vers/v12.20261007.t0a`. Leur absence ne bloque pas les témoins
synthétiques. Aucun fichier brut personnel n'est copié dans ce reçu.

`MANIFEST.json` ferme 13 sources contre le commit, le témoin et les sept fichiers
historiques effectivement lus. Le script exige les hashes des sources et de son
propre code avant/après, contrôle un HEAD stable et utilise des exceptions explicites
plutôt que des `assert`. La capture `RESULT.json` comporte 81 contrôles de ce lecteur,
auxquels s'ajoutent les 70 contrôles du test ciblé. Tous les témoins restent minuscules ;
aucun build natif, pleine matrice, gros cache, LiDAR ou campagne GPU.

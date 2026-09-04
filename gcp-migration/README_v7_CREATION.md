# Création G4 v7 bornée, sans calcul

`create_v7_g4.py` crée au plus une nouvelle `g4-standard-48`, dans le projet
`devpod-gpu-exploration`, en zone standard explicitement choisie. Le nom et
le label d'appartenance sont générés avec un nonce aléatoire. SPOT, STOP,
durée GCE 3600 secondes, maintenance TERMINATE et absence de redémarrage
automatique sont imposés. Aucun appel `instances start` n'est effectué.

La création GCE démarre nécessairement l'instance. Sa métadonnée de démarrage
contient donc dès la requête le texte exact de la garde invitée du dépôt,
obtenu par `start_and_verify.sh --print-guest-guard-script`. Il programme
45 minutes et refuse de dépasser l'instant d'intention de création plus
3300 secondes, conservateur par rapport à la garde GCE de 3600 secondes.
Une erreur de cette garde demande une extinction immédiate. Aucun benchmark,
installation, mise à jour du pilote ou reboot n'est exécuté par le créateur.

Cette garde de naissance est **création-only**. Après son succès seulement,
le script écrit `/var/lib/ehgp-v7-create/NONCE`, sous répertoire root 0700 et
fichier root 0600. Un démarrage suivant saute l'ancienne échéance absolue
uniquement si type, propriétaire, permissions et contenu de cette marque
correspondent ; une marque incohérente refuse le démarrage. SSH vérifie
la marque et le shutdown programmé, puis GCE recertifie la même génération.
Toutes les vraies sessions ultérieures passent obligatoirement par
`start_and_verify.sh`, qui arme une nouvelle échéance invitée.

Le créateur rend uniquement une VM **TERMINATED**, après appel versionné
de `stop_and_verify.sh` et relecture du même nom/projet/zone, nonce, id GCE
et `lastStartTimestamp`. Le nettoyage ne dépend pas des écritures de journal.
Une cible étrangère ou une autre génération n'est jamais arrêtée.
Un échec de certification rend le code 74 avec commande de contrôle exacte.

Un retour CLI ou un listing vide ne prouve pas l'absence d'une création
tardive. La requête est asynchrone et son opération `insert` est suivie
jusqu'à DONE, liée à l'instant d'intention et au lien exact de la cible.
Si la réponse initiale est perdue, une recherche strictement ciblée doit
retrouver une unique opération. Seuls DONE avec erreur **et** cible absente
permettent `no_session_created=true`. Une opération inconnue ou en cours
reste bloquante ; aucune nouvelle tentative ne doit masquer cet état.

Préconditions : SDK avec composant beta déjà installé explicitement,
`jq`, clé ED25519 privée 0600 et publique cohérente, inscription OS Login
unique avec 3600 à 4260 secondes restantes. Le contrôle de quotas existant
est exécuté sans bypass : quota RTX Spot exact, GPU global, Hyperdisk,
instance/adresse ; consommation GPU globale nulle. Le type et l'image
sont relus avant création. Le répertoire de reçu doit être absolu et nouveau.

Tests locaux (toutes les opérations cloud sont remplacées) :

```bash
python3 -B gcp-migration/selftest_create_v7.py
python3 -B -O gcp-migration/selftest_create_v7.py
```

Après audit indépendant des octets et autorisation SPOT applicable,
l'opérateur peut invoquer, **une seule cible à la fois** :

```bash
python3 gcp-migration/create_v7_g4.py --yes --zone us-central1-b --receipt-dir /CHEMIN/PRIVE/creation-NOUVELLE --ssh-key /CHEMIN/PRIVE/cle-session
```

La commande est un exemple, pas une preuve de capacité régionale disponible.
Le reçu JSON donne le nom généré pour la session v7 ultérieure. Le disque
de démarrage de 100 Go est conservé, reste facturable et n'est pas supprimé
automatiquement. Le résultat reste `public_status=not_claimed` ; aucun contrat
de performance ou d'exactitude du moteur n'est qualifié par ce lanceur.

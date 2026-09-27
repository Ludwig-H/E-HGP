# Nettoyage du volume de travail — 27 septembre 2026

Demande utilisateur : libérer de la place, éventuellement en retirant les
anciennes versions v2 à v5 si elles ne contiennent plus rien d'important.
L'opération vise le volume `/workspaces`, pas une réduction de la somme
des octets présents sur tous les disques de la machine.

## Périmètre retenu

Les sources v2 à v5 sont **conservées** : environ 18 Mio dans le dépôt
principal, avec des références mathématiques et des dépendances de lecteurs
encore utilisées. Le gain utile vient des anciens snapshots de `build/`.
Le dépôt principal sale, les worktrees `v9-open-worktree` et
`v9-audit-c-publish`, les builds et captures du clustering courant ne sont
pas nettoyés. Les huit cibles n'intersectent pas les 1 175 chemins épinglés
du dernier contre-audit du pilote pondéré.

Cibles exactes, sous `/workspaces/E-HGP/build/` :

- `realflow` ;
- `realflow2` ;
- `v8_index_prepared_bounds_20260913.qBZj6V` ;
- `v8_index_census_20260913.5OH9JA` ;
- `v8_index_additive_20260913.KTAC7N` ;
- `v8_index_shared_20260913.jRoeEH` ;
- `v7_rank_guard_staged.9B0dIH` ;
- `v7_birth_staged.AR3MkV`.

Chaque ancien chemin devient un lien vers une copie consultable sur `/tmp`.
Une archive compressée **persistante sur le volume du projet** et son
manifeste précèdent le retrait de la seule copie redondante. Les octets,
tailles, types, modes et ACL POSIX access/default sont vérifiés ; aucun
worktree enregistré, lien symbolique interne ou fichier spécial n'est admis.
L'absence de processus utilisateur de ces chemins est contrôlée avant la
bascule. La source est revérifiée avant retrait, puis la copie accessible
après retrait. Le nettoyage ne supprime donc pas l'unique exemplaire d'une
source ou d'une preuve.

## Premier essai arrêté avant toute suppression

R1 a refusé la première copie : les 48 915 fichiers de `realflow` avaient
des tailles et SHA256 identiques, mais des modes différents. L'ACL par
défaut héritée de `/tmp` expliquait exactement les bits absents. Aucun des
huit originaux n'avait été déplacé ni retiré par R1.

L'échec reste dans
`/workspaces/E-HGP/build/cold-archives-20260927/receipt.json`,
SHA256 `751634661eef210a696ecae4211ae58ea04463ef30be2cd3a65b7c0d5900a18d`.
Le tar et le déploiement R1 restent historiques ; ils ne remplacent pas la
reprise R2. La raison précise pour laquelle `--same-permissions` seul
n'avait pas restauré les bits n'est pas assimilée à un défaut démontré de tar.

## Reprise R2 et récupération

R2 archive et extrait avec `tar --acls`, capture les deux ACL dans chaque
entrée du manifeste, puis réapplique si nécessaire les métadonnées exactes
**uniquement à la copie privée**. L'égalité complète n'est pas affaiblie.
Le script est conservé dans le dossier d'archives R2, SHA256
`8d6d557124e55f9e90b0684de96509f7d18c501cacd3009ef4cbc6007e74369b`.

- Archives, manifestes et reçus individuels :
  `/workspaces/E-HGP/build/cold-archives-20260927-r2/`.
- Commandes, sorties, mesures disque avant/après et reçu externe :
  `/workspaces/E-HGP/build/cold-relocation-run-20260927-r2/`.
- Copies consultables :
  `/tmp/mhgp9-cold-20260927-r2-69jeu9bl/`.

La capsule de restauration est **le tar et son manifeste**, avec leurs SHA
dans le reçu, pas seulement le lien temporaire. Après disparition de `/tmp`,
extraire le tar avec `--acls` dans un nouveau répertoire privé ; restaurer
les modes puis les ACL du manifeste si nécessaire et comparer l'inventaire
complet avant de remettre le chemin d'origine en service. La fonction
`restore_private_metadata` du script conservé applique cette étape dans un
déploiement temporaire neuf portant le préfixe R2. Ne pas extraire par-dessus
un répertoire existant contenant du travail non sauvegardé.

Les liens conservent l'accès ordinaire aux fichiers mais changent le résultat
de `Path.resolve()`. Les lecteurs historiques qui exigent un chemin canonique
identique peuvent nécessiter la restauration d'un vrai répertoire au chemin
original. Ce nettoyage ne prétend pas requalifier ces anciens lecteurs.
Les copies `/tmp` ne sont pas l'unique moyen de récupération.

## Clôture

R2 est **close, 8/8 cibles réussies**, commande jointe avec code 0. Le
contrôle externe final revérifie les SHA des archives et des manifestes,
les huit liens et l'absence des huit copies redondantes retirées. Il ne
refait pas une campagne mathématique ni une qualification des anciens builds.

Les snapshots occupaient 5,48 Gio alloués ; leurs 292 769 fichiers
représentent 5 097 167 460 octets logiques. Les huit archives compressées
font 1 255 732 360 octets (1,17 Gio), hors manifestes et journaux. Le dossier
complet d'archives R2 occupe environ 1,28 Gio. Les preuves de l'échec R1
sont également conservées et sont donc incluses dans le coût final.

Mesures d'espace disponible sur `/workspaces` prises dans les reçus externes :

| Mesure | Octets | Gio |
|---|---:|---:|
| Avant le premier essai R1 | 2 737 758 208 | 2,55 |
| Après clôture R2 | 7 079 591 936 | 6,59 |
| Gain net, R1 conservé | 4 341 833 728 | **4,04** |

Le volume passe d'environ 96 % à 89 % d'occupation. La seule reprise R2
libère 4 512 743 424 octets ; ce n'est pas le gain net depuis le début.
Les petites écritures de documentation postérieures peuvent faire varier
les derniers blocs disponibles. `/tmp` conserve les copies déployées : il
s'agit bien d'un allègement du volume de travail, pas d'une baisse annoncée
de l'occupation totale de tous les disques.

Reçu des huit déplacements :
`/workspaces/E-HGP/build/cold-archives-20260927-r2/receipt.json`,
SHA256 `a0fb77fe37c006b3654c37917fd235f232d44d38d0c433f4917f2bd606df6843`.
Reçu externe des commandes et contrôles :
`/workspaces/E-HGP/build/cold-relocation-run-20260927-r2/receipt.json`,
SHA256 `af184dcadd36d25d1d5912285777b21d32e60e96ff520e74afc8aa166a3e97f5`.
Tous les processus de cette opération sont joints. Aucun résultat
mathématique ou chrono du moteur n'est modifié par ce nettoyage.

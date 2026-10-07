# Contre-lecture des corrections d’outillage

7 octobre 2026, pin `274592a30f6961cb7702125dcd2f031ff22b7df2`. Audit local ciblé,
`public_status=not_claimed`. Aucun GCP, aucune donnée réelle, aucun rejeu de matrice native,
de sanitizer ou de stress. Les sources versionnées de la v12 sont comparées au pin avant
et après les essais ; le résultat publie l’identifiant de l’arbre Git et l’empreinte SHA-256
du manifeste des contenus contrôlés. Les binaires temporaires du petit lecteur sont retirés.

## Corrections confirmées

| Constat | Contre-épreuve | Conclusion bornée |
| --- | --- | --- |
| `CST-0225` | [format.py](format.py), vrai `Reader` compilé en C++20 Release : huit fichiers synthétiques, deux admis et six refusés ; le fichier de 88 octets annonçant 2^62−1 éléments est refusé | fermeture du débordement d’avance ; aucune lecture de payload absent, aucun catalogue réel rejoué |
| `CST-0224` | [publication.py](publication.py), 21 appels du vrai `main` : cas valides, anonymisation, collisions plates/imbriquées/fichier-dossier, destinations absentes/vides/occupées ; six pannes d’écriture après dépôt de contenu partiel | collisions refusées code 3 avant écriture ; interruption sur reçu, résultat ou manifeste nettoyée code 3 ; destinations antérieures occupées préservées code 2 |
| `CST-0226` | `PROVENANCE.md` distingue maintenant le développement numérique `6a38f7e4b`, u32 admis, et la table historique du port `a0091e2b7` corrigée `95247cf4b` | la table ne prétend plus classer les fichiers actuels ; aucune qualification numérique héritée de cette correction documentaire |
| `CST-0003` | porte `test_gate_properties.py`, 69 contrôles avec le vrai CTest sur projet factice sans compilation | absence de données classée `Skipped`, présence jouée ; jeton de saut forgé classé échec |
| `CST-0004` | porte `test_g4_matrix.py`, 39 contrôles | commande construite avec `--output-on-failure`, option réservée ; appels externes simulés, aucune matrice G4 exécutée |
| `CST-0014` | porte `test_check_style.py`, 145 contrôles, puis style de l’arbre réel conforme | règle `[recus]` sur les mentions CMake hors commentaires ; ce contrôle textuel n’est pas un confinement général des accès aux fichiers |

Les deux premiers scripts adaptent explicitement les témoins de
[l’audit u32](../../audit_u32_20261007/README.md) : nouveau pin, attentes devenues refus,
trois troncatures/frontières supplémentaires du lecteur et six pannes d’écriture injectées.
Les archives précédentes restent inchangées. Pour ces pannes seulement, la fonction `open`
du module publié est remplacée : elle crée quelques octets puis lève `OSError`. Sélection,
publication et nettoyage restent les vrais chemins du programme ; les entrées sont hachées
avant/après. Les publications conformes ont un manifeste exact et ne gardent pas l’adresse
synthétique `audit@example.invalid`.

## CST-0024 : cause et correction corroborées, portée mono-fil

[evidence.py](evidence.py) relit les traces existantes du développeur sans lancer de binaire.
Les 34 fichiers du manifeste figé correspondent à `c764e121a`. Ce commit précède le correctif
du pool `8e3b76245` : l’ancien pool réutilise des champs de travail partagés qu’un ouvrier
retardataire peut lire sans synchronisation. Le passage à un fil supprime ce chemin concurrent.

Les trois stderr TSan multithreads contiennent respectivement **2, 0 et 1** avertissements de
course dans `Pool::run_chunks`, lignes 32–33, avec écriture par `Pool::parallel_for`. Le BuildId
dans les rapports correspond au binaire TSan disponible ; les options de compilation portent
`-fsanitize=thread`. Les cinq stderr mono-fil sont vides ; un dump mono-fil égale celui du
cas multithread sans avertissement. Le script corrigé disponible est identique à la porte du
pin (`THREADS = (1,)`). Les **45** lignes du bilan corrigé sont de code 0 et chaque sortie porte
le verdict complet attendu : 190 nuages, 190 catalogues, 640 tours, 26 530 lignes.

Le préfixe des six premiers tours du bilan antérieur confirme les **18** prises du reçu
développeur : 16 codes 0, un écart et une expiration. Le fichier local disponible contient
aussi une continuation : **27** lignes au total, soit 20 codes 0, trois codes 1, un code 124 et
trois codes 143. Les codes 143 sont séparés des écarts/expirations ; leur cause n’est pas
déduite du seul bilan. Trois sorties supplémentaires n’ont pas de ligne de bilan : leurs
codes de sortie demeurent inconnus. Ces ajouts ne sont pas effacés ni rétroactivement inclus
dans les 18 prises décrites. Les durées du bilan sont vides : aucun chrono n’en est repris.

Ces lectures corroborent la fermeture de la fragilité de cette porte **à un fil**. Elles
ne requalifient ni le parallélisme figé v10, ni les reçus v11, ni les matrices v12. Les hashes
des journaux et binaires sont ceux observés maintenant ; une attestation source→build faite
avant les exécutions n’est pas reconstruite. Aucune nouvelle exécution TSan/stress n’est alléguée.
Les chemins privés et les journaux bruts ne sont pas copiés ; seuls leurs noms relatifs et
empreintes figurent dans [evidence.json](evidence.json).

## Rejeu et fermeture

Depuis ce dossier : `python3 -B -S check.py`, puis `python3 -B -S -O check.py`.
Les sorties sont identiques octet pour octet et conservées une seule fois dans
[normal.json](normal.json). Même comparaison pour `evidence.py DOSSIER_LOCAL_DES_PREUVES`.
Cette seconde lecture exige les traces hors dépôt ; leur absence ne devient jamais un succès.
[verification.json](verification.json) consigne les deux comparaisons et [SHA256SUMS](SHA256SUMS)
ferme les fichiers du reçu. Aucun nouvel identifiant de constat demandé dans cette tranche.

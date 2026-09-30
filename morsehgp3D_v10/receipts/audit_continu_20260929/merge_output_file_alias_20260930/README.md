# Fusion des sessions : alias de fichiers encore destructeurs

Contre-audit indépendant du clone privé du développeur, 30 septembre 2026.
Périmètre : petites opérations CSV/JSON en répertoires temporaires privés.
Aucun moteur, import natif, NumPy, GCP ou fichier partagé exécuté/modifié.
Le paquet médian précédent reste fermé et inchangé.

## Défaut nouveau reproduit

La garde `same_directory(out,session)` protège correctement une session utilisée
comme dossier de sortie. Elle ne protège pas un **fichier** de sortie préexistant
qui partage une session source par un lien symbolique ou un lien physique.
Les dossiers sont réellement distincts ; le lien est stable, créé avant l'appel.
Il n'y a ni course, ni hypothèse TOCTOU, ni acteur concurrent.

Sept cas conservés, deux sessions complémentaires, plan préenregistré valide de
deux scènes × deux méthodes, toutes les métadonnées et hashes cohérents :

| Cas | Code fusion | Source modifiée | Code décideur check-only |
| --- | ---: | --- | ---: |
| Dossiers et fichiers distincts, sorties sentinelles | 0 | aucune | 0 |
| out est la première session | 2 | aucune | non exécuté |
| out/results.csv → session/results.csv, symlink | 0 | CSV source | 0 |
| Même alias, hardlink | 0 | CSV source | 0 |
| out/run.json → session/run.json, symlink | 0 | JSON source | 0 |
| Même alias, hardlink | 0 | JSON source | 0 |
| out/results.csv → session/run.json, symlink | 0 | JSON remplacé par CSV | 0 |

La fusion lit les sources, puis ouvre les sorties en mode `w`, en suivant les
liens ou en tronquant l'inode partagé. Les cinq variantes d'alias écrasent donc
silencieusement une session source tout en produisant une sortie fusionnée qui
passe encore le lecteur actuel. Dans les deux variantes CSV, une session d'une
scène reçoit le CSV des deux scènes ; dans les variantes JSON, ses métadonnées
sont remplacées par celles de la fusion ; dans la variante croisée, run.json
devient littéralement un CSV. Les sources originales sont conservées octet par
octet dans les captures base64 avant/après ; rien de partagé n'a été altéré.

Ce constat n'est **pas** un faux score de clustering ou une fausse revendication
statistique de supériorité : c'est une perte d'intégrité des entrées, compatible
avec un lot fusionné encore conforme. Il ne réouvre pas l'ancien défaut de
dossier identique, dont la correction est un contrôle positif du présent test.

## Correction conseillée au développeur

Avant toute ouverture destructive, comparer les fichiers de sortie potentiels
aux fichiers d'entrée protégés, avec résolution de liens et même inode ; prévoir
les alias croisés, pas seulement results↔results et run↔run. Les écritures dans
des fichiers temporaires frais puis remplacement atomique peuvent éviter de
tronquer un inode partagé, mais ne remplacent pas la définition de la politique
des destinations ni le contrôle des dossiers identiques. Tester aussi les trois
sorties entre elles et préserver le préenregistrement. Aucun correctif n'est
implémenté par cet auditeur.

## Sources et portée

Sources copiées octet pour octet depuis
`build/v10-integration-r2/src/morsehgp3D_v10/bench/` :

- decide.py : 1559ae328cec242d40d538e22649fb3b867db1d60e3ebee6606d05bf0b953889 ;
- merge_sessions.py : 059cc7ea3d9ebbe79729235abba3e9eb693f624e38f4d22bd1e501ce23cd17f9 ;
- scenes.py : 61ea9abc511726c2a9ff1e066f7603d6bef195a7c5356a2d2048c28229608f00.

Le contrôle de schéma P6, le rejet des clés JSON/en-têtes CSV dupliqués, le
contrôle des couples du plan et la substitution zéro des lignes refusées sont
présents dans le code relu. Ce paquet ne prétend pas rejouer toutes leurs gates,
certifier la provenance des scores, qualifier les calculs statistiques ou
exhaustivement couvrir tous les systèmes de fichiers. Le domaine NaN des lignes
refusées est intentionnel selon EVAL_v2 D8, pas présenté comme un nouveau défaut.

Un préflight réussi, puis deux captures finales normal/-O :
19:06:01.520159–19:06:04.801821 UTC, codes 0, stderr vides,
stdout identiques 33 162 octets. Sources du clone épinglées avant/après stables.
Le harness lance les petits CLI avec `-B -S`, donc sans site-packages ; en mode
-O il leur transmet également -O. Seules les chaînes d'affichage normalisent
les deux racines de chemins et omettent -O pour égalité sémantique des captures.
Les commandes extérieures réelles, timestamps et pins sont dans receipt.json.

Lecture autonome : `python3 -B verify.py` puis `python3 -B -O verify.py`.
Le lecteur ferme d'abord l'inventaire et tous les hashes avant de lire le reçu,
vérifie les snapshots base64 puis rejoue les sept cas deux fois. Chaque appel
utilise un TemporaryDirectory neuf ; seules ces données privées sont remplacées
ou supprimées. Aucune sortie industrielle ni qualification FULL/GPU héritée.

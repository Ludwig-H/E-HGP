# Tranche31 : raccord global q3/q4 et LiDAR

21 septembre2026, qualifications locales et pilote G4 CPU clos. Profil u16, CPU,
`public_status=not_claimed`. Ce dossier ne qualifie ni FULL ni GPU.

Déjà clos :

- `regression_s88mxc3a/` :92 CTests Release,196 sources fermées.
- Reprise du seul oracle : `regression_0cd1l_3e/`92 CTests Release,
  `gate_1a7ujimj/` dix gates Release, `gate_1q8uuu7m/` dix gates
  Clang ASan/UBSan/LSan, `mutations/compiled_wqf_kywc/` trois mutants
  causaux ; [clôture et distinction des sources](R2_QUALIFICATION.md).
- Instrumentation initiale : `gate_tb_fv9nw/` dix gates ASan/UBSan,
  `edge_pilot12_d2jo8ey8/` douze sondes instrumentées ;
  `global__6ze1yo7/` sonde64/K5/W4 sous Clang TSan,
  `global__rlwn_g2/` huit sondes64 ASan/UBSan. Ce ne sont pas92 tests TSan.
- `edge_scale_gom3hgqp/` :144 mesures, neuf arêtes originales sur trois
  scans, n8k/16k/32k/50k, K5/10, deux ordres d'exécution28/29/30.
- `mutations/compiled_5slu1ek1/` : trois fautes produit compilées et
  réfutées exactement par la comparaison géométrique de la nouvelle gate.
- [`global_vwtz76da.tar.gz`](global_vwtz76da.tar.gz) :48 mesures préliminaires globales avec records
  complets, n64/256, K5/10, s8/10/12, W1/4, backends28/30.
  Le lecteur initial exact est conservé dans `preflight/` ; cette capture
  reste attachée à son schéma et à ses sources propres. Les originaux ont
  été déplacés, sans suppression, dans
  `build/v8_lidar_global_20260921/archived_receipts/global_vwtz76da/`.

Les chronos de ces qualifications ont une charge concurrente ; ils ne
démontrent pas un gain de vitesse stable. Les comptes discrets et sorties
sont comparés. Une arête productive sélectionnée ne représente pas tout
le nuage ; les grandes mesures globales restent distinctes.

Les données sont les scans odométriques KITTI utilisés par SemanticKITTI,
sans labels sémantiques : quantification isotrope2cm et déduplication
historiquement documentées, échantillonnage imbriqué par priorité hash.
Scans0/100/200 séparés, coordonnées dans le repère LiDAR du scan0 ;
ce n'est pas une fusion multi-scan. Hashes des fichiers et métadonnées
vérifiés, mêmes IDs originaux entre les trois méthodes q4.

Les neuf fixtures d'audit fournissent des entrées, jamais des verdicts :
le constructeur refait les boules en rationnels, leur positivité,
propriétaire et census global à chaque taille.76 observations de cibles
retiennent la boule et68 la rejettent au bon seuil ; les coquilles de
toutes les boules effectivement émises sont recencées indépendamment.

Pour relire une capture edge/gate/régression :

```bash
python3 -B morsehgp3D_v8/bench/run_q34_lidar_checks.py read morsehgp3D_v8/receipts/lidar_global_20260921/edge_scale_gom3hgqp --compact
python3 -B -O morsehgp3D_v8/bench/run_q34_lidar_checks.py selftest morsehgp3D_v8/receipts/lidar_global_20260921/edge_scale_gom3hgqp
```

Les essais en échec ou de préflight sont conservés et ne sont pas promus
par une capture ultérieure. Une durée de composant n'est jamais le
contrat50k de toute la tour K1..10 sur G4.

## Extraire et relire la capture globale initiale

L'archive de28 256 326 octets a pour SHA256
`134f03c576d8a4f25f0ef546ef4d188d050910a42f9d3ac3864d4a7b4083b2c0`.
Deux générations GNU tar/gzip aux noms triés, date nulle et propriétaire
numérique nul donnent les mêmes octets. Tous les fichiers archivés, leur
extraction dans un répertoire neuf et les originaux déplacés ont les mêmes
SHA256. Le protocole, les commandes et les empreintes sont conservés dans
[`INITIAL_GLOBAL_ARCHIVE.json`](INITIAL_GLOBAL_ARCHIVE.json) ; le helper
[`archive_initial_global.py`](archive_initial_global.py) refuse tout écrasement.

Depuis la racine du dépôt, avec les entrées LiDAR originales toujours disponibles :

```bash
archive_dir=$(mktemp -d /tmp/mhgp8-global-v1-replay.XXXXXXXX)
tar -xzf morsehgp3D_v8/receipts/lidar_global_20260921/global_vwtz76da.tar.gz -C "$archive_dir"
python3 -B morsehgp3D_v8/bench/run_wspd_q34_lidar.py read "$archive_dir/global_vwtz76da" --compact
python3 -B -O morsehgp3D_v8/bench/run_wspd_q34_lidar.py read "$archive_dir/global_vwtz76da" --compact
```

Ne pas ajouter `--check-live` à cette capture historique : le lecteur a
évolué après son exécution. Sa relecture stricte ne réexécute pas le moteur
et ne la transforme pas en qualification des sources actuelles. Les lectures
initiales normal/−O figurent dans [`FINAL_READBACK.json`](FINAL_READBACK.json),
avec celles de sept autres captures closes, deux autotests du lecteur et
le rejet explicite de l'essai LeakSanitizer en échec :22 commandes,197 sources,
262 fichiers d'entrée et87 artefacts inchangés avant/après. Les chemins
historiques de `global_vwtz76da/` dans ce reçu désignent désormais les mêmes
octets dans l'archive et dans le répertoire de récupération du build.

## Essai global local8k terminé, campagne interrompue

`global_tsd9ofnm/` reste **FAILED**, interruption volontaire pendant16k.
Sa première commande8k/K5/s8/28/W4 a terminé :1360,996s,
2 285 750 arêtes,104 670 candidats, dont93 914 q3. Elle paie361,201
milliards de tests ponctuels q3, dont99,1% extérieurs. Les16k/32k ne
fournissent aucun résultat complet ni ratio de croissance. Voir le
[diagnostic et les priorités](../../docs/Q34_GLOBAL_ET_LIDAR_20260921.md).
Le temps local sous charge n'est pas une mesure G4 ; cette observation
ne qualifie ni la campagne entière ni le contrat50k.

## Reprises de portabilité et essais GCP

Les captures `gcp_r1_compile_failure/` et `gcp_r2_gate_failure/`
conservent les archives hôte, reçus et sources exactes, sans clés privées.
Leur génération cible est certifiée **TERMINATED** dans chaque `ARCHIVE.json`.
R1 échoue à construire une sonde auxiliaire inutilisée, sans lancer de
benchmark. R2 construit les deux cibles utiles mais sa gate reste bloquée
et est interrompue ; aucune mesure scientifique distante n'en découle.

Trois comparaisons `rational == 0` de la gate ont ensuite été remplacées
par `rational.numerator() == 0`, sans changement géométrique ni du moteur.
Les anciennes comparaisons sont incompatibles avec les règles C++20 de
certaines versions Boost ; les [notes Boost1.75](https://www.boost.org/releases/1.75.0/)
décrivent leur correction. L'ancien fichier est conservé intégralement dans
`preflight/wspd_q34_gate_before_boost_fix.cpp.txt`. Cette modification
ne transforme pas les captures précédentes en tests des nouvelles sources :
les relire sans `--check-live`, les reprises r2 possèdent leurs propres pins.

## R3 : cinq mesures CPU sur G4 SPOT, session terminée

L'[archive hôte](gcp_r3_completed/host_capture.tar.gz) et son
[manifeste](gcp_r3_completed/ARCHIVE.json) conservent la gate, la compilation
stricte GCC11.4, les cinq commandes, les sources/données et les logs d'arrêt.
SHA256 archive : `360313f28e1de842acfc52014c72730160148bc66d9f8671b14a501b102f5550`.
La génération `2026-09-21T00:57:56.826-07:00` est certifiée TERMINATED ;
aucune autre VM `project=e-hgp` active à la fermeture. Aucun GPU exécuté.

Scan0, préfixes imbriqués du fichier8k quantifié2cm, K5, s8, q3+q4,
Local28, flux/digests. Durées de préparation commune+front+candidats+
callbacks ; pas catalogue, intérieurs, Kruskal ou parents FULL.

| Points | Workers | Durée | Candidats émis |
|---:|---:|---:|---:|
|1 000|1|3,849s|11 357|
|1 000|48|0,863s|11 357|
|2 000|48|8,470s|24 061|
|4 000|48|59,274s|49 949|
|8 000|48|614,744s|104 670|

À1k, accélération observée ×4,459 pour1→48 workers, un seul essai de
chaque configuration, pas un gain stable qualifié. Le passage2k→4k→8k
multiplie les temps par environ7 puis10,4. Les tests ponctuels q3 font
33,554Md à4k puis361,201Md à8k, soit ×10,765 pour un doublement de n.
**Cette implémentation ne montre donc pas une croissance sous-quadratique
sur ce scan LiDAR.** Les petits échantillons ne déterminent pas une loi
asymptotique ; ils suffisent à motiver la réduction structurelle suivante.
Les campagnes globales16k/32k et les autres scans restent à faire.
q4 n'est pas dispensé de cette refonte : ses seeds font×5,59/×6,48/×5,67
et ses visites d'atlas×9,38/×10,91/×10,39 aux mêmes doublements, malgré
un petit travail final de balayage. Mesurer seulement ses16M sites actifs
ou ses16M comparaisons de tri masquerait les milliards de requêtes/préparations.

GNU time observe220,58s CPU pour59,27s mur à4k, puis1986,83s CPU pour
614,74s mur à8k : environ3,72 puis3,23 CPU utilisés en moyenne, malgré
48 workers. Le répartiteur garde des sous-arbres/rectangles/arêtes entiers.
Les données ne comportent pas de chronomètre par worker : la contribution
précise de chaque tâche au temps critique reste à instrumenter.

Les analyses [géométrie/croissance](gcp_r3_completed/ANALYSIS.json) et
[CPU/répartition](gcp_r3_completed/CPU_COSTS.json) concordent octet pour
octet avec leurs lectures `−O` ; [reçu des quatre lectures](gcp_r3_completed/POSTHOC_READBACKS.json).
La comparaison1k/W1-W48 conserve tous les comptes géométriques et digests,
hors des deux seuls pics privés documentés. La même comparaison à8k
entre localW4 et G4W48 passe aussi ; le facteur de temps inter-hôtes×2,214
ne mesure pas isolément le gain de parallélisme. Les grands flux sont
comparés par digests, pas par un oracle exhaustif de toute la sortie8k.

Les [propositions de suite](../../docs/Q34_GLOBAL_ET_LIDAR_20260921.md)
visent deux défauts distincts : trop de calculs répétés, et trop peu de
travail finement partageable. Aucun contrat50k/1s,100ms, FULL ou massif
n'est acquis ; une exécution CPU sur une G4 n'est pas un backend GPU.

Relecture locale des reçus G4, sans commande cloud ni relance du moteur :

```bash
g4_replay_dir=$(mktemp -d /tmp/mhgp8-g4-replay.XXXXXXXX)
tar -xzf morsehgp3D_v8/receipts/lidar_global_20260921/gcp_r3_completed/host_capture.tar.gz -C "$g4_replay_dir"
python3 -B gcp-migration/read_cpu_probe_v8.py --host "$g4_replay_dir/cpu_v8_host" --local-record morsehgp3D_v8/receipts/lidar_global_20260921/global_tsd9ofnm/record_0000.json
python3 -B -O gcp-migration/read_cpu_probe_v8.py --host "$g4_replay_dir/cpu_v8_host" --local-record morsehgp3D_v8/receipts/lidar_global_20260921/global_tsd9ofnm/record_0000.json
```

Le [rapport pédagogique G4](gcp_r3_completed/LECTURE_RESULTATS.md) présente
les coûts et la répartition, en distinguant observations et causes possibles.

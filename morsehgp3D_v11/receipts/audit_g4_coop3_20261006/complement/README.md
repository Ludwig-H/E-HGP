# Précision de portée après contrelecture du reçu coop3

Le reçu adjacent reste inchangé. Sa formule « 90 dumps conservés » doit
se lire **90 empreintes de sorties canoniques conservées dans les reports**.
Son champ `canonical_dumps_checked=90` compte ces empreintes, pas des
artefacts binaires ou textuels complets relus par un oracle indépendant.

Au pin `9eee2ed4bcef1e960cdf2456012b84416854dc20`,
`bench/gpu_ab.py:163–165` calcule le SHA puis supprime le dump. Les deux
reports contiennent les 90 champs `dump_sha256`. L'archive possède
43 fichiers, dont 42 couverts par son manifeste, sans sortie FULL brute.
Les empreintes correspondent aux six références attendues ; les registres
complets sont comparés par le juge épinglé, avec aucun refus, mais les
lignes individuelles ne sont pas conservées. Pour les 162 passes chaudes
intermédiaires, seul le statut de réussite reste vérifiable.

Cette précision ne change aucun temps, hash ni verdict du banc. Elle
interdit de présenter la contrelecture comme 90 nouveaux différentiels
sur des sorties complètes. Aucun natif ni appel GCP lancé par l'auditeur.

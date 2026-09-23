# Contre-audit B — session G4 R3, ablation des voies mortes

23 septembre 2026, lecture indépendante du reçu développeur
`morsehgp3D_v9/receipts/g4_tower_r3_20260923/` apparu vers 02:02 UTC.
Le dossier, encore non suivi par Git à la première lecture, a été publié
ensuite dans **`3d9d5d3d`**, sans changement des 258 fichiers hachés.
Pas de nouvelle commande GCP lancée par l'audit. Snapshot exécuté :
`b4e480fc`, profil u18/grille 1 mm sans sol, CPU G4 `g4-standard-48`,
trois trames **d'une seule séquence 08**, s8, W48 et FULL statique W48.
Ni GPU, ni trame brute, ni contrat public acquis.

## Fermeture et concordance vérifiées

Les **258** entrées de `SHA256SUMS` passent. Le reçu hôte et celui de la VM
annoncent `completed` ; les 14 commandes de sondes ont code 0 et issue
`complete_relative`, avec 5 ou 10 ordres effectivement publiés. Les 50
commandes du reçu invité portent `group_closed=true`. `guarded_stop` a
code 0, groupe fermé, génération ciblée et sortie archivée indiquant
`TERMINATED` ; aucune requête GCP *live* supplémentaire n'a été faite.
La cible, génération, provenance et évidence de garde invitées concordent
ici avec l'hôte. Le préflight de 1 500 sites est réellement actif
(55 523 covers, 3 476 126 formes, 23 848/26 428 voies q3/q4 prouvées),
et son stderr passe le parseur GNU time en relecture indépendante.
Le manifeste de sources avant/après est identique ; le paquet désigne le
commit Git annoncé et les trois fichiers u32 1 mm épinglés. L'archive de
transport et le binaire ne sont pas conservés comme blobs indépendants,
mais hashes, commandes, dépendances et sorties le sont.

Pour les six paires on/off et les deux comparaisons répétition/W24, les
JSON bruts ont les mêmes `input`, `generator`, `catalogue`, `orders`,
`tower_work` et `tower_digest`. Le `ledger` **diffère normalement** :
le certificat évite des voies et change leurs coûts. Le condensé FULL
FNV-64 inclut structure de forêt, contributions et IDs de population ;
c'est une forte comparaison d'exécution, **pas** une liste indépendante
de toutes les BallKeys et coquilles du catalogue. La complétude globale
reste relative aux clés émises et aux petits oracles existants.

Les identités de travail du certificat sont vraies pour chaque cas on :
`dead_loads=cover_builds`,
`dead_form_sites=cover_sites−2·cover_builds`, et
`dead_q{3,4}_proved+dead_q{3,4}_open=q{3,4}_edges` du cas off.
Le reçu réel ne manifeste aucune des quatre falsifications acceptées par
le [lecteur v5](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md) ; leur
existence interdit toutefois de transformer le simple statut
`completed` en preuve fail-closed pour les campagnes suivantes.

## Gain réel et mur restant

Temps de **toute la chaîne** et du seul q3/q4, en secondes ; chaque paire
est une exécution sur les mêmes octets, avec le certificat seul comme
levier déclaré. Les colonnes sont `sans → avec`.

| Trame | K | Chaîne | q3/q4 | FULL avec |
| --- | ---: | ---: | ---: | ---: |
| 000000 | 5 | 12,93 → 8,89 | 8,19 → 4,14 | 3,78 |
| 000100 | 5 | 10,12 → 6,19 | 6,43 → 2,48 | 3,05 |
| 000200 | 5 | 19,71 → 10,93 | 14,63 → 5,81 | 4,06 |
| 000000 | 10 | 48,33 → 35,56 | 22,57 → 9,70 | 22,80 |
| 000100 | 10 | 36,29 → 25,96 | 16,63 → 6,24 | 17,47 |
| 000200 | 10 | 62,99 → 37,75 | 37,75 → 12,49 | 22,14 |

Le gain q3/q4 est substantiel (environ ×2–3), et la chaîne gagne aussi,
mais reste **×6–11** au-dessus de 1 s à K5 et **×26–38** à K10. Le
cas 000000/K10 avec certificat répété fait 35,46 s ; W24 donne 38,92 s
et FULL 23,16 s contre 22,68 s en répétition W48. Ce seul rapprochement
indique un faible gain marginal de 24 à 48 fils pour FULL, non une borne
de scalabilité. Le nouveau tri/encodage FULL parallèle WIP est **absent**
du snapshot R3.

À K10, même en rendant magiquement **gratuits q3/q4 et FULL**, la somme
mesurée `q2+merge+census` vaut encore **2,000 / 1,388 / 2,041 s** selon
la trame ; la queue non attribuée aux postes chronométrés ajoute
0,836–1,048 s (résumé/digest compris). Il faudra donc aussi refondre ou
paralléliser ces étapes — et fixer si le digest diagnostique entre dans
le chrono du produit — pour atteindre 1 s. Une accélération du seul tri
Kruskal ne peut fermer le contrat.

Le prix restant du certificat n'est pas caché :
`dead_form_sites=1,79–3,59` milliards à K5 et `4,15–9,28` milliards à
K10, soit `1,42–1,86 n²` et `3,28–4,90 n²` sur ces trois tailles.
Ces rapports à un seul ordre de grandeur ne sont **pas un exposant de
croissance**. Le [certificat par nœuds avant cover](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md)
et les gardes par blocs sont à tester contre ce poste, avec toutes les
visites et l'aval comptés. Priorités de preuve : réparer le lecteur v5
avant d'autres dépenses, puis mesurer 8k/16k/32k
par coupes capteur appariées et plusieurs séquences, sans sol **et** brutes,
s8/10/12 ; toujours FULL, K1..5 puis K1..10, W1/W48 et RSS.

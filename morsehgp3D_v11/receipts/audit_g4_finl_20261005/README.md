# Finl : toutes les portes longues hors mutants passent

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.

Session `v11.20261005.claudefinl`, source publiée
`38b76701b9b0198fc1c37afe16e1480e638e513c`. DONE=3,
`failed_remote`, arrêt ciblé certifié. Reçu, plan, paquet et archive
vérifiés ; les 635 fichiers utiles du paquet sont identiques au commit.
Archive SHA-256 :
`2fc873bed69674bb060561387cac09e8b3ba7a793d62bda30d9a1ec14fbc39ee`.

**Les 28 portes longues hors mutants sont toutes PASS** : 11 portes
K10 (racines, Euler, identité de l’arbre K, registres et hiérarchie des
supports, identités CLI à 32k et sur ng00) et 17 références FULL exactes.
Le résumé conserve leurs noms et chacun de leurs verdicts terminés.
Aucune de ces 28 portes n’est à reprendre à cause du délai de finl.

La sélection `-L ^long$ -E _vs_python` inclut aussi les treize campagnes
mutants, car elles portent le label `long`. Sept ont un verdict PASS
en u21. CTest atteint ensuite l’échéance globale pendant
`mhgp11_mutants_tower`. Les campagnes supports, points, head, API et CLI
n’ont pas de démarrage consigné. Bilan brut : **35/41 PASS, zéro Failed,
six sans résultat**. La configuration reste `timeout`, non conforme.

Ce délai est celui du runner : 2 100 s pour la configuration, avec
2 080,4 s restant au départ de CTest. Le runner termine son processus
avec code −9 à cette échéance. Aucun TIMEOUT propre à une porte ni
défaut moteur n’est établi. Le `LastTest.log` archivé ne garde que son
en-tête de 121 octets ; le détail natif du lot tower manque.
La [contrelecture runtime](native_review.json) conserve les lignes de
verdict et les empreintes des journaux. Aucun décompte de mutants
individuels n’est reconstruit depuis ces seuls succès de campagnes.

Pour une future session consacrée aux portes longues fonctionnelles,
ajouter l’exclusion du label `mutant` conserve ces 28 portes et évite
de mêler leur qualification à une autre campagne. Le lot M dédié est
en u18 : ses résultats restent propres à ce profil et ne sont pas
transférés aux six campagnes u21 sans résultat de L. Les corrections
API/CLI et leurs rejugements sont suivis dans le reçu finm. Aucun
rejeu général de L n’est demandé pour ses identités K10 déjà terminées.

P10 et P9 gardent leurs huit différentiels complets : le filtre
`_vs_python` les excluait explicitement de L. Aucun contrat de temps
ni résultat GPU n’est déduit des durées CTest.

Rejeu depuis ce dossier :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

Le lecteur contrôle la fermeture, les empreintes, la source, l’inventaire
unique et les verdicts, puis sépare les portes selon leur label
`mutant`, contrôlé aussi contre leurs noms. Il dépend de l’archive
locale et du commit Git ; cette capsule n’est pas un reçu autonome.
Aucun build, test natif, réseau ou action cloud. Aucune donnée LiDAR,
sortie binaire native ou journal brut complet copié.

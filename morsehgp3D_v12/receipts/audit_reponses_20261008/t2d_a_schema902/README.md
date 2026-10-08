# T2-d-A : contre-lecture du schéma recouvert sur base 902

Capture du 8 octobre 2026 à 05:12:53 UTC. Copie A/w902 non commise, base déclarée
`902041f66`, pilote `ea9023cf…`, [sources et journaux épinglés](pins.json).
Suite de [l'admission avant refonte](../t2d_a_admission/README.md), dont le reçu reste immuable.
Aucun moteur, compilation, GCP ni donnée réelle exécuté ou lu par cet audit.

**Correction confirmée :** `lire_prise` impose désormais P+C+G+raccord+TMVR≤wall.
Le mur artificiel de 1 ns est refusé au rejeu du brut, avant la statistique.
La sonde garde le chemin séquentiel par défaut et expose explicitement `--recouvert` :
`etapes_ns` devient une partition murale, `fenetres_ns` conserve les sommes de fenêtres
G/forêt/T/M/V/R, et `memoire_octets` porte P/C/tour. Cela évite de présenter la somme des
fenêtres parallèles comme un temps mural. Aucun transfert automatique aux lecteurs de L1.

[check.py](check.py) est un port explicite du témoin antérieur, adapté aux champs exacts
émis par cette sonde et à ses prises `apres_sequentiel` supplémentaires sur ng00–02 K5.
Chaque journal est synthétique, réhaché et rejoué par `juger(..., verifier=True)` ; les
résumés sont reconstruits par le vrai lecteur. Les valeurs ne sont pas des mesures.
Le témoin positif possède toutes les classes d'identité annoncées (dont K10, trois tailles
uniformes, 37 noms synthétiques, W1 et les trois prises séquentielles après), puis cinq
paires de processus de dix passes sur les trois trames décisives.

| Contre-exemple isolé | Verdict au pin capturé |
| --- | --- |
| Mur après = 1 ns, partition conservée | **refuse**, correction vérifiée |
| Identité réduite à ng00 K5 | **adopte**, cohorte encore non fermée |
| `fin_ns=30000`, G=25000 et queue=TMVR=15000 | admis, malgré fin−G≠queue |
| `threads=true` pour W1 ou `liberation.pass=false` pour passe 0 | admis |
| Champ `cpu_ns` absent | admis, assimilé à null |
| Objet `memoire_octets` absent | admis |

Le lecteur typant les temps mais pas toutes les métadonnées laisse encore l'égalité
Python booléen/entier intervenir. Il ne contrôle pas le bloc mémoire publié. La cohorte
tronquée conserve de vrais journaux conformes : leur relecture ne prouve pas qu'une prise
attendue a été exécutée. Fermer la cohorte depuis le protocole et ses configurations,
contrôler les champs toujours émis, puis l'identité `fin=G+queue` déjà garantie par la
sonde. Ne pas imposer T+M+V+R≤wall : ces fenêtres peuvent se recouvrir.

Le journal de tests local du développeur `bw902.ctest.log` est clos à 05:08:08 avec code 0,
705 réussites et zéro échec ; `LastTest.log` contient 705 mentions `Test Passed.`.
Ce sont des traces conservées hors dépôt, réhachées dans les pins, pas des tests exécutés
par l'auditeur. La capture des sources est postérieure : elle ne constitue pas à elle seule
une chaîne complète sources→objets compilés. Aucun gain G4 n'en est déduit.

Résultats normal/−O identiques : [results.json](results.json). Le script refuse une autre
empreinte du pilote ; aucun sous-processus moteur.

```sh
python3 -B check.py --pilot /chemin/vers/pilote_t2d_a.py
python3 -B -O check.py --pilot /chemin/vers/pilote_t2d_a.py
```

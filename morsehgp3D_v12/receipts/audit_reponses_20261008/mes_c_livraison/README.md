# MES-C livré : défaut confirmé et correctif avec porte adaptée

Livraison **83ed7620d**, pilote **6309ac7e…**. Sa fonction `verdicts` a le même
AST que celle de la [prélecture](../mes_c_prelecture/README.md). La refonte
ordonne K5 avant K10 et extrait deux fonctions de lancement ; elle conserve le
verdict favorable possible sur une cohorte incomplète. Le patch de prélecture
s'applique à la livraison sans conflit. Aucune campagne réelle invalidée ici.

Les portes officielles passent en normal et −O. Elles ne réfutent pas le
contre-exemple : elles jouent le mode `essai`, CPU seul, et attendent encore C3
tenu alors que C3 demande les deux voies. Le verdict global `essai` demeure
correct, mais son critère C3 ne prouve pas la cohorte du contrat.

Le [patch complet proposé](cohorte_et_porte.patch) porte sur le pilote et sa
porte : cohorte fermée par les noms attendus, deux voies, K5/W48 ; cas non joué,
absent ou dupliqué non évalué ; critère non évalué bloquant le verdict global.
Les essais CPU seuls gardent leur verdict `essai` et leurs lignes de refus,
avec C3 non évalué. Neuf cas directs de la porte couvrent la cohorte complète,
les omissions, doublons, fils incorrects, refus et expiration observés.

**Validation sur copie des objets Git :** porte originale normal/−O code0,
porte adaptée normal/−O code0 ; les huit substitutions du runner officiel,
rejouées contre la porte adaptée, restent syntaxiquement valides et meurent
toutes code1 avec diagnostic logique. Aucun échec d'import, syntaxe ou moteur.
Les traces sont dans [results.json](results.json). Le [rejeu](check.py)
extrait et vérifie les cinq fichiers épinglés avant chaque essai ; les sondes
sont Python fictives, et les petits fichiers de tests sont fabriqués puis
supprimés. Il n'exécute aucun moteur HGP, CUDA ou commande distante.

```sh
python3 -B check.py --repo /workspaces/E-HGP
```

Le patch reste **non intégré par l'auditeur**. Les réserves de prélecture sur
l'interprétation de la pente C2 et la validation des manifestes restent hors
de cette correction. Sources et postimages : [pins.json](pins.json).

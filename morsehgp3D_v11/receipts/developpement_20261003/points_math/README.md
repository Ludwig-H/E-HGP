# Choix mathématique de la hiérarchie de points : workflow du 3 octobre 2026

Reçu de [HIERARCHIE_POINTS.md](../../../docs/HIERARCHIE_POINTS.md). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. GCP non utilisé par ce
workflow ; aucune construction ni test natif ; aucune commande git par les agents.

Demande de l'utilisateur : « Tu pourrais lancer un workflow sur la meilleure méthode mathématique à choisir ».
Workflow `wf_92a63749-ee7`, lancé à 21 h 03 UTC, terminé à 23 h 05 : neuf agents (quatre propositions, quatre
vérificateurs adverses, un juge), aucun en erreur, 720 appels d'outils. Les agents ont travaillé en Python local
sur l'oracle exact de la définition (nuages d'au plus neuf sites, quatorze pour Q3) et sur le code de la v10
(`build/v10-verrou-points/`, hors dépôt) ; ces petits nuages sont des oracles de correction, ni fréquence ni pente.

| Dossier | Rôle | Conclusion principale |
| --- | --- | --- |
| [`marge_cibles`](marge_cibles/RAPPORT.md) | la règle à marge face aux 125 cellules ancrées v10 et au catalogue | preuves F1, F2, H1, H3 justes ; $H_{k+1}$ 70/125 ; en rayon c'est l'ancrage $P_1$ de la v10 ; marge en rayon recommandée |
| [`majorite_vote`](majorite_vote/RAPPORT.md) | vote de la thèse rendu hiérarchique, ER0h | ER0h sans constante uniforme (proposition S) ; vote dépendant de $F_K$ et $p$, discontinu |
| [`fermeture`](fermeture/RAPPORT.md) | fermeture qualifiée de l'auditeur | garanties exactes ; liaison simple de $w_{k'}$ pour $m\leq k+1$ ; borne extérieure et certificat |
| [`axiomes`](axiomes/RAPPORT.md) | axiomes, impossibilités, caractérisation | théorèmes B (anticipation), C (front intrinsèque), E, F ; marge en niveau carré dominée |
| `verif_*` | vérifications adverses des quatre rapports | voir chaque `RAPPORT*.md` |
| [`SYNTHESE.md`](SYNTHESE.md), [`synthese`](synthese/) | juge final | recommandation $H^{r}_{k+1}=P_1\circ\Pi_{k+1}$, prix, points ouverts, expériences décisives |

**Prompt du juge.** Le juge a tourné avec le script tel que lancé ([`workflow/script_lance.js`](workflow/script_lance.js)).
Le script a ensuite été édité pour lui transmettre la consigne de l'utilisateur (« Q2 ou Q3 ne sont que de peu
d'importance par rapport au modèle mathématique »), les mesures G4 et les commits de l'auditeur
([`workflow/script_edite_apres_lancement.js`](workflow/script_edite_apres_lancement.js)) ; ces éditions ne
s'appliquent qu'à une reprise, qui n'a pas été jouée. Le juge a néanmoins lu la consigne dans `HIERARCHIE_POINTS.md`
(version de 22 h 01) et les archives G4 du développeur ; sa recommandation coïncide avec le choix du développeur.

Les corrections que le juge demande à `HIERARCHIE_POINTS.md` (§ 9 de la synthèse) sont intégrées dans la version
publiée avec ce reçu. Les rapports sont gardés tels que rendus, y compris leurs erreurs relevées par les
vérificateurs ; les chemins absolus qu'ils citent pointent vers l'arbre de travail du 3 octobre.

```sh
cd morsehgp3D_v11/receipts/developpement_20261003/points_math && sha256sum -c SHA256SUMS
```

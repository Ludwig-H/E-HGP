# Coop2 — correctif ee3 exécuté, exactitude préservée, critère de gain toujours refusé

Session `v11.20261006.claudecoop2` close : `completed`, worker 0, `DONE=0`, arrêt ciblé certifié le **6 octobre 2026 à 09:53:45.016 UTC**. Source exécutée **`ee3eabe5e7aae4d95515f63935e8f9a84adbd468`**, paquet de 639 fichiers utiles identique aux objets Git. Cette capture auditeur porte sur la session locale close ; aucun reçu développeur ultérieur n'est comparé ou promu.

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Aucun build, test natif ou accès cloud par l'auditeur.

Les commandes K5, K10 et profil K10 sont toutes closes avec code 0. Les bancs K5/feuille16 et K10/feuille24 sont conformes : mêmes trois trames entières de 39 885 / 35 551 / 45 845 sites, W48 et modes CPU16379 / GPU81915 / coop212987. **90 processus, 252 passes construites et 90 dumps canoniques** ont leur inventaire complet. Les 162 passes chaudes intermédiaires ont un statut OK ; seuls les processus froids et la dernière passe de chaque processus chaud conservent un dump. L'égalité du registre complet est imposée par le juge épinglé avec liste de refus vide ; les lignes natives individuelles ne sont pas conservées. Aucun nouveau CTest, mutant, Compute Sanitizer ou profil u24 n'est qualifié dans cette session.

Sur les passes résidentes 2..P, les intervalles bruts séparent à nouveau les prises coopératives du seuil visé, sur les trois trames : K5 exécuteur coop/GPU un fil reste au-dessus de 0,5 ; K10 domain coop/CPU reste au-dessus de 0,85. `summary.json` donne les bornes `min(coop)/max(référence)` et les durées exactes, sans calcul de médiane. Le correctif n'a donc pas acquis le critère du plan initial.

Le correctif ee3 ne constitue pas un retour démontré à l'ancien coût d'écriture. Dans les **dernières prises chaudes K10** du GPU à un fil, les `fill_ns` restent **232,102 / 238,811 / 201,161 ms** pour ng00/ng01/ng02 ; ceux du GPU coopératif sont **22,416 / 23,829 / 25,495 ms**. Comparaison bornée de ces prises seulement : le coût de comptage et les autres étages restent distincts. La capsule coop1 et le reçu historique L4 gardent leurs propres sources et observations.

## Profil séparé utile pour la suite

`profil_k10` porte uniquement sur **ng00, K10/feuille24, mode81915 à un fil**, et non sur le noyau coopératif. Les deux noyaux sont effectivement présents dans le rapport NCU ; les métriques du JSON concordent avec son CSV. Les durées NCU incluent l'instrumentation et ne remplacent pas les prises ordinaires du banc.

| Indicateur observé | count_kernel | fill_kernel |
| --- | --- | --- |
| Threads actifs moyens par warp | 3,14 | 1,39 |
| Occupation atteinte | 21,06 % | 2,08 % |
| Warps actifs par SM | 10,11 | 1,00 |
| Registres par thread | 146 | 154 |
| Taille de grille | 16 571 | 132 |
| Waves per SM | 7,35 | 0,06 |
| Cycles écoulés NCU | 501 710 358 | 538 363 052 |
| Durée instrumentée NCU | 270,83 ms | 290,61 ms |

Ce profil établit une utilisation faible des lanes et un petit nombre de blocs pour la seconde passe. Il rend pertinente une expérience coopérative sur un même préfixe et ses sites de recensement ; il ne prouve pas que cette seule transformation atteindra le contrat, ni une cause exclusive de latence. La divergence, les accès de tables et les autres coûts doivent rester des hypothèses à confronter à leurs propres ablations.

Le premier essai NCU a refusé l'accès aux compteurs ; la relance administrateur prévue par le script épinglé est passée. Les exportations finales sont closes, `problems=[]`. Les SHA256 des rapports et CSV sont contrôlés, mais aucun rapport binaire ni CSV lourd n'est copié ici.

`summary.json` conserve uniquement métadonnées et quelques indicateurs. Aucun octet LiDAR, dump natif, journal intégral ou identité de compte n'est recopié. `python3 -B replay.py` et `python3 -O -B replay.py` revalident la fermeture, les archives, les 639 sources, les inventaires et métriques conservées ; relectures identiques. Dépendances : session/archive locale close et objets Git. Aucun calcul HGP ni accès cloud ; cette capsule n'est pas une archive autonome.

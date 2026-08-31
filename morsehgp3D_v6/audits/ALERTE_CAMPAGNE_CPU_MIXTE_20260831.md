# Alerte active — campagne CPU v6 exécutée avec deux binaires

Date de constat : 2026-08-31

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict

La campagne `morsehgp3D_v6/receipts/campagne_decision_20260831/` est **invalide comme matrice épinglée et comme campagne de décision**. Ne pas recevoir son agrégat, même si son script finit avec 36 codes nuls.

Conserver les sorties comme diagnostic d'un run invalide et inscrire ce statut explicitement ; elles ne doivent pas être transformées en preuve de pente, de coût ou de déterminisme inter-graines.

Mise à jour après `cfaf6b41` : le reçu corrige le décompte à **32 sorties ancien binaire et 4 sorties contaminées**. Sa requalification explicite en `baseline_sonde_partielle`, limitée aux 27 sorties homogènes de `terrain_stationnaire`, `scanline_stationnaire` et `uniform`, est recevable comme lecture diagnostique post-hoc. Elle ne réhabilite ni la matrice de 36 runs, ni `eight_clusters`, ni un verdict E6.

## Preuve indépendante

À la coupe de 13:09 UTC :

- `META.txt` annonce le pin source `518e270683129a4badea9df31400a97582528401` et le SHA-256 exécutable `7607138784e763dc1e1341bca93db4ff3783aaaba5dc45d62ac9aad7ea92a321` ;
- le fichier partagé `build/v6/mhgp6`, remplacé à `12:58:38 UTC`, porte désormais le SHA-256 `4bbb257cd31413f2c1058ee7b873f2ffe84158e3ce299a76d1230e6ab3053359` ;
- le processus actif `eight_clusters n=32000 seed=4` exécute ce second hash, vérifié via `/proc/<pid>/exe` ;
- `eight_clusters_16000_s4.txt`, terminé à `12:59:32`, conserve l'ancien schéma (`m_anchor`, puis `vcensus nœuds=...`) parce que son processus avait démarré avant le remplacement ;
- le tuple suivant `eight_clusters_16000_s5.txt`, terminé à `13:02:43`, porte le nouveau schéma (`h_scan`, `entrees_ancres`, `octaves_q4_seeds`, census séparé), preuve visible du changement d'exécutable au milieu de la matrice ;
- 34 lignes de statut sur 36 étaient terminées, sans `DONE`, `PENTES.txt` ni `RECU.md`, et le tuple `eight_clusters n=32000 seed=4` était encore actif.

Le lanceur invoque le chemin partagé `build/v6/mhgp6` à chaque tuple. Le hash unique écrit au début ne protège donc pas les invocations suivantes contre une reconstruction concurrente. La campagne ne permet pas de rattacher chaque sortie à un seul binaire ni à un seul schéma.

## Correction utile à Claude

Pour le rejeu :

1. attendre la fin de tout build ou test concurrent ;
2. copier l'exécutable construit dans un répertoire privé de campagne avant le premier tuple, le rendre non modifiable pour la session et n'exécuter que cette copie ;
3. enregistrer son hash dans le META et dans chaque statut, puis le revérifier avant et après chaque invocation ;
4. faire refuser l'agrégateur si un hash, un schéma ou le pin diffère sur un seul tuple ;
5. inscrire la charge concurrente observée dans le reçu et ne jamais interpréter les temps de cette exécution invalide ;
6. produire un marqueur terminal `invalid`, pas `completed`, pour la campagne actuelle.

## Relance encore exposée

À 13:21 UTC, `receipts/campagne_sonde_octaves_20260831/` a démarré au pin `cfaf6b41` et au bon hash `4bbb257c...3359`, mais son META annonce encore l'exécution directe de `/workspaces/E-HGP/build/v6/mhgp6` à chaque tuple. Aucune copie privée immuable ni hash par run n'est visible.

Cette relance n'est pas encore contaminée au constat, mais elle reproduit exactement la condition qui a invalidé la précédente. Ne plus reconstruire ce chemin pendant son exécution ne remplace pas une preuve de provenance. Pour obtenir un reçu décisionnel, interrompre/rejouer depuis une copie privée hashée reste la solution propre ; à défaut, borner cette relance au statut exploratoire même si le hash final coïncide.

Cette alerte pourra être absorbée dans `ETAT_COURANT.md` puis supprimée lorsque le run aura été marqué invalide et que le lanceur immuable aura une fixture de reconstruction concurrente.

GCP non utilisé par le présent audit.

# Première session FULL 50k sur G4 SPOT

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**Les contrats 50k ne sont pas atteints.** Deux vrais processus, K=1..10
puis K=1..5, refusent avant tout ordre FULL. Ce sont des mesures de refus,
pas des temps de construction de hiérarchie. La VM est arrêtée et son
état `TERMINATED` est certifié pour la même génération.

## Protocole et résultat

Sources au commit `0072c88d`, limitées aux 42 dépendances v7 réellement
consommées par le compilateur. G4 `g4-standard-48` existante, SPOT,
48 vCPU accessibles ; calcul CPU seulement. Compilation g++ 11.4.0,
C++20, O3, NDEBUG, pthread et avertissements stricts avec Werror.
Le binaire `eb41fe665a202b429dc0af1d61066a2d3ef94e15164598b61803ec2ffaa2d463`
est identique avant/après ; les sources le sont aussi. Pas de bootstrap
nécessaire, aucune installation CUDA ni mesure GPU.

Nuage uniforme u16, seed=3, domaine de coordonnées 65536, n=50 000,
WSPD s=8, lazy first-C avec un million d'entrées, proposeur MEB sans quota
d'opérations. `--threads=48` raccorde les workers du pipeline ; FULL et
la boucle des ordres restent séquentiels. Les limites de payload logique
8 Gio et d'espace d'adressage 26 Gio du probe restent déclarées.
Le smoke n=8 termine ses huit ordres possibles et lie leur digest global.

| Exécution demandée | Temps externe jusqu'au refus | Pic RSS GNU time | Enregistrements à coquille supplémentaire | Ordres construits |
| --- | ---: | ---: | ---: | ---: |
| Tour K=1..10 | 21,372 s | 9 625,555 Mio | 4 | 0 |
| Tour K=1..5, processus distinct | 5,646 s | 2 932,879 Mio | 3 | 0 |

Les deux sorties ont le code 2, `outcome=unsupported_degeneracy`,
`reason=probe_rank_relevant_extra_shell`, `last_stage=regularity`.
Le bilan de masse du front est fermé, le census a terminé et les cinq
premiers étages parallèles ont employé 48 workers. Expansion et FULL
n'ont pas commencé. Le nombre d'enregistrements refusés n'est pas une
preuve de quatre ou trois plateaux géométriques distincts.

| Étape mesurée avant refus | K10 | K5 |
| --- | ---: | ---: |
| Génération | 12,263 s | 4,409 s |
| Tri/RLE | 3,123 s | 0,294 s |
| Préfiltre | 2,852 s | 0,394 s |
| Census | 2,819 s | 0,436 s |

Ces étapes dépassent déjà la seconde sur cette exécution ; elles ne sont
pas substituées à une mesure de tour complète. Aucun speedup 50k par
rapport au local, p95 ou comparaison s8/s10/s12 n'est déduit de ce passage.
Les [captures et le lecteur portable](../receipts/full_g4_spot_50000_20260906/README.md)
conservent aussi les échecs, codes et temps exacts.

## L'admission mémoire corrigée passe réellement

K10 produit U=21 685 604 candidats uniques et S=21 468 368 survivantes.
Le nouveau proxy census vaut 8 275 135 296 octets, sous le budget logique
de 8 589 934 592 octets. L'ancien calcul 608U aurait demandé
13 184 847 232 octets : c'est une comparaison arithmétique sur les volumes
observés, pas l'exécution d'un ancien binaire. Le census nominal termine.
La correction ne réduit pas à elle seule la RAM allouée et le pic RSS
peut dépasser ce budget logique nommé.

## Verrou suivant : régularité pertinente et plateaux

La sonde refuse actuellement toute boule de census de la fenêtre dont
`n_shell != arity`. Cette hypothèse appartient à l'autorité régulière du
chemin actuel ; la supprimer sans preuve n'est pas une optimisation.
Le cas 50k montre qu'il ne suffit pas de tester uniquement les petits
nuages uniformes qui passent cette hypothèse.

Les coordonnées des cas fautifs ne sont pas dans ce reçu G4 historique.
Une [extraction locale ultérieure](PLATEAUX_FULL_ET_ANCRES.md) conserve
désormais les quatre I/U et leurs clés, vérifiés contre tout le nuage.
L'auditeur prouve un raccord par ancres de boule et un certificat augmenté
des gains de couverture ; son intégration FULL reste à réaliser.
Ni changement de seed pour effacer l'échec, ni relâchement du prédicat
n'est appliqué. Les résultats G4 ci-dessus ne sont pas réétiquetés.
Pas de nouvelle campagne massive tant que ce verrou et le coût FULL
séquentiel ne sont pas traités. Les dizaines de millions et le GPU FULL
restent à qualifier séparément.

## Fermeture et sobriété

Une seule VM réutilisée : projet `devpod-gpu-exploration`, zone
`us-central1-b`, instance `ehgp-v7-4fa0e0789a7d5bb06b787d35`.
Génération exacte `2026-09-06T06:19:11.593-07:00`. Garde GCE SPOT/STOP
3600 s et arrêt invité 30 minutes certifiés avant transfert et calcul.
Résultats récupérés après les deux refus ; arrêt ciblé demandé immédiatement.
`stop_and_verify.sh` termine avec le code 0 et confirme `TERMINATED` à
13:22:35 UTC, environ 3 min 25 après le démarrage horodaté. Une relecture
GCP indépendante confirme la même génération arrêtée. Aucune autre VM
active signalée par le script ; aucune autre cible modifiée. Cette durée
de session n'est pas un relevé de facture. Aucune VM ou disque créé.

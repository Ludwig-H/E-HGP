# Tentatives mono FULL horizontales — 5 septembre 2026

Les [résultats et limites](../../docs/RESULTATS_MONO_FULL_20260905.md)
portent sur les ordres horizontaux relatifs, pas sur une tour inter-K
intégrée. `public_status=not_claimed`, CPU mono, GCP non utilisé.

Campagne close : trois tentatives horizontales terminées à 8k pour
s=8/10/12, deux refus d'alias à 16k/K9 et 32k/K7, aucun timeout.
[summary.json](summary.json) ne transforme pas ces deux refus en succès.

[protocol.json](protocol.json) fixe la séquence, les plafonds, le binaire
et le périmètre avant les mesures. [sources_before.json](sources_before.json)
lie la source et le binaire à l'admission micro. Pour chaque tentative,
`*.intent.json` conserve la commande prévue, `*.raw.txt` la capture fusionnée
stdout/stderr intégrale, et `*.receipt.json` les lignes d'ordres et terminal
extraites sans réinterprétation. Les durées principales sont celles du
processus, pas les différences des dates de collecte de ces fichiers.

Un code 2 avec terminal négatif est une tentative close en refus, jamais
un succès de tour. Une capture sans terminal ou interrompue ne peut être
promue. Le juge indépendant de cohérence est
[full_gabriel_probe_audit.py](../../bench/full_gabriel_probe_audit.py) ; il ne
relance aucun moteur et ne décide aucune exactitude géométrique.

[sources_after.json](sources_after.json) confirme les 51 pins identiques
avant/après ainsi que le binaire micro effectivement consommé. L'inventaire
inclut le script d'admission micro, mais pas tous les fichiers du dépôt.
Les captures restent distinctes de la qualification du générateur ; la
chaîne d'outils n'est pas revendiquée hermétique.

Les temps incluent les lectures sentinelles et les destructions. Les
certificats sont détruits ordre par ordre ; le pic RSS ne mesure pas une
archive complète gardée en mémoire. Il n'y a ni digest des forêts, ni
preuve d'égalité entre s, ni comparaison appariée au payload F historique.

## Vérification des captures

[verification.json](verification.json) conserve les dix commandes du juge
exécutées par ROOT : chacun des cinq reçus passe en Python normal puis
sous `-O`. Les deux refus gardent `attempt_success=false` malgré un reçu
cohérent. Le juge est épinglé
`24e789459ee7adb8b48819dddc8bef8832b2b152ad9418c1a1d281038315e2c7`.
Ses deux selftests ferment chacun un positif réel, deux refus synthétiques
et neuf mutations causales ; ils ne sont pas de nouveaux runs du moteur.

Depuis la racine, on peut reproduire une vérification sans calcul HGP :

```bash
python3 -B morsehgp3D_v7/bench/full_gabriel_probe_audit.py morsehgp3D_v7/receipts/full_gabriel_mono_20260905/n8000_s8_k10.receipt.json
python3 -B -O morsehgp3D_v7/bench/full_gabriel_probe_audit.py --selftest morsehgp3D_v7/receipts/full_gabriel_mono_20260905/n8000_s8_k10.receipt.json
```

La clôture compare aussi chaque capture brute à la sortie intégrale de
l'outil conservée pendant le run, y compris son dernier saut de ligne.
Les 51 sources et le binaire sont encore identiques à l'admission.
Les compteurs par ordre, hors durées et échantillons mémoire, concordent
entre les trois s à 8k ; ce contrôle ne remplace pas un digest sémantique.

Le manifeste couvre tous les fichiers du paquet hors lui-même et
`SHA256SUMS`. Ce dernier couvre aussi le manifeste, pas son propre contenu.
Depuis ce répertoire, `sha256sum -c SHA256SUMS` vérifie le sceau. Les logs
bruts ne sont pas reformattés pour satisfaire un contrôle de blancs Git.

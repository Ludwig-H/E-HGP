# Prochaine paire mono D/E — préparation, aucun moteur exécuté

Le runner compare une **nouvelle observation D puis une nouvelle observation E** pour une seule valeur `--separation` parmi 8, 10 et 12. Les trois appels auront trois répertoires create-only distincts. Aucun chrono historique n'est accepté comme observation, y compris le D s=8 déjà mesuré. L'absence de `--execute` ne crée rien et ne lance aucun processus.

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Tour **candidate complétée K1..10**, pas le seul préfiltre Gabriel : uniforme, n=8000, coord=65536, seed=3, `--complete-incidences`, CSR, digest inclus et aucune archive. Mono demandé : CPU logique 6, threads=1, fold-inflight=1, fold-join=1. Chaque processus est borné à 600 s et RLIMIT_AS=26 GiB (espace virtuel, pas RSS physique). Proxy partiel 16 GiB ; caps silent 8M core, 2M chain, 2M cofaces, 1 milliard de queries et de supports MEB par ordre. GCP non utilisé.

## Adaptation limitée et autorités

`runner.py` charge et exécute exactement les octets du runner v3 historique `3abe27f8…`. Il réutilise son parseur complémentaire mono/caps/étages, le juge strict des six projections (`digests`, `cardinalities`, `counts`, `silent`, `silent_limits`, `payload_caps`), la validation du build candidat et les helpers de processus possédés/drainés. Le rôle C/D du juge historique est explicitement projeté vers D/E ; les reçus ne sont pas réétiquetés a posteriori.

Le parseur incidence original impose s=8. L'adaptateur effectue exactement **deux substitutions de source** sur des octets vérifiés : ajout du paramètre `separation` et remplacement du 8 fixe dans son prédicat d'identité. Les deux ancres doivent être uniques. Le SHA de la source adaptée figure dans les métadonnées. Les sorties du moteur ne sont jamais normalisées ni réécrites. Les selftests acceptent s=8/10/12 et rejettent chaque mauvais s croisé.

D reste `build/v7_meb_qualification/mhgp7`, SHA `127c5f923fcc9618d826b89dedda4de0f5201ea48e27330e2ea68e83d76a1b3f`. Sa liaison utilise le build scellé `aedfe1b4…` et les reçus 323 portes / pins protégés scellés. Les sources D sont historiques, **jamais celles du futur worktree E**. Le cache et la base de compilation D doivent conserver leurs octets, tout comme les deux CLI C historiques et D. Leur qualification 323 n'est pas rejouée par ce banc.

E exige un reçu `mhgp7-mono-meb-build-v1` terminé, issu du build réel, attaché au bon source-root, aux deux inventaires sources, au vrai chemin `build_dir/mhgp7`, au cache/base du même build et à une compilation Release CPU sans TESTING/PROFILE ni flags supplémentaires. Toute copie étrangère, même byte-identique, est rejetée par le validateur existant. Le snapshot initial doit correspondre aux liaisons et tous les sources/helpers/binaires/reçus/cache/base sont comparés aux frontières de chaque essai et **encore après la collecte finale Git**. C'est une liaison enregistrée, pas une attestation hermétique.

## Plan après revue et GO seulement

1. Construire E dans un nouveau dossier, en conservant le builder scellé existant sans le modifier. Sa prévisualisation est :

```bash
python3 -B build/v7_meb_paired/build_cli.py --build-dir build/v7_next_q2_qualification --output build/v7_next_q2_build_20260905
```

Le builder ne compile qu'avec `--execute`, après GO indépendant. Il reste create-only, construit seulement `mhgp7` Release/CUDA OFF, parallel 2, avec bornes 30/30/300/300 s et 26 GiB. Son filename `build_D.json` est **hérité du protocole**, mais le contenu contient les véritables source, build et SHA du candidat E ; ce nom ne lui attribue pas la qualification historique D. Aucun nouveau builder n'est nécessaire.

2. Prévisualiser chacune des paires ; exemple s=8 :

```bash
python3 -B build/v7_next_pair_20260905/runner.py --candidate build/v7_next_q2_qualification/mhgp7 --candidate-build-receipt build/v7_next_q2_build_20260905/build_D.json --separation 8 --output build/v7_next_q2_s8_pair_20260905
```

Pour s=10 puis 12, remplacer à la fois la valeur et le suffixe du répertoire de reçu. Ajouter `--execute` seulement après revue/GO et fermeture de toute autre charge lourde locale. Chaque paire demande au plus deux processus de 600 s, jamais une boucle infinie. Ne pas reprendre un dossier partiel : un nouveau nom est requis et l'échec reste conservé.

3. Comparer D/E strictement **à s fixé**. Pour comparer s=8/10/12, vérifier d'abord les digests et cardinalités entre les séparations ; les compteurs de travail et le champ s n'ont pas à être identiques entre séparations. Préserver tous les refus, censures et divergences. Pas d'agrégation qui masque une paire manquante et pas de ratio en cas incomplet.

## Selftests et limites

`python3 -B selftest.py` puis `python3 -B -O selftest.py` exécutent 21 méthodes : les 12 portes historiques intactes et 9 spécifiques à D/E. Elles incluent 24 scénarios de cycle de vie (8 à chaque s), les refus/censures/interruptions, identité inter-s, chaque projection divergente, liaison D historique, candidate obligatoire, protection des CLI, dérives de chaque pin, échec Git et dérive tardive après Git, LD sans fuite, préparation inerte et create-only. Une minuscule sonde héritée de 1 s contrôle le drainage d'un descendant réfractaire ; ce n'est ni un moteur ni un benchmark. Les autres processus de moteur sont tous synthétiques.

`prepare.py` ne peut lancer que ces selftests et trois prévisualisations inertes. Il conserve codes/stdout/stderr et pins avant/après dans un dossier neuf. Un succès de préparation n'est **ni un build E, ni une mesure, ni une qualification produit**. Les temps éventuels resteront des observations froides ordonnées sur hôte partagé, pas un gain statistique, un SLO 50k/1 s/100 ms ou un résultat massif/GPU. L'absence réelle de création de threads relève des portes produit dédiées, pas du chrono de ce runner.

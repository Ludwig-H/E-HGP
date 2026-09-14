# Captures propres — redistribution du front q2

14 septembre 2026, exploration hors registre, CPU, entrée quantized u16,
`public_status=not_claimed`. Qualification et192 mesures closes.
Aucun contrat FULL/G4.

Ces reçus concernent les nouvelles sources de la
[redistribution des produits pendants](../../docs/P0_REDISTRIBUTION_FRONT_Q2.md).
Ils n'héritent pas des tests ni des performances de la tranche13.

Préparation exploratoire, avant gel : une configuration CMake a échoué car
la seconde fixture était encore en cours d'écriture. Deux demandes de build
ASan/TSan ont précédé la fin de leurs configurations asynchrones et échoué
sans lancer de compilation ; elles ont été reprises après la fin de CMake.
Ces erreurs de lancement ne sont pas des essais de l'algorithme. Le premier
exécutable front_dispatch_exploratory précède la réduction des notifications ;
son résultat n'est pas une qualification des sources finales.

Les qualifications conserveront commandes, sorties brutes, échecs éventuels,
sources et binaires épinglés à l'ouverture et à la fermeture. Les campagnes
gardent la collecte complète q2, les compteurs géométriques et ceux du dispatch
séparés. GCP non utilisé.

## Qualification close

- `qualification/release_0arz_yju` :57 CTests Release,45 commandes PASS.
- `qualification/sanitize_lx4dj5ju` :57 CTests Clang ASan/UBSan,
  45 commandes PASS, exécution locale hors sandbox pour LeakSanitizer.
- `qualification/differential_3hc6yx8q` :32 configurations/64 commandes,
  ancien mono b268cf6f et nouveau, tous les champs hors chronos identiques.
- `qualification/tsan_alq4su6u` : deux gates Clang ThreadSanitizer PASS,
  dont transfert inter-worker réel et réveil/jointure après exception.

Les captures `tsan_0dw_tmxd` (SIGSEGV, aucune sortie de gate) et
`tsan_ala784no` (frontPASS, q2 arrêté par le runtime GCC TSan avec
`unexpected memory mapping`) sont conservées. Elles ne qualifient pas
TSan ; la reprise utilise un build Clang distinct, sans modifier les sources.
Les quatre builds Release/ASan/GCC-TSan/Clang-TSan sont maintenant épinglés.

Les lecteurs normal/−O sont identiques dans `qualification/readers_skgqlpdl`.
L'[analyse complète](qualification/analysis_v8_47i6o/SUMMARY.json), avec
étendues et travail par worker, est identique entre les deux interpréteurs.
Les snapshots des deux runners correspondent exactement aux hashes qualifiés.

## Mesures : ce qui progresse, ce qui ne progresse pas

Les six campagnes totalisent192 mesures/148 configurations, sur les quatre
cœurs physiques0,2,4,6. Corpus :96 essais synthétiques8k K5/K10 s8/10/12
mono/quatre workers,32 essais16k/32k s8,32 répétitions8k K10 s8,
16 essais LiDAR8k/16k/32k/50k,4 répétitions du premier scan50k et12
essais sur deux autres scans50k. Les cellules8k/K10/s8 et LiDAR50k/K10
ont ainsi trois essais par mode. Autres cellules : un essai, sans stabilité
statistique revendiquée. Aucune mesure pendant nos qualifications de build ;
la machine reste partagée avec d'autres travaux.

Médianes q2 à quatre workers, K10/s8, en secondes :

| Entrée | Coarse | Donate | Lecture |
| --- | ---: | ---: | --- |
| Uniforme8k | 1,645 | 1,532 | Gain observé modeste, étendues partiellement communes |
| Terrain8k | 0,296 | 0,301 | Pas de gain |
| Amas8k | 0,857 | 1,176 | Régression observée, forte dispersion |
| Rangées8k | 0,180 | 0,181 | Pas de gain ; gros callback non divisé |
| LiDAR000000/50k | 5,460 | 2,868 | Ratio trompeur sans l'étendue Coarse3,038–5,506 s ; pas de gain×1,90 promu |
| LiDAR000100/50k | 3,057 | 2,642 | Gain observé, étendues2,913–3,535 et2,603–3,571 s |
| LiDAR000200/50k | 3,904 | 3,723 | Gain modeste, étendues communes |

Les temps ne suffisent pas à justifier un basculement par défaut : `Coarse`
reste la référence. Le partage effectif et l'équivalence exacte sont acquis
sur les fixtures ; une accélération stable et générale ne l'est pas.
Les médianes mono8k restent voisines entre les deux chemins (environ−1,5 %
à+0,6 % selon la famille), sans taxe mono importante observée.

## Croissance et limites

Les six principaux comptes géométriques gardent exactement les valeurs
Coarse/Donate. Sur les cinq séries8k/16k/32k (quatre synthétiques et
préfixes LiDAR000000 vérifiés), leurs ratios de doublement restent inférieurs
à3, donc sous4 sur ce corpus. Maximum des visites census :2,958 puis2,701
sur amas/K10 ; LiDAR/K10 :2,289 puis1,873. Cela n'est pas une borne globale.

Les compteurs du répartiteur ne sont pas tous sous4 : par exemple dons
amas/K5 ×4,075 et attentes ×5,205 au second doublement. Ces décisions varient
avec l'ordonnancement et de petits dénominateurs ; elles ne sont ni supprimées
ni additionnées aux tests géométriques. Les consultations du donneur suivent
les produits (maximum observé environ×2,683). Par construction, avec un
intervalle I, leur nombre est au plus Σ floor(T_worker/I), et les dons leur
sont inférieurs : cette voie ne crée pas de parcours A×A/B×B supplémentaire.
Cela ne borne toujours pas T sous-quadratiquement pour tous les nuages.

La prochaine étape utile est de rendre partageables les travaux de census
encore indivisibles, en possédant leurs continuations et le plan parental,
sans nouvelle préparation par tranche d'ancres. Ne pas multiplier les variantes
de file sur la foi d'un meilleur essai. q3/q4, parents FULL, GPU et plusieurs
dizaines de millions de points restent ouverts. Aucun GCP utilisé.

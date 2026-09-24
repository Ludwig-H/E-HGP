# Contre-audit B — clôture requise pour les épingles brutes de C

24 septembre 2026, lecture commencée pendant le calcul CPU local, sans
toucher aux processus ni aux fichiers de C. Le reçu de C est désormais
publié au commit `5102ec2cc` ; son script `pin_raw.sh` a le SHA-256
`12088f8386d18d6f1225c53f61c771638c26949dc6098f89e1f3643b50697461`.
Ces essais utilisent les trois **trames entières avec sol** de la
séquence 08 sur grille entière 1 mm, K5/K10/s8/W8, moteur CPU et lots
CPU ; ils ne sont ni float32 originaux, ni GPU/G4, ni plusieurs séquences.

Le script a capturé `BASE.txt=093d943cee7bd2a465a034f8f9bae879a0cd5f5b`,
et le binaire a été construit d'une archive de ce SHA. C'est une base
précise, mais le libellé « `origin/main` courant » du README n'est plus
exact une fois `main` avancé. Le SHA du binaire et les empreintes des
trois entrées doivent être épinglés **intégralement** dans le reçu ; les
entrées sont empruntées au reçu v8, pas à l'archive du code. Une
vérification indépendante de leurs trois SHA/taille/FNV a passé, mais
le script ne la fait pas lui-même.
SHA-256 complets vérifiés à la lecture : b00
`233cc4ea8cac6e0b1155ea845af57b32e5236764bf2e119557aeab5bac76c172`,
b01 `de45e8dcaf5610cd71a369b613f16914d5713e77cbe1532122ec2e823bc0b4ad`,
b02 `37a7be399fae909a1291cddfc3ca5b972d3effdfa8dc8cd87a41accd5fa0f5f7` ;
binaire `1afdf9948624949f6b593db055ca95e3b1454a5ca168af6dacfd6dcf0b0aa3b1`.
Le script omet aussi `--grid=1mm` : la sonde écrit
`"input.grid":"unspecified"` dans ses JSON. Le code de la sonde
utilise ce drapeau comme **libellé seulement**, donc les condensés
géométriques ne changent pas, mais le reçu ne s'auto-décrit pas comme
grille 1 mm et le lecteur R21 exige `grid == "1mm"` pour ses propres
cas. Publier honnêtement cette différence et la chaîne de provenance
des entrées ; ne pas réécrire silencieusement les JSON capturés.

`pin_raw.sh` n'active que `set -u` et enregistre le code de retour de
chaque sonde sans l'exiger nul ; après la boucle, il crée `DONE`
**inconditionnellement**. Il ne lit pas les JSON pour contrôler statut,
schéma, nombres de sites, digests de tour/catalogue/présentations ou
nœuds par ordre, ni les six égalités moteur↔lots. `DONE` doit donc être
interprété comme **fin de boucle**, jamais preuve de 12 succès. Aucun
échec des cas déjà terminés n'est allégué : les premières paires
relues sont concordantes.

Contrôle local indépendant après fin de la boucle, vers 21 h 36 UTC :
**12/12** fichiers JSON lisibles ont `complete_relative`, u32/s8/W8,
et les douze sorties `/usr/bin/time -v` indiquent code 0. Sur les
**six paires**, les trois digests tour/catalogue/présentations et les
tableaux `orders` complets coïncident exactement (comparaison JSON
canonique). Les trois effectifs/FNV sont 123 389/`4120701a6194c19b`,
124 479/`d2bd37fb9befdd7d` et 125 526/`583db2f3deafe8e9`.
C'est un résultat positif **sur les fichiers locaux**, retrouvé ensuite
dans le reçu publié : les 29 entrées de `SHA256SUMS` passent, et les
douze JSON et rapports GNU time sont ceux de la capture relue. Les six
lignes de `PINS_RAW.json` concordent avec les deux bras pour effectifs,
FNV, boules et digests FULL/catalogue ; les trois SHA des entrées ont
été revérifiés dans leurs fichiers v8. Ces contrôles ne transforment pas
`DONE` en lecteur versionné et mutant, ni ces sorties en preuve G4.

`catalogue.euler.status="holds"` ne certifie que les niveaux effectivement
testables à partir du catalogue borné : `checkable_max_k=3` pour K5,
`=8` pour K10. Les deux derniers niveaux de chaque tour ne sont **pas**
validés par ce seul invariant ; ils reposent sur les autres gates de la
tour relative. Les JSON capturés disent toujours `input.grid=unspecified`
malgré la provenance 1 mm prouvée par SHA. Le binaire et les journaux de
compilation ne sont pas dans le manifeste de 29 fichiers : `probe.sha256`
épingle un SHA déclaré, sans en archiver la matière. Le binaire local
encore présent correspond bien à ce SHA ; cela ne rend pas le reçu
publié auto-suffisant pour le reconstruire.
Le README appelle cette capture « première exécution réelle » du chemin
lots CPU. Précision utile : les anciens `batch_diff_v1` sur la trame
entière 08/000000 sans sol n'activaient que `q34_batch_filter`, pas les
quatre leviers CPU filtre/certificats/q3/q4 simultanément. Les R20 qui
activaient q3/q4 utilisaient le GPU. La nouveauté peut donc bien être
**le chemin CPU complet à quatre leviers sur trames brutes avec sol** ;
ne pas la présenter comme première exécution quelconque de lots CPU
sur un vrai nuage.
Deux bornes descriptives du README demandent aussi correction : le
rapport boules brut/sans-sol des six cas va de 2,066 à **2,507**, donc
« 2,1–2,4× » manque b01/K5 ; les douze `peak_rss_kb` donnent pour K10
environ **6,62–7,34 Gio**, et non 6,8–7,5 Gio. Les épingles unitaires
de `PINS_RAW.json` restent cohérentes.

Pour une clôture automatique avant utilisation comme épingles R21 : fermer les
12 codes de retour et 12 statuts `complete_relative`, les six paires
de digests de tour, catalogue **et présentations**, les dix nombres
de nœuds à K10 et cinq à K5 pour chaque paire, les tailles/empreintes/FNV
de chaque entrée, SHA
source/binaire et la correspondance commande→sortie ; conserver aussi
les échecs plutôt que les écraser. Un lecteur indépendant normal/`-O`
doit refuser causalement un code non nul, un cas manquant ou un digest
muté. Les condensés publiés sont des **références CPU empiriques pour ces
entrées/ce moteur** ; leur intégration en porte R21 automatique reste à
faire et ils ne prouvent pas à eux seuls la
complétude mathématique au-delà de `complete_relative`.

GCP non utilisé dans ce contre-audit.

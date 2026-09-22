# Marques historiques au premier parcours — raccord CPU

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Prototype privé, moteur actif inchangé. GCP non utilisé.

## Delta et portée

Le [parent à rangs certifiés](../rank_guard_streaming_20260911/README.md)
est conservé : census, Atlas, géométrie par fenêtres, workers persistants,
réduction native et export FULL. Seule la reconstruction depuis les
certificats utilise maintenant le premier DSU pour les marques, après
fermeture complète de chaque plateau. Aucun second DSU ni rejeu des parents.
La référence de la sonde reste l'ancien constructeur à deux parcours.

La [note mathématique](../../docs/MARQUES_PREMIER_PARCOURS_20260911.md)
précise dates brutes, naissances tardives, invariants et limites. Le helper
porte explicitement le prototype auditeur de 2970d679 ; ses propres
qualifications ci-dessous ne sont ni celles de l'auditeur, ni une preuve
universelle de géométrie/complétude WSPD.

Avec L naissances, E arêtes et J multifusions : N=L+J nœuds,
P=E+J parents, L−E racines. Chaque marque paie encore sa recherche,
désormais dans le premier DSU. Tris, normalisation et groupes restent
payés. Le travail est O((L+M) log(L+M+2)) en taille du certificat forestier,
pas une borne tous régimes en nombre de points.

Les 24 champs `fused_history_work` sont séparés du travail HLD de
l'export, qui ne change pas. Les douze capacités/tailles portent six
ensembles de vecteurs nommés ; la gate FULL et la sonde les additionnent
entre appels, même pour les maxima locaux de scratch. Ce ne sont pas des
pics simultanés ni du RSS. La gate structurelle rapporte au contraire le
maximum de chaque observation entre essais, avec son propre schéma.
Le retrait logique de trois tableaux, 24L octets sur cette plateforme,
ne prouve aucun retrait identique du pic : les marques précoces peuvent
cohabiter avec les temporaires des plateaux suivants.

## Gates closes

La gate FULL O2 et ASan/UBSan/LSan passe **31 commandes par build**,
résultats identiques : 114 census, 912 essais, 506 448 terminales et
49 519 620 contrôles. Fenêtres1/7/31/4096, workers1/4, s8/10/12,
permutations, coquilles, plateaux, K1 et K=n sont conservés du parent.

Les histoires sont comparées physiquement sur les mêmes certificats
**avant** l'export : tous les champs des six vecteurs, dont 150 000 ancres
de naissances et 308 208 marques. L'export ignore actuellement les marques :
une simple égalité FULL n'aurait pas suffi. Puis forêts, contributions,
verticales, φ, certificats et travail géométrique restent comparés aux
références indépendantes bornées et au parent scellé.

Totaux nouveaux : 4 368 constructions de DSU, 145 632 unions,
233 472 liens parentaux, 86 400 plateaux d'arêtes et 283 776 dates
d'événements. Aucun second DSU/rejeu. Les anciens champs scientifiques
du parent sont inchangés ; +744 096 contrôles de comparaisons et
d'instrumentation. Le nouveau mutant `fused-work` corrompt un compteur ;
les vrais mutants du parcours appartiennent à la gate suivante.

Gate structurelle propre, O2 et SAN : 19 fixtures × deux permutations,
**38 essais**, 45 136 contrôles, 4 548 comparaisons BFS de sommets et
6 956 de marques aux coupes ouvertes/fermées. 146 naissances, 72 arêtes,
210 nœuds, 136 parents, 74 racines et 194 marques. Les forêts non vides
sans marque, avec/sans arête, marques seules, naissances tardives,
identifiants non chronologiques et dates brutes équivalentes sont exercés.
Le résultat avec `Work*` est aussi identique au résultat sans pointeur.

Deux mutations du vrai helper — réponse avant plateau et date ramenée
à la naissance — sont refusées par leurs champs de marque, code4,
trois injections et 25 contrôles chacune dans leur invocation propre.
Le selftest les exerce aussi. Six certificats invalides produisent douze
refus appariés ancien/nouveau ; les six contrôles de publication vérifient
que Work reste intact après échec, même après un plateau et des marques
déjà calculés. Unknown/missing refusent code2, sans sortie.

Trois captures structurelles de dix commandes sont conservées :
`focused_o2_r1`, puis `focused_san_r1` lancé par erreur sans `--san`
(donc **O2**, comme l'atteste son reçu), enfin le véritable
`focused_san_r2` avec ASan/UBSan/LSan. La tentative mal nommée n'est
ni supprimée ni réinterprétée ; les trois résultats sont égaux. Aucun
nouveau verdict TSan ni CTest du moteur actif.

## Mesures propres

Les trois micros physiques n800/s8/10/12, K1..10/W65536/4-4-4,
passent avec mêmes objets FULL ; le travail géométrique est inchangé
entre séparations et par rapport aux bras correspondants du parent.
Ils ont chevauché
les qualifications ; aucun optimum temporel de s n'en est déduit.

Les mesures grandes sont successives, après fermeture de tous les tests
et compilations ROOT/sous-agents. L'hôte reste partagé. Synthèse d'entrée,
digests et comparaison diagnostique sont exclus et chronométrés à part ;
index, génération, census, validation, extraction, reconstruction, export
FULL en mémoire et libérations sont inclus dans le temps entier. Le RSS
externe couvre le processus complet. Aucune archive industrielle n'est
produite par cette sonde.

Les grands runs sont à un seul bras :
`physical_comparison_executed=false`. Le lecteur compare au parent leurs
digests de payload et leurs comptes complets/par ordre. Les comparaisons
physiques directes sont celles des gates et des micros, pas un double
calcul caché dans les chronométrages 8k/16k/32k. Les temps du parent sont
des captures historiques non appariées ; leur variation ne s'attribue pas
entièrement au changement de reconstruction.

## Sources et reproduction

Pins propres : helper `78fd73f0…`, observation `87572bba…`,
gate FULL `caabd44b…`, sonde `f81e24b0…`, gate structurelle
`7506ca8d…`. Recorders séparés et gelés : FULL/sonde `ac9b7ef6…`,
structurel `94073e59…`. Le header streaming parent reste inchangé.
`history_backend=fused_mark_first_sweep` désigne le bras streaming ;
`two_sweep_reference` désigne la référence. `history_work` reste
le travail HLD de l'export, pas celui du nouveau reconstructeur.

Parent unique : manifeste
`e97e07ee140030624bf6cdafb80cadee2787450be399ef550fc0fdf23fa780cb`,
lecteur `e4394c2a…`. Sources communes empruntées par nom/hash,
sans transfert de qualification. Copies minimales consommées, commandes,
dépendances -M/-MD, compilateur et ELF sont épinglés avant/après.
Les binaires sont omis du dépôt avec leurs empreintes conservées ; le
sysroot de lien/runtime n'est pas capturé intégralement.

Le lecteur épingle et vérifie les flux bruts de chaque capture, puis
compare les résultats JSON validés O2/SAN, les identités de travail et
les campagnes appariables au parent. Temps, RSS, distributions et scratch
dynamiques ne sont pas des égalités exigées. Aucune source non compilée
n'est présentée comme résultat qualifié.

```bash
python3 -B morsehgp3D_v7/receipts/fused_history_streaming_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/fused_history_streaming_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/fused_history_streaming_20260911/verify.py --extract build/fused_history_replay_fresh
```

Extraction explicite create-only, sans exécution implicite. Les recorders
extraits utilisent chacun un nouveau `--out` ; le recorder structurel
accepte `--san` sans `--kind`. Boost doit déjà être disponible au chemin
déclaré pour la gate FULL ; aucune installation automatique.

Ni le contrat 50k sous 1 s/100 ms, ni les dizaines de millions de points
sur G4, ni une borne sous-quadratique en points tous régimes ne sont acquis.

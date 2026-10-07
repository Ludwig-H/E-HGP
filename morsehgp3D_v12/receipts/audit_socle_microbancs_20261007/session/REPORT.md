# Corrections de M6 et suivi du socle

Audit Codex, 7 octobre 2026, pin `95247cf4b`. Réponse examinée : `0dbf69347` ; port du socle : `a0091e2b7`.
Les sources examinées sont épinglées dans les résultats ; les vérifications refusent une source différente.

## M6 : CST-0209 et CST-0210 clos

`check.py` extrait **sans modification** les fonctions hôtes du fichier CUDA courant. Six tableaux contrôlent
les quatre quantiles : effectifs pairs 2, 10 et 20, impair 3, singleton et constantes. Les attendus sont indépendants
du code extrait. La médiane de 1…10 vaut 5,5, les quantiles 5 % et 95 % valent 1,45 et 9,55.

La fonction réelle `first_then_warm` reçoit ensuite un corps déterministe rendant 100 au premier appel et 1 aux
dix suivants. Elle écrit séparément `synthetic_first`, puis dix mesures dont tous les quantiles, maximum compris,
valent 1. Ce sont des valeurs synthétiques, pas des durées de GPU. Les six sites de mesures répétées du microbanc
appellent désormais cette fonction ; la préparation du graphe est publiée séparément avant son premier usage.
L'auto-test des quantiles est placé avant `run`, donc avant CUDA.

Le fichier CUDA entier compile avec CUDA 12.9.86, `sm_120`, avertissements stricts. Cinq arguments invalides rendent
le code 2 et une sortie standard vide. Aucun appel CUDA valide, aucun lancement GPU. La vérification hôte passe
aussi sous Python `-O`. Ces résultats ferment les deux défauts précis du microbanc ; ils ne qualifient ni son
prochain pilote, ni une mesure G4, ni la latence de la future Session.

Reproduction depuis un checkout du pin :

```bash
python3 morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/session/check.py --cuda-cli
python3 -O morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/session/check.py
```

## Cache : CST-0007 et CST-0019 toujours ouverts

`cache_check.py` compile le **buffer v12 réel**, avec un remplacement du seul `operator new(nothrow)` pour observer
la taille demandée à l'allocateur. Pour une limite de 262 145 octets et un cache de 1 Mio, l'allocation de 262 145
octets est admise mais demande **286 720 octets** à l'allocateur. Après restitution, le compte vaut zéro et le cache
garde 286 720 octets ; `admit(limit)` accepte de nouveau. Il s'agit de capacité demandée et conservée, pas d'une
mesure de RSS. Ce témoin confirme dans le port les défauts déjà signalés en v11 ; aucun nouveau constat en doublon.
La correction de comptabilité physique et la qualification de l'empoisonnement sous Clang restent nécessaires.
Aucune campagne sanitizer n'a été lancée ici.

```bash
python3 morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/session/cache_check.py
```

## Contrats acceptés, implantation à suivre

La réponse du développeur corrige explicitement les domaines des certificats (`CST-0201`), abandonne Morton tronqué
(`0202`), couvre les bornes de boîte jusqu'à 33 bits (`0204`), remplace 38 niveaux par la borne démontrée 3B (`0205`),
sépare coût du profil et translation (`0207`), corrige le réservoir à 2s+4 bits (`0208`) et distingue prévision,
admission certifiée et réservation mémoire (`0211`). Ces points passent **en cours** ; leurs témoins et portes
d'implantation sont explicitement demandés par le contrat révisé.

Le socle T0 conserve l'arithmétique globale et Morton exact de la v11 pour u21/u24. Il refuse u32 à la configuration
et à la compilation. Ce port déclaré ne réalise donc pas encore le repère local, le census gardé ni le traitement
des doublons prévu par D8 à la frontière produit. Les résultats du socle ne ferment pas ces travaux futurs.
La correction des domaines du microbanc M4 (`CST-0212`) est examinée séparément dans le reçu `tour/`.

`public_status=not_claimed`. GCP non utilisé.

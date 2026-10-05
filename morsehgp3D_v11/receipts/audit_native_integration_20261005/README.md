# Relecture du raccord natif supports — 5 octobre 2026

Cadre `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Sources publiées9cbf, acteurs WIP S3/S5/S6 de basef98 ; fermetures propres à chaque capsule.
La [note F du développeur](developer_note_F.md), commit1c43d41ef, adopte le précédent
reçu et sépare L1=S3/S6 de L2=IO/API/CLI. [Contexte épinglé](SOURCE_CONTEXT.json).

**S3 est relu favorablement.** Journal avant DSU, publication par ordinal et attribution
après plateau ; compactage et coexistences de réservations contrôlés. Les résultats
natifs du rapport demeurent locaux, sans qualification G4 transférée.
**S5 corrige les anciennes alertes** SIGXFSZ, signature V2 et état publié, avec portes
causales. Reste à corriger le décodage variadique du seul hook de faute IO : cinq
arguments fournis, six `va_arg(long)` lus. Ce constat concerne le harnais, sans bug
produit ni crash observé. [Preuve et normes primaires](api/README.md).

Deux portes bornées nouvelles complètent K10/K12 :12 sites avec intérieurs à K10,
et la coquille Sphere5 à24 sites à K12, avec **116 traces strictes**. Le second cas
vise les primitives `Closure/Shape/counts`, sans reconstruire FULL12. Pour la
vitesse, conserver FULL16379 face à l'ordre seul7035, retirant seulement les trois
options inapplicables. Le juge S3 fait déjà cette différence.

| Capsule close | Gardes nouvelles | Portée |
| --- | ---: | --- |
| [tower](tower/README.md) | 15 283 | Journal scalaire, deux plateaux abstraits, majorants et compactage ; aucune nouvelle preuve géométrique FULL |
| [api](api/README.md) | 168 | Juges sur issues factices,60 définitions de mutants et contrat d'arité ; aucun mutant C++ exécuté |
| [qb](qb/README.md) | 4 135 | Comptes exacts indépendants K10/K12 et sources S6 inchangées |
| [root](root/README.md) | 127 | Bits des options et lecteur PH sur15 buffers synthétiques |

**19 713 gardes portables**, normal/−O identiques, bibliothèque standard seule.
Aucun build/test natif, moteur HGP, fit, KITTI, CUDA ou GCP lancé. Cela ne qualifie
ni performance, ni concurrence réelle, ni nouvelle livraison native.
[RESULTS.json](RESULTS.json) conserve ces distinctions.

Les trois notes de synthèse sont maintenues en place ; toujours six Markdown actifs
sous `audits/`. Les anciens reçus restent immuables. Les copies de sources conservent
les liens relatifs de leur topologie d'origine, sans prétendre être des guides copiés.
Les liens des README d'audit sont contrôlés séparément.

```sh
python3 -S -B ROOTcheck.py
python3 -O -S -B ROOTcheck.py
```

`SHA256SUMS` couvre chaque fichier sauf lui-même ; les inventaires enfants sont
vérifiés séparément. `ROOTcheck.py` rejoue les quatre capsules et compare leurs
sorties aux captures, puis recoupe les comptes et les limites publiés.

# Microbanc MES-P : la v11 gelée sur les petits nuages, à chaud

7 octobre 2026. Mesure de la tranche T0 ([`PLAN.md`](../../docs/PLAN.md)), **hors produit**, sans règle d'adoption :
publiée, elle fixe le seuil entre la voie CPU et la voie GPU des petits nuages de la v12 (décisions D5 et D7).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (v11 gelée ac081a06f, sonde mhgp11_full_bench, voie CPU 802811)
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`pilote_p.py`](pilote_p.py) (bibliothèque standard, Python 3.10 nu) joue chaque nuage du paquet `g4_small`
(159 nuages de 100 à 10 000 sites : morceaux d'objets et de contexte SemanticKITTI, bouts de scènes de la v11,
synthétiques ; `docs/DONNEES.md`) dans un processus neuf, avec `P` passes FULL successives dans le même processus
(régime chaud, la première passe paie les coûts froids), pour chaque `K` et chaque nombre de fils demandés. Durée d'une
passe : `wall_ns` de la ligne `pass` de la sonde. Valeur chaude d'un nuage : médiane des passes 2 à `P`. Puis, par `K`
et par nombre de fils, un ajustement par moindres carrés $t = a + b\,n$ : $a$ est le coût fixe, $b$ le coût par site.
La voie GPU de la v11 n'est pas jouée : le seuil se décidera avec la voie GPU de la v12 (T1-b), contre ces chiffres.

```bash
python3 pilote_p.py --v11-build <construction de la v11> --donnees <paquet g4_small> --sortie <dossier> \
    --fils 1,48 --k 5,10 --passes 6
```

Sur G4, le paquet voyage en **une seule archive tar** (`--archive g4_small.tar --deballage <dossier>`) : le
téléversement d'une session paie chaque fichier, et les 320 petits fichiers ont dépassé son délai le 7 octobre
(session `t1f`, VM arrêtée sans worker). L'archive est déballée après contrôle de chaque membre.

Premier passage local (7 octobre, trois nuages de 1 000 sites, indicatif) : 0,45 s par passe chaude à K5 sur un fil ;
les temps de la VM G4 font foi.

**Lecture par famille** : [`analyse_p.py`](analyse_p.py) `MES_P_JSON [--brut DOSSIER]` sépare les morceaux de trames
réels (droite des moindres carrés sur eux seuls, médianes par classe de taille), chaque famille synthétique (avec la
part de l'étage forêt lue dans les lignes brutes) et les prises en échec avec leur motif : la droite unique du
pilote mêle des familles dégénérées dont le coût explose et ne fixe aucun seuil (coût fixe négatif le 7 octobre).

**Mesuré sur G4 le 7 octobre** ([session G](../../receipts/g4_t0g_20261007/README.md), 48 fils, quatre passes) :
morceaux de trames, K5 6,9 ms + 7,4 µs par site et K10 5,5 ms + 42 µs par site ; réseau entier, étage forêt à 92 à
99 % du temps (8,4 s à 10 000 sites à K5, prise expirée à K10) ; sphère refusée dès 3 000 sites (`wide_leaf`).

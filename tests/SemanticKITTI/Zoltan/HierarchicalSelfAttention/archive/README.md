# Archive — conservé, mais hors des voies retenues

Ces six documents contiennent du travail correct. Ils ne sont sur **aucune** des voies que [VOIES.md](../VOIES.md) retient, et les garder au premier plan consommait le temps qui doit aller à la première mesure.

| Document | Pourquoi il est ici |
|---|---|
| `DESCRIPTEURS_DE_NOEUD.md` | Le descripteur de nœud est le levier le plus faible, mesuré : retirer *toutes* les features de nœud coûte $-0{,}7$ à $-4{,}1$ mIoU, contre jusqu'à $-8{,}4$ pour le nombre de niveaux. Le théorème de caractérisation du canal support qu'il contient reste juste, et reste du folklore. |
| `THEOREMES.md` | Le programme T0–T6 et QC-HSA supposent que HSA porte le modèle. HSA n'a aucune expérience 3D, aucun paramètre apprenable le long de la hiérarchie, et demande autant de passes séquentielles que l'arbre a de profondeur. |
| `CONTRAT_HGP.md` | Le contrat de l'objet marqué complet — facettes, cofaces, incidences, quatre carriers, autorité d'export. Rigoureux, et sans usage sur les voies retenues. La v3 ne le livre d'ailleurs pas. |
| `ARCHITECTURE.md` | Bâti autour du descripteur et de HSA. Sa seule partie encore utile, le choix de la baseline, est passée dans [PROTOCOLE.md](../PROTOCOLE.md). |
| `RESEARCH_PLAN.md` | Ses lots de travail découlent du plan précédent. Remplacé par les feuilles de route de [VOIES.md](../VOIES.md). |
| `STRATEGIE_PUBLICATION.md` | Ses parties vivantes — claims autorisés, figures décisives — sont passées dans [VOIES.md](../VOIES.md). |

**Rien n'est supprimé.** Si une voie retenue aboutit et qu'il faut construire un modèle complet, ces documents reprennent leur place. Mais ils ne doivent pas être lus avant que la voie 1 ait donné son résultat.

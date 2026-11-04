# CRTC0 : durée de la gestion additionnelle activée sur C0=1

Brouillon du 4 novembre 2026, **non envoyé**. Références : ACCC1.11 française.
Cette question est distincte de celle sur C5 du CRTC1.

Bonjour,

Sur les cas15/16 de SHAKE27E (nouveau paquet), nous cherchons à préciser la
séquence suivante : R0=63, R4=8, R5=0, R8=0, C4=8, C9=R9=0, puis R9←1 ou2
sur C0=1. Le programme écrit ensuite R7=3 et mesure le délai. Il utilise R3=#06,
donc une VSYNC de16lignes, et non notre ancienne hypothèse R3=#8E.

Les sections11.2.2 p82 et12.2 p95 indiquent que la modification inégale sur
C0=1 active la gestion additionnelle même avec R5=0 ; la ligne courante devient
la première ligne additionnelle. La section13.2.1 p106 indique qu'à C0=2,
la gestion additionnelle est désactivée si aucune ligne n'était programmée.
Cette dernière règle concerne-t-elle aussi la gestion déclenchée par l'écriture
sur C0=1, ou seulement la gestion provisoire activée par défaut ?

Concrètement, pour R9←2, après la ligne critique (C4=8,C9=0), doit-on obtenir :

- (8,1), puis directement l'origine (0,0) ;
- (8,1), puis (8,2), puis l'origine ;
- une autre séquence, notamment un comptage C9/R5 distinct ?

Le modèle actuel de CPCSyntaxError désactive cette gestion à C0=2 et suit
la deuxième séquence. Il donne #002F au cas16, comme la valeur attendue du
programme ; notre cœur donne #002B. Les deux donnent #001F au cas15 (R9←1),
où ces séquences ne se distinguent pas. Nous n'avons pas de photo matérielle
de ce module et ne voulons donc pas adopter un mécanisme à partir du seul
résultat d'un autre émulateur.

La fin « après une ligne » pour R5=0 décrite p106 concerne explicitement R0=1.
S'applique-t-elle également à cette entrée forcée avec R0=63 ? Une précision
sur l'état après C0=2 et sur les deux prochains couples (C4,C9) suffirait.

Merci !

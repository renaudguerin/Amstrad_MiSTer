# Questions ciblées : R0 du type0 et états du type3

Brouillon non envoyé, ACCC v1.11 française et paquet SHAKER2.7 actuel.
Les résultats d’émulateurs ci-dessous servent à préparer les questions ; ils
ne remplacent pas une mesure sur CPC. La question de durée de vie RFD reste
séparée dans le [brouillon du 3 novembre](accc-author-questions-2026-11-03-fr.md).

## Type0 : agrandissement de R0=1 par OUTI

La §13.7.2 p127 décrit un agrandissement de R0=1 par OUT, dont la
mise à jour est encore assez tôt pour éviter le retour de C0 à0. La règle
« too late » de la §13.6.1 p124 s’applique-t-elle également à R0=1 avec OUTI ?
Pour C4=R4, C9=R9, R5=0 et R8=0, nous cherchons à distinguer deux cas :

1. La cinquième microseconde d’OUTI tombe dans la cellule suivant C0=1.
   C0 revient-il à0 ou atteint-il2 ? Si C0 revient à0, C4 passe-t-il àR4+1
   et C9 à0 comme dans le fonctionnement R0=1 de la §13.2.5 p109 ?
2. La cinquième microseconde tombe sur C0=1. Obtient-on le même débordement
   que dans l’exemple OUT de la p127, ou un autre résultat ? Dans notre
   modèle, le début du cycle I/O se trouve avant le début de cette cellule.

Notre ancien contrôle confondait la cellule du chronogramme, C0 au début
physique du cycle I/O et l’état déjà décidé par la ligne précédente. Nous
conservons donc le comportement existant pour R0=1, sans le présenter comme
une mesure de ce cas OUTI. Le correctif A4 pour l’ancien R0=3F et le cas
R0=0 possède ses propres chronogrammes et contrôles indépendants.

[Preuves et limites du correctif C0 A4](../investigations/hardware-runs/shaker-crtc0-a4-qualified-phase-2026-11-06.md).

## Type3 : état2 bit4 et ajustement vertical

La §21.3.4.2 p249 décrit le bit4 comme toujours égal à1. DU/03C0 vérifie
bien cette valeur sur un frame normal, puis programme R4=25hex et R5=8,
effectue une nouvelle capture et rétablit R4=26hex/R5=0. Son second contrôle
attend le bit4 bas sur les 4DFF premiers échantillons, puis haut sur le dernier.

Cette attente décrit-elle une exception à la table pendant l’ajustement,
un effet de la méthode de capture, ou une autre interprétation du contrôle ?
Le collecteur reconstruit les données avec 624 lectures par phase et
32 phases ; nous ne traitons donc pas le tampon comme un seul frame continu.
Notre MiSTer et CPCSyntaxError échouent sur ce contrôle ; AmSpirit le passe
après achèvement de la mesure. Nous devons encore qualifier les lectures
et le transport du tampon avant d’attribuer ce résultat au bit physique.
Cette question concerne le bit4, distinct du bit3 et de son origine sur
seize frames.

[Captures et limites de qualification](../investigations/hardware-runs/shaker-crtc3-status-overflow-2026-11-06.md).

## Type3 : création tardive d’une égalité R4=C4

La §12.5 p103 décrit l’ordre entre R4 et le retour de C4 à0, avec une
exception liée à « ROM Select ». Avec une I/O ordinaire (bit5 de B à1),
encore active au passage de fin de ligne correspondant à ce cas, que se
passe-t-il si l’ancien R4 diffère de C4 et si la nouvelle valeur de R4 est
précisément C4 ? La trame peut-elle encore se terminer à cette frontière,
ou l’égalité ne sera-t-elle utilisée qu’à la suivante ?

Le premier couple E0406 distingue l’annulation tardive d’une égalité, mais
pas sa création. Nous conservons pour cette dernière le comportement existant
en attendant une mesure ou une clarification, sans en déduire un front
interne de comparaison. Le cas simultané « ROM Select » reste séparé.

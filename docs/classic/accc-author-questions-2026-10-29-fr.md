# Questions à l’auteur de l’ACCC — CRTC type 1

Brouillon du 29 octobre 2026, non envoyé. Rédigé par Terra 6.1, effort high,
puis relu pour conserver la distinction entre le modèle et le matériel réel.
Les références sont celles de l’ACCC v1.11 française.

---

**Objet : Deux clarifications sur la parité et la RFD du CRTC type 1**

Bonjour,

Je travaille sur un modèle du CRTC type 1 (UM6845R) du CPC classique.
Je souhaiterais clarifier deux points du Compendium.

**1. Quelle parité intervient dans la mémorisation de l’adresse ?**

Dans la section 11.6.1, page 90, la parité impaire est associée à une inhibition
de la comparaison C9/R9. Les sections 11.6.2 et 11.6.3, pages 91–92,
distinguent l’arrêt du mode IVM avant ou après la RFD.

Dans ce contexte, « parité » désigne-t-il ParitéFrame, ParitéC9, ou un terme de
comparaison qui dépend aussi de R9 ? La règle diffère-t-elle selon que R9 est
pair ou impair ?

Voici le cas concret qui motive la question. Avec R9=7, nous activons IVM
(R8=3), puis le désactivons (R8=0) sur un C9 pair avant la RFD. Celle-ci est
provoquée par R5 passant de 0 à 5 puis à 0 en fin de ligne. Les deux états de
parité valent alors 0 dans notre modèle.

Nous modifions ensuite l’adresse R12/R13 à &33BD, avant le premier test
C9=7 et C0=R1. Notre modèle mémorise &1030, puis recharge cette adresse.
Notre lecture de la séquence « avant RFD » nous ferait attendre &33BD.
Interprétons-nous mal cette séquence, ou manque-t-il une condition à notre lecture ?

**2. L’inhibition continue-t-elle après l’abandon de la source R12/R13 ?**

Supposons que la parité soit dans un état qui inhibe la mémorisation.
Dans la même trame, au moment du test C9=R9, R1 est supérieur à R0.
La sélection de R12/R13 comme source d’adresse cesse, sans mémorisation réussie.

Si l’on rétablit ensuite R1 avant un test suivant, la parité peut-elle encore
empêcher la mémorisation de VMA′ ? Ou cette inhibition ne s’applique-t-elle que
tant que R12/R13 reste sélectionné ? Dans cet exemple, il n’y a ni nouvelle
trame, ni modification de R8, ni autre RFD entre les deux tests.

Nous disposons d’un diagnostic qui distingue ces deux résultats, mais nous
ne l’avons pas encore exécuté sur un CPC réel. Les parités et adresses internes
citées ci-dessus sont celles de notre modèle ; nous ne les avons pas mesurées
sur le matériel.

Merci beaucoup pour votre aide et pour le Compendium.

Bien cordialement,

---

## Références pour préparer un éventuel complément

Le détail des phases du cas DSC4 et la comparaison avec CPCSyntaxError sont
dans [le relevé DSC4](../investigations/hardware-runs/dsc4-video-2026-10-29.md).
Le programme combiné pour un futur test sur CPC classique CRTC1 sera préparé
par V3 après le travail FIELD/ASCAL. Un résultat sur 6128 Plus (CRTC3) ne
trancherait pas ces questions de CRTC1. Aucun contact avec l’auteur n’est autorisé
par la seule préparation de ce brouillon.

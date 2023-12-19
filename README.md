Dans ce code on a un serveur en langage C utilisant des sockets TCP pour établir une communication avec les clients. Le serveur fonctionne de manière concurrente, gérant chaque connexion cliente dans un thread distinct.
le choix du langage : Le langage C est connu pour sa performance et sa vitesse d'exécution et aussi son accès direct à la mémoire. Pour une application qui a des contraintes de performance (ce qui est notre cas) ou qui nécessite un accès efficace à la mémoire, le langage C est un choix approprié.
J'ai ici un Makefile qui compile le programme ser.c et gènere un exécutable ser.
Guide :
Dans le serveur on 19 commandes différentes, je vais les présenter ci-desous avec une discription de la commande avec une explication d'utilisation :
1)PING : Le serveur répond à la commande PING par PONG elle sert à tester le serveur.
2)PING 'texte' : On fait PING suivi d'un message, et le serveur renvoie ce message.
3)SET : cette commande sert à ajouter une cle valeur, et enregistre directement après l'ajout dans un fichier appeler "keys_and_values.txt", elle fonctionne comme ceci SET 'key' 'valeur'.
4)GET : une commande qui retoourne la valeur d'une clé, elle fonctionne comme ceci GET 'key'.
5)DEL : une commande qui supprime une cle valeur de la mémoire actuelle. elle fonction comme suite DEL 'key'.
6)ALLDEL : sert à effacer toutes les clés valeurs de la mémoire actuelle. il suffit de taper ALLDEL.
7)BDEL : une commande qui supprime une clé-valeur de la base de données (le fichier qui contient les clés valeurs). on fait ceci BDEL 'key'.
8)KEYS : affiche les clé existantes.
9)VALUES : affiche les valeurs existantes.
10)UPLOAD : charge les clés valeurs du fichier qui contient les clés valeurs.
11)EXISTS : sert à vérifier si une clé existe ou non. (EXISTS 'key')
12)READ : affiche le countenu de la base de données (le fichier qui contient les clés-valeurs).
13)SHOW : affiche les clés valeus de la mémoire actuelle.
14)APPEND : ajoute du countenu à une valeur d'une clé. (APPEND 'key' 'text').
15)RENAME : sert à rennomer les clés, elle fonctionne comme suite RENAME 'oldkey' 'newkey'.
16)INC : sert à incrémenter la valeur d'une clé.
17)DEC : sert à décrémenter la valeur d'une clé.
18)HELP : affiche toutes les commandes existantes.
19)EXIT : faire quitter le serveur.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>

#define HASH_TABLE_SIZE 100

//les noeuds.
struct Node
{
    char *key;
    char *value;
    struct Node *next;
};

//hash table.
struct HashTable
{
    struct Node *buckets[HASH_TABLE_SIZE];
};

struct th_info
{
    int fd;
    int i;
    struct HashTable *hash_table;
};

void free_node(struct Node *node);

unsigned int hash_function(const char *key)
{
    unsigned int hash = 0;
    while (*key)
    {
        hash = (hash << 5) + *key++;
    }
    return hash % HASH_TABLE_SIZE;
}

//une fonction qui vérifie l'existence d'une clé. 
int key_exists(struct HashTable *hash_table, const char *key)
{
    unsigned int index = hash_function(key);
    struct Node *current = hash_table->buckets[index];

    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            //clé trouvée.
            return 1;
        }
        current = current->next;
    }

    //clé pas trouvée.
    return 0;
}


//une fonction qui ajoute à la table une clé-valeur.
void add_to_hash_table(int client_fd, struct HashTable *hash_table, const char *key, const char *value)
{
	
    unsigned int index = hash_function(key);

    struct Node *new_node = malloc(sizeof(struct Node));
    if (new_node == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    new_node->key = strdup(key);
    if (new_node->key == NULL)
    {
        perror("strdup");
        free(new_node);
        exit(EXIT_FAILURE);
    }

    new_node->value = strdup(value);
    if (new_node->value == NULL)
    {
        perror("strdup");
        free(new_node->key);
        free(new_node);
        exit(EXIT_FAILURE);
    }

    new_node->next = NULL;

    if (hash_table->buckets[index] == NULL)
    {
        hash_table->buckets[index] = new_node;
    }
    else
    {
        new_node->next = hash_table->buckets[index];
        hash_table->buckets[index] = new_node;
    }
    
}

void handle_get(int client_fd, struct HashTable *hash_table, const char *key)
{
    unsigned int index = hash_function(key);
    struct Node *current = hash_table->buckets[index];

    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            //clé trouvée envoyer la valeur asociée au client.
            write(client_fd, current->value, strlen(current->value));
            char *saut_ligne = "\n";
            write(client_fd, saut_ligne, strlen(saut_ligne));
            return;
        }
        current = current->next;
    }

    //envoyer un message au client.
    const char *message = "Key not found\n";
    write(client_fd, message, strlen(message));
}

//fonction pour supprimer une clé-valeur.
void handle_del(int client_fd, struct HashTable *hash_table, const char *key)
{
    unsigned int index = hash_function(key);
    struct Node *current = hash_table->buckets[index];
    struct Node *prev = NULL;

    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            
            if (prev == NULL)
            {
                hash_table->buckets[index] = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            //libération.
            free_node(current);
            return;
        }

        prev = current;
        current = current->next;
    }

    //envoyer un message au client
    const char *message = "Key not found\n";
    write(client_fd, message, strlen(message));
}
//free les noeuds.
void free_node(struct Node *node)
{
    free(node->key);
    free(node->value);
    free(node);
}
//free hash table.
void free_hash_table(struct HashTable *hash_table)
{
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        struct Node *current = hash_table->buckets[i];
        while (current != NULL)
        {
            struct Node *next = current->next;
            free_node(current);
            current = next;
        }
    }
    free(hash_table);
}

//fonction vérifie si une clé exist déjà dans le fichier.
int key_value_exists_in_file(const char *key, const char *value, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        return -1; //erreur d'ouverture du fichier.
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char current_key[128];
        //char current_value[128];

        if (sscanf(line, "%127s [^\n]", current_key) == 1)
        {
            if (strcmp(current_key, key) == 0)
            {
                fclose(file);
                return 1; //la paire clé-valeur déja existe.
            }
        }
    }

    fclose(file);
    return 0; //clé valeur existe pas.
}

//fonction pour supprimer de fichier.
void delete_key_from_file(const char *key, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        return; // Erreur d'ouverture du fichier
    }

    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.temp", filename);

    FILE *temp_file = fopen(temp_filename, "w");
    if (temp_file == NULL)
    {
        perror("fopen");
        fclose(file);
        return; 
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char current_key[128];

        if (sscanf(line, "%127s", current_key) == 1 && strcmp(current_key, key) != 0)
        {
            fputs(line, temp_file);
        }
    }

    fclose(file);
    fclose(temp_file);

    if (rename(temp_filename, filename) != 0)
    {
        perror("rename");
    }
}
//une fonction pour sauvegarder dans un fichier.
void save_to_file(int client_fd, struct HashTable *hash_table, const char *key, const char *value, const char *filename)
{
    //Verifier si cle valeur existe déjà dans le fichier.
    int exists = key_value_exists_in_file(key, value, filename);
    if (exists == -1)
    {
        printf("Erreur lors de l'ouverture du fichier.\n");
        return;
    }
    else if (exists == 1)
    {
        printf("La paire clé-valeur existe déjà. Aucune modification apportée.\n");
        delete_key_from_file(key, filename);
    }

    FILE *file = fopen(filename, "a");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    fprintf(file, "%s %s\n", key, value);

    fclose(file);
}





//fonction qui charge à partir du fichier.
void upload_and_display_from_file(struct HashTable *hash_table, const char *filename, int client_fd)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    char line[256];

    while ((fgets(line, sizeof(line), file)) != NULL)
    {
        
        char key[128];
        char value[128]; 

        if (sscanf(line, "%127s %127[^\n]", key, value) == 2)
        {
	    
            add_to_hash_table(client_fd, hash_table, key, value);
            
            
            write(client_fd, key, strlen(key));
            char *space = "\t";
            write(client_fd, space, strlen(space));

            
            write(client_fd, value, strlen(value));
            char *newline = "\n";
            write(client_fd, newline, strlen(newline));
            
        }
    }

    fclose(file);
}

//fonction pour afficher le countenu du fichier.
void read_and_write_file(const char *filename, int client_fd)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    char buffer[256]; 
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {    
    	write(client_fd, buffer, bytesRead);
    }

    fclose(file);
}

//fonction append.
void handle_append(int client_fd, struct HashTable *hash_table, const char *key, const char *value)
{
    unsigned int index = hash_function(key);
    struct Node *current = hash_table->buckets[index];

    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            size_t new_value_len = strlen(current->value) + strlen(value) + 1;
            char *new_value = malloc(new_value_len);
            if (new_value == NULL)
            {
                perror("malloc");
                //handle error.
            }

            strcpy(new_value, current->value);
            strcat(new_value, value);

            free(current->value);

            current->value = new_value;

            save_to_file(client_fd, hash_table, key, new_value, "keys_and_values.txt");
            const char *message = "APPEND command executed successfully\n";
            write(client_fd, message, strlen(message));
            return;
        }
        current = current->next;
    }

    const char *message = "Key not found. Use SET to create a new key-value.\n";
    write(client_fd, message, strlen(message));
}



//section rename




//fonction rename in the file.
void rename_key_in_file(const char *old_key, const char *new_key, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        return; // Erreur d'ouverture du fichier
    }

    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.temp", filename);

    FILE *temp_file = fopen(temp_filename, "w");
    if (temp_file == NULL)
    {
        perror("fopen");
        fclose(file);
        return; // Erreur d'ouverture du fichier temporaire
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char current_key[128];
        char current_value[128];

        if (sscanf(line, "%127s %127[^\n]", current_key, current_value) == 2)
        {
            if (strcmp(current_key, old_key) == 0)
            { 
                fprintf(temp_file, "%s %s\n", new_key, current_value);
            }
            else
            {
                fprintf(temp_file, "%s %s\n", current_key, current_value);
            }
        }
    }

    fclose(file);
    fclose(temp_file);

    if (rename(temp_filename, filename) != 0)
    {
        perror("rename");
    }
}

//fonction rename dans la hash table.
void rename_key(struct HashTable *hash_table, const char *old_key, const char *new_key)
{
    unsigned int old_index = hash_function(old_key);
    unsigned int new_index = hash_function(new_key);

    struct Node *current = hash_table->buckets[old_index];
    struct Node *prev = NULL;

    while (current != NULL)
    {
        if (strcmp(current->key, old_key) == 0)
        {
            
            if (prev == NULL)
            {
                hash_table->buckets[old_index] = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            current->next = hash_table->buckets[new_index];
            hash_table->buckets[new_index] = current;

            free(current->key); 
            current->key = strdup(new_key);

            return;
        }

        prev = current;
        current = current->next;
    }

}



// fonction affichage contenu du file.
void read_from_file(const char *filename, int client_fd)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    char line[256];

    while ((fgets(line, sizeof(line), file)) != NULL)
    {
        // Utilisez sscanf pour extraire la clé et la valeur à partir de la ligne
        char key[128];
        char value[128];

        if (sscanf(line, "%127s %127[^\n]", key, value) == 2)
        {
            // Envoyer la clé au client
            write(client_fd, key, strlen(key));
            char *space = "\t";
            write(client_fd, space, strlen(space));

            // Envoyer la valeur au client
            write(client_fd, value, strlen(value));
            char *newline = "\n";
            write(client_fd, newline, strlen(newline));
        }
    }

    fclose(file);
}


//increment function

void increment_value_if_integer(int client_fd, struct HashTable *hash_table, const char *key) {
    // Trouver le nœud correspondant à la clé
    unsigned int index = hash_function(key);
    struct Node *current = hash_table->buckets[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            // Si la clé est trouvée

            // Vérifier si la valeur est un nombre entier
            char *value = current->value;
            char *endptr;
            long intValue = strtol(value, &endptr, 10);

            if (*endptr == '\0') {  // Si endptr pointe à la fin de la chaîne, alors c'est un entier
                intValue++;
                // Allouer de la mémoire pour la nouvelle valeur
                char *new_value = (char *)malloc(sizeof(char) * (strlen(value) + 1));
                sprintf(new_value, "%ld", intValue);
                
                // Mettre à jour la valeur du nœud avec la nouvelle valeur
                current->value = new_value;

                // Afficher le message d'incrémentation
                printf("La valeur de la clé '%s' a été incrémentée à %ld.\n", key, intValue);
                save_to_file(client_fd, hash_table, key, new_value, "keys_and_values.txt");//à voir

                // Libérer la mémoire de l'ancienne valeur
                free(value);

            } else {
                // Sinon, afficher une erreur
                const char *message = "Erreur : la valeur de cette clé n'est pas un nombre entier.\n";
                write(client_fd, message, strlen(message));
            }

            return; // Sortir de la boucle après avoir traité la clé
        }
        current = current->next;
    }

    // Si la clé n'est pas trouvée, afficher une erreur
    printf("Erreur : la clé '%s' n'existe pas.\n", key);
    const char *message = "Erreur : la clé '%s' n'existe pas.\n";
    write(client_fd, message, strlen(message));
}

//decrement function

void decrement_value_if_integer(int client_fd, struct HashTable *hash_table, const char *key) {
    unsigned int index = hash_function(key);
    struct Node *current = hash_table->buckets[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {

            
            char *value = current->value;
            char *endptr;
            long intValue = strtol(value, &endptr, 10);

            if (*endptr == '\0') { 
                intValue--;
                char *new_value = (char *)malloc(sizeof(char) * (strlen(value) + 1));
                sprintf(new_value, "%ld", intValue);
                
                current->value = new_value;

                printf("La valeur de la clé '%s' a été decrémentée à %ld.\n", key, intValue);
                save_to_file(client_fd, hash_table, key, new_value, "keys_and_values.txt");

                free(value);

            } else {
                const char *message = "Erreur : la valeur de cette clé n'est pas un nombre entier.\n";
                write(client_fd, message, strlen(message));
            }

            return; 
        }
        current = current->next;
    }

   
    printf("Erreur : la clé '%s' n'existe pas.\n", key);
    const char *message = "Erreur : la clé '%s' n'existe pas.\n";
    write(client_fd, message, strlen(message));
}

//clear hash table function.
void clear_hash_table(struct HashTable *hash_table) {
   
    for (unsigned int i = 0; i < HASH_TABLE_SIZE; ++i) {
        struct Node *current = hash_table->buckets[i];

        while (current != NULL) {
            struct Node *temp = current;
            current = current->next;

            free_node(temp);
        }

        hash_table->buckets[i] = NULL;
    }
}




//client
void *handle_client(void *pctx)
{
    struct th_info *ctx = (struct th_info *)pctx;
    int client_fd = ctx->fd;

    while (1)
    {
        char buffer[128];
        ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);

        int rec = bytes_received - 1;

        //printf("%d\n", rec);
        if (rec > 0)
        {
            buffer[rec] = '\0'; // Ajoutez le terminateur null à la fin du buffer.

            printf("Received command: %s\n", buffer);
		printf("Number of bits received : %d\n", rec);
            if (strncmp(buffer, "PING", 4) == 0)
            {
                if (buffer[4] == ' ')
                {

                    const char *pong_message = &buffer[5];
                    printf("Sending response: %s\n", pong_message);
                    write(client_fd, pong_message, strlen(pong_message));
                    char *saut_ligne = "\n";
            	    write(client_fd, saut_ligne, strlen(saut_ligne));
                }
                else if (buffer[4] == '\0')
                {

                    const char *pong_message = "PONG\n";
                    printf("Sending response: %s\n", pong_message);
                    write(client_fd, pong_message, strlen(pong_message));
                }
            }
            else if (strncmp(buffer, "EXIT", 4) == 0)
            {
                printf("Client requested exit. Closing connection...\nConnection Closed\n");
                const char *message = "Connection Closed\n";
                write(client_fd, message, strlen(message));
                close(client_fd);
                break;
            }
            else if (strncmp(buffer, "SET", 3) == 0)
            {
                char *key = NULL;
                char *value = NULL;

                char *command = strtok(buffer, " ");

                if (command != NULL)
                {
                    command = strtok(NULL, " ");
                    if (command != NULL)
                    {
                        printf("Clé: %s\n", command);
                        key = strdup(command);

                        command = strtok(NULL, "\0");
                        if (command != NULL)
                        {
                            printf("Valeur: %s\n", command);
                            value = strdup(command);
                        }
                        else
                        {
                            printf("Erreur: Aucune valeur spécifiée.\n");
                        }
                    }
                    else
                    {
                        printf("Erreur: Aucune clé spécifiée.\n");
                    }
                }

                if (key != NULL && value != NULL)
                {
                	if (key_exists(ctx->hash_table, key))
   			 {
        		// Clé existe déjà, affichez un message ou faites ce que vous voulez
        		printf("La clé '%s' existe déjà. Aucune modification apportée.\n", key);
        		char *message = "La clé existe déjà. Sa valeur ancienne est écrasée.\n";
       			write(client_fd, message, strlen(message));
       			//
       			handle_del(client_fd, ctx->hash_table, key);
       			add_to_hash_table(client_fd, ctx->hash_table, key, value);
    			save_to_file(client_fd, ctx->hash_table, key, value, "keys_and_values.txt");
    			//
    			}
    			else {
    			add_to_hash_table(client_fd, ctx->hash_table, key, value);
    			save_to_file(client_fd, ctx->hash_table, key, value, "keys_and_values.txt");
    			}
    			 
                }
                else
                {
                    const char *message = "Invalid SET command\n";
                    write(client_fd, message, strlen(message));
                }
                free(key);
                free(value);
            }
            else if (strncmp(buffer, "GET", 3) == 0)
            {
                char *key = NULL;

                char *command = strtok(buffer, " ");

                if (command != NULL)
                {
                    command = strtok(NULL, " ");
                    if (command != NULL)
                    {
                        printf("Clé: %s\n", command);
                        key = strdup(command);
                      	handle_get(client_fd, ctx->hash_table, key);
                    }
                }
                else
                {
                    const char *message = "Invalid GET command\n";
                    write(client_fd, message, strlen(message));
                }
            }
            else if (strncmp(buffer, "DEL", 3) == 0)
            {
                char *key = NULL;

                char *command = strtok(buffer, " ");

                if (command != NULL)
                {
                    command = strtok(NULL, " ");
                    if (command != NULL)
                    {
                        printf("Clé: %s\n", command);
                        key = strdup(command);
                   	handle_del(client_fd, ctx->hash_table, key);
                   	const char *message = "DEL command executed successfully\n";
            		write(client_fd, message, strlen(message));
                    }
                }
                else
                {
                    const char *message = "Invalid DEL command\n";
                    write(client_fd, message, strlen(message));
                }
            }
            
            else if (strncmp(buffer, "KEYS", 9) == 0)
	   {
    		
   		 for (int i = 0; i < HASH_TABLE_SIZE; i++)
    		{
       		 struct Node *current = ctx->hash_table->buckets[i];
        		while (current != NULL)
        		{
         		   
          		  write(client_fd, current->key, strlen(current->key));
            		  char *newline = "\n";
          		  write(client_fd, newline, strlen(newline));

          		  current = current->next;
        		}
    		}
		}
            
            else if (strncmp(buffer, "VALUES", 6) == 0)
		{
    			for (int i = 0; i < HASH_TABLE_SIZE; i++)
    			{
     			   struct Node *current = ctx->hash_table->buckets[i];
      			   while (current != NULL)
       		 	   {
           		 	 write(client_fd, current->value, strlen(current->value));
           		 	 char *newline = "\n";
           			 write(client_fd, newline, strlen(newline));

          			 current = current->next;
       			   }
   		        }
		}
		
		else if (strncmp(buffer, "UPLOAD", 6) == 0)
		{
    			upload_and_display_from_file(ctx->hash_table, "keys_and_values.txt", client_fd);
		}
		
            else if (strncmp(buffer, "SHOW", 4) == 0)
	    {
	    	
    		for (int i = 0; i < HASH_TABLE_SIZE; i++)
    		{
        		struct Node *current = ctx->hash_table->buckets[i];
        		while (current != NULL)
        		{
            		char response[256];
            		snprintf(response, sizeof(response), "Key: %s, Value: %s\n", current->key, current->value);

            		write(client_fd, response, strlen(response));

           		current = current->next;
        		}
    		}
	   }
	   
	   else if (strncmp(buffer, "EXISTS", 6) == 0)
{
    char *key = NULL;

    char *command = strtok(buffer, " ");

        command = strtok(NULL, " ");
        if (command != NULL)
        {
            printf("Clé: %s\n", command);
            key = strdup(command);

            int exists = key_exists(ctx->hash_table, key);

            if (exists)
            {
                const char *response = "Key exists\n";
                write(client_fd, response, strlen(response));
            }
            else
            {
                const char *response = "Key does not exist\n";
                write(client_fd, response, strlen(response));
            }
        }

    free(key);
}

	   else if (strncmp(buffer, "HELP", 4) == 0)
	    {
    		const char *message = "Commands are:\n 1)PING \n 2)PING 'Text' \n 3)SET 'key' 'value' \n 4)GET 'key' \n 5)DEL 'key' \n 6)ALLDEL \n 7)BDEL 'key' \n 8)KEYS \n 9)VALUES \n 10)UPLOAD \n 11)SHOW \n 12)READ \n 13)EXISTS \n 14)APPEND 'key' 'txt'\n 15)RENAME 'oldkey' 'newkey' \n 16)DEC 'key' \n 17)INC 'key' \n 18)HELP \n 19)EXIT\n";
    		write(client_fd, message, strlen(message));
	   }
	   
	   else if (strncmp(buffer, "APPEND", 6) == 0)
{
    char *key = NULL;
    char *value = NULL;

    char *command = strtok(buffer, " ");

        command = strtok(NULL, " ");
        if (command != NULL)
        {
            printf("Clé: %s\n", command);
            key = strdup(command);

            command = strtok(NULL, "\0");
            if (command != NULL)
            {
                printf("Valeur à ajouter : %s\n", command);
                value = strdup(command);
            }
            else
            {
                printf("Erreur: Aucune valeur spécifiée.\n");
            }
        }
        else
        {
            printf("Erreur: Aucune clé spécifiée.\n");
        }

    if (key != NULL && value != NULL)
    {
        handle_append(client_fd, ctx->hash_table, key, value);
        
    }
    else
    {
        const char *message = "Invalid APPEND command\n";
        write(client_fd, message, strlen(message));
    }

    free(key);
    free(value);
}
else if (strncmp(buffer, "RENAME", 6) == 0)
{
    char *old_key = NULL;
    char *new_key = NULL;

    char *command = strtok(buffer, " ");
    if (command != NULL)
    {
        command = strtok(NULL, " ");
        if (command != NULL)
        {
            printf("Ancienne clé: %s\n", command);
            old_key = strdup(command);

            command = strtok(NULL, "\0");
            if (command != NULL)
            {
                printf("Nouvelle clé : %s\n", command);
                new_key = strdup(command);
            }
            else
            {
                printf("Erreur: Aucune nouvelle clé spécifiée.\n");
            }
        }
        else
        {
            printf("Erreur: Aucune ancienne clé spécifiée.\n");
        }
    }
    else
    {
        printf("Erreur: Commande RENAME mal formée.\n");
    }

    if (old_key != NULL && new_key != NULL)
    {
        rename_key(ctx->hash_table, old_key, new_key);//
        rename_key_in_file(old_key, new_key, "keys_and_values.txt");
    }

    free(old_key);
    free(new_key);
    }
	
	else if (strncmp(buffer, "READ", 4) == 0)
	    {
	    	
    		read_from_file("keys_and_values.txt", client_fd);
	    }
	   
	
	 
	 else if (strncmp(buffer, "INC", 3) == 0)
	 {
	    char *key = NULL;
    	    char command[256];

	    	if (sscanf(buffer, "%*s %255s", command) == 1) {
	        key = strdup(command);
	        increment_value_if_integer(client_fd, ctx->hash_table, key);
	
	       
	        free(key);
	    } else {
	        printf("Erreur : syntaxe incorrecte pour la commande INC.\n");
	    }
	}
	
	else if (strncmp(buffer, "DEC", 3) == 0)
	 {
	    char *key = NULL;
    	    char command[256]; 

	    	if (sscanf(buffer, "%*s %255s", command) == 1) {
	        key = strdup(command);
	
	        decrement_value_if_integer(client_fd, ctx->hash_table, key);
	        free(key);
	    } else {
	        printf("Erreur : syntaxe incorrecte pour la commande DEC.\n");
	    }
	}
	
	
	 else if (strncmp(buffer, "BDEL", 4) == 0)
            {
                char *key = NULL;

                char *command = strtok(buffer, " ");

                
                    command = strtok(NULL, " ");
                    if (command != NULL)
                    {
                        printf("Clé: %s\n", command);
                        key = strdup(command);
                   	delete_key_from_file(key, "keys_and_values.txt");
                   	const char *message = "DELB command executed successfully\n";
            		write(client_fd, message, strlen(message));
                    }
                
            }
            
            else if (strncmp(buffer, "ALLDEL", 4) == 0)
            {
                clear_hash_table(ctx->hash_table);
            }



	   
		
            else
            {
                printf("Unknown Command\n");
                const char *message = "Unknown Command, Commands are:\n 1)PING \n 2)PING 'Text' \n 3)SET 'key' 'value' \n 4)GET 'key' \n 5)DEL 'key' \n 6)ALLDEL \n 7)BDEL 'key' \n 8)KEYS \n 9)VALUES \n 10)UPLOAD \n 11)SHOW \n 12)READ \n 13)EXISTS \n 14)APPEND 'key' 'txt'\n 15)RENAME 'oldkey' 'newkey' \n 16)DEC 'key' \n 17)INC 'key' \n 18)HELP \n 19)EXIT\n";
                if (write(client_fd, message, strlen(message)) < 0)
                {
                    perror("write");
                    break;
                }
            }
        }
        else
        {
            printf("Error writing, Client didn't enter anything\n");
            const char *message = "Error writing, You didn't enter anything\n";
            write(client_fd, message, strlen(message)); 
        }
        const char *message = "\n";
	write(client_fd, message, strlen(message));
    }
    
    free(ctx);

    return NULL;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *result = NULL;

    int ret = getaddrinfo(NULL, argv[1], &hints, &result);

    if (ret < 0)
    {
        herror("getaddrinfo");
        return 1;
    }

    int sock = 0;
    int server_ready = 0;

    struct addrinfo *tmp;
    for (tmp = result; tmp != NULL; tmp = tmp->ai_next)
    {
        sock = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);

        if (sock < 0)
        {
            perror("socket");
            continue;
        }

        if (bind(sock, tmp->ai_addr, tmp->ai_addrlen) < 0)
        {
            perror("bind");
            continue;
        }

        if (listen(sock, 5) < 0)
        {
            perror("listen");
            continue;
        }

        server_ready = 1;
        break;
    }

    freeaddrinfo(result);

    if (server_ready == 0)
    {
        fprintf(stderr, "PAS SERVER READY :-'(");
        return 1;
    }

    sleep(1);

    int client_fd = -1;

    struct sockaddr client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct HashTable *hash_table = malloc(sizeof(struct HashTable));
    if (hash_table == NULL)
    {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        hash_table->buckets[i] = NULL;
    }
	
    	
	
    while (1)
    {
        client_fd = accept(sock, &client_addr, &client_addr_len);

        if (client_fd < 0)
        {
            perror("accept");
            break;
        }

        struct th_info *ctx = malloc(sizeof(struct th_info));
        if (ctx == NULL)
        {
            perror("malloc");
            //handle l'erreur.
        }
        ctx->fd = client_fd;
        ctx->i = 0;
        ctx->hash_table = hash_table;

        pthread_t th;
        if (pthread_create(&th, NULL, handle_client, (void *)ctx) != 0)
        {
            perror("pthread_create");
            //handle l'erreur.
        }
    }

    free_hash_table(hash_table);

    close(sock);

    return 0;
}


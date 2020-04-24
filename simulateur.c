#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

int main(int argc, char *argv[]){

  if(argc != 5){
    perror("Erreur, nombre incorrect d'argument. Quatre arguments sont attendus : le nom de la cliente, le nom du vendeur, le nom de la caissiere et le nom du produit.\n");
    exit(1);
  }
  char *cliente = argv[1];
  char *vendeur = argv[2];
  char *caissiere = argv[3];
  char *article = argv[4];
  
  if(strcmp(cliente, "Chloe") && strcmp(cliente, "Elise") && strcmp(cliente, "Lea")){
    perror("Erreur, le premier argument doit etre le nom de la cliente :\nChloe, Elise ou Lea. Attention a l'ordre des arguments !\n");
    exit(1);
  }
  if(strcmp(vendeur, "Pierre") && strcmp(vendeur, "Paul") && strcmp(vendeur, "Jacques")){
    perror("Erreur, le deuxieme argument doit etre le nom du vendeur :\nPierre, Paul ou Jacques. Attention a l'ordre des arguments !\n");
    exit(1);
  }
  if(strcmp(caissiere, "Lilou") && strcmp(caissiere, "Laura") && strcmp(caissiere, "Nadia")){
    perror("Erreur, le troisieme argument doit etre le nom de la caissiere :\nLilou, Laura ou Nadia. Attention a l'ordre des arguments !\n");
    exit(1);
  }
  if(strcmp(article, "body") && strcmp(article, "brassiere") && strcmp(article, "pyjama")){
    perror("Erreur, le quatrieme argument doit etre le nom de l'article :\nbody, brassiere ou pyjama. Attention a l'ordre des arguments !\n");
    exit(1);
  }
  int pfdClienteWrite[2], pfdClienteRead[2];
  int pfdVendeurWrite[2], pfdVendeurRead[2];
  int pfdCaissiereWrite[2], pfdCaissiereRead[2];
  int nb;
  char *buffer[100];
  pipe(pfdClienteWrite);
  pipe(pfdClienteRead);
  pipe(pfdVendeurWrite);
  pipe(pfdVendeurRead);
  pipe(pfdCaissiereWrite);
  pipe(pfdCaissiereRead);
  pid_t pidCliente, pidVendeur, pidCaissiere;

  if((pidCliente = fork()) == -1){
    perror("Erreur lors du fork\n");
    exit(1);
  }
  if(pidCliente != 0){
    close(pfdClienteWrite[0]);
    close(pfdClienteRead[1]);

    if((pidVendeur = fork()) == -1){
      perror("Erreur lors du fork\n");
      exit(1);
    }
    if(pidVendeur != 0){
      close(pfdVendeurWrite[0]);
      close(pfdVendeurRead[1]);

      if((pidCaissiere = fork()) == -1){
	perror("Erreur lors du fork\n");
	exit(1);
      }
      if(pidCaissiere != 0){
	// PROCESSUS PERE
	close(pfdCaissiereWrite[0]);
	close(pfdCaissiereRead[1]);
	int bodyStock = 10, brassiereStock = 10, pyjamaStock = 10;
	
	// recevoir le Bonjour du vendeur
	nb = read(pfdVendeurRead[0], buffer, sizeof(buffer));
	printf("%s", buffer);
	// envoyer ce Bonjour a la cliente
	write(pfdClienteWrite[1], buffer, strlen(buffer)+1);
	// recevoir le Bonjour de la cliente
	nb = read(pfdClienteRead[0], buffer, sizeof(buffer));
	printf("%s", buffer);
	// envoyer ce Bonjour au vendeur
	write(pfdVendeurWrite[1], buffer, strlen(buffer)+1);
	// recevoir la question du vendeur
	nb = read(pfdVendeurRead[0], buffer, sizeof(buffer));
	printf("%s", buffer);
	// envoyer cette question a la cliente
	write(pfdClienteWrite[1], buffer, strlen(buffer)+1);
	// recevoir la reponse de la cliente
	nb = read(pfdClienteRead[0], buffer, sizeof(buffer));
	printf("%s", buffer);
	// isoler l'article de la chaine de caracteres
	char *articleName = strchr(buffer, '/') + 5;
	// verifier que le stock n'est pas epuise et le mettre a jour
	if(strcmp(articleName, "body") == 0){
	  if(bodyStock <= 0){
	    perror("Erreur, l'article body est en rupture de stock.\n");
	    exit(1);
	  }
	  else
	    bodyStock--;
	}
	else if(strcmp(articleName, "brassiere") == 0){
	  if(brassiereStock <= 0){
	    perror("Erreur, l'article brassiere en rupture de stock.\n");
	    exit(1);
	  }
	  else
	    brassiereStock--;
	}
	else{
	  if(pyjamaStock <= 0){
	    perror("Erreur, l'article pyjama en rupture de stock.\n");
	    exit(1);
	  }
	  else
	    pyjamaStock--;
	}
	// dire au vendeur le nom de l'article
	write(pfdVendeurWrite[1], buffer, strlen(buffer)+1);
	// recevoir la reponse du vendeur
	nb = read(pfdVendeurRead[0], buffer, sizeof(buffer));
	printf("%s", buffer);
	// la cliente recoit l'article
	write(pfdClienteWrite[1], buffer, strlen(buffer)+1);
      }
      else{
	// LA CAISSIERE
	close(pfdCaissiereWrite[1]);
	close(pfdCaissiereRead[0]);
	// close aussi les pipes cliente et vendeur ??
      }
    }
    else{
      // LE VENDEUR
      close(pfdVendeurWrite[1]);
      close(pfdVendeurRead[0]);
      // dire Bonjour a la cliente
      char *chaine = vendeur;
      strcat(chaine, " : Bonjour");
      write(pfdVendeurRead[1], chaine, strlen(chaine)+1);
      // recevoir le Bonjour de la cliente
      nb = read(pfdVendeurWrite[0], buffer, strlen(buffer)+1);
      // demander a la cliente ce qui lui ferait plaisir
      chaine = vendeur;
      strcat(chaine, " : Qu'est ce qui vous ferait plaisir ?");
      write(pfdVendeurRead[1], chaine, strlen(chaine)+1);
      // recevoir la reponse de la cliente
      nb = read(pfdVendeurWrite[0], buffer, strlen(buffer)+1);
      // isoler l'article de la chaine de caracteres
      char *articleName = strchr(buffer, '/') + 5;
      // tendre l'article a la cliente
      chaine = vendeur;
      strcat(chaine, " : Tenez, voici un/une ");
      strcat(chaine, articleName);
      write(pfdVendeurRead[1], chaine, strlen(chaine)+1);
    }
  }
  else{
    // LA CLIENTE
    close(pfdClienteWrite[1]);
    close(pfdClienteRead[0]);
    int body=0, brassiere=0, pyjama=0;
    
    // recevoir le Bonjour du vendeur
    nb = read(pfdClienteWrite[0], buffer, sizeof(buffer));
    // repondre Bonjour au vendeur
    char *chaine = cliente;
    strcat(chaine, " : Bonjour");
    write(pfdClienteRead[1], chaine, strlen(chaine)+1);
    // recevoir la question du vendeur
    nb = read(pfdClienteWrite[0], buffer, sizeof(buffer));
    // dire au vendeur le nom de l'article
    chaine = cliente;
    strcat(chaine, " : Je voudrais un/une ");
    strcat(chaine, article);
    write(pfdClienteRead[1], chaine, strlen(chaine)+1);
    // prendre l'article
    char *articleName = strchr(buffer, '/') + 5;
    if(strcmp(articleName, "body") == 0)
      body++;
    else if(strcmp(articleName, "brassiere") == 0)
      brassiere++;
    else
      pyjama++;
    // tendre l'article a la caissiere
    chaine = cliente;
    strcat(chaine, " tend a la caissiere le / la ");
    write(pfdClienteRead[1], chaine, strlen(chaine)+1);
  }
}

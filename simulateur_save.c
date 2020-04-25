#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>


struct Canaux
{
    int vendeur_to_main[2];
    int main_to_vendeur[2];
    int cliente_to_main[2];
    int main_to_cliente[2];
    int caissiere_to_main[2];
    int main_to_caissiere[2];
};

struct Sac
{
    char produit[100];
    int montant_ticket; // en euros, sans centimes
};


pid_t pidCliente, pidVendeur, pidCaissiere;

const char* je_voudrais = "Je voudrais un/une ";
const char* bonjour = "Bonjour !";
const char* tu_veux_quoi = "Qu'est ce qui vous ferait plaisir ?";
const char* merci = "Merci et au revoir !";
const char* bienvenue = "Bienvenue a 'Fais DauDau', la boutique de vetements pour vos petits filous !";
const char* dommage = "Quel dommage, au revoir.";
const char* rupture_stock = "Je suis desole, nous sommes en rupture de stock de ";
const char* trop_cher = "Oh ! Je n'ai pas assez d'argent. Je reviendrais.";

const int NORMAL = 1, RUPTURE = 2, TROPCHER = 3;

int safe_fork() {
    int f = fork();
    if (f == -1) {
        perror("Erreur de creation de fork.");
        exit(1);
    }
    return f;
}

void kill_all(int sig) {
    kill(pidCliente, sig);
    kill(pidVendeur, sig);
    kill(pidCaissiere, sig);
}

void get_param(char* nom_var, char* val_var, char* dom0, char* dom1, char* dom2) {
    printf("Choisissez un.e %s entre %s, %s ou %s.\n", nom_var, dom0, dom1, dom2);
    scanf("%s", val_var);
    while (strcmp(val_var, dom0) && strcmp(val_var, dom1) && strcmp(val_var, dom2)) {
        printf("Nom de %s incorrect. Les personnes disponibles sont %s, %s et %s.\n", nom_var, dom0, dom1, dom2);
        scanf("%s", val_var);
    }
}

int inputIsInt(char *input){
  int tmp=atoi(input);
  int len=strlen(input);
  char *check = malloc(len * sizeof(char));
  sprintf(check, "%d", tmp);
  return(strcmp(input, check) == 0 ? 1 : 0);
}

void choix_scenario(int *prixArticle, int *argentCliente, int *stockArticle){
  printf("Nous vous proposons trois scenarios possibles.\n");
  printf("Si vous desirez le scenario 'Dans le meilleur des mondes', tapez 1.\n");
  printf("Si vous voulez le scenario 'Penurie de guerre', tapez 2.\n");
  printf("Si vous desirez le scenario 'Crise des subprimes', tapez 3.\n");
  printf("Si vous preferez choisir l'argent de la cliente et le prix de l'article, tapez 4.\n");
  printf("Si vous voulez que le gosse du voisin se taise, tapez le !\n\nOption : ");
  int ans;
  while(1){
    scanf("%d", &ans);
    if(ans == 1 || ans == 2 || ans == 3 || ans == 4)
      break;
    printf("\nOption non reconnue. Tapez un entier entre 1 et 4 : ");
  }
  switch(ans)
    {
    case 1:
      *prixArticle = 15;
      *argentCliente = 20;
      *stockArticle = 6;
      break;
      
    case 2:
      *prixArticle = 15;
      *argentCliente = 20;
      *stockArticle = 0;
      break;
      
    case 3:
      *prixArticle = 15;
      *argentCliente = 10;
      *stockArticle = 6;
      break;
      
    case 4:
      while(1){
	printf("\nEntrez l'argent de la cliente (entier attendu) : ");
	char strTemp[1024];
	scanf("%s", strTemp);
	if(inputIsInt(strTemp)) {
	  *argentCliente=atoi(strTemp);
	  break;
	}
	printf("\n%s n'est pas un entier. Reessayez !", strTemp);
      }
      while(1){
	printf("\nEntrez le prix de l'article (entier attendu) : ");
	char strTemp[1024];
	scanf("%s", strTemp);
	if(inputIsInt(strTemp)) {
	  *prixArticle=atoi(strTemp);
	  break;
	}
	printf("\n%s n'est pas un entier. Reessayez !", strTemp);
      }
      while(1){
	printf("\nEntrez le nombre d'articles en stock (entier attendu) : ");
	char strTemp[1024];
	scanf("%s", strTemp);
	if(inputIsInt(strTemp)) {
	  *stockArticle=atoi(strTemp);
	  printf("\n");
	  break;
	}
	printf("%\ns n'est pas un entier. Reessayez !", strTemp);
      }
    }
}


void safe_destock(char* nom_var, int* var_pt) {
    // verifier que le stock n'est pas epuise et le mettre a jour

    if (*var_pt <= 0) {
        char buf[1024];

        sprintf(buf, "Erreur, l'article %s est en rupture de stock.\n", nom_var);
        perror(buf);
        exit(1);
    }
    else{
        (*var_pt)--;
    }
}


char* echange(char* nom_parle, char* action, char* nom_ecoute, int canal_parle[], int canal_ecoute[]) {
    static char buffer[100];
    int len;

    len = read(canal_parle[0], buffer, sizeof(buffer));
    printf("%s %s a %s: %s\n", nom_parle, action, nom_ecoute, buffer);
    write(canal_ecoute[1], buffer, len);

    return buffer;
}


void echange_sac(char* nom_parle, char* nom_ecoute, int canal_parle[], int canal_ecoute[]) {
    struct Sac sac;
    int len;

    len = read(canal_parle[0], &sac, sizeof(sac));
    printf("%s donne un sac a %s contenant l'article %s et un ticket de montant %i.\n",
            nom_parle, nom_ecoute, sac.produit, sac.montant_ticket);
    write(canal_ecoute[1], &sac, len);
}

void echange_argent(char* nom_parle, char* nom_ecoute, int canal_parle[], int canal_ecoute[]) {
    int argent;
    int len;
    char argentStr[100];

    len = read(canal_parle[0], argentStr, sizeof(argentStr));
    argent = atoi(argentStr);
    printf("%s paye %i euros a %s.\n", nom_parle, argent, nom_ecoute);
    write(canal_ecoute[1], argentStr, len);
}

void processus_main(struct Canaux* c, char* cliente, char* vendeur, char* caissiere,
		    int prixArticle, int stockArticle, int argentCliente) {
    char buffer[100];
    char *phrase, *produit, prix[100];
    sprintf(prix, "%d", prixArticle);
    int bodyStock = stockArticle, brassiereStock = stockArticle, pyjamaStock = stockArticle;

    // le vendeur dit bonjour a la cliente
    echange(vendeur, "dit", cliente, (*c).vendeur_to_main,(*c).main_to_cliente);

    // la cliente repond bonjour au vendeur
    echange(cliente, "dit", vendeur, (*c).cliente_to_main, (*c).main_to_vendeur);

    // le vendeur demande a la cliente ce qui lui ferait plaisir
    echange(vendeur, "dit", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

    // la cliente dit le nom de l'article
    phrase = echange(cliente, "dit", vendeur, (*c).cliente_to_main, (*c).main_to_vendeur);
    produit = phrase + strlen(je_voudrais);

    if (stockArticle == 0) { // scenario rupture de stock
        // le vendeur annonce une rupture de stock
        echange(vendeur, "dit", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

        // la cliente est desolee et dit au revoir
        echange(cliente, "dit", vendeur, (*c).cliente_to_main, (*c).main_to_vendeur);

        // le vendeur est desole et dit au revoir
        echange(vendeur, "dit", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

        // fin de la simulation
        exit(0);
    }

    // verifier que le stock n'est pas epuise et le mettre a jour
    if (strcmp(produit, "body") == 0) {
        safe_destock("body", &bodyStock);
    }
    else if(strcmp(produit, "brassiere") == 0){
        safe_destock("brassiere", &brassiereStock);
    }
    else {
        safe_destock("pyjama", &pyjamaStock);
    }

    // le vendeur tend l'article a la cliente
    echange(vendeur, "donne", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

    // la cliente tend l'article a la caissiere
    echange(cliente, "donne", caissiere, (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissiere cherche le prix du produit
    read((*c).caissiere_to_main[0], buffer, sizeof(buffer));
    if(strcmp(buffer, "body") == 0){}
    write((*c).main_to_caissiere[1], prix, strlen(prix) + 1);

    // la caissiere annonce le total a payer a la cliente
    echange(caissiere, "dit" , cliente, (*c).caissiere_to_main, (*c).main_to_cliente);

    if (argentCliente < prixArticle) { // scenario crise economique
        // la cliente n'a pas assez d'argent
        echange(cliente, "dit", caissiere, (*c).cliente_to_main, (*c).main_to_caissiere);

        // la caissiere est desolee et dit au revoir
        echange(caissiere, "dit" ,cliente, (*c).caissiere_to_main, (*c).main_to_cliente);

        // la cliente est desolee et dit au revoir
        echange(cliente, "dit", caissiere, (*c).cliente_to_main, (*c).main_to_caissiere);

        // fin de la simulation
        exit(0);
    }
    // la cliente paie a la caissiere
    echange_argent(cliente, caissiere, (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissiere remet l'article et le ticket de caisse a la cliente, dans un sac
    echange_sac(caissiere, cliente, (*c).caissiere_to_main, (*c).main_to_cliente);

    // la cliente dit merci et au revoir à la caissière
    echange(cliente, "dit", caissiere, (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissière dit merci et au revoir à la cliente
    echange(caissiere, "dit", cliente, (*c).caissiere_to_main, (*c).main_to_cliente);
}


void processus_vendeur(struct Canaux* c, int stockArticle) {
    char buffer[100];
    char phrase[100];
    char *produit;

    // le vendeur dit bonjour a la cliente
    write((*c).vendeur_to_main[1], bonjour, strlen(bonjour) + 1);

    // la cliente repond bonjour au vendeur
    read((*c).main_to_vendeur[0], buffer, sizeof(buffer));

    // le vendeur demande a la cliente ce qui lui ferait plaisir
    write((*c).vendeur_to_main[1], tu_veux_quoi, strlen(tu_veux_quoi) + 1);

    // la cliente dit le nom de l'article
    read((*c).main_to_vendeur[0], phrase, sizeof(phrase));
    produit = phrase + strlen(je_voudrais);

    if (stockArticle == 0) { // scenario rupture de stock
        // le vendeur annonce une rupture de stock
        sprintf(buffer, "%s%s. \nRevenez une prochaine fois.", rupture_stock, produit);
        write((*c).vendeur_to_main[1], buffer, strlen(buffer) + 1);

        // la cliente est desolee et dit au revoir
        read((*c).main_to_vendeur[0], buffer, sizeof(buffer));

        // le vendeur est desole et dit au revoir
        write((*c).vendeur_to_main[1], dommage, strlen(dommage) + 1);

        return;
    }
    // le vendeur tend l'article à la cliente
    write((*c).vendeur_to_main[1], produit, strlen(produit) + 1);
}


void processus_cliente(struct Canaux* c, char* article, int *argentCliente) {
    char buffer[100];
    char produit[100];
    char prix[100];
    int argentToPay;
    struct Sac sac;

    // le vendeur dit bonjour à la cliente
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));

    // la cliente repond bonjour au vendeur
    write((*c).cliente_to_main[1], bonjour, strlen(bonjour) + 1);

    // le vendeur demande a la cliente ce qui lui ferait plaisir
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));

    // la cliente dit le nom de l'article
    sprintf(buffer, "%s%s", je_voudrais, article);
    write((*c).cliente_to_main[1], buffer, strlen(buffer) + 1);

    // le vendeur lui repond
    read((*c).main_to_cliente[0], produit, sizeof(produit));
    
    if (strstr(produit, "rupture") != NULL) { // scenario rupture de stock

        // la cliente est desolee et dit au revoir
        write((*c).cliente_to_main[1], dommage, strlen(dommage) + 1);

        // le vendeur est desole et dit au revoir
        read((*c).main_to_cliente[0], buffer, sizeof(buffer));

        return;
    }
    // le vendeur tend l'article a la cliente

    // check que produit == article

    // la cliente tend l'article a la caissiere
    write((*c).cliente_to_main[1], produit, strlen(produit) + 1);

    // la caissiere annonce le total a payer a la cliente
    read((*c).main_to_cliente[0], prix, sizeof(prix));

    argentToPay = atoi(prix);
    
    if (*argentCliente < argentToPay) { // scenario crise economique
        // la cliente n'a pas assez d'argent
        write((*c).cliente_to_main[1], trop_cher, strlen(trop_cher) + 1);

        // la caissiere est desolee et dit au revoir
        read((*c).main_to_cliente[0], buffer, sizeof(buffer));

        // la cliente est desolee et dit au revoir
        write((*c).cliente_to_main[1], dommage, strlen(dommage) + 1);

        return;
    }

    // la cliente paie a la caissiere
    *argentCliente = *argentCliente - argentToPay;
    
    char prixStr[100];
    sprintf(prixStr, "%d", argentToPay);
    
    write((*c).cliente_to_main[1], prixStr, sizeof(prixStr));

    // la caissiere remet l'article et le ticket de caisse a la cliente, dans un sac
    read((*c).main_to_cliente[0], &sac, sizeof(sac));

    // la cliente dit merci et au revoir a la caissiere
    write((*c).cliente_to_main[1], merci, strlen(merci) + 1);

    // la caissiere dit merci et au revoir a la cliente
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));
}


void processus_caissiere(struct Canaux* c, int stockArticle) {
    char buffer[100];
    char produit[100];
    char prix[100];
    int len;
    int argent;
    struct Sac sac;

    if (stockArticle == 0) {
        return;
    }
        // la cliente tend l'article a la caissiere
    len = read((*c).main_to_caissiere[0], produit, sizeof(produit));
    
    // la caissiere douche l'article (cherche le prix de l'article)
    write((*c).caissiere_to_main[1], produit, len);
    len = read((*c).main_to_caissiere[0], prix, sizeof(prix));

    // la caissiere annonce le total a payer a la cliente
    char prixToPay[150];
    strcpy(prixToPay, prix);
    sprintf(prixToPay, "%s euros s'il vous plait.", prix);
    write((*c).caissiere_to_main[1], prixToPay, strlen(prixToPay)+1);

    // reponse de la cliente
    read((*c).main_to_caissiere[0], buffer, sizeof(buffer));
    
    if (strstr(buffer, "pas assez d'argent") != NULL) { // scenario crise economique
        // la cliente n'a pas assez d'argent

        // la caissiere est desolee et dit au revoir
        write((*c).caissiere_to_main[1], dommage, strlen(dommage) + 1);

        // la cliente est desolee et dit au revoir
        read((*c).main_to_caissiere[0], buffer, sizeof(buffer));

        return;
    }

    // la cliente paie a la caissiere

    argent = atoi(buffer);

    // check bonne somme

    // la caissiere remet l'article et le ticket de caisse a la cliente, dans un sac
    sac.montant_ticket = argent;
    strcpy(sac.produit, produit);
    write((*c).caissiere_to_main[1], &sac, sizeof(sac));

    // la cliente dit merci et au revoir a la caissiere
    read((*c).main_to_caissiere[0], buffer, sizeof(buffer));

    // la caissiere dit merci et au revoir a la cliente
    write((*c).caissiere_to_main[1], merci, strlen(merci) + 1);

}


int main(int argc, char *argv[]) {
    struct Canaux c;
    char cliente[100];
    char vendeur[100];
    char caissiere[100];
    char article[100];
    int argentCliente, prixArticle, stockArticle;

    if (argc != 1) {
        perror("Erreur, pas d'arguments attendus.\n");
        exit(1);
    }
    printf("%s\n", bienvenue);
    get_param("cliente", cliente, "Chloe", "Elise", "Lea");
    get_param("vendeur", vendeur, "Pierre", "Paul", "Jacques");
    get_param("caissiere", caissiere, "Lilou", "Laura", "Nadia");
    get_param("article", article, "body", "brassiere", "pyjama");

    choix_scenario(&prixArticle, &argentCliente, &stockArticle);

    pipe(c.cliente_to_main);
    pipe(c.main_to_cliente);
    pipe(c.vendeur_to_main);
    pipe(c.main_to_vendeur);
    pipe(c.caissiere_to_main);
    pipe(c.main_to_caissiere);
    
    pidCliente = safe_fork();
    if (pidCliente) {
        close(c.cliente_to_main[0]);
        close(c.main_to_cliente[1]);
        processus_cliente(&c, article, &argentCliente);
    }
    else {
        close(c.cliente_to_main[1]);
        close(c.main_to_cliente[0]);
        
        pidVendeur = safe_fork();
        if (pidVendeur){
            close(c.vendeur_to_main[0]);
            close(c.main_to_vendeur[1]);
	  processus_vendeur(&c, stockArticle);
        }
        else {
            close(c.vendeur_to_main[1]);
            close(c.main_to_vendeur[0]);

            pidCaissiere = safe_fork();
            if(pidCaissiere){
	      close(c.caissiere_to_main[0]);
	      close(c.main_to_caissiere[1]);
	      processus_caissiere(&c, stockArticle);
            }
            else{
	      close(c.caissiere_to_main[1]);
	      close(c.main_to_caissiere[0]);
	      signal(SIGKILL, kill_all);
	      processus_main(&c, cliente, vendeur, caissiere, prixArticle, stockArticle, argentCliente);
            }
        }
    }
}

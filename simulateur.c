#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>


struct Canaux
{
    int vendeur_to_main[2];
    int main_to_vendeur[2];
    int cliente_to_main[2];
    int main_to_cliente[2];
    int caissiere_to_main[2];
    int main_to_caissiere[2];
};


const char* je_voudrais = "Je voudrais un/une ";
const char* bonjour = "Bonjour !";
const char* tu_veux_quoi = "Qu'est ce qui vous ferait plaisir ?";
const char* merci = "Merci et au revoir !";

int safe_fork() {
    int f = fork();
    if (f == -1) {
        perror("Erreur de création de fork.");
        exit(1);
    }
    return f;
}

void check_param_domain(char* nom_var, char* val_var, char* dom0, char* dom1, char* dom2) {
    if (strcmp(val_var, dom0) && strcmp(val_var, dom1) && strcmp(val_var, dom2)){
        char buf[1024];

        sprintf(buf, "Erreur, le premier argument doit etre le nom de la %s :\n%s, %s ou %s."
            "\nAttention a l'ordre des arguments !\n", nom_var, dom0, dom1, dom2);
        perror(buf);
        exit(1);
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

char* echange(char* action, int canal_parle[], int canal_ecoute[]) {
    static char buffer[100];

    read(canal_parle[0], buffer, sizeof(buffer));
    printf("%s : %s\n", action, buffer);
    write(canal_ecoute[1], buffer, strlen(buffer) + 1);

    return buffer;
}

void processus_main(struct Canaux* c) {
    char buffer[100];
    char *phrase, *produit;
    int bodyStock = 10, brassiereStock = 10, pyjamaStock = 10;

    // le vendeur dit bonjour à la cliente
    echange("Vendeur dit à cliente", (*c).vendeur_to_main,(*c).main_to_cliente);

    // la cliente répond bonjour au vendeur
    echange("Cliente dit à vendeur", (*c).cliente_to_main, (*c).main_to_vendeur);

    // le vendeur demande à la cliente ce qui lui ferait plaisir
    echange("Vendeur dit à cliente", (*c).vendeur_to_main, (*c).main_to_cliente);

    // la cliente dit le nom de l'article
    phrase = echange("Cliente dit à vendeur", (*c).cliente_to_main, (*c).main_to_vendeur);
    produit = phrase + strlen(je_voudrais);

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

    // le vendeur tend l'article à la cliente
    echange("Vendeur donne à cliente", (*c).vendeur_to_main, (*c).main_to_cliente);


    // la cliente tend l'article à la caissière
    echange("Cliente donne à caissière", (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissière annonce le total à payer à la cliente
    echange("Caissière dit à cliente", (*c).caissiere_to_main, (*c).main_to_cliente);

    // la cliente paie à la caissière
    echange("Cliente paye à caissière", (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissière remet l'article et le ticket de caisse à la cliente, dans un sac
    echange("Caissière donne à cliente", (*c).caissiere_to_main, (*c).main_to_cliente);

    // la cliente dit merci et au revoir à la caissière
    echange("Cliente dit à caissière", (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissière dit merci et au revoir à la cliente
    echange("Caissière dit à cliente", (*c).caissiere_to_main, (*c).main_to_cliente);
}


void processus_vendeur(struct Canaux* c) {
    char buffer[100];
    char phrase[100];
    char *produit;


    // le vendeur dit bonjour à la cliente
    write((*c).vendeur_to_main[1], bonjour, strlen(bonjour) + 1);

    // la cliente répond bonjour au vendeur
    read((*c).main_to_vendeur[0], buffer, sizeof(buffer));

    // le vendeur demande à la cliente ce qui lui ferait plaisir
    write((*c).vendeur_to_main[1], tu_veux_quoi, strlen(tu_veux_quoi) + 1);

    // la cliente dit le nom de l'article
    read((*c).main_to_vendeur[0], phrase, sizeof(phrase));
    produit = phrase + strlen(je_voudrais);

    // le vendeur tend l'article à la cliente
    write((*c).vendeur_to_main[1], produit, strlen(produit) + 1);
}


void processus_cliente(struct Canaux* c, char* article) {
    char buffer[100];
    char produit[100];
    char prix[100];
    char sac[100];

    // le vendeur dit bonjour à la cliente
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));

    // la cliente répond bonjour au vendeur
    write((*c).cliente_to_main[1], bonjour, strlen(bonjour) + 1);

    // le vendeur demande à la cliente ce qui lui ferait plaisir
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));

    // la cliente dit le nom de l'article
    sprintf(buffer, "%s%s", je_voudrais, article);
    write((*c).cliente_to_main[1], buffer, strlen(buffer) + 1);

    // le vendeur tend l'article à la cliente
    read((*c).main_to_cliente[0], produit, sizeof(produit));

    // check que produit == article

    // la cliente tend l'article à la caissière
    write((*c).cliente_to_main[1], produit, strlen(produit) + 1);

    // la caissière annonce le total à payer à la cliente
    read((*c).main_to_cliente[0], prix, sizeof(prix));

    // la cliente paie à la caissière
    write((*c).cliente_to_main[1], prix, strlen(prix) + 1);

    // la caissière remet l'article et le ticket de caisse à la cliente, dans un sac
    read((*c).main_to_cliente[0], sac, sizeof(sac));

    // la cliente dit merci et au revoir à la caissière
    write((*c).cliente_to_main[1], merci, strlen(merci) + 1);

    // la caissière dit merci et au revoir à la cliente
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));
}


void processus_caissiere(struct Canaux* c) {
    char buffer[100];
    char produit[100];
    char prix[100];
    char sac[100];
    char argent[100];
    int prix_provisoire;


    // la cliente tend l'article à la caissière
    read((*c).main_to_caissiere[0], produit, sizeof(produit));

    // la caissière douche l'article
    prix_provisoire = strlen(produit);

    // la caissière annonce le total à payer à la cliente
    sprintf(prix, "%i", prix_provisoire);
    write((*c).caissiere_to_main[1], prix, strlen(prix) + 1);

    // la cliente paie à la caissière
    read((*c).main_to_caissiere[0], argent, sizeof(argent));

    // check bonne somme

    // la caissière remet l'article et le ticket de caisse à la cliente, dans un sac
    sprintf(sac, "un sac contenant un/une %s et un ticket de caisse d'un montant de %s", produit, prix);
    write((*c).caissiere_to_main[1], sac, strlen(sac) + 1);

    // la cliente dit merci et au revoir à la caissière
    read((*c).main_to_caissiere[0], buffer, sizeof(buffer));

    // la caissière dit merci et au revoir à la cliente
    write((*c).caissiere_to_main[1], merci, strlen(merci) + 1);

}


int main(int argc, char *argv[]) {
    struct Canaux c;

    int nb;
    char *buffer[100];

    if (argc != 5){
        perror("Erreur, nombre incorrect d'argument. Quatre arguments sont attendus : le nom de la cliente, le nom du vendeur, le nom de la caissiere et le nom du produit.\n");
        exit(1);
    }
    char *cliente = argv[1];
    char *vendeur = argv[2];
    char *caissiere = argv[3];
    char *article = argv[4];


    check_param_domain("cliente", cliente, "Chloe", "Elise", "Lea");
    check_param_domain("vendeur", vendeur, "Pierre", "Paul", "Jacques");
    check_param_domain("caissiere", caissiere, "Lilou", "Laura", "Nadia");
    check_param_domain("article", article, "body", "brassiere", "pyjama");

    pipe(c.vendeur_to_main);
    pipe(c.main_to_vendeur);
    pipe(c.cliente_to_main);
    pipe(c.main_to_cliente);
    pipe(c.caissiere_to_main);
    pipe(c.main_to_caissiere);

    pid_t pidCliente, pidVendeur, pidCaissiere;

    pidCliente = safe_fork();
    if (pidCliente){
        close(c.cliente_to_main[0]);
         close(c.main_to_cliente[1]);
        processus_cliente(&c, article);
    }
    else {
        pidVendeur = safe_fork();
        if (pidVendeur){
            close(c.vendeur_to_main[0]);
            close(c.main_to_vendeur[1]);
            processus_vendeur(&c);
        }
        else {
            pidCaissiere = safe_fork();
            if(pidCaissiere){
                close(c.caissiere_to_main[0]);
                close(c.main_to_caissiere[1]);
                processus_caissiere(&c);
            }
            else{
                    close(c.vendeur_to_main[1]);
                    close(c.main_to_vendeur[0]);
                    close(c.cliente_to_main[1]);
                    close(c.main_to_cliente[0]);
                    close(c.caissiere_to_main[1]);
                    close(c.main_to_caissiere[0]);
                processus_main(&c);
            }
        }
    }
}

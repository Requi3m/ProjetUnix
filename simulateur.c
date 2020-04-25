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
const char* bienvenue = "Bienvenue à 'Fais DauDau', la boutique de vêtements pour vos petits filous !";
const char* dommage = "Quel dommage, au revoir.";
const char* rupture_stock = "Je suis désolé, nous sommes en rupture de stock de ";
const char* trop_cher = "Oh ! Je n'ai pas assez d'argent. Je reviendrais.";

const int NORMAL = 1, RUPTURE = 2, TROPCHER = 3;

int safe_fork() {
    int f = fork();
    if (f == -1) {
        perror("Erreur de création de fork.");
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

int choix_scenario() {
    int scenario;

    printf("Nous vous proposons trois scénarios possibles.\n");
    printf("Tapez 1 pour le scénario 'Dans le meilleur des mondes'.\n");
    printf("Tapez 2 pour le scénario 'Pénurie de guerre'.\n");
    printf("Tapez 3 pour le scénario 'Crise des subprimes'.\n");
    scanf("%i", &scenario);

    while (scenario != 1 && scenario != 2 && scenario !=3) {
        printf("Numéro de scénario incorrect. Tapez 1, 2 ou 3.\n");
        scanf("%i", &scenario);
    }

    printf("Vous avez choisi le scénario %i.\n\n", scenario);

    return scenario;
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
    printf("%s %s à %s: %s\n", nom_parle, action, nom_ecoute, buffer);
    write(canal_ecoute[1], buffer, len);

    return buffer;
}


void echange_sac(char* nom_parle, char* nom_ecoute, int canal_parle[], int canal_ecoute[]) {
    struct Sac sac;
    int len;

    len = read(canal_parle[0], &sac, sizeof(sac));
    printf("%s donne un sac à %s contenant l'article %s et un ticket de montant %i.\n",
            nom_parle, nom_ecoute, sac.produit, sac.montant_ticket);
    write(canal_ecoute[1], &sac, len);
}

void echange_argent(char* nom_parle, char* nom_ecoute, int canal_parle[], int canal_ecoute[]) {
    int argent;
    int len;

    len = read(canal_parle[0], &argent, sizeof(argent));
    printf("%s paye %i euros à %s.\n", nom_parle, argent, nom_ecoute);
    write(canal_ecoute[1], &argent, len);
}

void processus_main(struct Canaux* c, char* cliente, char* vendeur, char* caissière, int scenario) {
    char buffer[100];
    char *phrase, *produit;
    char prix[100];
    int len;
    int bodyStock = 10, brassiereStock = 10, pyjamaStock = 10;

    // le vendeur dit bonjour à la cliente
    echange(vendeur, "dit", cliente, (*c).vendeur_to_main,(*c).main_to_cliente);

    // la cliente répond bonjour au vendeur
    echange(cliente, "dit", vendeur, (*c).cliente_to_main, (*c).main_to_vendeur);

    // le vendeur demande à la cliente ce qui lui ferait plaisir
    echange(vendeur, "dit", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

    // la cliente dit le nom de l'article
    phrase = echange(cliente, "dit", vendeur, (*c).cliente_to_main, (*c).main_to_vendeur);
    produit = phrase + strlen(je_voudrais);

    if (scenario == RUPTURE) { // scénario rupture de stock
        // le vendeur annonce une rupture de stock
        echange(vendeur, "dit", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

        // la cliente est désolée et dit au revoir
        echange(cliente, "dit", vendeur, (*c).cliente_to_main, (*c).main_to_vendeur);

        // le vendeur est désolé et dit au revoir
        echange(vendeur, "dit", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

        // fin de la simulation
        exit(1);
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

    // le vendeur tend l'article à la cliente
    echange(vendeur, "donne", cliente, (*c).vendeur_to_main, (*c).main_to_cliente);

    // la cliente tend l'article à la caissière
    echange(cliente, "donne", caissière, (*c).cliente_to_main, (*c).main_to_caissiere);

    // // la caissière cherche le prix du produit
    len = read((*c).caissiere_to_main[0], buffer, sizeof(buffer));
    sprintf(prix, "%li", strlen(buffer));
    write((*c).main_to_caissiere[1], prix, strlen(prix) + 1);


    // la caissière annonce le total à payer à la cliente
    echange(caissière, "dit" ,cliente, (*c).caissiere_to_main, (*c).main_to_cliente);

    if (scenario == TROPCHER) { // scénario crise économique
        // la cliente n'a pas assez d'argent
        echange(cliente, "dit", caissière, (*c).cliente_to_main, (*c).main_to_caissiere);

        // la caissière est désolée et dit au revoir
        echange(caissière, "dit" ,cliente, (*c).caissiere_to_main, (*c).main_to_cliente);

        // la cliente est désolée et dit au revoir
        echange(cliente, "dit", caissière, (*c).cliente_to_main, (*c).main_to_caissiere);

        // fin de la simulation
        exit(1);
    }

    // la cliente paie à la caissière
    echange_argent(cliente, caissière, (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissière remet l'article et le ticket de caisse à la cliente, dans un sac
    echange_sac(caissière, cliente, (*c).caissiere_to_main, (*c).main_to_cliente);

    // la cliente dit merci et au revoir à la caissière
    echange(cliente, "dit", caissière, (*c).cliente_to_main, (*c).main_to_caissiere);

    // la caissière dit merci et au revoir à la cliente
    echange(caissière, "dit", cliente, (*c).caissiere_to_main, (*c).main_to_cliente);
}


void processus_vendeur(struct Canaux* c, int scenario) {
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

    if (scenario == RUPTURE) { // scénario rupture de stock
        // le vendeur annonce une rupture de stock
        sprintf(buffer, "%s%s. Revenez une prochaine fois.", rupture_stock, produit);
        write((*c).vendeur_to_main[1], buffer, strlen(buffer) + 1);

        // la cliente est désolée et dit au revoir
        read((*c).main_to_vendeur[0], buffer, sizeof(buffer));

        // le vendeur est désolé et dit au revoir
        write((*c).vendeur_to_main[1], dommage, strlen(dommage) + 1);

        return;
    }

    // le vendeur tend l'article à la cliente
    write((*c).vendeur_to_main[1], produit, strlen(produit) + 1);
}


void processus_cliente(struct Canaux* c, char* article, int scenario) {
    char buffer[100];
    char produit[100];
    char prix[100];
    int argent;
    struct Sac sac;

    // le vendeur dit bonjour à la cliente
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));

    // la cliente répond bonjour au vendeur
    write((*c).cliente_to_main[1], bonjour, strlen(bonjour) + 1);

    // le vendeur demande à la cliente ce qui lui ferait plaisir
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));

    // la cliente dit le nom de l'article
    sprintf(buffer, "%s%s", je_voudrais, article);
    write((*c).cliente_to_main[1], buffer, strlen(buffer) + 1);

    if (scenario == RUPTURE) { // scénario rupture de stock
        // le vendeur annonce une rupture de stock
        read((*c).main_to_cliente[0], buffer, sizeof(buffer));

        // la cliente est désolée et dit au revoir
        write((*c).cliente_to_main[1], dommage, strlen(dommage) + 1);

        // le vendeur est désolé et dit au revoir
        read((*c).main_to_cliente[0], buffer, sizeof(buffer));

        return;
    }

    // le vendeur tend l'article à la cliente
    read((*c).main_to_cliente[0], produit, sizeof(produit));

    // check que produit == article

    // la cliente tend l'article à la caissière
    write((*c).cliente_to_main[1], produit, strlen(produit) + 1);

    // la caissière annonce le total à payer à la cliente
    read((*c).main_to_cliente[0], prix, sizeof(prix));

    if (scenario == TROPCHER) { // scénario crise économique
        // la cliente n'a pas assez d'argent
        write((*c).cliente_to_main[1], trop_cher, strlen(trop_cher) + 1);

        // la caissière est désolée et dit au revoir
        read((*c).main_to_cliente[0], buffer, sizeof(buffer));

        // la cliente est désolée et dit au revoir
        write((*c).cliente_to_main[1], dommage, strlen(dommage) + 1);

        return;
    }

    // la cliente paie à la caissière
    argent = atoi(prix);
    write((*c).cliente_to_main[1], &argent, sizeof(argent));

    // la caissière remet l'article et le ticket de caisse à la cliente, dans un sac
    read((*c).main_to_cliente[0], &sac, sizeof(sac));

    // la cliente dit merci et au revoir à la caissière
    write((*c).cliente_to_main[1], merci, strlen(merci) + 1);

    // la caissière dit merci et au revoir à la cliente
    read((*c).main_to_cliente[0], buffer, sizeof(buffer));
}


void processus_caissiere(struct Canaux* c, int scenario) {
    char buffer[100];
    char produit[100];
    char prix[100];
    int len;
    int argent;
    struct Sac sac;

    if (scenario == RUPTURE) {
        return;
    }

    // la cliente tend l'article à la caissière
    len = read((*c).main_to_caissiere[0], produit, sizeof(produit));

    // la caissière douche l'article (cherche le prix de l'article)
    write((*c).caissiere_to_main[1], produit, len);
    len = read((*c).main_to_caissiere[0], prix, sizeof(prix));

    // la caissière annonce le total à payer à la cliente
    sprintf(prix, "%s euros s'il-vous-plaît.", prix);
    write((*c).caissiere_to_main[1], prix, strlen(prix) + 1);

    if (scenario == TROPCHER) { // scénario crise économique
        // la cliente n'a pas assez d'argent
        read((*c).main_to_caissiere[0], buffer, sizeof(buffer));

        // la caissière est désolée et dit au revoir
        write((*c).caissiere_to_main[1], dommage, strlen(dommage) + 1);

        // la cliente est désolée et dit au revoir
        read((*c).main_to_caissiere[0], buffer, sizeof(buffer));

        return;
    }

    // la cliente paie à la caissière
    read((*c).main_to_caissiere[0], &argent, sizeof(argent));

    // check bonne somme

    // la caissière remet l'article et le ticket de caisse à la cliente, dans un sac
    sac.montant_ticket = argent;
    strcpy(sac.produit, produit);

    write((*c).caissiere_to_main[1], &sac, sizeof(sac));

    // la cliente dit merci et au revoir à la caissière
    read((*c).main_to_caissiere[0], buffer, sizeof(buffer));

    // la caissière dit merci et au revoir à la cliente
    write((*c).caissiere_to_main[1], merci, strlen(merci) + 1);

}


int main(int argc, char *argv[]) {
    struct Canaux c;
    char cliente[100];
    char vendeur[100];
    char caissiere[100];
    char article[100];
    int scenario;

    if (argc != 1) {
        perror("Erreur, pas d'arguments attendus.\n");
        exit(1);
    }

    printf("%s\n", bienvenue);
    get_param("cliente", cliente, "Chloe", "Elise", "Lea");
    get_param("vendeur", vendeur, "Pierre", "Paul", "Jacques");
    get_param("caissiere", caissiere, "Lilou", "Laura", "Nadia");
    get_param("article", article, "body", "brassiere", "pyjama");

    scenario = choix_scenario();


    pipe(c.cliente_to_main);
    pipe(c.main_to_cliente);
    pipe(c.vendeur_to_main);
    pipe(c.main_to_vendeur);
    pipe(c.caissiere_to_main);
    pipe(c.main_to_caissiere);

    pidCliente = safe_fork();
    if (pidCliente) {
        // close(c.cliente_to_main[0]);
        // close(c.main_to_cliente[1]);
        processus_cliente(&c, article, scenario);
    }
    else {
        // close(c.cliente_to_main[1]);
        // close(c.main_to_cliente[0]);
        pidVendeur = safe_fork();
        if (pidVendeur) {
            // close(c.vendeur_to_main[0]);
            // close(c.main_to_vendeur[1]);
            processus_vendeur(&c, scenario);
        }
        else {
            // close(c.vendeur_to_main[1]);
            // close(c.main_to_vendeur[0]);

            pidCaissiere = safe_fork();
            if (pidCaissiere) {
                // close(c.caissiere_to_main[0]);
                // close(c.main_to_caissiere[1]);
                processus_caissiere(&c, scenario);
            }
            else{
                // close(c.caissiere_to_main[1]);
                // close(c.main_to_caissiere[0]);
                signal(SIGKILL, kill_all);

                processus_main(&c, cliente, vendeur, caissiere, scenario);
            }
        }
    }
}

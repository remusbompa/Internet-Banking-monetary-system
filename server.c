    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    #define MAX_CLIENTS	5
    #define BUFLEN 256

    typedef struct
    {
      char nume[13];
      char prenume[13];
      char nr_card[7];
      char pin[5];
      char parola[9];
      double sold;
    } TUser;

    typedef struct client
    {
      int sockfd;
      int prev_login;
      int nr_login;
      struct client *urm;
    } TClient;

    typedef struct celulaBl
    {
      int cont;
      struct celulaBl *urm;
    } TClientB;

    typedef struct celulaDBl
    {
      char ip[16];
      unsigned short port;
      int cont;
      struct celulaDBl *urm;
    } TClientD;

    void
    add_client (TClient ** clienti, int sockfd)
    {

      TClient *aux = (TClient *) malloc (sizeof (TClient));
      aux->sockfd = sockfd;
      aux->prev_login = -1;
      aux->nr_login = 0;
      aux->urm = (*clienti);
      *clienti = aux;
    }

    void
    remove_client (TClient ** clienti, int sockfd)
    {
      TClient **p;
      for (p = clienti; (*p) != NULL; p = &(*p)->urm)
          {
	    if ((*p)->sockfd == sockfd)
	        {
	          TClient *aux = *p;
	          *p = (*p)->urm;
	          free (aux);
	          break;
	        }
          }
    }

    TClient *
    find_client (TClient * clienti, int sockfd)
    {
      TClient *p;
      for (p = clienti; p != NULL; p = p->urm)
          {
	    if (p->sockfd == sockfd)
	      return p;
          }
      return NULL;
    }

    typedef struct
    {
      int dest;
      double suma;
    } TTransfer;

    void
    error (char *msg)
    {
      perror (msg);
      exit (1);
    }

    void
    cod_eroare (char *type, char *message, int cod)
    {
      if (cod == -10)
        sprintf (message, "%s -10 : Eroare la apel nume-functie", type);
      if (cod == -9)
        sprintf (message, "%s -9 : Operatie anulata", type);
      if (cod == -8)
        sprintf (message, "%s -8 : Fonduri insuficiente", type);
      if (cod == -7)
        sprintf (message, "%s -7 : Deblocare esuata", type);
      if (cod == -6)
        sprintf (message, "%s -6 : Operatie esuata", type);
      if (cod == -5)
        sprintf (message, "%s -5 : Card blocat", type);
      if (cod == -4)
        sprintf (message, "%s -4 : Numar card inexistent", type);
      if (cod == -3)
        sprintf (message, "%s -3 : Pin gresit", type);
      if (cod == -2)
        sprintf (message, "%s -2 : Sesiune deja deschisa", type);

    }

    int
    verificare_card (TUser * users, int N, char *numar_card)
    {
      int i;
      for (i = 0; i < N; i++)
          {
	    if (strcmp (users[i].nr_card, numar_card) == 0)
	      return i;
          }
      return -1;
    }

    int
    getCont (int sockfd, int *asociere, int N)
    {
      int i;
      for (i = 0; i < N; i++)
          {
	    if (asociere[i] == sockfd)
	      return i;
          }
      return -1;
    }

    void
    blocare (TClientB ** clienti_blocati, int cont)
    {
      TClientB *p = (TClientB *) malloc (sizeof (TClientB));
      p->cont = cont;
      p->urm = *clienti_blocati;
      *clienti_blocati = p;
    }

    void
    add_deblocare (TClientD ** clienti_deblocati, int cont,
	           struct sockaddr *sockaddr)
    {
      struct sockaddr_in *sin = (struct sockaddr_in *) sockaddr;
      char *ip = inet_ntoa (sin->sin_addr);
      unsigned short port = ntohs (sin->sin_port);
      TClientD *p = (TClientD *) malloc (sizeof (TClientD));
      sprintf (p->ip, "%s", ip);
      p->port = port;
      p->cont = cont;
      p->urm = *clienti_deblocati;
      *clienti_deblocati = p;
    }

    int
    remove_blocare (TClientB ** clienti_blocati, int cont)
    {
      TClientB **p;
      for (p = clienti_blocati; *p; p = &(*p)->urm)
          {
	    if ((*p)->cont == cont)
	        {
	          TClientB *aux = *p;
	          *p = (*p)->urm;
	          free (aux);
	          return 1;
	        }
          }
      return 0;
    }

    int
    is_blocked (TClientB * clienti_blocati, int cont)
    {
      TClientB *p;
      for (p = clienti_blocati; p; p = p->urm)
          {
	    if (p->cont == cont)
	        {
	          return 1;
	        }
          }
      return 0;
    }

    void
    remove_deblocare (TClientD ** clienti_deblocati, int cont)
    {
      TClientD **p;
      for (p = clienti_deblocati; *p; p = &(*p)->urm)
          {
	    if ((*p)->cont == cont)
	        {
	          TClientD *aux = *p;
	          *p = (*p)->urm;
	          free (aux);
	          return;
	        }
          }
    }

    int
    is_deblocking (TClientD * clienti_deblocati, int cont)
    {
      TClientD *p;
      for (p = clienti_deblocati; p; p = p->urm)
          {
	    if (p->cont == cont)
	        {
	          return 1;
	        }
          }
      return 0;
    }

    int
    asked_password (TClientD * clienti_deblocati, struct sockaddr *sockaddr)
    {
      struct sockaddr_in *sin = (struct sockaddr_in *) sockaddr;
      char *ip = inet_ntoa (sin->sin_addr);
      unsigned short port = ntohs (sin->sin_port);
      TClientD *p;
      for (p = clienti_deblocati; p; p = p->urm)
          {
	    if (!strcmp (p->ip, ip) && p->port == port)
	        {
	          return 1;
	        }
          }
      return 0;
    }

    void
    remove_all_clienti (TClient ** clienti, fd_set * read_fds, char *buffer)
    {
      TClient *p = *clienti;
      while (p)
          {
	    TClient *aux = p;
	    p = p->urm;
	    FD_CLR (aux->sockfd, read_fds);
	    int n = send (aux->sockfd, buffer, strlen (buffer), 0);
	    if (n < 0)
	      error ("ERROR writing to socket");
	    close (aux->sockfd);
	    free (aux);
          }
      FD_CLR (STDIN_FILENO, read_fds);
      close (STDIN_FILENO);
      *clienti = NULL;
    }

    void
    remove_all_blocari (TClientB ** clienti_blocati)
    {
      TClientB *p = *clienti_blocati;
      while (p)
          {
	    TClientB *aux = p;
	    p = p->urm;
	    free (aux);
          }
      *clienti_blocati = NULL;
    }

    void
    remove_all_deblocari (TClientD ** clienti_deblocati)
    {
      TClientD *p = *clienti_deblocati;
      while (p)
          {
	    TClientD *aux = p;
	    p = p->urm;
	    free (aux);
          }
      *clienti_deblocati = NULL;
    }

    int
    main (int argc, char *argv[])
    {

      int sockfd, newsockfd, portno, udp;
      unsigned int clilen;
      char buffer[BUFLEN];
      struct sockaddr_in serv_addr, cli_addr;
      int n, i;

      char ibank[] = "IBANK>", unlock[] = "UNLOCK>";
      fd_set read_fds;		//multimea de citire folosita in select()
      fd_set tmp_fds;		//multime folosita temporar
      int fdmax;			//valoare maxima file descriptor din multimea read_fds

      if (argc < 3)
          {
	    fprintf (stderr, "Usage : %s port users.data\n", argv[0]);
	    exit (1);
          }

      FILE *fin = fopen (argv[2], "r");
      if (fin < 0)
        error ("ERROR opening file users.data");
      int N;
      fscanf (fin, "%d", &N);
      TUser users[N];
      int asociere[N];
      TClientB *clienti_blocati = NULL;
      TClientD *clienti_deblocati = NULL;
      TTransfer transferuri[N];
      for (i = 0; i < N; i++)
          {
	    fscanf (fin, "%s", users[i].nume);
	    fscanf (fin, "%s", users[i].prenume);
	    fscanf (fin, "%s", users[i].nr_card);
	    fscanf (fin, "%s", users[i].pin);
	    fscanf (fin, "%s", users[i].parola);
	    fscanf (fin, "%lf", &users[i].sold);
	    asociere[i] = -1;
	    transferuri[i].dest = -1;
          }
      fclose (fin);

      //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
      FD_ZERO (&read_fds);
      FD_ZERO (&tmp_fds);

      sockfd = socket (AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
        error ("ERROR opening socket");


      udp = socket (AF_INET, SOCK_DGRAM, 0);
      if (udp < 0)
        error ("ERROR opening socket");

      portno = atoi (argv[1]);
      memset ((char *) &serv_addr, 0, sizeof (serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
      serv_addr.sin_port = htons (portno);

      if (bind (sockfd, (struct sockaddr *) &serv_addr, sizeof (struct sockaddr))
          < 0)
        error ("ERROR on binding");
      if (bind (udp, (struct sockaddr *) &serv_addr, sizeof (struct sockaddr)) <
          0)
        error ("ERROR on binding");

      listen (sockfd, MAX_CLIENTS);

      //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
      FD_SET (sockfd, &read_fds);
      FD_SET (udp, &read_fds);
      FD_SET (STDIN_FILENO, &read_fds);
      fdmax = sockfd > udp ? sockfd : udp;
      TClient *clienti = NULL;
      // main loop
      while (1)
          {
	    tmp_fds = read_fds;
	    if (select (fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
	      error ("ERROR in select");

	    for (i = 0; i <= fdmax; i++)
	        {
	          if (FD_ISSET (i, &tmp_fds))
		      {
		        if (i == udp)
			    {
			      struct sockaddr from;
			      memset (buffer, 0, BUFLEN);
			      if ((n =
			           recvfrom (i, buffer, sizeof (buffer), 0, &from,
					     &clilen)) <= 0)
			          {
				    if (n == 0)
				        {
				          printf
					    ("selectserver: socket %d hung up\n",
					     i);
				        }
				    else
				        {
				          error ("ERROR in recv");
				        }
				    close (i);
				    FD_CLR (i, &read_fds);
			          }
			      else
			          {
				    struct sockaddr_in *cli =
				      (struct sockaddr_in *) &from;
				    printf
				      ("Noua conexiune de la %s, port %d. Am primit de la clientul de pe socketul udp %d, mesajul: %s\n",
				       inet_ntoa (cli->sin_addr),
				       ntohs (cli->sin_port), udp, buffer);
				    if (asked_password (clienti_deblocati, &from))
				        {
				          char nr_card[7], parola[9];
				          sscanf (buffer, "%s %s", nr_card,
					          parola);
				          int user =
					    verificare_card (users, N, nr_card);
				          remove_deblocare (&clienti_deblocati,
							    user);
				          if (!strcmp
					      (users[user].parola, parola))
					      {	//daca au aceeasi parola
					        remove_blocare (&clienti_blocati,
							        user);
					        sprintf (buffer,
						         "UNLOCK> Card deblocat");
					      }
				          else
					    cod_eroare (unlock, buffer, -7);

				        }
				    else
				        {
				          char command[20];
				          sscanf (buffer, "%s", command);
				          if (!strcmp (command, "unlock"))
					      {
					        char nr_card[7];
					        sscanf (buffer, "unlock %s",
						        nr_card);
					        int user =
					          verificare_card (users, N,
							           nr_card);
					        if (user < 0)
					          cod_eroare (unlock, buffer, -4);
					        else
					          if (!is_blocked
						      (clienti_blocati, user))
					          cod_eroare (unlock, buffer, -6);
					        else
					          if (is_deblocking
						      (clienti_deblocati, user))
					          cod_eroare (unlock, buffer, -7);
					        else
						    {
						      sprintf (buffer,
							       "UNLOCK> Trimite parola secreta");
						      add_deblocare
						        (&clienti_deblocati, user,
						         &from);
						    }
					      }
				          else
					      {
					        sprintf (buffer,
						         "Comanda invalida udp");
					      }
				        }

				    n =
				      sendto (i, buffer, strlen (buffer), 0,
					      &from, clilen);
				    if (n < 0)
				      error ("ERROR writing to socket");
				    continue;
			          }
			    }
		        else if (i == STDIN_FILENO)
			    {
			      if ((n = scanf ("%s", buffer)) <= 0)
			          {
				    error ("ERROR in scanf");
			          }
			      else
			          {
				    printf
				      ("Am primit de la stdin de pe socketul %d, mesajul: %s\n",
				       i, buffer);
				    char command[20];
				    sscanf (buffer, "%s", command);
				    if (!strcmp (command, "quit"))
				        {
				          sprintf (buffer, "quit");
				          remove_all_clienti (&clienti, &read_fds,
							      buffer);
				          remove_all_blocari (&clienti_blocati);
				          remove_all_deblocari
					    (&clienti_deblocati);
				          close (sockfd);
				          close (udp);
				          FD_CLR (sockfd, &read_fds);
				          return 0;
				        }
				    else
				      printf ("Comanda invalida\n");
			          }

			    }
		        else if (i == sockfd)
			    {
			      clilen = sizeof (cli_addr);
			      if ((newsockfd =
			           accept (sockfd, (struct sockaddr *) &cli_addr,
				           &clilen)) == -1)
			          {
				    error ("ERROR in accept");
			          }
			      else
			          {
				    FD_SET (newsockfd, &read_fds);
				    add_client (&clienti, newsockfd);
				    if (newsockfd > fdmax)
				        {
				          fdmax = newsockfd;
				        }
			          }
			      printf
			        ("Noua conexiune de la %s, port %d, socket_client %d\n ",
			         inet_ntoa (cli_addr.sin_addr),
			         ntohs (cli_addr.sin_port), newsockfd);
			    }
		        else
			    {
			      memset (buffer, 0, BUFLEN);
			      if ((n = recv (i, buffer, sizeof (buffer), 0)) <= 0)
			          {
				    if (n == 0)
				        {
				          printf
					    ("selectserver: socket %d hung up\n",
					     i);
				        }
				    else
				        {
				          error ("ERROR in recv");
				        }
				    close (i);
				    FD_CLR (i, &read_fds);
				    remove_client (&clienti, i);
			          }

			      else
			          {
				    printf
				      ("Am primit de la clientul tcp de pe socketul %d, mesajul: %s\n",
				       i, buffer);
				    TClient *found = find_client (clienti, i);
				    char command[20];
				    sscanf (buffer, "%s", command);
				    int user = getCont (i, asociere, N);
				    if (user >= 0 && transferuri[user].dest != -1)
				        {
				          if (command[0] == 'y')
					      {
					        int dest = transferuri[user].dest;
					        double suma =
					          transferuri[user].suma;
					        users[user].sold =
					          users[user].sold - suma;
					        users[dest].sold =
					          users[dest].sold + suma;
					        transferuri[user].dest = -1;
					        transferuri[user].suma = 0;
					        sprintf (buffer,
						         "IBANK> Transfer realizat cu succes");
					      }
				          else
					      {
					        transferuri[user].dest = -1;
					        transferuri[user].suma = 0;
					        cod_eroare (ibank, buffer, -9);
					      }
				          n =
					    send (i, buffer, strlen (buffer), 0);
				          if (n < 0)
					    error ("ERROR writing to socket");
				          continue;
				        }
				    if (!strcmp (command, "login"))
				        {
				          char numar_card[20], pin[20];
				          sscanf (buffer, "login %s %s",
					          numar_card, pin);

				          if ((user =
					       verificare_card (users, N,
							        numar_card)) ==
					      -1)
					      {
					        cod_eroare (ibank, buffer, -4);
					        found->nr_login = 0;
					      }
				          else
					    if (is_blocked
					        (clienti_blocati, user))
					      {
					        cod_eroare (ibank, buffer, -5);
					        found->nr_login = 0;
					      }
				          else if (strcmp (users[user].pin, pin))
					      {
					        cod_eroare (ibank, buffer, -3);
					        if (found->prev_login == user)
					          (found->nr_login)++;
					        else
					          found->nr_login = 1;
					        if (found->nr_login == 3)
						    {
						      cod_eroare (ibank, buffer,
							          -5);
						      blocare (&clienti_blocati,
							       user);
						      found->nr_login = 0;
						    }
					      }
				          else if (asociere[user] != -1)
					      {
					        cod_eroare (ibank, buffer, -2);
					        found->nr_login = 0;
					      }
				          else
					      {
					        sprintf (buffer,
						         "IBANK> Welcome %s %s",
						         users[user].nume,
						         users[user].prenume);
					        asociere[user] = i;
					        found->nr_login = 0;
					      }
				          found->prev_login = user;
				        }
				    else
				        {
				          found->nr_login = 0;
				          found->prev_login = -1;
				          if (!strcmp (command, "logout"))
					      {
					        int user =
					          getCont (i, asociere, N);
					        asociere[user] = -1;
					        sprintf (buffer,
						         "IBANK> Clientul a fost deconectat");
					      }
				          else if (!strcmp (command, "listsold"))
					      {
					        int user =
					          getCont (i, asociere, N);
					        sprintf (buffer, "%s %.2lf",
						         ibank, users[user].sold);
					      }
				          else if (!strcmp (command, "transfer"))
					      {
					        char nr_card[7];
					        double suma;
					        sscanf (buffer, "transfer %s %lf",
						        nr_card, &suma);
					        int dest =
					          verificare_card (users, N,
							           nr_card);
					        if (dest < 0)
					          cod_eroare (ibank, buffer, -4);
					        else if (suma > users[user].sold)
					          cod_eroare (ibank, buffer, -8);
					        else
						    {
						      transferuri[user].dest =
						        dest;
						      transferuri[user].suma =
						        suma;
						      if (suma - (int) suma == 0)
						        sprintf (buffer,
							         "IBANK> Transfer %d catre %s %s? [y/n]",
							         (int) suma,
							         users[dest].nume,
							         users[dest].
							         prenume);
						      else
						        sprintf (buffer,
							         "IBANK> Transfer %.2lf catre %s %s? [y/n]",
							         suma,
							         users[dest].nume,
							         users[dest].
							         prenume);
						    }
					      }
				          else if (!strcmp (command, "quit"))
					      {
					        FD_CLR (i, &read_fds);
					        remove_client (&clienti, i);
					        close (i);
					        continue;
					      }
				        }
				    n = send (i, buffer, strlen (buffer), 0);
				    if (n < 0)
				      error ("ERROR writing to socket");

			          }
			    }
		      }
	        }
          }


      close (sockfd);

      return 0;
    }

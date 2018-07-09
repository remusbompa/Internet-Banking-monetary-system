    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>

    #define BUFLEN 256

    void
    error (char *msg)
    {
      perror (msg);
      exit (0);
    }

    int
    main (int argc, char *argv[])
    {
      int sockfd, n, udp;
      struct sockaddr_in serv_addr;

      char buffer[BUFLEN];
      if (argc < 3)
          {
	    fprintf (stderr, "Usage %s server_address server_port\n", argv[0]);
	    exit (0);
          }

      sockfd = socket (AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
        error ("ERROR opening socket");

      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons (atoi (argv[2]));
      inet_aton (argv[1], &serv_addr.sin_addr);

      if (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) <
          0)
        error ("ERROR connecting");

      udp = socket (AF_INET, SOCK_DGRAM, 0);
      if (udp < 0)
        error ("ERROR opening socket udp");


      char nume[14];
      sprintf (nume, "client-%u.log", getpid ());
      FILE *flog = fopen (nume, "w");
      printf ("%u\n", getpid ());
      int session_opened = 0;
      char last_login[7];

      fd_set read_fds, tmp_fds;
      FD_ZERO (&read_fds);
      FD_ZERO (&tmp_fds);
      FD_SET (STDIN_FILENO, &read_fds);
      FD_SET (sockfd, &read_fds);
      int fdmax = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
      while (1)
          {
	    tmp_fds = read_fds;
	    if (select (fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
	      error ("ERROR in select");
	    if (FD_ISSET (STDIN_FILENO, &tmp_fds))
	        {
	          //citesc de la tastatura
	          memset (buffer, 0, BUFLEN);
	          fgets (buffer, BUFLEN - 1, stdin);

	          //trimit mesaj la server
	          char command[20];
	          sscanf (buffer, "%s", command);
	          fprintf (flog, "%s", buffer);
	          if (!strcmp (command, "login"))
		      {
		        sscanf (buffer, "login %s", last_login);
		        if (!session_opened)
			    {
			      n = send (sockfd, buffer, strlen (buffer), 0);
			      if (n < 0)
			        error ("ERROR writing to socket");
			      memset (buffer, 0, BUFLEN);
			      if ((n =
			           recv (sockfd, buffer, sizeof (buffer), 0)) < 0)
			          {
				    error ("ERROR in recv");
			          }
			      char result[20];
			      sscanf (buffer, "IBANK> %s", result);
			      if (!strcmp (result, "Welcome"))
			        session_opened = 1;
			      fprintf (flog, "%s\n", buffer);
			      printf ("%s\n", buffer);
			    }
		        else
			    {
			      printf ("IBANK> -2 : Sesiune deja deschisa\n");
			      fprintf (flog,
				       "IBANK> -2 : Sesiune deja deschisa\n");
			    }
		      }
	          else if (!strcmp (command, "logout"))
		      {
		        if (session_opened)
			    {
			      n = send (sockfd, buffer, strlen (buffer), 0);
			      if (n < 0)
			        error ("ERROR writing to socket");
			      session_opened = 0;
			      memset (buffer, 0, BUFLEN);
			      if ((n =
			           recv (sockfd, buffer, sizeof (buffer), 0)) < 0)
			          {
				    error ("ERROR in recv");
			          }
			      fprintf (flog, "%s\n", buffer);
			      printf ("%s\n", buffer);
			    }
		        else
			    {
			      printf ("-1 : Clientul nu este autentificat\n");
			      fprintf (flog,
				       "-1 : Clientul nu este autentificat\n");
			    }
		      }
	          else if (!strcmp (command, "listsold"))
		      {
		        if (session_opened)
			    {
			      n = send (sockfd, buffer, strlen (buffer), 0);
			      if (n < 0)
			        error ("ERROR writing to socket");
			      memset (buffer, 0, BUFLEN);
			      if ((n =
			           recv (sockfd, buffer, sizeof (buffer), 0)) < 0)
			          {
				    error ("ERROR in recv");
			          }
			      fprintf (flog, "%s\n", buffer);
			      printf ("%s\n", buffer);
			    }
		        else
			    {
			      printf ("-1 : Clientul nu este autentificat\n");
			      fprintf (flog,
				       "-1 : Clientul nu este autentificat\n");
			    }
		      }

	          else if (!strcmp (command, "transfer"))
		      {
		        if (session_opened)
			    {
			      n = send (sockfd, buffer, strlen (buffer), 0);
			      if (n < 0)
			        error ("ERROR writing to socket");
			      memset (buffer, 0, BUFLEN);
			      if ((n =
			           recv (sockfd, buffer, sizeof (buffer), 0)) < 0)
			          {
				    error ("ERROR in recv");
			          }
			      fprintf (flog, "%s\n", buffer);
			      printf ("%s\n", buffer);

			      if (buffer[7] == '-')
			          {
				    fprintf (flog, "\n");
				    continue;
			          }
			      //daca nu se obtine o eroare, se trimite confirmare
			      memset (buffer, 0, BUFLEN);
			      fgets (buffer, BUFLEN - 1, stdin);
			      fprintf (flog, "%s", buffer);
			      n = send (sockfd, buffer, strlen (buffer), 0);
			      if (n < 0)
			        error ("ERROR writing to socket");
			      memset (buffer, 0, BUFLEN);
			      if ((n =
			           recv (sockfd, buffer, sizeof (buffer), 0)) < 0)
			          {
				    error ("ERROR in recv");
			          }
			      fprintf (flog, "%s\n", buffer);
			      printf ("%s\n", buffer);

			    }
		        else
			    {
			      printf ("-1 : Clientul nu este autentificat\n");
			      fprintf (flog,
				       "-1 : Clientul nu este autentificat\n");
			    }
		      }
	          else if (!strcmp (command, "unlock"))
		      {
		        struct sockaddr *from = NULL;
		        unsigned int clilen;
		        sprintf (buffer, "unlock %s", last_login);
		        n =
		          sendto (udp, buffer, strlen (buffer), 0,
			          (struct sockaddr *) &serv_addr,
			          sizeof (struct sockaddr));
		        if (n < 0)
		          error ("ERROR writing to socket");
		        memset (buffer, 0, BUFLEN);
		        if ((n =
			     recvfrom (udp, buffer, sizeof (buffer), 0, from,
				       &clilen)) < 0)
			    {
			      error ("ERROR in recv");
			    }
		        fprintf (flog, "%s\n", buffer);
		        printf ("%s\n", buffer);

		        if (buffer[8] == '-')
			    {
			      fprintf (flog, "\n");
			      continue;
			    }
		        char parola[BUFLEN];
		        memset (parola, 0, BUFLEN);
		        fgets (parola, BUFLEN - 1, stdin);
		        fprintf (flog, "%s", parola);

		        memset (buffer, 0, BUFLEN);
		        sprintf (buffer, "%s %s", last_login, parola);
		        n =
		          sendto (udp, buffer, strlen (buffer), 0,
			          (struct sockaddr *) &serv_addr,
			          sizeof (struct sockaddr));
		        if (n < 0)
		          error ("ERROR writing to socket");
		        memset (buffer, 0, BUFLEN);
		        if ((n =
			     recvfrom (udp, buffer, sizeof (buffer), 0, from,
				       &clilen)) < 0)
			    {
			      error ("ERROR in recv");
			    }
		        fprintf (flog, "%s\n", buffer);
		        printf ("%s\n", buffer);
		      }
	          else if (!strcmp (command, "quit"))
		      {
		        n = send (sockfd, buffer, strlen (buffer), 0);
		        if (n < 0)
		          error ("ERROR writing to socket");
		        close (sockfd);
		        close (udp);
		        fclose (flog);
		        return 0;
		      }
	          else
		      {
		        printf ("Comanda invalida\n");
		        fprintf (flog, "Comanda invalida\n");
		      }
	          fprintf (flog, "\n");
	        }
	    if (FD_ISSET (sockfd, &tmp_fds))
	        {
	          memset (buffer, 0, BUFLEN);
	          if ((n = recv (sockfd, buffer, sizeof (buffer), 0)) <= 0)
		      {
		        if (n == 0)
			    {
			      //conexiunea s-a inchis
			      printf ("selectserver: socket %d hung up\n",
				      sockfd);
			    }
		        else
			    {
			      error ("ERROR in recv");
			    }
		        close (sockfd);
		        FD_CLR (sockfd, &read_fds);
		      }

	          else
		      {
		        printf
		          ("Am primit de la clientul de pe socketul %d, mesajul: %s\n",
		           sockfd, buffer);
		        if (!strcmp (buffer, "quit"))
			    {
			      printf ("Conexiune inchisa de server\n");
			      close (sockfd);
			      close (udp);
			      FD_CLR (sockfd, &read_fds);
			      fclose (flog);
			      return 0;
			    }
		        else
			    {
			      printf ("Comanda invalida\n");
			    }
		      }
	        }

          }
      fclose (flog);
      return 0;
    }

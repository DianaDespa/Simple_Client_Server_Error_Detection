#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

void log_recv(msg t) {
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	char *time_buffer = getTime();
	char mes[100];

	if (time_buffer != NULL) {
		sprintf(mes, "[%s] [receiver] Am primit urmatorul pachet:\n", time_buffer);
		write(logfd, mes, strlen(mes));
		sprintf(mes, "Seq No: %d\nPayload: ", t.payload[0]);
		write(logfd, mes, strlen(mes));
		write(logfd, t.payload + 1, t.len - 2);
		write(logfd, "\n", 1);
		free(time_buffer);
	}
	close(logfd);
}

void log_recv_corrupt(msg t, unsigned char frame) {
	log_recv(t);
	
	char mes[200];
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	sprintf(mes, "Am calculat checksum si am detectat eroare. Voi trimite ACK \
pentru Seq No %d (ultimul cadru corect pe care l-am primit sau 255).\n", frame);
	write(logfd, mes, strlen(mes));
	write(logfd, DELIM, strlen(DELIM));
	close(logfd);
}

void log_recv_ok(msg t) {
	log_recv(t);
	
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	write(logfd, "OK.\n", 4);
	write(logfd, DELIM, strlen(DELIM));
	close(logfd);
}

void log_recv_dup(msg t) {
	log_recv(t);
	
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	write(logfd,"Duplicat.\n", 10);
	write(logfd, DELIM, strlen(DELIM));
	close(logfd);
}

void log_send_ack(msg t) {
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	char *time_buffer = getTime();
	char mes[100];

	if (time_buffer != NULL) {
		sprintf(mes, "[%s] [receiver] Trimit ACK:\n", time_buffer);
		write(logfd, mes, strlen(mes));
		sprintf(mes, "Seq No: %d\n", t.payload[0]);
		write(logfd, mes, strlen(mes));
		sprintf(mes, "Checksum: %s\n", getBitString(t.payload[t.len - 1]));
		write(logfd, mes, strlen(mes));
		write(logfd, DELIM, strlen(DELIM));
		free(time_buffer);
	}
	close(logfd);
}

int main(int argc,char** argv){
	msg r, s;
	init(HOST,PORT);

	unsigned char last_correct_frame, check_sum, frame_expected = 0;
	int result, i;
	
	// Adaug 1 la numele fisierului, pastrand extensia (daca aceasta exista).
	char file[20];
	if (strchr(argv[1], '.') != NULL) {
		int pos = strchr(argv[1], '.') - argv[1];
		strncpy(file, argv[1], pos);
		file[pos] = '\0';
		strcat(file, "1");
		strcat(file, argv[1] + pos);
	} else {
		strcpy(file, argv[1]);
		strcat(file, "1");
	}
	
	// Daca fisierul nu exista, il creez cu permisiuni de scriere si citire
	// pentru utilizator si grup, si cu drept de citire pentru altii.
	int fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 00664);
	
	while (1) {
		memset(&r, 0, sizeof(msg));
		memset(&s, 0, sizeof(msg));
		
		result = recv_message(&r);
		if (result < 0) {
			perror("Eroare primire.\n");
			return -1;
		}
		
		// Calculez si verific checksum.
		check_sum = r.payload[0];
		for (i = 1; i < r.len - 1; ++i) {
			check_sum ^= r.payload[i];
		}
		
		if (check_sum != r.payload[r.len - 1]) {
			// Daca primul pachet este corupt trimit 255, numarul maxim care
			// poate fi cuprins intr-un octet. Altfel, trimit indicele ultimului
			// pachet receptionat corect.
			if (frame_expected == 0) {
				s.payload[0] = 255;
				log_recv_corrupt(r, 255);
			} else {
				s.payload[0] = last_correct_frame;	
				log_recv_corrupt(r, last_correct_frame);
			}
		} else {
			// Daca pachetul nu este duplicat, scriu in fisier continutul lui si
			// actualizez numarul cadrului asteptat si ultimul cadru corect.
			if (r.payload[0] == frame_expected) {
				log_recv_ok(r);
				write(fd, r.payload + 1, r.len - 2);
				last_correct_frame = frame_expected;
				++frame_expected;
			} else {
				log_recv_dup(r);
			}
			// Trimit Seq No cu ultimul cadru corect receptionat.
			s.payload[0] = last_correct_frame;
		}
		
		s.payload[1] = check_sum;
		s.len = 2;
		
		log_send_ack(s);
		result = send_message(&s);
		if (result < 0) {
			perror("Eroare trimitere ACK.\n");
			return -1;
		}
		
		// Ies din bucla daca ajung la numarul maxim de cadre.
		if (frame_expected == MAXFRAMES) break;
	}
	
	close(fd);
	return 0;
}

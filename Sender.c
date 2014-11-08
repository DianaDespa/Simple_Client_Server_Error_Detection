#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

void log_send(msg t) {
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	char *time_buffer = getTime();
	char mes[100];

	if (time_buffer != NULL) {
		sprintf(mes, "[%s] [sender] Am trimis urmatorul pachet:\n", time_buffer);
		write(logfd, mes, strlen(mes));
		sprintf(mes, "Seq No: %d\nPayload: ", t.payload[0]);
		write(logfd, mes, strlen(mes));
		write(logfd, t.payload + 1, t.len - 2);
		sprintf(mes, "\nChecksum: %s\n", getBitString(t.payload[t.len - 1]));
		write(logfd, mes, strlen(mes));
		write(logfd, DELIM, strlen(DELIM));
		free(time_buffer);
	}
	close(logfd);
}

void log_cron_start() {
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	char *time_buffer = getTime();
	char mes[100];
	
	if (time_buffer != NULL) {
		sprintf(mes, "[%s] [sender] Am pornit cronometrul \
(timeout este de %dms).\n", time_buffer, TIMEOUT);
		write(logfd, mes, strlen(mes));
		write(logfd, DELIM, strlen(DELIM));
		free(time_buffer);
	}
	close(logfd);
}

void log_timeout(unsigned char frame) {
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	char *time_buffer = getTime();
	char mes[100];
	
	if (time_buffer != NULL) {
		sprintf(mes, "[%s] [sender] S-a depasit timpul pentru pachetul \
cu numarul %d. Retrimit.\n", time_buffer, frame);
		write(logfd, mes, strlen(mes));
		write(logfd, DELIM, strlen(DELIM));
		free(time_buffer);
	}
	close(logfd);
}

void log_wrong(unsigned char frame) {
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	char *time_buffer = getTime();
	char mes[100];
	
	if (time_buffer != NULL) {
		sprintf(mes, "[%s] [sender] Pachetul cu numarul %d a fost corupt. \
Retrimit.\n", time_buffer, frame);
		write(logfd, mes, strlen(mes));
		write(logfd, DELIM, strlen(DELIM));
		free(time_buffer);
	}
	close(logfd);
}

void log_ack(unsigned char frame) {
	int logfd = open("log.txt", O_WRONLY|O_APPEND);
	char *time_buffer = getTime();
	char mes[100];
	
	if (time_buffer != NULL) {
		sprintf(mes, "[%s] [sender] Am primit ACK pentru pachetul \
cu numarul %d.\n", time_buffer, frame);
		write(logfd, mes, strlen(mes));
		write(logfd, DELIM, strlen(DELIM));
		free(time_buffer);
	}
	close(logfd);
}

int main(int argc,char** argv){
	init(HOST,PORT);
	msg t;
	msg* response;
	
	// Daca fisierul nu exista, il creez cu permisiuni de scriere si citire
	// pentru utilizator si grup, si cu drept de citire pentru altii.
	int logfd = open("log.txt", O_WRONLY|O_CREAT|O_TRUNC, 00664);
	close(logfd);
	
	unsigned char next_frame_to_send = 0, check_sum;
	int i, len, random_payload_size, result;
	char *buff;

	int fd = open(argv[1], O_RDONLY);
	
	// Citeste primul set de date.
	random_payload_size = get_random_number();
	buff = (char*)malloc(random_payload_size);
	len = read(fd, buff, random_payload_size);

	while (1) {
		// Ies din bucla daca nu se mai citeste nimic din fisier.
		if (len == 0) break;
		
		// Curat cadrul de informatii.
		memset(&t, 0, sizeof(msg));
		t.payload[0] = next_frame_to_send;

		// Construiesc cadrul cu structura specificata.
		check_sum = t.payload[0];		
		for (i = 1; i <= len; ++i) {
			t.payload[i] = buff[i - 1];
			check_sum ^= buff[i - 1];
		}
		
		t.payload[len + 1] = check_sum;
		t.len = len + 2;
		log_send(t);
		
		result = send_message(&t);
		if (result < 0) {
			perror("Eroare trimitere.\n");
			return -1;
		}
		log_cron_start();
		response = receive_message_timeout(TIMEOUT);
		
		// Retrimit daca se depaseste timeout sau cadrul pentru care am primit
		// ACK nu este cadrul curent.
		while ((response == NULL) || 
				((*response).payload[0] != next_frame_to_send)){
				
			if (response == NULL) 
				log_timeout(next_frame_to_send);
			else 
				log_wrong(next_frame_to_send);
			
			log_send(t);
			result = send_message(&t);
			if (result < 0) {
				perror("Eroare trimitere.\n");
				return -1;
			}	
			free(response);
			log_cron_start();
			response = receive_message_timeout(TIMEOUT);
		}
		
		log_ack(next_frame_to_send);
		free(response);
		
		random_payload_size = get_random_number();
		free(buff);
		buff = (char*)malloc(random_payload_size);
		len = read(fd, buff, random_payload_size);
		
		++next_frame_to_send;
		
		// Ies din bucla daca ajung la numarul maxim de cadre.
		if (next_frame_to_send == MAXFRAMES) break;
	}
	
	free(buff);
	close(fd);
	return 0;
}

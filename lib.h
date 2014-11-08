#ifndef LIB
#define LIB

#define TYPE1		1
#define TYPE2		2
#define TYPE3		3
#define TYPE4		4
#define ACK_T1		"ACK(TYPE1)"
#define ACK_T2		"ACK(TYPE2)"
#define ACK_T3		"ACK(TYPE3)"

#define MSGSIZE		1400
#define PKTSIZE		1396

// Dimensiunea maxima a payload-ului unui cadru
#define MAXLEN_PAYLOAD 60

// Numarul maxim de cadre ce pot fi trimise
#define MAXFRAMES 255

// Numarul de milisecunde pana la timeout
#define TIMEOUT 5

// Delimitator in fisierul de log 
#define DELIM "--------------------------------------------------------------------------------\n"


// Intoarce reprezentarea in forma binara a parametrului byte, sub forma de
// vector de caractere.
char* getBitString(unsigned char byte) {
	char buffer[8];
	unsigned char mask = 1 << 7;
	int i;
	for (i = 0; i < 8; i++) {
		buffer[i] = ((byte & mask) != 0) + '0';
		mask >>= 1;
	}
	return buffer;
}

// Intoarce un numar aleator intre 1 si 60;
int get_random_number() {
	srand(time(NULL));
	int n = rand() % MAXLEN_PAYLOAD + 1;
	return n;
}

// Intoarce un string cu data si ora sistemului.
char* getTime() {
	time_t time_raw_format;
	struct tm * ptr_time;
	char* time_buffer = (char*)malloc(21);
	
	time(&time_raw_format);
	ptr_time = localtime(&time_raw_format);
	
	if(strftime(time_buffer, 21, "%d-%m-%Y %H:%M:%S", ptr_time) == 0) {
		perror("Eroare formatare.");
		return NULL;
	}
	return time_buffer;
}


typedef struct {
  	int len;
  	char payload[MSGSIZE];
} msg;

typedef struct {
	int type;
	char payload[PKTSIZE];	
} my_pkt;

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout);

#endif


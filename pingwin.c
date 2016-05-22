#include "def.h"

int rank,size,sender=0,receiver=1;
int tt; //czas transportu
int ships; // ile statkow
int ship_snow_cap; // pojemnosc statku
int snowZOO; //ile sniegu dla tego zoo
int actualSnow;// ile jest aktualnie sniegu
int free_ships; // ile jest wolnych statków
int clockT = 0; //zegar tego procesu (chyba skalarny)
int *clock_all;//zegary wszystkich procesow
int needed_ships; //ilosc potrzebnych statkow dla zoo
int *confirmation; //tablica confirmation
pthread_mutex_t mutex; //mutex dla kolejki
pthread_mutex_t mutex_clock; //mutex dla clocka

void print_msg(char *msg) // wyslanie wiadomosci do procesu o rank 0, aby printf'y byly wypisywane w odpowiedniej kolejnosci
{
	int scount = sizeof(msg)/sizeof(msg[0]);
	MPI_Request request;
	MPI_Isend(msg, 255, MPI_CHAR, 0, MSG_TAG_PRINTF, MPI_COMM_WORLD,&request);
}

void local_loop() //petla lokalna, zuzywajaca snieg w zoo
{
	actualSnow = snowZOO;
	while(actualSnow > 0)
	{
		usleep(SIZE_OF_UNIT_TIME*1000);// ile ms * 1000 (funkcja przyjmuje mikro)
		actualSnow--;
	}
}


//petla w sekcji krytycznej, symulujaca czas transportu i rozladunku
void travelAndUnload() 
{
	int r = rand()%MAX_UNLOAD_TIME+1;
	int temp = r+tt;
	temp=temp*SIZE_OF_UNIT_TIME;
	usleep(temp*1000);
}

//wyslanie wiadomosci do wszystkich procesow o odpowiednim tagu, i zawartosci
void send_msg(int tag,int count, int *buf,int source) 
{
	char msg_buf[60];
	sprintf(msg_buf,"%d SEND | %d TAG | clock %d\n",source, tag,buf[0]);
	print_msg(msg_buf);

	int i;
	for(i=1;i<size;i++) //bo 0-wy to master
	{
		if(i!=source)
		{
			MPI_Send(buf,count, MPI_INT, i, tag, MPI_COMM_WORLD);
		}
	}

}

//inicjalizacja "mastera", ktory wylosuje nam wartosci a potem bedzie wypisywal wiadomosci
void master_init(int size)
{
	int Tt = rand()%MAX_TRANSPORT_TIME; // czas transportu (podrozy) wyladunek losujemy przy wyladunku
	int ships = rand()%(MAX_NUMBER_OF_SHIPS-1)+2;
	int snow_ship = rand()%(MAX_SNOW_PER_SHIP-1)+2;
	int requiredSnow[size]; //ile sniegu potrzebuje i-te zoo
	int i;
	int msg[4];
	for(i=1;i<size;i++)
	{
		requiredSnow[i] = rand()%(ships*snow_ship/2-1)+3;
		msg[0] = Tt;
		msg[1] = ships;
		msg[2] = snow_ship;
		msg[3] = requiredSnow[i];
		MPI_Request request;
		MPI_Isend(msg, 4, MPI_INT, i, MSG_HELLO, MPI_COMM_WORLD,&request);
	}
	printf("Czas transportu - %d | Ilość statków - %d | Załadunek statku %d ton |\n", Tt, ships,snow_ship);
	MPI_Barrier(MPI_COMM_WORLD); //bariera inicjalizacyjna


}


//inizjalizacja zoo
void zoo_init()
{

	MPI_Status status;
	int msg[4];
	MPI_Recv(msg, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);//czekanie na wiadomosc od Mastera(ktory losuje czasy)

	tt = msg[0]; //czas transportu
	ships = msg[1]; // ilosc statkow
	ship_snow_cap = msg[2]; // ilosc sniegu na statek
	snowZOO = msg[3]; //ilosc potrzebnego sniegu dla zoo
	actualSnow = snowZOO;
	free_ships = ships; 
	clockT=0; //zegar skalarny


	confirmation = malloc(size * sizeof(int)); //inicjalizacja tablicy confirmation
	int i;
	for(i=0;i<size;i++)
	{
		confirmation[i]=0;
	}

	needed_ships = snowZOO/ship_snow_cap; //obliczenie ilosci potrzebnych statkow dla tego zoo
	if(snowZOO%ship_snow_cap!=0)
		needed_ships++;

	printf("Proces Nr %d | Ilość śniegu dla tego zoo - %d | Ilość statków do śniegu - %d\n", rank, snowZOO,needed_ships);
	
	MPI_Barrier(MPI_COMM_WORLD);//bariera, aby wszystkie zoo sie zainicjalizowaly
}


void master_loop() //petla procesu, ktory wyswietla wiadomosci
{
	while(1){
		char msg[255];
		MPI_Status status;
		MPI_Recv(msg, 255, MPI_INT, MPI_ANY_SOURCE, MSG_TAG_PRINTF, MPI_COMM_WORLD, &status);
		int i = status.MPI_SOURCE;
		printf("%s",msg);
	}
}


int confirmation_sum() //obliczenie sumy ilosci odpowiedzi confirmation
{
	int suma = 0;
	int i;
	for(i =0; i < size; i++)
		suma = suma + confirmation[i];
	return suma;
}


void entry_to_critical_section(int time)
{
	int msg[3];
	int i;
	for(i=0;i<size;i++) //wyzerowanie tablicy confirmation
	{
		confirmation[i]=0;
	}
	confirmation[rank] = 1;
	pthread_mutex_lock(&mutex_clock);
	msg[0] = ++clockT;
	pthread_mutex_unlock(&mutex_clock);
	msg[1] = rank;
	msg[2] = needed_ships;
	
	send_msg(MSG_TAG_REQUEST,3,msg,rank);//wyslanie REQUEST do innych procesow

	pthread_mutex_lock(&mutex);
	add_to_queue(msg[0],msg[1],msg[2]); //dodanie siebie do kolejki
	pthread_mutex_unlock(&mutex);

	while(confirmation_sum() < size-1); //sprawdzenie czy otrzymano CONFIRMATION od innych procesow

	while(1)
	{
		pthread_mutex_lock(&mutex);
		if(how_many_ships(rank) <= free_ships) //sprawdzenie czy sa wolne statki
		{
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);
	}
	char bu[50];
	sprintf(bu,"%d: ENTER CRITICAL SECTION, taking %d ships\n",rank,needed_ships);
	print_msg(bu);
	//print_queue();
	
}


void exit_critical_section(int time)
{
	int msg[3];
	pthread_mutex_lock(&mutex_clock);
	msg[0] = ++clockT;
	pthread_mutex_unlock(&mutex_clock);
	msg[1] = rank;
	msg[2] = needed_ships;
	//usun siebie z kolejki
	
	pthread_mutex_lock(&mutex);
	remove_rank(rank);
	pthread_mutex_unlock(&mutex);

	send_msg(MSG_TAG_RELEASE,3,msg,rank);
	

	char bu[50];
	sprintf(bu,"%d: EXIT CRITICAL SECTION, releasing %d ships\n",rank,needed_ships);
	print_msg(bu);
}


void *zoo_listen(void *arg)
{
	while(1)
	{
		MPI_Status status;
		int msg[3];
		MPI_Recv(msg,3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,MPI_COMM_WORLD, &status);
		char bu[40];
		sprintf(bu,"%d: Otrzymalem wiadomosc o tagu %d, od rank %d, ilosc potrzebnych statkow %d confirmation_sum - %d, size-1 - %d\n",rank, status.MPI_TAG, status.MPI_SOURCE, msg[2],confirmation_sum(),size-1);
		//print_msg(bu);
		int i = status.MPI_TAG;
		pthread_mutex_lock(&mutex_clock);
		if(msg[0] > clockT)
		clockT = msg[0];
		pthread_mutex_unlock(&mutex_clock);

		if( i == MSG_TAG_REQUEST)
		{
			pthread_mutex_lock(&mutex);
			add_to_queue(msg[0],msg[1],msg[2]); //dodanie siebie do kolejki
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex_clock);
			msg[0] = ++clockT;
			pthread_mutex_unlock(&mutex_clock);			
			msg[1] = rank;
			msg[2] = needed_ships;
			MPI_Send(msg,3, MPI_INT, status.MPI_SOURCE, MSG_TAG_CONFIRMATION, MPI_COMM_WORLD); //wyslanie confirmation do procesu co wyslal request
		}

		else if( i == MSG_TAG_CONFIRMATION )
		{
			confirmation[status.MPI_SOURCE]= 1;
		}
		else if( i == MSG_TAG_RELEASE )
		{
			//usuniencie z kolejki
			pthread_mutex_lock(&mutex);
			remove_rank(status.MPI_SOURCE);
			pthread_mutex_unlock(&mutex);
		}
	}
	pthread_exit((void *)NULL);
}
void *zoo_loop(void *arg)
{
	int tr = 0;
	while(1){
		local_loop(); //usuwanie sniegu
		entry_to_critical_section(clockT); // lamport
		travelAndUnload(); // czekanie czasu na rozladunek
		exit_critical_section(clockT); // wyjscie z sekcji krytycznej
	}
	//printf("%d LOOP END\n",rank);
	pthread_exit((void *)NULL);
}
int main( int argc, char **argv )
{
	TAILQ_INIT(&head);
	int master = 0;
	int msg[MSG_SIZE];
	MPI_Status status;
	int level;
	pthread_t thread[2];
	MPI_Init_thread(&argc, &argv,MPI_THREAD_MULTIPLE,&level);
	
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);


	int last=size-1;
	msg[0]=0;
	//printf( "Jestem %d z %d na \n", rank, size );
	if ( rank == master)
	{
		srand(time(NULL)+rank*1000);
		master_init(size);
		master_loop();
	}
	else
	{	
		pthread_mutex_init(&mutex, NULL);
		pthread_mutex_init(&mutex_clock, NULL);
		zoo_init();
		srand(time(NULL)+rank*1000);
		pthread_create(&thread[0], &attr, zoo_listen, (void *)0);		
		pthread_create(&thread[1], &attr, zoo_loop, (void *)1);
		while(1);
	}	
	MPI_Finalize();
}

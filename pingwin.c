#include "def.h"

int rank,size,sender=0,receiver=1;
int tt; //czas transportu
int ships; // ile statkow
int ship_snow_cap; // pojemnosc statku
int snowZOO; //ile sniegu dla tego zoo
int actualSnow;// ile jest aktualnie sniegu
int free_ships; // ile jest wolnych statków
int clockT; //zegar tego procesu (chyba skalarny)
int *clock_all;//zegary wszystkich procesow


void print_msg(char *msg) // wyslanie wiadomosci do rank 0, a potem mozna printfować nim
{
	int count = sizeof(msg)/sizeof(msg[0]);
	MPI_Request request;
	MPI_Isend(msg, 255, MPI_CHAR, 0, MSG_TAG_PRINTF, MPI_COMM_WORLD,&request);
}

void local_loop()
{
	//printf("%d : local enter\n", rank);
	actualSnow = snowZOO;
	while(actualSnow > 0)
	{
		usleep(SIZE_OF_UNIT_TIME*1000);// ile ms * 1000 (funkcja przyjmuje mikro)
		actualSnow--;
	}
}
void travelAndUnload()
{
	char msg_buf[255];
	int r = rand()%MAX_UNLOAD_TIME+1;
	int temp = r+tt;
	temp=temp*SIZE_OF_UNIT_TIME;
	sprintf(msg_buf,"%d : travel %d ms | unload %d ms\n",rank,temp,r*SIZE_OF_UNIT_TIME);
	//print_msg(msg_buf);
	usleep(temp*1000);
	int i = snowZOO/ship_snow_cap;
	if(snowZOO%ship_snow_cap != 0)
	i++;
	msg_buf[0]='\0';
	sprintf(msg_buf,"%d : zwalniam %d shipow\n",rank,i);
	print_msg(msg_buf);
}
void send_msg(int tag,int count, int *buf,int source)
{
	char msg_buf[60];
	
	sprintf(msg_buf,"%d SEND | %d TAG\n",source, tag);
	//print_msg(msg_buf);
	msg_buf[0] = '\0';
	sprintf(msg_buf,"%d : Uaktualniam statki, wolne zostaje = %d\n",rank,buf[1]);
	print_msg(msg_buf);	
	int i;
	MPI_Request request[size-1];
	MPI_Status status[size-1];
	for(i=1;i<size;i++) //bo 0-wy to master, wlasciwie to mozna go tez uzywac jako zoo po init'cie
	{
	if(i!=source){
	char bu[30];
	sprintf(bu,"%d wysylam zajecie %d shipow do %d\n",source,buf[1],i);
	//print_msg(bu);
	//MPI_Isend(buf, count, MPI_INT, i, tag, MPI_COMM_WORLD,&request[i-1]);
	MPI_Send(buf,count, MPI_INT, i, tag, MPI_COMM_WORLD);
	//MPI_Wait(&request[i-1],&status[i-1]);
	}
	}
	//print_msg("wyslane\n");
	//MPI_Waitall(size-1,request,status);
}
void check_and_recive_msg(int tag,int count)//sprawdzenie czy jest jakas wiadomosc o jakims tagu
{
	char msg_buf1[50];
	//sprintf(msg_buf,"%d sprawdzam msg\n",rank);
	//print_msg(msg_buf);
	MPI_Status status;
	int flag = 0;
	MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag,&status);
	MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag,&status);
	//sprintf(msg_buf1,"%d : %d flag\n",rank,flag);
	//print_msg(msg_buf1);
	while(flag){
	//printf("%d RECIEVE | %d tag\n",rank, tag);
	int msg[count];
	int number;
	MPI_Recv(msg, size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	MPI_Get_count(&status, MPI_INT, &number);
	if(tag==MSG_TAG_SHIPS){
	char temp_msg[100];
	sprintf(temp_msg,"%d : Otrzymalem %d wartosci od %d (%d clock | %d value)| SHIPS\n" , rank, number, status.MPI_SOURCE, msg[0],msg[1]);
	//print_msg(temp_msg);

	free_ships = msg[1];
	clock_all[status.MPI_SOURCE]=msg[0];
	}
	int licz = 0;
	flag = 0;
	while(licz++ < 10 && flag == 0)
	MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag,&status); //jezeli jest info o zajetych statkach
	}
}

void critical_loop()
{
	char msg_buf1[100];
	sprintf(msg_buf1,"%d : critical enter\n", rank);
	//print_msg(msg_buf1);
	MPI_Status status;
	int number,flag;
	//MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag,&status); //sprawdza czy jest wiadomosc i zapisuje informacje o niej bez niej
	//MPI_Get_count(&status, MPI_INT, &number); // ile wiadomosci.. chyba
	//msg_buf1[0]='\0';
	//sprintf(msg_buf1,"%d : %d flag %d number - \n",rank,flag, number);
	//print_msg(msg_buf1);
	int queue[2];
	queue[0]=0;
	
	check_and_recive_msg(MSG_TAG_SHIPS,2);
	//if tag == queue
	//przetworz kolejke
	//if tag == ships
	//zaktualizuj ships
	while(free_ships*ship_snow_cap < snowZOO || queue[0] > 1) //to queue pozniej jakos zmienic na dynamiczne mejbi?
	{
	// mpi_recv 
	//przetworzenie
	check_and_recive_msg(MSG_TAG_SHIPS,2);
	//printf("x");
	}
	int temp = free_ships;
	free_ships = free_ships - snowZOO/ship_snow_cap; //zajecie statkow
	if(snowZOO%ship_snow_cap!=0)
		free_ships--;
	int msg[2];
	msg[0]=++clockT;
	msg[1]=free_ships;
	char temp_msg[130];
	temp = temp- free_ships;
	sprintf(temp_msg,"%d : Zajmuje %d statków\n",rank,temp);
	print_msg(temp_msg);
	send_msg(MSG_TAG_SHIPS,2,msg,rank);
	//zwolnij kolejke
	//wyslij wiadomosci z kolejką do wszystkich Isendem o tagu=MSG_TAG_QUEUE
	//
	travelAndUnload();
	
	check_and_recive_msg(MSG_TAG_SHIPS,2);
	msg[0]=++clockT;
	free_ships = free_ships+snowZOO/ship_snow_cap;
	if(snowZOO%ship_snow_cap != 0)
		free_ships++;
	msg[1]=free_ships;
	//flag = 0;
	send_msg(MSG_TAG_SHIPS,2,msg,rank);
	
}


void master_init(int size)
{
	int Tt = rand()%MAX_TRANSPORT_TIME; // czas transportu (podrozy) wyladunek losujemy przy wyladunku
	int ships = rand()%(SHIPS-1)+1;
	int snow_ship = rand()%(MAX_SNOW_PER_SHIP-1)+1;
	int requiredSnow[size]; //ile sniegu potrzebuje i-te zoo
	int i;
	int msg[4];
	for(i=1;i<size;i++)
	{
		requiredSnow[i] = rand()%(ships*snow_ship/2-1)+1;
	//	if(i==1) 
	//	requiredSnow[i]=2;
		msg[0] = Tt;
		msg[1] = ships;
		msg[2] = snow_ship;
		msg[3] = requiredSnow[i];
		MPI_Request request;
		MPI_Isend(msg, 4, MPI_INT, i, MSG_HELLO, MPI_COMM_WORLD,&request);
	}
	//printf("Czas transportu - %d | Ilość statków - %d | Załadunek statku %d ton |\n", Tt, ships,snow_ship);
	
	MPI_Barrier(MPI_COMM_WORLD);

	//printf("%d: Wysylam (%d %d) do %d\n", rank, msg[0], msg[1], sender+1);
	//MPI_Send( msg, MSG_SIZE, MPI_INT, receiver, MSG_HELLO, MPI_COMM_WORLD );

}
void zoo_init()
{
	//printf( "Jestem %d z %d na \n", rank, size );
	MPI_Status status;
	int msg[4];
	MPI_Recv(msg, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	MPI_Get_count( &status, MPI_INT, &size);
	tt = msg[0]; //czas transportu
	ships = msg[1]; // ilosc statkow
	ship_snow_cap = msg[2]; // ilosc sniegu na statek
	snowZOO = msg[3]; //ilosc potrzebnego sniegu dla zoo
	actualSnow = snowZOO;
	free_ships = ships;
	clockT=0;
	clock_all = malloc(size * sizeof(int));
	int i;
	for(i=0;i<size;i++)
	{
	clock_all[i]=0;
	}
	printf("Proces Nr %d | Czas transportu - %d | Ilość statków - %d | Załadunek statku %d ton | Ilość śniegu dla tego zoo - %d\n", rank ,tt, ships,ship_snow_cap, snowZOO);
	//bariera
	MPI_Barrier(MPI_COMM_WORLD);
}
void master_loop()
{
	while(1){
	char msg[255];
	MPI_Status status;
	MPI_Recv(msg, 255, MPI_INT, MPI_ANY_SOURCE, MSG_TAG_PRINTF, MPI_COMM_WORLD, &status);
	int i = status.MPI_SOURCE;
	printf("%s",msg);
	}
}
int main( int argc, char **argv )
{
	
	int master = 0;
	int msg[MSG_SIZE];
	MPI_Status status;

	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	int last=size-1;
	msg[0]=0;
	//printf( "Jestem %d z %d na \n", rank, size );
	if ( rank == master)
	{
		srand(time(NULL)+rank*1000);
		master_init(size);
		master_loop();
	//losuj czas transportu+rozladunku
	//losuj ilosc statkow wieksza niz ilosc zoo
	
	}
	else
	{
		
		srand(time(NULL)+rank*1000);
		//printf( "Jestem %d z %d na \n", rank, size );
		zoo_init();
		int tr = 0;
		while(1)
		{
			local_loop();
			critical_loop();
			//char msg[50];
			//sprintf(msg,"%d %d PRAWIE SKONCZONE\n",rank,tr);
			//print_msg(msg);
		}
		//print_msg("SKONCZONEEEE\n");
	}	
	MPI_Finalize();
}


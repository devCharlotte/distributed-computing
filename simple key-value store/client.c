#include "util.h"

int SERVER_PORT; // 서버 포트번호
const char* dst_ip = "127.0.0.1"; // 하나의 host안에서 통신할 것이므로 서버주소는 localhost(i.e., 127.0.0.1)임

 
// 임의의 key를 생성해서 반환해줌
void generate_key(char* key) {
    uint64_t number = rand() % DATASET_SIZE;
    for (int i = 0; i < 5; ++i) number = ((number << 3) - number + 7) & 0xFFFFFFFFFFFFFFFF;
    key[KEY_SIZE - 1] = '\0';
    for (int i = KEY_SIZE - 2; i >= 0; i--) {
        int index = number % SET_SIZE;
        key[i] = SET[index];
        number /= SET_SIZE;
    }
}

int main(int argc, char *argv[]) {
  // 프로그램 시작시 입력받은 매개변수를 parsing한다.
	if ( argc < 2 ){ // 반드시 포트번호를 입력받아야하므로, argument가 없다면 에러를 띄운다.
	 printf("Input : %s port number\n", argv[0]);
	 return 1;
	}

  srand((unsigned int)time(NULL));  // 난수 발생기 초기화
  /* 서버 구조체 설정 */
	int SERVER_PORT = atoi(argv[1]); // 입력받은 argument를 포트번호 변수에 넣어준다.
	struct sockaddr_in srv_addr; // 패킷을 수신할 서버의 정보를 담을 소켓 구조체를 생성한다.
	memset(&srv_addr, 0, sizeof(srv_addr)); // 구조체를 모두 '0'으로 초기화해준다.
	srv_addr.sin_family = AF_INET; // IPv4를 사용할 것이므로 AF_INET으로 family를 지정한다.
	srv_addr.sin_port = htons(SERVER_PORT); // 서버의 포트번호를 넣어준다. 이 때 htons()를 통해 byte order를 network order로 변환한다.
	inet_pton(AF_INET, dst_ip, &srv_addr.sin_addr);  // 문자열인 IP주소를 바이너리로 변환한 후 소켓 구조체에 저장해준다.

  /* 소켓 생성 */
	int sock; // 소켓 디스크립터(socket descriptor)를 생성한다.
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // socket()으로 IPv4(AF_INET), UDP(SOC_DGRAM)를 사용하는 소켓을 생성 시도한다.
		printf("Could not create socket\n"); // sock으로 return되는 값이 -1이라면 소켓 생성에 실패한 것이다.
		exit(1);
	}

	int n = 0;
  struct KVS RecvMsg={0,}; // 수신용으로 쓸 메시지 구조체 생성 및 초기화
  struct KVS SendMsg={0,}; // 송신용으로 쓸 메시지 구조체 생성 및 초기화

  struct sockaddr_in src_addr; // 패킷을 수신하였을 때, 해당 패킷을 보낸 송신자(Source)의 정보를 저장하기 위한 소켓 구조체
  socklen_t src_addr_len = sizeof(src_addr); // 수신한 패킷의 소켓 구조체 크기를 저장함. IPv4를 사용하므로 sockaddr_in 크기인 16바이트가 저장됨.
  int cnt = 0; // 패킷 5개를 전송한다.
	while(cnt < 5){
    printf("Request ID: %d\n",cnt++);
    SendMsg.type = READ_REQ; // 요청 타입을 읽기로 선언한다.
    /*주의: 문자열의 마지막은 null-terminator \0가 들어가야 한다. 즉, 4바이트 문자열은 ABCD가 아니라 ABC로 보임. 실제 저장된 것은 ABC\0 */
    generate_key(SendMsg.key); // key를 새로 생성한다.

    strncpy(SendMsg.value, "AAAABBBBCCCCDDD", VALUE_SIZE-1); // value필드에 미리 생성해둔 value 값을 복사한다.
    SendMsg.value[VALUE_SIZE - 1] = '\0';  // 명시적으로 널 터미네이터 추가

    // strncpy는 \0을 자동으로 추가하지 않고, strcpy는 \0을 자동으로 추가해준다.
    //strcpy(SendMsg.value,"AAAABBBBCCCCDDD");

    printf("type: READ_REQ Key: %s Value: %s\n",SendMsg.key,SendMsg.value); // 생성한 key와 value를 출력해본다.
		sendto(sock, &SendMsg, sizeof(SendMsg), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr)); // 생성한 메시지를 서버로 송신한다.
    n = recvfrom(sock, &RecvMsg, sizeof(RecvMsg), 0, (struct sockaddr *)&src_addr, &src_addr_len); // 서버로부터 답장을 수신한다.

		if (n > 0) { // 만약 송신한 데이터가 0바이트를 초과한다면 (즉, 1바이트라도 수신했다면)
      //printf("Received bytes: %d, Length: %d\n",n,src_addr_len);
      char* type;
      if(RecvMsg.type == READ_REQ) type ="READ_REQ";
      else if(RecvMsg.type == READ_REP) type ="READ_REP";
      else if(RecvMsg.type == WRITE_REQ) type ="WRITE_REQ";
      else type ="WRITE_REP";
      printf("Type: %s Key: %s Value: %s\n",type,RecvMsg.key, RecvMsg.value); // 수신한 내용을 출력한다.
		}
	}

	close(sock); // 소켓을 닫아준다.
	return 0;
}
